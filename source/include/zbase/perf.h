/**
 * @file perf.h
 * @author 朱鹏 (Radica Zhu)
 * @date 2026-06-30
 * @version 0.2.0
 * @brief 性能统计 C ABI
 * @details 提供命名打点的 start/end、累计统计、dump 输出与 reset 清空。
 *          线程安全（内部加锁）；同名打点支持多次累计；按总耗时降序输出。
 *          v0.2.0 新增 z_perf_iterate：以结构化数据回调方式暴露打点统计，
 *          上层可自行决定消费方式（写文件、走 log、转 CSV/JSON 等）。
 */

#ifndef ZBASE_PERF_H_
#define ZBASE_PERF_H_

#include <stdint.h>

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
 * @details 内部基于 z_perf_iterate 实现，格式化逻辑由库持有。
 *          若需自定义输出目标（文件、log、CSV 等），请直接使用 z_perf_iterate。
 */
ZBASE_API void z_perf_dump(void);

/**
 * @brief 清空所有打点
 */
ZBASE_API void z_perf_reset(void);

/**
 * @brief 访问单条打点统计的回调函数类型
 * @param name 打点名（UTF-8，回调执行期间有效；回调返回后该指针可能失效，调用方如需保留请自行复制）
 * @param total_ns 该打点累计耗时（纳秒）
 * @param count 该打点累计调用次数
 * @param user_data 调用 z_perf_iterate 时透传的上下文指针
 */
typedef void (*z_perf_visit_fn)(const char* name, uint64_t total_ns,
                                uint64_t count, void* user_data);

/**
 * @brief 遍历所有打点统计（按总耗时降序）
 * @param visit 访问回调，每条打点调用一次；为 NULL 时直接返回（无操作）
 * @param user_data 透传给回调的上下文指针，可为 NULL
 * @details 上层可在回调中自行决定如何消费数据：写文件、走 z_log_write、
 *          转 CSV/JSON 等。回调内若抛出 C++ 异常，将被库内部静默捕获，
 *          不会穿越 C ABI 边界（保证调用方为 C 程序时的安全性）。
 * @note 回调执行期间库持有内部互斥锁，回调内禁止调用 z_perf_start/end/dump/reset，
 *       否则会发生死锁。
 */
ZBASE_API void z_perf_iterate(z_perf_visit_fn visit, void* user_data);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // ZBASE_PERF_H_
