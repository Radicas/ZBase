/**
 * @file crash.cpp
 * @author 朱鹏 (Radica Zhu)
 * @date 2026-07-10
 * @version 0.2.0
 * @brief 崩溃堆栈捕获实现
 * @details 安装全局崩溃处理器（信号/异常处理），崩溃时自动将堆栈写入文件。
 *          支持文本和 JSON 双格式输出。
 *          Windows：SetUnhandledExceptionFilter + CaptureStackBackTrace。
 *          POSIX：sigaction + backtrace + dladdr。
 *          信号处理器中仅使用异步信号安全操作。
 */

#include "zbase/crash.h"
#include "zbase/error.h"
#include "zbase/time.h"
#include "zbase/internal/c_api_guard.hpp"
#include "platform/crash_platform.hpp"

#include <atomic>
#include <cstdio>
#include <cstring>
#include <ctime>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#else
#include <csignal>
#include <fcntl.h>
#include <unistd.h>
#endif

// ============================================
// 信号/异常名称映射
// ============================================
namespace {

/// 信号信息
struct SignalInfo {
    int         code;        ///< 信号编号
    const char* name;        ///< 信号名
    const char* description; ///< 中文描述
};

#ifdef _WIN32
/// Windows 异常代码到描述的映射
const char* GetExceptionName(DWORD code)
{
    switch (code) {
    case EXCEPTION_ACCESS_VIOLATION:         return "EXCEPTION_ACCESS_VIOLATION";
    case EXCEPTION_DATATYPE_MISALIGNMENT:    return "EXCEPTION_DATATYPE_MISALIGNMENT";
    case EXCEPTION_BREAKPOINT:               return "EXCEPTION_BREAKPOINT";
    case EXCEPTION_SINGLE_STEP:              return "EXCEPTION_SINGLE_STEP";
    case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:    return "EXCEPTION_ARRAY_BOUNDS_EXCEEDED";
    case EXCEPTION_FLT_DENORMAL_OPERAND:     return "EXCEPTION_FLT_DENORMAL_OPERAND";
    case EXCEPTION_FLT_DIVIDE_BY_ZERO:       return "EXCEPTION_FLT_DIVIDE_BY_ZERO";
    case EXCEPTION_FLT_INEXACT_RESULT:       return "EXCEPTION_FLT_INEXACT_RESULT";
    case EXCEPTION_FLT_INVALID_OPERATION:    return "EXCEPTION_FLT_INVALID_OPERATION";
    case EXCEPTION_FLT_OVERFLOW:             return "EXCEPTION_FLT_OVERFLOW";
    case EXCEPTION_FLT_STACK_CHECK:          return "EXCEPTION_FLT_STACK_CHECK";
    case EXCEPTION_FLT_UNDERFLOW:            return "EXCEPTION_FLT_UNDERFLOW";
    case EXCEPTION_INT_DIVIDE_BY_ZERO:       return "EXCEPTION_INT_DIVIDE_BY_ZERO";
    case EXCEPTION_INT_OVERFLOW:             return "EXCEPTION_INT_OVERFLOW";
    case EXCEPTION_PRIV_INSTRUCTION:         return "EXCEPTION_PRIV_INSTRUCTION";
    case EXCEPTION_IN_PAGE_ERROR:            return "EXCEPTION_IN_PAGE_ERROR";
    case EXCEPTION_ILLEGAL_INSTRUCTION:      return "EXCEPTION_ILLEGAL_INSTRUCTION";
    case EXCEPTION_NONCONTINUABLE_EXCEPTION: return "EXCEPTION_NONCONTINUABLE_EXCEPTION";
    case EXCEPTION_STACK_OVERFLOW:           return "EXCEPTION_STACK_OVERFLOW";
    case EXCEPTION_INVALID_DISPOSITION:      return "EXCEPTION_INVALID_DISPOSITION";
    case EXCEPTION_GUARD_PAGE:               return "EXCEPTION_GUARD_PAGE";
    case EXCEPTION_INVALID_HANDLE:           return "EXCEPTION_INVALID_HANDLE";
    default:                                 return "UNKNOWN_EXCEPTION";
    }
}

/// Windows 异常代码到中文描述的映射
const char* GetExceptionDescription(DWORD code)
{
    switch (code) {
    case EXCEPTION_ACCESS_VIOLATION:         return "访问违规";
    case EXCEPTION_DATATYPE_MISALIGNMENT:    return "数据类型未对齐";
    case EXCEPTION_BREAKPOINT:               return "断点";
    case EXCEPTION_SINGLE_STEP:              return "单步执行";
    case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:    return "数组越界";
    case EXCEPTION_FLT_DENORMAL_OPERAND:     return "浮点非规格化操作数";
    case EXCEPTION_FLT_DIVIDE_BY_ZERO:       return "浮点除以零";
    case EXCEPTION_FLT_INEXACT_RESULT:       return "浮点结果不精确";
    case EXCEPTION_FLT_INVALID_OPERATION:    return "浮点非法操作";
    case EXCEPTION_FLT_OVERFLOW:             return "浮点溢出";
    case EXCEPTION_FLT_STACK_CHECK:          return "浮点栈检查";
    case EXCEPTION_FLT_UNDERFLOW:            return "浮点下溢";
    case EXCEPTION_INT_DIVIDE_BY_ZERO:       return "整数除以零";
    case EXCEPTION_INT_OVERFLOW:             return "整数溢出";
    case EXCEPTION_PRIV_INSTRUCTION:         return "特权指令";
    case EXCEPTION_IN_PAGE_ERROR:            return "页面错误";
    case EXCEPTION_ILLEGAL_INSTRUCTION:      return "非法指令";
    case EXCEPTION_NONCONTINUABLE_EXCEPTION: return "不可继续的异常";
    case EXCEPTION_STACK_OVERFLOW:           return "栈溢出";
    case EXCEPTION_INVALID_DISPOSITION:      return "非法异常处理";
    case EXCEPTION_GUARD_PAGE:               return "保护页异常";
    case EXCEPTION_INVALID_HANDLE:           return "无效句柄";
    default:                                 return "未知异常";
    }
}
#else
/// POSIX 信号表
const SignalInfo kSignalTable[] = {
    {SIGSEGV, "SIGSEGV", "段错误"},
    {SIGABRT, "SIGABRT", "异常终止"},
    {SIGFPE,  "SIGFPE",  "浮点异常"},
    {SIGILL,  "SIGILL",  "非法指令"},
    {SIGBUS,  "SIGBUS",  "总线错误"},
};
constexpr int kSignalCount = sizeof(kSignalTable) / sizeof(kSignalTable[0]);

/// 根据信号编号查找信号名称
const SignalInfo* FindSignal(int sig)
{
    for (int i = 0; i < kSignalCount; ++i) {
        if (kSignalTable[i].code == sig) return &kSignalTable[i];
    }
    return nullptr;
}
#endif

}  // namespace

