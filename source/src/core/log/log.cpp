/**
 * @file log.cpp
 * @author 朱鹏 (Radica Zhu)
 * @date 2026-06-30
 * @version 0.1.0
 * @brief 日志实现（最小版，stderr 输出）
 * @details v0.1 直接用 fprintf + 全局 atomic 级别过滤（YAGNI，未分离 core/c_api 两层，
 *          参考 phase-1-working-file.md §4.28）。线程安全：级别读取用 atomic，
 *          输出走 stderr（POSIX 默认线程安全，Windows 由 CRT 内部锁保护）。
 */

#include "zbase/log.h"
#include "zbase/error.h"
#include "zbase/internal/c_api_guard.hpp"
#include "zbase/time.h"
#include "core/log/console.hpp"

#include <atomic>
#include <cstdarg>
#include <cstdio>
#include <cstring>

namespace {

std::atomic<int> g_min_level{Z_LOG_LEVEL_INFO};  ///< 当前最低输出级别

/// 获取级别名称
/// @param l 日志级别
/// @return 静态字符串
const char* LevelName(z_log_level_t l) {
    switch (l) {
        case Z_LOG_LEVEL_DEBUG: return "DEBUG";
        case Z_LOG_LEVEL_INFO:  return "INFO";
        case Z_LOG_LEVEL_WARN:  return "WARN";
        case Z_LOG_LEVEL_ERROR: return "ERROR";
        default:                return "?";
    }
}

}  // namespace

extern "C" {

int z_log_init(z_log_level_t level) {
    ZBASE_C_API_BEGIN
    zbase::log::InitConsoleUtf8();
    g_min_level.store(static_cast<int>(level), std::memory_order_relaxed);
    return Z_OK;
    ZBASE_C_API_END(Z_ERR_UNKNOWN)
}

void z_log_shutdown(void) {
    zbase::log::RestoreConsole();
}

void z_log_write(z_log_level_t level, const char* file, int line,
                 const char* fmt, ...) {
    if (file == nullptr || fmt == nullptr) return;
    if (static_cast<int>(level) < g_min_level.load(std::memory_order_relaxed)) {
        return;
    }

    char time_buf[32];
    int64_t ms = z_time_now_ms();
    z_time_format_iso(ms, time_buf, sizeof(time_buf));

    // 提取文件 basename（同时支持 / 和 \）
    const char* base = file;
    if (const char* p = std::strrchr(file, '/'))  base = p + 1;
    if (const char* p = std::strrchr(base, '\\')) base = p + 1;

    char user_buf[1024];
    va_list args;
    va_start(args, fmt);
    std::vsnprintf(user_buf, sizeof(user_buf), fmt, args);
    va_end(args);

    std::fprintf(stderr, "[%s] [%s] [%s:%d] %s\n",
                 time_buf, LevelName(level), base, line, user_buf);
    std::fflush(stderr);
}

}  // extern "C"
