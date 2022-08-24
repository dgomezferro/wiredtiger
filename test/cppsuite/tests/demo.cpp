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

#include "src/common/constants.h"
#include "src/common/logger.h"
#include "src/main/test.h"

namespace test_harness {
/* Defines what data is written to the tracking table for use in custom validation. */
class operation_tracker_demo : public operation_tracker {

    public:
    operation_tracker_demo(
      configuration *config, const bool use_compression, timestamp_manager &tsm)
        : operation_tracker(config, use_compression, tsm)
    {
    }

    void
    set_tracking_cursor(WT_SESSION *session, const tracking_operation &operation,
      const uint64_t &collection_id, const std::string &key, const std::string &value,
      wt_timestamp_t ts, scoped_cursor &op_track_cursor) override final
    {
        /*
         * Set the key and value of the cursor used on the tracking table:
         *  The key is a timestamp.
         *  The value is the operation and the table name.
         */
        op_track_cursor->set_key(op_track_cursor.get(), ts);
        op_track_cursor->set_value(op_track_cursor.get(), operation, value.c_str());
    }
};

/*
 * Class that defines operations that do nothing as an example. This shows how database operations
 * can be overridden and customized.
 */
class demo : public test {
    public:
    demo(const test_args &args) : test(args)
    {
        init_operation_tracker(new operation_tracker_demo(_config->get_subconfig(OPERATION_TRACKER),
          _config->get_bool(COMPRESSION_ENABLED), *_timestamp_manager));
    }

    /*
     * Remove the unecessary:
     *  Anything that does not need to be customised can be removed from this class. When removed,
     * the default implementation is used. The function `run` can removed and probably a few more :)
     */
    void
    run() override final
    {
        /* You can remove the call to the base class to fully customize your test. */
        test::run();
    }

    void
    populate(database &, timestamp_manager *, configuration *, operation_tracker *) override final
    {
        logger::log_msg(LOG_WARN, "populate: nothing done");
    }

    void
    checkpoint_operation(thread_worker *) override final
    {
        logger::log_msg(LOG_WARN, "checkpoint_operation: nothing done");
    }

    void
    custom_operation(thread_worker *tw) override final
    {
        /* Define the length of the table name. */
        // TODO

        /* While the test is running. */
        while (tw->running()) {

            /*
             * Generate a table name:
             *  Using the `random_generator` class, generate a name that has the right number of
             * characters.
             */
            // TODO

            /*
             * Create the new table:
             *  The `session` object contained in the `thread_worker` object is a C++ object called
             * `scoped_session` that wraps around the C type `WT_SESSION`. It is possible to access
             * the underlying C type using the `->` or `get()`. Once you have access to the
             * `WT_SESSION`, simply use the `create` function.
             *  Note that the C API uses `const char*` and not `string`, use the `c_str()` function to do the conversion.
             */
            if (tw->session->create(tw->session.get(), <table_name>, NULL) != 0)
                /*
                 * Failure, log some error:
                 *  Again, using the `logger` class, generate some trace to indicate an error.
                 */
                // TODO
            else {
                /*
                 * Success, log some debugging trace:
                 *  It is also great to add traces when things go right. Use the severity level
                 * `LOG_TRACE` to indicate a new table has been created. Make sure the table name
                 * appears in the message.
                 */
                // TODO

                /*
                 * TRACKING SECTION: BEGIN.
                 *   This part requires a good knowledge of the framework, everything has been
                 * coded.
                 */

                /* Unused but needs to be declared for later use. */
                const uint64_t collection_id = 0;
                const std::string key = "";

                /*
                 * Set the key/value pair:
                 *   We are generate a timestamp using the `timestamp_manager` and the `get_next_ts`
                 * function.
                 */
                const wt_timestamp_t ts = tw->tsm->get_next_ts();
                const std::string value = <table_name>;

                /* Start a transaction. */
                tw->txn.begin();

                /*
                 * Track the operation:
                 *  To track an operation, we can use the `op_tracker` object contained in the
                 * `thread_worker`. The `op_tracker` is an object of the `operation_tracker` class
                 * that gives access to the `save_operation` function. It takes a predefined set of
                 * arguments that are required to track anything the user wants.
                 */
                int ret = tw->op_tracker->save_operation(tw->session.get(),
                  tracking_operation::CUSTOM, collection_id, key, value, ts, tw->op_track_cursor);

                /* Commit if successful. */
                if (ret == 0)
                    testutil_assert(tw->txn.commit());
                /* Otherwise, handle error. */
                else {
                    logger::log_msg(
                      LOG_ERROR, "Custom operation could not be saved for table: " + <table_name>);
                    tw->txn.rollback();
                }
                /* TRACKING SECTION: END. */
            }

            /* Give the system a break. */
            tw->sleep();
        }
    }

