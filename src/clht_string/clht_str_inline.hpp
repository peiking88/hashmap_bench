#pragma once

/**
 * CLHT String Key Support - Approach B: Fixed-Length Inline Storage
 * 
 * Implementation Strategy:
 * - Store string directly in bucket (max 16 bytes by default)
 * - Hash value first for fast comparison
 * - No external memory allocation needed
 * 
 * Pros:
 * - Best cache locality (all data in bucket)
 * - No memory allocation overhead
 * - Simple memory management
 * 
 * Cons:
 * - Limited key length (configurable)
 * - Wasted space for short strings
 * - Longer strings must be truncated or rejected
 */

#include "clht_str_common.hpp"
#include <atomic>
#include <cassert>
#include <cstring>

namespace clht_str {

// ============================================================================
// Configuration
// ============================================================================

// Maximum inline string length (must be power of 2 for alignment)
// 16 bytes is a good balance for short strings
#ifndef INLINE_STR_MAX_LEN
#define INLINE_STR_MAX_LEN 16
#endif

// Number of entries per bucket (tuned for 64-byte cache line)
#define INLINE_ENTRIES_PER_BUCKET 2

// ============================================================================
// Bucket Structure (128 bytes for better inline storage)
// ============================================================================

struct alignas(128) BucketInline {
    // Lock and metadata
    volatile uint8_t lock;                              // 1 byte
    uint8_t _pad1[7];                                   // 7 bytes
    
    // Entry array - each entry contains hash + inline string + value
    struct Entry {
        uint64_t hash;                                  // 8 bytes
        char key[INLINE_STR_MAX_LEN];                   // 16 bytes
        uint8_t length;                                 // 1 byte
        uint8_t _pad[7];                                // 7 bytes
        volatile uintptr_t value;                       // 8 bytes
    };  // 40 bytes per entry
    
    Entry entries[INLINE_ENTRIES_PER_BUCKET];           // 80 bytes
    
