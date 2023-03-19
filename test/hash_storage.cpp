/* Copyright (C) 2023 Gleb Bezborodov - All Rights Reserved
 * You may use, distribute and modify this code under the
 * terms of the MIT license.
 *
 * You should have received a copy of the MIT license with
 * this file. If not, please write to: bezborodoff.gleb@gmail.com, or visit : https://github.com/glensand/jerk-thread
 */

#include "gtest/gtest.h"
#include "jerk-thread/containers/hashmap/hash_storage.h"
#include "jerk-thread/synchronization/spinlock.h"

#include <mutex>
#include <shared_mutex>

template<typename TValue>
struct equal final {
    bool operator()(const TValue& a, const TValue& b) const {
        return a == b;
    }
};

template<typename TValue>
using storage_t = jt::hash_storage<TValue, equal<TValue>, std::hash<TValue>, 
    jt::rw_spinlock, std::unique_lock, std::shared_lock>;

TEST(HashStorageText, Initial)
{
    storage_t<int> storage;
    std::vector<std::thread> ts;
    constexpr std::size_t ElementsCount = 10000;
    constexpr std::size_t ThreadsCount = 10;
    for (std::size_t t = 0; t < ThreadsCount / 2; ++t){
        ts.emplace_back([&]{
            for (std::size_t i = 0; i < ElementsCount; ++i)
                storage.add(i);
        });
        ts.emplace_back([&]{
            for (std::size_t i = 0; i < ElementsCount; ++i)
                volatile auto found = storage.find(i);
        });
    }

    for (auto&& t : ts)
        t.join();

    for (std::size_t i = 0; i < ElementsCount; ++i)
        ASSERT_TRUE(storage.find(i) != nullptr);
}