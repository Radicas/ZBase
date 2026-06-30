/**
 * @file test_file.cpp
 * @author 朱鹏 (Radica Zhu)
 * @date 2026-06-29
 * @version 0.1.0
 * @brief file 模块测试
 * @details 覆盖 read_text/write_text/exists/mkdir/dir_iter 与边界条件。
 *          包含中文路径文件读写（验收要求）。
 */

#include <gtest/gtest.h>
#include "zbase/error.h"
#include "zbase/file.h"

#include <cstring>
#include <string>

// ============ ReadText ============

TEST(File, ReadText_NotFound) {
    char* buf = nullptr;
    size_t len = 0;
    EXPECT_EQ(z_file_read_text("nonexistent_zbase.txt", &buf, &len), Z_ERR_NOT_FOUND);
    EXPECT_EQ(buf, nullptr);
}

TEST(File, ReadText_InvalidArg) {
    EXPECT_EQ(z_file_read_text(nullptr, nullptr, nullptr), Z_ERR_INVALID_ARG);
}

TEST(File, WriteThenRead) {
    const char* path = "zbase_test_writeread.txt";
    const char* content = "hello zbase\nsecond line";
    EXPECT_EQ(z_file_write_text(path, content, std::strlen(content)), Z_OK);
    char* buf = nullptr;
    size_t len = 0;
    EXPECT_EQ(z_file_read_text(path, &buf, &len), Z_OK);
    ASSERT_NE(buf, nullptr);
    EXPECT_EQ(len, std::strlen(content));
    EXPECT_EQ(std::string(buf, len), std::string(content));
    z_file_free(buf);
    std::remove(path);
}

TEST(File, WriteThenRead_Empty) {
    const char* path = "zbase_test_empty.txt";
    EXPECT_EQ(z_file_write_text(path, "", 0), Z_OK);
    char* buf = nullptr;
    size_t len = 0;
    EXPECT_EQ(z_file_read_text(path, &buf, &len), Z_OK);
    ASSERT_NE(buf, nullptr);
    EXPECT_EQ(len, 0u);
    z_file_free(buf);
    std::remove(path);
}

TEST(File, WriteText_Overwrite) {
    const char* path = "zbase_test_overwrite.txt";
    EXPECT_EQ(z_file_write_text(path, "first", 5), Z_OK);
    EXPECT_EQ(z_file_write_text(path, "second", 6), Z_OK);
    char* buf = nullptr;
    size_t len = 0;
    EXPECT_EQ(z_file_read_text(path, &buf, &len), Z_OK);
    EXPECT_EQ(std::string(buf, len), "second");
    z_file_free(buf);
    std::remove(path);
}

// ============ Exists / Mkdir ============

TEST(File, Exists) {
    EXPECT_EQ(z_file_exists("nonexistent_zbase.txt"), 0);
    const char* path = "zbase_test_exists.txt";
    EXPECT_EQ(z_file_write_text(path, "x", 1), Z_OK);
    EXPECT_EQ(z_file_exists(path), 1);
    std::remove(path);
}

TEST(File, Exists_NullPath) {
    EXPECT_EQ(z_file_exists(nullptr), Z_ERR_INVALID_ARG);
}

TEST(File, Mkdir) {
    const char* path = "zbase_test_mkdir/sub";
    EXPECT_EQ(z_file_mkdir(path), Z_OK);
    EXPECT_EQ(z_file_exists(path), 1);
    std::remove("zbase_test_mkdir/sub");
    std::remove("zbase_test_mkdir");
}

TEST(File, Mkdir_AlreadyExists) {
    const char* path = "zbase_test_mkdir_existing";
    EXPECT_EQ(z_file_mkdir(path), Z_OK);
    EXPECT_EQ(z_file_mkdir(path), Z_OK);
    std::remove(path);
}

TEST(File, Free_NullptrSafe) {
    z_file_free(nullptr);
}

// ============ DirIter ============

TEST(File, DirIter_Basic) {
    const char* dir = "zbase_test_diriter";
    ASSERT_EQ(z_file_mkdir(dir), Z_OK);
    const char* files[] = {"a.txt", "b.txt"};
    for (const char* f : files) {
        std::string full = std::string(dir) + "/" + f;
        EXPECT_EQ(z_file_write_text(full.c_str(), "x", 1), Z_OK);
    }
    z_dir_iter* iter = nullptr;
    EXPECT_EQ(z_dir_iter_create(dir, &iter), Z_OK);
    ASSERT_NE(iter, nullptr);
    int count = 0;
    z_dir_iter_entry_t entry;
    std::memset(&entry, 0, sizeof(entry));
    while (z_dir_iter_next(iter, &entry) == Z_OK) {
        if (std::strcmp(entry.name, ".") == 0 || std::strcmp(entry.name, "..") == 0) continue;
        ++count;
    }
    EXPECT_GE(count, 2);
    z_dir_iter_destroy(iter);
    std::remove("zbase_test_diriter/a.txt");
    std::remove("zbase_test_diriter/b.txt");
    std::remove("zbase_test_diriter");
}

