cmake_minimum_required(VERSION 3.16)
project(hashmap_bench VERSION 1.0.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Optimization flags
set(CMAKE_CXX_FLAGS_RELEASE "-O3")
set(CMAKE_CXX_FLAGS_DEBUG "-g")

# Build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# ============================================================================
# Project directories
# ============================================================================
set(EXTERNAL_DIR "${CMAKE_SOURCE_DIR}/external")
set(SRC_DIR "${CMAKE_SOURCE_DIR}/src")

# ============================================================================
# External Dependencies
# ============================================================================

# ----------------------------------------------------------------------------
# NanoLog
# ----------------------------------------------------------------------------
add_library(nanolog STATIC
    ${EXTERNAL_DIR}/NanoLog/runtime/NanoLog.cc
    ${EXTERNAL_DIR}/NanoLog/runtime/Log.cc
    ${EXTERNAL_DIR}/NanoLog/runtime/RuntimeLogger.cc
    ${EXTERNAL_DIR}/NanoLog/runtime/TimeTrace.cc
    ${EXTERNAL_DIR}/NanoLog/runtime/Cycles.cc
)
target_include_directories(nanolog PUBLIC ${EXTERNAL_DIR}/NanoLog/runtime)
target_compile_features(nanolog PUBLIC cxx_std_17)

# ----------------------------------------------------------------------------
# Abseil-cpp
# ----------------------------------------------------------------------------
set(ABSL_BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(ABSL_USE_EXTERNAL_GOOGLETEST OFF CACHE BOOL "" FORCE)
add_subdirectory(${EXTERNAL_DIR}/abseil-cpp absl-cpp)

# ----------------------------------------------------------------------------
# Boost.Container (header-only)
# ----------------------------------------------------------------------------
add_library(boost_container INTERFACE)
target_include_directories(boost_container INTERFACE ${EXTERNAL_DIR}/container/include)

# ----------------------------------------------------------------------------
# Folly F14 (minimal build)
# ----------------------------------------------------------------------------
set(FOLLY_SOURCE_DIR "${EXTERNAL_DIR}/folly")

set(FOLLY_MINIMAL_SOURCES
    ${FOLLY_SOURCE_DIR}/folly/lang/SafeAssert.cpp
    ${FOLLY_SOURCE_DIR}/folly/lang/Exception.cpp
    ${FOLLY_SOURCE_DIR}/folly/lang/ToAscii.cpp
    ${FOLLY_SOURCE_DIR}/folly/lang/CString.cpp
    ${FOLLY_SOURCE_DIR}/folly/lang/UncaughtExceptions.cpp
    ${FOLLY_SOURCE_DIR}/folly/memory/Malloc.cpp
    ${FOLLY_SOURCE_DIR}/folly/container/detail/F14Table.cpp
)

add_library(folly_f14_minimal STATIC ${FOLLY_MINIMAL_SOURCES})
target_include_directories(folly_f14_minimal PUBLIC ${FOLLY_SOURCE_DIR})
target_compile_features(folly_f14_minimal PUBLIC cxx_std_20)
target_compile_definitions(folly_f14_minimal PUBLIC
    FOLLY_NO_CONFIG
    FOLLY_MOBILE=0
    FOLLY_X64=1
    FOLLY_SSE=2
    FOLLY_F14_VECTOR_INTRINSICS_AVAILABLE=1
)
target_link_libraries(folly_f14_minimal PUBLIC ${CMAKE_DL_LIBS} atomic pthread)

# ----------------------------------------------------------------------------
# rhashmap (C library)
# ----------------------------------------------------------------------------
add_library(rhashmap STATIC
    ${EXTERNAL_DIR}/rhashmap/src/rhashmap.c
    ${EXTERNAL_DIR}/rhashmap/src/murmurhash.c
    ${EXTERNAL_DIR}/rhashmap/src/siphash.c
)
# Source files use <rhashmap/xxx.h> style includes
target_include_directories(rhashmap PUBLIC ${EXTERNAL_DIR})

# ----------------------------------------------------------------------------
# sparsehash (header-only)
# ----------------------------------------------------------------------------
add_library(sparsehash INTERFACE)
target_include_directories(sparsehash INTERFACE
    ${EXTERNAL_DIR}/sparsehash
    ${EXTERNAL_DIR}/sparsehash/google
    ${EXTERNAL_DIR}/sparsehash/sparsehash
)

# ----------------------------------------------------------------------------
# libcuckoo (header-only)
# ----------------------------------------------------------------------------
add_library(libcuckoo INTERFACE)
target_include_directories(libcuckoo INTERFACE ${EXTERNAL_DIR}/libcuckoo)

# ----------------------------------------------------------------------------
# cista (header-only)
# ----------------------------------------------------------------------------
add_library(cista INTERFACE)
target_include_directories(cista INTERFACE ${EXTERNAL_DIR}/cista)
target_compile_features(cista INTERFACE cxx_std_17)

# ----------------------------------------------------------------------------
# ssmem
# ----------------------------------------------------------------------------
add_library(ssmem STATIC ${EXTERNAL_DIR}/clht/src/ssmem.c)
# Source files use <ssmem/ssmem.h> and <clht/xxx.h> style includes
target_include_directories(ssmem PUBLIC ${EXTERNAL_DIR}/clht)
target_compile_definitions(ssmem PRIVATE _GNU_SOURCE)

# ----------------------------------------------------------------------------
# CLHT (Cache-Line Hash Table)
# ----------------------------------------------------------------------------
add_library(clht_lb STATIC
    ${EXTERNAL_DIR}/clht/src/clht_lb_res.c
    ${EXTERNAL_DIR}/clht/src/clht_gc.c
)
# Source files use <clht/xxx.h> style includes
target_include_directories(clht_lb PUBLIC ${EXTERNAL_DIR}/clht)
target_compile_definitions(clht_lb PRIVATE _GNU_SOURCE CORES_PER_SOCKET=8 DEFAULT)
target_link_libraries(clht_lb PUBLIC ssmem pthread)

add_library(clht_lf STATIC
    ${EXTERNAL_DIR}/clht/src/clht_lf_res.c
    ${EXTERNAL_DIR}/clht/src/clht_gc.c
)
target_include_directories(clht_lf PUBLIC ${EXTERNAL_DIR}/clht)
target_compile_definitions(clht_lf PRIVATE _GNU_SOURCE CORES_PER_SOCKET=8 DEFAULT)
target_link_libraries(clht_lf PUBLIC ssmem pthread)

# ----------------------------------------------------------------------------
# OPIC Robin Hood Hash
# ----------------------------------------------------------------------------
add_library(opic STATIC
    ${EXTERNAL_DIR}/opic/src/op_hash_table.c
    ${EXTERNAL_DIR}/opic/src/cityhash.c
    ${EXTERNAL_DIR}/opic/src/op_malloc.c
    ${EXTERNAL_DIR}/opic/src/allocator.c
    ${EXTERNAL_DIR}/opic/src/deallocator.c
    ${EXTERNAL_DIR}/opic/src/init_helper.c
    ${EXTERNAL_DIR}/opic/src/lookup_helper.c
)
target_include_directories(opic PUBLIC ${EXTERNAL_DIR})
target_compile_definitions(opic PRIVATE HAVE_CONFIG_H NDEBUG)

# ----------------------------------------------------------------------------
# parallel-hashmap (header-only)
# ----------------------------------------------------------------------------
add_library(parallel_hashmap INTERFACE)
target_include_directories(parallel_hashmap INTERFACE ${EXTERNAL_DIR}/parallel-hashmap)

# ============================================================================
# Main executable
# ============================================================================
add_executable(hashmap_bench
    ${SRC_DIR}/hashmap_bench.cpp
    ${SRC_DIR}/benchmark.cpp
)

target_include_directories(hashmap_bench PRIVATE
    ${SRC_DIR}
    ${EXTERNAL_DIR}/NanoLog/runtime
)

target_link_libraries(hashmap_bench PRIVATE
    nanolog
    absl::flat_hash_map
    absl::node_hash_map
    cista
    rhashmap
    boost_container
    sparsehash
    libcuckoo
    opic
    clht_lb
    clht_lf
    folly_f14_minimal
    parallel_hashmap
    atomic
    pthread
)

# ============================================================================
# Test executable with Catch2
# ----------------------------------------------------------------------------
add_subdirectory(${EXTERNAL_DIR}/Catch2 catch2)

add_executable(hashmap_bench_test
    test/hashmap_bench_test.cpp
    ${SRC_DIR}/benchmark.cpp
)

target_include_directories(hashmap_bench_test PRIVATE
    ${SRC_DIR}
    ${EXTERNAL_DIR}/NanoLog/runtime
)

target_link_libraries(hashmap_bench_test PRIVATE
    Catch2::Catch2WithMain
    nanolog
    absl::flat_hash_map
    absl::node_hash_map
    cista
    rhashmap
    boost_container
    sparsehash
    libcuckoo
    opic
    clht_lb
    clht_lf
    folly_f14_minimal
    parallel_hashmap
    atomic
    pthread
)

enable_testing()
add_test(NAME hashmap_bench_test COMMAND hashmap_bench_test)

# ============================================================================
# Install
# ============================================================================
install(TARGETS hashmap_bench RUNTIME DESTINATION bin)
