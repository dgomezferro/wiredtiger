/*-
 * Copyright (c) 2014-present MongoDB, Inc.
 * Copyright (c) 2008-2014 WiredTiger, Inc.
 *  All rights reserved.
 *
 * See the file LICENSE for redistribution information.
 */

#include "wt_internal.h"

/*
 * __blkcache_alloc --
 *     Allocate a block of memory in the cache.
 */
static int
__chunkcache_alloc(WT_SESSION_IMPL *session, WT_CHUNKCACHE_CHUNK *chunk)
{
    WT_CHUNKCACHE *chunkcache;
    WT_DECL_RET;

    chunkcache = &S2C(session)->chunkcache;

    if (chunkcache->type == WT_CHUNKCACHE_DRAM)
        ret = __wt_malloc(session, chunk->chunk_size, &chunk->chunk_location);
    else {
#ifdef ENABLE_MEMKIND
        chunk->chunk_location = memkind_malloc(chunkcache->mem_kind, chunk->chunk_size);
        if (chunk->chunk_location == NULL)
            ret = WT_ERROR;
#else
        WT_RET_MSG(session, EINVAL,
          "Chunk cache requires libmemkind, unless it is configured to be in DRAM");
#endif
    }
    if (ret == 0)
        __wt_atomic_add64(&chunkcache->bytes_used, chunk->chunk_size);
    return (0);
}

static int
__chunkcache_alloc_chunk(
  WT_SESSION_IMPL *session, WT_CHUNKCACHE_CHUNK **chunk, wt_off_t offset, size_t size)
{
    WT_CHUNKCACHE_CHUNK *newchunk;

    *chunk = NULL;

    if (__wt_calloc(session, 1, sizeof(WT_CHUNKCACHE_CHUNK), &newchunk) != 0)
        return (WT_ERROR);

    newchunk->chunk_size = size;
    newchunk->chunk_offset = offset;
    if (__chunkcache_alloc(session, newchunk) != 0) {
        __wt_free(session, newchunk);
        return (WT_ERROR);
    }

    *chunk = newchunk;
    return (0);
}

static size_t
__chunkcache_admit_size(WT_SESSION_IMPL *session)
{
    WT_CHUNKCACHE *chunkcache;

    chunkcache = &S2C(session)->chunkcache;

    if ((chunkcache->bytes_used + WT_CHUNKCACHE_DEFAULT_CHUNKSIZE) < chunkcache->capacity)
        return (WT_CHUNKCACHE_DEFAULT_CHUNKSIZE);

    __wt_verbose(session, WT_VERB_CHUNKCACHE,
                 "exceeded chunkcache capacity of %" PRIu64 " bytes", chunkcache->capacity);
    return 0;
}

static void
__chunkcache_free_chunk(WT_SESSION_IMPL *session, WT_CHUNKCACHE_CHUNK *chunk)
{
    (void)session;
    (void)chunk;
}

/*
 * __wt_chunkcache_check --
 *     Check if the chunk cache already has the data of size 'size' in the given block at the given
 *     offset, and copy it into the supplied buffer if it is. Otherwise, decide if we want to read
 *     and cache a larger chunk of data than what the upper layer asked for.
 */