    // Overflow chain pointer
    BucketInline* next;                                 // 8 bytes
    uint8_t _pad2[8];                                   // 8 bytes
};  // Total: 104 bytes, padded to 128 for alignment

// ============================================================================
// Hash Table Structure
// ============================================================================

struct HashtableInline {
    BucketInline* table;
    size_t size;
    size_t mask;
    std::atomic<size_t> num_elements;
    std::atomic<bool> resizing;
};

// ============================================================================
// Helper Functions
// ============================================================================

inline void copy_key_inline(char* dest, const char* src, size_t len) {
    // Zero out destination first
    std::memset(dest, 0, INLINE_STR_MAX_LEN);
    
    // Copy source
    size_t copy_len = (len < INLINE_STR_MAX_LEN) ? len : INLINE_STR_MAX_LEN;
    std::memcpy(dest, src, copy_len);
}

inline bool compare_key_inline(const char* stored, const char* key, size_t len, size_t stored_len) {
    if (len != stored_len) return false;
    if (len > INLINE_STR_MAX_LEN) len = INLINE_STR_MAX_LEN;
    return strcmp_simd(stored, key, len) == 0;
}

// ============================================================================
// Lock Operations (same as Approach A)
// ============================================================================

inline bool lock_acquire_inline(BucketInline* bucket) {
    uint8_t expected = LOCK_FREE;
    while (!__atomic_compare_exchange_n(&bucket->lock, &expected, LOCK_UPDATE,
                                         false, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED)) {
        if (expected == LOCK_RESIZE) return false;
        expected = LOCK_FREE;
        _mm_pause();
    }
    return true;
}

inline void lock_release_inline(BucketInline* bucket) {
    __atomic_store_n(&bucket->lock, LOCK_FREE, __ATOMIC_RELEASE);
}

// ============================================================================
// API Functions
// ============================================================================

inline HashtableInline* hashtable_inline_create(size_t capacity) {
    HashtableInline* ht = new HashtableInline{};
    
    size_t size = 1;
    while (size < capacity / INLINE_ENTRIES_PER_BUCKET) {
        size <<= 1;
    }
    
    ht->table = static_cast<BucketInline*>(
        std::aligned_alloc(128, size * sizeof(BucketInline)));
    std::memset(ht->table, 0, size * sizeof(BucketInline));
    
    ht->size = size;
    ht->mask = size - 1;
    ht->resizing = false;
    
    return ht;
}

inline void hashtable_inline_destroy(HashtableInline* ht) {
    // Free overflow buckets
    for (size_t i = 0; i < ht->size; ++i) {
        BucketInline* bucket = ht->table[i].next;
        while (bucket) {
            BucketInline* next = bucket->next;
            std::free(bucket);
            bucket = next;
        }
    }
    
    std::free(ht->table);
    delete ht;
}

/**
 * Insert - returns true if key was inserted or updated
 * Note: Keys longer than INLINE_STR_MAX_LEN will be truncated
 */
inline bool hashtable_inline_put(HashtableInline* ht, const char* key, size_t len, uintptr_t val) {
    if (len > INLINE_STR_MAX_LEN) {
        // Option 1: Reject long keys
        // return false;
        
        // Option 2: Truncate (current implementation)
        len = INLINE_STR_MAX_LEN;
    }
    
    uint64_t hash = hash_string(key, len);
    size_t bucket_idx = hash & ht->mask;
    
    BucketInline* bucket = &ht->table[bucket_idx];
    
    if (!lock_acquire_inline(bucket)) {
        return false;
    }
    
    // First pass: check for existing key
    BucketInline* curr = bucket;
    while (curr) {
        for (int i = 0; i < INLINE_ENTRIES_PER_BUCKET; ++i) {
            if (curr->entries[i].hash == hash &&
                curr->entries[i].length == len &&
                compare_key_inline(curr->entries[i].key, key, len, len)) {
                // Key exists - update value
                curr->entries[i].value = val;
                lock_release_inline(bucket);
                return true;
            }
        }
        curr = curr->next;
    }
    
    // Second pass: find empty slot
    curr = bucket;
    while (curr) {
        for (int i = 0; i < INLINE_ENTRIES_PER_BUCKET; ++i) {
            if (curr->entries[i].hash == 0) {
                // Empty slot - insert here
                curr->entries[i].hash = hash;
                copy_key_inline(curr->entries[i].key, key, len);
                curr->entries[i].length = static_cast<uint8_t>(len);
                curr->entries[i].value = val;
                
                ht->num_elements.fetch_add(1, std::memory_order_relaxed);
                lock_release_inline(bucket);
                return true;
            }
        }
        
        if (!curr->next) {
            // Create overflow bucket
            curr->next = static_cast<BucketInline*>(
                std::aligned_alloc(128, sizeof(BucketInline)));
            std::memset(curr->next, 0, sizeof(BucketInline));
        }
        curr = curr->next;
    }
    
    lock_release_inline(bucket);
    return false;
}

/**
 * Lookup
 */
inline uintptr_t hashtable_inline_get(HashtableInline* ht, const char* key, size_t len) {
    if (len > INLINE_STR_MAX_LEN) {
        len = INLINE_STR_MAX_LEN;
    }
    
    uint64_t hash = hash_string(key, len);
    size_t bucket_idx = hash & ht->mask;
    
    BucketInline* bucket = &ht->table[bucket_idx];
    
    while (bucket) {
        for (int i = 0; i < INLINE_ENTRIES_PER_BUCKET; ++i) {
            if (bucket->entries[i].hash == hash) {
                if (bucket->entries[i].length == len &&
                    compare_key_inline(bucket->entries[i].key, key, len, len)) {
                    return bucket->entries[i].value;
                }
            }
            
            // Early termination
            if (bucket->entries[i].hash == 0) {
                return UINTPTR_MAX;
            }
        }
        
        bucket = bucket->next;
    }
    
    return UINTPTR_MAX;
}

/**
 * Remove
 */
inline bool hashtable_inline_remove(HashtableInline* ht, const char* key, size_t len) {
    if (len > INLINE_STR_MAX_LEN) {
        len = INLINE_STR_MAX_LEN;
    }
    
    uint64_t hash = hash_string(key, len);
    size_t bucket_idx = hash & ht->mask;
    
    BucketInline* const head_bucket = &ht->table[bucket_idx];
    
    if (!lock_acquire_inline(head_bucket)) {
        return false;
    }
    
    BucketInline* bucket = head_bucket;
    while (bucket) {
        for (int i = 0; i < INLINE_ENTRIES_PER_BUCKET; ++i) {
            if (bucket->entries[i].hash == hash &&
                bucket->entries[i].length == len &&
                compare_key_inline(bucket->entries[i].key, key, len, len)) {
                
                // Mark as deleted
                bucket->entries[i].hash = 0;
                bucket->entries[i].length = 0;
                bucket->entries[i].value = 0;
                
                ht->num_elements.fetch_sub(1, std::memory_order_relaxed);
                lock_release_inline(head_bucket);
                return true;
            }
        }
        
        bucket = bucket->next;
    }
    
    lock_release_inline(head_bucket);
    return false;
}

// ============================================================================
// C++ Wrapper
// ============================================================================

class ClhtStrInline {
public:
    explicit ClhtStrInline(size_t capacity = 1024) {
        ht_ = hashtable_inline_create(capacity);
    }
    
    ~ClhtStrInline() {
        hashtable_inline_destroy(ht_);
    }
    
    // Returns false if key is too long and truncation is disabled
    bool insert(const std::string& key, uintptr_t value) {
        return hashtable_inline_put(ht_, key.c_str(), key.size(), value);
    }
    
    uintptr_t lookup(const std::string& key) {
        return hashtable_inline_get(ht_, key.c_str(), key.size());
    }
    
    bool remove(const std::string& key) {
        return hashtable_inline_remove(ht_, key.c_str(), key.size());
    }
    
    size_t size() const {
        return ht_->num_elements.load(std::memory_order_relaxed);
    }
    
    static constexpr size_t max_key_length() {
        return INLINE_STR_MAX_LEN;
    }
    
private:
    HashtableInline* ht_;
};

}  // namespace clht_str
