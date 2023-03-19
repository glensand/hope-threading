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

    template<typename TValue, 
        typename THasher = std::hash<TValue>,
        typename TEqual = trivial_equal_operator<TValue>,
        typename TMutex = rw_spinlock,
        template <typename> typename TExclusiveLock = std::unique_lock,
        template <typename> typename TSharedLock = std::shared_lock,
        std::size_t BucketsCount = 8,
        std::size_t ResizeFactor = 2
    >
    using hash_set = hash_storage<TValue, THasher, 
        TEqual, TMutex, TExclusiveLock, TSharedLock, BucketsCount, ResizeFactor>;
}