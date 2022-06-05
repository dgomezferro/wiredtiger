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

#ifndef TEST_H
#define TEST_H

#include <string>

#include "database_operation.h"
#include "src/component/metrics_monitor.h"
#include "src/component/workload_manager.h"
#include "src/storage/connection_manager.h"

namespace test_harness {
struct test_args {
    const std::string testConfig;
    const std::string testName;
    const std::string wtOpenConfig;
};

/* The base class for a test, the standard usage pattern is to just call run(). */
class Test : public DatabaseOperation {
    public:
    explicit Test(const test_args &args);
    virtual ~Test();

    /* Delete the copy constructor and the assignment operator. */
    Test(const Test &) = delete;
    Test &operator=(const Test &) = delete;

    /* Initialize the operation tracker component and its dependencies. */
    void InitOperationTracker(OperationTracker *operation_tracker = nullptr);

    /* The primary run function that most tests will be able to utilize without much other code. */
    virtual void Run();

    protected:
    const test_args &_args;
    Configuration *_config;
    TimestampManager *_timestampManager = nullptr;
    OperationTracker *_operationTracker = nullptr;

    private:
    std::vector<Component *> _components;
    MetricsMonitor *_metricsMonitor = nullptr;
    ThreadManager *_threadManager = nullptr;
    WorkloadManager *_workloadManager = nullptr;
    Database _database;
};
} // namespace test_harness

#endif