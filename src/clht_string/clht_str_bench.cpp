/**
 * CLHT String Key Benchmark
 * 
 * Compares three approaches for supporting string keys in CLHT:
 *   - Approach A: Hash + Pointer
 *   - Approach B: Fixed-length Inline
 *   - Approach C: External Key Pool
 */

#include <iostream>
#include <chrono>
#include <random>
#include <string>
#include <vector>
#include <iomanip>

#include "clht_str_ptr.hpp"
#include "clht_str_inline.hpp"
#include "clht_str_pooled.hpp"

using namespace clht_str;

// ============================================================================
// Test Data Generation
// ============================================================================

std::string generate_random_string(size_t len, std::mt19937& rng) {
    static const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    std::uniform_int_distribution<size_t> dist(0, sizeof(charset) - 2);
    
    std::string result;
    result.reserve(len);
    for (size_t i = 0; i < len; ++i) {
        result += charset[dist(rng)];
    }
    return result;
}

std::vector<std::string> generate_keys(size_t count, size_t min_len, size_t max_len, unsigned seed = 42) {
    std::mt19937 rng(seed);
    std::uniform_int_distribution<size_t> len_dist(min_len, max_len);
    
    std::vector<std::string> keys;
    keys.reserve(count);
    
    for (size_t i = 0; i < count; ++i) {
        keys.push_back(generate_random_string(len_dist(rng), rng));
    }
    
    return keys;
}

// ============================================================================
// Benchmark Utilities
// ============================================================================

template<typename Func>
double measure_time_ms(Func&& func) {
    auto start = std::chrono::high_resolution_clock::now();
    func();
    auto end = std::chrono::high_resolution_clock::now();
    return std::chrono::duration<double, std::milli>(end - start).count();
}

template<typename Hashtable>
void benchmark_insert(Hashtable& ht, const std::vector<std::string>& keys, const std::string& name) {
    double time_ms = measure_time_ms([&]() {
        for (size_t i = 0; i < keys.size(); ++i) {
            ht.insert(keys[i], static_cast<uintptr_t>(i + 1));
        }
    });
    
    double ops_per_sec = (keys.size() / time_ms) * 1000.0;
    std::cout << std::setw(20) << name 
              << " Insert: " << std::setw(10) << std::fixed << std::setprecision(2) << time_ms << " ms"
              << "  (" << std::setw(12) << std::setprecision(0) << ops_per_sec << " ops/s)" << std::endl;
}

template<typename Hashtable>
void benchmark_lookup(Hashtable& ht, const std::vector<std::string>& keys, const std::string& name) {
    volatile uintptr_t result = 0;
    
    double time_ms = measure_time_ms([&]() {
        for (const auto& key : keys) {
            result = ht.lookup(key);
        }
    });
    
    double ops_per_sec = (keys.size() / time_ms) * 1000.0;
    std::cout << std::setw(20) << name 
              << " Lookup: " << std::setw(10) << std::fixed << std::setprecision(2) << time_ms << " ms"
              << "  (" << std::setw(12) << std::setprecision(0) << ops_per_sec << " ops/s)" << std::endl;
    
    (void)result;  // Suppress unused warning
}

// ============================================================================
// Main Benchmark
// ============================================================================

int main(int argc, char* argv[]) {
    size_t num_keys = 1000000;  // 1M keys
    size_t min_len = 6;         // Minimum string length
    size_t max_len = 16;        // Maximum string length (fits in inline)
    
    if (argc > 1) {
        num_keys = std::stoul(argv[1]);
    }
    if (argc > 2) {
        max_len = std::stoul(argv[2]);
    }
    
    std::cout << "========================================" << std::endl;
    std::cout << "CLHT String Key Benchmark" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Keys: " << num_keys << std::endl;
    std::cout << "String length: " << min_len << "-" << max_len << " bytes" << std::endl;
    std::cout << "========================================" << std::endl << std::endl;
    
    // Generate test data
    std::cout << "Generating test keys..." << std::endl;
    auto keys = generate_keys(num_keys, min_len, max_len);
    std::cout << "Done." << std::endl << std::endl;
    
    // ------------------------------------------------------------------
    // Approach A: Hash + Pointer
    // ------------------------------------------------------------------
    std::cout << "--- Approach A: Hash + Pointer ---" << std::endl;
    {
        ClhtStrPtr ht(num_keys * 2);
        
        benchmark_insert(ht, keys, "Ptr");
        benchmark_lookup(ht, keys, "Ptr");
        
        std::cout << "Size: " << ht.size() << std::endl;
    }
    std::cout << std::endl;
    
    // ------------------------------------------------------------------
    // Approach B: Fixed-length Inline
    // ------------------------------------------------------------------
    std::cout << "--- Approach B: Fixed-length Inline ---" << std::endl;
    std::cout << "(Max key length: " << ClhtStrInline::max_key_length() << " bytes)" << std::endl;
    {
        ClhtStrInline ht(num_keys * 2);
        
        benchmark_insert(ht, keys, "Inline");
        benchmark_lookup(ht, keys, "Inline");
        
        std::cout << "Size: " << ht.size() << std::endl;
    }
    std::cout << std::endl;
    
    // ------------------------------------------------------------------
    // Approach C: External Key Pool
    // ------------------------------------------------------------------
    std::cout << "--- Approach C: External Key Pool ---" << std::endl;
    {
        // Estimate pool size needed
        size_t estimated_pool_size = num_keys * ((min_len + max_len) / 2 + 8);
        ClhtStrPooled ht(num_keys * 2, estimated_pool_size * 2);
        
        benchmark_insert(ht, keys, "Pooled");
        benchmark_lookup(ht, keys, "Pooled");
        
        std::cout << "Size: " << ht.size() << std::endl;
        std::cout << "Pool used: " << ht.pool_used() / 1024 << " KB" << std::endl;
    }
    std::cout << std::endl;
    
    // ------------------------------------------------------------------
    // Longer Strings Benchmark
    // ------------------------------------------------------------------
    if (max_len <= 32) {
        std::cout << "========================================" << std::endl;
        std::cout << "Benchmark with longer strings (32-64 bytes)" << std::endl;
        std::cout << "========================================" << std::endl;
        
        auto long_keys = generate_keys(num_keys / 10, 32, 64);  // 100K keys
        
        // Approach A
        std::cout << "\n--- Approach A: Hash + Pointer (long strings) ---" << std::endl;
        {
            ClhtStrPtr ht(num_keys / 5);
            benchmark_insert(ht, long_keys, "Ptr");
            benchmark_lookup(ht, long_keys, "Ptr");
        }
        
        // Approach B (will truncate)
        std::cout << "\n--- Approach B: Inline (truncated to " << ClhtStrInline::max_key_length() << " bytes) ---" << std::endl;
        {
            ClhtStrInline ht(num_keys / 5);
            benchmark_insert(ht, long_keys, "Inline");
            benchmark_lookup(ht, long_keys, "Inline");
        }
        
        // Approach C
        std::cout << "\n--- Approach C: Key Pool (long strings) ---" << std::endl;
        {
            size_t pool_size = long_keys.size() * 50;
            ClhtStrPooled ht(num_keys / 5, pool_size);
            benchmark_insert(ht, long_keys, "Pooled");
            benchmark_lookup(ht, long_keys, "Pooled");
        }
    }
    
    return 0;
}
