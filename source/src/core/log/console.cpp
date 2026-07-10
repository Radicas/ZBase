/**
 * @file console.cpp
 * @author 朱鹏 (Radica Zhu)
 * @date 2026-06-30
 * @version 0.1.0
 * @brief 控制台 UTF-8 设置实现
 */

#include "core/log/console.hpp"

#ifdef _WIN32
#include <Windows.h>

namespace zbase {
namespace log {

namespace {
UINT g_orig_cp = 0;         ///< 原输入 CP
UINT g_orig_output_cp = 0;  ///< 原输出 CP
bool g_initialized = false; ///< 是否已初始化
}

void InitConsoleUtf8() {
    if (g_initialized) return;
    g_orig_cp = GetConsoleCP();
    g_orig_output_cp = GetConsoleOutputCP();
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);
    // 启用 ANSI 转义序列支持（Windows 10 1607+）
    // stderr 和 stdout 分别设置，避免管道重定向时失败
    {
        HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
        if (h && h != INVALID_HANDLE_VALUE) {
            DWORD mode = 0;
            if (GetConsoleMode(h, &mode)) {
                mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
                SetConsoleMode(h, mode);
            }
        }
    }
    {
        HANDLE h = GetStdHandle(STD_ERROR_HANDLE);
        if (h && h != INVALID_HANDLE_VALUE) {
            DWORD mode = 0;
            if (GetConsoleMode(h, &mode)) {
                mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
                SetConsoleMode(h, mode);
            }
        }
    }
    // 限制 DLL 搜索路径（详见 phase-1-working-file.md §4.26）
    SetDefaultDllDirectories(LOAD_LIBRARY_SEARCH_APPLICATION_DIR
                             | LOAD_LIBRARY_SEARCH_SYSTEM32);
    g_initialized = true;
}

void RestoreConsole() {
    if (!g_initialized) return;
    if (g_orig_cp) SetConsoleCP(g_orig_cp);
    if (g_orig_output_cp) SetConsoleOutputCP(g_orig_output_cp);
    g_initialized = false;
}

}  // namespace log
}  // namespace zbase

#else  // POSIX

namespace zbase {
namespace log {

void InitConsoleUtf8() {}
void RestoreConsole() {}

}  // namespace log
}  // namespace zbase

#endif
