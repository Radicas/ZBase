/**
 * @file log.h
 * @author 朱鹏 (Radica Zhu)
 * @date 2026-06-30
 * @version 0.2.0
 * @brief 日志 C ABI（v0.2：增加文件输出 + 滚动）
 * @details 提供 4 级日志（DEBUG/INFO/WARN/ERROR）+ init/shutdown + write。
 *          Windows 下 init 会保存原控制台 CP、设置 UTF-8 输出、限制 DLL 搜索路径。
 *          输出格式：[时间] [级别] [文件:行号] 用户消息
 *          v0.2 起 z_log_file_open 后，z_log_write 同时输出到 stderr 与文件；
 *          单文件超 max_size 字节时滚动为 path.1 path.2 ... path.max_files。
 */

#ifndef ZBASE_LOG_H_
#define ZBASE_LOG_H_

#include <stdint.h>

#include "zbase/zbase.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 日志级别枚举
 */
typedef enum {
    Z_LOG_LEVEL_DEBUG = 0,  ///< 调试
    Z_LOG_LEVEL_INFO  = 1,  ///< 信息
    Z_LOG_LEVEL_WARN  = 2,  ///< 警告
    Z_LOG_LEVEL_ERROR = 3,  ///< 错误
} z_log_level_t;

/**
 * @brief 初始化日志系统
 * @param level 最低输出级别（低于该级别的日志将被丢弃）
 * @return Z_OK 成功
 * @note Windows 下会：① 保存原控制台 CP ② SetConsoleOutputCP(CP_UTF8)
 *       ③ 调 SetDefaultDllDirectories 限制 DLL 搜索路径。
 *       可重复调用；shutdown 后可再次 init。
 */
ZBASE_API int z_log_init(z_log_level_t level);

/**
 * @brief 关闭日志，恢复控制台 CP（Windows）
 */
ZBASE_API void z_log_shutdown(void);

/**
 * @brief 写日志（内部接口，建议用 Z_LOG_* 宏）
 * @param level 日志级别
 * @param file 源文件路径（一般传 __FILE__）
 * @param line 源文件行号（一般传 __LINE__）
 * @param fmt printf 风格格式串
 * @param ... 格式参数
 * @note 若已调用 z_log_file_open，则同时输出到 stderr 与文件；
 *       文件写入失败时静默丢弃（不影响 stderr）
 */
ZBASE_API void z_log_write(z_log_level_t level, const char* file, int line,
                           const char* fmt, ...);

/**
 * @brief 打开日志文件输出（同时输出到 stderr 和文件）
 * @param path 日志文件路径（UTF-8，支持中文）
 * @param max_size 单文件最大字节数，超过则滚动（0 表示不滚动，无上限）
 * @param max_files 保留的历史文件数（1~99，超出部分删除最旧的；0 等同于 1）
 * @return Z_OK 成功；Z_ERR_INVALID_ARG path 为 NULL 或 max_files > 99；
 *         Z_ERR_IO 文件打开失败
 * @note 滚动命名规则：path（当前）→ path.1（次新）→ path.2 → ... → path.N（最旧）。
 *       再次调用会先关闭旧文件再打开新文件。
 */
ZBASE_API int z_log_file_open(const char* path, uint64_t max_size, uint32_t max_files);

/**
 * @brief 关闭日志文件输出（stderr 仍继续）
 * @note 不删除已写入的文件；可重复调用
 */
ZBASE_API void z_log_file_close(void);

#ifdef __cplusplus
}  // extern "C"
#endif

/// @cond INTERNAL
#define Z_LOG_DEBUG(fmt, ...) z_log_write(Z_LOG_LEVEL_DEBUG, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define Z_LOG_INFO(fmt, ...)  z_log_write(Z_LOG_LEVEL_INFO,  __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define Z_LOG_WARN(fmt, ...)  z_log_write(Z_LOG_LEVEL_WARN,  __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define Z_LOG_ERROR(fmt, ...) z_log_write(Z_LOG_LEVEL_ERROR, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
/// @endcond

#endif  // ZBASE_LOG_H_
