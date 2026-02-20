/*
 * CLHT Integer Key Implementation Performance Benchmarks
 * 
 * Simplified benchmark suite for CLHT integer key implementations.
 * Note: CLHT has memory management issues when created/destroyed frequently,
 * so we use a simplified approach to avoid crashes.
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>

#include <cstdint>
#include <vector>
#include <random>
#include <algorithm>
#include <chrono>
#include <unordered_map>

// Abseil
#include "absl/container/flat_hash_map.h"

// Folly F14 (minimal build)
#include <folly/container/F14Map.h>

// CLHT
extern "C" {
#include <clht.h>
#include <ssmem.h>
}

namespace clht_int_bench {

// ============================================================================
// CLHT Integer Key wrappers
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

// ============================================================================
// Benchmark Helpers
// ============================================================================

static std::vector<uint64_t> generate_sequential_keys(size_t count) {
    std::vector<uint64_t> keys(count);
    std::iota(keys.begin(), keys.end(), 1);  // Start from 1 (0 is reserved in CLHT)
    return keys;
}

static std::vector<uint64_t> generate_random_keys(size_t count, uint64_t seed = 42) {
    std::vector<uint64_t> keys(count);
    std::mt19937_64 rng(seed);
    std::uniform_int_distribution<uint64_t> dist(1, UINT64_MAX);  // Avoid 0
    for (auto& k : keys) {
        k = dist(rng);
    }
    return keys;
}

} // namespace clht_int_bench

using namespace clht_int_bench;

// ============================================================================
// Simple Insert Benchmarks (single map creation per test)
// ============================================================================

TEST_CASE("CLHT Int: Insert performance", "[!benchmark][clht_int]") {
    const size_t N = 20000;
    auto keys = generate_sequential_keys(N);
    
    BENCHMARK("ClhtLb insert 20K") {
        ClhtLbWrapper::Map map = ClhtLbWrapper::create(N * 2);
        for (size_t i = 0; i < N; ++i) {
            ClhtLbWrapper::insert(map, keys[i], i);
        }
        return N;
    };
    
    BENCHMARK("ClhtLf insert 20K") {
        ClhtLfWrapper::Map map = ClhtLfWrapper::create(N * 2);
        for (size_t i = 0; i < N; ++i) {
            ClhtLfWrapper::insert(map, keys[i], i);
        }
        return N;
    };
    
    BENCHMARK("std::unordered_map insert 20K") {
        std::unordered_map<uint64_t, uint64_t> map;
        map.reserve(N);
        for (size_t i = 0; i < N; ++i) {
            map[keys[i]] = i;
        }
        return map.size();
    };
    
    BENCHMARK("absl::flat_hash_map insert 20K") {
        absl::flat_hash_map<uint64_t, uint64_t> map;
        map.reserve(N);
        for (size_t i = 0; i < N; ++i) {
            map[keys[i]] = i;
        }
        return map.size();
    };
    
    BENCHMARK("folly::F14FastMap insert 20K") {
        folly::F14FastMap<uint64_t, uint64_t> map;
        map.reserve(N);
        for (size_t i = 0; i < N; ++i) {
            map[keys[i]] = i;
        }
        return map.size();
    };
}

// ============================================================================
// Lookup Performance (pre-populated maps)
// ============================================================================

TEST_CASE("CLHT Int: Lookup performance", "[!benchmark][clht_int]") {
    const size_t N = 20000;
    auto keys = generate_sequential_keys(N);
    
    // Pre-populate CLHT maps (create once, use for all lookups)
    ClhtLbWrapper::Map lb_map = ClhtLbWrapper::create(N * 2);
    ClhtLfWrapper::Map lf_map = ClhtLfWrapper::create(N * 2);
    
    for (size_t i = 0; i < N; ++i) {
        ClhtLbWrapper::insert(lb_map, keys[i], i);
        ClhtLfWrapper::insert(lf_map, keys[i], i);
    }
    
    // Pre-populate reference maps
    std::unordered_map<uint64_t, uint64_t> std_map;
    absl::flat_hash_map<uint64_t, uint64_t> absl_map;
    folly::F14FastMap<uint64_t, uint64_t> folly_map;
    
    for (size_t i = 0; i < N; ++i) {
        std_map[keys[i]] = i;
        absl_map[keys[i]] = i;
        folly_map[keys[i]] = i;
    }
    
    BENCHMARK("ClhtLb lookup 20K") {
        uint64_t sum = 0;
        for (size_t i = 0; i < N; ++i) {
            sum += ClhtLbWrapper::lookup(lb_map, keys[i]);
        }
        return sum;
    };
    
    BENCHMARK("ClhtLf lookup 20K") {
        uint64_t sum = 0;
        for (size_t i = 0; i < N; ++i) {
            sum += ClhtLfWrapper::lookup(lf_map, keys[i]);
        }
        return sum;
    };
    
    BENCHMARK("std::unordered_map lookup 20K") {
        uint64_t sum = 0;
        for (size_t i = 0; i < N; ++i) {
            sum += std_map.at(keys[i]);
        }
        return sum;
    };
    
    BENCHMARK("absl::flat_hash_map lookup 20K") {
        uint64_t sum = 0;
        for (size_t i = 0; i < N; ++i) {
            sum += absl_map.at(keys[i]);
        }
        return sum;
    };
    
    BENCHMARK("folly::F14FastMap lookup 20K") {
        uint64_t sum = 0;
        for (size_t i = 0; i < N; ++i) {
            sum += folly_map.at(keys[i]);
        }
        return sum;
    };
}

// ============================================================================
// Random Key Insert Benchmarks
// ============================================================================

TEST_CASE("CLHT Int: Random key insert", "[!benchmark][clht_int]") {
    const size_t N = 20000;
    auto keys = generate_random_keys(N);
    
    BENCHMARK("ClhtLb random insert 20K") {
        ClhtLbWrapper::Map map = ClhtLbWrapper::create(N * 2);
        for (size_t i = 0; i < N; ++i) {
            ClhtLbWrapper::insert(map, keys[i], i);
        }
        return N;
    };
    
    BENCHMARK("ClhtLf random insert 20K") {
        ClhtLfWrapper::Map map = ClhtLfWrapper::create(N * 2);
        for (size_t i = 0; i < N; ++i) {
            ClhtLfWrapper::insert(map, keys[i], i);
        }
        return N;
    };
    
    BENCHMARK("absl::flat_hash_map random insert 20K") {
        absl::flat_hash_map<uint64_t, uint64_t> map;
        map.reserve(N);
        for (size_t i = 0; i < N; ++i) {
            map[keys[i]] = i;
        }
        return map.size();
    };
    
    BENCHMARK("folly::F14FastMap random insert 20K") {
        folly::F14FastMap<uint64_t, uint64_t> map;
        map.reserve(N);
        for (size_t i = 0; i < N; ++i) {
            map[keys[i]] = i;
        }
        return map.size();
    };
}

// ============================================================================
// Mixed Operations Benchmark
// ============================================================================

TEST_CASE("CLHT Int: Mixed operations", "[!benchmark][clht_int]") {
    const size_t N = 10000;
    auto keys = generate_sequential_keys(N * 2);
    
    BENCHMARK("ClhtLb mixed 80% lookup 20% insert") {
        ClhtLbWrapper::Map map = ClhtLbWrapper::create(N * 2);
        
        // Initial fill
        for (size_t i = 0; i < N / 2; ++i) {
            ClhtLbWrapper::insert(map, keys[i], i);
        }
        
        uint64_t sum = 0;
        size_t insert_idx = N / 2;
        
        for (size_t i = 0; i < N * 4; ++i) {
            if (i % 5 == 0 && insert_idx < N) {
                ClhtLbWrapper::insert(map, keys[insert_idx], insert_idx);
                ++insert_idx;
            } else {
                sum += ClhtLbWrapper::lookup(map, keys[i % (N / 2)]);
            }
        }
        
        return sum;
    };
    
    BENCHMARK("ClhtLf mixed 80% lookup 20% insert") {
        ClhtLfWrapper::Map map = ClhtLfWrapper::create(N * 2);
        
        // Initial fill
        for (size_t i = 0; i < N / 2; ++i) {
            ClhtLfWrapper::insert(map, keys[i], i);
        }
        
        uint64_t sum = 0;
        size_t insert_idx = N / 2;
        
        for (size_t i = 0; i < N * 4; ++i) {
            if (i % 5 == 0 && insert_idx < N) {
                ClhtLfWrapper::insert(map, keys[insert_idx], insert_idx);
                ++insert_idx;
            } else {
                sum += ClhtLfWrapper::lookup(map, keys[i % (N / 2)]);
            }
        }
        
        return sum;
    };
}
