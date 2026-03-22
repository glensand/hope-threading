/* Copyright (C) 2026 Gleb Bezborodov - All Rights Reserved
 * You may use, distribute and modify this code under the
 * terms of the MIT license.
 *
 * You should have received a copy of the MIT license with
 * this file. If not, please write to: bezborodoff.gleb@gmail.com, or visit : https://github.com/glensand/hope-threading
 */

#pragma once

#include <atomic>
#include <cstring>

namespace hope::threading {

    template<typename T>
    class seq_lock final {
    public:
        void store(const T& val) {
            const auto seq = m_seq.load(std::memory_order_relaxed);
            m_seq.store(seq + 1, std::memory_order_release);
            m_val = val;
            m_seq.store(seq + 2, std::memory_order_release);
        }

        bool load(T& val) {
            const auto seq = m_seq.load(std::memory_order_acquire);
            auto* dst_buffer = &val;
            auto* src_buffer = &m_val;
            auto size = sizeof(T);
            std::memcpy(dst_buffer, src_buffer, size);
            return m_seq.load(std::memory_order_acquire) == seq;
        }
    private:
        std::atomic<std::size_t> m_seq{ 0 };
        T m_val;
    };

}