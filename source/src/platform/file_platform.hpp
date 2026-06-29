/**
 * @file file_platform.hpp
 * @author 朱鹏 (Radica Zhu)
 * @date 2026-06-29
 * @version 0.1.0
 * @brief 文件操作平台原语（内部，不对外导出）
 * @details Windows 走 W 版 Win32 API（CreateFileW/GetFileAttributesW 等），
 *          路径长度 > 248 时自动加 \\?\ 前缀以支持长路径（详见
 *          phase-1-working-file.md §4.23）。POSIX 走 sys/stat.h + dirent.h。
 */

#ifndef ZBASE_PLATFORM_FILE_PLATFORM_HPP_
#define ZBASE_PLATFORM_FILE_PLATFORM_HPP_

#include <cstdint>
#include <string>

#include "zbase/error.h"

namespace zbase {
namespace platform {

/// 文件属性结构
struct FileStat {
    uint64_t size;      ///< 字节数
    uint64_t mtime;     ///< 修改时间（毫秒 epoch）
    bool     is_dir;    ///< 是否目录
    bool     exists;    ///< 是否存在
};

/**
 * @brief 将 UTF-8 路径转为 Windows 可用的 UTF-16 路径，自动处理 \\?\ 长路径前缀
 * @param utf8_path 输入 UTF-8 路径
 * @param out 输出 std::wstring（Windows）或 std::string（POSIX）
 * @return Z_OK 成功；Z_ERR_ENCODING 编码错误
 */
int NormalizePath(const std::string& utf8_path, std::wstring& out);

/**
 * @brief 同上的 POSIX 重载（直接拷贝）
 */
int NormalizePath(const std::string& utf8_path, std::string& out);

/**
 * @brief 查询文件/目录信息
 * @param utf8_path 输入 UTF-8 路径
 * @param out 输出属性结构（不存在时 exists=false）
 * @return Z_OK 成功；Z_ERR_PERMISSION；Z_ERR_IO
 */
int FileStatGet(const std::string& utf8_path, FileStat& out);

/**
 * @brief 创建目录（递归）
 * @param utf8_path 输入 UTF-8 路径
 * @return Z_OK 成功（含已存在）；Z_ERR_PERMISSION；Z_ERR_IO
 */
int MakeDirs(const std::string& utf8_path);

}  // namespace platform
}  // namespace zbase

#endif  // ZBASE_PLATFORM_FILE_PLATFORM_HPP_
