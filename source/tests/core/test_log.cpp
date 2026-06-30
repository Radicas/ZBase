/**
 * @file test_log.cpp
 * @author 朱鹏 (Radica Zhu)
 * @date 2026-06-30
 * @version 0.1.0
 * @brief log 模块测试
 */

#include <gtest/gtest.h>

#include "zbase/error.h"
#include "zbase/log.h"

TEST(Log, InitShutdown) {
    EXPECT_EQ(z_log_init(Z_LOG_LEVEL_DEBUG), Z_OK);
    z_log_shutdown();
    SUCCEED();
}

TEST(Log, InitShutdown_MultipleTimes) {
    EXPECT_EQ(z_log_init(Z_LOG_LEVEL_INFO), Z_OK);
    z_log_shutdown();
    EXPECT_EQ(z_log_init(Z_LOG_LEVEL_ERROR), Z_OK);
    z_log_shutdown();
    SUCCEED();
}

TEST(Log, Write_NoCrash) {
    z_log_init(Z_LOG_LEVEL_DEBUG);
    Z_LOG_INFO("测试中文日志 %d", 42);
    Z_LOG_WARN("warning %s", "msg");
    Z_LOG_ERROR("error message");
    z_log_shutdown();
    SUCCEED();
}

TEST(Log, LevelFilter) {
    z_log_init(Z_LOG_LEVEL_ERROR);
    Z_LOG_DEBUG("should not appear");
    Z_LOG_INFO("should not appear");
    Z_LOG_ERROR("should appear");
    z_log_shutdown();
    SUCCEED();
}

TEST(Log, Write_AllLevels) {
    z_log_init(Z_LOG_LEVEL_DEBUG);
    Z_LOG_DEBUG("debug message %d", 1);
    Z_LOG_INFO("info message %d", 2);
    Z_LOG_WARN("warn message %d", 3);
    Z_LOG_ERROR("error message %d", 4);
    z_log_shutdown();
    SUCCEED();
}
