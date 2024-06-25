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
    protected:
        enum class policy : unsigned {
            Auto,
            Manual,
        };

        explicit synchronization_event(policy policy) noexcept {
            m_event = CreateEvent(nullptr, policy != policy::Auto,
                                  false, nullptr);
        }
    public:
        HOPE_THREADING_CONSTRUCTABLE_ONLY(synchronization_event);
        virtual ~synchronization_event() noexcept {
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
    protected:
        enum class policy : unsigned {
            Auto,
            Manual,
        };

        explicit synchronization_event(policy policy) noexcept {
        }
    public:
        HOPE_THREADING_CONSTRUCTABLE_ONLY(synchronization_event);
        virtual ~synchronization_event() noexcept {
        };

        void set() const noexcept {
            m_cv.notify_one();
        }

        void reset() const noexcept{

        }

        bool wait(long waiting_time = std::numeric_limits<long>::max()) const noexcept {
            std::unique_lock lk(m_mutex);
            m_cv.wait_until(lk, std::chrono::system_clock::now() + std::chrono::microseconds(waiting_time));
            return true;
        }

    private:
        mutable std::condition_variable m_cv;
        mutable std::mutex m_mutex;
    };
#endif

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