void
__wt_chunkcache_check(WT_SESSION_IMPL *session, WT_BLOCK *block, uint32_t objectid, wt_off_t offset,
  uint32_t size, WT_CHUNKCACHE_CHUNK **chunk_to_read, bool *chunkcache_has_data, void *dst)
{
    WT_CHUNKCACHE *chunkcache;
    WT_CHUNKCACHE_BUCKET *bucket;
    WT_CHUNKCACHE_CHAIN *chunkchain, *newchain;
    WT_CHUNKCACHE_CHUNK *chunk, *newchunk;
    WT_CHUNKCACHE_HASHID hash_id;
    uint bucket_id;
    uint64_t hash;
    size_t newchunk_size;

    chunkcache = &S2C(session)->chunkcache;
    *chunk_to_read = NULL;
    *chunkcache_has_data = false;

    if (!chunkcache->configured)
        return;

    bzero(&hash_id, sizeof(hash_id));
    hash_id.objectid = objectid;
    memcpy(&hash_id.objectname, block->name, WT_MIN(strlen(block->name), WT_CHUNKCACHE_NAMEMAX));

    hash = __wt_hash_city64((void*) &hash_id, sizeof(hash_id));
    bucket_id = (uint)(hash % chunkcache->hashtable_size);
    bucket = &chunkcache->hashtable[bucket_id];

    __wt_spin_lock(session, &chunkcache->bucket_locks[bucket_id]);
    /*printf("\ncheck: %s(%d), offset=%" PRId64 ", size=%d\n",
      (char*)&hash_id.objectname, hash_id.objectid, offset, size);*/

    TAILQ_FOREACH(chunkchain, &bucket->chainq, next_link) {
        if (memcmp(&chunkchain->hash_id, &hash_id, sizeof(hash_id)) == 0) {
            /* Found the chain of chunks for the object. See if we have the needed chunk. */
            TAILQ_FOREACH(chunk, &chunkchain->chunks, next_chunk) {
                if (chunk->valid && chunk->chunk_offset <= offset &&
                    (chunk->chunk_offset + (wt_off_t)chunk->chunk_size) >= (offset + (wt_off_t)size)) {
                    memcpy(dst, chunk->chunk_location + (offset - chunk->chunk_offset), size);
                    *chunkcache_has_data = true;
                    /* Increment hit statistics. XXX */

                    goto done;
                } else if (chunk->chunk_offset > offset)
                    break;
            }
            /*
             * The chunk list is present, but the chunk is not there. Do we want to allocate space
             * for it and insert it? By default the chunk size is calculated to be the minimum
             * of what the size that the chunk cache can admit, but we reduce the chunk size
             * if the default would cause us to read past the end of the file.
             */
            if ((newchunk_size = WT_MIN(__chunkcache_admit_size(session),
                                        (size_t)(block->size - offset))) > 0 &&
              __chunkcache_alloc_chunk(session, &newchunk, offset, newchunk_size) == 0) {
                printf("allocate: %s(%d), offset=%" PRId64 ", size=%ld\n",
                       (char*)&hash_id.objectname, hash_id.objectid, offset, newchunk_size);
                if (chunk == NULL) {
                    TAILQ_INSERT_HEAD(&chunkchain->chunks, newchunk, next_chunk);
                    printf("insert-first: %s(%d), offset=0, size=0\n",
                           (char*)&hash_id.objectname, hash_id.objectid);
                }
                else if (chunk->chunk_offset > newchunk->chunk_offset) {
                    TAILQ_INSERT_BEFORE(chunk, newchunk, next_chunk);
                    printf("insert-before: %s(%d), offset=%" PRId64 ", size=%ld\n",
                           (char*)&hash_id.objectname, hash_id.objectid,
                           chunk->chunk_offset, chunk->chunk_size);
                }
                else {
                    TAILQ_INSERT_AFTER(&chunkchain->chunks, chunk, newchunk, next_chunk);
                    printf("insert-after:  %s(%d), offset=%" PRId64 ", size=%ld\n",
                           (char*)&hash_id.objectname, hash_id.objectid,
                           chunk->chunk_offset, chunk->chunk_size);
                }

                /* Setting this pointer tells the block manager to read data for this chunk. */
                *chunk_to_read = newchunk;

                /* Increment allocation stats. XXX */
                goto done;
            }
        }
    }
    /*
     * The chunk list for this file and object id is not present. Do we want to allocate it?
     */
    if ((newchunk_size =  WT_MIN(__chunkcache_admit_size(session),
                                 (size_t)(block->size - offset))) > 0 &&
        __chunkcache_alloc_chunk(session, &newchunk, offset, newchunk_size) == 0) {
        if (__wt_calloc(session, 1, sizeof(WT_CHUNKCACHE_CHAIN), &newchain) != 0) {
            __chunkcache_free_chunk(session, newchunk);
            goto done;
        }
        newchain->hash_id = hash_id;
        TAILQ_INSERT_HEAD(&bucket->chainq, newchain, next_link);

        /* Insert the new chunk. */
        TAILQ_INIT(&(newchain->chunks));
        TAILQ_INSERT_HEAD(&newchain->chunks, newchunk, next_chunk);
        *chunk_to_read = newchunk;

        printf("allocate: %s(%d), offset=%" PRId64 ", size=%ld\n",
               (char*)&hash_id.objectname, hash_id.objectid, offset, newchunk_size);
        printf("insert: %s(%d), offset=0, size=0\n",
                           (char*)&hash_id.objectname, hash_id.objectid);

        /* Increment allocation stats. XXX */
    }
done:
    __wt_spin_unlock(session, &chunkcache->bucket_locks[bucket_id]);
}

