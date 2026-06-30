/**
 * @file test_demo.cpp
 * @author 朱鹏 (Radica Zhu)
 * @date 2026-06-30
 * @version 0.1.0
 * @brief 端到端集成测试：验证 demo 完整流程
 * @details 串联 file → string → perf → log 全部模块的 C++ 包装层。
 */

#include <gtest/gtest.h>

#include <cstdio>
#include <string>

#include "zbase++/file.hpp"
#include "zbase++/log.hpp"
#include "zbase++/perf.hpp"
#include "zbase++/string.hpp"
#include "zbase++/time.hpp"

TEST(IntegrationDemo, FullPipeline) {
    zbase::Logger logger(Z_LOG_LEVEL_DEBUG);

    const char* path = "zbase_integration_test.txt";
    zbase::WriteFile(path, "alpha,beta,γ,δ");

    {
        ZBASE_PERF_SCOPE("integration_total");
        zbase::FileReader reader(path);
        EXPECT_EQ(reader.Size(), 16u);  // "alpha,beta,γ,δ" = 5+1+4+1+2+1+2 字节

        auto parts = zbase::StringSplit(reader.Str(), ",");
        EXPECT_EQ(parts.size(), 4u);
        EXPECT_EQ(parts[0], "alpha");
        EXPECT_EQ(parts[1], "beta");
        EXPECT_EQ(parts[2], "γ");
        EXPECT_EQ(parts[3], "δ");

        std::string joined = zbase::StringJoin(parts, "+");
        EXPECT_EQ(joined, "alpha+beta+γ+δ");

        EXPECT_EQ(zbase::StringUtf8Len("γδ"), 2u);  // 2 个字符（4 字节）
    }

    zbase::PerfDump();
    zbase::PerfReset();
    std::remove(path);
}

TEST(IntegrationDemo, StringReplaceAndLog) {
    zbase::Logger logger(Z_LOG_LEVEL_DEBUG);

    std::string s = "name=zbase,version=0.1";
    std::string replaced = zbase::StringReplace(s, ",", ";");
    EXPECT_EQ(replaced, "name=zbase;version=0.1");

    // 中文日志输出
    Z_LOG_INFO("中文日志测试：%s", replaced.c_str());

    EXPECT_EQ(zbase::StringTrim("  zbase  "), "zbase");
    EXPECT_EQ(zbase::StringToUpper("zbase"), "ZBASE");
}

TEST(IntegrationDemo, TimePipeline) {
    int64_t t1 = zbase::NowMs();
    zbase::SleepMs(10);
    int64_t t2 = zbase::NowMs();
    EXPECT_GE(t2 - t1, 8);  // 至少 8ms（容差）

    std::string iso = zbase::FormatIso(t2);
    EXPECT_EQ(iso.size(), 23u);
    EXPECT_EQ(iso[4], '-');
    EXPECT_EQ(iso[7], '-');
    EXPECT_EQ(iso[10], 'T');
    EXPECT_EQ(iso[13], ':');
}
