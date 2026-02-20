#pragma once

/**
 * CLHT String Key with libfork Parallel Operations (Optimized)
 *
 * Strategy:
 * - Insert/Remove: SERIAL (CLHT bucket locks limit parallel scaling)
 * - Lookup: PARALLEL (lock-free reads scale near-linearly)
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
 * @brief Overload for PARALLEL lookup (lock-free reads)
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

// Global function object
inline constexpr lookup_str_overload lookup_str = {};

} // namespace impl

/**
 * Parallel CLHT String Key Implementation
 * 
 * Optimized: Insert/Remove are SERIAL, Lookup is PARALLEL
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

    // SERIAL batch insert (bucket locks limit parallel scaling)
    void batch_insert(const std::vector<std::string>& keys,
                      const std::vector<uintptr_t>& values);

    // PARALLEL batch lookup (lock-free reads)
    void batch_lookup(const std::vector<std::string>& keys,
                      std::vector<uintptr_t>& results);

    // SERIAL batch remove (write operation)
    void batch_remove(const std::vector<std::string>& keys,
                      std::vector<bool>& results);

    // Mixed workload: insert serial, lookup parallel
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

// SERIAL insert - CLHT bucket locks limit parallel scaling
inline void ParallelClhtStr::batch_insert(const std::vector<std::string>& keys,
                                          const std::vector<uintptr_t>& values) {
    if (keys.empty()) return;

    // Serial execution for insert
    for (size_t i = 0; i < keys.size(); ++i) {
        clht_str::hashtable_final_put(ht_, keys[i].c_str(), keys[i].size(), values[i]);
    }
}

// PARALLEL lookup - lock-free reads scale near-linearly
inline void ParallelClhtStr::batch_lookup(const std::vector<std::string>& keys,
                                          std::vector<uintptr_t>& results) {
    if (keys.empty()) return;

    results.resize(keys.size());
    lf::sync_wait(*pool_, impl::lookup_str, ht_,
                  std::span<const std::string>(keys),
                  std::span<uintptr_t>(results));
}

// SERIAL remove - write operation
inline void ParallelClhtStr::batch_remove(const std::vector<std::string>& keys,
                                          std::vector<bool>& results) {
    if (keys.empty()) return;

    results.resize(keys.size());
    // Serial execution for remove
    for (size_t i = 0; i < keys.size(); ++i) {
        results[i] = clht_str::hashtable_final_remove(ht_, keys[i].c_str(), keys[i].size());
    }
}

// Mixed workload: insert serial, lookup parallel
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
