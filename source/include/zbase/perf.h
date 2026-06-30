/**
 * @file perf.h
 * @author 朱鹏 (Radica Zhu)
 * @date 2026-06-30
 * @version 0.1.0
 * @brief 性能统计 C ABI
 * @details 提供命名打点的 start/end、累计统计、dump 输出与 reset 清空。
 *          线程安全（内部加锁）；同名打点支持多次累计；按总耗时降序输出。
 */

#ifndef ZBASE_PERF_H_
#define ZBASE_PERF_H_

#include "zbase/zbase.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 开始命名打点
 * @param name 打点名（建议 ≤ 64 字节，UTF-8）
 * @return Z_OK 成功；Z_ERR_INVALID_ARG name 为 NULL
 */
ZBASE_API int z_perf_start(const char* name);

/**
 * @brief 结束命名打点（累计到同名计数器）
 * @param name 打点名（必须与 start 同一指针或同字符串内容）
 * @return Z_OK 成功；Z_ERR_INVALID_ARG name 为 NULL；Z_ERR_NOT_FOUND 该 name 从未 start 或已 end
 */
ZBASE_API int z_perf_end(const char* name);

/**
 * @brief 输出所有打点到 stderr（按总耗时降序）
 */
ZBASE_API void z_perf_dump(void);

/**
 * @brief 清空所有打点
 */
ZBASE_API void z_perf_reset(void);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // ZBASE_PERF_H_
