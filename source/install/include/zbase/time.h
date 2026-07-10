/**
 * @file time.h
 * @author 朱鹏 (Radica Zhu)
 * @date 2026-06-29
 * @version 0.1.0
 * @brief 时间工具 C ABI
 * @details 提供 now_ms / now_us / format_iso / sleep_ms / sleep_us 共 5 个函数。
 *          ISO 8601 输出使用本地时区（v0.1，v0.2 可加 UTC 选项）。
 */

#ifndef ZBASE_TIME_H_
#define ZBASE_TIME_H_

#include <stddef.h>
#include <stdint.h>
#include "zbase/zbase.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 墙钟毫秒时间戳（epoch 1970-01-01 UTC）
 * @return 毫秒时间戳
 */
ZBASE_API int64_t z_time_now_ms(void);

/**
 * @brief 墙钟微秒时间戳（epoch 1970-01-01 UTC）
 * @return 微秒时间戳
 */
ZBASE_API int64_t z_time_now_us(void);

/**
 * @brief 格式化时间为 ISO 8601 字符串（本地时区）
 * @param ms 毫秒时间戳
 * @param buf 输出缓冲区
 * @param buf_size 缓冲区大小（至少 24 字节）
 * @return Z_OK 成功；Z_ERR_INVALID_ARG buf 为 NULL 或 size 为 0；Z_ERR_OVERFLOW 缓冲区不足
 * @note 输出形如 "2026-06-29T12:34:56.789"（23 字符 + '\0'）
 */
ZBASE_API int z_time_format_iso(int64_t ms, char* buf, size_t buf_size);

/**
 * @brief 休眠指定毫秒
 * @param ms 毫秒数
 */
ZBASE_API void z_time_sleep_ms(uint32_t ms);

/**
 * @brief 休眠指定微秒
 * @param us 微秒数
 * @note Windows 上小于 1000us 走忙等；POSIX 走 nanosleep
 */
ZBASE_API void z_time_sleep_us(uint64_t us);

#ifdef __cplusplus
}
#endif

#endif  // ZBASE_TIME_H_
