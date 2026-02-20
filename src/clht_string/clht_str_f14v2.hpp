#pragma once

/**
 * CLHT String Key Support - F14-Style Optimized Version v2
 * 
 * Key optimizations from F14:
 * 1. 7-bit tag + SIMD filtering for fast key lookup
 * 2. Contiguous arrays for cache efficiency
 * 3. Overflow counting for early exit optimization
 * 4. Quadratic probing for better distribution
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

constexpr size_t F14V2_ENTRIES_PER_GROUP = 6;  // 6 entries per group for SIMD
constexpr uint8_t F14V2_EMPTY_TAG = 0;

// ============================================================================
// SIMD Helper Functions
// ============================================================================

/**
 * SIMD-optimized tag matching for 6 tags
 * Returns bitmask of matching positions (lower 6 bits)
 */
inline uint32_t match_tags_simd6(const uint8_t* tags, uint8_t needle) {
#if defined(__SSE2__)
    // Load 8 bytes (6 tags + 2 padding)
    __m128i tag_vec = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(tags));
    __m128i needle_vec = _mm_set1_epi8(needle);
    __m128i cmp = _mm_cmpeq_epi8(tag_vec, needle_vec);
    int mask = _mm_movemask_epi8(cmp);
    return static_cast<uint32_t>(mask & 0x3F);  // Lower 6 bits
#else
    uint32_t mask = 0;
    for (size_t i = 0; i < 6; ++i) {
        if (tags[i] == needle) mask |= (1U << i);
    }
    return mask;
#endif
}

/**
 * Find empty slots using SIMD
 * Returns bitmask of empty positions
 */
inline uint32_t find_empty_tags_simd(const uint8_t* tags) {
#if defined(__SSE2__)
    __m128i tag_vec = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(tags));
    __m128i zero_vec = _mm_setzero_si128();
    __m128i cmp = _mm_cmpeq_epi8(tag_vec, zero_vec);
    int mask = _mm_movemask_epi8(cmp);
    return static_cast<uint32_t>(mask & 0x3F);
#else
    uint32_t mask = 0;
    for (size_t i = 0; i < 6; ++i) {
        if (tags[i] == 0) mask |= (1U << i);
    }
    return mask;
#endif
}

// ============================================================================
// F14-Style Hash Table Implementation
// ============================================================================

/**
 * ClhtStrF14V2 - F14-style optimized string hash table
 * 
 * Design:
 * - Uses contiguous arrays for hot data (tags, hashes, values)
 * - Tags are stored together for SIMD filtering
 * - Key data stored separately for cache efficiency
 * - Overflow counting enables early exit during lookup
 * - Quadratic probing for better distribution
 */
class ClhtStrF14V2 {
public:
    static constexpr size_t ENTRIES_PER_GROUP = F14V2_ENTRIES_PER_GROUP;
    
private:
    size_t capacity_;
    size_t mask_;
    size_t num_groups_;
    
    // Contiguous arrays for hot lookup data
    alignas(64) uint8_t* tags_;              // capacity_ entries
    uint64_t* key_hashes_;                   // capacity_ entries
    volatile uintptr_t* values_;             // capacity_ entries
    
    // Per-group metadata
    volatile uint8_t* locks_;                // num_groups_ entries
    uint8_t* outbound_overflow_counts_;      // num_groups_ entries
    
    // Key data (cold data)
    const char** key_ptrs_;
    uint16_t* key_lengths_;
    
    std::atomic<size_t> num_elements_;
    StringAllocator allocator_;
    
public:
    explicit ClhtStrF14V2(size_t capacity = 1024);
    ~ClhtStrF14V2();
    
    bool insert(const std::string& key, uintptr_t value);
    uintptr_t lookup(const std::string& key);
    bool remove(const std::string& key);
    size_t size() const { return num_elements_.load(std::memory_order_relaxed); }
    size_t capacity() const { return capacity_; }
    
private:
    static uint8_t compute_tag(uint64_t hash) {
        // Extract upper 7 bits and set MSB for valid tag
        // This ensures tag != 0 for all valid entries
        return static_cast<uint8_t>((hash >> 57) | 0x80);
    }
    
    bool acquire_lock(size_t group_idx);
    void release_lock(size_t group_idx);
    size_t probe_index(size_t base, size_t probe) const;
};

