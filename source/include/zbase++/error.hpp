/**
 * @file error.hpp
 * @author 朱鹏 (Radica Zhu)
 * @date 2026-06-30
 * @version 0.1.0
 * @brief C++ 异常 + 错误码工具
 * @details 提供 zbase::Exception 异常基类与 ThrowIfError 工具函数，
 *          供其它 C++ 包装头使用。C ABI 错误码转异常的统一入口。
 */

#pragma once

#include <stdexcept>
#include <string>

#include "zbase/error.h"

namespace zbase {

/// ZBase 异常基类，所有 C++ 包装层在 C ABI 返回错误时抛出
class Exception : public std::runtime_error {
 public:
  /// 用错误码构造，错误描述取自 z_error_msg
  explicit Exception(int err)
      : std::runtime_error(z_error_msg(err)), err_(err) {}
  /// 用错误码 + 附加信息构造
  Exception(int err, const std::string& extra)
      : std::runtime_error(std::string(z_error_msg(err)) + ": " + extra), err_(err) {}
  /// 获取错误码
  int code() const noexcept { return err_; }
 private:
  int err_;   ///< 错误码
};

/// 把错误码转为异常抛出（err != Z_OK 时抛）
/// @param err C ABI 返回的错误码
inline void ThrowIfError(int err) {
  if (err != Z_OK) throw Exception(err);
}

}  // namespace zbase
