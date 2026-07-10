/**
 * @file test_perf.cpp
 * @author 朱鹏 (Radica Zhu)
 * @date 2026-06-30
 * @version 0.2.0
 * @brief perf 模块测试
 */

#include <gtest/gtest.h>

#include <cstdint>
#include <string>
#include <tuple>
#include <vector>

#include "zbase/error.h"
#include "zbase/perf.h"
#include "zbase/time.h"
#include "zbase++/perf.hpp"

/// 测试用回调上下文：收集 iterate 输出的打点
struct IterateCollector {
    std::vector<std::tuple<std::string, uint64_t, uint64_t>> rows;  ///< (name, total_ns, count)
};

/// 测试用回调：把单条打点追加到 IterateCollector
static void TestVisitor(const char* name, uint64_t total_ns,
                        uint64_t count, void* user_data) {
    auto* c = static_cast<IterateCollector*>(user_data);
    c->rows.emplace_back(std::string(name), total_ns, count);
}

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
        ZBASE_PERF_SCOPE("scoped_task");
        z_time_sleep_ms(5);
    }
    // 作用域结束后应能查到该 name 的累计（通过 dump 不崩验证）
    z_perf_dump();
    SUCCEED();
}

// ============ z_perf_iterate 测试 ============

TEST(Perf, Iterate_NullCallback) {
    z_perf_reset();
    EXPECT_EQ(z_perf_start("a"), Z_OK);
    EXPECT_EQ(z_perf_end("a"), Z_OK);
    // 传 NULL 回调不应崩溃，应静默返回
    z_perf_iterate(nullptr, nullptr);
    SUCCEED();
}

TEST(Perf, Iterate_Empty) {
    z_perf_reset();
    IterateCollector c;
    z_perf_iterate(TestVisitor, &c);
    EXPECT_TRUE(c.rows.empty());
}

TEST(Perf, Iterate_VisitsAllSorted) {
    z_perf_reset();
    // slow 总耗时约 50ms，fast 总耗时约 5ms；iterate 应按总耗时降序访问
    // 间隔拉开到远超过 Windows 默认时钟滴答（15.6ms），避免精度导致不稳定
    EXPECT_EQ(z_perf_start("slow"), Z_OK);
    z_time_sleep_ms(50);
    EXPECT_EQ(z_perf_end("slow"), Z_OK);
    EXPECT_EQ(z_perf_start("fast"), Z_OK);
    z_time_sleep_ms(5);
    EXPECT_EQ(z_perf_end("fast"), Z_OK);

    IterateCollector c;
    z_perf_iterate(TestVisitor, &c);

    ASSERT_EQ(c.rows.size(), 2u);
    // 排序验证：第一项应为 slow（总耗时更大）
    EXPECT_EQ(std::get<0>(c.rows[0]), "slow");
    EXPECT_EQ(std::get<2>(c.rows[0]), 1ull);
    EXPECT_EQ(std::get<0>(c.rows[1]), "fast");
    EXPECT_EQ(std::get<2>(c.rows[1]), 1ull);
    // 降序：前一项 total_ns >= 后一项
    EXPECT_GE(std::get<1>(c.rows[0]), std::get<1>(c.rows[1]));
}

TEST(Perf, Iterate_AccumulateCount) {
    z_perf_reset();
    // 同名打点累计 3 次，验证 count 与 total_ns 正确累加
    for (int i = 0; i < 3; ++i) {
        EXPECT_EQ(z_perf_start("acc"), Z_OK);
        z_time_sleep_ms(1);
        EXPECT_EQ(z_perf_end("acc"), Z_OK);
    }

    IterateCollector c;
    z_perf_iterate(TestVisitor, &c);

    ASSERT_EQ(c.rows.size(), 1u);
    EXPECT_EQ(std::get<0>(c.rows[0]), "acc");
    EXPECT_EQ(std::get<2>(c.rows[0]), 3ull);
    EXPECT_GT(std::get<1>(c.rows[0]), 0ull);
}
