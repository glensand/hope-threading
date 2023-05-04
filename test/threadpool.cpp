/* Copyright (C) 2023 Gleb Bezborodov - All Rights Reserved
 * You may use, distribute and modify this code under the
 * terms of the MIT license.
 *
 * You should have received a copy of the MIT license with
 * this file. If not, please write to: bezborodoff.gleb@gmail.com, or visit : https://github.com/glensand/jerk-thread
 */

#include "gtest/gtest.h"
#include "jerk-thread/runtime/threadpool.h"
#include "jerk-thread/runtime/worker_thread.h"

TEST(ThreadPoolTest, JustAddTasks)
{
    jt::thread_pool pool(16);
    for (std::size_t i{ 0 }; i < 1000; ++i)
        pool.add_work([=]{
            std::cout << "Suck " << i << '\n';
        });
    pool.destroy();
}
