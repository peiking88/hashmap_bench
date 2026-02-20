#pragma once

#include <string>
#include <cstdint>

// Standard library
#include <unordered_map>
#include <unordered_set>

// Google sparsehash
#include <google/dense_hash_map>
#include <google/sparse_hash_map>

// Abseil
#include "absl/container/flat_hash_map.h"
#include "absl/container/node_hash_map.h"

// Cista
#include "cista/containers/hash_map.h"

// rhashmap (C library)
extern "C" {
#include <rhashmap.h>
}

// Boost.Container
#include <boost/container/flat_map.hpp>

// libcuckoo
#include <libcuckoo/cuckoohash_map.hh>

// parallel-hashmap
#include <parallel_hashmap/phmap.h>

// Folly F14 (minimal build)
#include <folly/container/F14Map.h>

// OPIC Robin Hood Hash
extern "C" {
#include <opic/op_malloc.h>
#include <opic/hash/op_hash_table.h>
#include <opic/hash/op_hash.h>
}

// CLHT (lock-based and lock-free hash tables)
extern "C" {
#include <clht.h>
#include <ssmem.h>
}

#include "benchmark.hpp"

namespace hashmap_bench {

// ============================================================================
// std::unordered_map wrapper
// ============================================================================
template <typename Key, typename Value>
class StdUnorderedMapWrapper {
public:
    using Map = std::unordered_map<Key, Value>;
    
    static Map create(size_t capacity) { return Map(capacity); }
    static void insert(Map& m, const Key& k, Value v) { m[k] = v; }
    static Value lookup(Map& m, const Key& k) { return m.at(k); }
    static void destroy(Map&) {}
};

// ============================================================================
// absl::flat_hash_map wrapper
// ============================================================================
template <typename Key, typename Value>
class AbslFlatHashMapWrapper {
public:
    using Map = absl::flat_hash_map<Key, Value>;
    
    static Map create(size_t capacity) { return Map(capacity); }
    static void insert(Map& m, const Key& k, Value v) { m[k] = v; }
    static Value lookup(Map& m, const Key& k) { return m.at(k); }
    static void destroy(Map&) {}
};

// ============================================================================
// absl::node_hash_map wrapper
// ============================================================================
template <typename Key, typename Value>
class AbslNodeHashMapWrapper {
public:
    using Map = absl::node_hash_map<Key, Value>;
    
    static Map create(size_t capacity) { return Map(capacity); }
    static void insert(Map& m, const Key& k, Value v) { m[k] = v; }
    static Value lookup(Map& m, const Key& k) { return m.at(k); }
    static void destroy(Map&) {}
};

// ============================================================================
// folly::F14FastMap wrapper
// ============================================================================
template <typename Key, typename Value>
class FollyF14FastMapWrapper {
public:
    using Map = folly::F14FastMap<Key, Value>;
    
    static Map create(size_t capacity) { 
        Map m;
        m.reserve(capacity);
        return m;
    }
    static void insert(Map& m, const Key& k, Value v) { m[k] = v; }
    static Value lookup(Map& m, const Key& k) { return m.at(k); }
    static void destroy(Map&) {}
};

// ============================================================================
// cista::raw::hash_map wrapper
// ============================================================================
template <typename Key, typename Value>
class CistaHashMapWrapper {
public:
    using Map = cista::raw::hash_map<Key, Value>;
    
    static Map create(size_t capacity) { 
        Map m;
        // cista hash_map doesn't have reserve, just return empty map
        return m;
    }
    static void insert(Map& m, const Key& k, Value v) { 
        m.emplace(k, v);
    }
    static Value lookup(Map& m, const Key& k) { 
        auto it = m.find(k);
        return it->second;
    }
    static void destroy(Map&) {}
};

// ============================================================================
// boost::container::flat_map wrapper
// ============================================================================
template <typename Key, typename Value>
class BoostFlatMapWrapper {
public:
    using Map = boost::container::flat_map<Key, Value>;
    
    static Map create(size_t capacity) { 
        Map m;
        m.reserve(capacity);
        return m;
    }
    static void insert(Map& m, const Key& k, Value v) { 
        m.emplace(k, v);
    }
    static Value lookup(Map& m, const Key& k) { 
        auto it = m.find(k);
        return it->second;
    }
    static void destroy(Map&) {}
};

// ============================================================================
// google::dense_hash_map wrapper
// ============================================================================
template <typename Key, typename Value>
class DenseHashMapWrapper {
public:
    using Map = google::dense_hash_map<Key, Value>;
    
    static Map create(size_t capacity) { 
        Map m(capacity);
        if constexpr (std::is_same_v<Key, std::string>) {
            m.set_empty_key("\x00");
            m.set_deleted_key("\xff");
        } else if constexpr (std::is_same_v<Key, uint32_t>) {
            m.set_empty_key(~0U);
            m.set_deleted_key(~0U - 1);
        } else if constexpr (std::is_same_v<Key, uint64_t>) {
            m.set_empty_key(~0ULL);
            m.set_deleted_key(~0ULL - 1);
        }
        return m;
    }
    static void insert(Map& m, const Key& k, Value v) { m[k] = v; }
    static Value lookup(Map& m, const Key& k) { return m[k]; }
    static void destroy(Map&) {}
};

// ============================================================================
// google::sparse_hash_map wrapper
// ============================================================================
template <typename Key, typename Value>
class SparseHashMapWrapper {
public:
    using Map = google::sparse_hash_map<Key, Value>;
    
