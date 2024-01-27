/* Copyright (C) 2023 - 2024 Gleb Bezborodov - All Rights Reserved
 * You may use, distribute and modify this code under the
 * terms of the MIT license.
 *
 * You should have received a copy of the MIT license with
 * this file. If not, please write to: bezborodoff.gleb@gmail.com, or visit : https://github.com/glensand/hope-threading
 */

#include "gtest/gtest.h"
#include "hope_thread/runtime/threadpool.h"
#include "hope_thread/runtime/worker_thread.h"

TEST(ThreadPoolTest, JustAddTasks)
{
    hope::threading::thread_pool pool(16);
    for (std::size_t i{ 0 }; i < 1000; ++i)
        pool.add_work([=]{
            std::cout << "Hello " << i << '\n';
        });
    pool.destroy();
}
