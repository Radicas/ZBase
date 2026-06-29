/**
 * @file test_file_platform.cpp
 * @author 朱鹏 (Radica Zhu)
 * @date 2026-06-29
 * @version 0.1.0
 * @brief 文件平台原语测试
 * @details 覆盖 stat 存在/不存在、mkdir 递归、中文路径。
 */

#include <gtest/gtest.h>
#include "platform/file_platform.hpp"
#include "zbase/error.h"

#include <cstdio>
#include <string>

// ============ FileStatGet ============

TEST(FilePlatform, Stat_NonExistent) {
    zbase::platform::FileStat st;
    EXPECT_EQ(zbase::platform::FileStatGet("nonexistent_file_zbase_test", st), Z_OK);
    EXPECT_FALSE(st.exists);
}

TEST(FilePlatform, Stat_ExistingFile) {
    std::string path = "zbase_test_file.txt";
    FILE* fp = std::fopen(path.c_str(), "wb");
    ASSERT_NE(fp, nullptr);
    std::fwrite("hello", 1, 5, fp);
    std::fclose(fp);
    zbase::platform::FileStat st;
    EXPECT_EQ(zbase::platform::FileStatGet(path, st), Z_OK);
    EXPECT_TRUE(st.exists);
    EXPECT_FALSE(st.is_dir);
    EXPECT_EQ(st.size, 5u);
    std::remove(path.c_str());
}

TEST(FilePlatform, Stat_Directory) {
    std::string path = "zbase_test_dir_stat";
    EXPECT_EQ(zbase::platform::MakeDirs(path), Z_OK);
    zbase::platform::FileStat st;
    EXPECT_EQ(zbase::platform::FileStatGet(path, st), Z_OK);
    EXPECT_TRUE(st.exists);
    EXPECT_TRUE(st.is_dir);
    std::remove(path.c_str());
}

// ============ MakeDirs ============

TEST(FilePlatform, MakeDirs_Basic) {
    std::string path = "zbase_test_dir/sub";
    zbase::platform::FileStat st;
    EXPECT_EQ(zbase::platform::MakeDirs(path), Z_OK);
    EXPECT_EQ(zbase::platform::FileStatGet(path, st), Z_OK);
    EXPECT_TRUE(st.exists);
    EXPECT_TRUE(st.is_dir);
    std::remove("zbase_test_dir/sub");
    std::remove("zbase_test_dir");
}

TEST(FilePlatform, MakeDirs_Deep) {
    std::string path = "zbase_test_dir/a/b/c";
    EXPECT_EQ(zbase::platform::MakeDirs(path), Z_OK);
    zbase::platform::FileStat st;
    EXPECT_EQ(zbase::platform::FileStatGet(path, st), Z_OK);
    EXPECT_TRUE(st.exists);
    EXPECT_TRUE(st.is_dir);
    std::remove("zbase_test_dir/a/b/c");
    std::remove("zbase_test_dir/a/b");
    std::remove("zbase_test_dir/a");
    std::remove("zbase_test_dir");
}

TEST(FilePlatform, MakeDirs_AlreadyExists) {
    std::string path = "zbase_test_dir_existing";
    EXPECT_EQ(zbase::platform::MakeDirs(path), Z_OK);
    // 再次创建应返回 Z_OK（已存在）
    EXPECT_EQ(zbase::platform::MakeDirs(path), Z_OK);
    std::remove(path.c_str());
}

// ============ 中文路径（验收要求） ============

TEST(FilePlatform, ChinesePath_StatAndMkdir) {
    // 中文目录路径：UTF-8 字节序列
    // 注：测试只验证目录（mkdir + stat），不创建中文文件名文件，因为 std::fopen
    // 在 Windows 上走 ANSI 代码页无法处理 UTF-8 路径；file 模块的 z_file_write_text
    // （Task 6）会用 CreateFileW 走 UTF-16，那时再补文件级测试。
    std::string path = "zbase_test_\xE4\xB8\xAD\xE6\x96\x87/sub";
    EXPECT_EQ(zbase::platform::MakeDirs(path), Z_OK);
    zbase::platform::FileStat st;
    EXPECT_EQ(zbase::platform::FileStatGet(path, st), Z_OK);
    EXPECT_TRUE(st.exists);
    EXPECT_TRUE(st.is_dir);

    // 验证父目录也能正确 stat
    std::string parent = "zbase_test_\xE4\xB8\xAD\xE6\x96\x87";
    EXPECT_EQ(zbase::platform::FileStatGet(parent, st), Z_OK);
    EXPECT_TRUE(st.exists);
    EXPECT_TRUE(st.is_dir);

    std::remove(path.c_str());
    std::remove(parent.c_str());
}
