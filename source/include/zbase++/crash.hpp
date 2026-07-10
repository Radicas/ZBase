/**
 * @file crash.hpp
 * @author 朱鹏 (Radica Zhu)
 * @date 2026-07-10
 * @version 0.2.0
 * @brief 崩溃堆栈捕获 C++ RAII 包装
 * @details 构造时安装崩溃处理器，析构时自动卸载。
 *          用法示例：
 *          @code
 *          int main() {
 *              zbase::CrashHandler crash({"logs/", "my_app"});
 *              // ... 业务代码 ...
 *              return 0;
 *          }  // 析构自动卸载
 *          @endcode
 */

#pragma once

#include <string>
#include <string_view>

#include "zbase/crash.h"
#include "zbase++/error.hpp"

namespace zbase {

/**
 * @brief 崩溃处理器 RAII 包装
 * @details 构造时自动调用 z_crash_install()，析构时自动调用 z_crash_uninstall()。
 *          不可拷贝，不可移动（单例语义）。
 */
class CrashHandler {
 public:
  /**
   * @brief 构造并安装崩溃处理器
   * @param output_dir 崩溃文件输出目录（空字符串表示可执行文件所在目录）
   * @param app_name 应用程序名称（空字符串默认为 "zbase_app"）
   * @throws zbase::Exception 安装失败时抛出
   */
  explicit CrashHandler(std::string_view output_dir = "",
                        std::string_view app_name = "")
  {
    z_crash_config_t config;
    config.output_dir = output_dir.empty() ? nullptr : output_dir.data();
    config.app_name   = app_name.empty() ? nullptr : app_name.data();
    int err = z_crash_install(&config);
    if (err != Z_OK) ThrowIfError(err);
    installed_ = true;
  }

  /// 析构时卸载崩溃处理器
  ~CrashHandler() noexcept
  {
    if (installed_) z_crash_uninstall();
  }

  // 禁止拷贝
  CrashHandler(const CrashHandler&) = delete;
  CrashHandler& operator=(const CrashHandler&) = delete;

  // 禁止移动（单例语义）
  CrashHandler(CrashHandler&&) = delete;
  CrashHandler& operator=(CrashHandler&&) = delete;

 private:
  bool installed_ = false;  ///< 是否已安装
};

}  // namespace zbase
