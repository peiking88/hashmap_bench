# hashmap_bench

一个 C++ 哈希表性能基准测试套件，用统一的接口对多种实现进行插入与查询性能对比。

## 功能特性

- **多实现对比**：标准库、Abseil、Folly、Google sparsehash、libcuckoo、parallel-hashmap、Cista、OPIC、CLHT、rhashmap 等
- **多键类型**：短字符串（6B）、中字符串（32B）、长字符串（256B）、64 位整数
- **可控参数**：元素规模、键类型、重复次数、实现选择、CLHT 容量因子
- **统一封装**：统一的 wrapper 与输出格式，便于公平对比
- **测试覆盖**：Catch2 测试覆盖 key 生成、哈希函数与基础正确性

## 支持的实现

| Implementation | Key Types | 并发安全 | 说明 |
|---|---|---|---|
| `std::unordered_map` | string, int | ❌ | STL 链式哈希表 |
| `absl::flat_hash_map` | string, int | ❌ | SwissTable，扁平布局 |
| `absl::node_hash_map` | string, int | ❌ | SwissTable，节点布局 |
| `folly::F14FastMap` | string, int | ❌ | F14 SIMD 优化 |
| `google::dense_hash_map` | string, int | ❌ | 高密度哈希表 |
| `google::sparse_hash_map` | string, int | ❌ | 稀疏哈希表 |
| `libcuckoo::cuckoohash_map` | string, int | ✅ | 细粒度锁并发哈希 |
| `phmap::flat_hash_map` | string, int | ❌ | parallel-hashmap 扁平实现 |
| `phmap::parallel_flat_hash_map` | string, int | ⚠️ | 可选锁（模板参数） |
| `cista::hash_map` | string, int | ❌ | 轻量高性能实现 |
| `boost::container::flat_map` | string, int | ❌ | 有序向量（非哈希表） |
| `rhashmap` | string | ❌ | C 库 Robin Hood 哈希 |
| `OPIC::robin_hood` | int | ❌ | OPIC Robin Hood 哈希 |
| `CLHT-LB` | int | ✅ | CLHT Lock-Based |
| `CLHT-LF` | int | ✅ | CLHT Lock-Free |

> 说明：基准测试目前为单线程执行，表格中的并发安全表示实现自身的线程安全能力。

## 依赖（子模块）

| Library | Repository |
|---|---|
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

## 项目结构

```
hashmap_bench/
├── CMakeLists.txt
├── README.md
├── external/               # 子模块依赖
├── stubs/                  # 修复/替代头文件
├── src/
│   ├── benchmark.cpp
│   ├── benchmark.hpp
│   ├── hash_maps.hpp
│   └── hashmap_bench.cpp
└── test/
    └── hashmap_bench_test.cpp
```

## 构建

```bash
git clone --recursive https://bgithub.xyz/peiking88/hashmap_bench.git
cd hashmap_bench

cmake -S . -B build
cmake --build build -j
```

### 构建目标

- `hashmap_bench`：主基准测试程序
- `hashmap_test`：Catch2 单元测试

## 运行基准测试

```bash
# 显示帮助
./build/hashmap_bench -h

# 默认模式（-n）：short_string + int
./build/hashmap_bench -n 20

# 指定键类型
./build/hashmap_bench -k short_string
./build/hashmap_bench -k mid_string
./build/hashmap_bench -k long_string
./build/hashmap_bench -k int

# 运行全部键类型 + 全部实现
./build/hashmap_bench -a

# 指定实现（示例）
./build/hashmap_bench -k int -i dense_hash_map

# 重复次数与插入/查询间隔
./build/hashmap_bench -n 20 -r 3 -p 1

# 调整 CLHT 容量因子
./build/hashmap_bench -k int -i CLHT_LB -c 4
```

### 命令行参数

| Option | 说明 | 默认值 |
|---|---|---|
| `-n POWER` | 元素数量为 \(2^{POWER}\) | 20 |
| `-k KEYTYPE` | `short_string` / `mid_string` / `long_string` / `int` | `short_string` |
| `-a` | 运行所有键类型与实现 | - |
| `-i IMPL` | 仅运行指定实现 | - |
| `-r N` | 重复次数 | 1 |
| `-p SEC` | 插入和查询之间暂停秒数 | 0 |
| `-c FACTOR` | CLHT 容量因子 | 4 |
| `-h` | 显示帮助 | - |

### `-i` 可用实现名

```
std_unordered_map
absl_flat_hash_map
absl_node_hash_map
folly_F14FastMap
cista_hash_map
boost_flat_map
dense_hash_map
sparse_hash_map
cuckoohash_map
rhashmap
phmap_flat
phmap_parallel
CLHT_LB
CLHT_LF
OPIC
```

> 提示：部分实现仅支持 `string` 或 `int` 键类型。

## 运行测试

```bash
# 直接运行可执行文件
./build/hashmap_test

# 或使用 CTest
cd build
ctest --output-on-failure
```

## License

MIT License (see LICENSE file)

## Acknowledgments

- Based on [hash_bench](https://github.com/felix-chern/hash_bench) by Felix Chern
- CLHT by [LPD-EPFL](https://github.com/LPD-EPFL/CLHT)
- Folly F14 by Facebook
- All hash map library authors for their excellent implementations