// ============================================
// 全局状态（编译期分配，避免崩溃时堆操作）
// ============================================
namespace {

/// 崩溃文件输出目录
char g_output_dir[4096] = {0};

/// 应用程序名称
char g_app_name[256] = "zbase_app";

/// 符号解析是否就绪
bool g_sym_ready = false;

/// 崩溃处理器是否已安装
std::atomic<bool> g_installed{false};

#ifdef _WIN32
/// 前一个异常过滤器（链式调用）
LPTOP_LEVEL_EXCEPTION_FILTER g_prev_filter = nullptr;
#else
/// 已安装信号的旧处理器（用于恢复）
struct sigaction g_prev_actions[kSignalCount];
/// 注册的信号列表
const int g_signal_list[kSignalCount] = {SIGSEGV, SIGABRT, SIGFPE, SIGILL, SIGBUS};
#endif

}  // namespace

// ============================================
// 格式化辅助（在栈上操作，崩溃时安全）
// ============================================
namespace {

/**
 * @brief 格式化时间戳字符串到缓冲区
 * @param buf 输出缓冲区
 * @param size 缓冲区大小
 */
void FormatTimestamp(char* buf, size_t size)
{
    int64_t now_ms = z_time_now_ms();
    int64_t sec = now_ms / 1000;
    int     ms  = static_cast<int>(now_ms % 1000);

    time_t  t = static_cast<time_t>(sec);
    struct tm tm_buf;
#ifdef _WIN32
    localtime_s(&tm_buf, &t);
#else
    localtime_r(&t, &tm_buf);
#endif
    std::snprintf(buf, size,
                  "%04d-%02d-%02dT%02d:%02d:%02d.%03d",
                  tm_buf.tm_year + 1900, tm_buf.tm_mon + 1, tm_buf.tm_mday,
                  tm_buf.tm_hour, tm_buf.tm_min, tm_buf.tm_sec, ms);
}

/**
 * @brief 获取可执行文件所在目录
 * @param buf 输出缓冲区
 * @param size 缓冲区大小
 * @return true 成功
 */
bool GetExeDirectory(char* buf, size_t size)
{
#ifdef _WIN32
    char exe_path[MAX_PATH] = {0};
    DWORD len = GetModuleFileNameA(nullptr, exe_path, sizeof(exe_path));
    if (len == 0 || len >= sizeof(exe_path)) return false;

    // 截断到最后一个反斜杠
    char* last_sep = nullptr;
    for (char* p = exe_path; *p; ++p) {
        if (*p == '\\' || *p == '/') last_sep = p;
    }
    if (last_sep) *last_sep = '\0';

    std::snprintf(buf, size, "%s", exe_path);
    return true;
#else
    // POSIX：读取 /proc/self/exe 获取可执行文件路径
    char exe_path[4096] = {0};
    ssize_t len = readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1);
    if (len <= 0) {
        // 回退到 macOS 的 _NSGetExecutablePath
#ifdef __APPLE__
#include <mach-o/dyld.h>
        uint32_t path_len = (uint32_t)sizeof(exe_path);
        if (_NSGetExecutablePath(exe_path, &path_len) != 0) return false;
        len = (ssize_t)strlen(exe_path);
#else
        return false;
#endif
    }
    exe_path[len] = '\0';

