#pragma once

/**
 * CLHT String Test Wrappers
 * 
 * Minimal wrapper definitions for CLHT string key testing.
 * Avoids dependencies on all hashmap implementations in hash_maps.hpp
 */

#include <string>
#include <cstdint>
#include <atomic>

// CLHT (lock-based and lock-free hash tables)
extern "C" {
#include <clht.h>
#include <ssmem.h>
}

// CLHT String Key implementations
#include "clht_string/clht_str_ptr.hpp"
#include "clht_string/clht_str_inline.hpp"
#include "clht_string/clht_str_pooled.hpp"
#include "clht_string/clht_str_tagged.hpp"
#include "clht_string/clht_str_final.hpp"

namespace clht_test {

// ============================================================================
// CLHT String Key wrappers for testing
// ============================================================================

class ClhtStrPtrWrapper {
public:
    using Map = clht_str::ClhtStrPtr*;
    
    static Map create(size_t capacity) {
        return new clht_str::ClhtStrPtr(capacity * 2);
    }
    static void insert(Map& ht, const std::string& k, uint64_t v) {
        ht->insert(k, v);
    }
    static uint64_t lookup(Map& ht, const std::string& k) {
        uintptr_t val = ht->lookup(k);
        return (val == UINTPTR_MAX) ? 0 : val;
    }
    static void destroy(Map& ht) {
        delete ht;
    }
};

class ClhtStrInlineWrapper {
public:
    using Map = clht_str::ClhtStrInline*;
    
    static Map create(size_t capacity) {
        return new clht_str::ClhtStrInline(capacity * 2);
    }
    static void insert(Map& ht, const std::string& k, uint64_t v) {
        ht->insert(k, v);
    }
    static uint64_t lookup(Map& ht, const std::string& k) {
        uintptr_t val = ht->lookup(k);
        return (val == UINTPTR_MAX) ? 0 : val;
    }
    static void destroy(Map& ht) {
        delete ht;
    }
};

class ClhtStrPooledWrapper {
public:
    using Map = clht_str::ClhtStrPooled*;
    
    static Map create(size_t capacity) {
        // Estimate pool size: avg key length ~16 bytes, total elements
        size_t pool_size = capacity * 24;
        return new clht_str::ClhtStrPooled(capacity * 2, pool_size);
    }
    static void insert(Map& ht, const std::string& k, uint64_t v) {
        ht->insert(k, v);
    }
    static uint64_t lookup(Map& ht, const std::string& k) {
        uintptr_t val = ht->lookup(k);
        return (val == UINTPTR_MAX) ? 0 : val;
    }
    static void destroy(Map& ht) {
        delete ht;
    }
};

class ClhtStrTaggedWrapper {
public:
    using Map = clht_str::ClhtStrTagged*;
    
    static Map create(size_t capacity) {
        return new clht_str::ClhtStrTagged(capacity * 2);
    }
    static void insert(Map& ht, const std::string& k, uint64_t v) {
        ht->insert(k, v);
    }
    static uint64_t lookup(Map& ht, const std::string& k) {
        uintptr_t val = ht->lookup(k);
        return (val == UINTPTR_MAX) ? 0 : val;
    }
    static void destroy(Map& ht) {
        delete ht;
    }
};

class ClhtStrFinalWrapper {
public:
    using Map = clht_str::ClhtStrFinal*;
    
    static Map create(size_t capacity) {
        return new clht_str::ClhtStrFinal(capacity * 2);
    }
    static void insert(Map& ht, const std::string& k, uint64_t v) {
        ht->insert(k, v);
    }
    static uint64_t lookup(Map& ht, const std::string& k) {
        uintptr_t val = ht->lookup(k);
        return (val == UINTPTR_MAX) ? 0 : val;
    }
    static void destroy(Map& ht) {
        delete ht;
    }
};

} // namespace clht_test
