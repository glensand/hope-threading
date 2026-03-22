/* Copyright (C) 2026 Gleb Bezborodov - All Rights Reserved
 * You may use, distribute and modify this code under the
 * terms of the MIT license.
 *
 * You should have received a copy of the MIT license with
 * this file. If not, please write to: bezborodoff.gleb@gmail.com, or visit : https://github.com/glensand/hope-threading
 */

#include <cassert>
#include <atomic>
#include <thread>

#include "hope_thread/synchronization/seq_lock.h"

namespace {

struct pod_pair {
    int a;
    int b;
};

} // namespace

void run_seq_lock_tests()
{
    {
        hope::threading::seq_lock<int> lock;
        int v = -1;
        lock.store(7);
        assert(lock.load(v));
        assert(v == 7);
        lock.store(42);
        assert(lock.load(v));
        assert(v == 42);
    }

    {
        hope::threading::seq_lock<pod_pair> lock;
        pod_pair read{};
        lock.store(pod_pair{ 1, 2 });
        assert(lock.load(read));
        assert(read.a == 1 && read.b == 2);
        lock.store(pod_pair{ 3, 4 });
        assert(lock.load(read));
        assert(read.a == 3 && read.b == 4);
    }

    {
        hope::threading::seq_lock<int> lock;
        std::atomic<bool> writer_active{ true };
        std::atomic<int> successful_loads{ 0 };

        std::thread writer([&] {
            for (int i = 0; i < 100'000; ++i) {
                lock.store(i * 2);
            }
            writer_active.store(false, std::memory_order_release);
        });

        std::thread reader([&] {
            while (writer_active.load(std::memory_order_acquire)) {
                int v = 0;
                if (lock.load(v)) {
                    assert(v % 2 == 0);
                    assert(v >= 0 && v < 200'000);
                    successful_loads.fetch_add(1, std::memory_order_relaxed);
                }
            }
        });

        writer.join();
        reader.join();
        assert(successful_loads.load(std::memory_order_relaxed) > 100);
    }
}
