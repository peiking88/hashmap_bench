# hashmap_bench

A comprehensive C++ hash map performance benchmark suite that compares multiple hash map implementations under various workloads.

## Features

- **20+ Hash Map Implementations**: Standard library, Abseil, Folly, Google, and specialized libraries
- **CLHT String Key Variants**: Multiple string key implementations with Tag+SIMD optimization
- **Multiple Key Types**: Short strings (6B), medium strings (32B), long strings (256B), and 64-bit integers
- **Consistent Benchmarking**: Unified wrapper interface for fair comparison
- **Unit Tests**: Catch2-based test suite for correctness verification

## Hash Map Implementations

| Implementation | Type | Key Types | Concurrent | Description |
|----------------|------|-----------|------------|-------------|
| `std::unordered_map` | Standard | string, int | ❌ | STL chained hash map |
| `absl::flat_hash_map` | Abseil | string, int | ❌ | Swiss table, flat layout |
| `absl::node_hash_map` | Abseil | string, int | ❌ | Swiss table, node-based |
| `folly::F14FastMap` | Facebook | string, int | ❌ | SIMD-optimized F14 hash |
| `cista::hash_map` | Cista | string, int | ❌ | High-performance, serialization-friendly |
| `boost::flat_map` | Boost | string, int | ❌ | Sorted vector (ordered, not hash) |
| `google::dense_hash_map` | Google | string, int | ❌ | Memory-efficient dense hash |
| `google::sparse_hash_map` | Google | string, int | ❌ | Memory-efficient sparse hash |
| `libcuckoo::cuckoohash_map` | libcuckoo | string, int | ✅ | Lock-free concurrent cuckoo hash |
| `phmap::flat_hash_map` | parallel-hashmap | string, int | ❌ | Abseil-compatible alternative |
| `phmap::parallel_flat_hash_map` | parallel-hashmap | string, int | ⚠️ | Sharded design, optional mutex |
| `rhashmap` | C library | string only | ❌ | Robin Hood hashing |
| `OPIC::robin_hood` | OPIC | int only | ❌ | Open-persistent Robin Hood |
| `CLHT-LB` | EPFL | int only | ✅ | Cache-line hash table (lock-based) |
| `CLHT-LF` | EPFL | int only | ✅ | Cache-line hash table (lock-free) |
| **`CLHT-Str-Tagged`** | Custom | string only | ✅ | **Tag+SIMD optimized, best query** |
| `CLHT-Str-Ptr` | Custom | string only | ✅ | Hash+Pointer, flexible allocator |
| `CLHT-Str-Inline` | Custom | string only | ✅ | Inline 16B, fastest insert |
| `CLHT-Str-Pooled` | Custom | string only | ✅ | Key pool, cache-friendly |
| `CLHT-Str-F14` | Custom | string only | ✅ | F14-style chunk design |

## CLHT String Key Implementations

Four specialized string key implementations based on CLHT architecture:

| Implementation | Description | Pros | Cons |
|----------------|-------------|------|------|
| **CLHT-Str-Tagged** | Tag+SIMD filtering, 128B cache-line | Best query performance | Moderate complexity |
| CLHT-Str-Ptr | Hash+Pointer storage | Flexible allocator | Extra pointer dereference |
| CLHT-Str-Inline | Inline 16-byte storage | Fastest insert, no alloc | Limited key length |
| CLHT-Str-Pooled | Key pool allocator | Cache-friendly | Pool management overhead |

### Tag+SIMD Optimization (CLHT-Str-Tagged)

Key optimizations for superior query performance:

1. **7-bit Tag Filtering**: 1/128 false positive rate
2. **SIMD Parallel Matching**: Single instruction compares 4 tags
3. **Early Exit**: `outbound_overflow_count` avoids unnecessary traversal
4. **Cache-line Aligned**: 128-byte bucket layout

## Dependencies (Git Submodules)

