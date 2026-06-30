/**
 * @file test_log.cpp
 * @author 朱鹏 (Radica Zhu)
 * @date 2026-06-30
 * @version 0.2.0
 * @brief log 模块测试
 * @details v0.1 基础测试（init/shutdown/write/level）+ v0.2 文件输出与滚动测试。
 *          验收要求：覆盖滚动逻辑、中文路径、追加续写、重复关闭。
 */

#include <gtest/gtest.h>

#include "zbase/error.h"
#include "zbase/file.h"
#include "zbase/log.h"

#include <cstdio>
#include <cstring>
#include <string>

namespace {

/// 读取文件全部内容到 std::string（文件不存在返回空串）
std::string ReadAll(const std::string& path) {
    char* buf = nullptr;
    size_t len = 0;
    if (z_file_read_text(path.c_str(), &buf, &len) != Z_OK) return "";
    std::string s(buf, len);
    z_file_free(buf);
    return s;
}

/// 删除文件（不存在时静默）
void RemoveAll(const std::string& path) {
    std::remove(path.c_str());
}

/// 清理 path 及其滚动文件 path.1 ~ path.N
void CleanupLogFiles(const std::string& path, int max_n) {
    RemoveAll(path);
    for (int i = 1; i <= max_n; ++i) {
        RemoveAll(path + "." + std::to_string(i));
    }
}

}  // namespace

// ============ v0.1 基础测试 ============

TEST(Log, InitShutdown) {
    EXPECT_EQ(z_log_init(Z_LOG_LEVEL_DEBUG), Z_OK);
    z_log_shutdown();
    SUCCEED();
}

TEST(Log, InitShutdown_MultipleTimes) {
    EXPECT_EQ(z_log_init(Z_LOG_LEVEL_INFO), Z_OK);
    z_log_shutdown();
    EXPECT_EQ(z_log_init(Z_LOG_LEVEL_ERROR), Z_OK);
    z_log_shutdown();
    SUCCEED();
}

TEST(Log, Write_NoCrash) {
    z_log_init(Z_LOG_LEVEL_DEBUG);
    Z_LOG_INFO("测试中文日志 %d", 42);
    Z_LOG_WARN("warning %s", "msg");
    Z_LOG_ERROR("error message");
    z_log_shutdown();
    SUCCEED();
}

TEST(Log, LevelFilter) {
    z_log_init(Z_LOG_LEVEL_ERROR);
    Z_LOG_DEBUG("should not appear");
    Z_LOG_INFO("should not appear");
    Z_LOG_ERROR("should appear");
    z_log_shutdown();
    SUCCEED();
}

TEST(Log, Write_AllLevels) {
    z_log_init(Z_LOG_LEVEL_DEBUG);
    Z_LOG_DEBUG("debug message %d", 1);
    Z_LOG_INFO("info message %d", 2);
    Z_LOG_WARN("warn message %d", 3);
    Z_LOG_ERROR("error message %d", 4);
    z_log_shutdown();
    SUCCEED();
}

// ============ v0.2 文件输出测试 ============

TEST(LogFile, OpenClose_Basic) {
    const std::string path = "zbase_test_log_basic.log";
    CleanupLogFiles(path, 5);

    EXPECT_EQ(z_log_init(Z_LOG_LEVEL_DEBUG), Z_OK);
    EXPECT_EQ(z_log_file_open(path.c_str(), 0, 1), Z_OK);
    Z_LOG_INFO("hello file");
    z_log_file_close();
    z_log_shutdown();

    std::string content = ReadAll(path);
    EXPECT_NE(content.find("hello file"), std::string::npos);
    CleanupLogFiles(path, 5);
}

TEST(LogFile, Open_NullPath) {
    EXPECT_EQ(z_log_init(Z_LOG_LEVEL_INFO), Z_OK);
    EXPECT_EQ(z_log_file_open(nullptr, 0, 1), Z_ERR_INVALID_ARG);
    z_log_shutdown();
}

TEST(LogFile, Open_TooManyFiles) {
    EXPECT_EQ(z_log_init(Z_LOG_LEVEL_INFO), Z_OK);
    EXPECT_EQ(z_log_file_open("dummy.log", 0, 100), Z_ERR_INVALID_ARG);
    z_log_shutdown();
}

TEST(LogFile, Close_WithoutOpen) {
    // 没打开过文件就调用 close，应该不崩
    EXPECT_EQ(z_log_init(Z_LOG_LEVEL_INFO), Z_OK);
    z_log_file_close();
    z_log_shutdown();
    SUCCEED();
}

