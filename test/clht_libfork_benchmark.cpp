/*
 * CLHT libfork Parallel Benchmark Tests
 * 
 * Comprehensive performance comparison:
 * - Serial vs Parallel operations
 * - Scaling with thread count
 * - Comparison with other hashmap implementations
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>
#include <vector>
#include <string>
#include <random>
#include <algorithm>
#include <chrono>
#include <thread>
#include <unordered_map>

// Abseil
#include "absl/container/flat_hash_map.h"

// Folly F14
#include <folly/container/F14Map.h>

// CLHT libfork implementations
#include "clht_libfork_str.hpp"
#include "clht_libfork_int.hpp"

// CLHT serial implementations
#include "clht_str_final.hpp"

// ============================================================================
// Test Helpers
// ============================================================================

static std::vector<std::string> generate_string_keys(size_t count, size_t key_len = 16) {
    std::vector<std::string> keys;
    keys.reserve(count);
    
    for (size_t i = 0; i < count; ++i) {
        std::string key = "bench_key_" + std::to_string(i) + "_";
        key.resize(key_len, 'x');
        keys.push_back(key);
    }
    return keys;
}

static std::vector<uintptr_t> generate_int_keys(size_t count) {
    std::vector<uintptr_t> keys;
    keys.reserve(count);
    
    for (size_t i = 0; i < count; ++i) {
        keys.push_back(i * 2 + 1);
    }
    return keys;
}

// ============================================================================
// String Key: Parallel Scaling Benchmarks
// ============================================================================

TEST_CASE("CLHT String: Insert scaling by thread count", "[!benchmark][clht_libfork][str][scaling]") {
    const size_t N = 50000;
    auto keys = generate_string_keys(N, 16);
    std::vector<uintptr_t> values(N);
    for (size_t i = 0; i < N; ++i) values[i] = i;
    
    BENCHMARK("1 thread") {
        clht_libfork::ParallelClhtStr ht(N * 2, 1);
        ht.batch_insert(keys, values);
        return ht.size();
    };
    
    BENCHMARK("2 threads") {
        clht_libfork::ParallelClhtStr ht(N * 2, 2);
        ht.batch_insert(keys, values);
        return ht.size();
    };
    
    BENCHMARK("4 threads") {
        clht_libfork::ParallelClhtStr ht(N * 2, 4);
        ht.batch_insert(keys, values);
        return ht.size();
    };
    
    BENCHMARK("8 threads") {
        clht_libfork::ParallelClhtStr ht(N * 2, 8);
        ht.batch_insert(keys, values);
        return ht.size();
    };
}

TEST_CASE("CLHT String: Lookup scaling by thread count", "[!benchmark][clht_libfork][str][scaling]") {
    const size_t N = 50000;
    auto keys = generate_string_keys(N, 16);
    std::vector<uintptr_t> values(N);
    for (size_t i = 0; i < N; ++i) values[i] = i;
    
    // Pre-populate with all cores
    clht_libfork::ParallelClhtStr pre_ht(N * 2);
    pre_ht.batch_insert(keys, values);
    
    BENCHMARK("1 thread") {
        clht_libfork::ParallelClhtStr ht(N * 2, 1);
        ht.batch_insert(keys, values);
        std::vector<uintptr_t> results;
        ht.batch_lookup(keys, results);
        return results[0];
    };
    
    BENCHMARK("2 threads") {
        clht_libfork::ParallelClhtStr ht(N * 2, 2);
        ht.batch_insert(keys, values);
        std::vector<uintptr_t> results;
        ht.batch_lookup(keys, results);
        return results[0];
    };
    
    BENCHMARK("4 threads") {
        clht_libfork::ParallelClhtStr ht(N * 2, 4);
        ht.batch_insert(keys, values);
        std::vector<uintptr_t> results;
        ht.batch_lookup(keys, results);
        return results[0];
    };
    
    BENCHMARK("8 threads") {
        clht_libfork::ParallelClhtStr ht(N * 2, 8);
        ht.batch_insert(keys, values);
        std::vector<uintptr_t> results;
        ht.batch_lookup(keys, results);
        return results[0];
    };
}

// ============================================================================
// Integer Key: Parallel Scaling Benchmarks
// ============================================================================

TEST_CASE("CLHT Integer: Insert scaling by thread count", "[!benchmark][clht_libfork][int][scaling]") {
    const size_t N = 50000;
    auto keys = generate_int_keys(N);
    std::vector<uintptr_t> values(N);
    for (size_t i = 0; i < N; ++i) values[i] = i;
    
    BENCHMARK("1 thread") {
        clht_libfork::ParallelClhtInt ht(N * 2, 1);
        ht.batch_insert(keys, values);
        return ht.size();
    };
    
    BENCHMARK("2 threads") {
        clht_libfork::ParallelClhtInt ht(N * 2, 2);
        ht.batch_insert(keys, values);
        return ht.size();
    };
    
    BENCHMARK("4 threads") {
        clht_libfork::ParallelClhtInt ht(N * 2, 4);
        ht.batch_insert(keys, values);
        return ht.size();
    };
    
    BENCHMARK("8 threads") {
        clht_libfork::ParallelClhtInt ht(N * 2, 8);
        ht.batch_insert(keys, values);
        return ht.size();
    };
}

TEST_CASE("CLHT Integer: Lookup scaling by thread count", "[!benchmark][clht_libfork][int][scaling]") {
    const size_t N = 50000;
    auto keys = generate_int_keys(N);
    std::vector<uintptr_t> values(N);
    for (size_t i = 0; i < N; ++i) values[i] = i;
    
    BENCHMARK("1 thread") {
        clht_libfork::ParallelClhtInt ht(N * 2, 1);
        ht.batch_insert(keys, values);
        std::vector<uintptr_t> results;
        ht.batch_lookup(keys, results);
        return results[0];
    };
    
    BENCHMARK("2 threads") {
        clht_libfork::ParallelClhtInt ht(N * 2, 2);
        ht.batch_insert(keys, values);
        std::vector<uintptr_t> results;
        ht.batch_lookup(keys, results);
        return results[0];
    };
    
    BENCHMARK("4 threads") {
        clht_libfork::ParallelClhtInt ht(N * 2, 4);
        ht.batch_insert(keys, values);
        std::vector<uintptr_t> results;
        ht.batch_lookup(keys, results);
        return results[0];
    };
    
    BENCHMARK("8 threads") {
        clht_libfork::ParallelClhtInt ht(N * 2, 8);
        ht.batch_insert(keys, values);
        std::vector<uintptr_t> results;
        ht.batch_lookup(keys, results);
        return results[0];
    };
}

// ============================================================================
// Comparison: CLHT libfork vs std::unordered_map vs Abseil vs Folly
// ============================================================================

TEST_CASE("String Hashmap Comparison: Batch Insert", "[!benchmark][clht_libfork][str][comparison]") {
    const size_t N = 20000;
    auto keys = generate_string_keys(N, 16);
    std::vector<uintptr_t> values(N);
    for (size_t i = 0; i < N; ++i) values[i] = i;
    
    BENCHMARK("std::unordered_map (serial)") {
        std::unordered_map<std::string, uintptr_t> map;
        map.reserve(N);
        for (size_t i = 0; i < N; ++i) {
            map[keys[i]] = values[i];
        }
        return map.size();
    };
    
    BENCHMARK("absl::flat_hash_map (serial)") {
        absl::flat_hash_map<std::string, uintptr_t> map;
        map.reserve(N);
        for (size_t i = 0; i < N; ++i) {
            map[keys[i]] = values[i];
        }
        return map.size();
    };
    
    BENCHMARK("folly::F14FastMap (serial)") {
        folly::F14FastMap<std::string, uintptr_t> map;
        map.reserve(N);
        for (size_t i = 0; i < N; ++i) {
            map[keys[i]] = values[i];
        }
        return map.size();
    };
    
    BENCHMARK("CLHT serial") {
        clht_str::ClhtStrFinal ht(N * 2);
        for (size_t i = 0; i < N; ++i) {
            ht.insert(keys[i], values[i]);
        }
        return ht.size();
    };
    
    BENCHMARK("CLHT libfork parallel") {
        clht_libfork::ParallelClhtStr ht(N * 2);
        ht.batch_insert(keys, values);
        return ht.size();
    };
}

TEST_CASE("String Hashmap Comparison: Batch Lookup", "[!benchmark][clht_libfork][str][comparison]") {
    const size_t N = 20000;
    auto keys = generate_string_keys(N, 16);
    std::vector<uintptr_t> values(N);
    for (size_t i = 0; i < N; ++i) values[i] = i;
    
    // Pre-populate all maps
    std::unordered_map<std::string, uintptr_t> std_map;
    std_map.reserve(N);
    for (size_t i = 0; i < N; ++i) std_map[keys[i]] = values[i];
    
    absl::flat_hash_map<std::string, uintptr_t> absl_map;
    absl_map.reserve(N);
    for (size_t i = 0; i < N; ++i) absl_map[keys[i]] = values[i];
    
    folly::F14FastMap<std::string, uintptr_t> folly_map;
    folly_map.reserve(N);
    for (size_t i = 0; i < N; ++i) folly_map[keys[i]] = values[i];
    
    clht_str::ClhtStrFinal clht_serial(N * 2);
    for (size_t i = 0; i < N; ++i) clht_serial.insert(keys[i], values[i]);
    
    clht_libfork::ParallelClhtStr clht_parallel(N * 2);
    clht_parallel.batch_insert(keys, values);
    
    BENCHMARK("std::unordered_map (serial)") {
        uintptr_t sum = 0;
        for (size_t i = 0; i < N; ++i) {
            sum += std_map[keys[i]];
        }
        return sum;
    };
    
    BENCHMARK("absl::flat_hash_map (serial)") {
        uintptr_t sum = 0;
        for (size_t i = 0; i < N; ++i) {
            sum += absl_map[keys[i]];
        }
        return sum;
    };
    
    BENCHMARK("folly::F14FastMap (serial)") {
        uintptr_t sum = 0;
        for (size_t i = 0; i < N; ++i) {
            sum += folly_map[keys[i]];
        }
        return sum;
    };
    
    BENCHMARK("CLHT serial") {
        uintptr_t sum = 0;
        for (size_t i = 0; i < N; ++i) {
            sum += clht_serial.lookup(keys[i]);
        }
        return sum;
    };
    
    BENCHMARK("CLHT libfork parallel") {
        std::vector<uintptr_t> results;
        clht_parallel.batch_lookup(keys, results);
        return results[0];
    };
}

// ============================================================================
// Integer Hashmap Comparison
// ============================================================================

TEST_CASE("Integer Hashmap Comparison: Batch Insert", "[!benchmark][clht_libfork][int][comparison]") {
    const size_t N = 20000;
    auto keys = generate_int_keys(N);
    std::vector<uintptr_t> values(N);
    for (size_t i = 0; i < N; ++i) values[i] = i;
    
    BENCHMARK("std::unordered_map (serial)") {
        std::unordered_map<uintptr_t, uintptr_t> map;
        map.reserve(N);
        for (size_t i = 0; i < N; ++i) {
            map[keys[i]] = values[i];
        }
        return map.size();
    };
    
    BENCHMARK("absl::flat_hash_map (serial)") {
        absl::flat_hash_map<uintptr_t, uintptr_t> map;
        map.reserve(N);
        for (size_t i = 0; i < N; ++i) {
            map[keys[i]] = values[i];
        }
        return map.size();
    };
    
    BENCHMARK("folly::F14FastMap (serial)") {
        folly::F14FastMap<uintptr_t, uintptr_t> map;
        map.reserve(N);
        for (size_t i = 0; i < N; ++i) {
            map[keys[i]] = values[i];
        }
        return map.size();
    };
    
    BENCHMARK("CLHT libfork parallel") {
        clht_libfork::ParallelClhtInt ht(N * 2);
        ht.batch_insert(keys, values);
        return ht.size();
    };
}

TEST_CASE("Integer Hashmap Comparison: Batch Lookup", "[!benchmark][clht_libfork][int][comparison]") {
    const size_t N = 20000;
    auto keys = generate_int_keys(N);
    std::vector<uintptr_t> values(N);
    for (size_t i = 0; i < N; ++i) values[i] = i;
    
    // Pre-populate
    std::unordered_map<uintptr_t, uintptr_t> std_map;
    std_map.reserve(N);
    for (size_t i = 0; i < N; ++i) std_map[keys[i]] = values[i];
    
    absl::flat_hash_map<uintptr_t, uintptr_t> absl_map;
    absl_map.reserve(N);
    for (size_t i = 0; i < N; ++i) absl_map[keys[i]] = values[i];
    
    folly::F14FastMap<uintptr_t, uintptr_t> folly_map;
    folly_map.reserve(N);
    for (size_t i = 0; i < N; ++i) folly_map[keys[i]] = values[i];
    
    clht_libfork::ParallelClhtInt clht_parallel(N * 2);
    clht_parallel.batch_insert(keys, values);
    
    BENCHMARK("std::unordered_map (serial)") {
        uintptr_t sum = 0;
        for (size_t i = 0; i < N; ++i) {
            sum += std_map[keys[i]];
        }
        return sum;
    };
    
    BENCHMARK("absl::flat_hash_map (serial)") {
        uintptr_t sum = 0;
        for (size_t i = 0; i < N; ++i) {
            sum += absl_map[keys[i]];
        }
        return sum;
    };
    
    BENCHMARK("folly::F14FastMap (serial)") {
        uintptr_t sum = 0;
        for (size_t i = 0; i < N; ++i) {
            sum += folly_map[keys[i]];
        }
        return sum;
    };
    
    BENCHMARK("CLHT libfork parallel") {
        std::vector<uintptr_t> results;
        clht_parallel.batch_lookup(keys, results);
        return results[0];
    };
}

// ============================================================================
// Load Factor Impact
// ============================================================================

TEST_CASE("CLHT String: Load factor impact on parallel performance", "[!benchmark][clht_libfork][str][load]") {
    const size_t N = 10000;
    auto keys = generate_string_keys(N, 16);
    std::vector<uintptr_t> values(N);
    for (size_t i = 0; i < N; ++i) values[i] = i;
    
    BENCHMARK("Load factor 25%") {
        clht_libfork::ParallelClhtStr ht(N * 4);  // 4x capacity
        ht.batch_insert(keys, values);
        return ht.size();
    };
    
    BENCHMARK("Load factor 50%") {
        clht_libfork::ParallelClhtStr ht(N * 2);  // 2x capacity
        ht.batch_insert(keys, values);
        return ht.size();
    };
    
    BENCHMARK("Load factor 75%") {
        clht_libfork::ParallelClhtStr ht(N * 4 / 3);  // 1.33x capacity
        ht.batch_insert(keys, values);
        return ht.size();
    };
    
    BENCHMARK("Load factor 90%") {
        clht_libfork::ParallelClhtStr ht(N * 10 / 9);  // 1.11x capacity
        ht.batch_insert(keys, values);
        return ht.size();
    };
}

// ============================================================================
// Key Length Impact
// ============================================================================

TEST_CASE("CLHT String: Key length impact on parallel performance", "[!benchmark][clht_libfork][str][keylen]") {
    const size_t N = 5000;
    std::vector<uintptr_t> values(N);
    for (size_t i = 0; i < N; ++i) values[i] = i;
    
    BENCHMARK("8 byte keys") {
        auto keys = generate_string_keys(N, 8);
        clht_libfork::ParallelClhtStr ht(N * 2);
        ht.batch_insert(keys, values);
        return ht.size();
    };
    
    BENCHMARK("32 byte keys") {
        auto keys = generate_string_keys(N, 32);
        clht_libfork::ParallelClhtStr ht(N * 2);
        ht.batch_insert(keys, values);
        return ht.size();
    };
    
    BENCHMARK("128 byte keys") {
        auto keys = generate_string_keys(N, 128);
        clht_libfork::ParallelClhtStr ht(N * 2);
        ht.batch_insert(keys, values);
        return ht.size();
    };
    
    BENCHMARK("256 byte keys") {
        auto keys = generate_string_keys(N, 256);
        clht_libfork::ParallelClhtStr ht(N * 2);
        ht.batch_insert(keys, values);
        return ht.size();
    };
}
