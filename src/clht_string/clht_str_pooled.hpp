#pragma once

/**
 * CLHT String Key Support - Approach C: External Key Pool
 * 
 * Implementation Strategy:
 * - Store hash + offset into shared string pool
 * - All strings stored in a contiguous memory region
 * - Compact bucket structure (16 bytes per key entry)
 * 
 * Pros:
 * - Supports arbitrary length strings
 * - Compact bucket storage
 * - Good memory locality for string access
 * - Can be persisted/mapped to file
 * 
 * Cons:
 * - Requires synchronization for pool allocation
 * - Fragmentation if keys are frequently deleted
 * - Pool resize is complex
 */

#include "clht_str_common.hpp"
#include <atomic>
#include <cassert>
#include <cstring>
#include <mutex>
#include <memory>

namespace clht_str {

// ============================================================================
// Configuration
// ============================================================================

#define POOLED_ENTRIES_PER_BUCKET 3

// Default pool size: 16MB
constexpr size_t DEFAULT_POOL_SIZE = 16 * 1024 * 1024;

// ============================================================================
// Key Pool with Thread-Safe Allocation
// ============================================================================

class ThreadSafeKeyPool {
public:
    explicit ThreadSafeKeyPool(size_t initial_size = DEFAULT_POOL_SIZE) {
        capacity_ = initial_size;
        data_ = static_cast<char*>(std::aligned_alloc(64, capacity_));
        offset_ = 0;
    }
    
    ~ThreadSafeKeyPool() {
        std::free(data_);
    }
    
    // Allocate string and return offset
    // Returns UINT32_MAX on failure
    uint32_t alloc(const char* str, size_t len) {
        size_t alloc_size = len + 1;  // Include null terminator
        alloc_size = (alloc_size + 7) & ~7ULL;  // 8-byte align
        
        size_t offset;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            
            if (offset_ + alloc_size > capacity_) {
                if (!expand(alloc_size)) {
                    return UINT32_MAX;
                }
            }
            
            offset = offset_;
            offset_ += alloc_size;
        }
        
        std::memcpy(data_ + offset, str, len);
        data_[offset + len] = '\0';
        
        return static_cast<uint32_t>(offset);
    }
    
    const char* get(uint32_t offset) const {
        if (offset >= offset_) {
            return nullptr;
        }
        return data_ + offset;
    }
    
    // Reset pool (for testing)
    void reset() {
        std::lock_guard<std::mutex> lock(mutex_);
        offset_ = 0;
    }
    
    size_t used() const { return offset_; }
    size_t capacity() const { return capacity_; }
    
private:
    bool expand(size_t needed) {
        size_t new_capacity = capacity_ * 2;
        while (new_capacity < offset_ + needed) {
            new_capacity *= 2;
        }
        
        char* new_data = static_cast<char*>(std::aligned_alloc(64, new_capacity));
        if (!new_data) return false;
        
        std::memcpy(new_data, data_, offset_);
        std::free(data_);
        
        data_ = new_data;
        capacity_ = new_capacity;
        return true;
    }
    
    char* data_;
    size_t capacity_;
    size_t offset_;
    std::mutex mutex_;
};

// ============================================================================
// Bucket Structure (64 bytes, cache-line aligned)
// ============================================================================

struct alignas(64) BucketPooled {
    volatile uint8_t lock;                                  // 1 byte
    uint8_t _pad1[7];                                       // 7 bytes
    
    // Compact entries: hash + offset + length
    struct KeyInfo {
        uint64_t hash;          // 8 bytes
        uint32_t offset;        // 4 bytes: offset in pool
        uint16_t length;        // 2 bytes: string length
        uint16_t _pad;          // 2 bytes
    };  // 16 bytes per key
    
    KeyInfo keys[POOLED_ENTRIES_PER_BUCKET];                 // 48 bytes
    
