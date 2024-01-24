/* Copyright (C) 2023 Gleb Bezborodov - All Rights Reserved
 * You may use, distribute and modify this code under the
 * terms of the MIT license.
 *
 * You should have received a copy of the MIT license with
 * this file. If not, please write to: bezborodoff.gleb@gmail.com, or visit : https://github.com/glensand/jerk-thread
 */

#pragma once

#include "jerk-thread/containers/hashmap/hash_storage.h"

#include <optional>
#include <mutex>
#include <shared_mutex>

namespace jt {

    template<typename TKey, typename TValue>
    struct key_value final {
        TKey key;
        TValue value;

        template<typename... Ts,
            typename = std::enable_if_t< (0 < sizeof...(Ts)) >>
        key_value(TKey&& k, Ts&&... args)
            : key(std::move(k))
            , value(std::forward<Ts>(args)...){ }

        template<typename... Ts,
            typename = std::enable_if_t< (0 < sizeof...(Ts)) >>
        key_value(const TKey& k, Ts&&... args)
            : key(k)
            , value(std::forward<Ts>(args)...) { }

        key_value(key_value&& kv) noexcept
            : key(std::move(kv.key))
            , value(std::move(kv.value)){}

        key_value(const key_value& kv)
            : key(kv.key)
            , value(kv.value) {}

        key_value(const TKey& k)
            : key(k) {}
    };

    template<typename TKey, typename TValue>
    struct map_traits final {

        template<typename T1, typename... Ts>
        static void assign_value(key_value<TKey, TValue>& kv, const T1&, Ts&&... args) {
            kv.value = TValue(std::forward<Ts>(args)...);
        }

        static void assign_value(key_value<TKey, TValue>& lhs, const key_value<TKey, TValue>& rhs) {
            lhs.value = rhs.value;
        }

        static decltype(auto) extract_key(const key_value<TKey, TValue>& kv) noexcept {
            return kv.key;
        }

        template<typename... Ts>
        static decltype(auto) extract_key(const TKey& k, Ts&&...) noexcept {
            return k;
        }
    };

    template<typename TKey, typename TValue, 
        template <typename> typename THasher = std::hash,
        typename TEqual = trivial_equal_operator,
        typename TMutex = rw_spinlock,
        template <typename> typename TExclusiveLock = std::unique_lock,
        template <typename> typename TSharedLock = std::shared_lock,
        std::size_t BucketsCount = 8,
        std::size_t ResizeFactor = 2
    >
    class hash_map final {
    public:
        template<typename... Ts>
        bool emplace(Ts&&...vs) {
            return m_storage.emplace(std::forward<Ts>(vs)...);
        }

        bool obtain(const TKey& k, TValue& v) const noexcept {
            key_value<TKey, TValue> kv{ k };
            const bool obtained = m_storage.obtain(kv);
            if (obtained)
                v = std::move(kv.value);
            return obtained;
        }

        std::optional<TValue> get(const TKey& k) const noexcept {
            key_value<TKey, TValue> kv{ k };
            const bool obtained = m_storage.obtain(kv);
            std::optional<TValue> ov;
            if (obtained)
                ov = std::move(kv.value);
            return ov;
        }

        void remove(const TKey& k) {
            m_storage.remove(k);
        }
    private:
        hash_storage<key_value<TKey, TValue>, map_traits<TKey, TValue>, THasher<TKey>,
            TEqual, TMutex, TExclusiveLock, TSharedLock, BucketsCount, ResizeFactor> m_storage;
    };

}