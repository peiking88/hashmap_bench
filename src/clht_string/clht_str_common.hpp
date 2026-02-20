#pragma once

/**
 * CLHT String Key Support - Common Definitions
 * 
 * This module extends CLHT (Cache-Line Hash Table) to support string keys
 * using SIMD-optimized hash functions and Volnitsky-style string comparison.
 * 
 * Three implementation approaches:
 *   - Approach A: Hash + Pointer (external string storage)
 *   - Approach B: Fixed-length inline storage
 *   - Approach C: External key pool
 */

#include <cstdint>
#include <cstring>
#include <string>
#include <atomic>
#include <memory>
#include <mutex>
#include <immintrin.h>  // AVX/SSE intrinsics

namespace clht_str {

// ============================================================================
// SIMD-Optimized String Hash Functions (based on Doris/ClickHouse approach)
// ============================================================================

/**
 * CityHash-inspired 64-bit hash using SIMD
 * Optimized for strings of various lengths
 */
inline uint64_t hash_str_simd(const char* str, size_t len) {
    if (len == 0) return 0;
    
    const uint64_t kMul = 0x9ddfea08eb382d69ULL;
    uint64_t h = len * kMul;
    
    if (len >= 8) {
        // Use SIMD for bulk processing
        size_t simd_len = len & ~7ULL;  // Process 8 bytes at a time
        
#if defined(__SSE4_2__)
        __m128i hash_vec = _mm_set1_epi64x(h);
        
        for (size_t i = 0; i < simd_len; i += 8) {
            __m128i data = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(str + i));
            __m128i mul = _mm_set1_epi64x(kMul);
            
            // Multiply and XOR
            __m128i prod = _mm_mullo_epi64(data, mul);  // Requires SSE4.1
            hash_vec = _mm_xor_si128(hash_vec, prod);
            hash_vec = _mm_xor_si128(hash_vec, _mm_srli_si128(hash_vec, 4));
        }
        
        // Extract final hash
        h = _mm_extract_epi64(hash_vec, 0);
#else
        // Fallback to scalar
        for (size_t i = 0; i < simd_len; i += 8) {
            uint64_t chunk;
            std::memcpy(&chunk, str + i, 8);
            h ^= chunk * kMul;
            h = (h >> 47) ^ h;
        }
#endif
    }
    
    // Handle remaining bytes
    size_t remaining = len & 7;
    if (remaining > 0) {
        uint64_t last = 0;
        std::memcpy(&last, str + (len & ~7ULL), remaining);
        h ^= last * kMul;
    }
    
    // Final mixing
    h ^= h >> 47;
    h *= kMul;
    h ^= h >> 47;
    
    return h;
}

/**
 * CRC32-based hash using hardware CRC instruction (very fast on modern CPUs)
 */
inline uint64_t hash_str_crc32(const char* str, size_t len) {
    if (len == 0) return 0;
    
    uint64_t crc = 0xFFFFFFFF;
    
#if defined(__SSE4_2__)
    size_t i = 0;
    
    // Process 8 bytes at a time
    for (; i + 8 <= len; i += 8) {
        uint64_t chunk;
        std::memcpy(&chunk, str + i, 8);
        crc = _mm_crc32_u64(crc, chunk);
    }
    
    // Process remaining 4 bytes
    if (i + 4 <= len) {
        uint32_t chunk;
        std::memcpy(&chunk, str + i, 4);
        crc = _mm_crc32_u32(crc, chunk);
        i += 4;
    }
    
    // Process remaining bytes
    for (; i < len; ++i) {
        crc = _mm_crc32_u8(crc, static_cast<uint8_t>(str[i]));
    }
#else
    // Fallback
    for (size_t i = 0; i < len; ++i) {
        crc ^= static_cast<uint8_t>(str[i]);
        for (int j = 0; j < 8; ++j) {
            crc = (crc >> 1) ^ (0xEDB88320 & -(crc & 1));
        }
    }
#endif
    
    return crc ^ 0xFFFFFFFF;
}

/**
 * Combined hash: high-quality distribution with good performance
 */
inline uint64_t hash_string(const char* str, size_t len) {
#if defined(__SSE4_2__)
    return hash_str_crc32(str, len);
#else
    return hash_str_simd(str, len);
#endif
}

// ============================================================================
// Volnitsky-Style SIMD String Comparison
// ============================================================================

/**
 * SIMD-optimized string comparison
 * Returns: 0 if equal, non-zero if different
 */
inline int strcmp_simd(const char* s1, const char* s2, size_t len) {
    if (len == 0) return 0;
    
    size_t i = 0;
    
#if defined(__AVX2__)
    // AVX2: Compare 32 bytes at a time
    for (; i + 32 <= len; i += 32) {
        __m256i v1 = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(s1 + i));
        __m256i v2 = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(s2 + i));
        __m256i cmp = _mm256_cmpeq_epi8(v1, v2);
        
        if (_mm256_movemask_epi8(cmp) != 0xFFFFFFFF) {
            // Found difference, fall back to byte comparison
            for (size_t j = i; j < i + 32; ++j) {
                if (s1[j] != s2[j]) return static_cast<int>(s1[j]) - static_cast<int>(s2[j]);
            }
        }
    }
