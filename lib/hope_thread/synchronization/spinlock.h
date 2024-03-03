/* Copyright (C) 2023 - 2024 Gleb Bezborodov - All Rights Reserved
 * You may use, distribute and modify this code under the
 * terms of the MIT license.
 *
 * You should have received a copy of the MIT license with
 * this file. If not, please write to: bezborodoff.gleb@gmail.com, or visit : https://github.com/glensand/hope-threading
 */

#pragma once

#include <atomic>
#include <cassert>

#include "hope_thread/core/thread_id.h"
#include "hope_thread/synchronization/backoff.h"
#include "hope_thread/foundation.h"

namespace hope::threading {

	class spinlock final {
	public:
		HOPE_THREADING_CONSTRUCTABLE_ONLY(spinlock)
	    HOPE_THREADING_EXPLICIT_DEFAULT_CONSTRUCTABLE(spinlock)

		void lock() {
			for (;;) {
				// Optimistically assume the lock is free on the first try
				if (!m_lock.exchange(true, std::memory_order_acquire))
					return;

				// Wait for lock to be released without generating cache misses
				while (m_lock.load(std::memory_order_relaxed))
					SYSTEM_PAUSE;
			}
		}

		bool try_lock() {

			// First do a relaxed load to check if lock is free in order to prevent
			// unnecessary cache misses if someone does while(!TryLock())
			return !m_lock.load(std::memory_order_relaxed) && !m_lock.exchange(true, std::memory_order_acquire);
		}

		void unlock() {
			m_lock.store(false, std::memory_order_release);
		}

	private:
		std::atomic_bool m_lock = false;
	};


	// Non recursive(in common meaning) read-write spinlock;
	// Read operation could not be upgraded to Write (to upgrade you should release readlock on this thread and then acquire write lock, NOTE: such op is not atomic)
	// Read lock is recursive by design
	// Write operation also could not be doungraded to read
	// In conclusion these sequences causes infinite loop when called from one thread: Write -> Read, Read -> Write, Write -> Write
	// TODO: RW spinlock might be upgraded to full recursive rw lock(TLS should be used for this purpose). In TLS thread's state should be stored (read, write, nostate)
	// The implementation is based off of: https://jfdube.wordpress.com/2014/01/12/optimizing-the-recursive-read-write-spinlock/
	class rw_spinlock final {
	public:
		HOPE_THREADING_CONSTRUCTABLE_ONLY(rw_spinlock)
	    HOPE_THREADING_EXPLICIT_DEFAULT_CONSTRUCTABLE(rw_spinlock)

		void lock_shared() {
			for (;;) {
				// Optimistically assume the lock is free on the first try
				if (((++m_lock) & 0xfff00000) == 0)
					// No active writer, acquired successfully
					return;
				// if any of writer bits are set, it means that a writer held the lock before us and we failed.
				// In that case, we atomically decrement the lock value and try again
				--m_lock;

				// Wait for lock to be released without generating cache misses
				while (m_lock.load(std::memory_order_relaxed) & 0xfff00000)
					SYSTEM_PAUSE;
			}
		}

		void unlock_shared() {
			--m_lock;
		}

		void lock() {
			for (;;)
			{
				// Optimistically assume the lock is free on the first try
				if ((m_lock += 0x100000) == 0x100000)
					return;

				// Another writer held the lock before usand we failed.
				// In that case, we atomically decrement the number of writers(using an atomic subtract) and try again
				m_lock -= 0x100000;

				// Wait until there's no active writer
				while (m_lock.load(std::memory_order_relaxed) & 0xfff00000)
					SYSTEM_PAUSE;
			}
		}

		void unlock() {
			m_lock -= 0x100000;
		}

	private:
		// The spinlock is implemented using a 32-bits value
		// The writer bits are placed on the most significant bits:
		// | WWWWWWWWWWWW | RRRRRRRRRRRRRRRRRRRR |
		std::atomic<int32_t> m_lock = 0u;
	};

	class recursive_spinlock final {
		constexpr static uint32_t Free = 0;
	public:
		HOPE_THREADING_CONSTRUCTABLE_ONLY(recursive_spinlock)
	    HOPE_THREADING_EXPLICIT_DEFAULT_CONSTRUCTABLE(recursive_spinlock)

		void lock() {
			const uint32_t current_thread = get_thread_id();

			if (m_owner_id.load(std::memory_order_acquire) == current_thread) {
				++m_recursion;
				// Recursion branch, the lock is already acquired, we are the SLAVE ;D
				return;
			}
			for (;;) {
				// Optimistically assume the lock is free on the first try
				auto Expected = Free;
				if (m_owner_id.compare_exchange_strong(Expected, current_thread, std::memory_order_acquire)) {
					assert(m_recursion == 0);
					m_recursion = 1;
					return;
				}

				// Wait for lock to be released without generating cache misses
				while (m_owner_id.load(std::memory_order_relaxed) != Free)
					// Issue X86 PAUSE instruction to reduce contention between hyper-threads
					SYSTEM_PAUSE;
			}
		}

