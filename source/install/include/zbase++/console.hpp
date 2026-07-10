/**
 * @file console.hpp
 * @author 朱鹏 (Radica Zhu)
 * @date 2026-07-01
 * @version 0.3.0
 * @brief 控制台 UTF-8 初始化 C++ 包装
 * @details 提供 zbase::InitConsole() 内联函数。
 *          所有 ZBase 可执行程序应在 main() 开头调用此函数。
 */

#pragma once

#include "zbase/zbase.h"

namespace zbase {

/// 初始化控制台为 UTF-8 编码（Windows 下解决中文乱码）
/// @note 可重复调用（内部幂等）
inline void InitConsole() { z_init_console(); }

}  // namespace zbase