TEST(File, DirIter_NextReturnsNotFoundAtEnd) {
    const char* dir = "zbase_test_diriter_end";
    ASSERT_EQ(z_file_mkdir(dir), Z_OK);
    z_dir_iter* iter = nullptr;
    EXPECT_EQ(z_dir_iter_create(dir, &iter), Z_OK);
    ASSERT_NE(iter, nullptr);

    // 空目录：第一条就应该是 "." 或 ".."，遍历完应返回 Z_ERR_NOT_FOUND
    z_dir_iter_entry_t entry;
    std::memset(&entry, 0, sizeof(entry));
    int rc;
    int count = 0;
    while ((rc = z_dir_iter_next(iter, &entry)) == Z_OK) {
        ++count;
    }
    EXPECT_EQ(rc, Z_ERR_NOT_FOUND);
    z_dir_iter_destroy(iter);
    std::remove(dir);
}

TEST(File, DirIter_OnFile_ReturnsInvalidArg) {
    const char* path = "zbase_test_diriter_file.txt";
    EXPECT_EQ(z_file_write_text(path, "x", 1), Z_OK);
    z_dir_iter* iter = nullptr;
    EXPECT_EQ(z_dir_iter_create(path, &iter), Z_ERR_INVALID_ARG);
    EXPECT_EQ(iter, nullptr);
    std::remove(path);
}

TEST(File, DirIter_NullIter) {
    z_dir_iter_entry_t entry;
    std::memset(&entry, 0, sizeof(entry));
    EXPECT_EQ(z_dir_iter_next(nullptr, &entry), Z_ERR_INVALID_ARG);
    z_dir_iter_destroy(nullptr);  // 应该不崩
}

// ============ 中文路径（验收要求） ============

TEST(File, ChinesePath_WriteReadExists) {
    // 中文文件名路径：UTF-8 字节序列
    std::string path = "zbase_test_\xE6\xB5\x8B\xE8\xAF\x95.txt";  // 测试.txt
    std::string content = "ZBase \xE4\xB8\xAD\xE6\x96\x87\xE5\x86\x85\xE5\xAE\xB9";  // ZBase 中文内容

    EXPECT_EQ(z_file_write_text(path.c_str(), content.data(), content.size()), Z_OK);
    EXPECT_EQ(z_file_exists(path.c_str()), 1);

    char* buf = nullptr;
    size_t len = 0;
    EXPECT_EQ(z_file_read_text(path.c_str(), &buf, &len), Z_OK);
    ASSERT_NE(buf, nullptr);
    EXPECT_EQ(len, content.size());
    EXPECT_EQ(std::string(buf, len), content);
    z_file_free(buf);
    std::remove(path.c_str());
}

TEST(File, ChinesePath_MkdirAndDirIter) {
    std::string dir = "zbase_test_\xE7\x9B\xAE\xE5\xBD\x95";  // 目录
    EXPECT_EQ(z_file_mkdir(dir.c_str()), Z_OK);
    EXPECT_EQ(z_file_exists(dir.c_str()), 1);

    // 在中文目录下创建中文文件
    std::string file = dir + "/" + "\xE6\x96\x87\xE4\xBB\xB6.txt";  // 文件.txt
    EXPECT_EQ(z_file_write_text(file.c_str(), "data", 4), Z_OK);

    z_dir_iter* iter = nullptr;
    EXPECT_EQ(z_dir_iter_create(dir.c_str(), &iter), Z_OK);
    ASSERT_NE(iter, nullptr);
    int count = 0;
    z_dir_iter_entry_t entry;
    std::memset(&entry, 0, sizeof(entry));
    while (z_dir_iter_next(iter, &entry) == Z_OK) {
        if (std::strcmp(entry.name, ".") == 0 || std::strcmp(entry.name, "..") == 0) continue;
        ++count;
    }
    EXPECT_GE(count, 1);  // 至少找到我们创建的 文件.txt
    z_dir_iter_destroy(iter);

    std::remove(file.c_str());
    std::remove(dir.c_str());
}
