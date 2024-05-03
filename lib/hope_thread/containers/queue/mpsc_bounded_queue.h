/* Copyright (C) 2024 Gleb Bezborodov - All Rights Reserved
 * You may use, distribute and modify this code under the
 * terms of the MIT license.
 *
 * You should have received a copy of the MIT license with
 * this file. If not, please write to: bezborodoff.gleb@gmail.com, or visit : https://github.com/glensand/hope_threading
 */

#pragma once

#include <cassert>
#include <array>
#include <atomic>

#include "hope_thread/foundation.h"

namespace hope::threading {

    template<typename T, std::size_t Size>
    class mpsc_bounded_queue final {
    public:
        HOPE_THREADING_CONSTRUCTABLE_ONLY(mpsc_bounded_queue)

        explicit mpsc_bounded_queue()
            : m_buffer_mask(Size - 1)
            , m_buffer_size(Size){
            assert((Size > 1) && ((Size & (Size - 1)) == 0));
        }

        template<typename TVal>
        bool try_enqueue(TVal&& v) {
            auto cur_head = m_head.load(std::memory_order_relaxed);
            while (true){
                if (cur_head - m_head > m_buffer_size) return false;
                if (m_head.compare_exchange_strong(cur_head, cur_head + 1,
                                                   std::memory_order_relaxed, std::memory_order_relaxed)){
                    break;
                }
            }

            const auto actual_index = cur_head & m_buffer_mask;
            m_buffer[actual_index].value = std::forward<TVal>(v);
            std::atomic_thread_fence(std::memory_order_seq_cst);
            m_buffer[actual_index].ready = true;
            return true;
        }

        bool try_dequeue(T& v) {
            if (m_head.load(std::memory_order_relaxed) == m_tail)
                return false;

            const auto actual_index = m_tail & m_buffer_mask;
            if (!m_buffer[actual_index].ready){
                return false;
            }
            v = std::move(m_buffer[actual_index].value);
            m_buffer[actual_index].ready = false;
            std::atomic_thread_fence(std::memory_order_seq_cst);
            ++m_tail;
            return true;
        }

    private:

        struct alignas(64) node final {
          T value;
          bool ready;
        };

        const std::size_t m_buffer_mask;
        const std::size_t m_buffer_size;

        std::atomic<std::size_t> m_head{ 0 };

        const char padding1[64]{ };

        std::size_t m_tail{ 0 };

        const char padding2[64]{ };

        std::array<node, Size> m_buffer;
    };

}