    volatile uintptr_t vals[POOLED_ENTRIES_PER_BUCKET];      // 24 bytes
    BucketPooled* next;                                      // 8 bytes
    uint8_t _pad2[8];                                        // 8 bytes
};  // ~104 bytes, aligned to 128

// ============================================================================
// Hash Table Structure
// ============================================================================

struct HashtablePooled {
    BucketPooled* table;
    size_t size;
    size_t mask;
    std::atomic<size_t> num_elements;
    ThreadSafeKeyPool* pool;
};

// ============================================================================
// Lock Operations
// ============================================================================

inline bool lock_acquire_pooled(BucketPooled* bucket) {
    uint8_t expected = LOCK_FREE;
    while (!__atomic_compare_exchange_n(&bucket->lock, &expected, LOCK_UPDATE,
                                         false, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED)) {
        if (expected == LOCK_RESIZE) return false;
        expected = LOCK_FREE;
        _mm_pause();
    }
    return true;
}

inline void lock_release_pooled(BucketPooled* bucket) {
    __atomic_store_n(&bucket->lock, LOCK_FREE, __ATOMIC_RELEASE);
}

// ============================================================================
// API Functions
// ============================================================================

inline HashtablePooled* hashtable_pooled_create(size_t capacity, ThreadSafeKeyPool* pool) {
    HashtablePooled* ht = new HashtablePooled{};
    
    size_t size = 1;
    while (size < capacity / POOLED_ENTRIES_PER_BUCKET) {
        size <<= 1;
    }
    
    ht->table = static_cast<BucketPooled*>(
        std::aligned_alloc(64, size * sizeof(BucketPooled)));
    std::memset(ht->table, 0, size * sizeof(BucketPooled));
    
    ht->size = size;
    ht->mask = size - 1;
    ht->pool = pool;
    
    return ht;
}

inline void hashtable_pooled_destroy(HashtablePooled* ht) {
    // Free overflow buckets
    for (size_t i = 0; i < ht->size; ++i) {
        BucketPooled* bucket = ht->table[i].next;
        while (bucket) {
            BucketPooled* next = bucket->next;
            std::free(bucket);
            bucket = next;
        }
    }
    
    std::free(ht->table);
    delete ht;
}

/**
 * Insert with key pool storage
 */
inline bool hashtable_pooled_put(HashtablePooled* ht, const char* key, size_t len, uintptr_t val) {
    uint64_t hash = hash_string(key, len);
    size_t bucket_idx = hash & ht->mask;
    
    BucketPooled* bucket = &ht->table[bucket_idx];
    
    if (!lock_acquire_pooled(bucket)) {
        return false;
    }
    
    // First pass: check for existing key
    BucketPooled* curr = bucket;
    while (curr) {
        for (int i = 0; i < POOLED_ENTRIES_PER_BUCKET; ++i) {
            if (curr->keys[i].hash == hash) {
                // Potential match - verify string
                const char* stored = ht->pool->get(curr->keys[i].offset);
                if (stored && curr->keys[i].length == len &&
                    strcmp_simd(key, stored, len) == 0) {
                    // Key exists - update value
                    curr->vals[i] = val;
                    lock_release_pooled(bucket);
                    return true;
                }
            }
        }
        curr = curr->next;
    }
    
    // Key not found - allocate in pool
    uint32_t offset = ht->pool->alloc(key, len);
    if (offset == UINT32_MAX) {
        lock_release_pooled(bucket);
        return false;  // Pool allocation failed
    }
    
    // Second pass: find empty slot
    curr = bucket;
    while (curr) {
        for (int i = 0; i < POOLED_ENTRIES_PER_BUCKET; ++i) {
            if (curr->keys[i].hash == 0) {
                curr->keys[i].hash = hash;
                curr->keys[i].offset = offset;
                curr->keys[i].length = static_cast<uint16_t>(len);
                curr->vals[i] = val;
                
                ht->num_elements.fetch_add(1, std::memory_order_relaxed);
                lock_release_pooled(bucket);
                return true;
            }
        }
        
        if (!curr->next) {
            curr->next = static_cast<BucketPooled*>(
                std::aligned_alloc(64, sizeof(BucketPooled)));
            std::memset(curr->next, 0, sizeof(BucketPooled));
        }
        curr = curr->next;
    }
    
    lock_release_pooled(bucket);
    return false;
}

