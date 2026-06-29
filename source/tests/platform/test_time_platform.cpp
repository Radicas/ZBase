/**
 * @file test_time_platform.cpp
 * @author 朱鹏 (Radica Zhu)
 * @date 2026-06-29
 * @version 0.1.0
 * @brief 时间平台原语测试
 * @details 覆盖单调时钟单调性、墙钟合理范围、休眠精度。
 */

#include <gtest/gtest.h>
#include "platform/time_platform.hpp"

#include <chrono>
#include <thread>

using zbase::platform::MonoNowNs;
using zbase::platform::SleepMs;
using zbase::platform::WallNowMs;

TEST(TimePlatform, MonoNow_Increases) {
    uint64_t t1 = MonoNowNs();
    uint64_t t2 = MonoNowNs();
    EXPECT_GE(t2, t1);
}

TEST(TimePlatform, MonoNow_MonotonicAcrossSleep) {
    uint64_t t1 = MonoNowNs();
    SleepMs(20);
    uint64_t t2 = MonoNowNs();
    EXPECT_GT(t2, t1);
}

TEST(TimePlatform, WallNow_RecentEpoch) {
    int64_t ms = WallNowMs();
    // 2023-11 之后 ~ 2033-05 之前
    EXPECT_GT(ms, 1700000000000LL);
    EXPECT_LT(ms, 2000000000000LL);
}

TEST(TimePlatform, SleepMs_ActuallySleeps) {
    uint64_t before = MonoNowNs();
    SleepMs(50);
    uint64_t after = MonoNowNs();
    // Sleep 精度有限，下限放宽到 30ms（Windows 默认 timer granularity ~15.6ms）
    EXPECT_GE(after - before, 30ULL * 1000000ULL);
}

TEST(TimePlatform, WallNow_MatchesStdChrono) {
    namespace ch = std::chrono;
    int64_t zbase_ms = WallNowMs();
    auto now = ch::system_clock::now();
    auto cpp_ms = ch::duration_cast<ch::milliseconds>(
        now.time_since_epoch()).count();
    // 两者差距应在 2 秒内
    int64_t diff = zbase_ms - cpp_ms;
    if (diff < 0) diff = -diff;
    EXPECT_LT(diff, 2000LL);
}
