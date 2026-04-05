/* Copyright (C) 2026 Gleb Bezborodov - All Rights Reserved
 * You may use, distribute and modify this code under the
 * terms of the MIT license.
 *
 * You should have received a copy of the MIT license with
 * this file. If not, please write to: bezborodoff.gleb@gmail.com, or visit : https://github.com/glensand/hope_threading
 */

#include <array>
#include <cassert>
#include <atomic>
#include <cstring>
#include <thread>

#include "hope_thread/containers/queue/spmc_bounded_non_uniform_queue.h"

namespace {

    constexpr std::size_t k_large_payload = 128;

    template<std::size_t BufferSize>
    void serialize_int(hope::threading::spmc_bounded_non_uniform_queue<BufferSize>& q, int v) {
        q.seirialize([&v](uint8_t* p) {
            std::memcpy(p, &v, sizeof(v));
        }, sizeof(v));
    }

    void make_large_blob(std::array<uint8_t, k_large_payload>& out, int v) {
        std::memcpy(out.data(), &v, sizeof(v));
        for (std::size_t i = sizeof(int); i < k_large_payload; ++i) {
            out[i] = static_cast<uint8_t>((static_cast<unsigned>(v) + i) & 0xFF);
        }
    }

    template<std::size_t BufferSize>
    void serialize_large_blob(hope::threading::spmc_bounded_non_uniform_queue<BufferSize>& q,
        const std::array<uint8_t, k_large_payload>& blob) {
        q.seirialize([&blob](uint8_t* p) {
            std::memcpy(p, blob.data(), k_large_payload);
        }, k_large_payload);
    }

} // namespace

