/* Copyright (C) 2023 Gleb Bezborodov - All Rights Reserved
 * You may use, distribute and modify this code under the
 * terms of the MIT license.
 *
 * You should have received a copy of the MIT license with
 * this file. If not, please write to: bezborodoff.gleb@gmail.com, or visit : https://github.com/glensand/jerk-thread
 */

#include <iostream>
#include <mutex>
#include <shared_mutex>
#include <atomic>
#include <vector>

#include "jerk-thread/synchronization/spinlock.h"
#include "jerk-thread/synchronization/object_safe_wrapper.h"

int main() {

    jt::object_safe_wrapper<std::vector<int>, jt::recursive_rw_spinlock,
        std::unique_lock, std::shared_lock> safevector;

    std::vector<std::thread> threads;
    for (std::size_t i{ 0 }; i < 10; ++i) {
        threads.emplace_back([i, &safevector] {
            for (std::size_t j{ 0 }; j < 1000; ++j)
                safevector->push_back((int)i);
        });
    }

    for (std::size_t i { 0 }; i < 10; ++i) {
        const auto& cv = safevector;
        threads.emplace_back([&cv] {
            const std::unique_lock lock(cv);
            const std::shared_lock lock2(cv);
            const std::shared_lock lock3(cv);
            for (auto&& v : cv.raw())
                std::cout << v << '\n';
        });
    }

/*    safevector.lock();
    safevector.unlock();

    safevector.lock_shared();
    safevector.unlock_shared();

    auto&& begin = safevector->cbegin();

    safevector.lock();
    safevector.lock_shared();
    safevector.lock_shared();
    safevector.unlock_shared();
    safevector.unlock_shared();
    safevector.unlock();*/

    for (auto&& t : threads)
        t.join();

    volatile int a = 1;
} 