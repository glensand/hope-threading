/* Copyright (C) 2023 - 2024 Gleb Bezborodov - All Rights Reserved
 * You may use, distribute and modify this code under the
 * terms of the MIT license.
 *
 * You should have received a copy of the MIT license with
 * this file. If not, please write to: bezborodoff.gleb@gmail.com, or visit : https://github.com/glensand/hope_threading
 */

#pragma once

#if defined(_WIN32) || defined(_WIN64)
#include "Windows.h"
#else
#include "pthread.h"
#endif

#if defined(__linux) || defined(__linux__)
#include <sched.h>
#endif

namespace hope::threading {

    inline auto get_thread_id() noexcept {
#if defined(_WIN32) || defined(_WIN64)
        return (unsigned long)GetCurrentThreadId();
#else
        unsigned long long tid;
        pthread_threadid_np(NULL, &tid);
        return (unsigned long)tid;
#endif
    }

    inline auto get_proc_id() noexcept {
#if defined(__linux) || defined(__linux__)
        return sched_getcpu();
#elif defined(_WIN32) || defined(_WIN64)
        return GetCurrentProcessorNumber();
#else
        return std::rand(); // osx is currently has no support for sched_getcpu
#endif
    }

}