/* Copyright (C) 2026 Gleb Bezborodov - All Rights Reserved
 * You may use, distribute and modify this code under the
 * terms of the MIT license.
 *
 * You should have received a copy of the MIT license with
 * this file. If not, please write to: bezborodoff.gleb@gmail.com, or visit : https://github.com/glensand/hope_threading
 */

#include <algorithm>
#include <atomic>
#include <cstring>
#include <iostream>
#include <thread>

#include "hope_thread/containers/queue/spmc_bounded_non_uniform_queue.h"

namespace {

    template<std::size_t BufferSize>
    void serialize_int(hope::threading::spmc_bounded_non_uniform_queue<BufferSize>& q, int v) {
        q.seirialize([&v](uint8_t* p) {
            std::memcpy(p, &v, sizeof(v));
        }, sizeof(v));
    }

} // namespace

int main()
{
    constexpr std::size_t k_capacity = 1024;
    using queue_t = hope::threading::spmc_bounded_non_uniform_queue<k_capacity>;

    std::cout << "--- empty consumer ---" << std::endl;
    {
        queue_t q;
        auto c = q.create_consumer();
        bool ran = false;
        auto mark = [&](uint8_t*, std::size_t) { ran = true; };
        if (c.try_deserialize(mark)) {
            std::cout << "unexpected message" << std::endl;
        } else {
            std::cout << "no message (ok)" << std::endl;
        }
        if (ran) {
            return 1;
        }
    }

    std::cout << "--- single int ---" << std::endl;
    {
        queue_t q;
        auto c = q.create_consumer();
        serialize_int(q, 10);
        int v = 0;
        auto read = [&](uint8_t* data, std::size_t sz) {
            std::memcpy(&v, data, std::min(sz, sizeof(v)));
        };
        if (!c.try_deserialize(read)) {
            std::cout << "failed to read" << std::endl;
            return 1;
        }
        std::cout << "value: " << v << std::endl;
    }

    std::cout << "--- five ints in order ---" << std::endl;
    {
        queue_t q;
        auto c = q.create_consumer();
        for (int i = 0; i < 5; ++i) {
            serialize_int(q, i);
        }
        for (int i = 0; i < 5; ++i) {
            int v = -1;
            auto read = [&](uint8_t* data, std::size_t sz) {
                if (sz >= sizeof(int)) {
                    std::memcpy(&v, data, sizeof(v));
                }
            };
            if (!c.try_deserialize(read)) {
                std::cout << "missing " << i << std::endl;
                return 1;
            }
            std::cout << v << ' ';
        }
        std::cout << std::endl;
    }

    std::cout << "--- consumer after some writes ---" << std::endl;
    {
        queue_t q;
        serialize_int(q, 1);
        serialize_int(q, 2);
        serialize_int(q, 3);
        auto c = q.create_consumer();
        serialize_int(q, 4);
        int v = -1;
        auto read = [&](uint8_t* data, std::size_t sz) {
            if (sz >= sizeof(int)) {
                std::memcpy(&v, data, sizeof(v));
            }
        };
        if (!c.try_deserialize(read)) {
            std::cout << "failed" << std::endl;
            return 1;
        }
        std::cout << "first seen after attach: " << v << " (expect 4)" << std::endl;
    }

    std::cout << "--- two consumers (broadcast) ---" << std::endl;
    {
        queue_t q;
        auto a = q.create_consumer();
        auto b = q.create_consumer();
        serialize_int(q, 100);
        serialize_int(q, 200);
        int va = -1;
        int vb = -1;
        auto read_a = [&](uint8_t* data, std::size_t sz) {
            if (sz >= sizeof(int)) {
                std::memcpy(&va, data, sizeof(va));
            }
        };
        auto read_b = [&](uint8_t* data, std::size_t sz) {
            if (sz >= sizeof(int)) {
                std::memcpy(&vb, data, sizeof(vb));
            }
        };
        a.try_deserialize(read_a);
        b.try_deserialize(read_b);
        std::cout << "consumer A first: " << va << " consumer B first: " << vb << std::endl;
        a.try_deserialize(read_a);
        b.try_deserialize(read_b);
        std::cout << "consumer A second: " << va << " consumer B second: " << vb << std::endl;
    }

    std::cout << "--- producer / consumer threads ---" << std::endl;
    {
        constexpr int k_items = 64;
        queue_t q;
        std::atomic<int> sum{ 0 };
        std::atomic<int> outstanding{ 0 };
        auto c = q.create_consumer();

        std::thread producer([&] {
            for (int i = 0; i < k_items; ++i) {
                while (outstanding.load(std::memory_order_acquire) >= 32) {
                    std::this_thread::yield();
                }
                outstanding.fetch_add(1, std::memory_order_acq_rel);
                serialize_int(q, i);
            }
        });

        std::thread consumer([&] {
            int received = 0;
            auto on_msg = [&](uint8_t* data, std::size_t sz) {
                int v = 0;
                if (sz >= sizeof(int)) {
                    std::memcpy(&v, data, sizeof(v));
                }
                sum.fetch_add(v, std::memory_order_relaxed);
                outstanding.fetch_sub(1, std::memory_order_release);
                ++received;
            };
            while (received < k_items) {
                if (c.try_deserialize(on_msg)) {
                }
            }
        });

        producer.join();
        consumer.join();

        const int expected = (k_items - 1) * k_items / 2;
        std::cout << "sum=" << sum.load() << " expected=" << expected << std::endl;
    }

    std::cout << "done" << std::endl;
    return 0;
}
