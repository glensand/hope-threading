#pragma once

#if defined(_WIN32) || defined(_WIN64)
#include "Windows.h"
#else
#include "pthread.h"
#endif

namespace jt::tls {

    inline auto get_thread_id() {
#if defined(_WIN32) || defined(_WIN64)
        return (unsigned long)GetCurrentThreadId();
#else
        unsigned long long tid;
        pthread_threadid_np(NULL, &tid);
        return (unsigned long)tid;
#endif
    }

}