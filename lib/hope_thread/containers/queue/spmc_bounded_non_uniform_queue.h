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
#include <cstdint>

#include "hope_thread/foundation.h"

namespace hope::threading {

    template<std::size_t BufferSize>
    class alignas(CACHE_LINE_SIZE) spmc_bounded_non_uniform_queue final {
        static_assert(BufferSize > 0, "BufferSize must be greater than zero");
        static_assert((BufferSize & (BufferSize - 1)) == 0, "BufferSize must be pow of 2");
    public:
        ~spmc_bounded_non_uniform_queue() = default;
        spmc_bounded_non_uniform_queue() = default;

        struct consumer final {
            consumer(spmc_bounded_non_uniform_queue* in_impl) {
                m_queue_impl = in_impl;
                m_local_read_position = m_queue_impl->m_writer_pos.load(std::memory_order_acquire);
            }

            template<typename F>
            bool try_deserialize(F& f) {
                auto writer_pos = m_queue_impl->m_writer_pos.load(std::memory_order_acquire);
                if (writer_pos == m_local_read_position) {
                    return false;
                }
                auto corrected_read_position = m_local_read_position & (BufferSize - 1);
                auto size = read_size(corrected_read_position);
                if (size + corrected_read_position + sizeof (uint32_t) > BufferSize) {
                    m_local_read_position = (m_local_read_position / BufferSize + 1) * BufferSize + size;
                    corrected_read_position = 0;
                } else {
                    m_local_read_position += sizeof(uint32_t) + size;
                    corrected_read_position += sizeof (uint32_t);
                }
                f(m_queue_impl->m_buffer.data() + corrected_read_position, size);
                return true;
            }
        private:

            uint32_t read_size(std::size_t position) {
                auto size_buffer = std::launder(reinterpret_cast<uint32_t*>(m_queue_impl->m_buffer.data() + position));
                return *size_buffer;
            }
            std::size_t m_local_read_position{ 0 };
            spmc_bounded_non_uniform_queue* m_queue_impl{ nullptr };
        };

        // serialize message to the queue, estimated size should be greater then actual size
        template<typename F>
        void seirialize(F&& f, const std::size_t size) {
            // if we can write in this "seriece" we will write, otherwise we will overlap to the beginning
            auto write_pos_absolute = m_writer_pos.load(std::memory_order_relaxed);
            auto writer_pos = write_pos_absolute & (BufferSize - 1);
            write_size((uint32_t)size, writer_pos);

            std::size_t position_after_write = 0;
            if (writer_pos + size + sizeof(uint32_t) > BufferSize) {
                writer_pos = 0;
                // use next sequence
                position_after_write = (write_pos_absolute / BufferSize + 1) * BufferSize + size;
            } else {
                writer_pos += sizeof(uint32_t);
                position_after_write = write_pos_absolute + sizeof(uint32_t) + size;
            }

            f(m_buffer.data() + writer_pos);

            m_writer_pos.store(position_after_write, std::memory_order_release);
        }

        consumer create_consumer() {
            return consumer{ this };
        }

    private:

        void write_size(uint32_t size, std::size_t position) {
            auto size_buffer = std::launder(reinterpret_cast<uint32_t*>(m_buffer.data() + position));
            *size_buffer = size;
        }

        // advanced once writer writes something, 1 element ahead of reader
        std::atomic<std::size_t> m_writer_pos{ };

        char pad[CACHE_LINE_SIZE]{ };

        // advanced at each overlap
        std::atomic<std::size_t> m_writer_seq{ };

        char pad1[CACHE_LINE_SIZE]{ };

        std::array<uint8_t, BufferSize> m_buffer;

        friend struct consumer;
    };

}