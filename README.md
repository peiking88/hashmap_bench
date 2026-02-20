# hashmap_bench

A comprehensive C++ hash map performance benchmark suite that compares multiple hash map implementations under various workloads.

## Features

- **15+ Hash Map Implementations**: Standard library, Abseil, Folly, Google, and specialized libraries
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
│   └── hashmap_bench.cpp   # Main benchmark program
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

### Short String Keys (6 bytes) - 1M Elements

| Implementation | Insert (s) | Query (s) | Insert Mops/s | Query Mops/s |
|----------------|------------|-----------|---------------|--------------|
| **folly::F14FastMap** | 0.067884 | 0.038905 | **15.5** | **27.0** |
| google::dense_hash_map | 0.071771 | 0.065380 | 14.6 | 16.1 |
| phmap::parallel_flat_hash_map | 0.080044 | 0.057708 | 13.1 | 18.2 |
| phmap::flat_hash_map | 0.081439 | 0.048193 | 12.9 | 21.8 |
| absl::node_hash_map | 0.091988 | 0.059196 | 11.4 | 17.7 |
| cista::hash_map | 0.104095 | 0.058336 | 10.1 | 18.0 |
| absl::flat_hash_map | 0.126048 | 0.070023 | 8.3 | 15.0 |
| google::sparse_hash_map | 0.147836 | 0.095696 | 7.1 | 11.0 |
| rhashmap | 0.179633 | 0.070432 | 5.8 | 14.9 |
| libcuckoo::cuckoohash_map | 0.216239 | 0.119444 | 4.8 | 8.8 |

### Key Findings

1. **Best for Integer Keys**: `google::dense_hash_map` (non-concurrent), `CLHT-LB/LF` (concurrent)
2. **Best for String Keys**: `folly::F14FastMap` 
3. **Concurrent Performance**: `CLHT-LF` (lock-free) achieves near non-concurrent performance
4. **Memory Efficient**: `google::sparse_hash_map` trades speed for memory

## Concurrency Support Summary

| Implementation | Thread Safe | Mechanism |
|----------------|-------------|-----------|
| CLHT-LB | ✅ Yes | Lock-based |
| CLHT-LF | ✅ Yes | Lock-free |
| libcuckoo::cuckoohash_map | ✅ Yes | Fine-grained locks |
| phmap::parallel_flat_hash_map | ⚠️ Optional | Submap-level mutex (template param) |
| All others | ❌ No | External sync required |

## License

MIT License (see LICENSE file)

## Acknowledgments

- Based on [hash_bench](https://github.com/felix-chern/hash_bench) by Felix Chern
- All hash map library authors for their excellent implementations
