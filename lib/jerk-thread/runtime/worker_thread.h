/* Copyright (C) 2023 Gleb Bezborodov - All Rights Reserved
 * You may use, distribute and modify this code under the
 * terms of the MIT license.
 *
 * You should have received a copy of the MIT license with
 * this file. If not, please write to: bezborodoff.gleb@gmail.com, or visit : https://github.com/glensand/jerk-thread
 */

#pragma once

#include "jerk-thread/containers/queue/spsc_queue.h"
#include "jerk-thread/synchronization/event.h"

namespace jt {

    namespace detail {

        template<typename TPayload, typename TQueued>
        class base_worker final {
        public:
            base_worker()
                : m_thread_impl([this]{ run(); }) {
            }

            void stop(){
                m_launched.store(false, std::memory_order_release);
                m_job_added.set();
                m_thread_impl.join();
            }

            template<typename TValue>
            void add(TValue&& v) {
                m_queued_work.enqueue(std::forward<TValue>(v));
                m_job_added.set();
            }

        private:
            void run(){
                for(; m_launched.load(std::memory_order_acquire);) {
                    m_wait.store(false, std::memory_order_release);
                    TQueued queued;
                    while(m_queued_work.try_dequeue(queued))
                        m_payload(queued);

                    m_wait.store(true, std::memory_order_release);
                    (void)m_job_added.wait();
                }
            }

            TPayload m_payload;
            std::thread m_thread_impl;
            spsc_queue<TQueued> m_queued_work;

            std::atomic_bool m_wait{ false };
            std::atomic_bool m_launched{ true };

            auto_reset_event m_job_added;
        };
    }

    class worker_thread final {
    public:
    };

    template<typename TValue>
    class async_processor final {
    public:

    private:

    };
}