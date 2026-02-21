/*
 * hashmap_bench - A comprehensive hashmap benchmark suite
 * 
 * Based on hash_bench (https://github.com/felix-chern/hash_bench)
 * Modified to use:
 * - nanolog for logging (instead of log4c)
 * - catch2 for testing (instead of googletest)
 * - Additional hashmap implementations:
 *   - absl::flat_hash_map, absl::node_hash_map
 *   - folly::F14FastMap, folly::ConcurrentHashMap
 *   - cista::hash_map
 *   - rhashmap
 *   - boost::container::flat_map
 */

#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <getopt.h>
#include <unistd.h>
#include <type_traits>
#include <utility>

// NanoLog for logging - disabled due to linker issues
// #include "NanoLogCpp17.h"

// Benchmark framework
#include "benchmark.hpp"
#include "hash_maps.hpp"

// Logging disabled for cleaner output
#define LOG_DEBUG(fmt, ...) ((void)0)
#define LOG_INFO(fmt, ...) ((void)0)

using namespace hashmap_bench;

template <typename T, typename = void>
struct has_batch_insert : std::false_type {};

template <typename T>
struct has_batch_insert<T, std::void_t<decltype(T::batch_insert(
    std::declval<typename T::Map&>(),
    std::declval<const std::vector<std::string>&>(),
    std::declval<const std::vector<uint64_t>&>()))>> : std::true_type {};

// ============================================================================
// String key benchmarks
// ============================================================================

template <typename Wrapper>
BenchmarkResult benchmark_string_keys(
    const std::string& impl_name,
    const std::string& key_type,
    const std::vector<std::string>& keys,
    const std::string& comments = "") {
    
    using Map = typename Wrapper::Map;
    
    LOG_INFO("Benchmarking %s with %s keys (%zu elements)...", 
             impl_name.c_str(), key_type.c_str(), keys.size());
    
    BenchmarkResult result;
    result.impl_name = impl_name;
    result.key_type = key_type;
    result.num_elements = keys.size();
    result.comments = comments;
    
    // Create map
    LOG_DEBUG("Creating map...");
    Map map = Wrapper::create(keys.size());
    
    // Insert benchmark
    LOG_DEBUG("Starting insert benchmark...");
    Timer timer;
    if constexpr (has_batch_insert<Wrapper>::value) {
        std::vector<uint64_t> values(keys.size(), 0);
        Wrapper::batch_insert(map, keys, values);
    } else {
        for (const auto& key : keys) {
            Wrapper::insert(map, key, uint64_t{0});
        }
    }
    result.insert_time_sec = timer.elapsed();
    
    LOG_INFO("Insert completed in %.6f seconds (%.2f Mops/sec)", 
             result.insert_time_sec, 
             keys.size() / result.insert_time_sec / 1000000.0);
    
    // Query benchmark
    LOG_DEBUG("Starting query benchmark...");
    timer.reset();
    for (const auto& key : keys) {
        side_effect += Wrapper::lookup(map, key);
    }
    result.query_time_sec = timer.elapsed();
    
    LOG_INFO("Query completed in %.6f seconds (%.2f Mops/sec)", 
             result.query_time_sec, 
             keys.size() / result.query_time_sec / 1000000.0);
    
    // Cleanup
    LOG_DEBUG("Destroying map...");
    Wrapper::destroy(map);
    
    return result;
}

// ============================================================================
// Integer key benchmarks
// ============================================================================

template <typename Wrapper>
BenchmarkResult benchmark_int_keys(
    const std::string& impl_name,
    const std::vector<uint64_t>& keys,
    const std::string& comments = "") {
    
    using Map = typename Wrapper::Map;
    
    LOG_INFO("Benchmarking %s with int keys (%zu elements)...", 
             impl_name.c_str(), keys.size());
    
    BenchmarkResult result;
    result.impl_name = impl_name;
    result.key_type = "int64";
    result.num_elements = keys.size();
    result.comments = comments;
    
    // Create map
    LOG_DEBUG("Creating map...");
    Map map = Wrapper::create(keys.size());
    
    // Insert benchmark
    LOG_DEBUG("Starting insert benchmark...");
    Timer timer;
    if constexpr (has_batch_insert<Wrapper>::value) {
        std::vector<uint64_t> values(keys.size(), 0);
        Wrapper::batch_insert(map, keys, values);
    } else {
        for (const auto& key : keys) {
            Wrapper::insert(map, key, uint64_t{0});
        }
    }
    result.insert_time_sec = timer.elapsed();
    
    LOG_INFO("Insert completed in %.6f seconds (%.2f Mops/sec)", 
             result.insert_time_sec, 
             keys.size() / result.insert_time_sec / 1000000.0);
    
    // Query benchmark
    LOG_DEBUG("Starting query benchmark...");
    timer.reset();
    for (const auto& key : keys) {
        side_effect += Wrapper::lookup(map, key);
    }
    result.query_time_sec = timer.elapsed();
    
    LOG_INFO("Query completed in %.6f seconds (%.2f Mops/sec)", 
             result.query_time_sec, 
             keys.size() / result.query_time_sec / 1000000.0);
    
    // Cleanup
    LOG_DEBUG("Destroying map...");
    Wrapper::destroy(map);
    
    return result;
}

