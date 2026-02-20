#pragma once

/**
 * CLHT Integer Key with libfork Parallel Operations (Optimized)
 *
 * Strategy:
 * - Insert/Remove: SERIAL (CLHT bucket locks limit parallel scaling)
 * - Lookup: PARALLEL (lock-free reads scale near-linearly)
 *
 * CRITICAL FIX: CLHT's GC uses global clht_alloc pointer, causing double-free
 * when multiple instances are created/destroyed. We use reference counting to
 * ensure only the last instance calls clht_gc_destroy.
 */

extern "C" {
#include "clht.h"
#include "ssmem.h"
}

#include "libfork/core.hpp"
#include "libfork/schedule.hpp"
#include <vector>
#include <span>
#include <thread>
#include <atomic>
#include <memory>

namespace clht_libfork {

// Global reference counter for CLHT instances
// CLHT's GC uses global clht_alloc, so we need to track instances
inline std::atomic<int>& get_clht_instance_count() {
    static std::atomic<int> count{0};
    return count;
}

// Flag to track if GC has been initialized for this thread
inline std::atomic<bool>& get_gc_initialized() {
    static std::atomic<bool> initialized{false};
    return initialized;
}

namespace impl {

/**
 * @brief Overload for PARALLEL lookup (lock-free reads)
 */
struct lookup_int_overload {
    LF_STATIC_CALL auto operator()(auto lookup_int,
                                    clht_t* ht,
                                    std::span<const uintptr_t> keys,
                                    std::span<uintptr_t> results) LF_STATIC_CONST -> lf::task<> {
        constexpr size_t THRESHOLD = 128;

        if (keys.size() <= THRESHOLD) {
            // Base case: serial processing
            for (size_t i = 0; i < keys.size(); ++i) {
                results[i] = clht_get(ht->ht, keys[i]);
            }
            co_return;
        }

        // Split task
        size_t mid = keys.size() / 2;

        co_await lf::fork(lookup_int)(ht,
            std::span<const uintptr_t>(keys.begin(), mid),
            std::span<uintptr_t>(results.begin(), mid));
        co_await lf::call(lookup_int)(ht,
            std::span<const uintptr_t>(keys.begin() + mid, keys.end()),
            std::span<uintptr_t>(results.begin() + mid, results.end()));

        co_await lf::join;
    }
};

// Global function object
inline constexpr lookup_int_overload lookup_int = {};

} // namespace impl

/**
 * Parallel CLHT Integer Key Implementation
 * 
 * Optimized: Insert/Remove are SERIAL, Lookup is PARALLEL
 * 
 * WARNING: Due to CLHT's global GC state, only ONE instance should exist
 * at a time. Creating multiple instances concurrently will cause memory issues.
 */
class ParallelClhtInt {
public:
    explicit ParallelClhtInt(size_t capacity = 1024, size_t num_threads = 0)
        : capacity_(capacity), owns_gc_(false) {
        if (num_threads == 0) {
            num_threads = std::thread::hardware_concurrency();
        }
        num_threads_ = num_threads;
        pool_ = std::make_unique<lf::lazy_pool>(num_threads);

        ht_ = clht_create(capacity);

        // Increment instance counter
        int prev_count = get_clht_instance_count().fetch_add(1);
        
        // Only initialize GC for the first instance
        // This avoids the double-free problem with clht_alloc
        if (prev_count == 0) {
            owns_gc_ = true;
            // Initialize GC for each thread (required for CLHT)
            for (size_t i = 0; i < num_threads; ++i) {
                clht_gc_thread_init(ht_, static_cast<int>(i));
            }
        }
    }

    ~ParallelClhtInt() {
        // Only destroy GC if we own it (first instance)
        // and we're the last instance being destroyed
        int new_count = get_clht_instance_count().fetch_sub(1) - 1;
        
        if (owns_gc_ && new_count == 0) {
            clht_gc_destroy(ht_);
        } else {
            // Just free the hashtable memory without full GC destroy
            // to avoid double-free of clht_alloc
            free(ht_);
        }
    }