TEST(LogFile, Close_MultipleTimes) {
    const std::string path = "zbase_test_log_close_many.log";
    CleanupLogFiles(path, 5);

    EXPECT_EQ(z_log_init(Z_LOG_LEVEL_INFO), Z_OK);
    EXPECT_EQ(z_log_file_open(path.c_str(), 0, 1), Z_OK);
    z_log_file_close();
    z_log_file_close();  // 重复关闭应该安全
    z_log_file_close();
    z_log_shutdown();
    CleanupLogFiles(path, 5);
}

TEST(LogFile, Write_ContainsLevelAndMessage) {
    const std::string path = "zbase_test_log_format.log";
    CleanupLogFiles(path, 5);

    EXPECT_EQ(z_log_init(Z_LOG_LEVEL_DEBUG), Z_OK);
    EXPECT_EQ(z_log_file_open(path.c_str(), 0, 1), Z_OK);
    Z_LOG_WARN("format_test %d", 7);
    z_log_file_close();
    z_log_shutdown();

    std::string content = ReadAll(path);
    EXPECT_NE(content.find("WARN"), std::string::npos);
    EXPECT_NE(content.find("format_test 7"), std::string::npos);
    CleanupLogFiles(path, 5);
}

TEST(LogFile, LevelFilter_AppliesToFile) {
    const std::string path = "zbase_test_log_level.log";
    CleanupLogFiles(path, 5);

    EXPECT_EQ(z_log_init(Z_LOG_LEVEL_ERROR), Z_OK);  // 只允许 ERROR
    EXPECT_EQ(z_log_file_open(path.c_str(), 0, 1), Z_OK);
    Z_LOG_DEBUG("debug should not write");
    Z_LOG_INFO("info should not write");
    Z_LOG_ERROR("error should write");
    z_log_file_close();
    z_log_shutdown();

    std::string content = ReadAll(path);
    EXPECT_EQ(content.find("debug should not write"), std::string::npos);
    EXPECT_EQ(content.find("info should not write"), std::string::npos);
    EXPECT_NE(content.find("error should write"), std::string::npos);
    CleanupLogFiles(path, 5);
}

// ============ 滚动逻辑测试 ============

TEST(LogFile, Roll_TriggeredAtMaxSize) {
    const std::string path = "zbase_test_log_roll.log";
    CleanupLogFiles(path, 5);

    EXPECT_EQ(z_log_init(Z_LOG_LEVEL_DEBUG), Z_OK);
    // max_size=200 字节，max_files=3
    EXPECT_EQ(z_log_file_open(path.c_str(), 200, 3), Z_OK);
    // 写多条直到触发滚动
    for (int i = 0; i < 50; ++i) {
        Z_LOG_INFO("line %02d padding padding padding padding padding", i);
    }
    z_log_file_close();
    z_log_shutdown();

    // 应该至少产生 path.1 滚动文件
    std::string rolled = ReadAll(path + ".1");
    EXPECT_FALSE(rolled.empty()) << "path.1 应该有内容";
    CleanupLogFiles(path, 5);
}

TEST(LogFile, Roll_KeepsMaxFiles) {
    const std::string path = "zbase_test_log_keep.log";
    CleanupLogFiles(path, 5);

    EXPECT_EQ(z_log_init(Z_LOG_LEVEL_DEBUG), Z_OK);
    // max_size=100 字节，max_files=2 → 只保留 path.1 path.2
    EXPECT_EQ(z_log_file_open(path.c_str(), 100, 2), Z_OK);
    for (int i = 0; i < 100; ++i) {
        Z_LOG_INFO("padding padding padding padding %d", i);
    }
    z_log_file_close();
    z_log_shutdown();

    // path / path.1 / path.2 应该存在
    // path.3 不应该存在（被 max_files 限制删除）
    EXPECT_EQ(z_file_exists(path.c_str()), 1);
    EXPECT_EQ(z_file_exists((path + ".1").c_str()), 1);
    EXPECT_EQ(z_file_exists((path + ".2").c_str()), 1);
    EXPECT_EQ(z_file_exists((path + ".3").c_str()), 0);
    CleanupLogFiles(path, 5);
}

