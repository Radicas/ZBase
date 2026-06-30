/**
 * @file bench_log.cpp
 * @author 朱鹏 (Radica Zhu)
 * @date 2026-06-30
 * @version 0.2.0
 * @brief log 模块性能基准：z_log_write 吞吐
 * @details 不开文件输出（仅 stderr，且 stderr 重定向到 NUL//dev/null），
 *          测纯格式化 + fprintf 路径开销。
 */

#include <benchmark/benchmark.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#include "zbase/error.h"
#include "zbase/log.h"

#ifdef _WIN32
#include <Windows.h>
#endif

namespace {

/// 把 stderr 重定向到空设备，避免 I/O 影响测得的数据
/// @return 原 stderr 句柄（用于恢复）
FILE* RedirectStderrToNull() {
    FILE* orig = stderr;
#ifdef _WIN32
    FILE* dummy = nullptr;
    freopen_s(&dummy, "NUL", "w", stderr);
#else
    stderr = freopen("/dev/null", "w", stderr);
    if (stderr == nullptr) stderr = orig;
#endif
    return orig;
}

}  // namespace

static void Bench_Log_Write_NoFile(benchmark::State& state) {
    z_log_init(Z_LOG_LEVEL_INFO);
    RedirectStderrToNull();
    for (auto _ : state) {
        Z_LOG_INFO("bench line %d padding %s", 1, "xyz");
    }
    z_log_shutdown();
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(Bench_Log_Write_NoFile);

static void Bench_Log_Write_Debug(benchmark::State& state) {
    // DEBUG 级别会触发更多分支（虽然实际不输出，因为没文件）
    z_log_init(Z_LOG_LEVEL_DEBUG);
    RedirectStderrToNull();
    for (auto _ : state) {
        Z_LOG_DEBUG("debug %d", 1);
    }
    z_log_shutdown();
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(Bench_Log_Write_Debug);

static void Bench_Log_Write_ToFile(benchmark::State& state) {
    const char* path = "zbase_bench_log.tmp";
    std::remove(path);
    z_log_init(Z_LOG_LEVEL_INFO);
    RedirectStderrToNull();
    // 不滚动，max_files=1
    if (z_log_file_open(path, 0, 1) != Z_OK) {
        state.SkipWithError("log file 打开失败");
        z_log_shutdown();
        return;
    }
    for (auto _ : state) {
        Z_LOG_INFO("bench line %d padding padding padding padding", 1);
    }
    z_log_file_close();
    z_log_shutdown();
    std::remove(path);
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(Bench_Log_Write_ToFile);
