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

#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>

#include "azure_connection.h"

TEST_CASE("testing class azure_connection", "azure_connection")
{
    SECTION("List blobs under the test container.") {}

    SECTION("Test object_exists.")
    {
        azure_connection conn = azure_connection("myblobcontainer1", "object_exist_test_prefix");
        bool object_exists = false;

        // Check for a non-existant object in the container.
        REQUIRE(conn.object_exists("test.txt", object_exists) == 0);
        REQUIRE(!object_exists);

        // Check for the newly added object in the container.
        conn.put_object(
          "test.txt", "/home/ubuntu/wiredtiger/ext/storage_sources/azure_store/test.txt");
        REQUIRE(conn.object_exists("test.txt", object_exists) == 0);
        REQUIRE(object_exists);

        // Delete test case object.
        REQUIRE(conn.delete_object("test.txt") == 0);
    }
}
