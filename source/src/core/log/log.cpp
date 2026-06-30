/**
 * @file log.cpp
 * @author 朱鹏 (Radica Zhu)
 * @date 2026-06-30
 * @version 0.2.0
 * @brief 日志实现（v0.2：增加文件输出 + 滚动）
 * @details v0.1 直接用 fprintf + 全局 atomic 级别过滤（YAGNI，未分离 core/c_api 两层，
 *          参考 phase-1-working-file.md §4.28）。线程安全：级别读取用 atomic，
 *          stderr 输出走 CRT 内部锁；文件输出走 g_file_mutex 串行化。
 *          v0.2 新增：z_log_file_open 后，z_log_write 同时写入 stderr 和文件。
 *          单文件超 max_size 字节时滚动：
 *            关闭 → 删除 path.N（最旧）→ path.(N-1) 改名为 path.N → ... →
 *            path 改名为 path.1 → 重新打开空 path。
 *          滚动时 Windows 用 MoveFileExW(MOVEFILE_REPLACE_EXISTING)，
 *          POSIX 用 rename(2)（原子替换）。
 */

#include "zbase/log.h"
#include "zbase/error.h"
#include "zbase/internal/c_api_guard.hpp"
#include "zbase/time.h"
#include "core/log/console.hpp"
#include "platform/file_platform.hpp"

#include <atomic>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <filesystem>
#include <mutex>
#include <string>

#ifdef _WIN32
#include <Windows.h>
#endif

