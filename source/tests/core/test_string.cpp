/**
 * @file test_string.cpp
 * @author 朱鹏 (Radica Zhu)
 * @date 2026-06-29
 * @version 0.1.0
 * @brief string 模块单元测试
 * @details 覆盖 split/join/replace/trim/tolower/toupper/utf8_len 与边界条件。
 */

#include <gtest/gtest.h>
#include "zbase/error.h"
#include "zbase/string.h"

#include <cstring>

// ============ Split ============

TEST(String, Split_Basic) {
    char** arr = nullptr;
    size_t count = 0;
    const char* input = "a,b,c";
    EXPECT_EQ(z_string_split(input, std::strlen(input), ",", &arr, &count), Z_OK);
    EXPECT_EQ(count, 3u);
    EXPECT_STREQ(arr[0], "a");
    EXPECT_STREQ(arr[1], "b");
    EXPECT_STREQ(arr[2], "c");
    z_string_free_array(arr, count);
}

TEST(String, Split_EmptyField) {
    char** arr = nullptr;
    size_t count = 0;
    const char* input = "a,,c";
    EXPECT_EQ(z_string_split(input, std::strlen(input), ",", &arr, &count), Z_OK);
    EXPECT_EQ(count, 3u);
    EXPECT_STREQ(arr[1], "");
    z_string_free_array(arr, count);
}

TEST(String, Split_NoSepFound) {
    char** arr = nullptr;
    size_t count = 0;
    const char* input = "abc";
    EXPECT_EQ(z_string_split(input, std::strlen(input), ",", &arr, &count), Z_OK);
    EXPECT_EQ(count, 1u);
    EXPECT_STREQ(arr[0], "abc");
    z_string_free_array(arr, count);
}

TEST(String, Split_MultiByteSep) {
    char** arr = nullptr;
    size_t count = 0;
    const char* input = "a::b::c";
    EXPECT_EQ(z_string_split(input, std::strlen(input), "::", &arr, &count), Z_OK);
    EXPECT_EQ(count, 3u);
    EXPECT_STREQ(arr[0], "a");
    EXPECT_STREQ(arr[1], "b");
    EXPECT_STREQ(arr[2], "c");
    z_string_free_array(arr, count);
}

// ============ Join ============

TEST(String, Join_Basic) {
    const char* parts[] = {"a", "b", "c"};
    char* out = nullptr;
    EXPECT_EQ(z_string_join(const_cast<char**>(parts), 3, "-", &out), Z_OK);
    EXPECT_STREQ(out, "a-b-c");
    z_string_free(out);
}

TEST(String, Join_EmptyArr) {
    char* out = nullptr;
    EXPECT_EQ(z_string_join(nullptr, 0, "-", &out), Z_OK);
    EXPECT_STREQ(out, "");
    z_string_free(out);
}

// ============ Replace ============

TEST(String, Replace_All) {
    char* out = nullptr;
    EXPECT_EQ(z_string_replace("hello world hello", "hello", "hi", &out), Z_OK);
    EXPECT_STREQ(out, "hi world hi");
    z_string_free(out);
}

TEST(String, Replace_NotFound) {
    char* out = nullptr;
    EXPECT_EQ(z_string_replace("hello", "xyz", "abc", &out), Z_OK);
    EXPECT_STREQ(out, "hello");
    z_string_free(out);
}

// ============ Trim ============

TEST(String, Trim_BothSides) {
    char* out = nullptr;
    EXPECT_EQ(z_string_trim("  hello  ", &out), Z_OK);
    EXPECT_STREQ(out, "hello");
    z_string_free(out);
}

TEST(String, Trim_NoSpace) {
    char* out = nullptr;
    EXPECT_EQ(z_string_trim("hello", &out), Z_OK);
    EXPECT_STREQ(out, "hello");
    z_string_free(out);
}

TEST(String, Trim_AllSpace) {
    char* out = nullptr;
    EXPECT_EQ(z_string_trim("   \t\n  ", &out), Z_OK);
    EXPECT_STREQ(out, "");
    z_string_free(out);
}

// ============ Case ============

TEST(String, Tolower) {
    char* out = nullptr;
    EXPECT_EQ(z_string_tolower("HeLLo", &out), Z_OK);
    EXPECT_STREQ(out, "hello");
    z_string_free(out);
}

TEST(String, Toupper) {
    char* out = nullptr;
    EXPECT_EQ(z_string_toupper("HeLLo", &out), Z_OK);
    EXPECT_STREQ(out, "HELLO");
    z_string_free(out);
}

// ============ UTF-8 length ============

TEST(String, Utf8Len_Ascii) {
    EXPECT_EQ(z_string_utf8_len("hello"), 5u);
}

TEST(String, Utf8Len_Chinese) {
    // 每个汉字 UTF-8 编码占 3 字节，"你好" 字节数=6，字符数=2
    EXPECT_EQ(z_string_utf8_len("\xE4\xBD\xA0\xE5\xA5\xBD"), 2u);
}

TEST(String, Utf8Len_Null) {
    EXPECT_EQ(z_string_utf8_len(nullptr), 0u);
}

// ============ Free / 边界 ============

TEST(String, Free_NullptrSafe) {
    z_string_free(nullptr);
    z_string_free_array(nullptr, 0);
}

TEST(String, InvalidArg_NullInput) {
    char** arr = nullptr;
    size_t count = 0;
    EXPECT_EQ(z_string_split(nullptr, 0, ",", &arr, &count), Z_ERR_INVALID_ARG);
    EXPECT_EQ(z_string_split("a", 1, nullptr, &arr, &count), Z_ERR_INVALID_ARG);
    EXPECT_EQ(z_string_split("a", 1, "", &arr, &count), Z_ERR_INVALID_ARG);
}

TEST(String, Replace_EmptyFrom) {
    char* out = nullptr;
    EXPECT_EQ(z_string_replace("hello", "", "x", &out), Z_ERR_INVALID_ARG);
}