/**
 * Lookup
 */
inline uintptr_t hashtable_pooled_get(HashtablePooled* ht, const char* key, size_t len) {
    uint64_t hash = hash_string(key, len);
    size_t bucket_idx = hash & ht->mask;
    
    BucketPooled* bucket = &ht->table[bucket_idx];
    
    while (bucket) {
        for (int i = 0; i < POOLED_ENTRIES_PER_BUCKET; ++i) {
            if (bucket->keys[i].hash == hash) {
                const char* stored = ht->pool->get(bucket->keys[i].offset);
                if (stored && bucket->keys[i].length == len &&
                    strcmp_simd(key, stored, len) == 0) {
                    return bucket->vals[i];
                }
            }
            
            if (bucket->keys[i].hash == 0) {
                return UINTPTR_MAX;  // Early termination
            }
        }
        
        bucket = bucket->next;
    }
    
    return UINTPTR_MAX;
}

/**
 * Remove (note: doesn't reclaim pool memory)
 */
inline bool hashtable_pooled_remove(HashtablePooled* ht, const char* key, size_t len) {
    uint64_t hash = hash_string(key, len);
    size_t bucket_idx = hash & ht->mask;
    
    BucketPooled* const head_bucket = &ht->table[bucket_idx];
    
    if (!lock_acquire_pooled(head_bucket)) {
        return false;
    }
    
    BucketPooled* bucket = head_bucket;
    while (bucket) {
        for (int i = 0; i < POOLED_ENTRIES_PER_BUCKET; ++i) {
            if (bucket->keys[i].hash == hash) {
                const char* stored = ht->pool->get(bucket->keys[i].offset);
                if (stored && bucket->keys[i].length == len &&
                    strcmp_simd(key, stored, len) == 0) {
                    
                    // Mark as deleted (pool memory not reclaimed)
                    bucket->keys[i].hash = 0;
                    bucket->keys[i].offset = 0;
                    bucket->keys[i].length = 0;
                    bucket->vals[i] = 0;
                    
                    ht->num_elements.fetch_sub(1, std::memory_order_relaxed);
                    lock_release_pooled(head_bucket);
                    return true;
                }
            }
        }
        
        bucket = bucket->next;
    }
    
    lock_release_pooled(head_bucket);
    return false;
}

// ============================================================================
// C++ Wrapper
// ============================================================================

class ClhtStrPooled {
public:
    explicit ClhtStrPooled(size_t capacity = 1024, size_t pool_size = DEFAULT_POOL_SIZE) {
        pool_ = new ThreadSafeKeyPool(pool_size);
        ht_ = hashtable_pooled_create(capacity, pool_);
    }
    
    ~ClhtStrPooled() {
        hashtable_pooled_destroy(ht_);
        delete pool_;
    }
    
    bool insert(const std::string& key, uintptr_t value) {
        return hashtable_pooled_put(ht_, key.c_str(), key.size(), value);
    }
    
    uintptr_t lookup(const std::string& key) {
        return hashtable_pooled_get(ht_, key.c_str(), key.size());
    }
    
    bool remove(const std::string& key) {
        return hashtable_pooled_remove(ht_, key.c_str(), key.size());
    }
    
    size_t size() const {
        return ht_->num_elements.load(std::memory_order_relaxed);
    }
    
    size_t pool_used() const {
        return pool_->used();
    }
    
    size_t pool_capacity() const {
        return pool_->capacity();
    }
    
private:
    HashtablePooled* ht_;
    ThreadSafeKeyPool* pool_;
};

}  // namespace clht_str
