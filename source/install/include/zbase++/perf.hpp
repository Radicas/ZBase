/**
 * @file perf.hpp
 * @author 朱鹏 (Radica Zhu)
 * @date 2026-06-30
 * @version 0.1.0
 * @brief perf C++ 包装
 * @details 提供 PerfScope（RAII 计时）、PerfDump、PerfReset。
 *          错误时抛 zbase::Exception。
 */

#pragma once

#include <string>
#include <string_view>

#include "zbase/perf.h"
#include "zbase++/error.hpp"

namespace zbase {

/// 性能打点 RAII 包装（构造时 start，析构时 end）
class PerfScope {
 public:
  /// 构造并开始打点
  /// @param name 打点名
  /// @throws Exception start 失败
  explicit PerfScope(std::string_view name) : name_(name) {
    ThrowIfError(z_perf_start(name_.c_str()));
  }
  ~PerfScope() { z_perf_end(name_.c_str()); }
  PerfScope(const PerfScope&) = delete;
  PerfScope& operator=(const PerfScope&) = delete;
 private:
  std::string name_;   ///< 打点名（持有，保证 c_str() 在生命周期内有效）
};

/// 输出所有打点到 stderr（按总耗时降序）
inline void PerfDump() { z_perf_dump(); }

/// 清空所有打点
inline void PerfReset() { z_perf_reset(); }

}  // namespace zbase

/// 作用域打点宏（C++ 用）
/// @param name 打点名
#define ZBASE_PERF_SCOPE(name) ::zbase::PerfScope _zbase_perf_##__LINE__(name)
