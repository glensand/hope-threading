/* Copyright (C) 2023 Gleb Bezborodov - All Rights Reserved
 * You may use, distribute and modify this code under the
 * terms of the MIT license.
 *
 * You should have received a copy of the MIT license with
 * this file. If not, please write to: bezborodoff.gleb@gmail.com, or visit : https://github.com/glensand/jerk-thread
 */

#pragma once

#include <cassert>
#include <type_traits>

#include "jerk-thread/synchronization/detector.h"

namespace jt {

    template<
        typename TObject,
        typename TMutex,
        template <typename> typename TExclusiveLock,
        template <typename> typename TSharedLock = TExclusiveLock>
    class object_safe_wrapper final {
    public:

        template<typename... TArgs>
        object_safe_wrapper(TArgs&&... args)
            : m_object(std::forward<TArgs>(args)...) {}

        void lock() const { lock_impl(m_object_guard, true); }
        void unlock() const { lock_impl(m_object_guard, false); }

        void lock_shared() const { lock_shared_impl(m_object_guard, true); }
        void unlock_shared() const { lock_shared_impl(m_object_guard, false); }

        decltype(auto) raw() { return m_object; }
        decltype(auto) raw() const { return m_object; }

        decltype(auto) operator -> () { return locker<TExclusiveLock<TMutex>>(&m_object, m_object_guard); }
        decltype(auto) operator * () { return locker<TExclusiveLock<TMutex>>(&m_object, m_object_guard); }

        decltype(auto) operator -> () const { return locker<TSharedLock<TMutex>>(&m_object, (TMutex&)m_object_guard); }
        decltype(auto) operator * () const { return locker<TSharedLock<TMutex>>(&m_object, (TMutex&)m_object_guard); }
    private:

        template<typename T>
        using lock_exclusive_t = decltype(std::declval<T>().lock());

        template<typename T>
        using lock_shared_t = decltype(std::declval<T>().lock_shared());

        template<typename TGuard> // just for sfinae
        void lock_impl(TGuard& guard, bool lock_flag) const {
            if constexpr (is_detected_v<lock_exclusive_t, TMutex>) {
                if (lock_flag) {
                    guard.lock();
                }
                else {
                    guard.unlock();
                }
            }
            else {
                assert(false && "To acquire lock on the object should be implemented global functions:"
                    "lock_exclusive, unlock_exclusive");
            }
        }

        template<typename TGuard> // just for sfinae
        void lock_shared_impl(TGuard& guard, bool lock_flag) const {
            if (is_detected_v<lock_shared_t, TMutex>) {
                if (lock_flag) {
                    guard.lock_shared();
                }
                else {
                    guard.unlock_shared();
                }
            }
            else {
                assert(false && "To acquire lock on the object should be implemented global functions:"
                    "lock_shared, unlock_shared");
            }
        }

    template<typename TLock>
    class locker final {
        TObject * const m_object;
    public:
        locker(locker&& rhs) noexcept
            : m_object(rhs.m_object)
              , m_lock(std::move(rhs.m_lock)) { 
                rhs.m_object = nullptr;
        }

        locker(const TObject * const object, TMutex& mutex) 
            : m_object((TObject*)object)
            , m_lock(mutex){}
        
        TObject* operator -> () { return m_object; }
        const TObject* operator -> () const { return m_object; }

        template<typename T>
        auto operator [] (const T& key) -> decltype((*m_object)[key]) { return (*m_object)[key]; }

        template<typename T>
        auto at(const T& key) -> decltype((*m_object)[key]) { return (*m_object)[key]; }
    private:
        
        TLock m_lock;
    };

        TObject m_object;
        mutable TMutex m_object_guard;
    };

}