    // 截断到最后一个斜杠
    char* last_sep = nullptr;
    for (char* p = exe_path; *p; ++p) {
        if (*p == '/') last_sep = p;
    }
    if (last_sep) *last_sep = '\0';

    std::snprintf(buf, size, "%s", exe_path);
    return true;
#endif
}

}  // namespace

// ============================================
// 崩溃报告写入
// ============================================
namespace {

/**
 * @brief 写入文本格式崩溃报告（POSIX 信号安全版本）
 * @details 直接使用 write() 系统调用，避免 stdio 和堆操作。
 * @param signame 信号/异常名称
 * @param sigdesc 中文描述
 * @param fault_addr 出错地址
 * @param frames 已解析的堆栈帧
 * @param frame_count 帧数
 * @param output_dir 输出目录
 * @param app_name 应用名称
 * @param timestamp 时间戳字符串
 */
void WriteCrashReportText(const char* signame,
                          const char* sigdesc,
                          void*       fault_addr,
                          zbase::platform::CrashFrame* frames,
                          int         frame_count,
                          const char* output_dir,
                          const char* app_name,
                          const char* timestamp)
{
    char path[4096];
    std::snprintf(path, sizeof(path), "%s/%s_crash_%s.txt",
                  output_dir, app_name, timestamp);

#ifdef _WIN32
    // Windows：异常处理器运行在普通上下文中，可使用标准文件 IO
    FILE* fp = nullptr;
    fopen_s(&fp, path, "w");
    if (!fp) return;

    std::fprintf(fp, "========================================\n");
    std::fprintf(fp, "ZBase 崩溃报告\n");
    std::fprintf(fp, "========================================\n");
    std::fprintf(fp, "应用程序: %s\n", app_name);
    std::fprintf(fp, "时间戳: %s\n", timestamp);
    std::fprintf(fp, "异常: %s (%s)\n", signame, sigdesc);
    std::fprintf(fp, "错误地址: 0x%p\n", fault_addr);
    std::fprintf(fp, "----------------------------------------\n");
    std::fprintf(fp, "堆栈回溯 (共 %d 帧):\n", frame_count);

    for (int i = 0; i < frame_count; ++i) {
        const auto& f = frames[i];

        // 基础信息：帧号 + 地址
        std::fprintf(fp, "  #%02d  0x%p  ", i, f.address);

        // 模块名 + 函数名
        if (f.function_name[0]) {
            std::fprintf(fp, "%s!%s+0x%llx",
                         f.module_name, f.function_name,
                         (unsigned long long)f.offset);
        } else if (f.module_name[0]) {
            std::fprintf(fp, "%s+0x%llx",
                         f.module_name,
                         (unsigned long long)f.offset);
        } else {
            std::fprintf(fp, "<unknown>+0x%llx",
                         (unsigned long long)f.offset);
        }

        // 源文件信息
        if (f.source_file[0]) {
            std::fprintf(fp, "  [%s:%d]", f.source_file, f.source_line);
        }

        std::fprintf(fp, "\n");
    }

    std::fprintf(fp, "========================================\n");
    std::fclose(fp);

#else
    // POSIX：信号处理器中使用异步安全操作
    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) return;

