/**
 * @file log.hpp
 * @author 朱鹏 (Radica Zhu)
 * @date 2026-06-30
 * @version 0.1.0
 * @brief log C++ 包装
 * @details 提供 Logger（RAII 自动 shutdown）。日志写入仍用 Z_LOG_* 宏。
 */

#pragma once

#include "zbase/log.h"

namespace zbase {

/// 日志系统 RAII 包装（构造时 init，析构时 shutdown）
class Logger {
 public:
  /// 构造并初始化日志系统
  /// @param level 最低输出级别
  explicit Logger(z_log_level_t level = Z_LOG_LEVEL_INFO) {
    z_log_init(level);
  }
  ~Logger() { z_log_shutdown(); }
  Logger(const Logger&) = delete;
  Logger& operator=(const Logger&) = delete;
};

}  // namespace zbase
