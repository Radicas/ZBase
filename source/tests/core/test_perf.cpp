/**
 * @file test_perf.cpp
 * @author 朱鹏 (Radica Zhu)
 * @date 2026-06-30
 * @version 0.1.0
 * @brief perf 模块测试
 */

#include <gtest/gtest.h>

#include "zbase/error.h"
#include "zbase/perf.h"
#include "zbase/time.h"

TEST(Perf, StartEnd_NoCrash) {
    z_perf_reset();
    EXPECT_EQ(z_perf_start("task1"), Z_OK);
    z_time_sleep_ms(10);
    EXPECT_EQ(z_perf_end("task1"), Z_OK);
}

TEST(Perf, End_NotStarted) {
    z_perf_reset();
    EXPECT_EQ(z_perf_end("never_started"), Z_ERR_NOT_FOUND);
}

TEST(Perf, Start_NullName) {
    EXPECT_EQ(z_perf_start(nullptr), Z_ERR_INVALID_ARG);
}

TEST(Perf, End_NullName) {
    EXPECT_EQ(z_perf_end(nullptr), Z_ERR_INVALID_ARG);
}

TEST(Perf, Dump_NoCrash) {
    z_perf_reset();
    EXPECT_EQ(z_perf_start("dumpable"), Z_OK);
    z_time_sleep_ms(2);
    EXPECT_EQ(z_perf_end("dumpable"), Z_OK);
    z_perf_dump();
    SUCCEED();
}

TEST(Perf, Reset_ClearsAll) {
    EXPECT_EQ(z_perf_start("a"), Z_OK);
    EXPECT_EQ(z_perf_end("a"), Z_OK);
    z_perf_reset();
    EXPECT_EQ(z_perf_end("a"), Z_ERR_NOT_FOUND);
}

TEST(Perf, Accumulate_SameName) {
    z_perf_reset();
    EXPECT_EQ(z_perf_start("acc"), Z_OK);
    z_time_sleep_ms(2);
    EXPECT_EQ(z_perf_end("acc"), Z_OK);
    EXPECT_EQ(z_perf_start("acc"), Z_OK);
    z_time_sleep_ms(2);
    EXPECT_EQ(z_perf_end("acc"), Z_OK);
    // 仅验证不崩溃且能 dump；累计正确性靠 dump 输出检查
    z_perf_dump();
    SUCCEED();
}

TEST(Perf, Nested_SameName) {
    z_perf_reset();
    EXPECT_EQ(z_perf_start("nested"), Z_OK);
    EXPECT_EQ(z_perf_start("nested"), Z_OK);
    z_time_sleep_ms(2);
    EXPECT_EQ(z_perf_end("nested"), Z_OK);
    EXPECT_EQ(z_perf_end("nested"), Z_OK);
    EXPECT_EQ(z_perf_end("nested"), Z_ERR_NOT_FOUND);
    z_perf_dump();
    SUCCEED();
}

TEST(Perf, Scope_Macro) {
    z_perf_reset();
    {
        Z_PERF_SCOPE("scoped_task");
        z_time_sleep_ms(5);
    }
    // 作用域结束后应能查到该 name 的累计（通过 dump 不崩验证）
    z_perf_dump();
    SUCCEED();
}