    // Disable copy
    ParallelClhtInt(const ParallelClhtInt&) = delete;
    ParallelClhtInt& operator=(const ParallelClhtInt&) = delete;

    // Single operations (serial)
    bool insert(uintptr_t key, uintptr_t value) {
        return clht_put(ht_, key, value) != 0;
    }

    uintptr_t lookup(uintptr_t key) {
        return clht_get(ht_->ht, key);
    }

    uintptr_t remove(uintptr_t key) {
        return clht_remove(ht_, key);
    }

    size_t size() const {
        return clht_size(ht_->ht);
    }

    // SERIAL batch insert (bucket locks limit parallel scaling)
    void batch_insert(const std::vector<uintptr_t>& keys,
                      const std::vector<uintptr_t>& values);

    // PARALLEL batch lookup (lock-free reads)
    void batch_lookup(const std::vector<uintptr_t>& keys,
                      std::vector<uintptr_t>& results);

    // SERIAL batch remove (write operation)
    void batch_remove(const std::vector<uintptr_t>& keys,
                      std::vector<uintptr_t>& results);

    // Mixed workload: insert serial, lookup parallel
    void batch_mixed(const std::vector<uintptr_t>& keys,
                     const std::vector<uintptr_t>& values,
                     std::vector<uintptr_t>& results,
                     double insert_ratio = 0.2);

    // Get underlying CLHT pointer for comparison
    clht_t* get_underlying() { return ht_; }

private:
    clht_t* ht_;
    size_t capacity_;
    size_t num_threads_;
    std::unique_ptr<lf::lazy_pool> pool_;
    bool owns_gc_;  // True if this instance owns the GC initialization
};

// SERIAL insert - CLHT bucket locks limit parallel scaling
inline void ParallelClhtInt::batch_insert(const std::vector<uintptr_t>& keys,
                                          const std::vector<uintptr_t>& values) {
    if (keys.empty()) return;

    // Serial execution for insert
    for (size_t i = 0; i < keys.size(); ++i) {
        clht_put(ht_, keys[i], values[i]);
    }
}

// PARALLEL lookup - lock-free reads scale near-linearly
inline void ParallelClhtInt::batch_lookup(const std::vector<uintptr_t>& keys,
                                          std::vector<uintptr_t>& results) {
    if (keys.empty()) return;

    results.resize(keys.size());
    lf::sync_wait(*pool_, impl::lookup_int, ht_,
                  std::span<const uintptr_t>(keys),
                  std::span<uintptr_t>(results));
}

// SERIAL remove - write operation
inline void ParallelClhtInt::batch_remove(const std::vector<uintptr_t>& keys,
                                          std::vector<uintptr_t>& results) {
    if (keys.empty()) return;

    results.resize(keys.size());
    // Serial execution for remove
    for (size_t i = 0; i < keys.size(); ++i) {
        results[i] = clht_remove(ht_, keys[i]);
    }
}

// Mixed workload: insert serial, lookup parallel
inline void ParallelClhtInt::batch_mixed(const std::vector<uintptr_t>& keys,
                                         const std::vector<uintptr_t>& values,
                                         std::vector<uintptr_t>& results,
                                         double insert_ratio) {
    results.resize(keys.size());

    // Simple mixed workload: insert first N%, lookup rest
    size_t insert_count = static_cast<size_t>(keys.size() * insert_ratio);

    if (insert_count > 0) {
        batch_insert(
            std::vector<uintptr_t>(keys.begin(), keys.begin() + insert_count),
            std::vector<uintptr_t>(values.begin(), values.begin() + insert_count)
        );
    }

    if (insert_count < keys.size()) {
        batch_lookup(
            std::vector<uintptr_t>(keys.begin() + insert_count, keys.end()),
            results
        );
    }
}

} // namespace clht_libfork
