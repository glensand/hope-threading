/* Copyright (C) 2023 Gleb Bezborodov - All Rights Reserved
 * You may use, distribute and modify this code under the
 * terms of the MIT license.
 *
 * You should have received a copy of the MIT license with
 * this file. If not, please write to: bezborodoff.gleb@gmail.com, or visit : https://github.com/glensand/jerk-thread
 */

#pragma once

#include <mutex>

template<
    typename TObject, 
    typename TMutex = std::recursive_mutex, 
    typename TLock = std::unique_lock<TMutex>>
class object_safe_wrapper {
public:
    
    template<typename... TArgs>
    object_safe_wrapper(TArgs&&... args) 
        : m_object(std::forward<TArgs>(args)...){}

    void lock() { m_object_guard.lock(); }
    void unlock() { m_object_guard.unlock(); }

    auto operator -> () const { return locker<TLock>(&m_object, m_object_guard); }
    auto operator * () const { return locker<TLock>(&m_object, m_object_guard); }
private:
    template<typename TLock>
    class locker final {
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
        decltype(auto) operator [] (const T& key) -> decltype((*m_object)[key]) { return (*m_object)[key]; }

        template<typename T>
        decltype(auto) at(const T& key) -> decltype((*m_object)[key]) { return (*m_object)[key]; }
    private:
        TObject * const m_object;
        TLock m_lock;
    };

    TObject m_object;
    TMutex m_object_guard;
};
