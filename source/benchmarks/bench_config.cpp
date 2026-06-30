/**
 * @file bench_config.cpp
 * @author 朱鹏 (Radica Zhu)
 * @date 2026-06-30
 * @version 0.2.0
 * @brief config 模块性能基准：load + get
 */

#include <benchmark/benchmark.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#include "zbase/config.h"
#include "zbase/error.h"
#include "zbase/file.h"

namespace {

const char* kConfigPath = "zbase_bench_config.ini";

/// 生成包含 n 节、每节 m key 的 INI 文件
/// @param sections 节数
/// @param keys_per_section 每节 key 数
/// @return 文件内容
std::string MakeConfig(int sections, int keys_per_section) {
    std::string s;
    s.reserve(static_cast<size_t>(sections) * keys_per_section * 30);
    for (int i = 0; i < sections; ++i) {
        s += "[section";
        s += std::to_string(i);
        s += "]\n";
        for (int j = 0; j < keys_per_section; ++j) {
            s += "key";
            s += std::to_string(j);
            s += " = value";
            s += std::to_string(j);
            s += "\n";
        }
    }
    return s;
}

}  // namespace

static void Bench_Config_Load(benchmark::State& state) {
    std::string content = MakeConfig(state.range(0), state.range(1));
    if (z_file_write_text(kConfigPath, content.data(), content.size()) != Z_OK) {
        state.SkipWithError("写文件失败");
        return;
    }
    for (auto _ : state) {
        z_config_t* cfg = nullptr;
        if (z_config_load(kConfigPath, &cfg) != Z_OK) {
            state.SkipWithError("load 失败");
            break;
        }
        z_config_free(cfg);
        benchmark::DoNotOptimize(cfg);
    }
    state.SetBytesProcessed(state.iterations() * static_cast<int64_t>(content.size()));
    std::remove(kConfigPath);
}
BENCHMARK(Bench_Config_Load)->Args({10, 10})->Args({100, 100})->Args({500, 50});

static void Bench_Config_Get(benchmark::State& state) {
    std::string content = MakeConfig(10, 10);
    if (z_file_write_text(kConfigPath, content.data(), content.size()) != Z_OK) {
        state.SkipWithError("写文件失败");
        return;
    }
    z_config_t* cfg = nullptr;
    if (z_config_load(kConfigPath, &cfg) != Z_OK) {
        state.SkipWithError("load 失败");
        std::remove(kConfigPath);
        return;
    }
    for (auto _ : state) {
        const char* v = z_config_get(cfg, "section5", "key5", "default");
        benchmark::DoNotOptimize(v);
    }
    z_config_free(cfg);
    std::remove(kConfigPath);
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(Bench_Config_Get);
