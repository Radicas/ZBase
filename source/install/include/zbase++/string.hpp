/**
 * @file string.hpp
 * @author 朱鹏 (Radica Zhu)
 * @date 2026-06-30
 * @version 0.1.0
 * @brief 字符串工具 C++ 包装
 * @details 提供 std::string / std::vector 风格的接口，内部调用 C ABI。
 *          错误时抛 zbase::Exception；库内申请的内存由 RAII 自动释放。
 */

#pragma once

#include <string>
#include <vector>

#include "zbase/string.h"
#include "zbase++/error.hpp"

namespace zbase {

/// 分隔字符串
/// @param s 输入字符串
/// @param sep 分隔符（不可为空串）
/// @return 分隔结果
inline std::vector<std::string> StringSplit(const std::string& s, const std::string& sep) {
    char** arr = nullptr;
    size_t count = 0;
    int err = z_string_split(s.data(), s.size(), sep.c_str(), &arr, &count);
    ThrowIfError(err);
    std::vector<std::string> result;
    result.reserve(count);
    for (size_t i = 0; i < count; ++i) {
        result.emplace_back(arr[i] ? arr[i] : "");
    }
    z_string_free_array(arr, count);
    return result;
}

/// 拼接字符串数组
/// @param parts 输入字符串数组
/// @param sep 分隔符
/// @return 拼接结果
inline std::string StringJoin(const std::vector<std::string>& parts, const std::string& sep) {
    std::vector<char*> ptrs;
    ptrs.reserve(parts.size());
    for (const auto& p : parts) ptrs.push_back(const_cast<char*>(p.c_str()));
    char* out = nullptr;
    int err = z_string_join(ptrs.data(), ptrs.size(), sep.c_str(), &out);
    ThrowIfError(err);
    std::string result(out);
    z_string_free(out);
    return result;
}

/// 替换字符串中所有匹配的子串
/// @param s 输入
/// @param from 待替换子串（不可为空串）
/// @param to 替换为
/// @return 替换结果
inline std::string StringReplace(const std::string& s, const std::string& from, const std::string& to) {
    char* out = nullptr;
    int err = z_string_replace(s.c_str(), from.c_str(), to.c_str(), &out);
    ThrowIfError(err);
    std::string result(out);
    z_string_free(out);
    return result;
}

/// 去除首尾空白字符
/// @param s 输入
/// @return 去除空白后的字符串
inline std::string StringTrim(const std::string& s) {
    char* out = nullptr;
    int err = z_string_trim(s.c_str(), &out);
    ThrowIfError(err);
    std::string result(out);
    z_string_free(out);
    return result;
}

/// 转小写（仅 ASCII 字母）
/// @param s 输入
/// @return 小写字符串
inline std::string StringToLower(const std::string& s) {
    char* out = nullptr;
    int err = z_string_tolower(s.c_str(), &out);
    ThrowIfError(err);
    std::string result(out);
    z_string_free(out);
    return result;
}

/// 转大写（仅 ASCII 字母）
/// @param s 输入
/// @return 大写字符串
inline std::string StringToUpper(const std::string& s) {
    char* out = nullptr;
    int err = z_string_toupper(s.c_str(), &out);
    ThrowIfError(err);
    std::string result(out);
    z_string_free(out);
    return result;
}

/// 计算 UTF-8 字符串的字符数
/// @param s 输入
/// @return 字符数
inline size_t StringUtf8Len(const std::string& s) {
    return z_string_utf8_len(s.c_str());
}

}  // namespace zbase
