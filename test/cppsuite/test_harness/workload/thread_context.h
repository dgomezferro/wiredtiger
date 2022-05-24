/*-
 * Public Domain 2014-present MongoDB, Inc.
 * Public Domain 2008-2014 WiredTiger, Inc.
 *
 * This is free and unencumbered software released into the public domain.
 *
 * Anyone is free to copy, modify, publish, use, compile, sell, or
 * distribute this software, either in source code form or as a compiled
 * binary, for any purpose, commercial or non-commercial, and by any
 * means.
 *
 * In jurisdictions that recognize copyright laws, the author or authors
 * of this software dedicate any and all copyright interest in the
 * software to the public domain. We make this dedication for the benefit
 * of the public at large and to the detriment of our heirs and
 * successors. We intend this dedication to be an overt act of
 * relinquishment in perpetuity of all present and future rights to this
 * software under copyright law.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef THREAD_CONTEXT_H
#define THREAD_CONTEXT_H

#include <string>

extern "C" {
#include "test_util.h"
}

#include "../workload/database_model.h"
#include "transaction_context.h"

/* Forward declarations for classes to reduce compilation time and modules coupling. */
class configuration;
class timestamp_manager;
class workload_tracking;

namespace test_harness {
enum thread_type { CHECKPOINT, CUSTOM, INSERT, READ, REMOVE, UPDATE };

const std::string type_string(thread_type type);

/* Container class for a thread and any data types it may need to interact with the database. */
class thread_context {
    public:
    thread_context(uint64_t id, thread_type type, configuration *config,
      scoped_session &&created_session, timestamp_manager *timestamp_manager,
      workload_tracking *tracking, database &dbase);

    virtual ~thread_context() = default;

    void finish();

    /* If the value's size is less than the given size, padding of '0' is added to the value. */
    std::string pad_string(const std::string &value, uint64_t size);

    /*
     * Generic update function, takes a collection_id, key and value.
     *
     * Return true if the operation was successful, a return value of false implies the transaction
     * needs to be rolled back.
     */
    bool update(scoped_cursor &cursor, uint64_t collection_id, const std::string &key,
      const std::string &value);

    /*
     * Generic insert function, takes a collection_id, key and value.
     *
     * Return true if the operation was successful, a return value of false implies the transaction
     * needs to be rolled back.
     */
    bool insert(scoped_cursor &cursor, uint64_t collection_id, const std::string &key,
      const std::string &value);

    /*
     * Generic remove function, takes a collection_id and key and will delete the key if it exists.
     *
     * Return true if the operation was successful, a return value of false implies the transaction
     * needs to be rolled back.
     */
    bool remove(scoped_cursor &cursor, uint64_t collection_id, const std::string &key);
    void sleep();
    bool running() const;

    public:
    const int64_t collection_count;
    const int64_t key_count;
    const int64_t key_size;
    const int64_t value_size;
    const int64_t thread_count;
    const thread_type type;
    const uint64_t id;
    database &db;
    scoped_session session;
    scoped_cursor op_track_cursor;
    scoped_cursor stat_cursor;
    timestamp_manager *tsm;
    transaction_context transaction;
    workload_tracking *tracking;

    private:
    bool _running = true;
    uint64_t _sleeping_time_ms = 1000;
};
} // namespace test_harness

#endif
