/* Copyright (C) 2024 Gleb Bezborodov - All Rights Reserved
 * You may use, distribute and modify this code under the
 * terms of the MIT license.
 *
 * You should have received a copy of the MIT license with
 * this file. If not, please write to: bezborodoff.gleb@gmail.com, or visit : https://github.com/glensand/hope_threading
 */

#pragma once

#include <cassert>
#include <vector>

namespace hope::threading {

    template<typename T>
    class spsc_bounded_queue final {
    public:

        spsc_bounded_queue(spsc_bounded_queue const&) = delete;
        spsc_bounded_queue& operator = (spsc_bounded_queue const&) = delete;
        spsc_bounded_queue(spsc_bounded_queue&& queue) = delete;
        spsc_bounded_queue& operator=(spsc_bounded_queue&& queue) = delete;

        explicit spsc_bounded_queue(std::size_t buffer_size)
            : m_buffer_size(buffer_size)
            , m_buffer_mask(buffer_size - 1){
            assert((buffer_size > 1) && ((buffer_size & (buffer_size - 1)) == 0));
            m_buffer.resize(buffer_size);
        }

        template<typename TVal>
        bool try_enqueue(TVal&& v) {
            if (m_head - m_tail > m_buffer_size)
                return false;

            const auto actual_index = m_head & m_buffer_mask;
            m_buffer[actual_index].value = std::forward<TVal>(v);
            std::atomic_thread_fence(std::memory_order_seq_cst);
            ++m_head;
            return true;
        }

        bool try_dequeue(T& v) {
            if (m_head == m_tail)
                return false;

            const auto actual_index = m_tail & m_buffer_mask;
            v = std::move(m_buffer[actual_index].value);
            std::atomic_thread_fence(std::memory_order_seq_cst);
            ++m_tail;
            return true;
        }

    private:

        struct alignas(64) node final {
          T value;
        };

        const std::size_t m_buffer_mask{ 0 };
        const std::size_t m_buffer_size{ 0 };

        std::size_t m_head{ 0 };

        const char padding1[64];

        std::size_t m_tail{ 0 };

        const char padding2[64];

        std::vector<node> m_buffer;
    };

}