namespace {

std::atomic<int> g_min_level{Z_LOG_LEVEL_INFO};  ///< 当前最低输出级别

/// 日志文件状态（v0.2 新增）
struct FileState {
    std::ofstream stream;       ///< 文件流
    std::string   path;         ///< UTF-8 路径
    uint64_t      current_size = 0;  ///< 当前文件已写字节数
    uint64_t      max_size = 0;      ///< 单文件最大字节数（0 = 不滚动）
    uint32_t      max_files = 0;     ///< 保留历史文件数
    bool          enabled = false;   ///< 是否已启用文件输出
};

FileState   g_file;             ///< 全局文件状态
std::mutex  g_file_mutex;       ///< 保护 g_file 的所有访问

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

/// 打开 ofstream（跨平台 UTF-8 路径处理）
/// @param path UTF-8 路径
/// @param mode 打开模式
/// @return true 成功；false 失败（路径编码错误或文件不可创建）
bool OpenOfstreamUtf8(const std::string& path, std::ios::openmode mode) {
#ifdef _WIN32
    std::wstring wpath;
    if (zbase::platform::NormalizePath(path, wpath) != Z_OK) return false;
    std::filesystem::path fs_path{wpath};
#else
    std::filesystem::path fs_path{path};
#endif
    g_file.stream.open(fs_path, mode);
    return g_file.stream.is_open();
}

/// UTF-8 路径转 std::filesystem::path（Windows 走 UTF-16，POSIX 走 UTF-8）
/// @param path UTF-8 路径
/// @return std::filesystem::path；编码失败时返回空 path
std::filesystem::path ToFsPath(const std::string& path) {
#ifdef _WIN32
    std::wstring wpath;
    if (zbase::platform::NormalizePath(path, wpath) != Z_OK) return {};
    return std::filesystem::path{wpath};
#else
    return std::filesystem::path{path};
#endif
}

/// 跨平台 UTF-8 路径重命名（覆盖目标）
/// @param src 源路径
/// @param dst 目标路径
/// @return true 成功；false 失败
bool RenameFileUtf8(const std::string& src, const std::string& dst) {
#ifdef _WIN32
    std::wstring wsrc, wdst;
    if (zbase::platform::NormalizePath(src, wsrc) != Z_OK) return false;
    if (zbase::platform::NormalizePath(dst, wdst) != Z_OK) return false;
    return MoveFileExW(wsrc.c_str(), wdst.c_str(),
                       MOVEFILE_REPLACE_EXISTING) != 0;
#else
    return std::rename(src.c_str(), dst.c_str()) == 0;
#endif
}

/// 跨平台 UTF-8 路径删除
/// @param path 路径
/// @return true 成功或文件不存在；false 失败
bool RemoveFileUtf8(const std::string& path) {
#ifdef _WIN32
    std::wstring wpath;
    if (zbase::platform::NormalizePath(path, wpath) != Z_OK) return false;
    return DeleteFileW(wpath.c_str()) != 0;
#else
    return std::remove(path.c_str()) == 0;
#endif
}

/// 触发滚动：关闭当前文件 → 倒序重命名 → 重新打开空文件
/// 调用前必须持有 g_file_mutex
void RollFiles() {
    g_file.stream.close();

    const uint32_t N = g_file.max_files > 0 ? g_file.max_files : 1;

    // 1. 删除最旧的 path.N（如果存在）
    if (N >= 1) {
        std::string oldest = g_file.path + "." + std::to_string(N);
        RemoveFileUtf8(oldest);  // 不存在时静默失败
    }

    // 2. 倒序重命名 path.(N-1) → path.N, ..., path.1 → path.2
    //    每一步目标在上一轮已被清空（要么原本不存在，要么已被移走）
    for (int32_t i = static_cast<int32_t>(N) - 1; i >= 1; --i) {
        std::string src = g_file.path + "." + std::to_string(i);
        std::string dst = g_file.path + "." + std::to_string(i + 1);
        RenameFileUtf8(src, dst);  // 源不存在时静默失败
    }

    // 3. 当前 path → path.1
    std::string dst1 = g_file.path + ".1";
    RenameFileUtf8(g_file.path, dst1);

    // 4. 重新打开空 path（截断模式）
    if (!OpenOfstreamUtf8(g_file.path, std::ios::binary | std::ios::out | std::ios::trunc)) {
        g_file.enabled = false;  // 重新打开失败，禁用文件输出
        return;
    }
    g_file.current_size = 0;
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
    // 同步关闭文件输出（防止 shutdown 后还有线程写入文件）
    std::lock_guard<std::mutex> lock(g_file_mutex);
    if (g_file.enabled) {
        g_file.stream.flush();
        g_file.stream.close();
        g_file.enabled = false;
        g_file.path.clear();
        g_file.current_size = 0;
    }
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

    // 完整日志行（stderr 用 fprintf）
    std::fprintf(stderr, "[%s] [%s] [%s:%d] %s\n",
                 time_buf, LevelName(level), base, line, user_buf);
    std::fflush(stderr);

    // 文件输出（持锁）
    std::lock_guard<std::mutex> lock(g_file_mutex);
    if (!g_file.enabled || !g_file.stream.is_open()) return;

    // 拼成一行后写入；用 std::string 一次性写以便统计字节数
    std::string line_str;
    line_str.reserve(64 + std::strlen(user_buf));
    line_str += '[';
    line_str += time_buf;
    line_str += "] [";
    line_str += LevelName(level);
    line_str += "] [";
    line_str += base;
    line_str += ':';
    line_str += std::to_string(line);
    line_str += "] ";
    line_str += user_buf;
    line_str += '\n';

    g_file.stream.write(line_str.data(), static_cast<std::streamsize>(line_str.size()));
    g_file.stream.flush();  // 简单策略：每次写完即 flush，避免崩溃丢日志
    g_file.current_size += line_str.size();

    // 滚动判断
    if (g_file.max_size > 0 && g_file.current_size >= g_file.max_size) {
        RollFiles();
    }
}

int z_log_file_open(const char* path, uint64_t max_size, uint32_t max_files) {
    ZBASE_C_API_BEGIN
    if (path == nullptr) return Z_ERR_INVALID_ARG;
    if (max_files > 99) return Z_ERR_INVALID_ARG;

    std::lock_guard<std::mutex> lock(g_file_mutex);

    // 若已有文件打开，先关闭（保留旧文件不删）
    if (g_file.enabled) {
        g_file.stream.flush();
        g_file.stream.close();
        g_file.enabled = false;
    }

    g_file.path = path;
    g_file.max_size = max_size;
    g_file.max_files = max_files == 0 ? 1 : max_files;
    g_file.current_size = 0;

    // 以追加模式打开（不截断已有内容）
    if (!OpenOfstreamUtf8(g_file.path, std::ios::binary | std::ios::app)) {
        g_file.path.clear();
        return Z_ERR_IO;
    }

    // 取当前文件大小作为起始计数（若已存在旧日志则从其大小累加）
    std::error_code ec;
    auto size = std::filesystem::file_size(ToFsPath(g_file.path), ec);

    if (!ec) {
        g_file.current_size = size;
    }

    g_file.enabled = true;
    return Z_OK;
    ZBASE_C_API_END(Z_ERR_UNKNOWN)
}

void z_log_file_close(void) {
    // void 返回函数不能用 ZBASE_C_API_END 宏，内部 try/catch 静默吞异常
    try {
        std::lock_guard<std::mutex> lock(g_file_mutex);
        if (g_file.enabled) {
            g_file.stream.flush();
            g_file.stream.close();
            g_file.enabled = false;
            g_file.path.clear();
            g_file.current_size = 0;
        }
    } catch (...) {
        // 静默：日志关闭失败不应影响调用方
    }
}

}  // extern "C"
