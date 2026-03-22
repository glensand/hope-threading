/* Copyright (C) 2026 Gleb Bezborodov - All Rights Reserved
 * You may use, distribute and modify this code under the
 * terms of the MIT license.
 *
 * You should have received a copy of the MIT license with
 * this file. If not, please write to: bezborodoff.gleb@gmail.com, or visit : https://github.com/glensand/hope_threading
 */

#pragma once

#include <atomic>
#include <array>

#include "hope_thread/foundation.h"

namespace hope::threading {

    template<typename T, std::size_t Size>
    class spmc_bounded_message_queue final {
        static_assert(Size > 0, "Size must be greater than zero");
        static_assert((Size & (Size - 1)) == 0, "Size must be pow of 2");
    public:
        ~spmc_bounded_message_queue() = default;
        spmc_bounded_message_queue() = default;

        struct consumer final {
            consumer(spmc_bounded_message_queue* in_impl) {
                m_queue_impl = in_impl;
                m_local_read_position = m_queue_impl->m_writer_pos.load(std::memory_order_acquire);
            }

            bool try_dequeue(T& data) {
                auto writer_pos = m_queue_impl->m_writer_pos.load(std::memory_order_acquire);
                if (writer_pos == m_local_read_position) {
                    return false;
                }
                auto peak_msg = m_local_read_position & (Size - 1);
                data = m_queue_impl->m_buffer[peak_msg];
                ++m_local_read_position;
                return true;
            }
        private:
            std::size_t m_local_read_position{ 0 };
            spmc_bounded_message_queue* m_queue_impl{ nullptr };
        };

        bool try_enqueue(const T& item) {
            auto write_pos = m_writer_pos.load(std::memory_order_acquire) & (Size - 1);
            m_buffer[write_pos] = item;
            m_writer_pos.store(write_pos + 1, std::memory_order_release);
            return true;
        }

        consumer create_consumer() {
            return consumer{ this };
        }

    private:

        // advanced once writer writes something, 1 element ahead of reader
        std::atomic<std::size_t> m_writer_pos{ 0 };

        std::array<T, Size> m_buffer;

        friend struct consumer;
    };

}