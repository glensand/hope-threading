/* Copyright (C) 2023 - 2026 Gleb Bezborodov - All Rights Reserved
 * You may use, distribute and modify this code under the
 * terms of the MIT license.
 *
 * You should have received a copy of the MIT license with
 * this file. If not, please write to: bezborodoff.gleb@gmail.com, or visit : https://github.com/glensand/hope-threading
 */

#include <cassert>
#include <atomic>
#include <thread>

#include "hope_thread/containers/queue/spmc_bounded_message_queue.h"

void run_spmc_bounded_message_queue_tests()
{
    constexpr std::size_t k_capacity = 1024;
    using queue_t = hope::threading::spmc_bounded_message_queue<int, k_capacity>;

    {
        queue_t q;
        auto c = q.create_consumer();
        int v = 0;
        assert(!c.try_dequeue(v));
        assert(q.try_enqueue(10));
        assert(c.try_dequeue(v) && v == 10);
        assert(!c.try_dequeue(v));
    }

    {
        queue_t q;
        auto c = q.create_consumer();
        for (int i = 0; i < 5; ++i) {
            assert(q.try_enqueue(i));
        }
        for (int i = 0; i < 5; ++i) {
            int v = -1;
            assert(c.try_dequeue(v));
            assert(v == i);
        }
        int v = -1;
        assert(!c.try_dequeue(v));
    }

    {
        queue_t q;
        assert(q.try_enqueue(1));
        assert(q.try_enqueue(2));
        assert(q.try_enqueue(3));
        auto c = q.create_consumer();
        int v = -1;
        assert(q.try_enqueue(4));
        assert(c.try_dequeue(v) && v == 4);
        assert(!c.try_dequeue(v));
    }

    {
        queue_t q;
        auto a = q.create_consumer();
        auto b = q.create_consumer();
        assert(q.try_enqueue(100));
        assert(q.try_enqueue(200));
        int va = -1;
        int vb = -1;
        assert(a.try_dequeue(va) && va == 100);
        assert(b.try_dequeue(vb) && vb == 100);
        assert(a.try_dequeue(va) && va == 200);
        assert(b.try_dequeue(vb) && vb == 200);
    }

    {
        constexpr int k_items = static_cast<int>(k_capacity);
        for (int rep = 0; rep < 200; ++rep) {
            queue_t q;
            std::atomic<int> sum{ 0 };
            std::atomic<int> outstanding{ 0 };

            auto c = q.create_consumer();

            std::thread producer([&] {
                for (int i = 0; i < k_items; ++i) {
                    while (outstanding.load(std::memory_order_acquire) >= static_cast<int>(k_capacity)) {
                        std::this_thread::yield();
                    }
                    outstanding.fetch_add(1, std::memory_order_acq_rel);
                    assert(q.try_enqueue(i));
                }
            });

            std::thread consumer([&] {
                int received = 0;
                while (received < k_items) {
                    int v = -1;
                    if (c.try_dequeue(v)) {
                        assert(v == received);
                        sum.fetch_add(v, std::memory_order_relaxed);
                        outstanding.fetch_sub(1, std::memory_order_release);
                        ++received;
                    }
                }
            });

            producer.join();
            consumer.join();

            const int expected = (k_items - 1) * k_items / 2;
            assert(sum.load(std::memory_order_relaxed) == expected);
            assert(outstanding.load(std::memory_order_relaxed) == 0);
        }
    }
}
