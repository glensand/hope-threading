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

template<
    typename TObject, 
    typename TMutex, 
    typename TExclusiveLock,
    typename TSharedLock = TExclusiveLock>
class object_safe_wrapper final {
public:
    
    template<typename... TArgs>
    object_safe_wrapper(TArgs&&... args) 
        : m_object(std::forward<TArgs>(args)...){}

    void lock() { lock_impl(m_object_guard, true); }
    void unlock() { lock_impl(m_object_guard, false); }

    void lock_read() { lock_shared_impl(m_object_guard, true); }
    void unlock_read() { unlock_shared_impl(m_object_guard, false); }

    auto operator -> () { return locker<TExclusiveLock>(&m_object, m_object_guard); }
    auto operator * () { return locker<TExclusiveLock>(&m_object, m_object_guard); }

    auto operator -> () const { return locker<TSharedLock>(&m_object, m_object_guard); }
    auto operator * () const { return locker<TSharedLock>(&m_object, m_object_guard); }
private:

    template<typename T>
    using lock_exclusive_t = decltype(lock_exclusive(std::declval<T&>()));

    template<typename T>
    using lock_shared_t = decltype(lock_shared(std::declval<T&>()));
    
    template<typename TGuard> // just for sfinae
    void lock_impl(TGuard& guard, bool lock_flag) {
        if constexpr (is_detected_v<lock_exclusive_t, TMutex>) {
            if (lock_flag) {
                lock_exclusive(guard);
            } else {
                unlock_exclusive(guard);
            }
        } else {
            assert(false && "To acquire lock on the object should be implemented global functions:"
            "lock_exclusive, unlock_exclusive");
        }
    }

    template<typename TGuard> // just for sfinae
    void lock_shared_impl(TGuard& guard, bool lock_flag) {
        if (is_detected_v<lock_shared_t, TMutex>) {
            if (lock_flag) {
                lock_shared(guard);
            } else {
                unlock_shared(guard);
            }
        } else {
            assert(false && "To acquire lock on the object should be implemented global functions:"
            "lock_shared, unlock_shared");
        }
    }

    template<typename TLock>
    class locker final {
        TObject * const m_object;
    public:
        locker(locker&& rhs) 
            : m_object(rhs.m_object)
            , m_lock(std::move(rhs.m_lock)) { 
                rhs.m_object = nullptr;
            }

        locker(TObject * const object, TMutex& mutex) 
            : m_object(object)
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
    TMutex m_object_guard;
};
