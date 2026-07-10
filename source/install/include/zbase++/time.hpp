/**
 * @file time.hpp
 * @author 朱鹏 (Radica Zhu)
 * @date 2026-06-30
 * @version 0.1.0
 * @brief 时间工具 C++ 包装
 * @details 提供 NowMs / NowUs / FormatIso / SleepMs / SleepUs 等，
 *          内部调用 C ABI，format_iso 失败时抛异常。
 */

#pragma once

#include <cstdint>
#include <string>

#include "zbase/time.h"
#include "zbase++/error.hpp"

namespace zbase {

/// 当前墙钟毫秒时间戳
/// @return 毫秒时间戳（epoch 1970-01-01 UTC）
inline int64_t NowMs() { return z_time_now_ms(); }

/// 当前墙钟微秒时间戳
/// @return 微秒时间戳
inline int64_t NowUs() { return z_time_now_us(); }

/// 休眠指定毫秒
/// @param ms 毫秒数
inline void SleepMs(uint32_t ms) { z_time_sleep_ms(ms); }

/// 休眠指定微秒
/// @param us 微秒数
inline void SleepUs(uint64_t us) { z_time_sleep_us(us); }

/// 格式化时间为 ISO 8601 字符串（本地时区）
/// @param ms 毫秒时间戳
/// @return 形如 "2026-06-30T12:34:56.789" 的字符串
inline std::string FormatIso(int64_t ms) {
    char buf[32];
    int err = z_time_format_iso(ms, buf, sizeof(buf));
    ThrowIfError(err);
    return std::string(buf);
}

}  // namespace zbase
