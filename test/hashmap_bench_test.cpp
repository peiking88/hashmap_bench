/*
 * hashmap_test - Catch2 test cases for hashmap_bench
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>

#include <string>
#include <vector>
#include <unordered_map>

#include "benchmark.hpp"
#include "hash_maps.hpp"

using namespace hashmap_bench;

// ============================================================================
// Key Generation Tests
// ============================================================================

TEST_CASE("Key generation - short_string", "[keys]") {
    std::vector<std::string> keys;
    generate_short_keys(keys, 16);  // 2^16 = 65536 elements
    
    REQUIRE(keys.size() == 65536);
    REQUIRE(keys[0].size() == 6);
    
    // Check uniqueness
    std::unordered_map<std::string, int> unique_check;
    for (const auto& key : keys) {
        unique_check[key]++;
    }
    REQUIRE(unique_check.size() == 65536);
}

TEST_CASE("Key generation - mid_string", "[keys]") {
    std::vector<std::string> keys;
    generate_mid_keys(keys, 16);
    
    REQUIRE(keys.size() == 65536);
    REQUIRE(keys[0].size() == 32);
}

TEST_CASE("Key generation - long_string", "[keys]") {
    std::vector<std::string> keys;
    generate_long_keys(keys, 16);
    
    REQUIRE(keys.size() == 65536);
    REQUIRE(keys[0].size() == 256);
}

TEST_CASE("Key generation - int", "[keys]") {
    std::vector<uint64_t> keys;
    generate_int_keys(keys, 16);
    
    REQUIRE(keys.size() == 65536);
    REQUIRE(keys[0] == 0);
    REQUIRE(keys[65535] == 65535);
}

// ============================================================================
// Hash Function Tests
// ============================================================================

TEST_CASE("Tomas Wang hash functions", "[hash]") {
    SECTION("int32 hash") {
        uint32_t key1 = 12345;
        uint32_t key2 = 12346;
        
        uint64_t hash1 = tomas_wang_int32_hash(key1);
        uint64_t hash2 = tomas_wang_int32_hash(key2);
        
        REQUIRE(hash1 != hash2);
        REQUIRE(hash1 == tomas_wang_int32_hash(key1));  // Deterministic
    }
    
    SECTION("int64 hash") {
        uint64_t key1 = 12345678901234ULL;
        uint64_t key2 = 12345678901235ULL;
        
        uint64_t hash1 = tomas_wang_int64_hash(key1);
        uint64_t hash2 = tomas_wang_int64_hash(key2);
        
        REQUIRE(hash1 != hash2);
        REQUIRE(hash1 == tomas_wang_int64_hash(key1));  // Deterministic
    }
}

// ============================================================================
// std::unordered_map Tests
// ============================================================================

TEST_CASE("std::unordered_map string keys", "[hashmap][std]") {
    using Wrapper = StdUnorderedMapWrapper<std::string, uint64_t>;
    using Map = typename Wrapper::Map;
    
    Map map = Wrapper::create(100);
    
    Wrapper::insert(map, "key1", 100);
    Wrapper::insert(map, "key2", 200);
    Wrapper::insert(map, "key3", 300);
    
    REQUIRE(Wrapper::lookup(map, "key1") == 100);
    REQUIRE(Wrapper::lookup(map, "key2") == 200);
    REQUIRE(Wrapper::lookup(map, "key3") == 300);
    
    Wrapper::destroy(map);
}

TEST_CASE("std::unordered_map int keys", "[hashmap][std]") {
    using Wrapper = StdUnorderedMapWrapper<uint64_t, uint64_t>;
    using Map = typename Wrapper::Map;
    
    Map map = Wrapper::create(100);
    
    Wrapper::insert(map, 1, 100);
    Wrapper::insert(map, 2, 200);
    Wrapper::insert(map, 3, 300);
    
    REQUIRE(Wrapper::lookup(map, 1) == 100);
    REQUIRE(Wrapper::lookup(map, 2) == 200);
    REQUIRE(Wrapper::lookup(map, 3) == 300);
    
    Wrapper::destroy(map);
}

// ============================================================================
// absl::flat_hash_map Tests
// ============================================================================

TEST_CASE("absl::flat_hash_map string keys", "[hashmap][absl]") {
    using Wrapper = AbslFlatHashMapWrapper<std::string, uint64_t>;
    using Map = typename Wrapper::Map;
    
    Map map = Wrapper::create(100);
    
    Wrapper::insert(map, "key1", 100);
    Wrapper::insert(map, "key2", 200);
    Wrapper::insert(map, "key3", 300);
    
    REQUIRE(Wrapper::lookup(map, "key1") == 100);
    REQUIRE(Wrapper::lookup(map, "key2") == 200);
    REQUIRE(Wrapper::lookup(map, "key3") == 300);
    
    Wrapper::destroy(map);
}

TEST_CASE("absl::flat_hash_map int keys", "[hashmap][absl]") {
    using Wrapper = AbslFlatHashMapWrapper<uint64_t, uint64_t>;
    using Map = typename Wrapper::Map;
    
    Map map = Wrapper::create(100);
    
    Wrapper::insert(map, 1, 100);
    Wrapper::insert(map, 2, 200);
    Wrapper::insert(map, 3, 300);
    
    REQUIRE(Wrapper::lookup(map, 1) == 100);
    REQUIRE(Wrapper::lookup(map, 2) == 200);
    REQUIRE(Wrapper::lookup(map, 3) == 300);
    
    Wrapper::destroy(map);
}

// ============================================================================
// absl::node_hash_map Tests
// ============================================================================

TEST_CASE("absl::node_hash_map string keys", "[hashmap][absl]") {
    using Wrapper = AbslNodeHashMapWrapper<std::string, uint64_t>;
    using Map = typename Wrapper::Map;
    
    Map map = Wrapper::create(100);
    
    Wrapper::insert(map, "key1", 100);
    Wrapper::insert(map, "key2", 200);
    Wrapper::insert(map, "key3", 300);
    
    REQUIRE(Wrapper::lookup(map, "key1") == 100);
    REQUIRE(Wrapper::lookup(map, "key2") == 200);
    REQUIRE(Wrapper::lookup(map, "key3") == 300);
    
    Wrapper::destroy(map);
}

// ============================================================================
// cista::hash_map Tests
// ============================================================================

TEST_CASE("cista::hash_map string keys", "[hashmap][cista]") {
    using Wrapper = CistaHashMapWrapper<std::string, uint64_t>;
    using Map = typename Wrapper::Map;
    
    Map map = Wrapper::create(100);
    
    Wrapper::insert(map, "key1", 100);
    Wrapper::insert(map, "key2", 200);
    Wrapper::insert(map, "key3", 300);
    
    REQUIRE(Wrapper::lookup(map, "key1") == 100);
    REQUIRE(Wrapper::lookup(map, "key2") == 200);
    REQUIRE(Wrapper::lookup(map, "key3") == 300);
    
    Wrapper::destroy(map);
}

TEST_CASE("cista::hash_map int keys", "[hashmap][cista]") {
    using Wrapper = CistaHashMapWrapper<uint64_t, uint64_t>;
    using Map = typename Wrapper::Map;
    
    Map map = Wrapper::create(100);
    
    Wrapper::insert(map, 1, 100);
    Wrapper::insert(map, 2, 200);
    Wrapper::insert(map, 3, 300);
    
    REQUIRE(Wrapper::lookup(map, 1) == 100);
    REQUIRE(Wrapper::lookup(map, 2) == 200);
    REQUIRE(Wrapper::lookup(map, 3) == 300);
    
    Wrapper::destroy(map);
}

// ============================================================================
// libcuckoo::cuckoohash_map Tests
// ============================================================================

TEST_CASE("libcuckoo::cuckoohash_map string keys", "[hashmap][cuckoo]") {
    using Wrapper = CuckooHashMapWrapper<std::string, uint64_t>;
    using Map = typename Wrapper::Map;
    
    Map map = Wrapper::create(100);
    
    Wrapper::insert(map, "key1", 100);
    Wrapper::insert(map, "key2", 200);
    Wrapper::insert(map, "key3", 300);
    
    REQUIRE(Wrapper::lookup(map, "key1") == 100);
    REQUIRE(Wrapper::lookup(map, "key2") == 200);
    REQUIRE(Wrapper::lookup(map, "key3") == 300);
    
    Wrapper::destroy(map);
}

TEST_CASE("libcuckoo::cuckoohash_map int keys", "[hashmap][cuckoo]") {
    using Wrapper = CuckooHashMapWrapper<uint64_t, uint64_t>;
    using Map = typename Wrapper::Map;
    
    Map map = Wrapper::create(100);
    
    Wrapper::insert(map, 1, 100);
    Wrapper::insert(map, 2, 200);
    Wrapper::insert(map, 3, 300);
    
    REQUIRE(Wrapper::lookup(map, 1) == 100);
    REQUIRE(Wrapper::lookup(map, 2) == 200);
    REQUIRE(Wrapper::lookup(map, 3) == 300);
    
    Wrapper::destroy(map);
}

// ============================================================================
// rhashmap Tests (C library - string keys only)
// ============================================================================

TEST_CASE("rhashmap string keys", "[hashmap][rhashmap]") {
    using Wrapper = RhashmapWrapper;
    using Map = typename Wrapper::Map;
    
    Map map = Wrapper::create(100);
    
    Wrapper::insert(map, "key1", 100);
    Wrapper::insert(map, "key2", 200);
    Wrapper::insert(map, "key3", 300);
    
    REQUIRE(Wrapper::lookup(map, "key1") == 100);
    REQUIRE(Wrapper::lookup(map, "key2") == 200);
    REQUIRE(Wrapper::lookup(map, "key3") == 300);
    
    Wrapper::destroy(map);
}

// ============================================================================
// boost::container::flat_map Tests
// ============================================================================

TEST_CASE("boost::flat_map string keys", "[hashmap][boost]") {
    using Wrapper = BoostFlatMapWrapper<std::string, uint64_t>;
    using Map = typename Wrapper::Map;
    
    Map map = Wrapper::create(100);
    
    Wrapper::insert(map, "key1", 100);
    Wrapper::insert(map, "key2", 200);
    Wrapper::insert(map, "key3", 300);
    
    REQUIRE(Wrapper::lookup(map, "key1") == 100);
    REQUIRE(Wrapper::lookup(map, "key2") == 200);
    REQUIRE(Wrapper::lookup(map, "key3") == 300);
    
    Wrapper::destroy(map);
}

TEST_CASE("boost::flat_map int keys", "[hashmap][boost]") {
    using Wrapper = BoostFlatMapWrapper<uint64_t, uint64_t>;
    using Map = typename Wrapper::Map;
    
    Map map = Wrapper::create(100);
    
    Wrapper::insert(map, 1, 100);
    Wrapper::insert(map, 2, 200);
    Wrapper::insert(map, 3, 300);
    
    REQUIRE(Wrapper::lookup(map, 1) == 100);
    REQUIRE(Wrapper::lookup(map, 2) == 200);
    REQUIRE(Wrapper::lookup(map, 3) == 300);
    
    Wrapper::destroy(map);
}

// ============================================================================
// Timer Tests
// ============================================================================

TEST_CASE("Timer functionality", "[timer]") {
    Timer timer;
    
    // Timer should start immediately
    double t1 = timer.elapsed();
    REQUIRE(t1 >= 0);
    
    // Sleep a bit
    usleep(10000);  // 10ms
    
    double t2 = timer.elapsed();
    REQUIRE(t2 > t1);
    REQUIRE(t2 >= 0.01);  // At least 10ms
}

// ============================================================================
// Result Printing Tests
// ============================================================================

TEST_CASE("BenchmarkResult printing", "[output]") {
    BenchmarkResult result;
    result.impl_name = "test_map";
    result.key_type = "string";
    result.num_elements = 1000000;
    result.insert_time_sec = 0.5;
    result.query_time_sec = 0.3;
    
    // Just ensure it doesn't crash
    REQUIRE_NOTHROW(print_result(result));
    
    std::vector<BenchmarkResult> results = {result};
    REQUIRE_NOTHROW(print_results(results));
}
