/**
 * @file perf.cpp
 * @author 朱鹏 (Radica Zhu)
 * @date 2026-06-30
 * @version 0.1.0
 * @brief 性能统计实现
 * @details v0.1 直接用 unordered_map 实现 C ABI（YAGNI，未分离 core/c_api 两层，
 *          参考 phase-1-working-file.md §4.28）。线程安全（mutex 保护）。
 *          同名嵌套 start/end 用 ref_count 计数；reset 清空所有累计与未结束打点。
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
    std::lock_guard<std::mutex> lock(g_mutex);
    // 按总耗时降序排列
    std::vector<std::pair<std::string, Accumulator>> v(g_accum.begin(), g_accum.end());
    std::sort(v.begin(), v.end(),
        [](const auto& a, const auto& b) { return a.second.total_ns > b.second.total_ns; });
    std::fprintf(stderr, "==== zbase 性能统计 dump ====\n");
    for (const auto& [name, acc] : v) {
        double ms = static_cast<double>(acc.total_ns) / 1000000.0;
        std::fprintf(stderr, "  %-30s %12.3f 毫秒  %10llu 次  平均 %.3f 毫秒\n",
                      name.c_str(), ms,
                      static_cast<unsigned long long>(acc.count),
                      acc.count ? ms / static_cast<double>(acc.count) : 0.0);
    }
    std::fprintf(stderr, "==============================\n");
}

void z_perf_reset(void) {
    std::lock_guard<std::mutex> lock(g_mutex);
    g_accum.clear();
    g_pending.clear();
}

}  // extern "C"
