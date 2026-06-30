/**
 * @file log.h
 * @author 朱鹏 (Radica Zhu)
 * @date 2026-06-30
 * @version 0.1.0
 * @brief 日志 C ABI（最小版）
 * @details 提供 4 级日志（DEBUG/INFO/WARN/ERROR）+ init/shutdown + write。
 *          Windows 下 init 会保存原控制台 CP、设置 UTF-8 输出、限制 DLL 搜索路径。
 *          输出格式：[时间] [级别] [文件:行号] 用户消息
 */

#ifndef ZBASE_LOG_H_
#define ZBASE_LOG_H_

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
 */
ZBASE_API void z_log_write(z_log_level_t level, const char* file, int line,
                           const char* fmt, ...);

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
