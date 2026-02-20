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
| **`CLHT-Str-Final`** | Custom | string only | ✅ | **Tag+SIMD, single-pass, best overall** |
| `CLHT-Str-Tagged` | Custom | string only | ✅ | Tag+SIMD optimized |
| `CLHT-Str-Inline` | Custom | string only | ✅ | Inline 16B, fast insert |
| `CLHT-Str-Ptr` | Custom | string only | ✅ | Hash+Pointer, flexible allocator |
| `CLHT-Str-Pooled` | Custom | string only | ✅ | Key pool, cache-friendly |

## CLHT String Key Implementations

Five specialized string key implementations based on CLHT architecture:

| Implementation | Description | Pros | Cons |
|----------------|-------------|------|------|
| **CLHT-Str-Final** | Single-pass + SIMD, 128B cache-line | Best insert & query | Moderate complexity |
| CLHT-Str-Tagged | Tag+SIMD filtering, 128B cache-line | Good query performance | Two-pass insert |
| CLHT-Str-Inline | Inline 16-byte storage | Fast insert, no alloc | Limited key length |
| CLHT-Str-Ptr | Hash+Pointer storage | Flexible allocator | Extra pointer dereference |
| CLHT-Str-Pooled | Key pool allocator | Cache-friendly | Pool management overhead |

### CLHT-Str-Final Optimizations

Key optimizations for superior performance:

1. **Single-Pass Traversal**: Merge existence check + empty slot search
2. **SIMD Tag Matching**: Single instruction compares 4 tags
3. **SIMD Empty Search**: Fast empty slot detection
4. **Early Exit**: `outbound_overflow_count` avoids unnecessary traversal
5. **Cache-line Aligned**: 128-byte bucket layout

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
| **libfork** | https://bgithub.xyz/ConorWilliams/libfork |

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
- `hashmap_bench_test` - 基础单元测试
- `clht_string_test` - CLHT 字符串键单元测试
- `clht_benchmark_test` - CLHT 字符串键性能测试
- `clht_int_test` - CLHT 整数键单元测试
- `clht_int_benchmark_test` - CLHT 整数键性能测试
- `clht_libfork_test` - CLHT libfork 并行单元测试
- `clht_libfork_benchmark` - CLHT libfork 并行性能测试
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

项目包含 5 个测试目标，覆盖三类测试：单元测试、性能测试、比较测试。

### 测试分类

#### 1. 单元测试 (Unit Tests)
验证 CLHT 实现的正确性，覆盖以下场景：
- **基本功能**: 空 table 查询、单键插入/查询、多键插入
- **更新行为**: CLHT 不支持更新已存在的键（设计特性）
- **冲突处理**: 强制哈希冲突场景下的正确性
- **键范围**: 短键、长键、边界键（空字符串、超长字符串）
- **边界条件**: 最小/最大容量、最大值、溢出桶处理
- **压力测试**: 大量键、随机顺序插入、高负载因子
- **数据一致性**: 键不被修改、相似键区分
- **删除操作**: 删除、删除后重新插入
- **内存管理**: 多次创建/销毁、溢出桶处理

#### 2. 性能测试 (Performance Tests)
使用 Catch2 benchmark 框架测量吞吐量：
- **Insert 性能**: 插入操作的耗时和吞吐量
- **Lookup 性能**: 查询操作的耗时和吞吐量
- **负载因子影响**: 25%、50%、75%、90% 负载因子下的性能
- **键长度影响**: 8B、32B、128B、512B 键的性能
- **混合操作**: 80% 查询 + 20% 插入场景
- **规模测试**: 1K、10K、100K 元素规模
- **吞吐量计算**: 精确计算 ops/sec

#### 3. 比较测试 (Comparison Tests)
CLHT 与主流 hashmap 实现的性能对比：
- `std::unordered_map` - STL 标准实现
- `absl::flat_hash_map` - Google Abseil Swiss Table
- `folly::F14FastMap` - Facebook F14 SIMD 优化

### 测试目标

| 测试目标 | 类型 | 测试用例数 | 断言数 | 说明 |
|----------|------|-----------|--------|------|
| `hashmap_bench_test` | 单元测试 | 19 | 55 | 基础 hashmap 单元测试 |
| `clht_string_test` | 单元测试 | 62 | 87405 | CLHT 字符串键单元测试 |
| `clht_int_test` | 单元测试 | 25 | - | CLHT 整数键单元测试 |
| `clht_benchmark_test` | 性能测试 | 10+ | - | CLHT 字符串键性能测试（含比较） |
| `clht_int_benchmark_test` | 性能测试 | 10+ | - | CLHT 整数键性能测试（含比较） |
| `clht_libfork_test` | 单元测试 | 18+ | - | CLHT libfork 并行单元测试 |
| `clht_libfork_benchmark` | 性能测试 | 10+ | - | CLHT libfork 并行性能测试（含比较） |

### 运行测试

```bash
# 运行所有测试
cd build
ctest --output-on-failure

# 仅运行单元测试
./hashmap_bench_test
./clht_string_test
./clht_int_test

# 仅运行性能测试（需要更多时间）
./clht_benchmark_test --benchmark-samples 10
./clht_int_benchmark_test --benchmark-samples 10
```

