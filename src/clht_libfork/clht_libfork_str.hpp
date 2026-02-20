#pragma once

/**
 * CLHT String Key with libfork Parallel Operations
 * 
 * Provides parallel batch operations using libfork's work-stealing scheduler:
 * - Parallel batch insert
 * - Parallel batch lookup
 * - Parallel batch remove
 */

#include "clht_str_final.hpp"
#include "libfork/core.hpp"
#include "libfork/schedule.hpp"
#include <vector>
#include <span>
#include <thread>

namespace clht_libfork {

namespace impl {

/**
 * @brief Overload for parallel insert
 */
struct insert_str_overload {
    template <typename Keys, typename Values>
    LF_STATIC_CALL auto operator()(auto insert_str, 
                                    clht_str::HashtableFinal* ht,
                                    Keys keys,
                                    Values values) LF_STATIC_CONST -> lf::task<> {
        constexpr size_t THRESHOLD = 32;
        
        if (keys.size() <= THRESHOLD) {
            // Base case: serial processing
            for (size_t i = 0; i < keys.size(); ++i) {
                clht_str::hashtable_final_put(ht, 
                    static_cast<std::string>(keys[i]).c_str(), 
                    static_cast<std::string>(keys[i]).size(), 
                    values[i]);
            }
            co_return;
        }
        
        // Split task
        size_t mid = keys.size() / 2;
        
        co_await lf::fork(insert_str)(ht, 
            std::span(keys.begin(), mid), 
            std::span(values.begin(), mid));
        co_await lf::call(insert_str)(ht, 
            std::span(keys.begin() + mid, keys.end()), 
            std::span(values.begin() + mid, values.end()));
        
        co_await lf::join;
    }
};

/**
 * @brief Overload for parallel lookup
 */
struct lookup_str_overload {
    template <typename Keys, typename Results>
    LF_STATIC_CALL auto operator()(auto lookup_str,
                                    clht_str::HashtableFinal* ht,
                                    Keys keys,
                                    Results results) LF_STATIC_CONST -> lf::task<> {
        constexpr size_t THRESHOLD = 64;
        
        if (keys.size() <= THRESHOLD) {
            // Base case: serial processing
            for (size_t i = 0; i < keys.size(); ++i) {
                results[i] = clht_str::hashtable_final_get(ht, 
                    static_cast<std::string>(keys[i]).c_str(), 
                    static_cast<std::string>(keys[i]).size());
            }
            co_return;
        }
        
        // Split task
        size_t mid = keys.size() / 2;
        
        co_await lf::fork(lookup_str)(ht, 
            std::span(keys.begin(), mid), 
            std::span(results.begin(), mid));
        co_await lf::call(lookup_str)(ht, 
            std::span(keys.begin() + mid, keys.end()), 
            std::span(results.begin() + mid, results.end()));
        
        co_await lf::join;
    }
};

/**
 * @brief Overload for parallel remove
 */
struct remove_str_overload {
    template <typename Keys, typename Results>
    LF_STATIC_CALL auto operator()(auto remove_str,
                                    clht_str::HashtableFinal* ht,
                                    Keys keys,
                                    Results results) LF_STATIC_CONST -> lf::task<> {
        constexpr size_t THRESHOLD = 32;
        
        if (keys.size() <= THRESHOLD) {
            // Base case: serial processing
            for (size_t i = 0; i < keys.size(); ++i) {
                results[i] = clht_str::hashtable_final_remove(ht, 
                    static_cast<std::string>(keys[i]).c_str(), 
                    static_cast<std::string>(keys[i]).size());
            }
            co_return;
        }
        
        // Split task
        size_t mid = keys.size() / 2;
        
        co_await lf::fork(remove_str)(ht, 
            std::span(keys.begin(), mid), 
            std::span(results.begin(), mid));
        co_await lf::call(remove_str)(ht, 
            std::span(keys.begin() + mid, keys.end()), 
            std::span(results.begin() + mid, results.end()));
        
        co_await lf::join;
    }
};

// Global function objects
inline constexpr insert_str_overload insert_str = {};
inline constexpr lookup_str_overload lookup_str = {};
inline constexpr remove_str_overload remove_str = {};

} // namespace impl

/**
 * Parallel CLHT String Key Implementation
 */
class ParallelClhtStr {
public:
    explicit ParallelClhtStr(size_t capacity = 1024, size_t num_threads = 0)
        : capacity_(capacity) {
        if (num_threads == 0) {
            num_threads = std::thread::hardware_concurrency();
        }
        num_threads_ = num_threads;
        pool_ = std::make_unique<lf::lazy_pool>(num_threads);
        
        allocator_ = new clht_str::StringAllocator();
        ht_ = clht_str::hashtable_final_create(capacity, allocator_);
    }
    
