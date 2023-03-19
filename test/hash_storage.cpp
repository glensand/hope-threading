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
using storage_t = jt::hash_storage<TValue>;

template<typename TStorage, typename TPostFunction, typename... TFunction>
void perform_work(std::size_t threads_count, std::size_t elements_count, 
    const TPostFunction& post_check,
    TFunction&&... fs) {
    storage_t<int> storage;
    std::vector<std::thread> ts;

    for (std::size_t t = 0; t < threads_count; ++t) {
        (ts.emplace_back([fs, &storage, elements_count] {
            for (std::size_t i = 0; i < elements_count; ++i)
                fs(storage, i);
        }), ...);
    }

    for (auto&& t : ts)
        t.join();

    post_check(storage);
}

void add_find(std::size_t threads_count) {
    constexpr static std::size_t ElementsCount = 10000;
    perform_work<storage_t<int>>(threads_count, ElementsCount,
        [](const storage_t<int>& storage) {
            for (std::size_t i = 0; i < ElementsCount; ++i)
                ASSERT_TRUE(storage.find(i) != nullptr);

            ASSERT_TRUE(storage.size() == ElementsCount);
        }, 
        [](storage_t<int>& storage, std::size_t i) {
            storage.add(i);
        }, 
        [](storage_t<int>& storage, std::size_t i) {
            auto volatile f = storage.find(i);
        }
    );
}

void add_remove(std::size_t threads_count) {
    constexpr static std::size_t ElementsCount = 10000;
    perform_work<storage_t<int>>(threads_count, ElementsCount,
        [](const storage_t<int>& storage) {
            ASSERT_TRUE(storage.size() == 0);
        }, 
        [](storage_t<int>& storage, std::size_t i) {
            storage.add(i);
        }, 
        [](storage_t<int>& storage, std::size_t i) {
            auto volatile f = storage.find(i);
        },
        [](storage_t<int>& storage, std::size_t i) {
            storage.remove(i);
        }
    );
}

TEST(HashStorageText, AddFindStatisticTest)
{
    add_find(1);
    add_find(3);
    add_find(10);
    add_find(30);
    add_find(100);
}

TEST(HashStorageText, AddRemove)
{
    add_remove(1);
    add_remove(3);
    add_remove(10);
    add_remove(30);
    add_remove(100);
}