### 最近测试结果

```
Test project /home/li/hashmap_bench/build

    Start 1: hashmap_bench_test
1/7 Test #1: hashmap_bench_test ...............   Passed    0.03 sec
    Start 2: clht_string_test
2/7 Test #2: clht_string_test .................   Passed    0.03 sec
    Start 3: clht_benchmark_test
3/7 Test #3: clht_benchmark_test ..............   Passed   25.80 sec
    Start 4: clht_int_test
4/7 Test #4: clht_int_test ....................   Passed    0.03 sec
    Start 5: clht_int_benchmark_test
5/7 Test #5: clht_int_benchmark_test ..........   Passed   13.23 sec
    Start 6: clht_libfork_test
6/7 Test #6: clht_libfork_test ................   Passed    0.04 sec
    Start 7: clht_libfork_benchmark
7/7 Test #7: clht_libfork_benchmark ...........   Passed   31.21 sec

100% tests passed, 0 tests failed out of 7
Total Test time (real) =  70.38 sec
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
│   ├── libfork/            # Work-stealing parallel scheduler
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
│   ├── clht_string/        # CLHT string key implementations
│   │   ├── clht_str_common.hpp    # Common utilities (hash, SIMD)
│   │   ├── clht_str_ptr.hpp       # Hash+Pointer storage
│   │   ├── clht_str_inline.hpp    # Inline 16-byte storage
│   │   ├── clht_str_pooled.hpp    # Key pool allocator
│   │   ├── clht_str_tagged.hpp    # Tag+SIMD optimized
│   │   ├── clht_str_final.hpp     # Single-pass optimized (best)
│   │   └── clht_str_bench.cpp     # Standalone benchmark
│   └── clht_libfork/       # CLHT libfork parallel implementations
│       ├── clht_libfork_str.hpp   # Parallel string key operations
│       └── clht_libfork_int.hpp   # Parallel integer key operations
└── test/
    ├── hashmap_bench_test.cpp      # 基础 hashmap 单元测试
    ├── clht_string_test.cpp        # CLHT 字符串键单元测试 (62 cases)
    ├── clht_benchmark_test.cpp     # CLHT 字符串键性能测试
    ├── clht_int_test.cpp           # CLHT 整数键单元测试 (25 cases)
    ├── clht_int_benchmark_test.cpp # CLHT 整数键性能测试
    ├── clht_libfork_test.cpp       # CLHT libfork 并行单元测试
    └── clht_libfork_benchmark.cpp  # CLHT libfork 并行性能测试
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
| **CLHT-Str-Final** | 0.00515 | 0.00220 | **51.0** | **119.1** | **Single-pass, +33% vs F14** |
| CLHT-Str-Inline | 0.00511 | 0.00264 | 51.3 | 99.3 | Inline 16B |
| folly::F14FastMap | 0.00682 | 0.00229 | 38.4 | 114.5 | SIMD F14 hash |
| CLHT-Str-Pooled | 0.00923 | 0.00282 | 28.4 | 93.0 | Key pool |
| CLHT-Str-Ptr | 0.00775 | 0.00371 | 33.8 | 70.7 | Hash+Pointer |
| CLHT-Str-Tagged | 0.01145 | 0.00241 | 22.9 | 108.9 | Tag+SIMD |
| absl::node_hash_map | 0.01161 | 0.00287 | 22.6 | 91.4 | Swiss table |
| phmap::flat_hash_map | 0.00836 | 0.00468 | 31.4 | 56.0 | Abseil-style |
| std::unordered_map | 0.03530 | 0.01306 | 7.4 | 20.1 | STL |

### Key Findings

1. **Best Overall for String Keys**: `CLHT-Str-Final` with single-pass + SIMD optimization
   - Insert: 51.0 Mops/s (+33% vs folly::F14FastMap)
   - Query: 119.1 Mops/s (+4% vs folly::F14FastMap)
2. **Best for Integer Keys**: `google::dense_hash_map` (non-concurrent), `CLHT-LB/LF` (concurrent)
3. **Single-Pass Optimization**: Provides **10%+** insert improvement over two-pass approach
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

## Parallel Operations with libfork

CLHT 现已集成 [libfork](https://github.com/ConorWilliams/libfork) 并行调度器，提供高效的批量并行操作。

### libfork 特性

- **Lock-free + Wait-free 任务调度**
- **基于 C++20 协程**，零分配 Cactus-Stack
- **NUMA-aware Work-Stealing 调度器**
- **任务开销极低**：相比函数调用仅 ~10x 开销
- **性能优势**：相比 TBB 快 7.5x，相比 OpenMP 快 24x

### 并行批量操作 API

```cpp
#include "clht_libfork_str.hpp"
#include "clht_libfork_int.hpp"

// 创建并行 CLHT 实例
clht_libfork::ParallelClhtStr ht_str(100000);  // 自动使用所有核心
clht_libfork::ParallelClhtInt ht_int(100000, 8); // 指定 8 线程

