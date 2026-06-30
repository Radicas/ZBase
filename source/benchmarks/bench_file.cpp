/**
 * @file bench_file.cpp
 * @author 朱鹏 (Radica Zhu)
 * @date 2026-06-30
 * @version 0.2.0
 * @brief file 模块性能基准：read_text / write_text
 */

#include <benchmark/benchmark.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#include "zbase/error.h"
#include "zbase/file.h"

namespace {

const char* kBenchPath = "zbase_bench_file.tmp";

/// 生成长度为 n 的字符串
/// @param n 字节数
/// @return std::string
std::string MakeContent(size_t n) {
    std::string s;
    s.reserve(n);
    for (size_t i = 0; i < n; ++i) {
        s.push_back(static_cast<char>('a' + (i % 26)));
    }
    return s;
}

}  // namespace

static void Bench_File_WriteText(benchmark::State& state) {
    std::string content = MakeContent(state.range(0));
    for (auto _ : state) {
        // Windows 下紧密循环写入同一文件时，Defender 偶发锁定目标文件导致
        // MoveFileExW 失败（非库代码 bug）。重试 3 次以跨过瞬时锁。
        int r = Z_OK;
        for (int retry = 0; retry < 3; ++retry) {
            r = z_file_write_text(kBenchPath, content.data(), content.size());
            if (r == Z_OK) break;
        }
        if (r != Z_OK) {
            state.SkipWithError("write 失败（重试 3 次后仍失败）");
            break;
        }
    }
    state.SetBytesProcessed(state.iterations() * static_cast<int64_t>(content.size()));
    std::remove(kBenchPath);
}
BENCHMARK(Bench_File_WriteText)->Arg(1024)->Arg(64 * 1024)->Arg(1024 * 1024);

static void Bench_File_ReadText(benchmark::State& state) {
    std::string content = MakeContent(state.range(0));
    if (z_file_write_text(kBenchPath, content.data(), content.size()) != Z_OK) {
        state.SkipWithError("预写失败");
        return;
    }
    for (auto _ : state) {
        char* buf = nullptr;
        size_t len = 0;
        if (z_file_read_text(kBenchPath, &buf, &len) != Z_OK) {
            state.SkipWithError("read 失败");
            break;
        }
        z_file_free(buf);
        benchmark::DoNotOptimize(buf);
    }
    state.SetBytesProcessed(state.iterations() * static_cast<int64_t>(content.size()));
    std::remove(kBenchPath);
}
BENCHMARK(Bench_File_ReadText)->Arg(1024)->Arg(64 * 1024)->Arg(1024 * 1024);