		bool try_lock() {
			const uint32_t current_thread = get_thread_id();
			const uint32_t current_owner = m_owner_id.load(std::memory_order_acquire);

			if (current_owner == current_thread) {
				++m_recursion;
				// Recursion branch, the lock is already acquired, we are the SLAVE ;D
				return true;
			}

			auto expected = Free;
			const bool locked = Free == current_owner
				&& m_owner_id.compare_exchange_strong(expected, current_thread, std::memory_order_acquire);

			if (locked) {
				assert(!locked || m_recursion == 0);
				++m_recursion;
			}

			return locked;
		}

		void unlock() {
			--m_recursion;
			if (m_recursion == 0)
			{
				assert(m_owner_id == get_thread_id());
				m_owner_id.store(Free);
			}
		}

	private:
		std::atomic<uint32_t> m_owner_id = Free;
		std::size_t m_recursion = 0;
	};


	// almost recursive shared lock:
	// works with sequences: S->S, X->X, X->S
	// S->X causes deadlock
	class recursive_rw_spinlock final {
	public:
		HOPE_THREADING_CONSTRUCTABLE_ONLY(recursive_rw_spinlock)
	    HOPE_THREADING_EXPLICIT_DEFAULT_CONSTRUCTABLE(recursive_rw_spinlock)

		void lock_shared() noexcept {
			if (check_for_recursion()) {
				// we are the owner, do nothing here? 
				return;
			}

			for (;;) {
				// Optimistically assume the lock is free on the first try
				if (((++m_lock) & ExclusiveMask) == 0)
					// No active writer, acquired successfully
					return;

				// if any of writer bits are set, it means that a writer held the lock before us and we failed.
				// In that case, we atomically decrement the lock value and try again
				--m_lock;

				// Wait for lock to be released without generating cache misses
				while (m_lock.load(std::memory_order_relaxed) & ExclusiveMask)
					// Here and below issue X86 PAUSE instruction to reduce contention between hyper-threads
					SYSTEM_PAUSE;
			}
		}

		void unlock_shared() noexcept {
			if (m_write_recursion_depth == 0) { // no writers on this thread, only readers
				--m_lock;
			}
		}

		void lock() noexcept {
			if (check_for_recursion()) {
				++m_write_recursion_depth;
				return;
			}
			for (;;) {
				// Optimistically assume the lock is free on the first try
				if ((m_lock += 0x100000) == 0x100000) {

					const uint64_t this_id = ((uint64_t)get_thread_id()) << 32;
					assert(0 == (this_id & (ExclusiveMask | SharedMask)));
					m_lock += this_id;

					m_write_recursion_depth = 1;
					return;
				}

				// Another writer held the lock before usand we failed.
				// In that case, we atomically decrement the number of writers(using an atomic subtract) and try again
				m_lock -= 0x100000;

				// Wait until there's no active writer
				while (m_lock.load(std::memory_order_relaxed) & ExclusiveMask)
					SYSTEM_PAUSE;
			}
		}

		void unlock() noexcept {
			--m_write_recursion_depth;
			if (m_write_recursion_depth == 0) {
				const auto this_id = (uint64_t)get_thread_id() << 32;
				assert(0 == (this_id & (ExclusiveMask | SharedMask)));
				m_lock -= (this_id);
				m_lock -= (0x100000);
			}
		}

	private:
		bool check_for_recursion() const noexcept {
			const uint64_t current_lock_thread = ThreadIdMask & m_lock.load(std::memory_order_acquire);
			const auto owner_thread_id = (uint32_t)(current_lock_thread >> 32);
			const uint32_t this_id = get_thread_id();
			assert(owner_thread_id != this_id || m_write_recursion_depth > 0);
			return this_id == owner_thread_id;
		}

		// The spinlock is implemented using a 64-bits value
		// The writer bits are placed on the most significant bits of the second 32bit part:
		// | WWWWWWWWWWWW | RRRRRRRRRRRRRRRRRRRR |
		// Most significant 32bit part contains owner thread id (only for exclusive locks)
		constexpr static uint64_t ExclusiveMask	{ 0x00000000fff00000 };
		constexpr static uint64_t SharedMask	{ 0x00000000000fffff };
		constexpr static uint64_t ThreadIdMask	{ 0xffffffff00000000 };

		std::atomic<uint64_t> m_lock = 0;
		std::size_t m_write_recursion_depth = 0;
	};
}
