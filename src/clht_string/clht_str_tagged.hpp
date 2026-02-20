#pragma once

/**
 * CLHT String Key Support - Tag-Optimized Version
 * 
 * Based on CLHT-Str-Ptr with F14-style tag optimization:
 * - Adds 8-bit tag to each entry for SIMD filtering
 * - Tags stored contiguously in bucket header
 * - Preserves cache-line alignment
 * - Early exit optimization via overflow counting
 */

#include "clht_str_common.hpp"
#include <atomic>
#include <cassert>
#include <cstring>
#include <immintrin.h>

namespace clht_str {

// ============================================================================
// Configuration
// ============================================================================

constexpr size_t TAGGED_ENTRIES = 4;  // 4 entries per bucket
constexpr uint8_t EMPTY_TAG = 0;

// ============================================================================
// SIMD Tag Matching
// ============================================================================

inline uint32_t match_tags_4(const uint8_t* tags, uint8_t needle) {
#if defined(__SSE2__)
    // Load 4 tags into low 32 bits
    uint32_t tag32;
    std::memcpy(&tag32, tags, 4);
    
    // Use SIMD to compare
    __m128i tag_vec = _mm_set1_epi32(static_cast<int32_t>(tag32));
    __m128i needle_vec = _mm_set1_epi8(needle);
    __m128i cmp = _mm_cmpeq_epi8(tag_vec, needle_vec);
    int mask = _mm_movemask_epi8(cmp);
    return mask & 0x0F;  // Lower 4 bits
#else
    uint32_t m = 0;
    for (size_t i = 0; i < 4; ++i) {
        if (tags[i] == needle) m |= (1U << i);
    }
    return m;
#endif
}

inline uint32_t find_empty_tags_4(const uint8_t* tags) {
#if defined(__SSE2__)
    uint32_t tag32;
    std::memcpy(&tag32, tags, 4);
    __m128i tag_vec = _mm_set1_epi32(static_cast<int32_t>(tag32));
    __m128i zero_vec = _mm_setzero_si128();
    __m128i cmp = _mm_cmpeq_epi8(tag_vec, zero_vec);
    int mask = _mm_movemask_epi8(cmp);
    return mask & 0x0F;
#else
    uint32_t m = 0;
    for (size_t i = 0; i < 4; ++i) {
        if (tags[i] == 0) m |= (1U << i);
    }
    return m;
#endif
}

// ============================================================================
// Bucket Structure (128 bytes, 2 cache lines)
// ============================================================================

struct alignas(128) BucketTagged {
    // === Header: 8 bytes ===
    volatile uint8_t lock;                      // 1 byte
    uint8_t outbound_overflow_count;            // 1 byte
    uint8_t tags[TAGGED_ENTRIES];               // 4 bytes
    uint8_t _pad;                               // 1 byte
    
    // === Hash values: 32 bytes ===
    uint64_t key_hashes[TAGGED_ENTRIES];        // 32 bytes
    
    // === Values: 32 bytes ===
    volatile uintptr_t values[TAGGED_ENTRIES];  // 32 bytes
    
    // Total so far: 72 bytes
    
    // === Key pointers: 32 bytes ===
    const char* key_ptrs[TAGGED_ENTRIES];       // 32 bytes
    
    // === Key lengths: 8 bytes ===
    uint16_t key_lengths[TAGGED_ENTRIES];       // 8 bytes
    
    // Total: 72 + 32 + 8 = 112 bytes
    
    // === Overflow chain: 8 bytes ===
    BucketTagged* next;                         // 8 bytes
    
    // === Padding to 128 bytes: 8 bytes ===
    uint8_t _final_pad[8];                      // 8 bytes
    
