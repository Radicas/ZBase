/**
 * @file c_api_guard.hpp
 * @author 朱鹏 (Radica Zhu)
 * @date 2026-06-30
 * @version 0.1.0
 * @brief C ABI 异常包裹宏（内部头，不对外发布）
 * @details 详见 phase-1-working-file.md §4.13。所有返回 int 的 C ABI 函数体
 *          必须用此宏包裹，防止 C++ 异常逃逸到 C 调用方（UB）。
 *          用法：
 *          @code
 *          int z_xxx(...) {
 *              ZBASE_C_API_BEGIN
 *              ... 业务逻辑 ...
 *              return Z_OK;
 *              ZBASE_C_API_END(Z_ERR_UNKNOWN)
 *          }
 *          @endcode
 */

#ifndef ZBASE_INTERNAL_C_API_GUARD_HPP_
#define ZBASE_INTERNAL_C_API_GUARD_HPP_

#include <exception>
#include <new>

#include "zbase/error.h"

/// C ABI 函数体起始宏，开 try 块
#define ZBASE_C_API_BEGIN try {

/// C ABI 函数体结束宏，捕获所有异常并返回错误码
/// @param default_err 默认错误码（未知异常时返回）
#define ZBASE_C_API_END(default_err)                                       \
    } catch (const std::bad_alloc&) { return Z_ERR_NOMEM;            } \
      catch (const std::exception&)  { return (default_err);          } \
      catch (...)                    { return (default_err);          }

#endif  // ZBASE_INTERNAL_C_API_GUARD_HPP_
