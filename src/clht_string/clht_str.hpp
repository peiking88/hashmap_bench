#pragma once

/**
 * CLHT String Key Support - Unified Header
 * 
 * This module provides three implementation approaches for extending
 * CLHT (Cache-Line Hash Table) to support string keys:
 * 
 * 1. ClhtStrPtr    - Hash + Pointer (external string storage)
 * 2. ClhtStrInline - Fixed-length inline storage
 * 3. ClhtStrPooled - External key pool
 * 
 * All implementations use:
 *   - SIMD-optimized hash functions (CRC32 or CityHash-style)
 *   - Volnitsky-style SIMD string comparison
 *   - Cache-line aligned bucket structures
 * 
 * Usage:
 * 
 *   // Approach A: Best for general use, arbitrary length strings
 *   clht_str::ClhtStrPtr ht1;
 *   ht1.insert("hello", 42);
 *   auto val = ht1.lookup("hello");
 * 
 *   // Approach B: Best for short strings (< 16 bytes)
 *   clht_str::ClhtStrInline ht2;
 *   ht2.insert("short_key", 123);
 * 
 *   // Approach C: Best for memory-constrained environments
 *   clht_str::ClhtStrPooled ht3;
 *   ht3.insert("any_length_key", 456);
 * 
 * Reference:
 *   - CLHT: https://github.com/LPD-EPFL/CLHT
 *   - Volnitsky Algorithm: http://volnitsky.com/project/str_search/
 *   - SIMD String Processing: Apache Doris, ClickHouse
 */

#include "clht_str_common.hpp"
#include "clht_str_ptr.hpp"
#include "clht_str_inline.hpp"
#include "clht_str_pooled.hpp"

namespace clht_str {

/**
 * Quick comparison table:
 * 
 * | Feature            | ClhtStrPtr | ClhtStrInline | ClhtStrPooled |
 * |--------------------|------------|---------------|---------------|
 * | Max key length     | Unlimited  | 16 bytes      | Unlimited     |
 * | Memory overhead    | Medium     | Low           | Low           |
 * | Cache locality     | Medium     | Best          | Medium        |
 * | Insert speed       | Fast       | Fastest       | Fast          |
 * | Lookup speed       | Fast       | Fastest       | Fast          |
 * | Memory management  | Allocator  | None          | Pool          |
 * | Thread safety      | Yes        | Yes           | Yes           |
 */

}  // namespace clht_str