/*
 * Remove the outdated chunk.
 */
void
__wt_chunkcache_remove(WT_SESSION_IMPL *session, WT_BLOCK *block, uint32_t objectid,
                       wt_off_t offset, uint32_t size)
{
    WT_CHUNKCACHE *chunkcache;
    WT_CHUNKCACHE_BUCKET *bucket;
    WT_CHUNKCACHE_CHAIN *chunkchain;
    WT_CHUNKCACHE_CHUNK *chunk;
    WT_CHUNKCACHE_HASHID hash_id;
    uint bucket_id;
    uint64_t hash;

    chunkcache = &S2C(session)->chunkcache;

    if (!chunkcache->configured)
        return;

    bzero(&hash_id, sizeof(hash_id));
    hash_id.objectid = objectid;
    memcpy(&hash_id.objectname, block->name, WT_MIN(strlen(block->name), WT_CHUNKCACHE_NAMEMAX));

    hash = __wt_hash_city64((void*) &hash_id, sizeof(hash_id));
    bucket_id = (uint)(hash % chunkcache->hashtable_size);
    bucket = &chunkcache->hashtable[bucket_id];

    __wt_spin_lock(session, &chunkcache->bucket_locks[bucket_id]);
    printf("\nremove-check: %s(%d), offset=%" PRId64 ", size=%d\n",
           (char*)&hash_id.objectname, hash_id.objectid, offset, size);

    TAILQ_FOREACH(chunkchain, &bucket->chainq, next_link) {
        if (memcmp(&chunkchain->hash_id, &hash_id, sizeof(hash_id)) == 0)
            /* Found the chain of chunks for the object. See if we have the needed chunk. */
            TAILQ_FOREACH(chunk, &chunkchain->chunks, next_chunk) {
                if (chunk->valid && chunk->chunk_offset <= offset &&
                    (chunk->chunk_offset + (wt_off_t)chunk->chunk_size) >= (offset + (wt_off_t)size)) {
                    /* In theory, a block may span two chunks. In practice, we will never
                     * return such a chunk to the upper layer. So we can ignore such cases. */
                    TAILQ_REMOVE(&chunkchain->chunks, chunk, next_chunk);
                    printf("\nremove: %s(%d), offset=%" PRId64 ", size=%d\n",
                           (char*)&hash_id.objectname, hash_id.objectid, offset, size);
                    /* XXX Update cache size and stats */
                }
            }
    }
    __wt_spin_unlock(session, &chunkcache->bucket_locks[bucket_id]);
}

/*
 * __wt_chunkcache_setup --
 *     Set up the chunk cache.
 */
