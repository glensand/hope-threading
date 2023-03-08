#pragma once

#include "Windows.h"

namespace jt::tls {

    inline auto get_thread_id() {
        return (unsigned long)GetCurrentThreadId();
    }

}