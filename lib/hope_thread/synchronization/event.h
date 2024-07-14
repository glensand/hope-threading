/* Copyright (C) 2023 - 2024 Gleb Bezborodov - All Rights Reserved
 * You may use, distribute and modify this code under the
 * terms of the MIT license.
 *
 * You should have received a copy of the MIT license with
 * this file. If not, please write to: bezborodoff.gleb@gmail.com, or visit : https://github.com/glensand/hope-threading
 */

#pragma once

#include <limits>

#include "hope_thread/foundation.h"

#if !defined(_WIN32) && !defined(_WIN64)
#include <condition_variable>
#include <mutex>
#else
#include "Windows.h"
#undef min
#undef max
#endif

namespace hope::threading {

#if defined(_WIN32) || defined(_WIN64)
 class synchronization_event {
    public:
        synchronization_event() noexcept {
            m_event = CreateEvent(nullptr, false, false, nullptr);
        }

        HOPE_THREADING_CONSTRUCTABLE_ONLY(synchronization_event);
        ~synchronization_event() noexcept {
            CloseHandle(m_event);
        };

        void set() const noexcept {
            SetEvent(m_event);
        }

        void reset() const noexcept{
            ResetEvent(m_event);
        }

        bool wait(long waiting_time = std::numeric_limits<long>::max()) const noexcept {
            return  WaitForSingleObject(m_event, waiting_time) != WAIT_TIMEOUT;
        }

    private:
        using handle = void*;
        handle m_event;
    };
#else
 class synchronization_event {
    public:
        HOPE_THREADING_CONSTRUCTABLE_ONLY(synchronization_event);
        synchronization_event() noexcept = default;
        ~synchronization_event() noexcept = default;

        void set() const noexcept {
            m_cv.notify_one();
        }

        bool wait(long waiting_time = -1) const noexcept {
            std::unique_lock lk(m_mutex);
            std::chrono::milliseconds delay(waiting_time);
            if (waiting_time > 0) {
                m_cv.wait_for(lk, delay);
            } else {
                m_cv.wait(lk);
            }

            return true;
        }

    private:
        mutable std::condition_variable m_cv;
        mutable std::mutex m_mutex;
    };
#endif

    using auto_reset_event = synchronization_event;
    using manual_reset_event = synchronization_event;
}