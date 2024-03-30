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

    class synchronization_event {
    protected:
        enum class policy : unsigned {
            Auto,
            Manual,
        };

        explicit synchronization_event(policy policy) noexcept {
#if defined(_WIN32) || defined(_WIN64)
            m_event = CreateEvent(nullptr, policy != policy::Auto,
                                  false, nullptr);
#endif
        }
    public:
        HOPE_THREADING_CONSTRUCTABLE_ONLY(synchronization_event);
        virtual ~synchronization_event() noexcept {
#if defined(_WIN32) || defined(_WIN64)
            CloseHandle(m_event);
#endif
        };

        void set() const noexcept {
#if defined(_WIN32) || defined(_WIN64)
            SetEvent(m_event);
#else
            m_cv.notify_one();
#endif
        }

        void reset() const noexcept{
#if defined(_WIN32) || defined(_WIN64)
            ResetEvent(m_event);
#endif
        }

        bool wait(long waiting_time = std::numeric_limits<long>::max()) const noexcept {
#if defined(_WIN32) || defined(_WIN64)
            return  WaitForSingleObject(m_event, waiting_time) != WAIT_TIMEOUT;
#else
            std::unique_lock lk(m_mutex);
            m_cv.wait_for(lk, std::chrono::microseconds(waiting_time));
            return true;
#endif
        }

    private:
#if defined(_WIN32) || defined(_WIN64)
        using handle = void*;
        handle m_event;
#else
        mutable std::condition_variable m_cv;
        mutable std::mutex m_mutex;
#endif
    };

    class auto_reset_event final : public synchronization_event {
    public:
        HOPE_THREADING_CONSTRUCTABLE_ONLY(auto_reset_event);
        auto_reset_event()
            : synchronization_event(policy::Auto) {}
        virtual ~auto_reset_event() override = default;
    };

    class manual_reset_event final : public synchronization_event {
    public:
        HOPE_THREADING_CONSTRUCTABLE_ONLY(manual_reset_event);
        manual_reset_event()
            : synchronization_event(policy::Manual) {}
        virtual ~manual_reset_event() override = default;
    };
}