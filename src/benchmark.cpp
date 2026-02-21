#include "benchmark.hpp"
#include <algorithm>
#include <iomanip>
#include <iostream>

namespace hashmap_bench {

uint64_t side_effect = 0;

void generate_short_keys(std::vector<std::string>& keys, int num_power) {
    uint64_t num = 1ULL << num_power;
    keys.clear();
    keys.reserve(num);
    
    int i_bound = 1 << (num_power - 12);
    std::string uuid = "!!!!!!";
    uint64_t counter = 0;
    
    for (int i = 0; i < i_bound; i++) {
        for (int j = 2, val = counter >> 12; j < 6; j++, val >>= 6) {
            uuid[j] = 0x21 + (val & 0x3F);
        }
        for (int j = 0; j < 64; j++) {
            uuid[1] = 0x21 + j;
            for (int k = 0; k < 64; k++) {
                uuid[0] = 0x21 + k;
                counter++;
                keys.push_back(uuid);
            }
        }
    }
}

void generate_mid_keys(std::vector<std::string>& keys, int num_power) {
    uint64_t num = 1ULL << num_power;
    keys.clear();
    keys.reserve(num);
    
    int i_bound = 1 << (num_power - 12);
    std::string uuid = "!!!!!!--!!!!!!--!!!!!!--!!!!!!--";
    uint64_t counter = 0;
    
    for (int i = 0; i < i_bound; i++) {
        for (int j = 2, val = counter >> 12; j < 6; j++, val >>= 6) {
            uuid[j] = 0x21 + (val & 0x3F);
            uuid[j + 8] = 0x21 + (val & 0x3F);
            uuid[j + 16] = 0x21 + (val & 0x3F);
            uuid[j + 24] = 0x21 + (val & 0x3F);
        }
        for (int j = 0; j < 64; j++) {
            uuid[1] = 0x21 + j;
            uuid[1 + 8] = 0x21 + j;
            uuid[1 + 16] = 0x21 + j;
            uuid[1 + 24] = 0x21 + j;
            for (int k = 0; k < 64; k++) {
                uuid[0] = 0x21 + k;
                uuid[0 + 8] = 0x21 + k;
                uuid[0 + 16] = 0x21 + k;
                uuid[0 + 24] = 0x21 + k;
                counter++;
                keys.push_back(uuid);
            }
        }
    }
}

void generate_long_keys(std::vector<std::string>& keys, int num_power) {
    uint64_t num = 1ULL << num_power;
    keys.clear();
    keys.reserve(num);
    
    int i_bound = 1 << (num_power - 12);
    std::string uuid =
        "!!!!!!--!!!!!!--!!!!!!--!!!!!!--"
        "!!!!!!--!!!!!!--!!!!!!--!!!!!!--"
        "!!!!!!--!!!!!!--!!!!!!--!!!!!!--"
        "!!!!!!--!!!!!!--!!!!!!--!!!!!!--"
        "!!!!!!--!!!!!!--!!!!!!--!!!!!!--"
        "!!!!!!--!!!!!!--!!!!!!--!!!!!!--"
        "!!!!!!--!!!!!!--!!!!!!--!!!!!!--"
        "!!!!!!--!!!!!!--!!!!!!--!!!!!!--";
    uint64_t counter = 0;
    
    for (int i = 0; i < i_bound; i++) {
        for (int j = 2, val = counter >> 12; j < 6; j++, val >>= 6) {
            for (int k = 0; k < 256; k += 8) {
                uuid[j + k] = 0x21 + (val & 0x3F);
            }
        }
        for (int j = 0; j < 64; j++) {
            for (int h = 0; h < 256; h += 8) {
                uuid[h + 1] = 0x21 + j;
            }
            for (int k = 0; k < 64; k++) {
                for (int h = 0; h < 256; h += 8) {
                    uuid[h] = 0x21 + k;
                }
                counter++;
                keys.push_back(uuid);
            }
        }
    }
}

void generate_int_keys(std::vector<uint64_t>& keys, int num_power) {
    uint64_t num = 1ULL << num_power;
    keys.clear();
    keys.reserve(num);
    
    for (uint64_t i = 0; i < num; i++) {
        keys.push_back(i);
    }
}

uint64_t tomas_wang_int32_hash(uint32_t key) {
    uint64_t k = key;
    k += ~(k << 15);
    k ^= (k >> 10);
    k += (k << 3);
    k ^= (k >> 6);
    k += ~(k << 11);
    k ^= (k >> 16);
    return k;
}

uint64_t tomas_wang_int64_hash(uint64_t key) {
    key = (~key) + (key << 21);
    key = key ^ (key >> 24);
    key = (key + (key << 3)) + (key << 8);
    key = key ^ (key >> 14);
    key = (key + (key << 2)) + (key << 4);
    key = key ^ (key >> 28);
    key = key + (key << 31);
    return key;
}

void print_result(const BenchmarkResult& result) {
    double insert_mops = result.num_elements / result.insert_time_sec / 1000000.0;
    double query_mops = result.num_elements / result.query_time_sec / 1000000.0;
    
    std::cout << std::left << std::setw(28) << result.impl_name
              << std::fixed << std::setprecision(6) << result.insert_time_sec << "\t"
              << std::setprecision(6) << result.query_time_sec << "\t"
              << std::setprecision(1) << insert_mops << "\t"
              << std::setprecision(1) << query_mops << "\t"
              << result.comments
              << std::endl;
}

void print_results(const std::vector<BenchmarkResult>& results) {
    std::cout << "\n";
    std::cout << std::left 
              << std::setw(28) << "Implementation" << "\t"
              << "Insert (s)\tQuery (s)\tInsert Mops/s\tQuery Mops/s\tComments\n";
    std::cout << std::string(100, '-') << "\n";
    
    for (const auto& result : results) {
        print_result(result);
    }
    std::cout << std::endl;
}

} // namespace hashmap_bench
