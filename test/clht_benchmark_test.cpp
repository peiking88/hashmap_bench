/*
 * CLHT String Implementation Performance Benchmarks
 * 
 * Comprehensive benchmark suite for CLHT string key implementations:
 * - Insert/Query throughput
 * - Load factor impact
 * - Key length impact
 * - Comparison with other hashmap implementations
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <string>
#include <vector>
#include <random>
#include <algorithm>
#include <chrono>
#include <unordered_map>

// Abseil
#include "absl/container/flat_hash_map.h"

// Folly F14 (minimal build)
#include <folly/container/F14Map.h>

// CLHT test wrappers
#include "clht_test_wrapper.hpp"

using namespace clht_test;
using namespace clht_str;

// ============================================================================
// Benchmark Helpers
// ============================================================================

static std::vector<std::string> generate_benchmark_keys(size_t count, size_t key_len) {
    std::vector<std::string> keys;
    keys.reserve(count);
    
    for (size_t i = 0; i < count; ++i) {
        std::string key(key_len, 'a');
        size_t n = i;
        for (size_t j = 0; j < std::min(key_len, size_t(8)); ++j) {
            key[j] = 'a' + (n % 26);
            n /= 26;
        }
        // Make each key unique
        key = "k" + std::to_string(i) + "_" + key;
        if (key.size() > key_len) {
            key.resize(key_len);
        } else {
            key.resize(key_len, 'x');
        }
        keys.push_back(key);
    }
    return keys;
}

// ============================================================================
// Insert Performance Benchmarks
// ============================================================================

TEST_CASE("CLHT String: Insert performance comparison", "[!benchmark][clht][insert]") {
    const size_t N = 10000;
    auto keys = generate_benchmark_keys(N, 16);
    
    BENCHMARK("ClhtStrPtr insert") {
        ClhtStrPtrWrapper::Map map = ClhtStrPtrWrapper::create(N * 2);
        for (size_t i = 0; i < N; ++i) {
            ClhtStrPtrWrapper::insert(map, keys[i], i);
        }
        ClhtStrPtrWrapper::destroy(map);
        return N;
    };
    
    BENCHMARK("ClhtStrInline insert") {
        ClhtStrInlineWrapper::Map map = ClhtStrInlineWrapper::create(N * 2);
        for (size_t i = 0; i < N; ++i) {
            ClhtStrInlineWrapper::insert(map, keys[i], i);
        }
        ClhtStrInlineWrapper::destroy(map);
        return N;
    };
    
    BENCHMARK("ClhtStrPooled insert") {
        ClhtStrPooledWrapper::Map map = ClhtStrPooledWrapper::create(N * 2);
        for (size_t i = 0; i < N; ++i) {
            ClhtStrPooledWrapper::insert(map, keys[i], i);
        }
        ClhtStrPooledWrapper::destroy(map);
        return N;
    };
    
    BENCHMARK("ClhtStrTagged insert") {
        ClhtStrTaggedWrapper::Map map = ClhtStrTaggedWrapper::create(N * 2);
        for (size_t i = 0; i < N; ++i) {
            ClhtStrTaggedWrapper::insert(map, keys[i], i);
        }
        ClhtStrTaggedWrapper::destroy(map);
        return N;
    };
    
    BENCHMARK("ClhtStrFinal insert") {
        ClhtStrFinalWrapper::Map map = ClhtStrFinalWrapper::create(N * 2);
        for (size_t i = 0; i < N; ++i) {
            ClhtStrFinalWrapper::insert(map, keys[i], i);
        }
        ClhtStrFinalWrapper::destroy(map);
        return N;
    };
    
    // Reference implementations
    BENCHMARK("std::unordered_map insert") {
        std::unordered_map<std::string, uint64_t> map;
        map.reserve(N);
        for (size_t i = 0; i < N; ++i) {
            map[keys[i]] = i;
        }
        return map.size();
    };
    
    BENCHMARK("absl::flat_hash_map insert") {
        absl::flat_hash_map<std::string, uint64_t> map;
        map.reserve(N);
        for (size_t i = 0; i < N; ++i) {
            map[keys[i]] = i;
        }
        return map.size();
    };
    
    BENCHMARK("folly::F14FastMap insert") {
        folly::F14FastMap<std::string, uint64_t> map;
        map.reserve(N);
        for (size_t i = 0; i < N; ++i) {
            map[keys[i]] = i;
        }
        return map.size();
    };
}

// ============================================================================
// Lookup Performance Benchmarks
// ============================================================================

TEST_CASE("CLHT String: Lookup performance comparison", "[!benchmark][clht][lookup]") {
    const size_t N = 10000;
    auto keys = generate_benchmark_keys(N, 16);
    uint64_t dummy = 0;
    
    // Pre-populate all maps
    ClhtStrPtrWrapper::Map ptr_map = ClhtStrPtrWrapper::create(N * 2);
    ClhtStrInlineWrapper::Map inline_map = ClhtStrInlineWrapper::create(N * 2);
    ClhtStrPooledWrapper::Map pooled_map = ClhtStrPooledWrapper::create(N * 2);
    ClhtStrTaggedWrapper::Map tagged_map = ClhtStrTaggedWrapper::create(N * 2);
    ClhtStrFinalWrapper::Map final_map = ClhtStrFinalWrapper::create(N * 2);
    std::unordered_map<std::string, uint64_t> std_map;
    absl::flat_hash_map<std::string, uint64_t> absl_map;
    folly::F14FastMap<std::string, uint64_t> folly_map;
    
    for (size_t i = 0; i < N; ++i) {
        ClhtStrPtrWrapper::insert(ptr_map, keys[i], i);
        ClhtStrInlineWrapper::insert(inline_map, keys[i], i);
        ClhtStrPooledWrapper::insert(pooled_map, keys[i], i);
        ClhtStrTaggedWrapper::insert(tagged_map, keys[i], i);
        ClhtStrFinalWrapper::insert(final_map, keys[i], i);
        std_map[keys[i]] = i;
        absl_map[keys[i]] = i;
        folly_map[keys[i]] = i;
    }
    
    BENCHMARK("ClhtStrPtr lookup") {
        uint64_t sum = 0;
        for (size_t i = 0; i < N; ++i) {
            sum += ClhtStrPtrWrapper::lookup(ptr_map, keys[i]);
        }
        return sum;
    };
    
    BENCHMARK("ClhtStrInline lookup") {
        uint64_t sum = 0;
        for (size_t i = 0; i < N; ++i) {
            sum += ClhtStrInlineWrapper::lookup(inline_map, keys[i]);
        }
        return sum;
    };
    
    BENCHMARK("ClhtStrPooled lookup") {
        uint64_t sum = 0;
        for (size_t i = 0; i < N; ++i) {
            sum += ClhtStrPooledWrapper::lookup(pooled_map, keys[i]);
        }
        return sum;
    };
    
    BENCHMARK("ClhtStrTagged lookup") {
        uint64_t sum = 0;
        for (size_t i = 0; i < N; ++i) {
            sum += ClhtStrTaggedWrapper::lookup(tagged_map, keys[i]);
        }
        return sum;
    };
    
    BENCHMARK("ClhtStrFinal lookup") {
        uint64_t sum = 0;
        for (size_t i = 0; i < N; ++i) {
            sum += ClhtStrFinalWrapper::lookup(final_map, keys[i]);
        }
        return sum;
    };
    
    BENCHMARK("std::unordered_map lookup") {
        uint64_t sum = 0;
        for (size_t i = 0; i < N; ++i) {
            sum += std_map.at(keys[i]);
        }
        return sum;
    };
    
    BENCHMARK("absl::flat_hash_map lookup") {
        uint64_t sum = 0;
        for (size_t i = 0; i < N; ++i) {
            sum += absl_map.at(keys[i]);
        }
        return sum;
    };
    
    BENCHMARK("folly::F14FastMap lookup") {
        uint64_t sum = 0;
        for (size_t i = 0; i < N; ++i) {
            sum += folly_map.at(keys[i]);
        }
        return sum;
    };
    
    // Cleanup
    ClhtStrPtrWrapper::destroy(ptr_map);
    ClhtStrInlineWrapper::destroy(inline_map);
    ClhtStrPooledWrapper::destroy(pooled_map);
    ClhtStrTaggedWrapper::destroy(tagged_map);
    ClhtStrFinalWrapper::destroy(final_map);
}

// ============================================================================
// Load Factor Impact Benchmarks
// ============================================================================

TEST_CASE("CLHT String: Load factor impact", "[!benchmark][clht][load]") {
    const size_t N = 10000;
    auto keys = generate_benchmark_keys(N, 16);
    
    // Test different initial capacities
    auto test_load = [&](double load_factor, auto& wrapper) {
        using Wrapper = std::decay_t<decltype(wrapper)>;
        using Map = typename Wrapper::Map;
        
        size_t capacity = static_cast<size_t>(N / load_factor);
        Map map = Wrapper::create(capacity);
        
        auto start = std::chrono::high_resolution_clock::now();
        for (size_t i = 0; i < N; ++i) {
            Wrapper::insert(map, keys[i], i);
        }
        auto insert_time = std::chrono::high_resolution_clock::now() - start;
        
        start = std::chrono::high_resolution_clock::now();
        uint64_t sum = 0;
        for (size_t i = 0; i < N; ++i) {
            sum += Wrapper::lookup(map, keys[i]);
        }
        auto lookup_time = std::chrono::high_resolution_clock::now() - start;
        
        Wrapper::destroy(map);
        return std::make_pair(insert_time, lookup_time);
    };
    
    SECTION("Low load factor (0.25)") {
        // capacity = 4 * N
        BENCHMARK("ClhtStrFinal at 25% load") {
            ClhtStrFinalWrapper wrapper;
            auto [ins, look] = test_load(0.25, wrapper);
            return ins.count() + look.count();
        };
    }
    
    SECTION("Medium load factor (0.50)") {
        // capacity = 2 * N
        BENCHMARK("ClhtStrFinal at 50% load") {
            ClhtStrFinalWrapper wrapper;
            auto [ins, look] = test_load(0.50, wrapper);
            return ins.count() + look.count();
        };
    }
    
    SECTION("High load factor (0.75)") {
        // capacity = 1.33 * N
        BENCHMARK("ClhtStrFinal at 75% load") {
            ClhtStrFinalWrapper wrapper;
            auto [ins, look] = test_load(0.75, wrapper);
            return ins.count() + look.count();
        };
    }
    
    SECTION("Very high load factor (0.90)") {
        // capacity = 1.11 * N
        BENCHMARK("ClhtStrFinal at 90% load") {
            ClhtStrFinalWrapper wrapper;
            auto [ins, look] = test_load(0.90, wrapper);
            return ins.count() + look.count();
        };
    }
}

// ============================================================================
// Key Length Impact Benchmarks
// ============================================================================

TEST_CASE("CLHT String: Key length impact", "[!benchmark][clht][keylen]") {
    const size_t N = 5000;
    
    auto test_keylen = [&](size_t key_len, auto& wrapper) {
        using Wrapper = std::decay_t<decltype(wrapper)>;
        using Map = typename Wrapper::Map;
        
        auto keys = generate_benchmark_keys(N, key_len);
        Map map = Wrapper::create(N * 2);
        
        auto start = std::chrono::high_resolution_clock::now();
        for (size_t i = 0; i < N; ++i) {
            Wrapper::insert(map, keys[i], i);
        }
        auto insert_time = std::chrono::high_resolution_clock::now() - start;
        
        start = std::chrono::high_resolution_clock::now();
        uint64_t sum = 0;
        for (size_t i = 0; i < N; ++i) {
            sum += Wrapper::lookup(map, keys[i]);
        }
        auto lookup_time = std::chrono::high_resolution_clock::now() - start;
        
        Wrapper::destroy(map);
        return std::make_pair(insert_time, lookup_time);
    };
    
    SECTION("Short keys (8 bytes)") {
        BENCHMARK("ClhtStrFinal with 8-byte keys") {
            ClhtStrFinalWrapper wrapper;
            auto [ins, look] = test_keylen(8, wrapper);
            return ins.count() + look.count();
        };
    }
    
    SECTION("Medium keys (32 bytes)") {
        BENCHMARK("ClhtStrFinal with 32-byte keys") {
            ClhtStrFinalWrapper wrapper;
            auto [ins, look] = test_keylen(32, wrapper);
            return ins.count() + look.count();
        };
    }
    
    SECTION("Long keys (128 bytes)") {
        BENCHMARK("ClhtStrFinal with 128-byte keys") {
            ClhtStrFinalWrapper wrapper;
            auto [ins, look] = test_keylen(128, wrapper);
            return ins.count() + look.count();
        };
    }
    
    SECTION("Very long keys (512 bytes)") {
        BENCHMARK("ClhtStrFinal with 512-byte keys") {
            ClhtStrFinalWrapper wrapper;
            auto [ins, look] = test_keylen(512, wrapper);
            return ins.count() + look.count();
        };
    }
}

// ============================================================================
// Mixed Operations Benchmark
// ============================================================================

TEST_CASE("CLHT String: Mixed operations", "[!benchmark][clht][mixed]") {
    const size_t N = 10000;
    auto keys = generate_benchmark_keys(N * 2, 16);  // Extra keys for misses
    
    BENCHMARK("ClhtStrFinal mixed (80% lookup, 20% insert)") {
        ClhtStrFinalWrapper::Map map = ClhtStrFinalWrapper::create(N * 2);
        
        // Initial fill
        for (size_t i = 0; i < N / 2; ++i) {
            ClhtStrFinalWrapper::insert(map, keys[i], i);
        }
        
        uint64_t sum = 0;
        size_t insert_idx = N / 2;
        
        for (size_t i = 0; i < N * 4; ++i) {
            if (i % 5 == 0 && insert_idx < N) {
                // Insert
                ClhtStrFinalWrapper::insert(map, keys[insert_idx], insert_idx);
                ++insert_idx;
            } else {
                // Lookup
                sum += ClhtStrFinalWrapper::lookup(map, keys[i % (N / 2)]);
            }
        }
        
        ClhtStrFinalWrapper::destroy(map);
        return sum;
    };
}

// ============================================================================
// Scale Benchmarks
// ============================================================================

TEST_CASE("CLHT String: Scale test", "[!benchmark][clht][scale]") {
    auto test_scale = [&](size_t n) {
        auto keys = generate_benchmark_keys(n, 16);
        
        auto start = std::chrono::high_resolution_clock::now();
        ClhtStrFinalWrapper::Map map = ClhtStrFinalWrapper::create(n * 2);
        for (size_t i = 0; i < n; ++i) {
            ClhtStrFinalWrapper::insert(map, keys[i], i);
        }
        auto insert_time = std::chrono::high_resolution_clock::now() - start;
        
        start = std::chrono::high_resolution_clock::now();
        uint64_t sum = 0;
        for (size_t i = 0; i < n; ++i) {
            sum += ClhtStrFinalWrapper::lookup(map, keys[i]);
        }
        auto lookup_time = std::chrono::high_resolution_clock::now() - start;
        
        ClhtStrFinalWrapper::destroy(map);
        return std::make_pair(insert_time.count(), lookup_time.count());
    };
    
    SECTION("Small scale (1K)") {
        BENCHMARK("ClhtStrFinal 1K elements") {
            return test_scale(1000);
        };
    }
    
    SECTION("Medium scale (10K)") {
        BENCHMARK("ClhtStrFinal 10K elements") {
            return test_scale(10000);
        };
    }
    
    SECTION("Large scale (100K)") {
        BENCHMARK("ClhtStrFinal 100K elements") {
            return test_scale(100000);
        };
    }
}

// ============================================================================
// Comparison: CLHT vs Other High-Performance Maps
// ============================================================================

TEST_CASE("CLHT String: Final vs F14FastMap detailed comparison", "[!benchmark][clht][compare]") {
    const size_t N = 50000;
    auto keys = generate_benchmark_keys(N, 16);
    
    // Pre-generate lookup order (random)
    std::vector<size_t> lookup_order(N);
    std::iota(lookup_order.begin(), lookup_order.end(), 0);
    std::mt19937 rng(12345);
    std::shuffle(lookup_order.begin(), lookup_order.end(), rng);
    
    SECTION("Insert throughput") {
        BENCHMARK("ClhtStrFinal insert 50K") {
            ClhtStrFinalWrapper::Map map = ClhtStrFinalWrapper::create(N * 2);
            for (size_t i = 0; i < N; ++i) {
                ClhtStrFinalWrapper::insert(map, keys[i], i);
            }
            ClhtStrFinalWrapper::destroy(map);
            return N;
        };
        
        BENCHMARK("F14FastMap insert 50K") {
            folly::F14FastMap<std::string, uint64_t> map;
            map.reserve(N);
            for (size_t i = 0; i < N; ++i) {
                map[keys[i]] = i;
            }
            return map.size();
        };
    }
    
    SECTION("Lookup throughput (sequential)") {
        ClhtStrFinalWrapper::Map final_map = ClhtStrFinalWrapper::create(N * 2);
        folly::F14FastMap<std::string, uint64_t> f14_map;
        f14_map.reserve(N);
        
        for (size_t i = 0; i < N; ++i) {
            ClhtStrFinalWrapper::insert(final_map, keys[i], i);
            f14_map[keys[i]] = i;
        }
        
        BENCHMARK("ClhtStrFinal sequential lookup") {
            uint64_t sum = 0;
            for (size_t i = 0; i < N; ++i) {
                sum += ClhtStrFinalWrapper::lookup(final_map, keys[i]);
            }
            return sum;
        };
        
        BENCHMARK("F14FastMap sequential lookup") {
            uint64_t sum = 0;
            for (size_t i = 0; i < N; ++i) {
                sum += f14_map.at(keys[i]);
            }
            return sum;
        };
        
        ClhtStrFinalWrapper::destroy(final_map);
    }
    
    SECTION("Lookup throughput (random order)") {
        ClhtStrFinalWrapper::Map final_map = ClhtStrFinalWrapper::create(N * 2);
        folly::F14FastMap<std::string, uint64_t> f14_map;
        f14_map.reserve(N);
        
        for (size_t i = 0; i < N; ++i) {
            ClhtStrFinalWrapper::insert(final_map, keys[i], i);
            f14_map[keys[i]] = i;
        }
        
        BENCHMARK("ClhtStrFinal random lookup") {
            uint64_t sum = 0;
            for (size_t idx : lookup_order) {
                sum += ClhtStrFinalWrapper::lookup(final_map, keys[idx]);
            }
            return sum;
        };
        
        BENCHMARK("F14FastMap random lookup") {
            uint64_t sum = 0;
            for (size_t idx : lookup_order) {
                sum += f14_map.at(keys[idx]);
            }
            return sum;
        };
        
        ClhtStrFinalWrapper::destroy(final_map);
    }
}

// ============================================================================
// Throughput Calculation Benchmarks
// ============================================================================

TEST_CASE("CLHT String: Throughput calculation", "[!benchmark][clht][throughput]") {
    const size_t N = 100000;
    auto keys = generate_benchmark_keys(N, 16);
    
    BENCHMARK_ADVANCED("ClhtStrFinal insert throughput (ops/sec)") {
        ClhtStrFinalWrapper::Map map = ClhtStrFinalWrapper::create(N * 2);
        
        auto start = std::chrono::high_resolution_clock::now();
        for (size_t i = 0; i < N; ++i) {
            ClhtStrFinalWrapper::insert(map, keys[i], i);
        }
        auto end = std::chrono::high_resolution_clock::now();
        
        auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
        double ops_per_sec = static_cast<double>(N) * 1e9 / ns;
        
        ClhtStrFinalWrapper::destroy(map);
        
        return ops_per_sec;
    };
    
    // Pre-populate for lookup test
    ClhtStrFinalWrapper::Map lookup_map = ClhtStrFinalWrapper::create(N * 2);
    for (size_t i = 0; i < N; ++i) {
        ClhtStrFinalWrapper::insert(lookup_map, keys[i], i);
    }
    
    BENCHMARK_ADVANCED("ClhtStrFinal lookup throughput (ops/sec)") {
        auto start = std::chrono::high_resolution_clock::now();
        uint64_t sum = 0;
        for (size_t i = 0; i < N; ++i) {
            sum += ClhtStrFinalWrapper::lookup(lookup_map, keys[i]);
        }
        auto end = std::chrono::high_resolution_clock::now();
        
        auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
        double ops_per_sec = static_cast<double>(N) * 1e9 / ns;
        
        return ops_per_sec;
    };
    
    ClhtStrFinalWrapper::destroy(lookup_map);
}
