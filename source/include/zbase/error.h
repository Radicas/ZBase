/**
 * @file error.h
 * @author 朱鹏 (Radica Zhu)
 * @date 2026-06-29
 * @version 0.1.0
 * @brief 错误码体系 C ABI
 * @details 详见 architecture.md §9。所有 C ABI 函数返回 int，0 表示成功，负数表示错误。
 */

#ifndef ZBASE_ERROR_H_
#define ZBASE_ERROR_H_

#include "zbase/zbase.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 错误码枚举
 * @note sizeof(z_error_t) 必须等于 sizeof(int)，详见文件末 static_assert
 */
typedef enum {
    Z_OK               = 0,   ///< 成功
    Z_ERR_INVALID_ARG  = -1,  ///< 参数非法
    Z_ERR_NOT_FOUND    = -2,  ///< 找不到
    Z_ERR_PERMISSION   = -3,  ///< 权限不足
    Z_ERR_IO          = -4,   ///< IO 错误
    Z_ERR_ENCODING    = -5,   ///< 编码错误
    Z_ERR_NOMEM       = -6,   ///< 内存不足
    Z_ERR_OVERFLOW    = -7,   ///< 溢出
    Z_ERR_UNSUPPORTED = -8,   ///< 不支持
    Z_ERR_UNKNOWN     = -99,  ///< 未知错误
} z_error_t;

/**
 * @brief 获取错误码对应的中文描述
 * @param err 错误码
 * @return 静态字符串指针，不需要释放
 */
ZBASE_API const char* z_error_msg(int err);

#ifdef __cplusplus
}  // extern "C"

#include <type_traits>
static_assert(sizeof(z_error_t) == sizeof(int),
              "z_error_t 必须是 int 大小（详见 architecture.md §9）");
#else
#include <assert.h>
_Static_assert(sizeof(z_error_t) == sizeof(int),
               "z_error_t 必须是 int 大小");
#endif

#endif  // ZBASE_ERROR_H_
