#pragma once

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <functional>
#include <string>
#include <vector>

#include <sys/time.h>

namespace hashmap_bench {

// Benchmark result structure
struct BenchmarkResult {
    std::string impl_name;
    std::string key_type;
    uint64_t num_elements;
    double insert_time_sec;
    double query_time_sec;
    size_t memory_bytes;
};

// Time measurement helper
inline double get_time_diff(const struct timeval& start, const struct timeval& end) {
    long sec = end.tv_sec - start.tv_sec;
    long usec = end.tv_usec - start.tv_usec;
    if (usec < 0) {
        sec--;
        usec += 1000000;
    }
    return static_cast<double>(sec) + static_cast<double>(usec) / 1000000.0;
}

// Timer class for RAII-style timing
class Timer {
public:
    Timer() { gettimeofday(&start_, nullptr); }
    
    double elapsed() const {
        struct timeval end;
        gettimeofday(&end, nullptr);
        return get_time_diff(start_, end);
    }
    
    void reset() { gettimeofday(&start_, nullptr); }

private:
    struct timeval start_;
};

// Key generation functions
void generate_short_keys(std::vector<std::string>& keys, int num_power);
void generate_mid_keys(std::vector<std::string>& keys, int num_power);
void generate_long_keys(std::vector<std::string>& keys, int num_power);
void generate_int_keys(std::vector<uint64_t>& keys, int num_power);

// Hash functions
uint64_t tomas_wang_int32_hash(uint32_t key);
uint64_t tomas_wang_int64_hash(uint64_t key);

// Side effect to prevent compiler optimization
extern uint64_t side_effect;

// Benchmark runner interface
template <typename Map, typename Key, typename Value>
class MapBenchmark {
public:
    using CreateMap = std::function<Map(size_t)>;
    using InsertFunc = std::function<void(Map&, const Key&, Value)>;
    using LookupFunc = std::function<Value(Map&, const Key&)>;
    using DestroyMap = std::function<void(Map&)>;
    
    static BenchmarkResult run(
        const std::string& impl_name,
        const std::string& key_type,
        const std::vector<Key>& keys,
        CreateMap create_map,
        InsertFunc insert,
        LookupFunc lookup,
        DestroyMap destroy = [](Map&) {}) {
        
        BenchmarkResult result;
        result.impl_name = impl_name;
        result.key_type = key_type;
        result.num_elements = keys.size();
        
        // Create map
        Map map = create_map(keys.size());
        
        // Insert benchmark
        Timer timer;
        for (const auto& key : keys) {
            insert(map, key, Value{});
        }
        result.insert_time_sec = timer.elapsed();
        
        // Query benchmark
        timer.reset();
        for (const auto& key : keys) {
            side_effect += lookup(map, key);
        }
        result.query_time_sec = timer.elapsed();
        
        // Cleanup
        destroy(map);
        
        return result;
    }
};

// Result printer
void print_result(const BenchmarkResult& result);
void print_results(const std::vector<BenchmarkResult>& results);

} // namespace hashmap_bench
