/**
 * @file console.hpp
 * @author 朱鹏 (Radica Zhu)
 * @date 2026-06-30
 * @version 0.1.0
 * @brief 控制台 UTF-8 设置（Windows save/restore，详见 phase-1-working-file.md §4.21）
 * @details Windows 下 init 保存原 console CP 并设置为 UTF-8；
 *          shutdown 恢复。POSIX 下为空操作。
 */

#ifndef ZBASE_CORE_LOG_CONSOLE_HPP_
#define ZBASE_CORE_LOG_CONSOLE_HPP_

namespace zbase {
namespace log {

/**
 * @brief 设置控制台为 UTF-8（Windows 保存原 CP）
 * @note Windows 下还会调 SetDefaultDllDirectories 限制 DLL 搜索路径
 */
void InitConsoleUtf8();

/**
 * @brief 恢复原控制台 CP（仅 Windows）
 */
void RestoreConsole();

}  // namespace log
}  // namespace zbase

#endif  // ZBASE_CORE_LOG_CONSOLE_HPP_
