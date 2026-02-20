# hashmap_bench

A comprehensive C++ hash map performance benchmark suite that compares multiple hash map implementations under various workloads.

## Features

- **15+ Hash Map Implementations**: Standard library, Abseil, Folly, Google, and specialized libraries
- **Multiple Key Types**: Short strings (6B), medium strings (32B), long strings (256B), and 64-bit integers
- **Consistent Benchmarking**: Unified wrapper interface for fair comparison
- **Unit Tests**: Catch2-based test suite for correctness verification

## Hash Map Implementations

| Implementation | Type | Key Types | Description |
|----------------|------|-----------|-------------|
| `std::unordered_map` | Standard | string, int | STL chained hash map |
| `absl::flat_hash_map` | Abseil | string, int | Swiss table, flat layout |
| `absl::node_hash_map` | Abseil | string, int | Swiss table, node-based |
| `folly::F14FastMap` | Facebook | string, int | SIMD-optimized F14 hash |
| `cista::hash_map` | Cista | string, int | High-performance, serialization-friendly |
| `boost::flat_map` | Boost | string, int | Sorted vector (ordered, not hash) |
| `google::dense_hash_map` | Google | string, int | Memory-efficient dense hash |
| `google::sparse_hash_map` | Google | string, int | Memory-efficient sparse hash |
| `libcuckoo::cuckoohash_map` | libcuckoo | string, int | Lock-free concurrent cuckoo hash |
| `phmap::flat_hash_map` | parallel-hashmap | string, int | Abseil-compatible alternative |
| `phmap::parallel_flat_hash_map` | parallel-hashmap | string, int | Concurrent parallel hash |
| `rhashmap` | C library | string only | Robin Hood hashing |
| `OPIC::robin_hood` | OPIC | int only | Open-persistent Robin Hood |
| `CLHT-LB` | EPFL | int only | Cache-line hash table (lock-based) |
| `CLHT-LF` | EPFL | int only | Cache-line hash table (lock-free) |

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

## License

MIT License (see LICENSE file)

## Acknowledgments

- Based on [hash_bench](https://github.com/felix-chern/hash_bench) by Felix Chern
- All hash map library authors for their excellent implementations
