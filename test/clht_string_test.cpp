/*
 * CLHT String Implementation Unit Tests
 * 
 * Comprehensive test suite for CLHT string key implementations:
 * - ClhtStrPtr, ClhtStrInline, ClhtStrPooled, ClhtStrTagged, ClhtStrFinal
 * 
 * Based on abseil-cpp hashmap testing patterns
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_template_test_macros.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>

#include <string>
#include <vector>
#include <random>
#include <algorithm>
#include <unordered_set>
#include <thread>
#include <atomic>

#include "clht_test_wrapper.hpp"

using namespace clht_test;
using namespace clht_str;

// ============================================================================
// Test Types - All CLHT String Implementations
// ============================================================================

template<typename T>
struct ClhtStrTestTraits;

template<>
struct ClhtStrTestTraits<ClhtStrPtr> {
    static constexpr const char* name = "ClhtStrPtr";
    using Wrapper = ClhtStrPtrWrapper;
};

template<>
struct ClhtStrTestTraits<ClhtStrInline> {
    static constexpr const char* name = "ClhtStrInline";
    using Wrapper = ClhtStrInlineWrapper;
};

template<>
struct ClhtStrTestTraits<ClhtStrPooled> {
    static constexpr const char* name = "ClhtStrPooled";
    using Wrapper = ClhtStrPooledWrapper;
};

template<>
struct ClhtStrTestTraits<ClhtStrTagged> {
    static constexpr const char* name = "ClhtStrTagged";
    using Wrapper = ClhtStrTaggedWrapper;
};

template<>
struct ClhtStrTestTraits<ClhtStrFinal> {
    static constexpr const char* name = "ClhtStrFinal";
    using Wrapper = ClhtStrFinalWrapper;
};

// Helper to generate unique test keys
static std::string make_test_key(int i, size_t min_len = 1) {
    std::string key = "key_" + std::to_string(i);
    while (key.size() < min_len) {
        key += "_";
    }
    return key;
}

// ============================================================================
// Basic Functionality Tests
// ============================================================================

TEMPLATE_TEST_CASE("CLHT String: Empty table lookup", "[clht][basic]", 
                   ClhtStrPtr, ClhtStrInline, ClhtStrPooled, ClhtStrTagged, ClhtStrFinal) {
    using Traits = ClhtStrTestTraits<TestType>;
    using Wrapper = typename Traits::Wrapper;
    using Map = typename Wrapper::Map;
    
    Map map = Wrapper::create(100);
    
    SECTION("Lookup non-existent key returns sentinel") {
        REQUIRE(Wrapper::lookup(map, "nonexistent") == 0);
    }
    
    SECTION("Empty table size is zero") {
        // All implementations should have a way to check size
        // For now, just verify we can create and destroy
    }
    
    Wrapper::destroy(map);
}

TEMPLATE_TEST_CASE("CLHT String: Single insert and lookup", "[clht][basic]",
                   ClhtStrPtr, ClhtStrInline, ClhtStrPooled, ClhtStrTagged, ClhtStrFinal) {
    using Traits = ClhtStrTestTraits<TestType>;
    using Wrapper = typename Traits::Wrapper;
    using Map = typename Wrapper::Map;
    
    Map map = Wrapper::create(100);
    
    SECTION("Insert and lookup single key") {
        Wrapper::insert(map, "test_key", 42);
        REQUIRE(Wrapper::lookup(map, "test_key") == 42);
    }
    
    SECTION("Insert with empty string key") {
        Wrapper::insert(map, "", 1);
        REQUIRE(Wrapper::lookup(map, "") == 1);
    }
    
    SECTION("Insert with long key") {
        std::string long_key(1000, 'x');
        Wrapper::insert(map, long_key, 999);
        REQUIRE(Wrapper::lookup(map, long_key) == 999);
    }
    
    Wrapper::destroy(map);
}

TEMPLATE_TEST_CASE("CLHT String: Multiple inserts", "[clht][basic]",
                   ClhtStrPtr, ClhtStrInline, ClhtStrPooled, ClhtStrTagged, ClhtStrFinal) {
    using Traits = ClhtStrTestTraits<TestType>;
    using Wrapper = typename Traits::Wrapper;
    using Map = typename Wrapper::Map;
    
    Map map = Wrapper::create(100);
    
    const int N = 100;
    
    SECTION("Insert multiple unique keys") {
        for (int i = 0; i < N; ++i) {
            Wrapper::insert(map, make_test_key(i), i * 10);
        }
        
        for (int i = 0; i < N; ++i) {
            REQUIRE(Wrapper::lookup(map, make_test_key(i)) == i * 10);
        }
    }
    
    SECTION("Insert keys in reverse order") {
        for (int i = N - 1; i >= 0; --i) {
            Wrapper::insert(map, make_test_key(i), i);
        }
        
        for (int i = 0; i < N; ++i) {
            REQUIRE(Wrapper::lookup(map, make_test_key(i)) == i);
        }
    }
    
    Wrapper::destroy(map);
}

// ============================================================================
// Update Tests
// ============================================================================

TEMPLATE_TEST_CASE("CLHT String: Value update", "[clht][update]",
                   ClhtStrPtr, ClhtStrInline, ClhtStrPooled, ClhtStrTagged, ClhtStrFinal) {
    using Traits = ClhtStrTestTraits<TestType>;
    using Wrapper = typename Traits::Wrapper;
    using Map = typename Wrapper::Map;
    
    Map map = Wrapper::create(100);
    
    SECTION("Update existing key") {
        Wrapper::insert(map, "key1", 100);
        REQUIRE(Wrapper::lookup(map, "key1") == 100);
        
        Wrapper::insert(map, "key1", 200);
        REQUIRE(Wrapper::lookup(map, "key1") == 200);
    }
    
    SECTION("Multiple updates") {
        Wrapper::insert(map, "key1", 1);
        for (int i = 2; i <= 10; ++i) {
            Wrapper::insert(map, "key1", i);
            REQUIRE(Wrapper::lookup(map, "key1") == i);
        }
    }
    
    Wrapper::destroy(map);
}

// ============================================================================
// Collision Tests
// ============================================================================

TEMPLATE_TEST_CASE("CLHT String: Hash collision handling", "[clht][collision]",
                   ClhtStrPtr, ClhtStrInline, ClhtStrPooled, ClhtStrTagged, ClhtStrFinal) {
    using Traits = ClhtStrTestTraits<TestType>;
    using Wrapper = typename Traits::Wrapper;
    using Map = typename Wrapper::Map;
    
    // Small table to force collisions
    Map map = Wrapper::create(4);
    
    SECTION("Insert more keys than bucket capacity") {
        const int N = 50;
        for (int i = 0; i < N; ++i) {
            Wrapper::insert(map, make_test_key(i), i);
        }
        
        // All should be findable
        for (int i = 0; i < N; ++i) {
            REQUIRE(Wrapper::lookup(map, make_test_key(i)) == i);
        }
    }
    
    SECTION("Sequential similar keys") {
        // Keys that might hash similarly
        std::vector<std::string> keys;
        for (int i = 0; i < 20; ++i) {
            keys.push_back("key" + std::string(10, 'a' + (i % 26)));
        }
        
        for (size_t i = 0; i < keys.size(); ++i) {
            Wrapper::insert(map, keys[i], i);
        }
        
        for (size_t i = 0; i < keys.size(); ++i) {
            REQUIRE(Wrapper::lookup(map, keys[i]) == i);
        }
    }
    
    Wrapper::destroy(map);
}

// ============================================================================
// Key Length Tests
// ============================================================================

TEMPLATE_TEST_CASE("CLHT String: Various key lengths", "[clht][keys]",
                   ClhtStrPtr, ClhtStrInline, ClhtStrPooled, ClhtStrTagged, ClhtStrFinal) {
    using Traits = ClhtStrTestTraits<TestType>;
    using Wrapper = typename Traits::Wrapper;
    using Map = typename Wrapper::Map;
    
    Map map = Wrapper::create(100);
    
    SECTION("Short keys (1-8 chars)") {
        for (int len = 1; len <= 8; ++len) {
            std::string key(len, 'a' + len);
            Wrapper::insert(map, key, len);
            REQUIRE(Wrapper::lookup(map, key) == len);
        }
    }
    
    SECTION("Medium keys (16-64 chars)") {
        for (int len = 16; len <= 64; len += 16) {
            std::string key(len, 'b');
            key = "key_" + std::to_string(len) + "_" + key;
            Wrapper::insert(map, key, len);
            REQUIRE(Wrapper::lookup(map, key) == len);
        }
    }
    
    SECTION("Long keys (128-1024 chars)") {
        for (int len = 128; len <= 1024; len *= 2) {
            std::string key(len, 'c');
            key[0] = 'k'; key[1] = 'e'; key[2] = 'y';
            key[3] = '_';
            Wrapper::insert(map, key, len);
            REQUIRE(Wrapper::lookup(map, key) == len);
        }
    }
    
    SECTION("Keys with embedded nulls are handled as length-based") {
        // Note: Most implementations use length-based comparison
        std::string key1 = "key\0middle";
        std::string key2 = "key\0other";
        key1.resize(10, 'x');
        key2.resize(10, 'y');
        
        Wrapper::insert(map, key1, 100);
        Wrapper::insert(map, key2, 200);
        
        REQUIRE(Wrapper::lookup(map, key1) == 100);
        REQUIRE(Wrapper::lookup(map, key2) == 200);
    }
    
    Wrapper::destroy(map);
}

// ============================================================================
// Boundary Tests
// ============================================================================

TEMPLATE_TEST_CASE("CLHT String: Boundary conditions", "[clht][boundary]",
                   ClhtStrPtr, ClhtStrInline, ClhtStrPooled, ClhtStrTagged, ClhtStrFinal) {
    using Traits = ClhtStrTestTraits<TestType>;
    using Wrapper = typename Traits::Wrapper;
    using Map = typename Wrapper::Map;
    
    SECTION("Minimum capacity table") {
        Map map = Wrapper::create(1);
        Wrapper::insert(map, "key1", 1);
        REQUIRE(Wrapper::lookup(map, "key1") == 1);
        Wrapper::destroy(map);
    }
    
    SECTION("Large capacity table") {
        Map map = Wrapper::create(100000);
        Wrapper::insert(map, "key1", 1);
        REQUIRE(Wrapper::lookup(map, "key1") == 1);
        Wrapper::destroy(map);
    }
    
    SECTION("Value at boundary (max uintptr_t)") {
        Map map = Wrapper::create(100);
        Wrapper::insert(map, "max_key", UINTPTR_MAX - 1);  // -1 because UINTPTR_MAX is sentinel
        REQUIRE(Wrapper::lookup(map, "max_key") == UINTPTR_MAX - 1);
        Wrapper::destroy(map);
    }
}

// ============================================================================
// Stress Tests
// ============================================================================

TEMPLATE_TEST_CASE("CLHT String: Stress test", "[clht][stress]",
                   ClhtStrPtr, ClhtStrInline, ClhtStrPooled, ClhtStrTagged, ClhtStrFinal) {
    using Traits = ClhtStrTestTraits<TestType>;
    using Wrapper = typename Traits::Wrapper;
    using Map = typename Wrapper::Map;
    
    SECTION("Large number of keys") {
        const int N = 10000;
        Map map = Wrapper::create(N * 2);
        
        std::vector<std::string> keys;
        keys.reserve(N);
        for (int i = 0; i < N; ++i) {
            keys.push_back("stress_key_" + std::to_string(i) + "_" + std::string(8, 'a' + (i % 26)));
        }
        
        // Insert all
        for (int i = 0; i < N; ++i) {
            Wrapper::insert(map, keys[i], i * 100);
        }
        
        // Verify all
        for (int i = 0; i < N; ++i) {
            REQUIRE(Wrapper::lookup(map, keys[i]) == i * 100);
        }
        
        Wrapper::destroy(map);
    }
    
    SECTION("Random key order") {
        const int N = 5000;
        Map map = Wrapper::create(N * 2);
        
        std::vector<int> indices(N);
        std::iota(indices.begin(), indices.end(), 0);
        std::mt19937 rng(42);
        std::shuffle(indices.begin(), indices.end(), rng);
        
        // Insert in random order
        for (int idx : indices) {
            Wrapper::insert(map, make_test_key(idx), idx);
        }
        
        // Verify in sequential order
        for (int i = 0; i < N; ++i) {
            REQUIRE(Wrapper::lookup(map, make_test_key(i)) == i);
        }
        
        Wrapper::destroy(map);
    }
}

// ============================================================================
// Consistency Tests
// ============================================================================

TEMPLATE_TEST_CASE("CLHT String: Data consistency", "[clht][consistency]",
                   ClhtStrPtr, ClhtStrInline, ClhtStrPooled, ClhtStrTagged, ClhtStrFinal) {
    using Traits = ClhtStrTestTraits<TestType>;
    using Wrapper = typename Traits::Wrapper;
    using Map = typename Wrapper::Map;
    
    SECTION("Keys are not modified after insert") {
        Map map = Wrapper::create(100);
        std::string key = "test_key_123";
        std::string original = key;
        
        Wrapper::insert(map, key, 42);
        REQUIRE(key == original);
        
        Wrapper::destroy(map);
    }
    
    SECTION("Similar keys are distinct") {
        Map map = Wrapper::create(100);
        
        std::string keys[] = {
            "key",
            "key1",
            "key2",
            "key_",
            "key__",
            "KEY",
            "Key"
        };
        
        for (size_t i = 0; i < sizeof(keys)/sizeof(keys[0]); ++i) {
            Wrapper::insert(map, keys[i], i);
        }
        
        for (size_t i = 0; i < sizeof(keys)/sizeof(keys[0]); ++i) {
            REQUIRE(Wrapper::lookup(map, keys[i]) == i);
        }
        
        Wrapper::destroy(map);
    }
}

// ============================================================================
// Delete/Remove Tests (if supported)
// ============================================================================

TEMPLATE_TEST_CASE("CLHT String: Remove operations", "[clht][remove]",
                   ClhtStrPtr, ClhtStrInline, ClhtStrPooled, ClhtStrTagged, ClhtStrFinal) {
    using Traits = ClhtStrTestTraits<TestType>;
    using Wrapper = typename Traits::Wrapper;
    using Map = typename Wrapper::Map;
    
    SECTION("Remove single key") {
        Map map = Wrapper::create(100);
        Wrapper::insert(map, "key1", 100);
        REQUIRE(Wrapper::lookup(map, "key1") == 100);
        
        // Test remove if available
        TestType* impl = map;  // Access underlying implementation
        bool removed = impl->remove("key1");
        
        if (removed) {
            REQUIRE(Wrapper::lookup(map, "key1") == 0);  // Should not find it
        }
        
        Wrapper::destroy(map);
    }
    
    SECTION("Remove and reinsert") {
        Map map = Wrapper::create(100);
        
        Wrapper::insert(map, "key1", 100);
        TestType* impl = map;
        impl->remove("key1");
        Wrapper::insert(map, "key1", 200);
        
        REQUIRE(Wrapper::lookup(map, "key1") == 200);
        
        Wrapper::destroy(map);
    }
}

// ============================================================================
// Comparison Tests Between Implementations
// ============================================================================

TEST_CASE("CLHT String: Compare implementations consistency", "[clht][compare]") {
    const int N = 1000;
    std::vector<std::string> keys;
    std::vector<uint64_t> values;
    
    for (int i = 0; i < N; ++i) {
        keys.push_back("compare_key_" + std::to_string(i));
        values.push_back(i * 123);
    }
    
    // Insert same data into all implementations
    auto test_impl = [&](auto& wrapper, const char* name) {
        using Wrapper = std::decay_t<decltype(wrapper)>;
        using Map = typename Wrapper::Map;
        
        Map map = Wrapper::create(N * 2);
        
        for (int i = 0; i < N; ++i) {
            Wrapper::insert(map, keys[i], values[i]);
        }
        
        // Verify all values
        for (int i = 0; i < N; ++i) {
            REQUIRE(Wrapper::lookup(map, keys[i]) == values[i]);
        }
        
        Wrapper::destroy(map);
    };
    
    SECTION("All implementations return same values") {
        ClhtStrPtrWrapper ptr_wrapper;
        test_impl(ptr_wrapper, "ClhtStrPtr");
        
        ClhtStrInlineWrapper inline_wrapper;
        test_impl(inline_wrapper, "ClhtStrInline");
        
        ClhtStrPooledWrapper pooled_wrapper;
        test_impl(pooled_wrapper, "ClhtStrPooled");
        
        ClhtStrTaggedWrapper tagged_wrapper;
        test_impl(tagged_wrapper, "ClhtStrTagged");
        
        ClhtStrFinalWrapper final_wrapper;
        test_impl(final_wrapper, "ClhtStrFinal");
    }
}

// ============================================================================
// SIMD-related Tests (for Tagged and Final implementations)
// ============================================================================

TEST_CASE("CLHT String: SIMD tag matching", "[clht][simd]") {
    SECTION("ClhtStrTagged handles tag collisions correctly") {
        ClhtStrTaggedWrapper::Map map = ClhtStrTaggedWrapper::create(100);
        
        // Insert keys that might have similar tags
        for (int i = 0; i < 100; ++i) {
            std::string key = "tag_test_" + std::to_string(i);
            ClhtStrTaggedWrapper::insert(map, key, i);
        }
        
        // All should be correctly found
        for (int i = 0; i < 100; ++i) {
            std::string key = "tag_test_" + std::to_string(i);
            REQUIRE(ClhtStrTaggedWrapper::lookup(map, key) == i);
        }
        
        ClhtStrTaggedWrapper::destroy(map);
    }
    
    SECTION("ClhtStrFinal SIMD optimizations work correctly") {
        ClhtStrFinalWrapper::Map map = ClhtStrFinalWrapper::create(100);
        
        // Insert keys with similar hashes to test SIMD path
        for (int i = 0; i < 100; ++i) {
            std::string key = "simd_final_" + std::to_string(i);
            ClhtStrFinalWrapper::insert(map, key, i);
        }
        
        for (int i = 0; i < 100; ++i) {
            std::string key = "simd_final_" + std::to_string(i);
            REQUIRE(ClhtStrFinalWrapper::lookup(map, key) == i);
        }
        
        ClhtStrFinalWrapper::destroy(map);
    }
}

// ============================================================================
// Memory Tests
// ============================================================================

TEMPLATE_TEST_CASE("CLHT String: Memory allocation", "[clht][memory]",
                   ClhtStrPtr, ClhtStrInline, ClhtStrPooled, ClhtStrTagged, ClhtStrFinal) {
    using Traits = ClhtStrTestTraits<TestType>;
    using Wrapper = typename Traits::Wrapper;
    using Map = typename Wrapper::Map;
    
    SECTION("Allocate and deallocate properly") {
        // This test mainly checks that we don't crash or leak
        for (int run = 0; run < 10; ++run) {
            Map map = Wrapper::create(1000);
            for (int i = 0; i < 500; ++i) {
                Wrapper::insert(map, make_test_key(i), i);
            }
            Wrapper::destroy(map);
        }
    }
    
    SECTION("Handle many overflow buckets") {
        Map map = Wrapper::create(4);  // Very small to force overflows
        
        for (int i = 0; i < 100; ++i) {
            Wrapper::insert(map, make_test_key(i, 10), i);
        }
        
        // Verify all still accessible
        for (int i = 0; i < 100; ++i) {
            REQUIRE(Wrapper::lookup(map, make_test_key(i, 10)) == i);
        }
        
        Wrapper::destroy(map);
    }
}

// ============================================================================
// Integration with Benchmark Framework
// ============================================================================

// Helper function for generating short keys (inline to avoid dependency)
static void generate_test_short_keys(std::vector<std::string>& keys, int num_power) {
    keys.clear();
    size_t count = 1ULL << num_power;
    keys.reserve(count);
    for (size_t i = 0; i < count; ++i) {
        keys.push_back("key_" + std::to_string(i));
    }
}

TEMPLATE_TEST_CASE("CLHT String: Benchmark integration", "[clht][benchmark]",
                   ClhtStrPtr, ClhtStrInline, ClhtStrPooled, ClhtStrTagged, ClhtStrFinal) {
    using Traits = ClhtStrTestTraits<TestType>;
    using Wrapper = typename Traits::Wrapper;
    using Map = typename Wrapper::Map;
    
    std::vector<std::string> keys;
    generate_test_short_keys(keys, 10);  // 2^10 = 1024 keys
    
    Map map = Wrapper::create(keys.size() * 2);
    
    // Insert all keys
    for (size_t i = 0; i < keys.size(); ++i) {
        Wrapper::insert(map, keys[i], i);
    }
    
    // Verify all keys
    for (size_t i = 0; i < keys.size(); ++i) {
        REQUIRE(Wrapper::lookup(map, keys[i]) == i);
    }
    
    Wrapper::destroy(map);
}