| Library | Repository |
|---------|------------|
| abseil-cpp | https://bgithub.xyz/abseil/abseil-cpp.git |
| Catch2 | https://bgithub.xyz/catchorg/Catch2.git |
| container (Boost) | https://bgithub.xyz/boostorg/container.git |
| folly | https://bgithub.xyz/facebook/folly.git |
| NanoLog | https://bgithub.xyz/PlatformLab/NanoLog.git |
| parallel-hashmap | https://bgithub.xyz/greg7mdp/parallel-hashmap.git |
| cista | https://github.com/felixguendling/cista.git |
| clht | https://github.com/LPD-EPFL/CLHT.git |
| libcuckoo | https://github.com/efficient/libcuckoo.git |
| sparsehash | https://github.com/sparsehash/sparsehash.git |
| opic | https://bgithub.xyz/dryman/opic |
| rhashmap | https://bgithub.xyz/rmind/rhashmap |

## Build

```bash
# Clone with submodules
git clone --recursive https://bgithub.xyz/peiking88/hashmap_bench.git
cd hashmap_bench

# Or clone and update submodules separately
git clone https://bgithub.xyz/peiking88/hashmap_bench.git
cd hashmap_bench
git submodule update --init --recursive

# Build
mkdir build && cd build
cmake ..
make -j$(nproc)
```

### Build Targets

- `hashmap_bench` - Main benchmark executable
- `hashmap_bench_test` - Unit test executable
- `clht_str_bench` - CLHT string key standalone benchmark

## Usage

```bash
# Run with default settings (2^20 = 1M short string keys)
./hashmap_bench

# Run with specific number of elements
./hashmap_bench -n 22  # 2^22 = 4M elements

# Run with specific key type
./hashmap_bench -k int           # Integer keys
./hashmap_bench -k mid_string    # 32-byte strings
./hashmap_bench -k long_string   # 256-byte strings

# Run all key types
./hashmap_bench -a

# Multiple repetitions
./hashmap_bench -n 20 -k int -r 3

# Show help
./hashmap_bench -h
```

### Command Line Options

| Option | Description | Default |
|--------|-------------|---------|
| `-n POWER` | Number of elements as power of 2 | 20 (1M) |
| `-k KEYTYPE` | Key type: `short_string`, `mid_string`, `long_string`, `int` | `short_string` |
| `-a` | Run all key types | - |
| `-r N` | Number of repetitions | 1 |
| `-p SEC` | Pause seconds between runs | 0 |
| `-h` | Show help | - |

## Run Tests

```bash
./hashmap_bench_test

# Or with CTest
cd build
ctest --output-on-failure
```

## Project Structure

```
hashmap_bench/
├── CMakeLists.txt          # Build configuration
├── README.md               # This file
├── external/               # Git submodules (dependencies)
│   ├── abseil-cpp/
│   ├── Catch2/
│   ├── cista/
│   ├── clht/
│   ├── container/
│   ├── folly/
│   ├── libcuckoo/
│   ├── NanoLog/
│   ├── opic/
│   ├── parallel-hashmap/
│   ├── rhashmap/
│   └── sparsehash/
├── src/
│   ├── benchmark.cpp       # Benchmark utilities
│   ├── benchmark.hpp       # Timer, key generation
│   ├── hash_maps.hpp       # Hash map wrappers
│   ├── hashmap_bench.cpp   # Main benchmark program
│   └── clht_string/        # CLHT string key implementations
│       ├── clht_str_common.hpp    # Common utilities (hash, SIMD)
│       ├── clht_str_ptr.hpp       # Hash+Pointer storage
│       ├── clht_str_inline.hpp    # Inline 16-byte storage
│       ├── clht_str_pooled.hpp    # Key pool allocator
│       ├── clht_str_tagged.hpp    # Tag+SIMD optimized (best query)
│       └── clht_str_bench.cpp     # Standalone benchmark
└── test/
    └── hashmap_bench_test.cpp  # Catch2 unit tests
```

## Benchmark Results

Test environment: Linux, C++20, O3 optimization, single-threaded execution

### Integer Keys (int64) - 1M Elements