    // Total: 120 bytes, aligned to 128
};

// ============================================================================
// Hash Table Structure
// ============================================================================

struct HashtableTagged {
    BucketTagged* buckets;
    size_t size;           // Number of buckets (power of 2)
    size_t mask;           // size - 1
    std::atomic<size_t> num_elements;
    StringAllocator* allocator;
};

// ============================================================================
// Lock Operations
// ============================================================================

inline bool lock_acquire_tagged(BucketTagged* bucket) {
    uint8_t expected = 0;
    size_t attempts = 0;
    while (!__atomic_compare_exchange_n(
        const_cast<uint8_t*>(&bucket->lock), 
        &expected, 1, false, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED)) {
        if (++attempts > 10000) return false;
        expected = 0;
        _mm_pause();
    }
    return true;
}

inline void lock_release_tagged(BucketTagged* bucket) {
    __atomic_store_n(const_cast<uint8_t*>(&bucket->lock), 0, __ATOMIC_RELEASE);
}

// ============================================================================
// API Functions
// ============================================================================

inline HashtableTagged* hashtable_tagged_create(size_t capacity, StringAllocator* alloc) {
    HashtableTagged* ht = new HashtableTagged{};
    
    size_t size = 1;
    while (size < (capacity + TAGGED_ENTRIES - 1) / TAGGED_ENTRIES) {
        size <<= 1;
    }
    
    ht->buckets = static_cast<BucketTagged*>(
        std::aligned_alloc(128, size * sizeof(BucketTagged)));
    std::memset(ht->buckets, 0, size * sizeof(BucketTagged));
    
    ht->size = size;
    ht->mask = size - 1;
    ht->allocator = alloc;
    
    return ht;
}

inline void hashtable_tagged_destroy(HashtableTagged* ht) {
    // Free overflow buckets
    for (size_t i = 0; i < ht->size; ++i) {
        BucketTagged* bucket = ht->buckets[i].next;
        while (bucket) {
            BucketTagged* next = bucket->next;
            std::free(bucket);
            bucket = next;
        }
    }
    
    std::free(ht->buckets);
    delete ht;
}

/**
 * Insert with tag optimization
 */
inline bool hashtable_tagged_put(HashtableTagged* ht, const char* key, size_t len, uintptr_t val) {
    uint64_t hash = hash_string(key, len);
    uint8_t tag = static_cast<uint8_t>((hash >> 56) | 0x80);  // Upper 8 bits, MSB set
    size_t bucket_idx = hash & ht->mask;
    
    BucketTagged* const head_bucket = &ht->buckets[bucket_idx];
    
    if (!lock_acquire_tagged(head_bucket)) {
        return false;
    }
    
    // First pass: check for existing key
    BucketTagged* bucket = head_bucket;
    while (bucket) {
        // SIMD tag matching - single instruction checks all 4 tags
        uint32_t match_mask = match_tags_4(bucket->tags, tag);
        
        while (match_mask) {
            int slot = __builtin_ctz(match_mask);
            match_mask &= (match_mask - 1);
            
            // Quick hash check
            if (bucket->key_hashes[slot] == hash) {
                // Full key comparison
                if (bucket->key_lengths[slot] == len &&
                    strcmp_simd(key, bucket->key_ptrs[slot], len) == 0) {
                    // Key exists, update
                    bucket->values[slot] = val;
                    lock_release_tagged(head_bucket);
                    return true;
                }
            }
        }
        
        bucket = bucket->next;
    }
    
    // Second pass: find empty slot
    bucket = head_bucket;
    BucketTagged* prev = nullptr;
    
    while (bucket) {
        uint32_t empty_mask = find_empty_tags_4(bucket->tags);
        
        if (empty_mask) {
            int slot = __builtin_ctz(empty_mask);
            
            // Store entry
            bucket->tags[slot] = tag;
            bucket->key_hashes[slot] = hash;
            bucket->values[slot] = val;
            bucket->key_ptrs[slot] = ht->allocator->alloc(key, len);
            bucket->key_lengths[slot] = static_cast<uint16_t>(len);
            
            // Update overflow count if this is an overflow bucket
            if (prev && prev->outbound_overflow_count < 255) {
                prev->outbound_overflow_count++;
            }
            
            ht->num_elements.fetch_add(1, std::memory_order_relaxed);
            lock_release_tagged(head_bucket);
            return true;
        }
        
        // Track for overflow counting
        if (bucket->outbound_overflow_count < 255) {
            bucket->outbound_overflow_count++;
        }
        
        prev = bucket;
        
        // Create overflow bucket if needed
        if (!bucket->next) {
            bucket->next = static_cast<BucketTagged*>(
                std::aligned_alloc(128, sizeof(BucketTagged)));
            std::memset(bucket->next, 0, sizeof(BucketTagged));
        }
        
        bucket = bucket->next;
    }
    
    lock_release_tagged(head_bucket);
    return false;
}

/**
 * Lookup with tag optimization
 */
inline uintptr_t hashtable_tagged_get(HashtableTagged* ht, const char* key, size_t len) {
    uint64_t hash = hash_string(key, len);
    uint8_t tag = static_cast<uint8_t>((hash >> 56) | 0x80);
    size_t bucket_idx = hash & ht->mask;
    
    BucketTagged* bucket = &ht->buckets[bucket_idx];
    
    while (bucket) {
        // SIMD tag matching
        uint32_t match_mask = match_tags_4(bucket->tags, tag);
        
        while (match_mask) {
            int slot = __builtin_ctz(match_mask);
            match_mask &= (match_mask - 1);
            
            if (bucket->key_hashes[slot] == hash) {
                if (bucket->key_lengths[slot] == len &&
                    strcmp_simd(key, bucket->key_ptrs[slot], len) == 0) {
                    return bucket->values[slot];
                }
            }
        }
        
        // Early exit: if no overflow, we're done
        if (bucket->outbound_overflow_count == 0) {
            return UINTPTR_MAX;
        }
        
        bucket = bucket->next;
    }
    
    return UINTPTR_MAX;
}

/**
 * Remove with tag optimization
 */
inline bool hashtable_tagged_remove(HashtableTagged* ht, const char* key, size_t len) {
    uint64_t hash = hash_string(key, len);
    uint8_t tag = static_cast<uint8_t>((hash >> 56) | 0x80);
    size_t bucket_idx = hash & ht->mask;
    
    BucketTagged* const head_bucket = &ht->buckets[bucket_idx];
    
    if (!lock_acquire_tagged(head_bucket)) {
        return false;
    }
    
    BucketTagged* bucket = head_bucket;
    BucketTagged* prev = nullptr;
    
    while (bucket) {
        uint32_t match_mask = match_tags_4(bucket->tags, tag);
        
        while (match_mask) {
            int slot = __builtin_ctz(match_mask);
            match_mask &= (match_mask - 1);
            
            if (bucket->key_hashes[slot] == hash) {
                if (bucket->key_lengths[slot] == len &&
                    strcmp_simd(key, bucket->key_ptrs[slot], len) == 0) {
                    // Found, remove
                    bucket->tags[slot] = EMPTY_TAG;
                    bucket->key_hashes[slot] = 0;
                    bucket->values[slot] = 0;
                    bucket->key_ptrs[slot] = nullptr;
                    bucket->key_lengths[slot] = 0;
                    
                    // Decrement overflow count
                    if (prev && prev->outbound_overflow_count > 0) {
                        prev->outbound_overflow_count--;
                    }
                    
                    ht->num_elements.fetch_sub(1, std::memory_order_relaxed);
                    lock_release_tagged(head_bucket);
                    return true;
                }
            }
        }
        
        prev = bucket;
        bucket = bucket->next;
    }
    
    lock_release_tagged(head_bucket);
    return false;
}

// ============================================================================
// C++ Wrapper
// ============================================================================

class ClhtStrTagged {
public:
    explicit ClhtStrTagged(size_t capacity = 1024) {
        allocator_ = new StringAllocator();
        ht_ = hashtable_tagged_create(capacity, allocator_);
    }
    
    ~ClhtStrTagged() {
        hashtable_tagged_destroy(ht_);
        delete allocator_;
    }
    
    bool insert(const std::string& key, uintptr_t value) {
        return hashtable_tagged_put(ht_, key.c_str(), key.size(), value);
    }
    
    uintptr_t lookup(const std::string& key) {
        return hashtable_tagged_get(ht_, key.c_str(), key.size());
    }
    
    bool remove(const std::string& key) {
        return hashtable_tagged_remove(ht_, key.c_str(), key.size());
    }
    
    size_t size() const {
        return ht_->num_elements.load(std::memory_order_relaxed);
    }
    
private:
    HashtableTagged* ht_;
    StringAllocator* allocator_;
};

}  // namespace clht_str
