/* Copyright (C) 2023 Gleb Bezborodov - All Rights Reserved
 * You may use, distribute and modify this code under the
 * terms of the MIT license.
 *
 * You should have received a copy of the MIT license with
 * this file. If not, please write to: bezborodoff.gleb@gmail.com, or visit : https://github.com/glensand/jerk-thread
 */

#pragma once

#include <thread>

#undef SYSTEM_PAUSE
#if defined(_MSC_VER) || defined(__INTEL_COMPILER)
#   include <immintrin.h>
#   define SYSTEM_PAUSE _mm_pause()
#elif defined(__clang__)
// #	include <xmmintrin.h>
#	define SYSTEM_PAUSE
#else
#   define SYSTEM_PAUSE
#endif

namespace jt {

    class exponential_backoff final {
    public:
        void operator()() {
            if (m_spins <= MaxSpinsCount) {
                for (std::size_t i{ 0 }; i < m_spins; ++i)
                    SYSTEM_PAUSE;
                m_spins = m_spins << 1;
            }
            else {
                std::this_thread::yield();
            }
        }

    private:
        constexpr static std::size_t MaxSpinsCount = 32;
        std::size_t m_spins{ 0 };
    };

}
