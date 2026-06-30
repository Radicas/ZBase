/**
 * @file test_cpp_wrapper.cpp
 * @author 朱鹏 (Radica Zhu)
 * @date 2026-06-30
 * @version 0.1.0
 * @brief C++ 包装层冒烟测试
 * @details 验证 zbase++ 头文件可正确编译并运行基本流程。
 */

#include <gtest/gtest.h>

#include <cstdio>
#include <string>

#include "zbase++/file.hpp"
#include "zbase++/log.hpp"
#include "zbase++/perf.hpp"
#include "zbase++/string.hpp"
#include "zbase++/time.hpp"

TEST(CppWrapper, StringSplit) {
    auto v = zbase::StringSplit("a,b,c", ",");
    EXPECT_EQ(v.size(), 3u);
    EXPECT_EQ(v[0], "a");
    EXPECT_EQ(v[1], "b");
    EXPECT_EQ(v[2], "c");
}

TEST(CppWrapper, StringJoinReplaceTrim) {
    auto parts = zbase::StringSplit("alpha,beta,gamma", ",");
    EXPECT_EQ(zbase::StringJoin(parts, "|"), "alpha|beta|gamma");
    EXPECT_EQ(zbase::StringReplace("hello world", "world", "zbase"), "hello zbase");
    EXPECT_EQ(zbase::StringTrim("  hi  "), "hi");
    EXPECT_EQ(zbase::StringToLower("AB"), "ab");
    EXPECT_EQ(zbase::StringToUpper("ab"), "AB");
}

TEST(CppWrapper, StringUtf8Len) {
    EXPECT_EQ(zbase::StringUtf8Len("abc"), 3u);
    EXPECT_EQ(zbase::StringUtf8Len("你好"), 2u);  // 6 字节但 2 字符
}

TEST(CppWrapper, FileWriteReadRoundTrip) {
    const char* path = "zbase_cpp_test.txt";
    zbase::WriteFile(path, "hello cpp");
    zbase::FileReader r(path);
    EXPECT_EQ(r.Str(), "hello cpp");
    EXPECT_EQ(r.Size(), 9u);
    EXPECT_EQ(std::string(r.View()), "hello cpp");
    std::remove(path);
}

TEST(CppWrapper, FileExistsAndMkdir) {
    const char* dir = "zbase_cpp_test_dir";
    zbase::MakeDirs(dir);
    EXPECT_TRUE(zbase::FileExists(dir));
    EXPECT_FALSE(zbase::FileExists("zbase_no_such_file_xyz"));
}

TEST(CppWrapper, DirIterator) {
    const char* dir = "zbase_cpp_iter_dir";
    zbase::MakeDirs(dir);
    zbase::WriteFile("zbase_cpp_iter_dir/a.txt", "A");
    zbase::WriteFile("zbase_cpp_iter_dir/b.txt", "B");

    zbase::DirIterator it(dir);
    std::vector<std::string> names;
    zbase::DirEntry e;
    while (it.Next(&e)) {
        names.push_back(e.name);
    }
    // 应至少包含 a.txt 和 b.txt（也可能含 . 和 ..）
    bool has_a = false, has_b = false;
    for (const auto& n : names) {
        if (n == "a.txt") has_a = true;
        if (n == "b.txt") has_b = true;
    }
    EXPECT_TRUE(has_a);
    EXPECT_TRUE(has_b);

    std::remove("zbase_cpp_iter_dir/a.txt");
    std::remove("zbase_cpp_iter_dir/b.txt");
    std::remove(dir);
}

TEST(CppWrapper, PerfScope) {
    zbase::Logger logger(Z_LOG_LEVEL_DEBUG);
    {
        ZBASE_PERF_SCOPE("cpp_task");
        zbase::SleepMs(5);
    }
    zbase::PerfDump();
    zbase::PerfReset();
    SUCCEED();
}

TEST(CppWrapper, ExceptionOnFileNotFound) {
    try {
        zbase::FileReader r("no_such_file_xyz_abc.txt");
        FAIL() << "应当抛异常";
    } catch (const zbase::Exception& e) {
        EXPECT_EQ(e.code(), Z_ERR_NOT_FOUND);
        EXPECT_NE(std::string(e.what()).find("找不到"), std::string::npos);
    }
}

TEST(CppWrapper, TimeFormat) {
    int64_t ms = zbase::NowMs();
    std::string s = zbase::FormatIso(ms);
    EXPECT_EQ(s.size(), 23u);  // "YYYY-MM-DDTHH:MM:SS.mmm"
    EXPECT_EQ(s[4], '-');
    EXPECT_EQ(s[10], 'T');
}
