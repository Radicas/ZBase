/**
 * @file file.hpp
 * @author 朱鹏 (Radica Zhu)
 * @date 2026-06-30
 * @version 0.1.0
 * @brief 文件 RAII 包装
 * @details 提供 FileReader（RAII 自动释放）、WriteFile、FileExists、MakeDirs、
 *          DirIterator（RAII 自动关闭）。错误时抛 zbase::Exception。
 */

#pragma once

#include <cstring>
#include <string>
#include <string_view>
#include <vector>

#include "zbase/file.h"
#include "zbase++/error.hpp"

namespace zbase {

/// 文件读取器（RAII，析构自动释放缓冲区）
class FileReader {
 public:
  /// 构造并读取整个文件
  /// @param path 文件路径（UTF-8）
  /// @throws Exception 文件不存在/无权限/IO 错误等
  explicit FileReader(std::string_view path) {
    int err = z_file_read_text(std::string(path).c_str(), &buf_, &len_);
    ThrowIfError(err);
  }
  ~FileReader() { if (buf_) z_file_free(buf_); }
  FileReader(const FileReader&) = delete;
  FileReader& operator=(const FileReader&) = delete;
  FileReader(FileReader&& o) noexcept : buf_(o.buf_), len_(o.len_) { o.buf_ = nullptr; o.len_ = 0; }
  FileReader& operator=(FileReader&& o) noexcept {
    if (this != &o) {
      if (buf_) z_file_free(buf_);
      buf_ = o.buf_; len_ = o.len_;
      o.buf_ = nullptr; o.len_ = 0;
    }
    return *this;
  }
  /// 获取内容视图（不拷贝）
  /// @return string_view，对象存活期间有效
  [[nodiscard]] std::string_view View() const noexcept { return {buf_, len_}; }
  /// 获取内容副本
  /// @return std::string
  [[nodiscard]] std::string Str() const { return {buf_, len_}; }
  /// 获取字节数
  /// @return 文件大小（字节）
  [[nodiscard]] size_t Size() const noexcept { return len_; }
 private:
  char*  buf_ = nullptr;   ///< 缓冲区指针（库内申请）
  size_t len_ = 0;         ///< 缓冲区长度
};

/// 写文件（原子写：临时文件 + rename）
/// @param path 文件路径（UTF-8）
/// @param content 内容
/// @throws Exception IO 错误等
inline void WriteFile(std::string_view path, std::string_view content) {
    int err = z_file_write_text(std::string(path).c_str(),
                                content.data(), content.size());
    ThrowIfError(err);
}

/// 判断文件/目录是否存在
/// @param path 路径
/// @return true 存在；false 不存在
/// @throws Exception IO 错误（非"不存在"错误）
inline bool FileExists(std::string_view path) {
    int r = z_file_exists(std::string(path).c_str());
    if (r < 0) ThrowIfError(r);
    return r == 1;
}

/// 递归创建目录（已存在不报错）
/// @param path 目录路径
/// @throws Exception 权限错误等
inline void MakeDirs(std::string_view path) {
    int err = z_file_mkdir(std::string(path).c_str());
    ThrowIfError(err);
}

/// 目录条目（值类型，next 后立即拷贝）
struct DirEntry {
    std::string name;   ///< 条目名（UTF-8）
    bool        is_dir; ///< 是否目录
    uint64_t    size;   ///< 字节数
    uint64_t    mtime;  ///< 修改时间（毫秒，epoch）
};

/// 目录迭代器（RAII，析构自动关闭）
class DirIterator {
 public:
  /// 构造并打开目录
  /// @param path 目录路径
  /// @throws Exception 路径不存在/非目录
  explicit DirIterator(std::string_view path) {
    int err = z_dir_iter_create(std::string(path).c_str(), &iter_);
    ThrowIfError(err);
  }
  ~DirIterator() { if (iter_) z_dir_iter_destroy(iter_); }
  DirIterator(const DirIterator&) = delete;
  DirIterator& operator=(const DirIterator&) = delete;
  DirIterator(DirIterator&& o) noexcept : iter_(o.iter_) { o.iter_ = nullptr; }

  /// 读取下一个条目
  /// @param out 输出条目（值拷贝）
  /// @return true 还有条目；false 迭代结束
  /// @throws Exception IO 错误
  bool Next(DirEntry* out) {
    z_dir_iter_entry_t e;
    std::memset(&e, 0, sizeof(e));
    int err = z_dir_iter_next(iter_, &e);
    if (err == Z_ERR_NOT_FOUND) return false;
    ThrowIfError(err);
    out->name   = e.name ? e.name : "";
    out->is_dir = e.is_dir != 0;
    out->size   = e.size;
    out->mtime  = e.mtime;
    return true;
  }
 private:
  z_dir_iter* iter_ = nullptr;   ///< 迭代器不透明句柄
};

}  // namespace zbase