// ============================================================================
// Implementation
// ============================================================================

inline ClhtStrF14V2::ClhtStrF14V2(size_t capacity) 
    : allocator_() {
    // Round capacity up to power of 2, aligned to group size
    size_t min_capacity = ((capacity + ENTRIES_PER_GROUP - 1) / ENTRIES_PER_GROUP) * ENTRIES_PER_GROUP;
    capacity_ = 1;
    while (capacity_ < min_capacity) {
        capacity_ <<= 1;
    }
    mask_ = capacity_ - 1;
    num_groups_ = capacity_ / ENTRIES_PER_GROUP;
    
    // Allocate contiguous arrays with proper alignment
    tags_ = static_cast<uint8_t*>(std::aligned_alloc(64, capacity_));
    std::memset(tags_, 0, capacity_);
    
    key_hashes_ = new uint64_t[capacity_]();
    values_ = new uintptr_t[capacity_]();
    
    locks_ = new uint8_t[num_groups_]();
    outbound_overflow_counts_ = new uint8_t[num_groups_]();
    
    key_ptrs_ = new const char*[capacity_]();
    key_lengths_ = new uint16_t[capacity_]();
    
    num_elements_.store(0, std::memory_order_relaxed);
}

inline ClhtStrF14V2::~ClhtStrF14V2() {
    std::free(tags_);
    delete[] key_hashes_;
    delete[] values_;
    delete[] locks_;
    delete[] outbound_overflow_counts_;
    delete[] key_ptrs_;
    delete[] key_lengths_;
}

inline bool ClhtStrF14V2::acquire_lock(size_t group_idx) {
    uint8_t expected = 0;
    size_t attempts = 0;
    while (!__atomic_compare_exchange_n(
        const_cast<uint8_t*>(&locks_[group_idx]), 
        &expected, 1, false, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED)) {
        if (++attempts > 10000) return false;
        expected = 0;
        _mm_pause();
    }
    return true;
}

inline void ClhtStrF14V2::release_lock(size_t group_idx) {
    __atomic_store_n(const_cast<uint8_t*>(&locks_[group_idx]), 0, __ATOMIC_RELEASE);
}

inline size_t ClhtStrF14V2::probe_index(size_t base, size_t probe) const {
    // Quadratic probing: base + probe^2 * group_size
    // Ensures we visit different groups
    return (base + probe * probe * ENTRIES_PER_GROUP) & mask_;
}

inline uintptr_t ClhtStrF14V2::lookup(const std::string& key) {
    uint64_t hash = hash_string(key.c_str(), key.size());
    uint8_t tag = compute_tag(hash);
    
    // Primary probe location
    size_t base_idx = (hash & mask_);
    size_t base_group = base_idx / ENTRIES_PER_GROUP * ENTRIES_PER_GROUP;
    
    for (size_t probe = 0; probe < 8; ++probe) {
        size_t idx = probe_index(base_group, probe);
        size_t group = idx / ENTRIES_PER_GROUP;
        
        // SIMD tag matching - single instruction checks all 6 tags
        uint32_t match_mask = match_tags_simd6(&tags_[idx], tag);
        
        // Check each matching slot
        while (match_mask) {
            int slot = __builtin_ctz(match_mask);
            match_mask &= (match_mask - 1);
            
            size_t entry_idx = idx + slot;
            
            // Quick hash verification (cache-friendly)
            if (key_hashes_[entry_idx] == hash) {
                // Full key comparison (only on hash match)
                if (key_lengths_[entry_idx] == key.size() &&
                    strcmp_simd(key.c_str(), key_ptrs_[entry_idx], key.size()) == 0) {
                    return values_[entry_idx];
                }
            }
        }
        
        // Early exit optimization: if no overflow, we're done
        if (probe > 0 && outbound_overflow_counts_[group] == 0) {
            break;
        }
        
        // Check if group has any empty slots (key would be here if existed)
        bool has_empty = false;
        for (size_t i = 0; i < ENTRIES_PER_GROUP; ++i) {
            if (tags_[idx + i] == F14V2_EMPTY_TAG) {
                has_empty = true;
                break;
            }
        }
        if (has_empty && probe == 0) {
            break;  // Key would be in first group if it existed
        }
    }
    
    return UINTPTR_MAX;
}