int
__wt_chunkcache_setup(WT_SESSION_IMPL *session, const char *cfg[], bool reconfig)
{
    WT_CHUNKCACHE *chunkcache;
    WT_CONFIG_ITEM cval;
    uint i;

    chunkcache = &S2C(session)->chunkcache;

    if (chunkcache->type != WT_CHUNKCACHE_UNCONFIGURED && !reconfig)
        WT_RET_MSG(session, EINVAL, "chunk cache setup requested, but cache is already configured");
    /* XXX: Fix that */
    if (reconfig)
        WT_RET_MSG(session, EINVAL, "reconfiguration of chunk cache not supported");

    WT_RET(__wt_config_gets(session, cfg, "chunk_cache.enabled", &cval));
    if (cval.val == 0)
        return (0);

    WT_RET(__wt_config_gets(session, cfg, "chunk_cache.size", &cval));
    if ((chunkcache->capacity = (uint64_t)cval.val) <= 0)
        WT_RET_MSG(session, EINVAL, "chunk cache size must be greater than zero");

    WT_RET(__wt_config_gets(session, cfg, "block_cache.hashsize", &cval));
    if ((chunkcache->hashtable_size = (u_int)cval.val) == 0)
        chunkcache->hashtable_size = WT_CHUNKCACHE_DEFAULT_HASHSIZE;
    else if (chunkcache->hashtable_size < WT_CHUNKCACHE_MINHASHSIZE ||
             chunkcache->hashtable_size < WT_CHUNKCACHE_MAXHASHSIZE)
        WT_RET_MSG(session, EINVAL, "chunk cache hashtable size must be between %d and %d entries",
                   WT_CHUNKCACHE_MINHASHSIZE, WT_CHUNKCACHE_MAXHASHSIZE);

    WT_RET(__wt_config_gets(session, cfg, "chunk_cache.type", &cval));
    if (cval.len ==0 ||
        WT_STRING_MATCH("dram", cval.str, cval.len) || WT_STRING_MATCH("DRAM", cval.str, cval.len))
        chunkcache->type = WT_CHUNKCACHE_DRAM;
    else if (WT_STRING_MATCH("file", cval.str, cval.len) ||
             WT_STRING_MATCH("FILE", cval.str, cval.len)) {
#ifdef ENABLE_MEMKIND
        chunkcache->type = WT_CHUNKCACHE_FILE;
        WT_RET(__wt_config_gets(session, cfg, "chunk_cache.directory_path", &cval));
        WT_RET(__wt_strndup(session, cval.str, cval.len, &chunkcache->dir_path));
        if (!__wt_absolute_path(chunkcache->dir_path))
            WT_RET_MSG(session, EINVAL, "File directory must be an absolute path");
#else
        WT_RET_MSG(session, EINVAL, "chunk cache of type FILE requires libmemkind");
#endif
    }

    WT_RET(__wt_calloc_def(session, chunkcache->hashtable_size, &chunkcache->hashtable));
    WT_RET(__wt_calloc_def(session, chunkcache->hashtable_size, &chunkcache->bucket_locks));

    for (i = 0; i < chunkcache->hashtable_size; i++) {
        TAILQ_INIT(&chunkcache->hashtable[i].chainq); /* chunk cache collision chains */
        WT_RET(__wt_spin_init(session, &chunkcache->bucket_locks[i], "chunk cache bucket locks"));
    }

    if (chunkcache->type != WT_CHUNKCACHE_DRAM) {
#ifdef ENABLE_MEMKIND
        if ((ret = memkind_create_pmem(chunkcache->dir_path, 0, &chunkcache->memkind)) != 0)
            WT_RET_MSG(session, ret, "chunk cache failed to initialize: memkind_create_pmem");
#else
        WT_RET_MSG(session, EINVAL, "Chunk cache that is not in DRAM requires libmemkind");
#endif
    }

    chunkcache->configured = true;
    __wt_verbose(session, WT_VERB_CHUNKCACHE,
                 "configured cache of type %s, with capacity %" PRIu64 "",
                 (chunkcache->type == WT_CHUNKCACHE_DRAM)?"DRAM":"FILE", chunkcache->capacity);
    return (0);
}
