/* Copyright (C) 2023 - 2026 Gleb Bezborodov - All Rights Reserved
 * You may use, distribute and modify this code under the
 * terms of the MIT license.
 *
 * You should have received a copy of the MIT license with
 * this file. If not, please write to: bezborodoff.gleb@gmail.com, or visit : https://github.com/glensand/jerk-thread
 */

#include <atomic>
#include <cassert>
#include <chrono>
#include <iostream>

#include "hope_thread/runtime/worker_thread.h"

void run_thread_pool_tests();
void run_hash_storage_tests();
void run_async_worker_tests();
void run_seq_lock_tests();
void run_spmc_bounded_message_queue_tests();
void run_spmc_bounded_non_uniform_queue_tests();
void run_interproc_test();
void run_interproc_bounded_non_uniform_queue_test();

int main()
{
    std::cerr << "Running thread pool tests..." << std::endl;
    run_thread_pool_tests();
    std::cerr << "Running hash storage tests..." << std::endl;
    run_hash_storage_tests();
    std::cerr << "Running async worker tests..." << std::endl;
    run_async_worker_tests();
    std::cerr << "Running seq_lock tests..." << std::endl;
    run_seq_lock_tests();
    std::cerr << "Running spmc_bounded_message_queue tests..." << std::endl;
    run_spmc_bounded_message_queue_tests();
    std::cerr << "Running spmc_bounded_non_uniform_queue tests..." << std::endl;
    run_spmc_bounded_non_uniform_queue_tests();
    std::cout << "Running interproc tests..." << std::endl;
    run_interproc_test();
    run_interproc_bounded_non_uniform_queue_test();

    std::cerr << "All tests passed" << std::endl;
    return 0;
}

namespace {
    bool wait_until(const std::function<bool()>& predicate, std::chrono::milliseconds timeout) {
        const auto deadline = std::chrono::steady_clock::now() + timeout;
        while (std::chrono::steady_clock::now() < deadline) {
            if (predicate()) {
                return true;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        return predicate();
    }
}

void run_async_worker_tests() {
    {
        std::atomic<int> processed{ 0 };
        std::atomic<int> sum{ 0 };

        auto worker = hope::threading::async_worker_impl(
            [&processed, &sum](int&& value) {
                processed.fetch_add(1, std::memory_order_relaxed);
                sum.fetch_add(value, std::memory_order_relaxed);
            }, int{}
        );

        constexpr int items = 500;
        for (int i = 1; i <= items; ++i) {
            worker.add(i);
        }

        const bool all_processed = wait_until([&processed] {
            return processed.load(std::memory_order_relaxed) == items;
        }, std::chrono::milliseconds(2000));

        assert(all_processed);
        assert(sum.load(std::memory_order_relaxed) == (items * (items + 1)) / 2);
        worker.stop();
    }

    {
        std::atomic<int> executed{ 0 };

        auto worker = hope::threading::async_worker_impl(
            [&executed](hope::threading::worker_function_t&& task) {
                task();
                executed.fetch_add(1, std::memory_order_relaxed);
            }, hope::threading::worker_function_t{}
        );

        constexpr int tasks = 128;
        for (int i = 0; i < tasks; ++i) {
            worker.add([&executed] {
                // Count inside task so we verify task execution independently.
                executed.fetch_add(1, std::memory_order_relaxed);
            });
        }

        const bool all_executed = wait_until([&executed] {
            return executed.load(std::memory_order_relaxed) >= tasks * 2;
        }, std::chrono::milliseconds(2000));

        assert(all_executed);
        worker.stop();
        worker.stop();
    }
}
