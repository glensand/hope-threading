/* Copyright (C) 2026 Gleb Bezborodov - All Rights Reserved
 * You may use, distribute and modify this code under the
 * terms of the MIT license.
 *
 * You should have received a copy of the MIT license with
 * this file. If not, please write to: bezborodoff.gleb@gmail.com, or visit : https://github.com/glensand/hope-threading
 */

#include <cassert>
#include <atomic>
#include <iostream>
#include <thread>

#include "hope_thread/containers/queue/spmc_bounded_message_queue.h"
#include "hope_thread/platform/shared_memory.h"
#include <vector>

void run_interproc_test()
{
    constexpr std::size_t k_capacity = 1024;

    std::vector<int> values;
    for (auto i = 0; i < k_capacity; ++i) {
        values.push_back(i);
    }

    constexpr std::size_t shm_size = sizeof(int) * 1024 + 1024;

    {
        hope::threading::platform::unlink_shared_memory("/hope-shared_memory_test_seg");
        // clear memory
        hope::threading::platform::shared_memory_segment buffer;
        std::ignore = hope::threading::platform::create_or_open_shared_memory("/hope-shared_memory_test_seg",
            shm_size, &buffer);
        std::fill((char*)buffer.data, (char*)buffer.data + shm_size, 0);
    }
    if (auto pid = fork(); pid == 0) {
        // child - producer

        hope::threading::platform::shared_memory_segment buffer;
        std::ignore = hope::threading::platform::create_or_open_shared_memory("/hope-shared_memory_test_seg",
            shm_size, &buffer);
        using queue_t = hope::threading::spmc_bounded_message_queue<int, k_capacity>;
        auto* queue = new (buffer.data) queue_t();

        std::this_thread::sleep_for(std::chrono::seconds(10));

        for (auto v : values) {
            queue->try_enqueue(v);
        }
    } else {
        // me - consumer
        hope::threading::platform::shared_memory_segment buffer;
        std::ignore = hope::threading::platform::create_or_open_shared_memory("/hope-shared_memory_test_seg",
            shm_size, &buffer);

        using queue_t = hope::threading::spmc_bounded_message_queue<int, k_capacity>;
        auto* queue = (queue_t*)(buffer.data);
        auto reader = queue->create_consumer();
        std::cout << "Consuming values" << std::endl;
        for (auto v : values) {
            int consumed = 0;
            while (!reader.try_dequeue(consumed)) {
                volatile int test = 0;
            }
            assert(consumed == v);
        }
        std::cout << "All values was consumed" << std::endl;
    }
}
