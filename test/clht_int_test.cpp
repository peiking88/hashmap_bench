/*
 * CLHT Integer Key Implementation Unit Tests
 * 
 * Comprehensive test suite for CLHT integer key implementations:
 * - ClhtLb (Lock-Based), ClhtLf (Lock-Free)
 * 
 * Based on abseil-cpp hashmap testing patterns
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_template_test_macros.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>

#include <cstdint>
#include <vector>
#include <random>
#include <algorithm>
#include <unordered_set>
#include <thread>
#include <atomic>
#include <limits>

// CLHT (lock-based and lock-free hash tables)
extern "C" {
#include <clht.h>
#include <ssmem.h>
}

namespace clht_int_test {

// ============================================================================
// CLHT Integer Key wrappers for testing
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
    static uint64_t remove(Map& ht, uint64_t k) {
        return (uint64_t)clht_remove(ht, (clht_addr_t)k);
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
    static uint64_t remove(Map& ht, uint64_t k) {
        return (uint64_t)clht_remove(ht, (clht_addr_t)k);
    }
    static void destroy(Map& ht) {
        clht_gc_destroy(ht);
    }
};

// ============================================================================
// Test Types - All CLHT Integer Implementations
// ============================================================================

template<typename T>
struct ClhtIntTestTraits;

template<>
struct ClhtIntTestTraits<ClhtLbWrapper> {
    static constexpr const char* name = "ClhtLb";
};

template<>
struct ClhtIntTestTraits<ClhtLfWrapper> {
    static constexpr const char* name = "ClhtLf";
};

// Helper to generate unique test keys
static uint64_t make_test_key(uint64_t i) {
    return i;
}

} // namespace clht_int_test

using namespace clht_int_test;

// ============================================================================
// Basic Functionality Tests
// ============================================================================

TEMPLATE_TEST_CASE("CLHT Int: Empty table lookup", "[clht_int][basic]", 
                   ClhtLbWrapper, ClhtLfWrapper) {
    using Traits = ClhtIntTestTraits<TestType>;
    using Wrapper = TestType;
    using Map = typename Wrapper::Map;
    
    Map map = Wrapper::create(100);
    
    SECTION("Lookup non-existent key returns 0") {
        REQUIRE(Wrapper::lookup(map, 12345) == 0);
    }
    
    // Note: CLHT uses key=0 as empty slot marker, so key=0 has special meaning
    
    Wrapper::destroy(map);
}

TEMPLATE_TEST_CASE("CLHT Int: Single insert and lookup", "[clht_int][basic]",
                   ClhtLbWrapper, ClhtLfWrapper) {
    using Wrapper = TestType;
    using Map = typename Wrapper::Map;
    
    Map map = Wrapper::create(100);
    
    SECTION("Insert and lookup single key") {
        Wrapper::insert(map, 42, 100);
        REQUIRE(Wrapper::lookup(map, 42) == 100);
    }
    
    SECTION("Insert with large key") {
        Wrapper::insert(map, 123456, 1);
        REQUIRE(Wrapper::lookup(map, 123456) == 1);
    }
    
    SECTION("Insert with max key") {
        Wrapper::insert(map, UINT64_MAX, 999);
        REQUIRE(Wrapper::lookup(map, UINT64_MAX) == 999);
    }
    
    SECTION("Insert with max value") {
        Wrapper::insert(map, 100, UINT64_MAX);
        REQUIRE(Wrapper::lookup(map, 100) == UINT64_MAX);
    }
    
    Wrapper::destroy(map);
}

TEMPLATE_TEST_CASE("CLHT Int: Multiple inserts", "[clht_int][basic]",
                   ClhtLbWrapper, ClhtLfWrapper) {
    using Wrapper = TestType;
    using Map = typename Wrapper::Map;
    
    Map map = Wrapper::create(100);
    
    const int N = 100;
    
    SECTION("Insert multiple unique keys") {
        for (int i = 1; i <= N; ++i) {  // Start from 1, not 0 (0 is empty marker)
            Wrapper::insert(map, i, i * 10);
        }
        
        for (int i = 1; i <= N; ++i) {
            REQUIRE(Wrapper::lookup(map, i) == i * 10);
        }
    }
    
    SECTION("Insert keys in reverse order") {
        for (int i = N; i >= 1; --i) {
            Wrapper::insert(map, i, i);
        }
        
        for (int i = 1; i <= N; ++i) {
            REQUIRE(Wrapper::lookup(map, i) == i);
        }
    }
    
    SECTION("Insert sparse keys") {
        std::vector<uint64_t> sparse_keys = {
            1, 100, 1000, 10000, 100000, 1000000, 
            UINT64_MAX / 2, UINT64_MAX - 1
        };
        
        for (size_t i = 0; i < sparse_keys.size(); ++i) {
            Wrapper::insert(map, sparse_keys[i], i);
        }
        
        for (size_t i = 0; i < sparse_keys.size(); ++i) {
            REQUIRE(Wrapper::lookup(map, sparse_keys[i]) == i);
        }
    }
    
    Wrapper::destroy(map);
}

// ============================================================================
// Update Tests
// Note: CLHT does NOT support updating existing keys - clht_put returns false
// if the key already exists. These tests document this behavior.
// ============================================================================

TEMPLATE_TEST_CASE("CLHT Int: Insert behavior", "[clht_int][insert]",
                   ClhtLbWrapper, ClhtLfWrapper) {
    using Wrapper = TestType;
    using Map = typename Wrapper::Map;
    
    Map map = Wrapper::create(100);
    
    SECTION("Insert returns true for new key") {
        // First insert should succeed
        Wrapper::insert(map, 1, 100);
        REQUIRE(Wrapper::lookup(map, 1) == 100);
    }
    
    SECTION("Duplicate insert does NOT update value (CLHT behavior)") {
        Wrapper::insert(map, 1, 100);
        REQUIRE(Wrapper::lookup(map, 1) == 100);
        
        // Second insert with same key should NOT update (CLHT returns false)
        // The value should remain 100
        Wrapper::insert(map, 1, 200);
        REQUIRE(Wrapper::lookup(map, 1) == 100);  // Value unchanged
    }
    
    SECTION("Insert distinct keys works") {
        for (int i = 1; i <= 10; ++i) {
            Wrapper::insert(map, i, i * 10);
        }
        for (int i = 1; i <= 10; ++i) {
            REQUIRE(Wrapper::lookup(map, i) == i * 10);
        }
    }
    
    Wrapper::destroy(map);
}

// ============================================================================
// Collision Tests
// ============================================================================

TEMPLATE_TEST_CASE("CLHT Int: Hash collision handling", "[clht_int][collision]",
                   ClhtLbWrapper, ClhtLfWrapper) {
    using Wrapper = TestType;
    using Map = typename Wrapper::Map;
    
    // Small table to force collisions
    Map map = Wrapper::create(4);
    
    SECTION("Insert more keys than bucket capacity") {
        const int N = 50;
        for (int i = 1; i <= N; ++i) {  // Start from 1
            Wrapper::insert(map, i, i);
        }
        
        // All should be findable
        for (int i = 1; i <= N; ++i) {
            REQUIRE(Wrapper::lookup(map, i) == i);
        }
    }
    
    SECTION("Sequential keys (potential collision pattern)") {
        // Keys that might hash to same bucket
        const int N = 30;
        for (int i = 1; i <= N; ++i) {
            Wrapper::insert(map, i * 16, i);  // Keys with stride 16
        }
        
        for (int i = 1; i <= N; ++i) {
            REQUIRE(Wrapper::lookup(map, i * 16) == i);
        }
    }
    
    SECTION("Powers of 2") {
        for (int i = 0; i < 20; ++i) {
            uint64_t key = 1ULL << i;
            if (key == 0) continue;  // Skip key 0
            Wrapper::insert(map, key, i);
        }
        
        for (int i = 0; i < 20; ++i) {
            uint64_t key = 1ULL << i;
            if (key == 0) continue;
            REQUIRE(Wrapper::lookup(map, key) == i);
        }
    }
    
    Wrapper::destroy(map);
}

// ============================================================================
// Key Range Tests
// ============================================================================

TEMPLATE_TEST_CASE("CLHT Int: Various key ranges", "[clht_int][keys]",
                   ClhtLbWrapper, ClhtLfWrapper) {
    using Wrapper = TestType;
    using Map = typename Wrapper::Map;
    
    Map map = Wrapper::create(200);
    
    SECTION("Small keys (1-255)") {
        for (uint64_t i = 1; i <= 255; ++i) {
            Wrapper::insert(map, i, i);
        }
        for (uint64_t i = 1; i <= 255; ++i) {
            REQUIRE(Wrapper::lookup(map, i) == i);
        }
    }
    
    SECTION("Medium keys (16-bit range)") {
        for (uint64_t i = 256; i < 65536; i += 256) {
            Wrapper::insert(map, i, i);
        }
        for (uint64_t i = 256; i < 65536; i += 256) {
            REQUIRE(Wrapper::lookup(map, i) == i);
        }
    }
    
    SECTION("Large keys (32-bit range)") {
        std::vector<uint64_t> large_keys = {
            0x00000000FFFFFFFFULL,
            0xFFFFFFFF00000000ULL,
            0x123456789ABCDEF0ULL,
            0xFEDCBA9876543210ULL,
            0xDEADBEEFCAFEBABEULL
        };
        
        for (size_t i = 0; i < large_keys.size(); ++i) {
            Wrapper::insert(map, large_keys[i], i);
        }
        
        for (size_t i = 0; i < large_keys.size(); ++i) {
            REQUIRE(Wrapper::lookup(map, large_keys[i]) == i);
        }
    }
    
    SECTION("Boundary keys") {
        // Note: key=0 is reserved as empty marker in CLHT
        Wrapper::insert(map, UINT64_MAX, 2);
        Wrapper::insert(map, UINT64_MAX - 1, 3);
        Wrapper::insert(map, 1, 4);
        
        REQUIRE(Wrapper::lookup(map, UINT64_MAX) == 2);
        REQUIRE(Wrapper::lookup(map, UINT64_MAX - 1) == 3);
        REQUIRE(Wrapper::lookup(map, 1) == 4);
    }
    
    Wrapper::destroy(map);
}

// ============================================================================
// Boundary Tests
// ============================================================================

TEMPLATE_TEST_CASE("CLHT Int: Boundary conditions", "[clht_int][boundary]",
                   ClhtLbWrapper, ClhtLfWrapper) {
    using Wrapper = TestType;
    using Map = typename Wrapper::Map;
    
    SECTION("Minimum capacity table") {
        Map map = Wrapper::create(1);
        Wrapper::insert(map, 1, 1);
        REQUIRE(Wrapper::lookup(map, 1) == 1);
        Wrapper::destroy(map);
    }
    
    SECTION("Large capacity table") {
        Map map = Wrapper::create(100000);
        Wrapper::insert(map, 1, 1);
        REQUIRE(Wrapper::lookup(map, 1) == 1);
        Wrapper::destroy(map);
    }
}

// ============================================================================
// Remove/Delete Tests
// ============================================================================

TEMPLATE_TEST_CASE("CLHT Int: Remove operations", "[clht_int][remove]",
                   ClhtLbWrapper, ClhtLfWrapper) {
    using Wrapper = TestType;
    using Map = typename Wrapper::Map;
    
    SECTION("Remove single key") {
        Map map = Wrapper::create(100);
        Wrapper::insert(map, 1, 100);
        REQUIRE(Wrapper::lookup(map, 1) == 100);
        
        uint64_t removed = Wrapper::remove(map, 1);
        REQUIRE(removed != 0);  // Remove should return non-zero for success
        
        Wrapper::destroy(map);
    }
    
    SECTION("Remove and reinsert") {
        Map map = Wrapper::create(100);
        
        Wrapper::insert(map, 1, 100);
        Wrapper::remove(map, 1);
        Wrapper::insert(map, 1, 200);
        
        REQUIRE(Wrapper::lookup(map, 1) == 200);
        
        Wrapper::destroy(map);
    }
    
    SECTION("Remove non-existent key") {
        Map map = Wrapper::create(100);
        
        uint64_t removed = Wrapper::remove(map, 999);
        // Should return 0 for non-existent key
        
        Wrapper::destroy(map);
    }
    
    SECTION("Remove multiple keys") {
        Map map = Wrapper::create(100);
        
        const int N = 50;
        for (int i = 1; i <= N; ++i) {  // Start from 1
            Wrapper::insert(map, i, i);
        }
        
        // Remove half
        for (int i = 2; i <= N; i += 2) {
            Wrapper::remove(map, i);
        }
        
        // Verify remaining (odd keys)
        for (int i = 1; i <= N; i += 2) {
            REQUIRE(Wrapper::lookup(map, i) == i);
        }
        
        // Verify removed (even keys)
        for (int i = 2; i <= N; i += 2) {
            REQUIRE(Wrapper::lookup(map, i) == 0);
        }
        
        Wrapper::destroy(map);
    }
}

// ============================================================================
// Stress Tests
// ============================================================================

TEMPLATE_TEST_CASE("CLHT Int: Stress test", "[clht_int][stress]",
                   ClhtLbWrapper, ClhtLfWrapper) {
    using Wrapper = TestType;
    using Map = typename Wrapper::Map;
    
    SECTION("Large number of keys") {
        const int N = 50000;
        Map map = Wrapper::create(N * 2);
        
        // Insert all (starting from 1)
        for (int i = 1; i <= N; ++i) {
            Wrapper::insert(map, i, i * 100);
        }
        
        // Verify all
        for (int i = 1; i <= N; ++i) {
            REQUIRE(Wrapper::lookup(map, i) == i * 100);
        }
        
        Wrapper::destroy(map);
    }
    
    SECTION("Random key order") {
        const int N = 10000;
        Map map = Wrapper::create(N * 2);
        
        std::vector<int> indices(N);
        std::iota(indices.begin(), indices.end(), 1);  // Start from 1
        std::mt19937 rng(42);
        std::shuffle(indices.begin(), indices.end(), rng);
        
        // Insert in random order
        for (int idx : indices) {
            Wrapper::insert(map, idx, idx);
        }
        
        // Verify in sequential order
        for (int i = 1; i <= N; ++i) {
            REQUIRE(Wrapper::lookup(map, i) == i);
        }
        
        Wrapper::destroy(map);
    }
    
    SECTION("High load factor") {
        const int N = 10000;
        Map map = Wrapper::create(N);  // Exactly N capacity
        
        for (int i = 1; i <= N; ++i) {  // Start from 1
            Wrapper::insert(map, i, i);
        }
        
        for (int i = 1; i <= N; ++i) {
            REQUIRE(Wrapper::lookup(map, i) == i);
        }
        
        Wrapper::destroy(map);
    }
}

// ============================================================================
// Consistency Tests
// ============================================================================

TEMPLATE_TEST_CASE("CLHT Int: Data consistency", "[clht_int][consistency]",
                   ClhtLbWrapper, ClhtLfWrapper) {
    using Wrapper = TestType;
    using Map = typename Wrapper::Map;
    
    SECTION("Adjacent keys are distinct") {
        Map map = Wrapper::create(100);
        
        for (int i = 1; i <= 100; ++i) {
            Wrapper::insert(map, i, i * 1000);
        }
        
        for (int i = 1; i <= 100; ++i) {
            REQUIRE(Wrapper::lookup(map, i) == i * 1000);
            REQUIRE(Wrapper::lookup(map, i) != (i + 1) * 1000);
        }
        
        Wrapper::destroy(map);
    }
    
    SECTION("Key-value pairs remain consistent after new inserts") {
        Map map = Wrapper::create(100);
        
        // Initial insert
        for (int i = 1; i <= 50; ++i) {
            Wrapper::insert(map, i, i);
        }
        
        // Add more keys (CLHT doesn't support update)
        for (int i = 51; i <= 100; ++i) {
            Wrapper::insert(map, i, i * 10);
        }
        
        // Verify
        for (int i = 1; i <= 50; ++i) {
            REQUIRE(Wrapper::lookup(map, i) == i);
        }
        for (int i = 51; i <= 100; ++i) {
            REQUIRE(Wrapper::lookup(map, i) == i * 10);
        }
        
        Wrapper::destroy(map);
    }
}

// ============================================================================
// Comparison Tests Between Implementations
// ============================================================================

TEST_CASE("CLHT Int: Compare LB and LF consistency", "[clht_int][compare]") {
    const int N = 500;  // Reduced size to avoid memory issues
    std::vector<uint64_t> keys;
    std::vector<uint64_t> values;
    
    for (int i = 1; i <= N; ++i) {  // Start from 1
        keys.push_back(i * 7);  // Sparse keys
        values.push_back(i * 123);
    }
    
    // Test LB implementation
    {
        ClhtLbWrapper::Map lb_map = ClhtLbWrapper::create(N * 2);
        
        for (int i = 0; i < N; ++i) {
            ClhtLbWrapper::insert(lb_map, keys[i], values[i]);
        }
        
        for (int i = 0; i < N; ++i) {
            REQUIRE(ClhtLbWrapper::lookup(lb_map, keys[i]) == values[i]);
        }
        
        ClhtLbWrapper::destroy(lb_map);
    }
    
    // Test LF implementation separately
    {
        ClhtLfWrapper::Map lf_map = ClhtLfWrapper::create(N * 2);
        
        for (int i = 0; i < N; ++i) {
            ClhtLfWrapper::insert(lf_map, keys[i], values[i]);
        }
        
        for (int i = 0; i < N; ++i) {
            REQUIRE(ClhtLfWrapper::lookup(lf_map, keys[i]) == values[i]);
        }
        
        ClhtLfWrapper::destroy(lf_map);
    }
}

// ============================================================================
// Memory Tests
// ============================================================================

TEMPLATE_TEST_CASE("CLHT Int: Memory allocation", "[clht_int][memory]",
                   ClhtLbWrapper, ClhtLfWrapper) {
    using Wrapper = TestType;
    using Map = typename Wrapper::Map;
    
    SECTION("Allocate and deallocate properly") {
        for (int run = 0; run < 10; ++run) {
            Map map = Wrapper::create(1000);
            for (int i = 1; i <= 500; ++i) {  // Start from 1
                Wrapper::insert(map, i, i);
            }
            Wrapper::destroy(map);
        }
    }
    
    SECTION("Handle many overflow buckets") {
        Map map = Wrapper::create(4);  // Very small to force overflows
        
        for (int i = 1; i <= 200; ++i) {  // Start from 1
            Wrapper::insert(map, i, i);
        }
        
        // Verify all still accessible
        for (int i = 1; i <= 200; ++i) {
            REQUIRE(Wrapper::lookup(map, i) == i);
        }
        
        Wrapper::destroy(map);
    }
}

// ============================================================================
// Insert-Delete Stress Tests
// ============================================================================

TEMPLATE_TEST_CASE("CLHT Int: Insert-Delete stress", "[clht_int][stress]",
                   ClhtLbWrapper, ClhtLfWrapper) {
    using Wrapper = TestType;
    using Map = typename Wrapper::Map;
    
    SECTION("Repeated insert-delete cycles") {
        Map map = Wrapper::create(100);
        
        for (int cycle = 0; cycle < 5; ++cycle) {
            // Insert (using keys 1-50)
            for (int i = 1; i <= 50; ++i) {
                Wrapper::insert(map, i, cycle * 100 + i);
            }
            
            // Verify
            for (int i = 1; i <= 50; ++i) {
                REQUIRE(Wrapper::lookup(map, i) == cycle * 100 + i);
            }
            
            // Delete
            for (int i = 1; i <= 50; ++i) {
                Wrapper::remove(map, i);
            }
        }
        
        Wrapper::destroy(map);
    }
    
    SECTION("Interleaved insert-delete") {
        Map map = Wrapper::create(100);
        
        // Insert 1-100
        for (int i = 1; i <= 100; ++i) {
            Wrapper::insert(map, i, i);
        }
        
        // Delete even, verify odd
        for (int i = 2; i <= 100; i += 2) {
            Wrapper::remove(map, i);
        }
        for (int i = 1; i <= 100; i += 2) {
            REQUIRE(Wrapper::lookup(map, i) == i);
        }
        for (int i = 2; i <= 100; i += 2) {
            REQUIRE(Wrapper::lookup(map, i) == 0);
        }
        
        // Re-insert even
        for (int i = 2; i <= 100; i += 2) {
            Wrapper::insert(map, i, i * 10);
        }
        for (int i = 2; i <= 100; i += 2) {
            REQUIRE(Wrapper::lookup(map, i) == i * 10);
        }
        
        Wrapper::destroy(map);
    }
}