// ============================================================================
// All benchmarks runner
// ============================================================================

std::vector<BenchmarkResult> run_all_string_benchmarks(
    const std::string& key_type, int num_power, bool run_all_impls) {
    
    std::vector<BenchmarkResult> results;
    
    // Generate keys
    std::vector<std::string> keys;
    if (key_type == "short_string") {
        generate_short_keys(keys, num_power);
    } else if (key_type == "mid_string") {
        generate_mid_keys(keys, num_power);
    } else if (key_type == "long_string") {
        generate_long_keys(keys, num_power);
    } else {
        LOG_INFO( "Unknown key type: %s", key_type.c_str());
        return results;
    }
    
    LOG_DEBUG( "Generated %zu keys of type %s", keys.size(), key_type.c_str());
    
    // Benchmark each implementation
    std::cout << "\n=== String Key Benchmarks (Key Type: " << key_type << ") ===\n";
    
    // std::unordered_map
    results.push_back(benchmark_string_keys<StdUnorderedMapWrapper<std::string, uint64_t>>(
        "std::unordered_map", key_type, keys, "KV: string/uintptr_t"));
    
    // absl::flat_hash_map
    results.push_back(benchmark_string_keys<AbslFlatHashMapWrapper<std::string, uint64_t>>(
        "absl::flat_hash_map", key_type, keys, "KV: string/uintptr_t"));
    
    // absl::node_hash_map
    results.push_back(benchmark_string_keys<AbslNodeHashMapWrapper<std::string, uint64_t>>(
        "absl::node_hash_map", key_type, keys, "KV: string/uintptr_t"));

    // folly::F14FastMap
    results.push_back(benchmark_string_keys<FollyF14FastMapWrapper<std::string, uint64_t>>(
        "folly::F14FastMap", key_type, keys, "KV: string/uintptr_t"));
    
    // google::dense_hash_map
    results.push_back(benchmark_string_keys<DenseHashMapWrapper<std::string, uint64_t>>(
        "google::dense_hash_map", key_type, keys, "KV: string/uintptr_t"));
    
    // google::sparse_hash_map
    results.push_back(benchmark_string_keys<SparseHashMapWrapper<std::string, uint64_t>>(
        "google::sparse_hash_map", key_type, keys, "KV: string/uintptr_t"));

    if (run_all_impls) {
        // cista::hash_map
        results.push_back(benchmark_string_keys<CistaHashMapWrapper<std::string, uint64_t>>(
            "cista::hash_map", key_type, keys, "KV: string/uintptr_t"));
        
        // boost::container::flat_map
        results.push_back(benchmark_string_keys<BoostFlatMapWrapper<std::string, uint64_t>>(
            "boost::flat_map", key_type, keys, "KV: string/uintptr_t"));
        
        // libcuckoo::cuckoohash_map
        results.push_back(benchmark_string_keys<CuckooHashMapWrapper<std::string, uint64_t>>(
            "libcuckoo::cuckoohash_map", key_type, keys, "KV: string/uintptr_t"));
        
        // rhashmap (C library)
        results.push_back(benchmark_string_keys<RhashmapWrapper>(
            "rhashmap", key_type, keys, "KV: string/uintptr_t"));
        
        // phmap::flat_hash_map
        results.push_back(benchmark_string_keys<PhmapFlatHashMapWrapper<std::string, uint64_t>>(
            "phmap::flat_hash_map", key_type, keys, "KV: string/uintptr_t"));
        
        // phmap::parallel_flat_hash_map
        results.push_back(benchmark_string_keys<PhmapParallelHashMapWrapper<std::string, uint64_t>>(
            "phmap::parallel_flat_hash_map", key_type, keys, "KV: string/uintptr_t"));
    }
    
    print_results(results);
    return results;
}

