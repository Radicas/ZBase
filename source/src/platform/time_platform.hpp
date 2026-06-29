/**
 * @file time_platform.hpp
 * @author 朱鹏 (Radica Zhu)
 * @date 2026-06-29
 * @version 0.1.0
 * @brief 时间平台原语（内部）
 * @details Windows 走 QueryPerformanceCounter（单调）+ GetSystemTimeAsFileTime（墙钟）；
 *          POSIX 走 clock_gettime(CLOCK_MONOTONIC/CLOCK_REALTIME)。
 */

#ifndef ZBASE_PLATFORM_TIME_PLATFORM_HPP_
#define ZBASE_PLATFORM_TIME_PLATFORM_HPP_

#include <cstdint>

namespace zbase {
namespace platform {

/**
 * @brief 获取单调时钟纳秒值（用于 perf 高精度计时）
 * @return 单调时钟纳秒值（单调递增，不受系统时间调整影响）
 */
uint64_t MonoNowNs();

/**
 * @brief 获取墙钟毫秒时间戳（epoch 1970）
 * @return 毫秒时间戳
 */
int64_t WallNowMs();

/**
 * @brief 休眠指定毫秒
 * @param ms 毫秒数
 */
void SleepMs(uint32_t ms);

}  // namespace platform
}  // namespace zbase

#endif  // ZBASE_PLATFORM_TIME_PLATFORM_HPP_
