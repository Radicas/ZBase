/**
 * @file test_time.cpp
 * @author 朱鹏 (Radica Zhu)
 * @date 2026-06-29
 * @version 0.1.0
 * @brief time 模块测试
 * @details 覆盖 now_ms 范围、now_us 与 now_ms 一致性、format_iso 格式与缓冲区边界、sleep 精度。
 *          FormatIso_Basic 用 std::localtime 自算期望值，让测试与时区无关。
 */

#include <gtest/gtest.h>
#include "zbase/error.h"
#include "zbase/time.h"

#include <cstring>
#include <ctime>
#include <cstdio>

// ============ NowMs / NowUs ============

TEST(Time, NowMs_Recent) {
    int64_t ms = z_time_now_ms();
    EXPECT_GT(ms, 1700000000000LL);  // 2023-11 之后
    EXPECT_LT(ms, 2000000000000LL);  // 2033-05 之前
}

TEST(Time, NowUs_GreaterThanMs) {
    int64_t us = z_time_now_us();
    int64_t ms = z_time_now_ms();
    // us 先采，ms 后采；us/1000 应该在 ms 的 1 秒误差内
    EXPECT_GE(us / 1000, ms - 1000);
}

TEST(Time, NowUs_RecentEpoch) {
    int64_t us = z_time_now_us();
    EXPECT_GT(us, 1700000000000000LL);  // 2023-11 之后
    EXPECT_LT(us, 2000000000000000LL);  // 2033-05 之前
}

// ============ FormatIso ============

TEST(Time, FormatIso_Basic) {
    // 用 std::localtime 自算期望值，让测试与时区无关
    char buf[32] = {0};
    int64_t ms = 1772265296789LL;
    ASSERT_EQ(z_time_format_iso(ms, buf, sizeof(buf)), Z_OK);

    time_t seconds = static_cast<time_t>(ms / 1000);
    int millis = static_cast<int>(ms % 1000);
    struct tm tmv;
#ifdef _WIN32
    ASSERT_EQ(localtime_s(&tmv, &seconds), 0);
#else
    ASSERT_NE(localtime_r(&seconds, &tmv), nullptr);
#endif
    char expected[32];
    std::snprintf(expected, sizeof(expected),
                  "%04d-%02d-%02dT%02d:%02d:%02d.%03d",
                  tmv.tm_year + 1900, tmv.tm_mon + 1, tmv.tm_mday,
                  tmv.tm_hour, tmv.tm_min, tmv.tm_sec, millis);
    EXPECT_STREQ(buf, expected);
}

TEST(Time, FormatIso_FormatPattern) {
    char buf[32] = {0};
    ASSERT_EQ(z_time_format_iso(z_time_now_ms(), buf, sizeof(buf)), Z_OK);
    // "YYYY-MM-DDTHH:MM:SS.mmm" 23 字符
    ASSERT_EQ(std::strlen(buf), 23u);
    EXPECT_EQ(buf[4], '-');
    EXPECT_EQ(buf[7], '-');
    EXPECT_EQ(buf[10], 'T');
    EXPECT_EQ(buf[13], ':');
    EXPECT_EQ(buf[16], ':');
    EXPECT_EQ(buf[19], '.');
}

TEST(Time, FormatIso_BufferTooSmall) {
    char buf[23] = {0};  // 23 字符刚好放不下 '\0'
    EXPECT_EQ(z_time_format_iso(z_time_now_ms(), buf, sizeof(buf)), Z_ERR_OVERFLOW);
}

TEST(Time, FormatIso_BufferExactlyEnough) {
    char buf[24] = {0};  // 23 字符 + '\0' = 24 字节
    EXPECT_EQ(z_time_format_iso(z_time_now_ms(), buf, sizeof(buf)), Z_OK);
}

TEST(Time, FormatIso_NullBuf) {
    EXPECT_EQ(z_time_format_iso(0, nullptr, 0), Z_ERR_INVALID_ARG);
    EXPECT_EQ(z_time_format_iso(0, nullptr, 32), Z_ERR_INVALID_ARG);
}

TEST(Time, FormatIso_NegativeTimestamp) {
    // 1970-01-01 之前 1ms，应该能正确处理（不崩溃）
    char buf[32] = {0};
    int rc = z_time_format_iso(-1, buf, sizeof(buf));
    EXPECT_TRUE(rc == Z_OK || rc == Z_ERR_UNKNOWN);
}

// ============ Sleep ============

TEST(Time, SleepMs_Blocks) {
    int64_t before = z_time_now_ms();
    z_time_sleep_ms(50);
    int64_t after = z_time_now_ms();
    // Windows Sleep 精度有限，放宽到 30ms
    EXPECT_GE(after - before, 30);
}

TEST(Time, SleepUs_SubMillisecond) {
    int64_t before = z_time_now_us();
    z_time_sleep_us(500);  // 500us
    int64_t after = z_time_now_us();
    // 忙等精度较高，500us 应该能稳定达到（允许 200us 误差）
    EXPECT_GE(after - before, 200);
}

TEST(Time, SleepUs_Millisecond) {
    int64_t before = z_time_now_us();
    z_time_sleep_us(5000);  // 5ms
    int64_t after = z_time_now_us();
    EXPECT_GE(after - before, 4000);  // 允许 1ms 误差
}