std::vector<BenchmarkResult> run_all_int_benchmarks(int num_power, bool run_all_impls) {
    std::vector<BenchmarkResult> results;
    
    // Generate keys
    std::vector<uint64_t> keys;
    generate_int_keys(keys, num_power);
    
    LOG_DEBUG( "Generated %zu int keys", keys.size());
    
    std::cout << "\n=== Integer Key Benchmarks ===\n";
    
    // std::unordered_map
    results.push_back(benchmark_int_keys<StdUnorderedMapWrapper<uint64_t, uint64_t>>(
        "std::unordered_map", keys, "KV: int64/uintptr_t"));
    
    // absl::flat_hash_map
    results.push_back(benchmark_int_keys<AbslFlatHashMapWrapper<uint64_t, uint64_t>>(
        "absl::flat_hash_map", keys, "KV: int64/uintptr_t"));
    
    // absl::node_hash_map
    results.push_back(benchmark_int_keys<AbslNodeHashMapWrapper<uint64_t, uint64_t>>(
        "absl::node_hash_map", keys, "KV: int64/uintptr_t"));

    // folly::F14FastMap
    results.push_back(benchmark_int_keys<FollyF14FastMapWrapper<uint64_t, uint64_t>>(
        "folly::F14FastMap", keys, "KV: int64/uintptr_t"));
    
    if (run_all_impls) {
        // cista::hash_map
        results.push_back(benchmark_int_keys<CistaHashMapWrapper<uint64_t, uint64_t>>(
            "cista::hash_map", keys, "KV: int64/uintptr_t"));
        
        // boost::container::flat_map
        results.push_back(benchmark_int_keys<BoostFlatMapWrapper<uint64_t, uint64_t>>(
            "boost::flat_map", keys, "KV: int64/uintptr_t"));
    }
    
    // google::dense_hash_map
    results.push_back(benchmark_int_keys<DenseHashMapWrapper<uint64_t, uint64_t>>(
        "google::dense_hash_map", keys, "KV: int64/uintptr_t"));
    
    // google::sparse_hash_map
    results.push_back(benchmark_int_keys<SparseHashMapWrapper<uint64_t, uint64_t>>(
        "google::sparse_hash_map", keys, "KV: int64/uintptr_t"));
    
    // CLHT (Lock-Based and Lock-Free hash tables)
    results.push_back(benchmark_int_keys<ClhtLbWrapper>("CLHT-LB", keys, "✅ Lock-Based, KV: int64/uintptr_t"));
    results.push_back(benchmark_int_keys<ClhtLfWrapper>("CLHT-LF", keys, "✅ Lock-Free, KV: int64/uintptr_t"));

    if (run_all_impls) {
        // libcuckoo::cuckoohash_map
        results.push_back(benchmark_int_keys<CuckooHashMapWrapper<uint64_t, uint64_t>>(
            "libcuckoo::cuckoohash_map", keys, "KV: int64/uintptr_t"));
        
        // OPIC Robin Hood Hash
        results.push_back(benchmark_int_keys<OpicRobinHoodWrapper>(
            "OPIC::robin_hood", keys, "KV: int64/uintptr_t"));
        
        // phmap::flat_hash_map
        results.push_back(benchmark_int_keys<PhmapFlatHashMapWrapper<uint64_t, uint64_t>>(
            "phmap::flat_hash_map", keys, "KV: int64/uintptr_t"));
        
        // phmap::parallel_flat_hash_map
        results.push_back(benchmark_int_keys<PhmapParallelHashMapWrapper<uint64_t, uint64_t>>(
            "phmap::parallel_flat_hash_map", keys, "KV: int64/uintptr_t"));
    }
    
    print_results(results);
    return results;
}

// ============================================================================
// Main function
// ============================================================================

void print_help(const char* program) {
    std::cout << 
        "Usage: " << program << " [OPTIONS]\n"
        "\n"
        "Options:\n"
        "  -n POWER      Number of elements as power of 2 (default: 20, i.e., 2^20 = 1M)\n"
        "  -k KEYTYPE    Key type: short_string, mid_string, long_string, int (default: short_string)\n"
        "  -a            Run all key types and all implementations\n"
        "  -i IMPL       Run only specified implementation\n"
        "  -r REPEAT     Number of repetitions (default: 1)\n"
        "  -p PAUSE      Pause seconds between insert and query (default: 0)\n"
        "  -c FACTOR     CLHT capacity factor (default: 4)\n"
        "  -h            Show this help\n"
        "\n"
        "Implementations:\n"
        "  std_unordered_map      - std::unordered_map\n"
        "  absl_flat_hash_map     - absl::flat_hash_map\n"
        "  absl_node_hash_map     - absl::node_hash_map\n"
        "  folly_F14FastMap       - folly::F14FastMap\n"
        "  cista_hash_map         - cista::hash_map\n"
        "  boost_flat_map         - boost::container::flat_map\n"
        "  dense_hash_map         - google::dense_hash_map\n"
        "  sparse_hash_map        - google::sparse_hash_map\n"
        "  cuckoohash_map         - libcuckoo::cuckoohash_map\n"
        "  rhashmap               - rhashmap (C Robin Hood hash)\n"
        "  phmap_flat             - phmap::flat_hash_map\n"
        "  phmap_parallel         - phmap::parallel_flat_hash_map\n"
        "  CLHT_LB                - CLHT Lock-Based (int keys only)\n"
        "  CLHT_LF                - CLHT Lock-Free (int keys only)\n"
        "  OPIC                   - OPIC Robin Hood Hash (int keys only)\n"
        << std::endl;
}