inline bool ClhtStrF14V2::insert(const std::string& key, uintptr_t value) {
    uint64_t hash = hash_string(key.c_str(), key.size());
    uint8_t tag = compute_tag(hash);
    
    size_t base_idx = (hash & mask_);
    size_t base_group = base_idx / ENTRIES_PER_GROUP * ENTRIES_PER_GROUP;
    size_t primary_group = base_group / ENTRIES_PER_GROUP;
    
    if (!acquire_lock(primary_group)) {
        return false;
    }
    
    // First pass: check for existing key
    for (size_t probe = 0; probe < 8; ++probe) {
        size_t idx = probe_index(base_group, probe);
        
        uint32_t match_mask = match_tags_simd6(&tags_[idx], tag);
        
        while (match_mask) {
            int slot = __builtin_ctz(match_mask);
            match_mask &= (match_mask - 1);
            
            size_t entry_idx = idx + slot;
            
            if (key_hashes_[entry_idx] == hash &&
                key_lengths_[entry_idx] == key.size() &&
                strcmp_simd(key.c_str(), key_ptrs_[entry_idx], key.size()) == 0) {
                // Key exists, update value
                values_[entry_idx] = value;
                release_lock(primary_group);
                return true;
            }
        }
    }
    
    // Second pass: find empty slot
    for (size_t probe = 0; probe < 8; ++probe) {
        size_t idx = probe_index(base_group, probe);
        size_t group = idx / ENTRIES_PER_GROUP;
        
        uint32_t empty_mask = find_empty_tags_simd(&tags_[idx]);
        
        if (empty_mask) {
            int slot = __builtin_ctz(empty_mask);
            size_t entry_idx = idx + slot;
            
            // Store in arrays
            tags_[entry_idx] = tag;
            key_hashes_[entry_idx] = hash;
            values_[entry_idx] = value;
            key_ptrs_[entry_idx] = allocator_.alloc(key.c_str(), key.size());
            key_lengths_[entry_idx] = static_cast<uint16_t>(key.size());
            
            // Update overflow count if we're in a secondary group
            if (probe > 0 && outbound_overflow_counts_[primary_group] < 255) {
                outbound_overflow_counts_[primary_group]++;
            }
            
            num_elements_.fetch_add(1, std::memory_order_relaxed);
            release_lock(primary_group);
            return true;
        }
    }
    
    // Table is full or too many collisions
    release_lock(primary_group);
    return false;
}

inline bool ClhtStrF14V2::remove(const std::string& key) {
    uint64_t hash = hash_string(key.c_str(), key.size());
    uint8_t tag = compute_tag(hash);
    
    size_t base_idx = (hash & mask_);
    size_t base_group = base_idx / ENTRIES_PER_GROUP * ENTRIES_PER_GROUP;
    size_t primary_group = base_group / ENTRIES_PER_GROUP;
    
    if (!acquire_lock(primary_group)) {
        return false;
    }
    
    for (size_t probe = 0; probe < 8; ++probe) {
        size_t idx = probe_index(base_group, probe);
        
        uint32_t match_mask = match_tags_simd6(&tags_[idx], tag);
        
        while (match_mask) {
            int slot = __builtin_ctz(match_mask);
            match_mask &= (match_mask - 1);
            
            size_t entry_idx = idx + slot;
            
            if (key_hashes_[entry_idx] == hash &&
                key_lengths_[entry_idx] == key.size() &&
                strcmp_simd(key.c_str(), key_ptrs_[entry_idx], key.size()) == 0) {
                // Found, remove entry
                tags_[entry_idx] = F14V2_EMPTY_TAG;
                key_hashes_[entry_idx] = 0;
                values_[entry_idx] = 0;
                key_ptrs_[entry_idx] = nullptr;
                key_lengths_[entry_idx] = 0;
                
                // Decrement overflow count
                if (probe > 0 && outbound_overflow_counts_[primary_group] > 0) {
                    outbound_overflow_counts_[primary_group]--;
                }
                
                num_elements_.fetch_sub(1, std::memory_order_relaxed);
                release_lock(primary_group);
                return true;
            }
        }
    }
    
    release_lock(primary_group);
    return false;
}

}  // namespace clht_str
