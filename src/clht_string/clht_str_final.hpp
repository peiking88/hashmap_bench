#pragma once

/**
 * CLHT String Key Support - Final Optimized Version
 * 
 * Combines best of both worlds:
 * - Single-pass traversal from V3 (best insert)
 * - SIMD tag matching from original (best query)
 * - Keep 128B bucket alignment
 */

#include "clht_str_common.hpp"
#include <atomic>
#include <cassert>
#include <cstring>
#include <immintrin.h>

namespace clht_str {

constexpr size_t FINAL_ENTRIES_V = 4;

// SIMD helpers (keep for best query performance)
inline uint32_t match_tags_final(const uint8_t* tags, uint8_t needle) {
#if defined(__SSE2__)
    uint32_t tag32;
    std::memcpy(&tag32, tags, 4);
    __m128i tag_vec = _mm_set1_epi32(static_cast<int32_t>(tag32));
    __m128i needle_vec = _mm_set1_epi8(needle);
    __m128i cmp = _mm_cmpeq_epi8(tag_vec, needle_vec);
    int mask = _mm_movemask_epi8(cmp);
    return mask & 0x0F;
#else
    uint32_t m = 0;
    for (size_t i = 0; i < 4; ++i) {
        if (tags[i] == needle) m |= (1U << i);
    }
    return m;
#endif
}

inline uint32_t find_empty_tags_final(const uint8_t* tags) {
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

// Original 128B bucket structure
struct alignas(128) BucketFinal {
    volatile uint8_t lock;
    uint8_t outbound_overflow_count;
    uint8_t tags[FINAL_ENTRIES_V];
    uint8_t _pad;
    
    uint64_t key_hashes[FINAL_ENTRIES_V];
    volatile uintptr_t values[FINAL_ENTRIES_V];
    const char* key_ptrs[FINAL_ENTRIES_V];
    uint16_t key_lengths[FINAL_ENTRIES_V];
    
    BucketFinal* next;
    uint8_t _final_pad[8];
};

struct HashtableFinal {
    BucketFinal* buckets;
    size_t size;
    size_t mask;
    std::atomic<size_t> num_elements;
    StringAllocator* allocator;
};

inline bool lock_acquire_final(BucketFinal* bucket) {
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

inline void lock_release_final(BucketFinal* bucket) {
    __atomic_store_n(const_cast<uint8_t*>(&bucket->lock), 0, __ATOMIC_RELEASE);
}

inline HashtableFinal* hashtable_final_create(size_t capacity, StringAllocator* alloc) {
    HashtableFinal* ht = new HashtableFinal{};
    
    size_t size = 1;
    while (size < (capacity + FINAL_ENTRIES_V - 1) / FINAL_ENTRIES_V) {
        size <<= 1;
    }
    
    ht->buckets = static_cast<BucketFinal*>(
        std::aligned_alloc(128, size * sizeof(BucketFinal)));
    std::memset(ht->buckets, 0, size * sizeof(BucketFinal));
    
    ht->size = size;
    ht->mask = size - 1;
    ht->allocator = alloc;
    
    return ht;
}

inline void hashtable_final_destroy(HashtableFinal* ht) {
    for (size_t i = 0; i < ht->size; ++i) {
        BucketFinal* bucket = ht->buckets[i].next;
        while (bucket) {
            BucketFinal* next = bucket->next;
            std::free(bucket);
            bucket = next;
        }
    }
    std::free(ht->buckets);
    delete ht;
}

/**
 * Insert: single-pass with SIMD for both match and empty search
 */
inline bool hashtable_final_put(HashtableFinal* ht, const char* key, size_t len, uintptr_t val) {
    uint64_t hash = hash_string(key, len);
    uint8_t tag = static_cast<uint8_t>((hash >> 56) | 0x80);
    size_t bucket_idx = hash & ht->mask;
    
    BucketFinal* const head = &ht->buckets[bucket_idx];
    
    if (!lock_acquire_final(head)) {
        return false;
    }
    
    // Single-pass: check existence + record first empty
    BucketFinal* bucket = head;
    BucketFinal* empty_bucket = nullptr;
    int empty_slot = -1;
    BucketFinal* prev = nullptr;
    
    while (bucket) {
        // SIMD match check
        uint32_t match_mask = match_tags_final(bucket->tags, tag);
        
        while (match_mask) {
            int slot = __builtin_ctz(match_mask);
            match_mask &= (match_mask - 1);
            
            if (bucket->key_hashes[slot] == hash &&
                bucket->key_lengths[slot] == len &&
                strcmp_simd(key, bucket->key_ptrs[slot], len) == 0) {
                bucket->values[slot] = val;
                lock_release_final(head);
                return true;
            }
        }
        
        // SIMD empty search (only if not found yet)
        if (empty_slot < 0) {
            uint32_t empty_mask = find_empty_tags_final(bucket->tags);
            if (empty_mask) {
                empty_bucket = bucket;
                empty_slot = __builtin_ctz(empty_mask);
            }
        }
        
        prev = bucket;
        bucket = bucket->next;
    }
    
    // Insert
    if (empty_slot >= 0) {
        empty_bucket->tags[empty_slot] = tag;
        empty_bucket->key_hashes[empty_slot] = hash;
        empty_bucket->values[empty_slot] = val;
        empty_bucket->key_ptrs[empty_slot] = ht->allocator->alloc(key, len);
        empty_bucket->key_lengths[empty_slot] = static_cast<uint16_t>(len);
        
        if (empty_bucket != head && prev) {
            // Find actual predecessor
            BucketFinal* p = head;
            while (p && p->next != empty_bucket) p = p->next;
            if (p && p->outbound_overflow_count < 255) p->outbound_overflow_count++;
        }
        
        ht->num_elements.fetch_add(1, std::memory_order_relaxed);
        lock_release_final(head);
        return true;
    }
    
    // Create overflow
    BucketFinal* last = head;
    while (last->next) {
        if (last->outbound_overflow_count < 255) last->outbound_overflow_count++;
        last = last->next;
    }
    if (last->outbound_overflow_count < 255) last->outbound_overflow_count++;
    
    last->next = static_cast<BucketFinal*>(
        std::aligned_alloc(128, sizeof(BucketFinal)));
    std::memset(last->next, 0, sizeof(BucketFinal));
    
    last->next->tags[0] = tag;
    last->next->key_hashes[0] = hash;
    last->next->values[0] = val;
    last->next->key_ptrs[0] = ht->allocator->alloc(key, len);
    last->next->key_lengths[0] = static_cast<uint16_t>(len);
    
    ht->num_elements.fetch_add(1, std::memory_order_relaxed);
    lock_release_final(head);
    return true;
}

/**
 * Lookup: SIMD match (same as original - best query)
 */
inline uintptr_t hashtable_final_get(HashtableFinal* ht, const char* key, size_t len) {
    uint64_t hash = hash_string(key, len);
    uint8_t tag = static_cast<uint8_t>((hash >> 56) | 0x80);
    size_t bucket_idx = hash & ht->mask;
    
    BucketFinal* bucket = &ht->buckets[bucket_idx];
    
    while (bucket) {
        uint32_t match_mask = match_tags_final(bucket->tags, tag);
        
        while (match_mask) {
            int slot = __builtin_ctz(match_mask);
            match_mask &= (match_mask - 1);
            
            if (bucket->key_hashes[slot] == hash &&
                bucket->key_lengths[slot] == len &&
                strcmp_simd(key, bucket->key_ptrs[slot], len) == 0) {
                return bucket->values[slot];
            }
        }
        
        if (bucket->outbound_overflow_count == 0) {
            return UINTPTR_MAX;
        }
        bucket = bucket->next;
    }
    
    return UINTPTR_MAX;
}

inline bool hashtable_final_remove(HashtableFinal* ht, const char* key, size_t len) {
    uint64_t hash = hash_string(key, len);
    uint8_t tag = static_cast<uint8_t>((hash >> 56) | 0x80);
    size_t bucket_idx = hash & ht->mask;
    
    BucketFinal* const head = &ht->buckets[bucket_idx];
    
    if (!lock_acquire_final(head)) return false;
    
    BucketFinal* bucket = head;
    BucketFinal* prev = nullptr;
    
    while (bucket) {
        uint32_t match_mask = match_tags_final(bucket->tags, tag);
        
        while (match_mask) {
            int slot = __builtin_ctz(match_mask);
            match_mask &= (match_mask - 1);
            
            if (bucket->key_hashes[slot] == hash &&
                bucket->key_lengths[slot] == len &&
                strcmp_simd(key, bucket->key_ptrs[slot], len) == 0) {
                
                bucket->tags[slot] = 0;
                bucket->key_hashes[slot] = 0;
                bucket->values[slot] = 0;
                bucket->key_ptrs[slot] = nullptr;
                bucket->key_lengths[slot] = 0;
                
                if (prev && prev->outbound_overflow_count > 0) {
                    prev->outbound_overflow_count--;
                }
                
                ht->num_elements.fetch_sub(1, std::memory_order_relaxed);
                lock_release_final(head);
                return true;
            }
        }
        prev = bucket;
        bucket = bucket->next;
    }
    
    lock_release_final(head);
    return false;
}

// C++ Wrapper
class ClhtStrFinal {
public:
    explicit ClhtStrFinal(size_t capacity = 1024) {
        allocator_ = new StringAllocator();
        ht_ = hashtable_final_create(capacity, allocator_);
    }
    
    ~ClhtStrFinal() {
        hashtable_final_destroy(ht_);
        delete allocator_;
    }
    
    bool insert(const std::string& key, uintptr_t value) {
        return hashtable_final_put(ht_, key.c_str(), key.size(), value);
    }
    
    uintptr_t lookup(const std::string& key) {
        return hashtable_final_get(ht_, key.c_str(), key.size());
    }
    
    bool remove(const std::string& key) {
        return hashtable_final_remove(ht_, key.c_str(), key.size());
    }
    
    size_t size() const {
        return ht_->num_elements.load(std::memory_order_relaxed);
    }
    
private:
    HashtableFinal* ht_;
    StringAllocator* allocator_;
};

}  // namespace clht_str
