/* Copyright (C) 2023 - 2024 Gleb Bezborodov - All Rights Reserved
 * You may use, distribute and modify this code under the
 * terms of the MIT license.
 *
 * You should have received a copy of the MIT license with
 * this file. If not, please write to: bezborodoff.gleb@gmail.com, or visit : https://github.com/glensand/hope-threading
 */

#pragma once

#include <thread>
#include <type_traits>

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
            m_job_added.set();
        }

        void stop(){
            m_launched.store(false, std::memory_order_release);
            m_job_added.set();
            m_thread_impl.join();
        }

    protected:
        virtual void run() = 0;

        void start() {
            m_thread_impl = std::thread([this] { run(); });
        }

        std::thread m_thread_impl;
        
        alignas(64) std::atomic<bool> m_launched{ false };
        alignas(64) mpsc_queue<TData> m_queued_work;
        alignas(64) auto_reset_event m_job_added{};
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
        virtual void run() override {
            m_launched = true;
            for(; m_launched.load(std::memory_order_acquire);) {
                TData queued;
                while(this->m_queued_work.try_dequeue(queued))
                    m_payload(std::move(queued));

                (void)this->m_job_added.wait();
            }
        }

        TPayload m_payload;

        std::atomic_bool m_launched{ true };
    };

    template<typename TQueued, typename TPayload>
    async_worker_impl(TPayload, TQueued)->async_worker_impl<TPayload, TQueued>;

    using worker_function_t = std::function<void()>;

    using worker_thread_t = decltype(async_worker_impl(
        [](worker_function_t&& f) { f(); }, worker_function_t{}
    ));
}