int main(int argc, char* argv[]) {
    // Initialize logging
    LOG_DEBUG("hashmap_bench starting...");
    
    // Default parameters
    int num_power = 20;
    std::string key_type = "short_string";
    int repeat = 1;
    unsigned int pause = 0;
    bool run_all = false;
    bool run_all_impls = false;
    bool run_default = false;  // -n mode: short_string + int
    std::string specific_impl;
    
    int opt;
    while ((opt = getopt(argc, argv, "n:k:r:p:i:c:ah")) != -1) {
        switch (opt) {
            case 'n':
                num_power = atoi(optarg);
                run_default = true;
                break;
            case 'k':
                key_type = optarg;
                run_default = false;  // -k overrides -n default mode
                break;
            case 'r':
                repeat = atoi(optarg);
                break;
            case 'p':
                pause = atoi(optarg);
                break;
            case 'c': {
                int cap_factor = atoi(optarg);
                if (cap_factor > 0) {
                    clht_capacity_factor = static_cast<size_t>(cap_factor);
                }
                break;
            }
            case 'i':
                specific_impl = optarg;
                break;
            case 'a':
                run_all = true;
                run_all_impls = true;
                break;
            case 'h':
                print_help(argv[0]);
                return 0;
            default:
                print_help(argv[0]);
                return 1;
        }
    }
    
    // No arguments -> show help
    if (argc == 1) {
        print_help(argv[0]);
        return 0;
    }
    
    LOG_DEBUG( "Parameters: num_power=%d, key_type=%s, repeat=%d, pause=%u",
             num_power, key_type.c_str(), repeat, pause);
    
    std::cout << "hashmap_bench - Hash Map Performance Benchmark\n";
    std::cout << "Elements: 2^" << num_power << " = " << (1ULL << num_power) << "\n";
    std::cout << "Repetitions: " << repeat << "\n\n";
    
    std::vector<BenchmarkResult> all_results;
    
    for (int i = 0; i < repeat; i++) {
        std::cout << "\n=== Repetition " << (i + 1) << "/" << repeat << " ===\n";
        
        if (run_all) {
            // Run all key types
            auto short_results = run_all_string_benchmarks("short_string", num_power, run_all_impls);
            all_results.insert(all_results.end(), short_results.begin(), short_results.end());
            
            auto mid_results = run_all_string_benchmarks("mid_string", num_power, run_all_impls);
            all_results.insert(all_results.end(), mid_results.begin(), mid_results.end());
            
            auto long_results = run_all_string_benchmarks("long_string", num_power, run_all_impls);
            all_results.insert(all_results.end(), long_results.begin(), long_results.end());
            
            auto int_results = run_all_int_benchmarks(num_power, run_all_impls);
            all_results.insert(all_results.end(), int_results.begin(), int_results.end());
        } else if (run_default) {
            // -n mode: run short_string + int
            auto short_results = run_all_string_benchmarks("short_string", num_power, run_all_impls);
            all_results.insert(all_results.end(), short_results.begin(), short_results.end());
            
            auto int_results = run_all_int_benchmarks(num_power, run_all_impls);
            all_results.insert(all_results.end(), int_results.begin(), int_results.end());
        } else if (key_type == "int") {
            auto results = run_all_int_benchmarks(num_power, run_all_impls);
            all_results.insert(all_results.end(), results.begin(), results.end());
        } else {
            auto results = run_all_string_benchmarks(key_type, num_power, run_all_impls);
            all_results.insert(all_results.end(), results.begin(), results.end());
        }
        
        if (pause > 0) {
            sleep(pause);
        }
    }
    
    std::cout << "\nSide effect (anti-optimization): " << side_effect << "\n";
    
    LOG_DEBUG( "Benchmark completed. Side effect: %lu", side_effect);
    
    return 0;
}
