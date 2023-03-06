/* Copyright (C) 2023 Gleb Bezborodov - All Rights Reserved
 * You may use, distribute and modify this code under the
 * terms of the MIT license.
 *
 * You should have received a copy of the MIT license with
 * this file. If not, please write to: bezborodoff.gleb@gmail.com, or visit : https://github.com/glensand/jerk-thread
 */

#include <iostream>
#include <mutex>
#include <atomic>

void lock_exclusive(std::recursive_mutex& m) {
    m.lock();
}

void unlock_exclusive(std::recursive_mutex& m) {
    m.unlock();
}

#include "jerk-thread/synchronization/object_safe_wrapper.h"

struct object final{
    int lol;
};

int main() {
    object_safe_wrapper<object, std::recursive_mutex, 
        std::unique_lock<std::recursive_mutex>> safe;

    safe.lock();
    safe.unlock();

    std::atomic<uint64_t> atomic;
    std::cout << atomic.is_lock_free() << std::endl;
} 