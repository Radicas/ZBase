/**
 * @file config.hpp
 * @author 朱鹏 (Radica Zhu)
 * @date 2026-06-30
 * @version 0.2.0
 * @brief config C++ RAII 包装
 * @details 提供 Config 类（构造时加载，析构时释放）。Get/GetInt/GetBool
 *          返回值类型而非指针，调用方无需关心生命周期。
 */

#pragma once

#include <string>
#include <string_view>

#include "zbase++/error.hpp"
#include "zbase/config.h"

namespace zbase {

/// INI 配置文件 RAII 包装（构造时加载，析构时释放）
class Config {
 public:
  /// 构造并加载配置文件
  /// @param path 配置文件路径（UTF-8）
  /// @throws Exception 文件不存在/无权限/IO 错误等
  explicit Config(std::string_view path) {
    int err = z_config_load(std::string(path).c_str(), &handle_);
    ThrowIfError(err);
  }
  ~Config() { if (handle_) z_config_free(handle_); }
  Config(const Config&) = delete;
  Config& operator=(const Config&) = delete;
  Config(Config&& o) noexcept : handle_(o.handle_) { o.handle_ = nullptr; }
  Config& operator=(Config&& o) noexcept {
    if (this != &o) {
      if (handle_) z_config_free(handle_);
      handle_ = o.handle_;
      o.handle_ = nullptr;
    }
    return *this;
  }

  /// 读取字符串配置项
  /// @param section 节名（空串表示全局节）
  /// @param key 键名
  /// @param default_val 找不到时返回的默认值
  /// @return 配置值或默认值
  std::string Get(std::string_view section, std::string_view key,
                  std::string_view default_val = "") const {
    // 用局部 std::string 持有 c_str()，避免临时对象在 z_config_get 返回前析构
    // （找不到 key 时 z_config_get 返回 default_val 指针本身）
    std::string sec_str{section};
    std::string key_str{key};
    std::string def_str{default_val};
    const char* r = z_config_get(handle_,
                                 section.empty() ? nullptr : sec_str.c_str(),
                                 key_str.c_str(),
                                 def_str.c_str());
    return r == nullptr ? std::string{} : std::string{r};
  }

  /// 读取整数配置项
  /// @param section 节名
  /// @param key 键名
  /// @param default_val 找不到或解析失败时返回
  /// @return 整数值
  int GetInt(std::string_view section, std::string_view key, int default_val = 0) const {
    std::string sec_str{section};
    std::string key_str{key};
    return z_config_get_int(handle_,
                            section.empty() ? nullptr : sec_str.c_str(),
                            key_str.c_str(),
                            default_val);
  }

  /// 读取布尔配置项
  /// @param section 节名
  /// @param key 键名
  /// @param default_val 找不到或解析失败时返回
  /// @return true 为 1；false 为 0
  bool GetBool(std::string_view section, std::string_view key, bool default_val = false) const {
    std::string sec_str{section};
    std::string key_str{key};
    int r = z_config_get_bool(handle_,
                              section.empty() ? nullptr : sec_str.c_str(),
                              key_str.c_str(),
                              default_val ? 1 : 0);
    return r != 0;
  }
 private:
  z_config_t* handle_ = nullptr;   ///< 配置句柄
};

}  // namespace zbase
