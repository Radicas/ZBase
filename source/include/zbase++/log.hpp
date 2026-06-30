/**
 * @file log.hpp
 * @author 朱鹏 (Radica Zhu)
 * @date 2026-06-30
 * @version 0.2.0
 * @brief log C++ 包装
 * @details 提供 Logger（RAII 自动 shutdown）+ LogFile（RAII 自动关闭文件输出）。
 *          日志写入仍用 Z_LOG_* 宏。
 */

#pragma once

#include <string>
#include <string_view>

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

/// 日志文件输出 RAII 包装（构造时 open，析构时 close）
/// 日志同时输出到 stderr 和文件；超 max_size 字节滚动为 path.1 path.2 ...
class LogFile {
 public:
  /// 打开日志文件
  /// @param path 日志文件路径（UTF-8，支持中文）
  /// @param max_size 单文件最大字节数（0 = 不滚动）
  /// @param max_files 保留历史文件数（0~99，0 视为 1）
  /// @throws zbase::Exception 失败时抛出错误码
  LogFile(std::string_view path, uint64_t max_size = 0, uint32_t max_files = 0) {
    int err = z_log_file_open(path.data(), max_size, max_files);
    if (err != Z_OK) throw Exception(err, std::string{path});
  }
  ~LogFile() { z_log_file_close(); }
  LogFile(const LogFile&) = delete;
  LogFile& operator=(const LogFile&) = delete;
};

}  // namespace zbase
