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

#ifndef DATABASE_OPERATION_H
#define DATABASE_OPERATION_H

#include "database.h"
#include "thread_worker.h"

namespace test_harness {
class DatabaseOperation {
    public:
    /*
     * Function that performs the following steps using the configuration that is defined by the
     * test:
     *  - Creates N collections as per the configuration.
     *  - Creates M threads as per the configuration, each thread will:
     *      - Open a cursor on each collection.
     *      - Insert K key/value pairs in each collection. Values are random strings which size is
     * defined by the configuration.
     */
    virtual void Populate(Database &database, TimestampManager *timestampManager,
      Configuration *config, OperationTracker *operationTracker);

    /* Performs a checkpoint periodically. */
    virtual void CheckpointOperation(thread_worker *threadWorker);

    /* Custom operation without a default implementation. */
    virtual void CustomOperation(thread_worker *threadWorker);

    /* Basic insert operation that adds a new key every rate tick. */
    virtual void InsertOperation(thread_worker *threadWorker);

    /* Basic read operation that chooses a random collection and walks a cursor. */
    virtual void ReadOperation(thread_worker *threadWorker);

    /* Basic remove operation that chooses a random key and deletes it. */
    virtual void RemoveOperation(thread_worker *threadWorker);

    /* Basic update operation that chooses a random key and updates it. */
    virtual void UpdateOperation(thread_worker *threadWorker);

    virtual void Validate(const std::string &operationTableName, const std::string &schemaTableName,
      const std::vector<uint64_t> &knownCollectionIds);

    virtual ~DatabaseOperation() = default;
};
} // namespace test_harness
#endif