    /// 向栈缓冲区写入格式化字符串的辅助 lambda（信号安全上下文）
    char buf[65536];
    int  pos = 0;
    auto snp = [&]<typename... Args>(const char* fmt, Args... args) {
        int rem = (int)sizeof(buf) - pos;
        if (rem <= 0) return;
        int n = snprintf(buf + pos, rem, fmt, args...);
        if (n > 0 && n < rem) pos += n;
    };

    // 构建文本报告
    snp("========================================\n");
    snp("ZBase 崩溃报告\n");
    snp("========================================\n");
    snp("应用程序: %s\n", app_name);
    snp("时间戳: %s\n", timestamp);
    snp("信号: %s (%s)\n", signame, sigdesc);
    snp("错误地址: %p\n", fault_addr);
    snp("----------------------------------------\n");
    snp("堆栈回溯 (共 %d 帧):\n", frame_count);

    for (int i = 0; i < frame_count; ++i) {
        if ((int)sizeof(buf) - pos < 256) break;  // 缓冲区不足，截断
        const auto& f = frames[i];

        snp("  #%02d  %p  ", i, f.address);

        if (f.function_name[0]) {
            snp("%s!%s+0x%llx", f.module_name, f.function_name,
                (unsigned long long)f.offset);
        } else if (f.module_name[0]) {
            snp("%s+0x%llx", f.module_name, (unsigned long long)f.offset);
        } else {
            snp("<unknown>+0x%llx", (unsigned long long)f.offset);
        }

        if (f.source_file[0]) {
            snp("  [%s:%d]", f.source_file, f.source_line);
        }
        snp("\n");
    }

    snp("========================================\n");

    // 一次性写入
    ssize_t written = write(fd, buf, pos);
    (void)written;  // 崩溃时无法处理写入失败
    close(fd);
#endif
}

/**
 * @brief 写入 JSON 格式崩溃报告
 */
