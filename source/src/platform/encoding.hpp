/**
 * @file encoding.hpp
 * @author 朱鹏 (Radica Zhu)
 * @date 2026-06-29
 * @version 0.1.0
 * @brief UTF-8 ↔ UTF-16 转换内部接口（平台层，不对外导出）
 * @details Windows 走 MultiByteToWideChar；其他平台走纯算法实现（RFC 3629）。
 */

#ifndef ZBASE_PLATFORM_ENCODING_HPP_
#define ZBASE_PLATFORM_ENCODING_HPP_

#include <cstdint>
#include <string>

#include "zbase/error.h"

namespace zbase {
namespace platform {

/**
 * @brief UTF-8 字符串转 UTF-16
 * @param in 输入 UTF-8 字符串
 * @param out 输出 UTF-16 字符串（库内追加，不清空）
 * @return Z_OK 成功；Z_ERR_ENCODING 输入非法 UTF-8；Z_ERR_NOMEM 内存不足
 */
int Utf8ToUtf16(const std::string& in, std::u16string& out);

/**
 * @brief UTF-16 字符串转 UTF-8
 * @param in 输入 UTF-16 字符串
 * @param out 输出 UTF-8 字符串
 * @return Z_OK 成功；Z_ERR_ENCODING 输入非法；Z_ERR_NOMEM 内存不足
 */
int Utf16ToUtf8(const std::u16string& in, std::string& out);

/**
 * @brief UTF-8 字符串转 std::wstring（Windows 下为 UTF-16，POSIX 下为 UTF-32）
 * @param in 输入 UTF-8
 * @param out 输出 wstring
 * @return Z_OK 成功；否则错误码
 * @note Windows 上等价于 Utf8ToUtf16；POSIX 上走 UTF-32 路径
 */
int Utf8ToWString(const std::string& in, std::wstring& out);

/**
 * @brief std::wstring 转 UTF-8（Utf8ToWString 的逆操作）
 * @param in 输入 wstring
 * @param out 输出 UTF-8
 * @return Z_OK 成功；否则错误码
 */
int WStringToUtf8(const std::wstring& in, std::string& out);

}  // namespace platform
}  // namespace zbase

#endif  // ZBASE_PLATFORM_ENCODING_HPP_
