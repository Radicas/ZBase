/**
 * @file bench_string.cpp
 * @author 朱鹏 (Radica Zhu)
 * @date 2026-06-30
 * @version 0.2.0
 * @brief string 模块性能基准：split / join / replace / trim
 */

#include <benchmark/benchmark.h>

#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "zbase/string.h"

namespace {

/// 生成长度为 n 的随机 ASCII 字符串（含分隔符）
/// @param n 长度
/// @param sep 分隔符（每 8 字符插入一个）
/// @return std::string
std::string MakeInput(size_t n, char sep) {
    std::string s;
    s.reserve(n);
    for (size_t i = 0; i < n; ++i) {
        if (i > 0 && i % 8 == 0) s.push_back(sep);
        s.push_back(static_cast<char>('a' + (i % 26)));
    }
    return s;
}

}  // namespace

static void Bench_String_Split(benchmark::State& state) {
    std::string input = MakeInput(state.range(0), ',');
    for (auto _ : state) {
        char** arr = nullptr;
        size_t count = 0;
        z_string_split(input.data(), input.size(), ",", &arr, &count);
        z_string_free_array(arr, count);
        benchmark::DoNotOptimize(arr);
    }
    state.SetBytesProcessed(state.iterations() * static_cast<int64_t>(input.size()));
}
BENCHMARK(Bench_String_Split)->Arg(128)->Arg(1024)->Arg(8192);

static void Bench_String_Join(benchmark::State& state) {
    // 准备一个数组：必须先 reserve 容量，避免 push_back 触发 vector 重分配
    // （重分配会使之前 storage[i] 移动到新地址，items 中保存的 c_str() 指针全部悬挂）
    const int n = static_cast<int>(state.range(0));
    std::vector<std::string> storage;
    storage.reserve(n);
    std::vector<char*> items;
    items.reserve(n);
    size_t total = 0;
    for (int i = 0; i < n; ++i) {
        std::string s = "item" + std::to_string(i);
        total += s.size() + 1;
        storage.push_back(std::move(s));
        items.push_back(const_cast<char*>(storage.back().c_str()));
    }
    for (auto _ : state) {
        char* out = nullptr;
        z_string_join(items.data(), items.size(), "|", &out);
        z_string_free(out);
        benchmark::DoNotOptimize(out);
    }
    state.SetBytesProcessed(state.iterations() * static_cast<int64_t>(total));
}
BENCHMARK(Bench_String_Join)->Arg(8)->Arg(64)->Arg(256);

static void Bench_String_Replace(benchmark::State& state) {
    std::string input = MakeInput(state.range(0), 'x');
    for (auto _ : state) {
        char* out = nullptr;
        z_string_replace(input.c_str(), "abc", "XYZ", &out);
        z_string_free(out);
        benchmark::DoNotOptimize(out);
    }
    state.SetBytesProcessed(state.iterations() * static_cast<int64_t>(input.size()));
}
BENCHMARK(Bench_String_Replace)->Arg(128)->Arg(1024)->Arg(8192);

static void Bench_String_Trim(benchmark::State& state) {
    std::string input(state.range(0), ' ');
    input += "hello";
    input.append(state.range(0), ' ');
    for (auto _ : state) {
        char* out = nullptr;
        z_string_trim(input.c_str(), &out);
        z_string_free(out);
        benchmark::DoNotOptimize(out);
    }
    state.SetBytesProcessed(state.iterations() * static_cast<int64_t>(input.size()));
}
BENCHMARK(Bench_String_Trim)->Arg(32)->Arg(256)->Arg(1024);