    ~ParallelClhtStr() {
        clht_str::hashtable_final_destroy(ht_);
        delete allocator_;
    }
    
    // Disable copy
    ParallelClhtStr(const ParallelClhtStr&) = delete;
    ParallelClhtStr& operator=(const ParallelClhtStr&) = delete;
    
    // Single operations (serial)
    bool insert(const std::string& key, uintptr_t value) {
        return clht_str::hashtable_final_put(ht_, key.c_str(), key.size(), value);
    }
    
    uintptr_t lookup(const std::string& key) {
        return clht_str::hashtable_final_get(ht_, key.c_str(), key.size());
    }
    
    bool remove(const std::string& key) {
        return clht_str::hashtable_final_remove(ht_, key.c_str(), key.size());
    }
    
    size_t size() const {
        return ht_->num_elements.load(std::memory_order_relaxed);
    }
    
    // Parallel batch operations
    void batch_insert(const std::vector<std::string>& keys, 
                      const std::vector<uintptr_t>& values);
    
    void batch_lookup(const std::vector<std::string>& keys,
                      std::vector<uintptr_t>& results);
    
    void batch_remove(const std::vector<std::string>& keys,
                      std::vector<bool>& results);
    
    // Parallel mixed workload
    void batch_mixed(const std::vector<std::string>& keys,
                     const std::vector<uintptr_t>& values,
                     std::vector<uintptr_t>& results,
                     double insert_ratio = 0.2);
    
private:
    clht_str::HashtableFinal* ht_;
    clht_str::StringAllocator* allocator_;
    size_t capacity_;
    size_t num_threads_;
    std::unique_ptr<lf::lazy_pool> pool_;
};

inline void ParallelClhtStr::batch_insert(const std::vector<std::string>& keys,
                                          const std::vector<uintptr_t>& values) {
    if (keys.empty()) return;
    
    lf::sync_wait(*pool_, impl::insert_str, ht_,
                  std::span<const std::string>(keys), 
                  std::span<const uintptr_t>(values));
}

inline void ParallelClhtStr::batch_lookup(const std::vector<std::string>& keys,
                                          std::vector<uintptr_t>& results) {
    if (keys.empty()) return;
    
    results.resize(keys.size());
    lf::sync_wait(*pool_, impl::lookup_str, ht_,
                  std::span<const std::string>(keys), 
                  std::span<uintptr_t>(results));
}

inline void ParallelClhtStr::batch_remove(const std::vector<std::string>& keys,
                                          std::vector<bool>& results) {
    if (keys.empty()) return;
    
    results.resize(keys.size());
    // Convert bool vector to uintptr_t for processing
    std::vector<uintptr_t> int_results(keys.size());
    lf::sync_wait(*pool_, impl::remove_str, ht_,
                  std::span<const std::string>(keys), 
                  std::span<uintptr_t>(int_results));
    for (size_t i = 0; i < int_results.size(); ++i) {
        results[i] = int_results[i] != 0;
    }
}

inline void ParallelClhtStr::batch_mixed(const std::vector<std::string>& keys,
                                         const std::vector<uintptr_t>& values,
                                         std::vector<uintptr_t>& results,
                                         double insert_ratio) {
    results.resize(keys.size());
    
    // Simple mixed workload: insert first N%, lookup rest
    size_t insert_count = static_cast<size_t>(keys.size() * insert_ratio);
    
    if (insert_count > 0) {
        batch_insert(
            std::vector<std::string>(keys.begin(), keys.begin() + insert_count),
            std::vector<uintptr_t>(values.begin(), values.begin() + insert_count)
        );
    }
    
    if (insert_count < keys.size()) {
        batch_lookup(
            std::vector<std::string>(keys.begin() + insert_count, keys.end()),
            results
        );
    }
}

} // namespace clht_libfork
