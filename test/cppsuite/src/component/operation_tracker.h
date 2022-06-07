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

#ifndef OPERATION_TRACKER_H
#define OPERATION_TRACKER_H

#include "component.h"
#include "src/storage/scoped_cursor.h"
#include "src/storage/scoped_session.h"
#include "timestamp_manager.h"

/*
 * Default schema for tracking operations on collections (key_format: Collection id / Key /
 * Timestamp, value_format: Operation type / Value)
 */
#define OPERATION_TRACKING_KEY_FORMAT WT_UNCHECKED_STRING(QSQ)
#define OPERATION_TRACKING_VALUE_FORMAT WT_UNCHECKED_STRING(iS)

/*
 * Default schema for tracking schema operations on collections (key_format: Collection id /
 * Timestamp, value_format: Operation type)
 */
#define SCHEMA_TRACKING_KEY_FORMAT WT_UNCHECKED_STRING(QQ)
#define SCHEMA_TRACKING_VALUE_FORMAT WT_UNCHECKED_STRING(i)
#define SCHEMA_TRACKING_TABLE_CONFIG                                                       \
    "key_format=" SCHEMA_TRACKING_KEY_FORMAT ",value_format=" SCHEMA_TRACKING_VALUE_FORMAT \
    ",log=(enabled=true)"

namespace test_harness {
/* Tracking operations. */
enum class TrackingOperation { kCreateCollection, kCustom, kDeleteCollection, kDeleteKey, kInsert };

/* Class used to track operations performed on collections */
class OperationTracker : public Component {
    public:
    OperationTracker(Configuration *_config, const bool useCompression, TimestampManager &tsm);
    virtual ~OperationTracker() = default;

    const std::string &getSchemaTableName() const;
    const std::string &getOperationTableName() const;
    void Load() override final;

    /*
     * As every operation is tracked in the tracking table we need to clear out obsolete operations
     * otherwise the file size grow continuously, as such we cleanup operations that are no longer
     * relevant, i.e. older than the oldest timestamp.
     */
    void DoWork() override final;

    void saveSchemaOperation(
      const TrackingOperation &operation, const uint64_t &collectionId, wt_timestamp_t timestamp);

    virtual void setTrackingCursor(const uint64_t transactionId, const TrackingOperation &operation,
      const uint64_t &collectionId, const std::string &key, const std::string &value,
      wt_timestamp_t timestamp, ScopedCursor &opTrackingCursor);

    int save_operation(const uint64_t transactionId, const TrackingOperation &operation,
      const uint64_t &collectionId, const std::string &key, const std::string &value,
      wt_timestamp_t timestamp, ScopedCursor &opTrackingCursor);

    private:
    ScopedSession _session;
    ScopedSession _sweepSession;
    ScopedCursor _schemaTrackingCursor;
    ScopedCursor _sweepCursor;
    std::string _operationTableConfig;
    const std::string _operationTableName;
    const std::string _schemaTableConfig;
    const std::string _schemaTableName;
    const bool _useCompression;
    TimestampManager &_timestampManager;
};
} // namespace test_harness

#endif