void WriteCrashReportJson(const char* signame,
                          const char* sigdesc,
                          void*       fault_addr,
                          zbase::platform::CrashFrame* frames,
                          int         frame_count,
                          const char* output_dir,
                          const char* app_name,
                          const char* timestamp)
{
    char path[4096];
    std::snprintf(path, sizeof(path), "%s/%s_crash_%s.json",
                  output_dir, app_name, timestamp);

#ifdef _WIN32
    FILE* fp = nullptr;
    fopen_s(&fp, path, "w");
    if (!fp) return;

    std::fprintf(fp, "{\n");
    std::fprintf(fp, "  \"app_name\": \"%s\",\n", app_name);
    std::fprintf(fp, "  \"timestamp\": \"%s\",\n", timestamp);
    std::fprintf(fp, "  \"signal\": \"%s\",\n", signame);
    std::fprintf(fp, "  \"signal_description\": \"%s\",\n", sigdesc);
    char addr_buf[32];
    std::snprintf(addr_buf, sizeof(addr_buf), "0x%p", fault_addr);
    std::fprintf(fp, "  \"fault_address\": \"%s\",\n", addr_buf);
    std::fprintf(fp, "  \"frame_count\": %d,\n", frame_count);
    std::fprintf(fp, "  \"frames\": [\n");

    for (int i = 0; i < frame_count; ++i) {
        const auto& f = frames[i];
        char addr_str[32];
        std::snprintf(addr_str, sizeof(addr_str), "0x%p", f.address);

        std::fprintf(fp, "    {\n");
        std::fprintf(fp, "      \"index\": %d,\n", i);
        std::fprintf(fp, "      \"address\": \"%s\",\n", addr_str);
        std::fprintf(fp, "      \"module\": \"%s\",\n", f.module_name);
        std::fprintf(fp, "      \"function\": \"%s\",\n",
                     f.function_name[0] ? f.function_name : "");

        char offset_str[32];
        std::snprintf(offset_str, sizeof(offset_str), "0x%llx",
                      (unsigned long long)f.offset);
        std::fprintf(fp, "      \"offset\": \"%s\"", offset_str);

        if (f.source_file[0]) {
            std::fprintf(fp, ",\n      \"source\": \"%s:%d\"",
                         f.source_file, f.source_line);
        }
        std::fprintf(fp, "\n    }%s\n",
                     (i < frame_count - 1) ? "," : "");
    }

    std::fprintf(fp, "  ]\n");
    std::fprintf(fp, "}\n");
    std::fclose(fp);

#else
    // POSIX：信号安全版本，使用 write()
    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) return;

    char buf[65536];
    int  pos = 0;
    auto snp = [&]<typename... Args>(const char* fmt, Args... args) {
        int rem = (int)sizeof(buf) - pos;
        if (rem <= 0) return;
        int n = snprintf(buf + pos, rem, fmt, args...);
        if (n > 0 && n < rem) pos += n;
    };

    char addr_str[32];
    std::snprintf(addr_str, sizeof(addr_str), "%p", fault_addr);

    snp("{\n");
    snp("  \"app_name\": \"%s\",\n", app_name);
    snp("  \"timestamp\": \"%s\",\n", timestamp);
    snp("  \"signal\": \"%s\",\n", signame);
    snp("  \"signal_description\": \"%s\",\n", sigdesc);
    snp("  \"fault_address\": \"%s\",\n", addr_str);
    snp("  \"frame_count\": %d,\n", frame_count);
    snp("  \"frames\": [\n");

    for (int i = 0; i < frame_count; ++i) {
        const auto& f = frames[i];
        char faddr[32];
        std::snprintf(faddr, sizeof(faddr), "%p", f.address);
        char foff[32];
        std::snprintf(foff, sizeof(foff), "0x%llx",
                      (unsigned long long)f.offset);

        snp("    {\n");
        snp("      \"index\": %d,\n", i);
        snp("      \"address\": \"%s\",\n", faddr);
        snp("      \"module\": \"%s\",\n", f.module_name);
        snp("      \"function\": \"%s\",\n",
            f.function_name[0] ? f.function_name : "");
        snp("      \"offset\": \"%s\"", foff);

        if (f.source_file[0]) {
            snp(",\n      \"source\": \"%s:%d\"",
                f.source_file, f.source_line);
        }
        snp("\n    }%s\n", (i < frame_count - 1) ? "," : "");
    }

    snp("  ]\n");
    snp("}\n");

    ssize_t written = write(fd, buf, pos);
    (void)written;
    close(fd);
#endif
}

}  // namespace

// ============================================
// 崩溃处理器入口
// ============================================
namespace {

/**
 * @brief 通用崩溃处理流程
 * @details 捕获堆栈、解析符号、写入文件。POSIX 信号安全。
 */
void HandleCrash(const char* signame,
                 const char* sigdesc,
                 void*       fault_addr,
                 void*       platform_context)
{
    // 1. 捕获堆栈帧地址
    void* raw_frames[zbase::platform::kMaxCrashFrames];
    int frame_count = zbase::platform::CrashCaptureBacktrace(
        raw_frames, zbase::platform::kMaxCrashFrames, platform_context);

    // 2. 解析每帧符号
    zbase::platform::CrashFrame resolved[zbase::platform::kMaxCrashFrames];
    for (int i = 0; i < frame_count; ++i) {
        zbase::platform::CrashResolveFrame(raw_frames[i], resolved[i]);
    }

    // 3. 生成时间戳
    char timestamp[32];
    FormatTimestamp(timestamp, sizeof(timestamp));

    // 4. 确定输出目录
    char output_dir[4096];
    if (g_output_dir[0] != '\0') {
        std::snprintf(output_dir, sizeof(output_dir), "%s", g_output_dir);
    } else {
        if (!GetExeDirectory(output_dir, sizeof(output_dir))) {
            // 回退到当前目录
            output_dir[0] = '.';
            output_dir[1] = '\0';
        }
    }

    // 5. 写入文本报告
    WriteCrashReportText(signame, sigdesc, fault_addr,
                         resolved, frame_count,
                         output_dir, g_app_name, timestamp);

    // 6. 写入 JSON 报告
    WriteCrashReportJson(signame, sigdesc, fault_addr,
                         resolved, frame_count,
                         output_dir, g_app_name, timestamp);
}

}  // namespace

