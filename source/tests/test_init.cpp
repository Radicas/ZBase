/**
 * @file test_init.cpp
 * @author 朱鹏 (Radica Zhu)
 * @date 2026-07-01
 * @version 1.0.0
 * @brief 测试程序入口初始化：设置控制台为 UTF-8
 * @details 通过全局对象构造函数在 main() 之前调用 z_init_console()，
 *          确保所有测试输出的中文不乱码。POSIX 下为空操作。
 */

#include "zbase/zbase.h"

namespace {

/// 全局初始化器：构造时设控制台 UTF-8
struct TestInit {
    TestInit() { z_init_console(); }
};

TestInit g_test_init;  ///< 全局实例（main 之前构造）

}  // namespace
