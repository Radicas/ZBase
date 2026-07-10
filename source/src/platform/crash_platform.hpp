/**
 * @file crash_platform.hpp
 * @author 朱鹏 (Radica Zhu)
 * @date 2026-07-10
 * @version 0.2.0
 * @brief 崩溃堆栈平台原语（内部，不对外导出）
 * @details 封装平台相关的堆栈行走和符号解析。
 *          Windows：CaptureStackBackTrace + dbghelp.dll（动态加载）进行符号解析。
 *          POSIX：backtrace() + dladdr() 进行堆栈行走和符号解析。
 *          支持三级降级：完整符号 → 导出函数名 → 模块名+偏移。
 */

#ifndef ZBASE_PLATFORM_CRASH_PLATFORM_HPP_
#define ZBASE_PLATFORM_CRASH_PLATFORM_HPP_

#include <cstdint>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#else
#include <dlfcn.h>
#include <execinfo.h>
#endif

namespace zbase {
namespace platform {

/// 最大捕获的堆栈帧数
constexpr int kMaxCrashFrames = 128;

/**
 * @brief 解析后的单帧堆栈信息
 */
struct CrashFrame {
    void*  address;                    ///< 原始地址
    char   module_name[256];          ///< 模块名（如 "zbase.dll"、"my_app"）
    char   function_name[256];        ///< 函数名（如 "z_file_read_text"），无法解析时为 ""
    uint64_t offset;                  ///< 相对模块基址的偏移
    char   source_file[256];          ///< 源文件名，无法解析时为 ""
    int    source_line;               ///< 源文件行号，无法解析时为 0
};

// ============================================
// 平台接口
// ============================================

/**
 * @brief 初始化符号解析子系统
 * @return true 初始化成功（符号可解析）；false 失败（回退到仅地址模式）
 * @note Windows 下动态加载 dbghelp.dll；POSIX 下为 no-op
 */
bool CrashSymInit();

/**
 * @brief 清理符号解析子系统
 * @note Windows 下卸载 dbghelp.dll；POSIX 下为 no-op
 */
void CrashSymCleanup();

/**
 * @brief 捕获当前堆栈帧地址
 * @param[out] frames 输出地址数组（调用方分配）
 * @param max_frames 数组容量
 * @param context 平台上下文指针（Windows: EXCEPTION_POINTERS* 可为 NULL；
 *                POSIX: ucontext_t*，可为 NULL）
 * @return 实际捕获的帧数
 */
int CrashCaptureBacktrace(void** frames, int max_frames, void* context);

/**
 * @brief 将地址解析为可读的堆栈帧信息
 * @param address 待解析的地址
 * @param[out] out 解析结果
 * @note 内部进行三级降级：
 *       1. 完整符号（函数名 + 源文件 + 行号）
 *       2. 仅导出函数名 + 偏移
 *       3. 仅模块名 + 偏移
 */
void CrashResolveFrame(void* address, CrashFrame& out);

}  // namespace platform
}  // namespace zbase

#endif  // ZBASE_PLATFORM_CRASH_PLATFORM_HPP_
