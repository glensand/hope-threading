/* Copyright (C) 2023 - 2024 Gleb Bezborodov - All Rights Reserved
 * You may use, distribute and modify this code under the
 * terms of the MIT license.
 *
 * You should have received a copy of the MIT license with
 * this file. If not, please write to: bezborodoff.gleb@gmail.com, or visit : https://github.com/glensand/hope-threading
 */

#pragma once

#include <type_traits>

#include "hope_thread/containers/queue/spsc_queue.h"
#include "hope_thread/synchronization/event.h"

// todo:: even not tested yet, idk what the code is going here...
namespace hope::threading {

    namespace detail {

        template<typename TPayload, typename TQueued>
        class base_worker {
        public:
            explicit base_worker(TPayload&& p)
                : m_payload(std::move(p))
                , m_thread_impl([this]{ run(); }) {
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
                    TQueued queued;
                    while(m_queued_work.try_dequeue(queued))
                        m_payload(std::move(queued));

                    (void)m_job_added.wait();
                }
            }

            TPayload m_payload;
            std::thread m_thread_impl;
            spsc_queue<TQueued> m_queued_work;

            std::atomic_bool m_launched{ true };
            auto_reset_event m_job_added{};
        };

        using worker_function_t = std::function<void()>;
        constexpr static auto&& worker_function = [](worker_function_t&&){};

        using worker_payload_t = std::decay_t<decltype(worker_function)>;
    }

    class worker_thread final :
        public detail::base_worker<detail::worker_payload_t, detail::worker_function_t> {
    public:
    };

    template<typename TValue>
    class async_processor final {
    public:

    private:

    };
}