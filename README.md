# Hope Threading

`hope-threading` is a concurrency-focused part of the Hope ecosystem.

## NOTE:

This repository contains experimental and practical threading utilities.
Some modules are production-ready, some are still evolving. API details may change.

## Library purpose

This project collects lock-free containers, synchronization primitives, and runtime helpers
that are useful for daily C++ multithreaded development.

The library includes:

- lock-free queues (`SPSC`, `MPSC`, `MPMC`, bounded and unbounded variants)
- thread-safe hash containers (`hash_set`, `hash_map`)
- synchronization utilities (`spinlock`, rw-lock variants, events, backoff)
- safe object wrapper with customizable lock policies
- runtime helpers (`thread_pool`, `worker_thread`)

## System requirements

- C++17 compiler
- CMake 3.11 or newer
- STL only (no external runtime dependencies for the library itself)

## Build

```bash
git clone https://github.com/glensand/hope-threading.git
cd hope-threading
cmake -S . -B build
cmake --build build -j
```

Project layout:

- `lib/` header-only library target `hope_thread`
- `samples/` runnable examples
- `test/` assert-based test target (`hope-thread-test`)

## Run tests

```bash
cmake --build build --target hope-thread-test -j
ctest --test-dir build --output-on-failure
```

> Tests are self-contained and rely on standard C++ `assert`.

## Examples

### Thread pool

```cpp
#include "hope_thread/runtime/threadpool.h"

int main() {
    hope::threading::thread_pool pool(8);

    for (std::size_t i = 0; i < 1000; ++i) {
        pool.add_work([i] {
            // your task body
            (void)i;
        });
    }

    pool.destroy();
}
```

### MPMC bounded queue

```cpp
#include "hope_thread/containers/queue/mpmc_bounded_queue.h"

int main() {
    hope::threading::mpmc_bounded_queue<int> q(1024); // size must be power of two
    q.try_enqueue(42);

    int value = 0;
    if (q.try_dequeue(value)) {
        // value == 42
    }
}
```

### Thread-safe hash set

```cpp
#include "hope_thread/containers/hashmap/hash_set.h"
#include <string>

int main() {
    hope::threading::hash_set<std::string> set;
    set.emplace("task_1");
    auto* found = set.find("task_1");
    (void)found;
}
```

### Safe wrapper for shared objects

```cpp
#include "hope_thread/synchronization/object_safe_wrapper.h"
#include "hope_thread/synchronization/spinlock.h"
#include <shared_mutex>
#include <vector>

int main() {
    using safe_vector_t = hope::threading::object_safe_wrapper<
        std::vector<int>,
        hope::threading::recursive_rw_spinlock,
        std::unique_lock,
        std::shared_lock>;

    safe_vector_t safe_vector;
    safe_vector->push_back(7); // exclusive lock through proxy
}
```

### Contention-free RW lock

`contention_free_rw_lock` is a read-write lock optimized for very frequent reads.
Readers use per-slot counters (cache-line aligned), and writers wait until all active readers leave.

```cpp
#include "hope_thread/synchronization/contention_free_rw_lock.h"

int main() {
    // MaxProcNumber should match your expected maximum parallel execution slots.
    hope::threading::contention_free_rw_lock<64> rw_lock;

    // Reader path
    const std::size_t reader_slot = rw_lock.lock_shared();
    // read shared state...
    rw_lock.unlock_shared(reader_slot);

    // Writer path
    rw_lock.lock_exclusive();
    // modify shared state...
    rw_lock.unlock_exclusive();
}
```

## License

MIT License. See `LICENSE`.
