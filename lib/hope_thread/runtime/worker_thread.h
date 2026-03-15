/* Copyright (C) 2023 - 2026 Gleb Bezborodov - All Rights Reserved
 * You may use, distribute and modify this code under the
 * terms of the MIT license.
 *
 * You should have received a copy of the MIT license with
 * this file. If not, please write to: bezborodoff.gleb@gmail.com, or visit : https://github.com/glensand/hope-threading
 */

#pragma once

#include <functional>
#include <thread>

#include "hope_thread/containers/queue/mpsc_queue.h"
#include "hope_thread/synchronization/event.h"

namespace hope::threading {

    template<typename TData>
    class async_worker {
    public:
        async_worker() = default;
        virtual ~async_worker() = default;

        template<typename TValue>
        void add(TValue&& v) {
            m_queued_work.enqueue(std::forward<TValue>(v));
            m_work_added.store(true, std::memory_order_release);
            m_work_added.notify_one();
        }

        void stop(){
            m_thread_impl.request_stop();
            m_work_added.store(true, std::memory_order_release);
            m_work_added.notify_one();
            m_thread_impl = {};
        }

    protected:
        virtual void run(std::stop_token token) = 0;

        void start() {
            m_thread_impl = std::jthread([this] (std::stop_token token) {
                run(token);
            });
        }

        std::jthread m_thread_impl;

        alignas(64) mpsc_queue<TData> m_queued_work;
        std::atomic_bool m_work_added{ false };
    };

    template<typename TPayload, typename TData>
    class async_worker_impl final : public async_worker<TData> {
    public:
        async_worker_impl(TPayload p, TData)
            : m_payload(std::move(p)) 
        {
            this->start();
        }

        async_worker_impl() = default;

    private:
        virtual void run(std::stop_token token) override {
            while(!token.stop_requested()) {
                TData queued;
                while(this->m_queued_work.try_dequeue(queued))
                    m_payload(std::move(queued));

                this->m_work_added.store(false, std::memory_order_release);
                this->m_work_added.wait(false, std::memory_order_acquire);
            }
        }

        TPayload m_payload;
    };

    template<typename TQueued, typename TPayload>
    async_worker_impl(TPayload, TQueued)->async_worker_impl<TPayload, TQueued>;

    using worker_function_t = std::function<void()>;

    using worker_thread_t = decltype(async_worker_impl(
        [](worker_function_t&& f) { f(); }, worker_function_t{}
    ));
}