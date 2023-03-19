/* Copyright (C) 2023 Gleb Bezborodov - All Rights Reserved
 * You may use, distribute and modify this code under the
 * terms of the MIT license.
 *
 * You should have received a copy of the MIT license with
 * this file. If not, please write to: bezborodoff.gleb@gmail.com, or visit : https://github.com/glensand/jerk-thread
 */

#pragma once

#include "jerk-thread/synchronization/hash_storage.h"

namespace jt {

    template<typename TKey, typename TValue, 
        typename THasher = std::hash<TKey>,
        typename TEqual = trivial_equal_operator<TKey>,
        typename TMutex = rw_spinlock,
        template <typename> typename TExclusiveLock = std::unique_lock,
        template <typename> typename TSharedLock = std::shared_lock,
        std::size_t BucketsCount = 8,
        std::size_t ResizeFactor = 2
    >
    class hash_map final {
        using key_value_t = std::pair<TKey, TValue>;
    public:
        template<typename... Ts>
        bool emplace(const TKey& k, Ts&&...vs) {
            return m_storage.add(key_value_t(k, TValue(std::forward<Ts>(vs)...)));
        }

        bool obtain(const TKey& k, TValue& v) {
            key_value_t kv(k, {});
            const bool obtained = m_storage.obtain(kv);
            if (obtained)
                v = kv.value;
            return obtained;
        }

        std::optional<TValue> get(const TKey& k) {
            key_value_t kv(k, {});
            const bool obtained = m_storage.obtain(kv);
            std::optional<TValue> ov;
            if (obtained)
                ov = kv.value;
            return ov;
        }
    private:
        hash_storage<key_value_t, THasher, TEqual, 
            TMutex, TExclusiveLock, TSharedLock, 
            BucketsCount, ResizeFactor> m_storage;
    };

}