void run_spmc_bounded_non_uniform_queue_tests()
{
    constexpr std::size_t k_capacity = 1024;
    using queue_t = hope::threading::spmc_bounded_non_uniform_queue<k_capacity>;

    {
        queue_t q;
        auto c = q.create_consumer();
        int v = 0;
        bool called = false;
        auto noop = [&](uint8_t*, std::size_t) {
            called = true;
        };
        assert(!c.try_deserialize(noop));
        assert(!called);
        serialize_int(q, 10);
        auto read_int = [&](uint8_t* data, std::size_t sz) {
            assert(sz == sizeof(int));
            std::memcpy(&v, data, sizeof(v));
        };
        assert(c.try_deserialize(read_int));
        assert(v == 10);
        called = false;
        assert(!c.try_deserialize(noop));
        assert(!called);
    }

    {
        queue_t q;
        auto c = q.create_consumer();
        for (int i = 0; i < 5; ++i) {
            serialize_int(q, i);
        }
        for (int i = 0; i < 5; ++i) {
            int v = -1;
            auto read_int = [&](uint8_t* data, std::size_t sz) {
                assert(sz == sizeof(int));
                std::memcpy(&v, data, sizeof(v));
            };
            assert(c.try_deserialize(read_int));
            assert(v == i);
        }
        bool called = false;
        auto noop = [&](uint8_t*, std::size_t) {
            called = true;
        };
        assert(!c.try_deserialize(noop));
        assert(!called);
    }

    {
        queue_t q;
        serialize_int(q, 1);
        serialize_int(q, 2);
        serialize_int(q, 3);
        auto c = q.create_consumer();
        int v = -1;
        serialize_int(q, 4);
        auto read_int = [&](uint8_t* data, std::size_t sz) {
            assert(sz == sizeof(int));
            std::memcpy(&v, data, sizeof(v));
        };
        assert(c.try_deserialize(read_int));
        assert(v == 4);
        auto mutate = [&](uint8_t*, std::size_t) {
            v = -2;
        };
        assert(!c.try_deserialize(mutate));
    }

    {
        queue_t q;
        auto a = q.create_consumer();
        auto b = q.create_consumer();
        serialize_int(q, 100);
        serialize_int(q, 200);
        int va = -1;
        int vb = -1;
        auto read_a = [&](uint8_t* data, std::size_t sz) {
            assert(sz == sizeof(int));
            std::memcpy(&va, data, sizeof(va));
        };
        auto read_b = [&](uint8_t* data, std::size_t sz) {
            assert(sz == sizeof(int));
            std::memcpy(&vb, data, sizeof(vb));
        };
        assert(a.try_deserialize(read_a));
        assert(b.try_deserialize(read_b));
        assert(va == 100);
        assert(vb == 100);
        assert(a.try_deserialize(read_a));
        assert(b.try_deserialize(read_b));
        assert(va == 200);
        assert(vb == 200);
    }

    {
        constexpr int k_items = static_cast<int>(k_capacity / 16);
        for (int rep = 0; rep < 200; ++rep) {
            queue_t q;
            std::atomic<int> sum{ 0 };
            std::atomic<int> outstanding{ 0 };

            auto c = q.create_consumer();

            std::thread consumer([&] {
                int received = 0;
                auto on_msg = [&](uint8_t* data, std::size_t sz) {
                    assert(sz == sizeof(int));
                    int v = 0;
                    std::memcpy(&v, data, sizeof(v));
                    assert(v == received);
                    sum.fetch_add(v, std::memory_order_relaxed);
                    outstanding.fetch_sub(1, std::memory_order_release);
                    ++received;
                };
                while (received < k_items) {
                    if (c.try_deserialize(on_msg)) {
                    }
                }
            });

            std::thread producer([&] {
                for (int i = 0; i < k_items; ++i) {
                    while (outstanding.load(std::memory_order_acquire) >= static_cast<int>(k_capacity / 8)) {
                        std::this_thread::yield();
                    }
                    outstanding.fetch_add(1, std::memory_order_acq_rel);
                    serialize_int(q, i);
                }
            });

            producer.join();
            consumer.join();

            const int expected = (k_items - 1) * k_items / 2;
            assert(sum.load(std::memory_order_relaxed) == expected);
            assert(outstanding.load(std::memory_order_relaxed) == 0);
        }
    }

    // 128-byte payloads, 1024-byte ring: int in first sizeof(int) bytes, rest is deterministic fill.
    {
        queue_t q;
        auto c = q.create_consumer();
        std::array<uint8_t, k_large_payload> expected{};
        make_large_blob(expected, 0x12345678);
        serialize_large_blob(q, expected);
        std::array<uint8_t, k_large_payload> got{};
        auto read_large = [&](uint8_t* data, std::size_t sz) {
            assert(sz == k_large_payload);
            assert(sz <= got.size());
            std::memcpy(got.data(), data, sz);
        };
        assert(c.try_deserialize(read_large));
        assert(std::memcmp(got.data(), expected.data(), k_large_payload) == 0);
        int decoded = 0;
        std::memcpy(&decoded, got.data(), sizeof(decoded));
        assert(decoded == 0x12345678);
    }

    {
        queue_t q;
        auto c = q.create_consumer();
        constexpr int k_batches = 5;
        std::array<std::array<uint8_t, k_large_payload>, k_batches> expected{};
        for (int i = 0; i < k_batches; ++i) {
            make_large_blob(expected[static_cast<std::size_t>(i)], i * 1000 + 7);
            serialize_large_blob(q, expected[static_cast<std::size_t>(i)]);
        }
        for (int i = 0; i < k_batches; ++i) {
            std::array<uint8_t, k_large_payload> got{};
            auto read_large = [&](uint8_t* data, std::size_t sz) {
                assert(sz == k_large_payload);
                std::memcpy(got.data(), data, sz);
            };
            assert(c.try_deserialize(read_large));
            assert(std::memcmp(got.data(), expected[static_cast<std::size_t>(i)].data(), k_large_payload) == 0);
        }
    }

    {
        constexpr int k_items = 4;
        queue_t q;
        std::atomic<int> sum{ 0 };
        std::atomic<int> outstanding{ 0 };
        auto c = q.create_consumer();

        std::thread consumer([&] {
            int received = 0;
            auto on_large = [&](uint8_t* data, std::size_t sz) {
                assert(sz == k_large_payload);
                int v = 0;
                std::memcpy(&v, data, sizeof(v));
                assert(v == received * 31 + 11);
                std::array<uint8_t, k_large_payload> expected{};
                make_large_blob(expected, v);
                assert(std::memcmp(data, expected.data(), k_large_payload) == 0);
                sum.fetch_add(v, std::memory_order_relaxed);
                outstanding.fetch_sub(1, std::memory_order_release);
                ++received;
            };
            while (received < k_items) {
                if (c.try_deserialize(on_large)) {
                }
            }
        });

        std::thread producer([&] {
            for (int i = 0; i < k_items; ++i) {
                while (outstanding.load(std::memory_order_acquire) >= 2) {
                    std::this_thread::yield();
                }
                outstanding.fetch_add(1, std::memory_order_acq_rel);
                const int v = i * 31 + 11;
                std::array<uint8_t, k_large_payload> blob{};
                make_large_blob(blob, v);
                serialize_large_blob(q, blob);
            }
        });

        producer.join();
        consumer.join();

        const int expected_sum = (0 * 31 + 11) + (1 * 31 + 11) + (2 * 31 + 11) + (3 * 31 + 11);
        assert(sum.load(std::memory_order_relaxed) == expected_sum);
        assert(outstanding.load(std::memory_order_relaxed) == 0);
    }

    // Sequential overlap: write one record, read it back, repeat. Payload 22 bytes (stride 26)
    // yields both producer tail-overlap (seirialize wrap branch) and consumer ring wrap at least
    // five times each over 200 iterations, without m_local_read_position reaching 1024 (strict
    // `>` in the wrap test would otherwise miss 4+1016+4 == 1024 for int-sized records).
    {
        constexpr std::size_t k_overlap_payload = 22;
        constexpr int k_overlap_iterations = 200;
        queue_t q;
        auto c = q.create_consumer();
        std::size_t write_pos_absolute = 0;
        int producer_overlap_count = 0;
        std::size_t read_shadow = 0;
        int consumer_overlap_count = 0;
        for (int i = 0; i < k_overlap_iterations; ++i) {
            const std::size_t w_before = write_pos_absolute & (k_capacity - 1);
            if (w_before + k_overlap_payload + sizeof(uint32_t) > k_capacity) {
                ++producer_overlap_count;
            }

            q.seirialize([&i](uint8_t* p) {
                std::memset(p, 0, k_overlap_payload);
                std::memcpy(p, &i, sizeof(i));
            }, k_overlap_payload);

            const std::size_t w_after_load = write_pos_absolute & (k_capacity - 1);
            if (w_after_load + k_overlap_payload + sizeof(uint32_t) > k_capacity) {
                write_pos_absolute = (write_pos_absolute / k_capacity + 1) * k_capacity;
            } else {
                write_pos_absolute += sizeof(uint32_t) + k_overlap_payload;
            }

            int decoded = -1;
            auto reader = [&](uint8_t* data, std::size_t sz) {
                assert(sz == k_overlap_payload);
                std::memcpy(&decoded, data, sizeof(decoded));
            };
            assert(c.try_deserialize(reader));
            assert(decoded == i);

            if (k_overlap_payload + read_shadow + sizeof(uint32_t) > k_capacity) {
                ++consumer_overlap_count;
                read_shadow = 0;
            } else {
                read_shadow += sizeof(uint32_t);
            }
            read_shadow += k_overlap_payload;
        }
        assert(producer_overlap_count >= 5);
        assert(consumer_overlap_count >= 5);
    }
}
