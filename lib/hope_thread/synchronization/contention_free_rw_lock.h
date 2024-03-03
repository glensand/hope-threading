/* Copyright (C) 2024 Gleb Bezborodov - All Rights Reserved
 * You may use, distribute and modify this code under the
 * terms of the MIT license.
 *
 * You should have received a copy of the MIT license with
 * this file. If not, please write to: bezborodoff.gleb@gmail.com, or visit : https://github.com/glensand/hope-threading
 */

#pragma once

#include <atomic>
#include <thread>
#include <cassert>
#include <array>

#include "hope_thread/platform/tls.h"
#include "hope_thread/synchronization/backoff.h"

namespace hope::threading {

    template<std::size_t MaxProcNumber>
	class contention_free_rw_lock final {
    public:
        std::size_t lock_shared() {
            auto cur_cpu = std::max(get_proc_id(), MaxProcNumber - 1);
            assert(cur_cpu < MaxProcNumber);
            for (;;){
                ++m_read_lock[cur_cpu];
                std::atomic_thread_fence(std::memory_order_seq_cst);
                if (!m_write_lock.load(std::memory_order_relaxed))
                    break;
                --m_read_lock[cur_cpu];
                while (m_write_lock.load(std::memory_order_relaxed))
                    SYSTEM_PAUSE;
            }

            return cur_cpu;
        }

        void unlock_shared(std::size_t cur_cpu){
            --m_read_lock[cur_cpu];
        }

        void lock_exclusive(){
            auto lock_ex = false;
            while (!m_write_lock.compare_exchange_strong(lock_ex, true)){
                SYSTEM_PAUSE;
            }

            bool all_free = false;
            while(!all_free){
                all_free = true;
                for (auto&& read_lock : m_read_lock){
                    all_free & = m_read_lock.load(std::memory_order_acquire);
                    if (!all_free){
                        SYSTEM_PAUSE;
                        break;
                    }
                }
            }
        }

        void unlock_exclusive(){
            m_write_lock.store(false, std::memory_order_release);
        }

    private:
        struct alignas(64) contention_free_flag final {
            std::atomic<int> locked{ 0 };
        };

        std::atomic<bool> m_write_lock;

        char m_padding1[64];

        std::array<contention_free_flag, MaxProcNumber> m_read_lock;
    };

}
