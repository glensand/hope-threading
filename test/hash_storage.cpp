/* Copyright (C) 2023 - 2024 Gleb Bezborodov - All Rights Reserved
 * You may use, distribute and modify this code under the
 * terms of the MIT license.
 *
 * You should have received a copy of the MIT license with
 * this file. If not, please write to: bezborodoff.gleb@gmail.com, or visit : https://github.com/glensand/hope-threading
 */

#include "gtest/gtest.h"
#include "hope_thread/containers/hashmap/hash_set.h"
#include "hope_thread/containers/hashmap/hash_map.h"
#include "hope_thread/synchronization/spinlock.h"
#include "hope_thread/synchronization/contention_free_rw_lock.h"

#include <mutex>
#include <shared_mutex>

template<typename TValue>
using storage_t = hope::threading::hash_set<TValue>;

template<typename TStorage, typename TPostFunction, typename... TFunction>
void perform_work(std::size_t threads_count, std::size_t elements_count, 
    const TPostFunction& post_check,
    TFunction&&... fs) {
    TStorage storage;
    std::vector<std::thread> ts;

    for (std::size_t t = 0; t < threads_count; ++t) {
        (ts.emplace_back([fs, &storage, elements_count] {
            for (int i = 0; i < elements_count; ++i)
                fs(storage, i);
        }), ...);
    }

    for (auto&& t : ts)
        t.join();

    post_check(storage);
}

void add_find(std::size_t threads_count) {
    constexpr static int ElementsCount = 10000;
    perform_work<storage_t<int>>(threads_count, ElementsCount,
        [](const storage_t<int>& storage) {
            for (int i = 0; i < ElementsCount; ++i)
                ASSERT_TRUE(storage.find(i) != nullptr);

            ASSERT_TRUE(storage.size() == ElementsCount);
        }, 
        [](storage_t<int>& storage, int i) {
            storage.emplace(i);
        }, 
        [](storage_t<int>& storage, int i) {
            auto volatile f = storage.find(i);
        }
    );
}

void add_remove(std::size_t threads_count) {
    constexpr static int ElementsCount = 10000;
    perform_work<storage_t<int>>(threads_count, ElementsCount,
        [](storage_t<int>& storage) {
            for (int i = 0; i < ElementsCount; ++i)
                storage.remove(i);
            ASSERT_TRUE(storage.size() == 0);
        }, 
        [](storage_t<int>& storage, int i) {
            storage.emplace(i);
        }, 
        [](storage_t<int>& storage, int i) {
            auto volatile f = storage.find(i);
        },
        [](storage_t<int>& storage, int i) {
            storage.remove(i);
        }
    );
}

TEST(HashStorageTest, AddFindStatisticTest)
{
    return;
    add_find(1);
    add_find(3);
    add_find(10);
    add_find(30);
    add_find(300);
}

TEST(HashStorageTest, AddRemove)
{
    return;
    add_remove(1);
    add_remove(3);
    add_remove(10);
    add_remove(30);
    add_remove(300);
}

struct dumb
{
    std::string a, b, c;

    dumb(std::string _a = "", std::string _b = "", std::string _c = "")
        : a(_a), b(_b), c(_c){}
};

template<typename TKey, typename TValue>
using map_t = hope::threading::hash_map<TKey, TValue>;

TEST(HashMapTest, AddFindStatisticTest)
{
    map_t<std::string, dumb> m;
    m.emplace("lol", "a", " ", "-");
    dumb d;
    m.obtain("lol", d);
    ASSERT_TRUE(d.a == "a");
    auto&& res = m.get("lol");
    
    ASSERT_TRUE(res.value().a == "a");
}