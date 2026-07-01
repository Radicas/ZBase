/**
 * @file perf.cpp
 * @author 朱鹏 (Radica Zhu)
 * @date 2026-06-30
 * @version 0.2.0
 * @brief 性能统计实现
 * @details v0.1 直接用 unordered_map 实现 C ABI（YAGNI，未分离 core/c_api 两层，
 *          参考 phase-1-working-file.md §4.28）。线程安全（mutex 保护）。
 *          同名嵌套 start/end 用 ref_count 计数；reset 清空所有累计与未结束打点。
 *          v0.2.0 新增 z_perf_iterate：以回调方式暴露结构化打点数据，
 *          z_perf_dump 改为基于 iterate + 内部格式化辅助函数实现，输出格式不变。
 */

#include "zbase/perf.h"
#include "zbase/error.h"
#include "zbase/internal/c_api_guard.hpp"
#include "platform/time_platform.hpp"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

/// 累计计数器
struct Accumulator {
    uint64_t total_ns = 0;   ///< 累计纳秒
    uint64_t count = 0;      ///< 调用次数
};

/// 进行中的打点（支持同名嵌套）
struct PendingStart {
    uint64_t start_ns = 0;   ///< 起始时间戳
    int      ref_count = 0;  ///< 同名 start 嵌套计数（v0.1 不严格区分线程）
};

std::mutex g_mutex;                                                   ///< 全局互斥锁
std::unordered_map<std::string, Accumulator> g_accum;                 ///< 累计表
std::unordered_map<std::string, PendingStart> g_pending;              ///< 进行中表

}  // namespace

extern "C" {

/// z_perf_dump 的回调：把单条打点格式化为一行文本写入 FILE*
/// @param name 打点名
/// @param total_ns 累计纳秒
/// @param count 调用次数
/// @param user_data 指向 FILE* 的指针（为 NULL 时写 stderr）
/// @note extern "C" + static 保证 C 链接 + 内部链接，可安全传给 z_perf_iterate
static void DumpLineToStream(const char* name, uint64_t total_ns,
                              uint64_t count, void* user_data) {
    FILE* fp = static_cast<FILE*>(user_data);
    if (fp == nullptr) fp = stderr;
    double ms = static_cast<double>(total_ns) / 1000000.0;
    std::fprintf(fp, "  %-30s %12.3f 毫秒  %10llu 次  平均 %.3f 毫秒\n",
                 name, ms,
                 static_cast<unsigned long long>(count),
                 count ? ms / static_cast<double>(count) : 0.0);
}

int z_perf_start(const char* name) {
    ZBASE_C_API_BEGIN
    if (name == nullptr) return Z_ERR_INVALID_ARG;
    std::lock_guard<std::mutex> lock(g_mutex);
    auto& p = g_pending[name];
    if (p.ref_count == 0) {
        p.start_ns = zbase::platform::MonoNowNs();
    }
    ++p.ref_count;
    return Z_OK;
    ZBASE_C_API_END(Z_ERR_UNKNOWN)
}

int z_perf_end(const char* name) {
    ZBASE_C_API_BEGIN
    if (name == nullptr) return Z_ERR_INVALID_ARG;
    uint64_t now = zbase::platform::MonoNowNs();
    std::lock_guard<std::mutex> lock(g_mutex);
    auto it = g_pending.find(name);
    if (it == g_pending.end() || it->second.ref_count == 0) {
        return Z_ERR_NOT_FOUND;
    }
    PendingStart& p = it->second;
    uint64_t elapsed = now - p.start_ns;
    --p.ref_count;
    if (p.ref_count == 0) {
        g_pending.erase(it);
    }
    auto& a = g_accum[name];
    a.total_ns += elapsed;
    a.count += 1;
    return Z_OK;
    ZBASE_C_API_END(Z_ERR_UNKNOWN)
}

void z_perf_dump(void) {
    z_init_console();  // 确保控制台为 UTF-8，避免中文乱码
    std::fprintf(stderr, "==== zbase 性能统计 dump ====\n");
    z_perf_iterate(&DumpLineToStream, stderr);
    std::fprintf(stderr, "==============================\n");
}

void z_perf_iterate(z_perf_visit_fn visit, void* user_data) {
    if (visit == nullptr) return;
    try {
        std::lock_guard<std::mutex> lock(g_mutex);
        // 按总耗时降序排列
        std::vector<std::pair<std::string, Accumulator>> v(g_accum.begin(), g_accum.end());
        std::sort(v.begin(), v.end(),
            [](const auto& a, const auto& b) { return a.second.total_ns > b.second.total_ns; });
        for (const auto& [name, acc] : v) {
            visit(name.c_str(), acc.total_ns, acc.count, user_data);
        }
    } catch (...) {
        // 静默吞异常，防止 C++ 异常穿越 C ABI 边界（UB）
    }
}

void z_perf_reset(void) {
    std::lock_guard<std::mutex> lock(g_mutex);
    g_accum.clear();
    g_pending.clear();
}

}  // extern "C"
