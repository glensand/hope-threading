/* Copyright (C) 2023 Gleb Bezborodov - All Rights Reserved
 * You may use, distribute and modify this code under the
 * terms of the MIT license.
 *
 * You should have received a copy of the MIT license with
 * this file. If not, please write to: bezborodoff.gleb@gmail.com, or visit : https://github.com/glensand/jerk-thread
 */

#pragma once

#include <array>
#include <vector>
#include <list>
#include <atomic>
#include <mutex>
#include <shared_mutex>

#include "jerk-thread/synchronization/spinlock.h"

namespace jt {

    template<typename TValue>
    struct trivial_equal_operator final {
        bool operator()(const TValue& a, const TValue& b) const noexcept { 
            return a == b;
        }
    };

    // TODO:: add allocator
    template<typename TValue,
        typename TValueAssigner,
        typename TKeyExtractor,
        typename THasher = std::hash<TValue>,
        typename TEqual = trivial_equal_operator<TValue>,
        typename TMutex = rw_spinlock,
        template <typename> typename TExclusiveLock = std::unique_lock,
        template <typename> typename TSharedLock = std::shared_lock,
        std::size_t BucketsCount = 8,
        std::size_t ResizeFactor = 2
    >
    class hash_storage final {
        static_assert((BucketsCount & (BucketsCount - 1)) == 0);
    public: 
        using shared_lock_t = TSharedLock<TMutex>;
        using exclusive_lock_t = TExclusiveLock<TMutex>;

        using collision_list_t = std::list<TValue>;
        using bucket_t = std::vector<collision_list_t>;

        struct lockable_bucket final {
            bucket_t bucket;
            mutable TMutex guard;
            bool changed_while_resize = true;
        };

        using storage_t = std::array<lockable_bucket, BucketsCount>;
    
        hash_storage() {
            for (auto&& b : m_storage)
                b.bucket.resize(1);
        }

        template<typename... Ts>
        bool emplace(Ts&&... value) {
            auto&& key = m_key_extractor(value...);
            const std::size_t hash = m_hasher(key);
            auto&& bucket = m_storage[hash % BucketsCount];
            // todo:: flat-combining?
            bool new_element = true;
            {
                const exclusive_lock_t lock(bucket.guard);
                const std::size_t sub_bucket_id = hash % bucket.bucket.size();
                auto&& collision_list = bucket.bucket[sub_bucket_id];
                auto&& found = std::find_if(begin(collision_list), end(collision_list), 
                [this, &key] (const TValue& candidate){
                    return m_equal(m_key_extractor(candidate), key);
                });

                if (found != end(collision_list)){
                    new_element = false;
                    m_value_assigner(*found, std::forward<Ts>(value)...); // renew the value
                } else {
                    collision_list.emplace_front(std::forward<Ts>(value)...);
                    ++m_size;
                }
                bucket.changed_while_resize = true;
            }

            resize();

            return new_element;
        }

        bool obtain(TValue& value) const noexcept {
            const std::size_t hash = m_hasher(value);
            auto&& bucket = m_storage[hash % BucketsCount];
            const shared_lock_t lock(bucket.guard);
            auto* obtained = find_locked(hash, value, bucket.bucket);
            if (obtained)
                value = *obtained;
            return obtained != nullptr;
        }

        template<typename TKey>
        const TValue* find(const TKey& value) const noexcept {
            const std::size_t hash = m_hasher(value);
            auto&& bucket = m_storage[hash % BucketsCount];
            const shared_lock_t lock(bucket.guard);
            return find_locked(hash, value, bucket.bucket);
        }

        template<typename TKey>
        void remove(const TKey& value) noexcept {
            const std::size_t hash = m_hasher(value);
            auto&& bucket = m_storage[hash % BucketsCount];
            const exclusive_lock_t lock(bucket.guard);
            auto&& collision_list = bucket.bucket[hash % bucket.bucket.size()];
            const auto prev_size = collision_list.size();
            collision_list.remove_if([this, &value] (const TValue& v){
                return m_equal(v, value);
            });
            bucket.changed_while_resize = true;
            if (prev_size != collision_list.size())
                --m_size;
        }

        template<typename TKey>
        void lock_bucket(const TKey& key, bool shared = false) noexcept {
            auto&& bucket = m_storage[m_hash(key) % BucketsCount];
            if (shared)
                bucket.lock_shared();
            else
                bucket.lock();
        }

        template<typename TKey>
        void unlock_bucket(const TKey& key, bool shared = false) noexcept {
            auto&& bucket = m_storage[m_hash(key) % BucketsCount];
            if (shared)
                bucket.unlock_shared();
            else
                bucket.unlock();
        }

        std::size_t size() const noexcept { return m_size.load(std::memory_order_acquire); }
        std::size_t capacity() const noexcept { return m_capacity.load(std::memory_order_acquire); }

    private:
        const TValue* find_locked(std::size_t hash, const TValue& value, const bucket_t& bucket) const {
            auto&& collision_list = bucket[hash % bucket.size()];
            auto&& found = std::find_if(begin(collision_list), end(collision_list), 
            [this, &value] (const TValue& candidate){
                return m_equal(candidate, value);
            });
            return found != end(collision_list) ? &(*found) : nullptr;
        }

        void resize(){
            if (m_resizing.load(std::memory_order_acquire) || 
                m_size.load(std::memory_order_acquire) * ResizeFactor < m_capacity.load(std::memory_order_acquire))
                    return;

            bool resizing = false;
            if (!m_resizing.compare_exchange_strong(resizing, true, 
                    std::memory_order_acquire, std::memory_order_relaxed))
                return;
            
            const std::size_t bucket_capacity = m_capacity * ResizeFactor / BucketsCount;
            for (auto&& bucket : m_storage) {
                bool copied = false;
                while(!copied) {
                    // TODO:: probably better to add something like flat-combining
                    // and do not try to resize the bucket inside the loop, but instead share the value 
                    // via the active resizing context or something like that
                    bucket_t new_bucket;
                    new_bucket.resize(bucket_capacity);

                    // prepare bucket for swap
                    {
                        const shared_lock_t lock(bucket.guard);
                        bucket.changed_while_resize = false;
                        for (auto&& collision_list : bucket.bucket) {
                            for (auto&& value : collision_list) {
                                auto&& new_collision_list = new_bucket[m_hasher(value) % bucket_capacity];
                                new_collision_list.push_front(value);
                            }
                        }
                    }

                    // try to swap buckets
                    {
                        const exclusive_lock_t lock(bucket.guard);
                        if (!bucket.changed_while_resize) {
                            bucket.bucket = std::move(new_bucket);
                            copied = true;
                        }
                        // otherwise shit happened, we should try one more time
                    }
                }
            }

            m_capacity = BucketsCount * bucket_capacity;
            m_resizing.store(false, std::memory_order_release);
        }

        std::atomic_bool m_resizing;
        std::atomic<std::size_t> m_size = 0u;
        std::atomic<std::size_t> m_capacity = BucketsCount;

        TKeyExtractor m_key_extractor;
        TValueAssigner m_value_assigner;
        THasher m_hasher;
        TEqual m_equal;
        storage_t m_storage;
    };

}