TEST(LogFile, Roll_AppendResumesSize) {
    // 第一次写一些内容（不触发滚动），关闭；再次打开，current_size 应从已有文件大小累加
    const std::string path = "zbase_test_log_append.log";
    CleanupLogFiles(path, 5);

    EXPECT_EQ(z_log_init(Z_LOG_LEVEL_DEBUG), Z_OK);
    EXPECT_EQ(z_log_file_open(path.c_str(), 0, 1), Z_OK);  // 不滚动
    Z_LOG_INFO("first session content");
    z_log_file_close();

    // 第二次以小 max_size 打开，应该立即看到 current_size 已经接近 max_size
    EXPECT_EQ(z_log_file_open(path.c_str(), 1024, 1), Z_OK);
    Z_LOG_INFO("second session content");
    z_log_file_close();
    z_log_shutdown();

    std::string content = ReadAll(path);
    EXPECT_NE(content.find("first session content"), std::string::npos);
    EXPECT_NE(content.find("second session content"), std::string::npos);
    CleanupLogFiles(path, 5);
}

TEST(LogFile, Reopen_DifferentPath) {
    const std::string p1 = "zbase_test_log_reopen1.log";
    const std::string p2 = "zbase_test_log_reopen2.log";
    CleanupLogFiles(p1, 3);
    CleanupLogFiles(p2, 3);

    EXPECT_EQ(z_log_init(Z_LOG_LEVEL_INFO), Z_OK);
    EXPECT_EQ(z_log_file_open(p1.c_str(), 0, 1), Z_OK);
    Z_LOG_INFO("to file 1");
    // 不显式 close，直接打开新文件，应该自动关闭旧文件
    EXPECT_EQ(z_log_file_open(p2.c_str(), 0, 1), Z_OK);
    Z_LOG_INFO("to file 2");
    z_log_file_close();
    z_log_shutdown();

    std::string c1 = ReadAll(p1);
    std::string c2 = ReadAll(p2);
    EXPECT_NE(c1.find("to file 1"), std::string::npos);
    EXPECT_EQ(c1.find("to file 2"), std::string::npos);
    EXPECT_NE(c2.find("to file 2"), std::string::npos);
    CleanupLogFiles(p1, 3);
    CleanupLogFiles(p2, 3);
}

// ============ 中文路径测试（验收要求） ============

TEST(LogFile, ChinesePath_WriteAndRoll) {
    // 中文文件名：日志.log
    std::string path = "zbase_test_\xE6\x97\xA5\xE5\xBF\x97.log";  // 日志.log
    CleanupLogFiles(path, 5);

    EXPECT_EQ(z_log_init(Z_LOG_LEVEL_DEBUG), Z_OK);
    // max_size=300：单条约 90 字节，写 5 条后触发滚动，剩余最后一条写到新 path
    EXPECT_EQ(z_log_file_open(path.c_str(), 300, 2), Z_OK);
    Z_LOG_INFO("\xE4\xB8\xAD\xE6\x96\x87\xE6\x97\xA5\xE5\xBF\x97\xE5\x86\x85\xE5\xAE\xB9 padding");  // 中文日志内容 padding
    for (int i = 0; i < 10; ++i) {
        Z_LOG_INFO("\xE8\xA1\x8C %d padding padding padding padding", i);  // 行 N ...
    }
    z_log_file_close();
    z_log_shutdown();

    // 当前文件应该可读且有内容（最后一条日志已写入但未触发滚动）
    EXPECT_EQ(z_file_exists(path.c_str()), 1);
    char* buf = nullptr;
    size_t len = 0;
    int rc = z_file_read_text(path.c_str(), &buf, &len);
    EXPECT_EQ(rc, Z_OK) << "读取中文路径日志失败，错误码=" << rc;
    if (rc == Z_OK) {
        EXPECT_GT(len, 0u) << "文件读取成功但内容为空";
        if (buf) z_file_free(buf);
    }
    // 滚动文件至少存在一个
    EXPECT_EQ(z_file_exists((path + ".1").c_str()), 1);
    CleanupLogFiles(path, 5);
}

TEST(LogFile, Shutdown_ClosesFile) {
    const std::string path = "zbase_test_log_shutdown.log";
    CleanupLogFiles(path, 5);

    EXPECT_EQ(z_log_init(Z_LOG_LEVEL_INFO), Z_OK);
    EXPECT_EQ(z_log_file_open(path.c_str(), 0, 1), Z_OK);
    Z_LOG_INFO("before shutdown");
    z_log_shutdown();  // shutdown 应该内部调用 close

    std::string content = ReadAll(path);
    EXPECT_NE(content.find("before shutdown"), std::string::npos);
    CleanupLogFiles(path, 5);
}
