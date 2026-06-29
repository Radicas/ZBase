/**
 * @file encoding.cpp
 * @author 朱鹏 (Radica Zhu)
 * @date 2026-06-29
 * @version 0.1.0
 * @brief UTF-8 ↔ UTF-16 转换实现
 */

#include "platform/encoding.hpp"

#ifdef _WIN32
#include <Windows.h>
#else
#include <cstdint>
#endif

namespace zbase {
namespace platform {

#ifdef _WIN32
// Windows: 走 MultiByteToWideChar / WideCharToMultiByte

int Utf8ToUtf16(const std::string& in, std::u16string& out) {
    if (in.empty()) {
        out.clear();
        return Z_OK;
    }
    int len = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
                                  in.data(), static_cast<int>(in.size()),
                                  nullptr, 0);
    if (len == 0) {
        return (GetLastError() == ERROR_NO_UNICODE_TRANSLATION)
               ? Z_ERR_ENCODING : Z_ERR_UNKNOWN;
    }
    out.resize(static_cast<size_t>(len));
    MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
                        in.data(), static_cast<int>(in.size()),
                        reinterpret_cast<LPWSTR>(&out[0]), len);
    return Z_OK;
}

int Utf16ToUtf8(const std::u16string& in, std::string& out) {
    if (in.empty()) {
        out.clear();
        return Z_OK;
    }
    int len = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS,
                                  reinterpret_cast<LPCWSTR>(in.data()),
                                  static_cast<int>(in.size()),
                                  nullptr, 0, nullptr, nullptr);
    if (len == 0) {
        return (GetLastError() == ERROR_NO_UNICODE_TRANSLATION)
               ? Z_ERR_ENCODING : Z_ERR_UNKNOWN;
    }
    out.resize(static_cast<size_t>(len));
    WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS,
                        reinterpret_cast<LPCWSTR>(in.data()),
                        static_cast<int>(in.size()),
                        &out[0], len, nullptr, nullptr);
    return Z_OK;
}

int Utf8ToWString(const std::string& in, std::wstring& out) {
    std::u16string u16;
    int err = Utf8ToUtf16(in, u16);
    if (err != Z_OK) return err;
    out.assign(reinterpret_cast<const wchar_t*>(u16.data()), u16.size());
    return Z_OK;
}

int WStringToUtf8(const std::wstring& in, std::string& out) {
    return Utf16ToUtf8(
        std::u16string(reinterpret_cast<const char16_t*>(in.data()), in.size()),
        out);
}

#else
// POSIX: 纯算法实现（RFC 3629 UTF-8 + UCS-2/BMP-only UTF-16）
// 注：v0.1 仅支持 BMP（U+0000 ~ U+FFFF），代理对留 v0.2 补

namespace {

/// 解码一个 UTF-8 字符，返回码点 + 消耗字节数；失败返回 -1
int32_t DecodeUtf8(const char* s, size_t len, size_t& consumed) {
    if (len == 0) return -1;
    uint8_t c0 = static_cast<uint8_t>(s[0]);
    if (c0 < 0x80) { consumed = 1; return c0; }
    if ((c0 & 0xE0) == 0xC0) {
        if (len < 2) return -1;
        uint8_t c1 = static_cast<uint8_t>(s[1]);
        if ((c1 & 0xC0) != 0x80) return -1;
        uint32_t cp = ((c0 & 0x1F) << 6) | (c1 & 0x3F);
        if (cp < 0x80) return -1;  // 过长编码
        consumed = 2;
        return static_cast<int32_t>(cp);
    }
    if ((c0 & 0xF0) == 0xE0) {
        if (len < 3) return -1;
        uint8_t c1 = static_cast<uint8_t>(s[1]);
        uint8_t c2 = static_cast<uint8_t>(s[2]);
        if ((c1 & 0xC0) != 0x80 || (c2 & 0xC0) != 0x80) return -1;
        uint32_t cp = ((c0 & 0x0F) << 12) | ((c1 & 0x3F) << 6) | (c2 & 0x3F);
        if (cp < 0x800) return -1;
        if (cp >= 0xD800 && cp <= 0xDFFF) return -1;  // 代理对区
        consumed = 3;
        return static_cast<int32_t>(cp);
    }
    // 4 字节序列超出 BMP，v0.1 不支持，返回错误
    return -1;
}

}  // namespace

int Utf8ToUtf16(const std::string& in, std::u16string& out) {
    out.clear();
    size_t i = 0;
    while (i < in.size()) {
        size_t consumed = 0;
        int32_t cp = DecodeUtf8(in.data() + i, in.size() - i, consumed);
        if (cp < 0) return Z_ERR_ENCODING;
        if (cp > 0xFFFF) return Z_ERR_ENCODING;  // v0.1 不支持代理对
        out.push_back(static_cast<char16_t>(cp));
        i += consumed;
    }
    return Z_OK;
}

int Utf16ToUtf8(const std::u16string& in, std::string& out) {
    out.clear();
    for (char16_t c : in) {
        uint32_t cp = static_cast<uint32_t>(c);
        if (cp >= 0xD800 && cp <= 0xDFFF) return Z_ERR_ENCODING;
        if (cp < 0x80) {
            out.push_back(static_cast<char>(cp));
        } else if (cp < 0x800) {
            out.push_back(static_cast<char>(0xC0 | (cp >> 6)));
            out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        } else {
            out.push_back(static_cast<char>(0xE0 | (cp >> 12)));
            out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        }
    }
    return Z_OK;
}

int Utf8ToWString(const std::string& in, std::wstring& out) {
    // POSIX: wchar_t 是 UTF-32
    out.clear();
    size_t i = 0;
    while (i < in.size()) {
        size_t consumed = 0;
        int32_t cp = DecodeUtf8(in.data() + i, in.size() - i, consumed);
        if (cp < 0) return Z_ERR_ENCODING;
        out.push_back(static_cast<wchar_t>(cp));
        i += consumed;
    }
    return Z_OK;
}

int WStringToUtf8(const std::wstring& in, std::string& out) {
    out.clear();
    for (wchar_t wc : in) {
        uint32_t cp = static_cast<uint32_t>(wc);
        if (cp < 0x80) {
            out.push_back(static_cast<char>(cp));
        } else if (cp < 0x800) {
            out.push_back(static_cast<char>(0xC0 | (cp >> 6)));
            out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        } else if (cp < 0x10000) {
            out.push_back(static_cast<char>(0xE0 | (cp >> 12)));
            out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        } else {
            out.push_back(static_cast<char>(0xF0 | (cp >> 18)));
            out.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        }
    }
    return Z_OK;
}

#endif

}  // namespace platform
}  // namespace zbase