    void
    insert_operation(thread_worker *tw) override final
    {
        /*
         * Retrieve the number of collections created during the populate phase:
         *  The `thread_worker` object has access to the database through its `db` field.
         *  The method `get_collection_count()` retrieves the number of collections currently in the
         * database.
         */
        // TODO

        /*
         * Make sure we have at least a collection to work on:
         *  The macro `testutil_assert()` could be useful.
         */
        // TODO

        /*
         * Retrieve a random collection:
         *  Using the database object again, this can be done through the function
         * `get_random_collection()`. It returns a *reference* to a random collection from the
         * database.
         */
        // TODO

        /*
         * Open a cursor on the collection:
         *  The framework can declare c++ cursors using the `scoped_cursor` class. To create one,
         * use the `open_scoped_cursor()` function accessible through the `session` object contained
         * within the `thread_worker` object.
         */
        // TODO

        /* While the test is running. */
        while (tw->running()) {

            /*
             * Generate a random key/value pair:
             *  For this, we can use our `random_generator` class which is a singleton. To access
             * it, call `random_generator::instance()` and use the function of your choice. Note
             * that the framework only handles string for keys and values.
             */
            // TODO

            /*
             * Start a txn if not started yet:
             *  The `thread_worker` object has a `txn` object that represents a transaction. We can
             * start a transaction by calling `begin` but since we are in a loop, we want
             * `try_begin` that starts the transaction if it is not active yet.
             * This will make more sense when we will get to the commit part.
             */
            // TODO

            /*
             * Perform the insertion:
             *  The `thread_worker` object can call the `insert` function to perform an insertion.
             *  Note that it returns a boolean.
             */
            // TODO
            if () {
                /*
                 * Success, try to commit:
                 *  We can try to commit using the `can_commit` function of the `txn` object. The
                 * `can_commit` function checks if we have done enough operations within the current
                 * transaction.
                 *  The function `can_commit` returns a boolean. If this is `true`, the transaction
                 * is ready to be committed and this can be done using the `commit` function. We can ignore the result of the `commit` function using the macro `WT_IGNORE_RET_BOOL`.
                 */
                // TODO
            }
            else {
                /*
                * Failure, handle error:
                *  The insertion might fail and `insert` returns `false` if this is the case. It is the
                * right place to handle the error. Call the `rollback` function using the `txn` object.
                * It would also be great to log some traces and this can be done using the `logger`
                * class. A message can be generated using `logger::log_msg` and then the severity level
                * needs to be specified as well as the message. The different levels are defined in
                * `logger.h`.
                */
                logger::log_msg(LOG_ERROR, "Insertions failed.");
                // TODO
            }

            /*
             * Give the system a break:
             *  In order to pace our operation, we can take a break here. Using the `sleep` function
             * of the `thread_worker` object, the thread will sleep the amount of time defined in
             * the configuration.
             */
            tw->sleep();
        }

        /*
         * Cancel any active transaction:
         *  The test is now over. Make sure there is no running transaction using the `try_rollback`
         * function using the `txn` object.
         */
        tw->txn.try_rollback();
    }

    void
    read_operation(thread_worker *) override final
    {
        logger::log_msg(LOG_WARN, "read_operation: nothing done");
    }

    void
    remove_operation(thread_worker *) override final
    {
        logger::log_msg(LOG_WARN, "remove_operation: nothing done");
    }

    void
    update_operation(thread_worker *) override final
    {
        logger::log_msg(LOG_WARN, "update_operation: nothing done");
    }

    void
    validate(
      const std::string &operation_table_name, const std::string &, database &) override final
    {
        /* Open a new session:
         *  Use the `connection_manager` class to create a new session of type `scoped_session`. The
         * function to create a new `scoped_session` is
         * `connection_manager::instance().create_session()`.
         */
        // TODO

        /* Open a new cursor:
         *  Use the newly created `scoped_session` to create a `scoped_cursor` using the
         * `open_scoped_cursor` function.
         */
        // TODO

        /*
         * We will need to keep track of some return values and the number of collections, we can
         * declare them here.
         */
        int ret;
        uint64_t created_tables_num = 0;

        /*
         * Read the tracking table:
         *  In order to read the tracking table, we can simply use the `next` function of our
         * cursor. Similarly to the `scoped_session`, in order to have access to the underlying
         * `WT_CURSOR` of a `scoped_cursor`, use the `->` or `get()`.
         */
        while ((ret = <cursor>->next(<cursor>.get())) == 0) {

            /* Components stored in the tracking table, */
            uint64_t tracked_ts;
            char *tracked_table_name;
            uint64_t tracked_op_type;

            testutil_check(<cursor>->get_key(<cursor>.get(), &tracked_ts));
            testutil_check(<cursor>->get_value(<cursor>.get(), &tracked_op_type, &tracked_table_name));

            /* We are only looking for records from the custom_operation() function. */
            tracking_operation op_type = static_cast<tracking_operation>(tracked_op_type);
            if (op_type != tracking_operation::CUSTOM)
                continue;

            /* Print some trace. */
            logger::log_msg(LOG_TRACE,
              "Timestamp: " + std::to_string(tracked_ts) +
                ", table name: " + std::string(tracked_table_name));

            /* +1 record! */
            ++created_tables_num;
        }

        /* We expect at least 1 table to be created during the test. */
        testutil_assert(created_tables_num > 0);
    }
};

} // namespace test_harness