    static Map create(size_t capacity) { 
        Map m(capacity);
        if constexpr (std::is_same_v<Key, std::string>) {
            m.set_deleted_key("\xff");
        } else if constexpr (std::is_same_v<Key, uint32_t>) {
            m.set_deleted_key(~0U);
        } else if constexpr (std::is_same_v<Key, uint64_t>) {
            m.set_deleted_key(~0ULL);
        }
        return m;
    }
    static void insert(Map& m, const Key& k, Value v) { m[k] = v; }
    static Value lookup(Map& m, const Key& k) { return m[k]; }
    static void destroy(Map&) {}
};

// ============================================================================
// libcuckoo::cuckoohash_map wrapper
// ============================================================================
template <typename Key, typename Value>
class CuckooHashMapWrapper {
public:
    using Map = libcuckoo::cuckoohash_map<Key, Value>;
    
    static Map create(size_t capacity) { return Map(capacity); }
    static void insert(Map& m, const Key& k, Value v) { m.insert(k, v); }
    static Value lookup(Map& m, const Key& k) { return m.find(k); }
    static void destroy(Map&) {}
};

// ============================================================================
// rhashmap wrapper (C library - string key only)
// ============================================================================
class RhashmapWrapper {
public:
    using Map = rhashmap_t*;
    
    static Map create(size_t capacity) { 
        return rhashmap_create(capacity, RHM_NONCRYPTO);
    }
    static void insert(Map& m, const std::string& k, uint64_t v) { 
        rhashmap_put(m, k.c_str(), k.length(), reinterpret_cast<void*>(v));
    }
    static uint64_t lookup(Map& m, const std::string& k) { 
        void* val = rhashmap_get(m, k.c_str(), k.length());
        return reinterpret_cast<uint64_t>(val);
    }
    static void destroy(Map& m) { rhashmap_destroy(m); }
};

// ============================================================================
// phmap::flat_hash_map wrapper (parallel-hashmap)
// ============================================================================
template <typename Key, typename Value>
class PhmapFlatHashMapWrapper {
public:
    using Map = phmap::flat_hash_map<Key, Value>;

    static Map create(size_t capacity) {
        Map m;
        m.reserve(capacity);
        return m;
    }
    static void insert(Map& m, const Key& k, Value v) { m[k] = v; }
    static Value lookup(Map& m, const Key& k) { return m.at(k); }
    static void destroy(Map&) {}
};

// ============================================================================
// phmap::parallel_flat_hash_map wrapper (parallel-hashmap)
// ============================================================================
template <typename Key, typename Value>
class PhmapParallelHashMapWrapper {
public:
    using Map = phmap::parallel_flat_hash_map<Key, Value>;

    static Map create(size_t capacity) {
        Map m;
        m.reserve(capacity);
        return m;
    }
    static void insert(Map& m, const Key& k, Value v) { m[k] = v; }
    static Value lookup(Map& m, const Key& k) { return m.at(k); }
    static void destroy(Map&) {}
};

// ============================================================================
// OPIC Robin Hood Hash wrapper
// Only supports integer keys
// ============================================================================
class OpicRobinHoodWrapper {
public:
    struct Context {
        OPHeap* heap;
        OPHashTable* table;
    };
    using Map = Context*;

    static Map create(size_t capacity) {
        Context* ctx = new Context();
        ctx->heap = OPHeapOpenTmp();
        ctx->table = HTNew(ctx->heap, capacity, 0.95, sizeof(uint64_t), sizeof(uint64_t));
        return ctx;
    }
    static void insert(Map& ctx, uint64_t k, uint64_t v) {
        bool is_dup = false;
        HTUpsertCustom(ctx->table, OPDefaultHash, &k,
                       reinterpret_cast<void**>(&v), &is_dup);
    }
    static uint64_t lookup(Map& ctx, uint64_t k) {
        uint64_t* val = reinterpret_cast<uint64_t*>(
            HTGetCustom(ctx->table, OPDefaultHash, &k));
        return val ? *val : 0;
    }
    static void destroy(Map& ctx) {
        HTDestroy(ctx->table);
        OPHeapClose(ctx->heap);
        delete ctx;
    }
};

// ============================================================================
// CLHT wrappers (Lock-Based and Lock-Free hash tables)
// Only supports integer keys (uintptr_t)
// ============================================================================
class ClhtLbWrapper {
public:
    using Map = clht_t*;
    
    static Map create(size_t capacity) {
        clht_t* ht = clht_create(capacity);
        clht_gc_thread_init(ht, 0);
        return ht;
    }
    static void insert(Map& ht, uint64_t k, uint64_t v) {
        clht_put(ht, (clht_addr_t)k, (clht_val_t)v);
    }
    static uint64_t lookup(Map& ht, uint64_t k) {
        return (uint64_t)clht_get(ht->ht, (clht_addr_t)k);
    }
    static void destroy(Map& ht) {
        clht_gc_destroy(ht);
    }
};

class ClhtLfWrapper {
public:
    using Map = clht_t*;
    
    static Map create(size_t capacity) {
        clht_t* ht = clht_create(capacity);
        clht_gc_thread_init(ht, 0);
        return ht;
    }
    static void insert(Map& ht, uint64_t k, uint64_t v) {
        clht_put(ht, (clht_addr_t)k, (clht_val_t)v);
    }
    static uint64_t lookup(Map& ht, uint64_t k) {
        return (uint64_t)clht_get(ht->ht, (clht_addr_t)k);
    }
    static void destroy(Map& ht) {
        clht_gc_destroy(ht);
    }
};

} // namespace hashmap_bench
