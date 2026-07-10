/**
 * @file test_crash.cpp
 * @author 朱鹏 (Radica Zhu)
 * @date 2026-07-10
 * @version 0.2.0
 * @brief crash 模块测试
 * @details 测试崩溃处理器的安装、卸载、配置等生命周期。
 *          注意：实际崩溃触发需要在集成测试或手动测试中验证。
 */

#include <gtest/gtest.h>

#include <cstdio>
#include <cstring>
#include <string>

#include "zbase/crash.h"
#include "zbase/error.h"
#include "zbase++/crash.hpp"

// ============================================
// C ABI 接口测试
// ============================================

TEST(Crash, Install_Uninstall_Basic)
{
    // 安装崩溃处理器
    z_crash_config_t config;
    config.output_dir = nullptr;
    config.app_name   = "test_app";
    int err = z_crash_install(&config);
    EXPECT_EQ(err, Z_OK);

    // 卸载
    z_crash_uninstall();
}

TEST(Crash, Install_DoubleInstall_AutoUninstall)
{
    // 第一次安装
    z_crash_config_t config;
    config.output_dir = nullptr;
    config.app_name   = "test_app";
    EXPECT_EQ(z_crash_install(&config), Z_OK);

    // 第二次安装（自动先卸载）
    EXPECT_EQ(z_crash_install(&config), Z_OK);

    // 清理
    z_crash_uninstall();
}

TEST(Crash, Install_NullConfig)
{
    // NULL 配置使用默认值
    int err = z_crash_install(nullptr);
    EXPECT_EQ(err, Z_OK);

    z_crash_uninstall();
}

TEST(Crash, Install_WithOutputDir)
{
    z_crash_config_t config;
    config.output_dir = ".";
    config.app_name   = "test_crash";
    int err = z_crash_install(&config);
    EXPECT_EQ(err, Z_OK);

    z_crash_uninstall();
}

TEST(Crash, Install_WithChineseAppName)
{
    z_crash_config_t config;
    config.output_dir = ".";
    config.app_name   = "测试程序";
    int err = z_crash_install(&config);
    EXPECT_EQ(err, Z_OK);

    z_crash_uninstall();
}

TEST(Crash, Uninstall_WhenNotInstalled)
{
    // 未安装时卸载不应崩溃
    z_crash_uninstall();
    z_crash_uninstall();  // 可重复调用
    SUCCEED();
}

TEST(Crash, Uninstall_ThenReinstall)
{
    z_crash_config_t config;
    config.output_dir = nullptr;
    config.app_name   = "test_app";

    // 安装 → 卸载 → 再安装
    EXPECT_EQ(z_crash_install(&config), Z_OK);
    z_crash_uninstall();
    EXPECT_EQ(z_crash_install(&config), Z_OK);
    z_crash_uninstall();
}

TEST(Crash, ErrorMessage_CrashAlreadyInstalled)
{
    const char* msg = z_error_msg(Z_ERR_CRASH_ALREADY_INSTALLED);
    EXPECT_NE(msg, nullptr);
    EXPECT_GT(std::strlen(msg), 0u);
}

TEST(Crash, ErrorMessage_CrashInstallFailed)
{
    const char* msg = z_error_msg(Z_ERR_CRASH_INSTALL_FAILED);
    EXPECT_NE(msg, nullptr);
    EXPECT_GT(std::strlen(msg), 0u);
}

// ============================================
// C++ RAII 包装测试
// ============================================

TEST(CppCrash, RAII_Basic)
{
    {
        zbase::CrashHandler crash("", "test_raii");
        // 作用域内崩溃处理器激活
        SUCCEED();
    }
    // 析构后已卸载
}

TEST(CppCrash, RAII_DefaultConfig)
{
    {
        zbase::CrashHandler crash;
        SUCCEED();
    }
}

TEST(CppCrash, RAII_WithOutputDir)
{
    {
        zbase::CrashHandler crash(".", "raii_test");
        SUCCEED();
    }
}

TEST(CppCrash, RAII_NestedNotAllowed)
{
    // CrashHandler 不可拷贝/移动，确保单例语义
    zbase::CrashHandler crash1;
    // crash2 会先卸载 crash1 再安装（因 z_crash_install 内部自动先卸载）
    {
        zbase::CrashHandler crash2;
        SUCCEED();
    }
    // crash2 析构后，crash1 也已卸载
}
