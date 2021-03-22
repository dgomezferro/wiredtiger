/*-
 * Copyright (c) 2014-2020 MongoDB, Inc.
 * Copyright (c) 2008-2014 WiredTiger, Inc.
 *	All rights reserved.
 *
 * See the file LICENSE for redistribution information.
 */

/*
 * WiredTiger's block cache. It is used to cache blocks identical to those that live on disk in a
 * faster storage medium, such as NVRAM.
 */

#ifdef HAVE_LIBMEMKIND
#include <memkind.h>
#endif /* HAVE_LIBMEMKIND */

#define BLKCACHE_FREQ_TARGET_INCREMENT 10
#define BLKCACHE_MAX_FREQUENCY_TARGET 100
#define BLKCACHE_MAX_RECENCY_TARGET 5
#define BLKCACHE_REC_TARGET_INCREMENT 1

#define BLKCACHE_HASHSIZE_DEFAULT 32768
#define BLKCACHE_HASHSIZE_MIN 512
#define BLKCACHE_HASHSIZE_MAX 1024*1024*1024

#define BLKCACHE_OVERHEAD_THRESHOLD 0.1

#define BLKCACHE_TRACE 0

#define WT_BLKCACHE_FULL   -2
#define WT_BLKCACHE_BYPASS -3

/*
 * WT_BLKCACHE_ID --
 *    Checksum, offset and size uniquely identify a block.
 *    These are the same items used to compute the cookie.
 */
struct __wt_blkcache_id {
    uint64_t checksum;
    uint64_t offset;
    uint64_t size;
};

/*
 * WT_BLKCACHE_ITEM --
 *     Block cache item. It links with other items in the same hash bucket.
 */
struct __wt_blkcache_item {

    struct __wt_blkcache_id id;
    TAILQ_ENTRY(__wt_blkcache_item) hashq;
    void *data;
    uint32_t num_references;
    /*
     * The virtual recency timestamp is incremented every time the block is
     * referenced, but saturates at the set threshold. It is decremented every
     * time the eviction thread scans the cache.
     */
    uint32_t virtual_recency_timestamp;
};

/*
 * WT_BLKCACHE --
 *     Block cache metadata includes the hashtable of cached items, number
 *     of cached data blocks and the total amount of space they occupy.
 */
struct __wt_blkcache {
    /* Locked: Block manager cache. Locks are per-bucket. */
    TAILQ_HEAD(__wt_blkcache_hash, __wt_blkcache_item) * hash;
    WT_SPINLOCK *hash_locks;
    WT_CONDVAR *blkcache_cond;
    wt_thread_t evict_thread_tid;

    volatile bool blkcache_exiting;
    bool write_allocate;
    char *nvram_device_path;
    double full_target;
    double overhead_pct;
    float fraction_in_dram;
    int refs_since_filesize_estimated;
    int type;
    volatile size_t bytes_used;
    size_t estimated_file_size;
    size_t hash_size;
    size_t num_data_blocks;
    size_t max_bytes;
    size_t system_ram;

    /*
     * Various metrics helping us measure the overhead and
     * decide if to bypass the cache.
     * We access some of them without synchronization despite races.
     * These serve as heuristics, and we don't need precise values
     * for them to be useful. If, because of races, we lose updates
     * of these values, assuming that we lose them at the same
     * rate for all variables, the ratio should remain roughly
     * accurate. We care about the ratio.
     */
    size_t lookups;
    size_t inserts;
    size_t removals;

#ifdef HAVE_LIBMEMKIND
    struct memkind *pmem_kind;
#endif /* HAVE_LIBMEMKIND */

    /* Histograms keeping track of number of references to each block */
#define BLKCACHE_HIST_BUCKETS 11
#define BLKCACHE_HIST_BOUNDARY 1
    uint32_t cache_references[BLKCACHE_HIST_BUCKETS];
};

#define BLKCACHE_UNCONFIGURED 0
#define BLKCACHE_DRAM 1
#define BLKCACHE_NVRAM 2

#define BLKCACHE_PERCENT_FILE_IN_DRAM 50
