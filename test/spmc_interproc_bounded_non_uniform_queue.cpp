/* Copyright (C) 2026 Gleb Bezborodov - All Rights Reserved
 * You may use, distribute and modify this code under the
 * terms of the MIT license.
 *
 * You should have received a copy of the MIT license with
 * this file. If not, please write to: bezborodoff.gleb@gmail.com, or visit : https://github.com/glensand/hope_threading
 */

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <thread>
#include <vector>

#include <sys/wait.h>
#include <unistd.h>

#include "hope_thread/containers/queue/spmc_bounded_non_uniform_queue.h"
#include "hope_thread/platform/shared_memory.h"

void run_interproc_bounded_non_uniform_queue_test()
{
    constexpr std::size_t k_capacity = 1024;
    using queue_t = hope::threading::spmc_bounded_non_uniform_queue<k_capacity>;

    std::vector<int> values;
    for (auto i = 0; i < static_cast<int>(k_capacity); ++i) {
        values.push_back(i);
    }

    constexpr std::size_t shm_size = sizeof(queue_t) + 4096;

    {
        hope::threading::platform::unlink_shared_memory("/hope-shared_non_uniform_queue_seg");
        hope::threading::platform::shared_memory_segment buffer;
        std::ignore = hope::threading::platform::create_or_open_shared_memory("/hope-shared_non_uniform_queue_seg",
            shm_size, &buffer);
        std::fill(static_cast<char*>(buffer.data), static_cast<char*>(buffer.data) + shm_size, 0);
    }
    const pid_t pid = fork();
    if (pid == 0) {
        hope::threading::platform::shared_memory_segment buffer;
        std::ignore = hope::threading::platform::create_or_open_shared_memory("/hope-shared_non_uniform_queue_seg",
            shm_size, &buffer);
        auto* queue = new (buffer.data) queue_t();

        std::this_thread::sleep_for(std::chrono::seconds(10));

        for (auto v : values) {
            queue->seirialize([&v](uint8_t* p) {
                std::memcpy(p, &v, sizeof(v));
            }, sizeof(v));
        }
        std::_Exit(0);
    }
    if (pid < 0) {
        assert(false && "fork failed");
    } else {
        hope::threading::platform::shared_memory_segment buffer;
        std::ignore = hope::threading::platform::create_or_open_shared_memory("/hope-shared_non_uniform_queue_seg",
            shm_size, &buffer);

        auto* queue = reinterpret_cast<queue_t*>(buffer.data);
        auto reader = queue->create_consumer();
        std::cout << "Consuming non-uniform queue values (interproc)" << std::endl;
        for (auto v : values) {
            int consumed = 0;
            auto read_fn = [&](uint8_t* data, std::size_t sz) {
                assert(sz == sizeof(int));
                std::memcpy(&consumed, data, sizeof(consumed));
            };
            while (!reader.try_deserialize(read_fn)) {
                volatile int spin = 0;
                (void)spin;
            }
            assert(consumed == v);
        }
        std::cout << "All non-uniform queue values were consumed (interproc)" << std::endl;
        int status = 0;
        (void)waitpid(pid, &status, 0);
    }
}
