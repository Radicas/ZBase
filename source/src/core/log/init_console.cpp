/**
 * @file init_console.cpp
 * @author 朱鹏 (Radica Zhu)
 * @date 2026-07-01
 * @version 0.3.0
 * @brief z_init_console C ABI 公共接口实现
 * @details 封装 InitConsoleUtf8()，对外暴露为稳定的 C ABI。
 *          所有 ZBase 可执行程序应调用此函数初始化控制台 UTF-8 编码。
 */

#include "zbase/zbase.h"
#include "core/log/console.hpp"

extern "C" {

void z_init_console(void) {
    zbase::log::InitConsoleUtf8();
}

}  // extern "C"
