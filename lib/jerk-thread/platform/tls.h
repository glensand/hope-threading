#pragma once

#if defined(_MSC_VER) || defined(__INTEL_COMPILER)
#include "Windows.h"
#endif

#include "pthread.h"

namespace jt::tls {

    inline auto get_thread_id() {
#if defined(_MSC_VER) || defined(__INTEL_COMPILER)
        return (unsigned long)GetCurrentThreadId();
#endif
        unsigned long long tid;
        pthread_threadid_np(NULL, &tid);
        return (unsigned long)tid;
    }

}