| Implementation | Insert (s) | Query (s) | Insert Mops/s | Query Mops/s | Concurrent |
|----------------|------------|-----------|---------------|--------------|------------|
| **google::dense_hash_map** | 0.002230 | 0.002013 | **470.4** | **521.1** | ❌ |
| **CLHT-LB** | 0.003944 | 0.002244 | **266.0** | **467.3** | ✅ Lock-Based |
| **CLHT-LF** | 0.004757 | 0.003306 | **220.3** | **317.2** | ✅ Lock-Free |
| boost::flat_map | 0.013117 | 0.024905 | 80.0 | 42.1 | ❌ |
| google::sparse_hash_map | 0.024970 | 0.009661 | 42.0 | 108.6 | ❌ |
| std::unordered_map | 0.032244 | 0.002028 | 32.5 | 517.2 | ❌ |
| phmap::parallel_flat_hash_map | 0.026494 | 0.026416 | 39.6 | 39.7 | ⚠️ Optional |
| phmap::flat_hash_map | 0.038006 | 0.015987 | 27.6 | 65.6 | ❌ |
| folly::F14FastMap | 0.043801 | 0.028675 | 23.9 | 36.6 | ❌ |
| libcuckoo::cuckoohash_map | 0.054017 | 0.008153 | 19.4 | 128.7 | ✅ |
| absl::node_hash_map | 0.079278 | 0.028938 | 13.2 | 36.2 | ❌ |
| absl::flat_hash_map | 0.081118 | 0.032465 | 12.9 | 32.3 | ❌ |
| cista::hash_map | 0.120215 | 0.010070 | 8.7 | 104.1 | ❌ |
| OPIC::robin_hood | 0.122422 | 0.063377 | 8.6 | 16.6 | ❌ |

### String Keys (6-16 bytes) - 262K Elements

| Implementation | Insert (s) | Query (s) | Insert Mops/s | Query Mops/s | Notes |
|----------------|------------|-----------|---------------|--------------|-------|
| **CLHT-Str-Tagged** | 0.00642 | 0.00218 | 40.9 | **120.0** | **Tag+SIMD, +28% vs F14** |
| folly::F14FastMap | 0.00489 | 0.00280 | 53.7 | 93.5 | SIMD F14 hash |
| CLHT-Str-Inline | 0.00586 | 0.00390 | 44.7 | 67.2 | Inline 16B |
| CLHT-Str-F14 | 0.00762 | 0.00374 | 34.4 | 70.1 | F14-style chunk |
| CLHT-Str-Ptr | 0.00854 | 0.00462 | 30.7 | 56.7 | Hash+Pointer |
| CLHT-Str-Pooled | 0.00928 | 0.00486 | 28.3 | 53.9 | Key pool |
| absl::node_hash_map | 0.01012 | 0.00456 | 25.9 | 57.4 | Swiss table |
| phmap::flat_hash_map | 0.01234 | 0.00568 | 21.2 | 46.1 | Abseil-style |
| std::unordered_map | 0.01856 | 0.00692 | 14.1 | 37.9 | STL |

### Key Findings

1. **Best Query for String Keys**: `CLHT-Str-Tagged` with Tag+SIMD optimization (120 Mops/s)
2. **Best for Integer Keys**: `google::dense_hash_map` (non-concurrent), `CLHT-LB/LF` (concurrent)
3. **Tag+SIMD Optimization**: Provides **2.1x** query improvement over baseline CLHT-Str-Ptr
4. **Concurrent Performance**: `CLHT-LF` (lock-free) achieves near non-concurrent performance

## Concurrency Support Summary

| Implementation | Thread Safe | Mechanism |
|----------------|-------------|-----------|
| CLHT-Str-* (all) | ✅ Yes | Lock-based bucket locks |
| CLHT-LB | ✅ Yes | Lock-based |
| CLHT-LF | ✅ Yes | Lock-free |
| libcuckoo::cuckoohash_map | ✅ Yes | Fine-grained locks |
| phmap::parallel_flat_hash_map | ⚠️ Optional | Submap-level mutex (template param) |
| All others | ❌ No | External sync required |

## License

MIT License (see LICENSE file)

## Acknowledgments

- Based on [hash_bench](https://github.com/felix-chern/hash_bench) by Felix Chern
- CLHT by [LPD-EPFL](https://github.com/LPD-EPFL/CLHT)
- Folly F14 by Facebook
- All hash map library authors for their excellent implementations