// ============================================
// Windows 异常处理器
// ============================================
#ifdef _WIN32

namespace {

LONG WINAPI UnhandledExceptionFilter(EXCEPTION_POINTERS* ex_info)
{
    DWORD code = ex_info->ExceptionRecord->ExceptionCode;
    void* addr = ex_info->ExceptionRecord->ExceptionAddress;

    HandleCrash(GetExceptionName(code),
                GetExceptionDescription(code),
                addr,
                ex_info);

    // 调用前一个过滤器（如果存在），否则让系统处理
    if (g_prev_filter) {
        return g_prev_filter(ex_info);
    }
    return EXCEPTION_CONTINUE_SEARCH;
}

}  // namespace

#else  // POSIX

// ============================================
// POSIX 信号处理器
// ============================================
namespace {

void SignalHandler(int sig, siginfo_t* info, void* context)
{
    const SignalInfo* siginfo = FindSignal(sig);
    const char* name = siginfo ? siginfo->name : "UNKNOWN";
    const char* desc = siginfo ? siginfo->description : "未知信号";

    HandleCrash(name, desc, info ? info->si_addr : nullptr, context);

    // 恢复默认处理器并重新触发信号（生成 core dump）
    struct sigaction sa;
    sa.sa_handler = SIG_DFL;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(sig, &sa, nullptr);
    raise(sig);
}

}  // namespace
#endif

// ============================================
// C ABI 接口
// ============================================

extern "C" {

int z_crash_install(const z_crash_config_t* config)
{
    ZBASE_C_API_BEGIN

    // 如果已安装，先卸载
    if (g_installed.load(std::memory_order_acquire)) {
        z_crash_uninstall();
    }

    // 保存配置
    if (config) {
        if (config->output_dir && config->output_dir[0]) {
            std::snprintf(g_output_dir, sizeof(g_output_dir),
                          "%s", config->output_dir);
        } else {
            g_output_dir[0] = '\0';
        }
        if (config->app_name && config->app_name[0]) {
            std::snprintf(g_app_name, sizeof(g_app_name),
                          "%s", config->app_name);
        } else {
            std::snprintf(g_app_name, sizeof(g_app_name), "zbase_app");
        }
    } else {
        g_output_dir[0] = '\0';
        std::snprintf(g_app_name, sizeof(g_app_name), "zbase_app");
    }

    // 初始化符号解析
    zbase::platform::CrashSymCleanup();  // 先清理遗留状态
    g_sym_ready = zbase::platform::CrashSymInit();

#ifdef _WIN32
    // Windows：使用 UnhandledExceptionFilter
    g_prev_filter = SetUnhandledExceptionFilter(UnhandledExceptionFilter);
#else
    // POSIX：注册信号处理器
    struct sigaction sa;
    std::memset(&sa, 0, sizeof(sa));
    sa.sa_sigaction = SignalHandler;
    sa.sa_flags = SA_SIGINFO | SA_RESETHAND;  // SA_RESETHAND: 单次触发后恢复默认
    sigemptyset(&sa.sa_mask);

    // 安装时允许嵌套信号触发（避免递归死锁）
    for (int i = 0; i < kSignalCount; ++i) {
        sigaction(g_signal_list[i], &sa, &g_prev_actions[i]);
    }
#endif

    g_installed.store(true, std::memory_order_release);
    return Z_OK;

    ZBASE_C_API_END(Z_ERR_CRASH_INSTALL_FAILED)
}

void z_crash_uninstall(void)
{
    if (!g_installed.load(std::memory_order_acquire)) return;

#ifdef _WIN32
    if (g_prev_filter) {
        SetUnhandledExceptionFilter(g_prev_filter);
        g_prev_filter = nullptr;
    }
#else
    // 恢复旧信号处理器
    for (int i = 0; i < kSignalCount; ++i) {
        sigaction(g_signal_list[i], &g_prev_actions[i], nullptr);
    }
#endif

    // 清理符号系统
    if (g_sym_ready) {
        zbase::platform::CrashSymCleanup();
        g_sym_ready = false;
    }

    g_installed.store(false, std::memory_order_release);
}

}  // extern "C"
