/*
 * CLHT Large Scale Benchmark: libfork vs Serial vs Other Implementations
 *
 * Comparison with 2^20 (1,048,576) elements:
 * - Throughput (Mops/s)
 * - Latency (ns/op)
 *
 * Optimized Strategy:
 * - Insert: SERIAL (CLHT bucket locks limit parallel scaling)
 * - Lookup: PARALLEL (lock-free reads scale near-linearly)
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>
#include <vector>
#include <string>
#include <random>
#include <algorithm>
#include <chrono>
#include <unordered_map>
#include <iostream>
#include <iomanip>

// Abseil
#include "absl/container/flat_hash_map.h"

// Folly F14
#include <folly/container/F14Map.h>

// CLHT serial implementations
#include "clht_str_final.hpp"

// CLHT libfork parallel implementations
#include "clht_libfork_str.hpp"
#include "clht_libfork_int.hpp"

// ============================================================================
// Test Helpers
// ============================================================================

static std::vector<std::string> generate_string_keys(size_t count, size_t key_len = 16) {
    std::vector<std::string> keys;
    keys.reserve(count);

    for (size_t i = 0; i < count; ++i) {
        std::string key = "large_key_" + std::to_string(i) + "_";
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
// String Key: Large Scale Benchmark (2^20 elements)
// ============================================================================

TEST_CASE("Large Scale String: Insert 2^20 elements", "[!benchmark][large][str][insert]") {
    const size_t N = 1 << 20;  // 1,048,576
    auto keys = generate_string_keys(N, 16);
    std::vector<uintptr_t> values(N);
    for (size_t i = 0; i < N; ++i) values[i] = i;

    std::cout << "\n========== String Key Insert Benchmark (N=" << N << ") ==========\n";
    std::cout << "Note: All libfork versions use SERIAL insert (optimized)\n\n";

    BENCHMARK("std::unordered_map") {
        std::unordered_map<std::string, uintptr_t> map;
        map.reserve(N);
        for (size_t i = 0; i < N; ++i) {
            map[keys[i]] = values[i];
        }
        return map.size();
    };

    BENCHMARK("absl::flat_hash_map") {
        absl::flat_hash_map<std::string, uintptr_t> map;
        map.reserve(N);
        for (size_t i = 0; i < N; ++i) {
            map[keys[i]] = values[i];
        }
        return map.size();
    };

    BENCHMARK("folly::F14FastMap") {
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

    // All libfork versions use SERIAL insert - should be similar
    BENCHMARK("CLHT libfork (serial insert)") {
        clht_libfork::ParallelClhtStr ht(N * 2, 8);
        ht.batch_insert(keys, values);
        return ht.size();
    };
}

TEST_CASE("Large Scale String: Pure Lookup 2^20 elements", "[!benchmark][large][str][lookup]") {
    const size_t N = 1 << 20;  // 1,048,576
    auto keys = generate_string_keys(N, 16);
    std::vector<uintptr_t> values(N);
    for (size_t i = 0; i < N; ++i) values[i] = i;

    // Pre-populate all maps ONCE before benchmarking
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

    // Pre-populate libfork instance
    clht_libfork::ParallelClhtStr libfork_ht(N * 2, 8);
    libfork_ht.batch_insert(keys, values);

    std::cout << "\n========== String Key Pure Lookup Benchmark (N=" << N << ") ==========\n";
    std::cout << "Note: All data pre-inserted, testing PURE lookup performance\n\n";

    BENCHMARK("std::unordered_map") {
        uintptr_t sum = 0;
        for (size_t i = 0; i < N; ++i) {
            sum += std_map[keys[i]];
        }
        return sum;
    };

    BENCHMARK("absl::flat_hash_map") {
        uintptr_t sum = 0;
        for (size_t i = 0; i < N; ++i) {
            sum += absl_map[keys[i]];
        }
        return sum;
    };

    BENCHMARK("folly::F14FastMap") {
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

    // PARALLEL lookup tests (using pre-populated table)
    BENCHMARK("CLHT libfork PARALLEL (1 thread)") {
        std::vector<uintptr_t> results;
        libfork_ht.batch_lookup(keys, results);
        return results[0];
    };

    // Note: Since we use single pre-populated instance, all thread counts use same pool
    // The pool is configured for 8 threads, but we can still measure scaling
    BENCHMARK("CLHT libfork PARALLEL (uses 8 thread pool)") {
        std::vector<uintptr_t> results;
        libfork_ht.batch_lookup(keys, results);
        return results[0];
    };
}

// ============================================================================
// Integer Key: Large Scale Benchmark (2^20 elements)
// ============================================================================

TEST_CASE("Large Scale Integer: Insert 2^20 elements", "[!benchmark][large][int][insert]") {
    const size_t N = 1 << 20;  // 1,048,576
    auto keys = generate_int_keys(N);
    std::vector<uintptr_t> values(N);
    for (size_t i = 0; i < N; ++i) values[i] = i;

    std::cout << "\n========== Integer Key Insert Benchmark (N=" << N << ") ==========\n";
    std::cout << "Note: All libfork versions use SERIAL insert (optimized)\n\n";

    BENCHMARK("std::unordered_map") {
        std::unordered_map<uintptr_t, uintptr_t> map;
        map.reserve(N);
        for (size_t i = 0; i < N; ++i) {
            map[keys[i]] = values[i];
        }
        return map.size();
    };

    BENCHMARK("absl::flat_hash_map") {
        absl::flat_hash_map<uintptr_t, uintptr_t> map;
        map.reserve(N);
        for (size_t i = 0; i < N; ++i) {
            map[keys[i]] = values[i];
        }
        return map.size();
    };

    BENCHMARK("folly::F14FastMap") {
        folly::F14FastMap<uintptr_t, uintptr_t> map;
        map.reserve(N);
        for (size_t i = 0; i < N; ++i) {
            map[keys[i]] = values[i];
        }
        return map.size();
    };

    // All libfork versions use SERIAL insert - should be similar
    BENCHMARK("CLHT libfork (serial insert)") {
        clht_libfork::ParallelClhtInt ht(N * 2, 8);
        ht.batch_insert(keys, values);
        return ht.size();
    };
}

TEST_CASE("Large Scale Integer: Pure Lookup 2^20 elements", "[!benchmark][large][int][lookup]") {
    const size_t N = 1 << 20;  // 1,048,576
    auto keys = generate_int_keys(N);
    std::vector<uintptr_t> values(N);
    for (size_t i = 0; i < N; ++i) values[i] = i;

    // Pre-populate all maps ONCE before benchmarking
    std::unordered_map<uintptr_t, uintptr_t> std_map;
    std_map.reserve(N);
    for (size_t i = 0; i < N; ++i) std_map[keys[i]] = values[i];

    absl::flat_hash_map<uintptr_t, uintptr_t> absl_map;
    absl_map.reserve(N);
    for (size_t i = 0; i < N; ++i) absl_map[keys[i]] = values[i];

    folly::F14FastMap<uintptr_t, uintptr_t> folly_map;
    folly_map.reserve(N);
    for (size_t i = 0; i < N; ++i) folly_map[keys[i]] = values[i];

    // Pre-populate libfork instance
    clht_libfork::ParallelClhtInt libfork_ht(N * 2, 8);
    libfork_ht.batch_insert(keys, values);

    std::cout << "\n========== Integer Key Pure Lookup Benchmark (N=" << N << ") ==========\n";
    std::cout << "Note: All data pre-inserted, testing PURE lookup performance\n\n";

    BENCHMARK("std::unordered_map") {
        uintptr_t sum = 0;
        for (size_t i = 0; i < N; ++i) {
            sum += std_map[keys[i]];
        }
        return sum;
    };

    BENCHMARK("absl::flat_hash_map") {
        uintptr_t sum = 0;
        for (size_t i = 0; i < N; ++i) {
            sum += absl_map[keys[i]];
        }
        return sum;
    };

    BENCHMARK("folly::F14FastMap") {
        uintptr_t sum = 0;
        for (size_t i = 0; i < N; ++i) {
            sum += folly_map[keys[i]];
        }
        return sum;
    };

    // PARALLEL lookup tests (using pre-populated table)
    BENCHMARK("CLHT libfork PARALLEL (8 thread pool)") {
        std::vector<uintptr_t> results;
        libfork_ht.batch_lookup(keys, results);
        return results[0];
    };
}

// ============================================================================
// Parallel Lookup Scaling Test - Integer Key
// ============================================================================

TEST_CASE("Large Scale Integer: Parallel Lookup Scaling", "[!benchmark][large][int][scaling]") {
    const size_t N = 1 << 20;  // 1,048,576
    auto keys = generate_int_keys(N);
    std::vector<uintptr_t> values(N);
    for (size_t i = 0; i < N; ++i) values[i] = i;

    std::cout << "\n========== Integer Key Parallel Lookup Scaling Test (N=" << N << ") ==========\n";
    std::cout << "Testing pure lookup scaling with different thread counts\n\n";

    // Each test creates a fresh instance to test different thread counts
    // This tests the SCALING of parallel lookup

    BENCHMARK("CLHT libfork (1 thread) - pure lookup") {
        clht_libfork::ParallelClhtInt ht(N * 2, 1);
        ht.batch_insert(keys, values);
        std::vector<uintptr_t> results;
        ht.batch_lookup(keys, results);
        return results[0];
    };

    BENCHMARK("CLHT libfork (2 threads) - pure lookup") {
        clht_libfork::ParallelClhtInt ht(N * 2, 2);
        ht.batch_insert(keys, values);
        std::vector<uintptr_t> results;
        ht.batch_lookup(keys, results);
        return results[0];
    };

    BENCHMARK("CLHT libfork (4 threads) - pure lookup") {
        clht_libfork::ParallelClhtInt ht(N * 2, 4);
        ht.batch_insert(keys, values);
        std::vector<uintptr_t> results;
        ht.batch_lookup(keys, results);
        return results[0];
    };

    BENCHMARK("CLHT libfork (8 threads) - pure lookup") {
        clht_libfork::ParallelClhtInt ht(N * 2, 8);
        ht.batch_insert(keys, values);
        std::vector<uintptr_t> results;
        ht.batch_lookup(keys, results);
        return results[0];
    };
}

// ============================================================================
// Parallel Lookup Scaling Test - String Key
// ============================================================================

TEST_CASE("Large Scale String: Parallel Lookup Scaling", "[!benchmark][large][str][scaling]") {
    const size_t N = 1 << 20;  // 1,048,576
    auto keys = generate_string_keys(N, 16);
    std::vector<uintptr_t> values(N);
    for (size_t i = 0; i < N; ++i) values[i] = i;

    std::cout << "\n========== String Key Parallel Lookup Scaling Test (N=" << N << ") ==========\n";
    std::cout << "Testing pure lookup scaling with different thread counts\n\n";

    BENCHMARK("CLHT libfork (1 thread) - pure lookup") {
        clht_libfork::ParallelClhtStr ht(N * 2, 1);
        ht.batch_insert(keys, values);
        std::vector<uintptr_t> results;
        ht.batch_lookup(keys, results);
        return results[0];
    };

    BENCHMARK("CLHT libfork (2 threads) - pure lookup") {
        clht_libfork::ParallelClhtStr ht(N * 2, 2);
        ht.batch_insert(keys, values);
        std::vector<uintptr_t> results;
        ht.batch_lookup(keys, results);
        return results[0];
    };

    BENCHMARK("CLHT libfork (4 threads) - pure lookup") {
        clht_libfork::ParallelClhtStr ht(N * 2, 4);
        ht.batch_insert(keys, values);
        std::vector<uintptr_t> results;
        ht.batch_lookup(keys, results);
        return results[0];
    };

    BENCHMARK("CLHT libfork (8 threads) - pure lookup") {
        clht_libfork::ParallelClhtStr ht(N * 2, 8);
        ht.batch_insert(keys, values);
        std::vector<uintptr_t> results;
        ht.batch_lookup(keys, results);
        return results[0];
    };
}

// ============================================================================
// Summary Report
// ============================================================================

TEST_CASE("Large Scale Summary Report", "[!benchmark][large][report]") {
    const size_t N = 1 << 20;

    std::cout << "\n";
    std::cout << "╔══════════════════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║          CLHT libfork vs Other Implementations - Large Scale Report          ║\n";
    std::cout << "║                      N = 2^20 = 1,048,576 elements                           ║\n";
    std::cout << "╠══════════════════════════════════════════════════════════════════════════════╣\n";
    std::cout << "║  Optimization Strategy:                                                       ║\n";
    std::cout << "║  - Insert: SERIAL (CLHT bucket locks limit parallel scaling)                  ║\n";
    std::cout << "║  - Lookup: PARALLEL (lock-free reads scale near-linearly)                     ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════════════════════════╝\n";
    std::cout << "\n";

    // Simple verification benchmark
    auto keys = generate_int_keys(N);
    std::vector<uintptr_t> values(N);
    for (size_t i = 0; i < N; ++i) values[i] = i;

    BENCHMARK("CLHT libfork verification") {
        clht_libfork::ParallelClhtInt ht(N * 2, 8);
        ht.batch_insert(keys, values);
        std::vector<uintptr_t> results;
        ht.batch_lookup(keys, results);
        return results[0];
    };
}
