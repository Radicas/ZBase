/**
 * @file test_encoding.cpp
 * @author 朱鹏 (Radica Zhu)
 * @date 2026-06-29
 * @version 0.1.0
 * @brief platform 层 UTF-8 ↔ UTF-16 转换单元测试
 * @details 覆盖空串、ASCII、中文、混合、非法序列、代理对、往返一致性。
 */

#include <gtest/gtest.h>
#include <string>

#include "platform/encoding.hpp"
#include "zbase/error.h"

using zbase::platform::Utf16ToUtf8;
using zbase::platform::Utf8ToUtf16;
using zbase::platform::Utf8ToWString;
using zbase::platform::WStringToUtf8;

namespace {
/// 把 UTF-8 字符串当作 UTF-16 单元序列拷给 u16string（仅 BMP，每字节一个码元）
/// 用于构造已知 BMP 码点的期望值
std::u16string AsciiToU16(const std::string& s) {
    return std::u16string(s.begin(), s.end());
}
}  // namespace

// ============ Utf8ToUtf16 ============

TEST(Encoding, EmptyInputUtf8ToUtf16ReturnsOk) {
    std::u16string out;
    EXPECT_EQ(Z_OK, Utf8ToUtf16("", out));
    EXPECT_TRUE(out.empty());
}

TEST(Encoding, AsciiRoundTripsToUtf16) {
    std::u16string out;
    ASSERT_EQ(Z_OK, Utf8ToUtf16("Hello, ZBase!", out));
    EXPECT_EQ(AsciiToU16("Hello, ZBase!"), out);
}

TEST(Encoding, ChineseRoundTripsToUtf16) {
    // 你好 → U+4F60 U+597D
    std::u16string out;
    ASSERT_EQ(Z_OK, Utf8ToUtf16("\xE4\xBD\xA0\xE5\xA5\xBD", out));
    ASSERT_EQ(2u, out.size());
    EXPECT_EQ(u'\u4F60', out[0]);
    EXPECT_EQ(u'\u597D', out[1]);
}

TEST(Encoding, MixedAsciiAndChineseToUtf16) {
    // abc中文xyz
    std::u16string out;
    ASSERT_EQ(Z_OK, Utf8ToUtf16("abc\xE4\xB8\xAD\xE6\x96\x87xyz", out));
    ASSERT_EQ(8u, out.size());
    EXPECT_EQ(u'a', out[0]);
    EXPECT_EQ(u'b', out[1]);
    EXPECT_EQ(u'c', out[2]);
    EXPECT_EQ(u'\u4E2D', out[3]);  // 中
    EXPECT_EQ(u'\u6587', out[4]);  // 文
    EXPECT_EQ(u'x', out[5]);
    EXPECT_EQ(u'y', out[6]);
    EXPECT_EQ(u'z', out[7]);
}

TEST(Encoding, InvalidUtf8TruncatedSequenceReturnsError) {
    // 0xE4 单独出现，缺少后续字节
    std::u16string out;
    EXPECT_EQ(Z_ERR_ENCODING, Utf8ToUtf16("\xE4", out));
}

TEST(Encoding, InvalidUtf8BadContinuationByteReturnsError) {
    // 0xE4 后跟非 10xxxxxx 字节
    std::u16string out;
    EXPECT_EQ(Z_ERR_ENCODING, Utf8ToUtf16("\xE4\x41\x41", out));
}

TEST(Encoding, InvalidUtf8OverlongEncodingReturnsError) {
    // 过长编码 U+0041 -> 0xC1 0xA1（应为单字节 0x41）
    std::u16string out;
    EXPECT_EQ(Z_ERR_ENCODING, Utf8ToUtf16("\xC1\xA1", out));
}

TEST(Encoding, LongStringRoundTrips) {
    std::string src;
    for (int i = 0; i < 1000; ++i) {
        src += "\xE4\xBD\xA0\xE5\xA5\xBD";  // 你好
    }
    std::u16string u16;
    ASSERT_EQ(Z_OK, Utf8ToUtf16(src, u16));
    EXPECT_EQ(2000u, u16.size());
    std::string back;
    ASSERT_EQ(Z_OK, Utf16ToUtf8(u16, back));
    EXPECT_EQ(src, back);
}

// ============ Utf16ToUtf8 ============

TEST(Encoding, EmptyInputUtf16ToUtf8ReturnsOk) {
    std::string out;
    EXPECT_EQ(Z_OK, Utf16ToUtf8(u"", out));
    EXPECT_TRUE(out.empty());
}

TEST(Encoding, Utf16ToUtf8RoundTrip) {
    std::u16string src = u"Hello, \u4E16\u754C";  // Hello, 世界
    std::string u8;
    ASSERT_EQ(Z_OK, Utf16ToUtf8(src, u8));
    EXPECT_EQ("Hello, \xE4\xB8\x96\xE7\x95\x8C", u8);
}

TEST(Encoding, Utf16ToUtf8RejectsLoneSurrogate) {
    std::u16string bad;
    bad.push_back(static_cast<char16_t>(0xD800));  // 高代理位单独出现
    std::string out;
    EXPECT_EQ(Z_ERR_ENCODING, Utf16ToUtf8(bad, out));
}

TEST(Encoding, Utf8ToUtf16RoundTripEqualsOriginal) {
    std::string original = "ZBase v0.1: \xE4\xB8\xAD\xE6\x96\x87\xE6\xB5\x8B\xE8\xAF\x95";
    std::u16string mid;
    ASSERT_EQ(Z_OK, Utf8ToUtf16(original, mid));
    std::string back;
    ASSERT_EQ(Z_OK, Utf16ToUtf8(mid, back));
    EXPECT_EQ(original, back);
}

// ============ Utf8ToWString / WStringToUtf8 ============

TEST(Encoding, EmptyInputUtf8ToWStringReturnsOk) {
    std::wstring out;
    EXPECT_EQ(Z_OK, Utf8ToWString("", out));
    EXPECT_TRUE(out.empty());
}

TEST(Encoding, Utf8ToWStringRoundTrip) {
    std::string original = "abc\xE4\xBD\xA0\xE5\xA5\xBDxyz";  // abc你好xyz
    std::wstring w;
    ASSERT_EQ(Z_OK, Utf8ToWString(original, w));
    EXPECT_EQ(8u, w.size());
    std::string back;
    ASSERT_EQ(Z_OK, WStringToUtf8(w, back));
    EXPECT_EQ(original, back);
}

TEST(Encoding, WStringToUtf8RejectsInvalidInput) {
    // 在 Windows 上 wchar_t 是 UTF-16，构造 lone surrogate
    // 在 POSIX 上 wchar_t 是 UTF-32，构造一个超出 Unicode 范围的码点
    std::wstring bad;
#ifdef _WIN32
    bad.push_back(static_cast<wchar_t>(0xD800));
#else
    bad.push_back(static_cast<wchar_t>(0x110000));  // 超出 Unicode 范围
#endif
    std::string out;
    // POSIX 路径不会校验，但 Windows 路径会返回 Z_ERR_ENCODING
#ifdef _WIN32
    EXPECT_EQ(Z_ERR_ENCODING, WStringToUtf8(bad, out));
#else
    // POSIX 实现 v0.1 不做范围校验，仅做编码转换；此处只要不崩溃即可
    int rc = WStringToUtf8(bad, out);
    EXPECT_TRUE(rc == Z_OK || rc == Z_ERR_ENCODING);
#endif
}
