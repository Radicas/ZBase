/**
 * @file string.cpp
 * @author 朱鹏 (Radica Zhu)
 * @date 2026-06-29
 * @version 0.1.0
 * @brief 字符串工具实现
 * @details v0.1 直接用 std::string 实现 C ABI（YAGNI，未分离 core/c_api 两层，
 *          参考 phase-1-working-file.md §4.28）。所有返回字符串都通过 malloc
 *          分配，调用方用 z_string_free / z_string_free_array 释放。
 */

#include "zbase/string.h"
#include "zbase/error.h"
#include "zbase/internal/c_api_guard.hpp"

#include <cctype>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

namespace {

/// 在堆上分配并复制字符串，返回 char*（调方需用 z_string_free 释放）
/// @param s 源字符串
/// @return 新分配的 char*；内存不足返回 nullptr
char* DupString(const std::string& s) {
    char* p = static_cast<char*>(std::malloc(s.size() + 1));
    if (p == nullptr) return nullptr;
    std::memcpy(p, s.data(), s.size());
    p[s.size()] = '\0';
    return p;
}

}  // namespace

extern "C" {

int z_string_split(const char* input, size_t input_len,
                   const char* sep,
                   char*** out_arr, size_t* out_count) {
    ZBASE_C_API_BEGIN
    if (input == nullptr || sep == nullptr || out_arr == nullptr || out_count == nullptr) {
        return Z_ERR_INVALID_ARG;
    }
    std::string sep_str(sep);
    if (sep_str.empty()) {
        return Z_ERR_INVALID_ARG;
    }
    std::string s(input, input_len);
    std::vector<std::string> parts;
    size_t start = 0;
    while (true) {
        size_t pos = s.find(sep_str, start);
        if (pos == std::string::npos) {
            parts.emplace_back(s.substr(start));
            break;
        }
        parts.emplace_back(s.substr(start, pos - start));
        start = pos + sep_str.size();
    }
    char** arr = static_cast<char**>(std::calloc(parts.size(), sizeof(char*)));
    if (arr == nullptr) return Z_ERR_NOMEM;
    for (size_t i = 0; i < parts.size(); ++i) {
        arr[i] = DupString(parts[i]);
        if (arr[i] == nullptr) {
            z_string_free_array(arr, i);
            return Z_ERR_NOMEM;
        }
    }
    *out_arr = arr;
    *out_count = parts.size();
    return Z_OK;
    ZBASE_C_API_END(Z_ERR_UNKNOWN)
}

int z_string_join(char** arr, size_t count, const char* sep, char** out) {
    ZBASE_C_API_BEGIN
    if (sep == nullptr || out == nullptr) {
        return Z_ERR_INVALID_ARG;
    }
    // count=0 时 arr 允许为 nullptr（视为空数组，输出空串）
    if (count > 0 && arr == nullptr) {
        return Z_ERR_INVALID_ARG;
    }
    std::string result;
    std::string sep_str(sep);
    for (size_t i = 0; i < count; ++i) {
        if (i > 0) result += sep_str;
        if (arr[i] != nullptr) result += arr[i];
    }
    char* p = DupString(result);
    if (p == nullptr) return Z_ERR_NOMEM;
    *out = p;
    return Z_OK;
    ZBASE_C_API_END(Z_ERR_UNKNOWN)
}

int z_string_replace(const char* input, const char* from,
                     const char* to, char** out) {
    ZBASE_C_API_BEGIN
    if (input == nullptr || from == nullptr || to == nullptr || out == nullptr) {
        return Z_ERR_INVALID_ARG;
    }
    std::string from_str(from);
    if (from_str.empty()) return Z_ERR_INVALID_ARG;
    std::string s(input);
    std::string to_str(to);
    size_t pos = 0;
    while ((pos = s.find(from_str, pos)) != std::string::npos) {
        s.replace(pos, from_str.size(), to_str);
        pos += to_str.size();
    }
    char* p = DupString(s);
    if (p == nullptr) return Z_ERR_NOMEM;
    *out = p;
    return Z_OK;
    ZBASE_C_API_END(Z_ERR_UNKNOWN)
}

int z_string_trim(const char* input, char** out) {
    ZBASE_C_API_BEGIN
    if (input == nullptr || out == nullptr) return Z_ERR_INVALID_ARG;
    std::string s(input);
    auto is_space = [](unsigned char c) { return std::isspace(c) != 0; };
    size_t b = 0, e = s.size();
    while (b < e && is_space(static_cast<unsigned char>(s[b]))) ++b;
    while (e > b && is_space(static_cast<unsigned char>(s[e - 1]))) --e;
    std::string trimmed = s.substr(b, e - b);
    char* p = DupString(trimmed);
    if (p == nullptr) return Z_ERR_NOMEM;
    *out = p;
    return Z_OK;
    ZBASE_C_API_END(Z_ERR_UNKNOWN)
}

int z_string_tolower(const char* input, char** out) {
    ZBASE_C_API_BEGIN
    if (input == nullptr || out == nullptr) return Z_ERR_INVALID_ARG;
    std::string s(input);
    for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    char* p = DupString(s);
    if (p == nullptr) return Z_ERR_NOMEM;
    *out = p;
    return Z_OK;
    ZBASE_C_API_END(Z_ERR_UNKNOWN)
}

int z_string_toupper(const char* input, char** out) {
    ZBASE_C_API_BEGIN
    if (input == nullptr || out == nullptr) return Z_ERR_INVALID_ARG;
    std::string s(input);
    for (char& c : s) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    char* p = DupString(s);
    if (p == nullptr) return Z_ERR_NOMEM;
    *out = p;
    return Z_OK;
    ZBASE_C_API_END(Z_ERR_UNKNOWN)
}

void z_string_free(char* s) {
    if (s == nullptr) return;
    std::free(s);
}

void z_string_free_array(char** arr, size_t count) {
    if (arr == nullptr) return;
    for (size_t i = 0; i < count; ++i) {
        z_string_free(arr[i]);
    }
    std::free(arr);
}

}  // extern "C"
