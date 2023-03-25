/* Copyright (C) 2023 Gleb Bezborodov - All Rights Reserved
 * You may use, distribute and modify this code under the
 * terms of the MIT license.
 *
 * You should have received a copy of the MIT license with
 * this file. If not, please write to: bezborodoff.gleb@gmail.com, or visit : https://github.com/glensand/jerk-thread
 */

#pragma once

#include "jerk-thread/synchronization/spinlock.h"

#include <set>
#include <mutex>
#include <shared_mutex>
#include <array>

namespace jt {

    template<
        typename TKey,
        typename TGuard = rw_spinlock,
        template <typename> typename TSharedLock = std::shared_lock,
        template <typename> typename TExclusiveLock = std::unique_lock,
        std::size_t ChunkCount = 8
    >
    class stl_chunked_set final {
        struct chunk {
            std::unordered_set<TKey> set;
            mutable TGuard guard;
        };

        using storage_t = std::array<chunk, ChunkCount>;
    public:
        template<typename Ts>
        void emplace(Ts&& k) {
            const auto chunk = std::hash<TKey>{}(k) % ChunkCount;
            const TExclusiveLock lock(m_storage[chunk].guard);
            m_storage[chunk].set.emplace(std::forward<Ts>(k));
        }

        auto find(const TKey& k) const {
            const auto chunk = std::hash<TKey>{}(k) % ChunkCount;
            const TSharedLock lock(m_storage[chunk].guard);
            return m_storage[chunk].set.find(k);
        }
    private:
        storage_t m_storage;
    };

}