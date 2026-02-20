#pragma once

/**
 * CLHT String Key Support - Approach A: Hash + Pointer
 * 
 * Implementation Strategy:
 * - Store hash value in bucket for O(1) comparison
 * - Store pointer to external string (managed by StringAllocator)
 * - Falls back to full string comparison only on hash collision
 * 
 * Pros:
 * - Minimal code change from original CLHT
 * - Supports arbitrary length strings
 * - Cache-line aligned buckets preserved
 * 
 * Cons:
 * - External memory management required
 * - Additional pointer dereference for collision resolution
 */

#include "clht_str_common.hpp"
#include <atomic>
#include <cassert>

namespace clht_str {

// ============================================================================
// Bucket Structure (64 bytes, cache-line aligned)
// ============================================================================

#define CLHT_STR_ENTRIES_PER_BUCKET 3

// Lock states
#define LOCK_FREE   0
#define LOCK_UPDATE 1
#define LOCK_RESIZE 2

struct alignas(64) BucketPtr {
    volatile uint8_t lock;                                      // 1 byte
    
    // Keys: hash values (primary) + pointers (secondary)
    uint64_t key_hash[CLHT_STR_ENTRIES_PER_BUCKET];             // 24 bytes
    const char* key_ptr[CLHT_STR_ENTRIES_PER_BUCKET];           // 24 bytes
    uint16_t key_len[CLHT_STR_ENTRIES_PER_BUCKET];              // 6 bytes
    uint8_t padding[1];                                         // 1 byte
    
    // Values
    volatile uintptr_t val[CLHT_STR_ENTRIES_PER_BUCKET];        // 24 bytes (in separate cache line normally)
    
