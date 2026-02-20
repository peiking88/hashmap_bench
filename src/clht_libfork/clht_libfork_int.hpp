#pragma once

/**
 * CLHT Integer Key with libfork Parallel Operations
 * 
 * Provides parallel batch operations using libfork's work-stealing scheduler:
 * - Parallel batch insert
 * - Parallel batch lookup
 * - Parallel batch remove
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

namespace impl {

/**
 * @brief Overload for parallel insert (integer keys)
 */
struct insert_int_overload {
    LF_STATIC_CALL auto operator()(auto insert_int,
                                    clht_t* ht,
                                    std::span<const uintptr_t> keys,
                                    std::span<const uintptr_t> values) LF_STATIC_CONST -> lf::task<> {
        constexpr size_t THRESHOLD = 64;
        
        if (keys.size() <= THRESHOLD) {
            // Base case: serial processing
            for (size_t i = 0; i < keys.size(); ++i) {
                clht_put(ht, keys[i], values[i]);
            }
            co_return;
        }
        
        // Split task
        size_t mid = keys.size() / 2;
        
        co_await lf::fork(insert_int)(ht, 
            std::span<const uintptr_t>(keys.begin(), mid), 
            std::span<const uintptr_t>(values.begin(), mid));
        co_await lf::call(insert_int)(ht, 
            std::span<const uintptr_t>(keys.begin() + mid, keys.end()), 
            std::span<const uintptr_t>(values.begin() + mid, values.end()));
        
        co_await lf::join;
    }
};

/**
 * @brief Overload for parallel lookup (integer keys)
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

/**
 * @brief Overload for parallel remove (integer keys)
 */
struct remove_int_overload {
    LF_STATIC_CALL auto operator()(auto remove_int,
                                    clht_t* ht,
                                    std::span<const uintptr_t> keys,
                                    std::span<uintptr_t> results) LF_STATIC_CONST -> lf::task<> {
        constexpr size_t THRESHOLD = 64;
        
        if (keys.size() <= THRESHOLD) {
            // Base case: serial processing
            for (size_t i = 0; i < keys.size(); ++i) {
                results[i] = clht_remove(ht, keys[i]);
            }
            co_return;
        }
        
        // Split task
        size_t mid = keys.size() / 2;
        
        co_await lf::fork(remove_int)(ht, 
            std::span<const uintptr_t>(keys.begin(), mid), 
            std::span<uintptr_t>(results.begin(), mid));
        co_await lf::call(remove_int)(ht, 
            std::span<const uintptr_t>(keys.begin() + mid, keys.end()), 
            std::span<uintptr_t>(results.begin() + mid, results.end()));
        
        co_await lf::join;
    }
};

// Global function objects
inline constexpr insert_int_overload insert_int = {};
inline constexpr lookup_int_overload lookup_int = {};
inline constexpr remove_int_overload remove_int = {};

} // namespace impl

/**
 * Parallel CLHT Integer Key Implementation
 */
class ParallelClhtInt {
public:
    explicit ParallelClhtInt(size_t capacity = 1024, size_t num_threads = 0)
        : capacity_(capacity) {
        if (num_threads == 0) {
            num_threads = std::thread::hardware_concurrency();
        }
        num_threads_ = num_threads;
        pool_ = std::make_unique<lf::lazy_pool>(num_threads);
        
        ht_ = clht_create(capacity);
        
        // Initialize GC for each thread (required for CLHT)
        for (size_t i = 0; i < num_threads; ++i) {
            clht_gc_thread_init(ht_, static_cast<int>(i));
        }
    }
    
    ~ParallelClhtInt() {
        clht_gc_destroy(ht_);
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
    
    // Parallel batch operations
    void batch_insert(const std::vector<uintptr_t>& keys,
                      const std::vector<uintptr_t>& values);
    
    void batch_lookup(const std::vector<uintptr_t>& keys,
                      std::vector<uintptr_t>& results);
    
    void batch_remove(const std::vector<uintptr_t>& keys,
                      std::vector<uintptr_t>& results);
    
    // Parallel mixed workload
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
};

inline void ParallelClhtInt::batch_insert(const std::vector<uintptr_t>& keys,
                                          const std::vector<uintptr_t>& values) {
    if (keys.empty()) return;
    
    lf::sync_wait(*pool_, impl::insert_int, ht_,
                  std::span<const uintptr_t>(keys), 
                  std::span<const uintptr_t>(values));
}

inline void ParallelClhtInt::batch_lookup(const std::vector<uintptr_t>& keys,
                                          std::vector<uintptr_t>& results) {
    if (keys.empty()) return;
    
    results.resize(keys.size());
    lf::sync_wait(*pool_, impl::lookup_int, ht_,
                  std::span<const uintptr_t>(keys), 
                  std::span<uintptr_t>(results));
}

inline void ParallelClhtInt::batch_remove(const std::vector<uintptr_t>& keys,
                                          std::vector<uintptr_t>& results) {
    if (keys.empty()) return;
    
    results.resize(keys.size());
    lf::sync_wait(*pool_, impl::remove_int, ht_,
                  std::span<const uintptr_t>(keys), 
                  std::span<uintptr_t>(results));
}

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