#elif defined(__SSE2__)
    // SSE2: Compare 16 bytes at a time
    for (; i + 16 <= len; i += 16) {
        __m128i v1 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(s1 + i));
        __m128i v2 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(s2 + i));
        __m128i cmp = _mm_cmpeq_epi8(v1, v2);
        
        if (_mm_movemask_epi8(cmp) != 0xFFFF) {
            for (size_t j = i; j < i + 16; ++j) {
                if (s1[j] != s2[j]) return static_cast<int>(s1[j]) - static_cast<int>(s2[j]);
            }
        }
    }
#endif
    
    // Handle remaining bytes
    for (; i < len; ++i) {
        if (s1[i] != s2[i]) {
            return static_cast<int>(static_cast<uint8_t>(s1[i])) - 
                   static_cast<int>(static_cast<uint8_t>(s2[i]));
        }
    }
    
    return 0;
}

/**
 * Length-aware string comparison (handles different lengths)
 */
inline int strcmp_len(const char* s1, size_t len1, const char* s2, size_t len2) {
    if (len1 != len2) {
        return len1 < len2 ? -1 : 1;
    }
    return strcmp_simd(s1, s2, len1);
}

// ============================================================================
// Key Entry Types for Different Approaches
// ============================================================================

/**
 * Approach A: Hash + Pointer
 * Stores hash for fast comparison, pointer to external string
 */
struct KeyEntryPtr {
    uint64_t hash;      // 8 bytes: string hash value
    const char* ptr;    // 8 bytes: pointer to external string
    uint16_t length;    // 2 bytes: string length
    uint16_t padding;   // 2 bytes: alignment padding
};  // Total: 20 bytes (padded to 24 for alignment)

/**
 * Approach B: Fixed-length inline storage
 * Stores string directly in the bucket (limited length)
 */
constexpr size_t INLINE_KEY_SIZE = 16;  // Max inline string length

struct KeyEntryInline {
    uint64_t hash;              // 8 bytes: hash value for fast comparison
    char data[INLINE_KEY_SIZE]; // 16 bytes: inline string data
    uint8_t length;             // 1 byte: actual string length
    uint8_t padding[7];         // 7 bytes: alignment padding
};  // Total: 32 bytes

/**
 * Approach C: External key pool
 * Uses offset into a shared string pool
 */
struct KeyEntryPooled {
    uint64_t hash;      // 8 bytes: hash value
    uint32_t offset;    // 4 bytes: offset in key pool
    uint16_t length;    // 2 bytes: string length
    uint16_t padding;   // 2 bytes: alignment padding
};  // Total: 16 bytes

// ============================================================================
// String Memory Management
// ============================================================================

/**
 * Simple string allocator for Approach A
 * Thread-safe allocation using mutex (simpler and more reliable)
 */
class StringAllocator {
public:
    static constexpr size_t CHUNK_SIZE = 64 * 1024;  // 64KB chunks
    
    struct Chunk {
        char data[CHUNK_SIZE];
        size_t offset;
        Chunk* next;
        
        Chunk() : offset(0), next(nullptr) {}
    };
    
    StringAllocator() {
        head_ = new Chunk();
    }
    
    ~StringAllocator() {
        std::lock_guard<std::mutex> lock(mutex_);
        Chunk* current = head_;
        while (current) {
            Chunk* next = current->next;
            delete current;
            current = next;
        }
    }
    
    // Allocate and copy string
    const char* alloc(const char* str, size_t len) {
        // Round up to 8-byte alignment
        size_t alloc_size = (len + 8) & ~7ULL;
        
        std::lock_guard<std::mutex> lock(mutex_);
        
        Chunk* chunk = head_;
        size_t offset = chunk->offset;
        
        if (offset + alloc_size > CHUNK_SIZE) {
            // Need new chunk
            Chunk* new_chunk = new Chunk();
            new_chunk->next = chunk;
            head_ = new_chunk;
            chunk = new_chunk;
            offset = 0;
        }
        
        char* result = chunk->data + offset;
        std::memcpy(result, str, len);
        result[len] = '\0';  // Null terminator
        chunk->offset += alloc_size;
        
        return result;
    }
    
private:
    Chunk* head_;
    std::mutex mutex_;
};

/**
 * Key Pool for Approach C
 * Shared string storage with offsets
 */
class KeyPool {
public:
    static constexpr size_t POOL_SIZE = 16 * 1024 * 1024;  // 16MB initial
    
    KeyPool() {
        data_ = static_cast<char*>(std::aligned_alloc(64, POOL_SIZE));
        capacity_ = POOL_SIZE;
    }
    
    ~KeyPool() {
        std::free(data_);
    }
    
    // Allocate string and return offset
    uint32_t alloc(const char* str, size_t len) {
        size_t alloc_size = len + 1;
        size_t offset = offset_.fetch_add(alloc_size);
        
        if (offset + alloc_size > capacity_) {
            // TODO: Handle pool expansion
            return 0;  // Error
        }
        
        std::memcpy(data_ + offset, str, len);
        data_[offset + len] = '\0';
        return static_cast<uint32_t>(offset);
    }
    
    const char* get(uint32_t offset) const {
        return data_ + offset;
    }
    
private:
    char* data_;
    size_t capacity_;
    std::atomic<size_t> offset_{0};
};

}  // namespace clht_str