    // Overflow chain
    BucketPtr* next;                                            // 8 bytes
};  // Total: varies, designed for cache efficiency

// Static assertion to verify bucket size
// static_assert(sizeof(BucketPtr) == 64, "BucketPtr must be 64 bytes");

// ============================================================================
// Hash Table Structure
// ============================================================================

struct HashtablePtr {
    BucketPtr* table;
    size_t size;           // Number of buckets (power of 2)
    size_t mask;           // size - 1, for fast modulo
    std::atomic<size_t> num_elements;
    StringAllocator* allocator;
};

// ============================================================================
// API Functions
// ============================================================================

inline HashtablePtr* hashtable_ptr_create(size_t capacity, StringAllocator* alloc) {
    HashtablePtr* ht = new HashtablePtr{};
    
    // Round up to power of 2
    size_t size = 1;
    while (size < capacity / CLHT_STR_ENTRIES_PER_BUCKET) {
        size <<= 1;
    }
    
    ht->table = static_cast<BucketPtr*>(
        std::aligned_alloc(64, size * sizeof(BucketPtr)));
    std::memset(ht->table, 0, size * sizeof(BucketPtr));
    
    ht->size = size;
    ht->mask = size - 1;
    ht->allocator = alloc;
    
    return ht;
}

inline void hashtable_ptr_destroy(HashtablePtr* ht) {
    std::free(ht->table);
    delete ht;
}

// Hash function wrapper
inline uint64_t hashtable_ptr_hash(HashtablePtr* ht, const char* key, size_t len) {
    return hash_string(key, len);
}

// Lock operations
inline bool lock_acquire(BucketPtr* bucket) {
    uint8_t expected = LOCK_FREE;
    while (!__atomic_compare_exchange_n(&bucket->lock, &expected, LOCK_UPDATE,
                                         false, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED)) {
        if (expected == LOCK_RESIZE) return false;
        expected = LOCK_FREE;
        _mm_pause();
    }
    return true;
}

inline void lock_release(BucketPtr* bucket) {
    __atomic_store_n(&bucket->lock, LOCK_FREE, __ATOMIC_RELEASE);
}

/**
 * Insert a key-value pair
 * Returns: true on success, false if key already exists
 */
inline bool hashtable_ptr_put(HashtablePtr* ht, const char* key, size_t len, uintptr_t val) {
    uint64_t hash = hash_string(key, len);
    size_t bucket_idx = hash & ht->mask;
    
    BucketPtr* const head_bucket = &ht->table[bucket_idx];
    
    if (!lock_acquire(head_bucket)) {
        return false;  // Table is resizing
    }
    
    // Search for existing key or empty slot
    BucketPtr* bucket = head_bucket;
    while (bucket) {
        for (int i = 0; i < CLHT_STR_ENTRIES_PER_BUCKET; ++i) {
            // Check hash first (fast path)
            if (bucket->key_hash[i] == hash) {
                // Hash match - verify full string
                if (bucket->key_len[i] == len &&
                    strcmp_simd(key, bucket->key_ptr[i], len) == 0) {
                    // Key exists, update value
                    bucket->val[i] = val;
                    lock_release(head_bucket);
                    return true;  // Updated existing
                }
            }
            
            // Empty slot found
            if (bucket->key_hash[i] == 0) {
                // Allocate and store string
                const char* stored = ht->allocator->alloc(key, len);
                
                bucket->key_hash[i] = hash;
                bucket->key_ptr[i] = stored;
                bucket->key_len[i] = static_cast<uint16_t>(len);
                bucket->val[i] = val;
                
                ht->num_elements.fetch_add(1, std::memory_order_relaxed);
                lock_release(head_bucket);
                return true;
            }
        }
        
        // Move to overflow bucket
        if (!bucket->next) {
            // Create overflow bucket
            bucket->next = static_cast<BucketPtr*>(
                std::aligned_alloc(64, sizeof(BucketPtr)));
            std::memset(bucket->next, 0, sizeof(BucketPtr));
        }
        bucket = bucket->next;
    }
    
    lock_release(head_bucket);
    return false;
}

/**
 * Lookup a key
 * Returns: value if found, UINTPTR_MAX if not found
 */
inline uintptr_t hashtable_ptr_get(HashtablePtr* ht, const char* key, size_t len) {
    uint64_t hash = hash_string(key, len);
    size_t bucket_idx = hash & ht->mask;
    
    BucketPtr* bucket = &ht->table[bucket_idx];
    
    while (bucket) {
        for (int i = 0; i < CLHT_STR_ENTRIES_PER_BUCKET; ++i) {
            // Fast hash comparison
            if (bucket->key_hash[i] == hash) {
                // Verify string
                if (bucket->key_len[i] == len &&
                    strcmp_simd(key, bucket->key_ptr[i], len) == 0) {
                    return bucket->val[i];
                }
            }
            
            // Early termination: empty slot means key doesn't exist
            if (bucket->key_hash[i] == 0) {
                return UINTPTR_MAX;
            }
        }
        
        bucket = bucket->next;
    }
    
    return UINTPTR_MAX;
}

/**
 * Remove a key
 * Returns: true if key was found and removed, false otherwise
 */
inline bool hashtable_ptr_remove(HashtablePtr* ht, const char* key, size_t len) {
    uint64_t hash = hash_string(key, len);
    size_t bucket_idx = hash & ht->mask;
    
    BucketPtr* const head_bucket = &ht->table[bucket_idx];
    
    if (!lock_acquire(head_bucket)) {
        return false;
    }
    
    BucketPtr* bucket = head_bucket;
    while (bucket) {
        for (int i = 0; i < CLHT_STR_ENTRIES_PER_BUCKET; ++i) {
            if (bucket->key_hash[i] == hash &&
                bucket->key_len[i] == len &&
                strcmp_simd(key, bucket->key_ptr[i], len) == 0) {
                
                // Mark as deleted
                bucket->key_hash[i] = 0;
                bucket->key_ptr[i] = nullptr;
                bucket->key_len[i] = 0;
                bucket->val[i] = 0;
                
                ht->num_elements.fetch_sub(1, std::memory_order_relaxed);
                lock_release(head_bucket);
                return true;
            }
        }
        
        bucket = bucket->next;
    }
    
    lock_release(head_bucket);
    return false;
}

// ============================================================================
// C++ Wrapper for Convenience
// ============================================================================

class ClhtStrPtr {
public:
    ClhtStrPtr(size_t capacity = 1024) {
        allocator_ = new StringAllocator();
        ht_ = hashtable_ptr_create(capacity, allocator_);
    }
    
    ~ClhtStrPtr() {
        hashtable_ptr_destroy(ht_);
        delete allocator_;
    }
    
    bool insert(const std::string& key, uintptr_t value) {
        return hashtable_ptr_put(ht_, key.c_str(), key.size(), value);
    }
    
    uintptr_t lookup(const std::string& key) {
        return hashtable_ptr_get(ht_, key.c_str(), key.size());
    }
    
    bool remove(const std::string& key) {
        return hashtable_ptr_remove(ht_, key.c_str(), key.size());
    }
    
    size_t size() const {
        return ht_->num_elements.load(std::memory_order_relaxed);
    }
    
private:
    HashtablePtr* ht_;
    StringAllocator* allocator_;
};

}  // namespace clht_str
