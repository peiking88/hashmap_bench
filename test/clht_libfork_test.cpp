/*
 * CLHT libfork Parallel Implementation Unit Tests
 * 
 * Tests for parallel batch operations using libfork work-stealing scheduler
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>
#include <vector>
#include <string>
#include <random>
#include <algorithm>
#include <thread>

// CLHT libfork implementations
#include "clht_libfork_str.hpp"
#include "clht_libfork_int.hpp"

// ============================================================================
// Test Helpers
// ============================================================================

static std::vector<std::string> generate_string_keys(size_t count, size_t key_len = 16) {
    std::vector<std::string> keys;
    keys.reserve(count);
    
    for (size_t i = 0; i < count; ++i) {
        std::string key = "key_" + std::to_string(i) + "_";
        key.resize(key_len, 'x');
        keys.push_back(key);
    }
    return keys;
}

static std::vector<uintptr_t> generate_int_keys(size_t count) {
    std::vector<uintptr_t> keys;
    keys.reserve(count);
    
    for (size_t i = 0; i < count; ++i) {
        keys.push_back(i * 2 + 1);  // Odd numbers to avoid 0
    }
    return keys;
}

// ============================================================================
// CLHT String Key Parallel Tests
// ============================================================================

TEST_CASE("CLHT String Parallel: Basic operations", "[clht_libfork][str]") {
    clht_libfork::ParallelClhtStr ht(1024);
    
    SECTION("Single insert and lookup") {
        REQUIRE(ht.insert("test_key", 42));
        REQUIRE(ht.lookup("test_key") == 42);
    }
    
    SECTION("Single remove") {
        REQUIRE(ht.insert("remove_key", 100));
        REQUIRE(ht.lookup("remove_key") == 100);
        REQUIRE(ht.remove("remove_key"));
        REQUIRE(ht.lookup("remove_key") == UINTPTR_MAX);
    }
    
    SECTION("Size tracking") {
        REQUIRE(ht.size() == 0);
        ht.insert("key1", 1);
        REQUIRE(ht.size() == 1);
        ht.insert("key2", 2);
        REQUIRE(ht.size() == 2);
    }
}

TEST_CASE("CLHT String Parallel: Batch insert", "[clht_libfork][str][batch]") {
    clht_libfork::ParallelClhtStr ht(16384);
    
    SECTION("Batch insert 1000 keys") {
        const size_t N = 1000;
        auto keys = generate_string_keys(N);
        std::vector<uintptr_t> values(N);
        for (size_t i = 0; i < N; ++i) {
            values[i] = i;
        }
        
        ht.batch_insert(keys, values);
        
        // Verify all keys exist
        for (size_t i = 0; i < N; ++i) {
            REQUIRE(ht.lookup(keys[i]) == i);
        }
        REQUIRE(ht.size() == N);
    }
    
    SECTION("Batch insert with duplicates") {
        auto keys = generate_string_keys(100);
        std::vector<uintptr_t> values(100, 42);
        
        ht.batch_insert(keys, values);
        ht.batch_insert(keys, values);  // Insert again
        
        REQUIRE(ht.size() == 100);  // Size should not double
    }
}

TEST_CASE("CLHT String Parallel: Batch lookup", "[clht_libfork][str][batch]") {
    clht_libfork::ParallelClhtStr ht(16384);
    
    const size_t N = 500;
    auto keys = generate_string_keys(N);
    std::vector<uintptr_t> values(N);
    for (size_t i = 0; i < N; ++i) {
        values[i] = i * 10;
    }
    
    ht.batch_insert(keys, values);
    
    SECTION("Batch lookup all keys") {
        std::vector<uintptr_t> results;
        ht.batch_lookup(keys, results);
        
        REQUIRE(results.size() == N);
        for (size_t i = 0; i < N; ++i) {
            REQUIRE(results[i] == i * 10);
        }
    }
    
    SECTION("Batch lookup with missing keys") {
        auto lookup_keys = keys;
        lookup_keys.push_back("nonexistent_key_1");
        lookup_keys.push_back("nonexistent_key_2");
        
        std::vector<uintptr_t> results;
        ht.batch_lookup(lookup_keys, results);
        
        REQUIRE(results.size() == N + 2);
        for (size_t i = 0; i < N; ++i) {
            REQUIRE(results[i] == i * 10);
        }
        REQUIRE(results[N] == UINTPTR_MAX);
        REQUIRE(results[N + 1] == UINTPTR_MAX);
    }
}

TEST_CASE("CLHT String Parallel: Batch remove", "[clht_libfork][str][batch]") {
    clht_libfork::ParallelClhtStr ht(8192);
    
    const size_t N = 200;
    auto keys = generate_string_keys(N);
    std::vector<uintptr_t> values(N, 123);
    
    ht.batch_insert(keys, values);
    REQUIRE(ht.size() == N);
    
    SECTION("Batch remove half keys") {
        std::vector<std::string> remove_keys(keys.begin(), keys.begin() + N / 2);
        std::vector<bool> results;
        ht.batch_remove(remove_keys, results);
        
        REQUIRE(results.size() == N / 2);
        for (size_t i = 0; i < N / 2; ++i) {
            REQUIRE(results[i] == true);
        }
        REQUIRE(ht.size() == N / 2);
    }
}

TEST_CASE("CLHT String Parallel: Mixed workload", "[clht_libfork][str][mixed]") {
    clht_libfork::ParallelClhtStr ht(16384);
    
    const size_t N = 1000;
    auto keys = generate_string_keys(N);
    std::vector<uintptr_t> values(N);
    for (size_t i = 0; i < N; ++i) {
        values[i] = i;
    }
    
    SECTION("80% lookup, 20% insert") {
        std::vector<uintptr_t> results;
        ht.batch_mixed(keys, values, results, 0.2);
        
        size_t insert_count = static_cast<size_t>(N * 0.2);
        REQUIRE(ht.size() == insert_count);
    }
}

// ============================================================================
// CLHT Integer Key Parallel Tests
// ============================================================================

TEST_CASE("CLHT Integer Parallel: Basic operations", "[clht_libfork][int]") {
    clht_libfork::ParallelClhtInt ht(1024);
    
    SECTION("Single insert and lookup") {
        REQUIRE(ht.insert(42, 100));
        REQUIRE(ht.lookup(42) == 100);
    }
    
    SECTION("Single remove") {
        REQUIRE(ht.insert(100, 200));
        REQUIRE(ht.lookup(100) == 200);
        REQUIRE(ht.remove(100) == 200);
        REQUIRE(ht.lookup(100) == 0);
    }
    
    SECTION("Size tracking") {
        REQUIRE(ht.size() == 0);
        ht.insert(1, 10);
        REQUIRE(ht.size() == 1);
        ht.insert(2, 20);
        REQUIRE(ht.size() == 2);
    }
}

TEST_CASE("CLHT Integer Parallel: Batch insert", "[clht_libfork][int][batch]") {
    clht_libfork::ParallelClhtInt ht(16384);
    
    SECTION("Batch insert 1000 keys") {
        const size_t N = 1000;
        auto keys = generate_int_keys(N);
        std::vector<uintptr_t> values(N);
        for (size_t i = 0; i < N; ++i) {
            values[i] = i;
        }
        
        ht.batch_insert(keys, values);
        
        // Verify all keys exist
        for (size_t i = 0; i < N; ++i) {
            REQUIRE(ht.lookup(keys[i]) == i);
        }
    }
}

TEST_CASE("CLHT Integer Parallel: Batch lookup", "[clht_libfork][int][batch]") {
    clht_libfork::ParallelClhtInt ht(16384);
    
    const size_t N = 500;
    auto keys = generate_int_keys(N);
    std::vector<uintptr_t> values(N);
    for (size_t i = 0; i < N; ++i) {
        values[i] = i * 10;
    }
    
    ht.batch_insert(keys, values);
    
    SECTION("Batch lookup all keys") {
        std::vector<uintptr_t> results;
        ht.batch_lookup(keys, results);
        
        REQUIRE(results.size() == N);
        for (size_t i = 0; i < N; ++i) {
            REQUIRE(results[i] == i * 10);
        }
    }
}

TEST_CASE("CLHT Integer Parallel: Batch remove", "[clht_libfork][int][batch]") {
    clht_libfork::ParallelClhtInt ht(8192);
    
    const size_t N = 200;
    auto keys = generate_int_keys(N);
    std::vector<uintptr_t> values(N, 123);
    
    ht.batch_insert(keys, values);
    
    SECTION("Batch remove half keys") {
        std::vector<uintptr_t> remove_keys(keys.begin(), keys.begin() + N / 2);
        std::vector<uintptr_t> results;
        ht.batch_remove(remove_keys, results);
        
        REQUIRE(results.size() == N / 2);
    }
}

TEST_CASE("CLHT Integer Parallel: Mixed workload", "[clht_libfork][int][mixed]") {
    clht_libfork::ParallelClhtInt ht(16384);
    
    const size_t N = 1000;
    auto keys = generate_int_keys(N);
    std::vector<uintptr_t> values(N);
    for (size_t i = 0; i < N; ++i) {
        values[i] = i;
    }
    
    SECTION("80% lookup, 20% insert") {
        std::vector<uintptr_t> results;
        ht.batch_mixed(keys, values, results, 0.2);
        
        size_t insert_count = static_cast<size_t>(N * 0.2);
        REQUIRE(ht.size() >= insert_count);
    }
}

// ============================================================================
// Performance Tests
// ============================================================================

TEST_CASE("CLHT String Parallel: Performance scaling", "[!benchmark][clht_libfork][str][perf]") {
    const size_t N = 10000;
    auto keys = generate_string_keys(N, 16);
    std::vector<uintptr_t> values(N);
    for (size_t i = 0; i < N; ++i) {
        values[i] = i;
    }
    
    BENCHMARK("Serial insert baseline") {
        clht_libfork::ParallelClhtStr ht(N * 2, 1);
        for (size_t i = 0; i < N; ++i) {
            ht.insert(keys[i], values[i]);
        }
        return ht.size();
    };
    
    BENCHMARK("Parallel insert (4 threads)") {
        clht_libfork::ParallelClhtStr ht(N * 2, 4);
        ht.batch_insert(keys, values);
        return ht.size();
    };
    
    BENCHMARK("Parallel insert (all cores)") {
        clht_libfork::ParallelClhtStr ht(N * 2);
        ht.batch_insert(keys, values);
        return ht.size();
    };
}

TEST_CASE("CLHT Integer Parallel: Performance scaling", "[!benchmark][clht_libfork][int][perf]") {
    const size_t N = 10000;
    auto keys = generate_int_keys(N);
    std::vector<uintptr_t> values(N);
    for (size_t i = 0; i < N; ++i) {
        values[i] = i;
    }
    
    BENCHMARK("Serial insert baseline") {
        clht_libfork::ParallelClhtInt ht(N * 2, 1);
        for (size_t i = 0; i < N; ++i) {
            ht.insert(keys[i], values[i]);
        }
        return ht.size();
    };
    
    BENCHMARK("Parallel insert (4 threads)") {
        clht_libfork::ParallelClhtInt ht(N * 2, 4);
        ht.batch_insert(keys, values);
        return ht.size();
    };
    
    BENCHMARK("Parallel insert (all cores)") {
        clht_libfork::ParallelClhtInt ht(N * 2);
        ht.batch_insert(keys, values);
        return ht.size();
    };
}

// ============================================================================
// Comparison Tests: Parallel vs Serial
// ============================================================================

TEST_CASE("CLHT String Parallel vs Serial: Lookup comparison", "[!benchmark][clht_libfork][str][compare]") {
    const size_t N = 10000;
    auto keys = generate_string_keys(N, 16);
    std::vector<uintptr_t> values(N);
    for (size_t i = 0; i < N; ++i) {
        values[i] = i;
    }
    
    // Pre-populate
    clht_libfork::ParallelClhtStr ht(N * 2);
    ht.batch_insert(keys, values);
    
    BENCHMARK("Serial lookup") {
        std::vector<uintptr_t> results(N);
        for (size_t i = 0; i < N; ++i) {
            results[i] = ht.lookup(keys[i]);
        }
        return results[0];
    };
    
    BENCHMARK("Parallel lookup") {
        std::vector<uintptr_t> results;
        ht.batch_lookup(keys, results);
        return results[0];
    };
}

TEST_CASE("CLHT Integer Parallel vs Serial: Lookup comparison", "[!benchmark][clht_libfork][int][compare]") {
    const size_t N = 10000;
    auto keys = generate_int_keys(N);
    std::vector<uintptr_t> values(N);
    for (size_t i = 0; i < N; ++i) {
        values[i] = i;
    }
    
    // Pre-populate
    clht_libfork::ParallelClhtInt ht(N * 2);
    ht.batch_insert(keys, values);
    
    BENCHMARK("Serial lookup") {
        std::vector<uintptr_t> results(N);
        for (size_t i = 0; i < N; ++i) {
            results[i] = ht.lookup(keys[i]);
        }
        return results[0];
    };
    
    BENCHMARK("Parallel lookup") {
        std::vector<uintptr_t> results;
        ht.batch_lookup(keys, results);
        return results[0];
    };
}
