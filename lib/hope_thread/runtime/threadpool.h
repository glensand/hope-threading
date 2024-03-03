/* Copyright (C) 2023 - 2024 Gleb Bezborodov - All Rights Reserved
 * You may use, distribute and modify this code under the
 * terms of the MIT license.
 *
 * You should have received a copy of the MIT license with
 * this file. If not, please write to: bezborodoff.gleb@gmail.com, or visit : https://github.com/glensand/hope-threading
 */

#pragma once

#include <functional>
#include <thread>
#include <deque>
#include <atomic>
#include <cassert>
#include <mutex>

#include "hope_thread/synchronization/event.h"
#include "hope_thread/synchronization/spinlock.h"
#include "hope_thread/containers/queue/mpmc_bounded_queue.h"

// todo:: even not tested yet, idk what the code is going here...
namespace hope::threading {

    /**
    * Implementation of a queued thread pool.
    */
    class thread_pool final {

        struct work_wrapper final{
            std::function<void()> work_impl;
        };

        class queued_thread final {
        public:
            HOPE_THREADING_CONSTRUCTABLE_ONLY(queued_thread)

            explicit queued_thread(class thread_pool* tp)
                    : m_thread_pool(tp)
                    , m_thread_impl([this]{ run();})
            { }

            ~queued_thread() {
                shutdown();
            }

            void shutdown() {
                if (!m_time_to_die.load(std::memory_order_acquire)) {
                    m_time_to_die.store(true, std::memory_order_release);
                    m_wait_event.set();
                    m_thread_impl.join();
                }
            }

            void perform(work_wrapper* w){
                m_queued_work.store(w, std::memory_order_release);
                m_wait_event.set();
            }

        private:
            void run() {
                while (!m_time_to_die.load(std::memory_order_acquire)){
                    auto* current_work = m_queued_work.load(std::memory_order_acquire);
                    m_queued_work = nullptr;
                    while (current_work != nullptr) {
                        current_work->work_impl();
                        m_thread_pool->return_wrapper_to_pool(current_work);
                        // Let the object cleanup before we remove our ref to it
                        current_work = m_thread_pool->return_or_get(this);
                    }
                    (void)m_wait_event.wait();
                }
            }

            class thread_pool* m_thread_pool;
            std::atomic<work_wrapper*> m_queued_work{nullptr };
            auto_reset_event m_wait_event{};
            std::atomic_bool m_time_to_die{ false };
            std::thread m_thread_impl;

            friend class thread_pool;
        };
    public:

        HOPE_THREADING_CONSTRUCTABLE_ONLY(thread_pool)

        /** Virtual destructor (cleans up the synchronization objects). */
        ~thread_pool() {
            destroy();
        }

        explicit thread_pool(std::size_t num_threads) {
            for (std::size_t i{ 0u }; i < num_threads ; ++i) {
                // Create a new queued thread
                auto* new_thread = new(std::nothrow) queued_thread(this);
                assert(new_thread);
                m_free_threads.emplace_back(new_thread);
                m_all_threads.push_back(new_thread);
            }
        }

        void destroy() {
            {
                // wait for all threads to finish up
                const std::lock_guard lock(m_queue_guard);
                m_time_to_die.store(true, std::memory_order_release);
                // Clean up all queued objects
                m_queued_work.clear();
            }

            while (true) {
                {
                    const std::lock_guard lock(m_queue_guard);
                    if (m_all_threads.size() == m_free_threads.size())
                        break;
                }
                std::this_thread::yield();
            }

            // Delete all threads
            const std::lock_guard lock(m_queue_guard);
            for (const auto* thread : m_all_threads)
                delete thread;

            work_wrapper* w;
            while (m_work_wrapper_pool.try_dequeue(w))
                delete w;

            m_free_threads.clear();
            m_all_threads.clear();
        }

        void add_work(std::function<void()>&& w) {
            if (m_time_to_die.load(std::memory_order_acquire))
                return;

            work_wrapper* queued_work = nullptr;
            if (!m_work_wrapper_pool.try_dequeue(queued_work)){
                queued_work = new work_wrapper;
            }

            queued_work->work_impl = std::move(w);
            m_queue_guard.lock();
            if (m_free_threads.empty()) {
                // No thread available, queue the work to be done
                // as soon as one does become available
                m_queued_work.emplace_back(queued_work);
                m_queue_guard.unlock();
            } else{
                auto* thread = m_free_threads.back();
                m_free_threads.pop_back();
                m_queue_guard.unlock();
                thread->perform(queued_work);
            }
        }
    private:

        work_wrapper* return_or_get(queued_thread* thread) {
            work_wrapper* queued_work = nullptr;
            // Check to see if there is any work to be done
            const std::lock_guard lock(m_queue_guard);
            if (!m_queued_work.empty()){
                queued_work = m_queued_work.front();
                m_queued_work.pop_front();
            }
            if (queued_work == nullptr) {
                // There was no work to be done, so add the thread to the pool
                m_free_threads.emplace_back(thread);
            }
            return queued_work;
        }

        void return_wrapper_to_pool(work_wrapper* w){
            const auto success = m_work_wrapper_pool.try_enqueue(w);
            assert(success);
        }

        mpmc_bounded_queue<work_wrapper*> m_work_wrapper_pool{1024 };

        /** The work queue to pull from. */
        std::deque<work_wrapper*> m_queued_work;

        /** The thread pool to dole work out to. */
        std::deque<queued_thread*> m_free_threads;

        /** All threads in the pool. */
        std::deque<queued_thread*> m_all_threads;

        /** The synchronization object used to protect access to the queued work. */
        spinlock m_queue_guard;

        /** If true, indicates the destruction process has taken place. */
        std::atomic_bool m_time_to_die{ false };

        friend class queued_thread;
    };

}