// 并行批量插入
std::vector<std::string> keys = {...};
std::vector<uintptr_t> values = {...};
ht_str.batch_insert(keys, values);

// 并行批量查询
std::vector<uintptr_t> results;
ht_str.batch_lookup(keys, results);

// 并行批量删除
std::vector<bool> removed;
ht_str.batch_remove(keys, removed);

// 混合操作 (80% 查询 + 20% 插入)
ht_str.batch_mixed(keys, values, results, 0.2);
```

### 优化策略

```
╔══════════════════════════════════════════════════════════════════════════════╗
║                           优化策略                                            ║
╠══════════════════════════════════════════════════════════════════════════════╣
║  - Insert: SERIAL (CLHT bucket 锁限制并行扩展)                                ║
║  - Lookup: PARALLEL (无锁读取，近线性扩展)                                    ║
╚══════════════════════════════════════════════════════════════════════════════╝
```

### 大规模性能测试 (N = 2^20 = 1,048,576 元素)

#### String Key Lookup 性能对比

| 实现 | 耗时 (ms) | 吞吐量 (Mops/s) | 并发安全 | 说明 |
|------|-----------|-----------------|----------|------|
| folly::F14FastMap | 11.3 | 92.8 | ❌ | SIMD 优化，最优 |
| **CLHT libfork PARALLEL** | **14.0** | **74.9** | ✅ | **并发安全最优** |
| CLHT-Str-Ptr | 31.0 | 33.8 | ✅ | Hash+Pointer |
| CLHT-Str-Final | 39.3 | 26.7 | ✅ | Tag+SIMD |
| absl::flat_hash_map | 42.5 | 24.7 | ❌ | Swiss Table |
| std::unordered_map | 58.6 | 17.9 | ❌ | STL |

#### String Key Insert 性能对比

| 实现 | 耗时 (ms) | 吞吐量 (Mops/s) | 并发安全 | 说明 |
|------|-----------|-----------------|----------|------|
| folly::F14FastMap | 31.5 | 33.3 | ❌ | 最优 |
| CLHT-Str-Inline | 31.2 | 33.6 | ✅ | **并发安全最优** |
| CLHT-Str-Ptr | 54.7 | 19.2 | ✅ | Hash+Pointer |
| CLHT-Str-Final | 71.5 | 14.7 | ✅ | Tag+SIMD |
| absl::flat_hash_map | 96.6 | 10.9 | ❌ | - |
| std::unordered_map | 131.9 | 7.9 | ❌ | - |

#### Integer Key Lookup 性能对比

| 实现 | 耗时 (ms) | 吞吐量 (Mops/s) | 并发安全 | 说明 |
|------|-----------|-----------------|----------|------|
| CLHT-LB | 2.09 | 502 | ✅ | Lock-Based |
| CLHT-LF | 2.17 | 483 | ✅ | Lock-Free |
| google::dense_hash_map | 2.30 | 456 | ❌ | 非并发最优 |
| **CLHT libfork PARALLEL (8t)** | **2.4** | **437** | ✅ | **并行版本** |
| std::unordered_map | 2.35 | 446 | ❌ | STL |
| libcuckoo | 8.59 | 122 | ✅ | Cuckoo Hash |
| folly::F14FastMap | 14.4 | 72.8 | ❌ | - |

#### Integer Key Insert 性能对比

| 实现 | 耗时 (ms) | 吞吐量 (Mops/s) | 并发安全 | 说明 |
|------|-----------|-----------------|----------|------|
| google::dense_hash_map | 2.16 | 485 | ❌ | 非并发最优 |
| CLHT-LF | 3.57 | 294 | ✅ | **并发安全最优** |
| CLHT-LB | 3.71 | 283 | ✅ | Lock-Based |
| libcuckoo | 17.7 | 59.2 | ✅ | Cuckoo Hash |
| std::unordered_map | 22.1 | 47.5 | ❌ | - |
| folly::F14FastMap | 27.9 | 37.6 | ❌ | - |

### 关键发现

1. **Integer 并发安全最优**: CLHT-LB/LF 单线程性能已超越 std::unordered_map，且支持并发
2. **String 并发安全最优**: CLHT-Str-Inline Insert 接近 folly::F14FastMap，CLHT libfork Lookup 最优
3. **libfork 并行扩展**: Integer Lookup 在 8 线程下达到 437 Mops/s，展现良好扩展性
4. **性能权衡**: 非并发场景下 folly::F14FastMap 和 google::dense_hash_map 更优

### 使用建议

- **并发场景**: CLHT-LB/LF (Integer) 或 CLHT libfork (批量 Lookup)
- **非并发场景**: folly::F14FastMap 或 google::dense_hash_map
- **批量 Lookup**: libfork 并行版本获得 4x 加速
- **混合工作负载**: `batch_mixed()` 自动采用最优策略

## License

MIT License (see LICENSE file)

## Acknowledgments

- Based on [hash_bench](https://github.com/felix-chern/hash_bench) by Felix Chern
- CLHT by [LPD-EPFL](https://github.com/LPD-EPFL/CLHT)
- Folly F14 by Facebook
- All hash map library authors for their excellent implementations
