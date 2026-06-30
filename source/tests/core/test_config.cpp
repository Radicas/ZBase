/**
 * @file test_config.cpp
 * @author 朱鹏 (Radica Zhu)
 * @date 2026-06-30
 * @version 0.2.0
 * @brief config 模块测试
 * @details 覆盖 INI 解析：基本 load/get、注释、trim、节、整数、布尔、
 *          中文 key/value、UTF-8 BOM、空文件、错误参数、C++ 包装。
 */

#include <gtest/gtest.h>

#include "zbase++/config.hpp"
#include "zbase/config.h"
#include "zbase/error.h"
#include "zbase/file.h"

#include <cstdio>
#include <cstring>
#include <string>

namespace {

/// 写一个临时文件并返回路径
std::string WriteTmpFile(const std::string& name, const std::string& content) {
    EXPECT_EQ(z_file_write_text(name.c_str(), content.data(), content.size()), Z_OK);
    return name;
}

/// 删除文件（不存在时静默）
void RemoveTmpFile(const std::string& name) {
    std::remove(name.c_str());
}

}  // namespace

// ============ 基本读写 ============

TEST(Config, LoadAndGet_Basic) {
    const std::string path = "zbase_test_config_basic.ini";
    std::string content =
        "name = zbase\n"
        "version = 1\n"
        "[server]\n"
        "host = 127.0.0.1\n"
        "port = 8080\n"
        "debug = true\n";
    WriteTmpFile(path, content);

    z_config_t* cfg = nullptr;
    EXPECT_EQ(z_config_load(path.c_str(), &cfg), Z_OK);
    ASSERT_NE(cfg, nullptr);

    // 全局节
    EXPECT_STREQ(z_config_get(cfg, "", "name", ""), "zbase");
    EXPECT_STREQ(z_config_get(cfg, "", "version", ""), "1");

    // server 节
    EXPECT_STREQ(z_config_get(cfg, "server", "host", ""), "127.0.0.1");
    EXPECT_STREQ(z_config_get(cfg, "server", "port", ""), "8080");
    EXPECT_STREQ(z_config_get(cfg, "server", "debug", ""), "true");

    z_config_free(cfg);
    RemoveTmpFile(path);
}

TEST(Config, Load_InvalidArg) {
    EXPECT_EQ(z_config_load(nullptr, nullptr), Z_ERR_INVALID_ARG);
    z_config_t* cfg = nullptr;
    EXPECT_EQ(z_config_load("dummy.ini", nullptr), Z_ERR_INVALID_ARG);
    EXPECT_EQ(z_config_load(nullptr, &cfg), Z_ERR_INVALID_ARG);
}

TEST(Config, Load_FileNotFound) {
    z_config_t* cfg = nullptr;
    EXPECT_EQ(z_config_load("nonexistent_zbase_config.ini", &cfg), Z_ERR_NOT_FOUND);
    EXPECT_EQ(cfg, nullptr);
}

TEST(Config, Free_NullptrSafe) {
    z_config_free(nullptr);  // 不应崩溃
    SUCCEED();
}

// ============ 默认值 ============

TEST(Config, Get_MissingKey_ReturnsDefault) {
    const std::string path = "zbase_test_config_default.ini";
    WriteTmpFile(path, "[s]\nk = v\n");

    z_config_t* cfg = nullptr;
    ASSERT_EQ(z_config_load(path.c_str(), &cfg), Z_OK);

    // 节存在但 key 不存在
    EXPECT_STREQ(z_config_get(cfg, "s", "missing", "fallback"), "fallback");
    // 节不存在
    EXPECT_STREQ(z_config_get(cfg, "no_such_section", "k", "fb"), "fb");
    // default_val 为 NULL
    EXPECT_EQ(z_config_get(cfg, "s", "missing", nullptr), nullptr);

    z_config_free(cfg);
    RemoveTmpFile(path);
}

TEST(Config, Get_NullConfig_ReturnsDefault) {
    EXPECT_STREQ(z_config_get(nullptr, "s", "k", "default"), "default");
    EXPECT_EQ(z_config_get(nullptr, "s", "k", nullptr), nullptr);
}

// ============ 注释 ============

TEST(Config, Comments_ignored) {
    const std::string path = "zbase_test_config_comments.ini";
    std::string content =
        "# 这是行首注释\n"
        "; 这是分号注释\n"
        "  # 行首带空格的注释\n"
        "key1 = value1\n"
        "key2 = value2  trailing  # 行尾井号不当作注释\n";
    WriteTmpFile(path, content);

    z_config_t* cfg = nullptr;
    ASSERT_EQ(z_config_load(path.c_str(), &cfg), Z_OK);

    EXPECT_STREQ(z_config_get(cfg, "", "key1", ""), "value1");
    // 行尾 # 不被特殊处理，整个 trim 后的字符串作为 value
    std::string v2 = z_config_get(cfg, "", "key2", "");
    EXPECT_NE(v2.find("value2"), std::string::npos);
    EXPECT_NE(v2.find("trailing"), std::string::npos);

    z_config_free(cfg);
    RemoveTmpFile(path);
}

// ============ trim ============

TEST(Config, KeyValue_Trimmed) {
    const std::string path = "zbase_test_config_trim.ini";
    std::string content =
        "   key1   =   value1   \n"
        "\tkey2\t=\tvalue2\t\n"
        "[ section1 ]\n"
        "  inner = val\n";
    WriteTmpFile(path, content);

    z_config_t* cfg = nullptr;
    ASSERT_EQ(z_config_load(path.c_str(), &cfg), Z_OK);

    EXPECT_STREQ(z_config_get(cfg, "", "key1", ""), "value1");
    EXPECT_STREQ(z_config_get(cfg, "", "key2", ""), "value2");
    // [ section1 ] 应该被 trim 为 "section1"
    EXPECT_STREQ(z_config_get(cfg, "section1", "inner", ""), "val");

    z_config_free(cfg);
    RemoveTmpFile(path);
}

// ============ 整数 ============

TEST(Config, GetInt_Basic) {
    const std::string path = "zbase_test_config_int.ini";
    std::string content =
        "zero = 0\n"
        "positive = 42\n"
        "negative = -7\n"
        "[s]\n"
        "v = 100\n";
    WriteTmpFile(path, content);

    z_config_t* cfg = nullptr;
    ASSERT_EQ(z_config_load(path.c_str(), &cfg), Z_OK);

    EXPECT_EQ(z_config_get_int(cfg, "", "zero", -1), 0);
    EXPECT_EQ(z_config_get_int(cfg, "", "positive", -1), 42);
    EXPECT_EQ(z_config_get_int(cfg, "", "negative", -1), -7);
    EXPECT_EQ(z_config_get_int(cfg, "s", "v", -1), 100);

    z_config_free(cfg);
    RemoveTmpFile(path);
}

TEST(Config, GetInt_InvalidValue_ReturnsDefault) {
    const std::string path = "zbase_test_config_int_invalid.ini";
    std::string content =
        "not_a_number = abc\n"
        "partial = 12abc\n"
        "empty =\n";
    WriteTmpFile(path, content);

    z_config_t* cfg = nullptr;
    ASSERT_EQ(z_config_load(path.c_str(), &cfg), Z_OK);

    EXPECT_EQ(z_config_get_int(cfg, "", "not_a_number", 999), 999);
    EXPECT_EQ(z_config_get_int(cfg, "", "partial", 999), 999);
    EXPECT_EQ(z_config_get_int(cfg, "", "empty", 999), 999);
    EXPECT_EQ(z_config_get_int(cfg, "", "missing", 999), 999);

    z_config_free(cfg);
    RemoveTmpFile(path);
}

// ============ 布尔 ============

TEST(Config, GetBool_Variants) {
    const std::string path = "zbase_test_config_bool.ini";
    std::string content =
        "t1 = true\n"
        "t2 = TRUE\n"
        "t3 = True\n"
        "t4 = 1\n"
        "t5 = yes\n"
        "t6 = ON\n"
        "f1 = false\n"
        "f2 = 0\n"
        "f3 = no\n"
        "f4 = off\n"
        "f5 = FALSE\n"
        "bad = random\n";
    WriteTmpFile(path, content);

    z_config_t* cfg = nullptr;
    ASSERT_EQ(z_config_load(path.c_str(), &cfg), Z_OK);

    EXPECT_EQ(z_config_get_bool(cfg, "", "t1", -1), 1);
    EXPECT_EQ(z_config_get_bool(cfg, "", "t2", -1), 1);
    EXPECT_EQ(z_config_get_bool(cfg, "", "t3", -1), 1);
    EXPECT_EQ(z_config_get_bool(cfg, "", "t4", -1), 1);
    EXPECT_EQ(z_config_get_bool(cfg, "", "t5", -1), 1);
    EXPECT_EQ(z_config_get_bool(cfg, "", "t6", -1), 1);

    EXPECT_EQ(z_config_get_bool(cfg, "", "f1", -1), 0);
    EXPECT_EQ(z_config_get_bool(cfg, "", "f2", -1), 0);
    EXPECT_EQ(z_config_get_bool(cfg, "", "f3", -1), 0);
    EXPECT_EQ(z_config_get_bool(cfg, "", "f4", -1), 0);
    EXPECT_EQ(z_config_get_bool(cfg, "", "f5", -1), 0);

    // 非法字符串 → 默认值
    EXPECT_EQ(z_config_get_bool(cfg, "", "bad", -1), -1);
    EXPECT_EQ(z_config_get_bool(cfg, "", "missing", 0), 0);

    z_config_free(cfg);
    RemoveTmpFile(path);
}

// ============ 中文 key/value（验收要求） ============

TEST(Config, Chinese_KeysAndValues) {
    const std::string path = "zbase_test_config_cn.ini";
    // 中文内容：名字=zbase、[服务器] 主机=本地
    std::string content =
        "\xE5\x90\x8D\xE5\xAD\x97 = zbase\n"  // 名字 = zbase
        "[\xE6\x9C\x8D\xE5\x8A\xA1\xE5\x99\xA8]\n"  // [服务器]
        "\xE4\xB8\xBB\xE6\x9C\xBA = \xE6\x9C\xAC\xE5\x9C\xB0\n"  // 主机 = 本地
        "\xE7\xAB\xAF\xE5\x8F\xA3 = 8080\n";  // 端口 = 8080
    WriteTmpFile(path, content);

    z_config_t* cfg = nullptr;
    ASSERT_EQ(z_config_load(path.c_str(), &cfg), Z_OK);

    // 名字 = zbase
    EXPECT_STREQ(z_config_get(cfg, "", "\xE5\x90\x8D\xE5\xAD\x97", ""), "zbase");
    // 服务器节下的 主机 = 本地
    EXPECT_STREQ(z_config_get(cfg, "\xE6\x9C\x8D\xE5\x8A\xA1\xE5\x99\xA8",
                              "\xE4\xB8\xBB\xE6\x9C\xBA", ""),
                 "\xE6\x9C\xAC\xE5\x9C\xB0");  // 本地
    EXPECT_EQ(z_config_get_int(cfg, "\xE6\x9C\x8D\xE5\x8A\xA1\xE5\x99\xA8",
                               "\xE7\xAB\xAF\xE5\x8F\xA3", -1), 8080);

    z_config_free(cfg);
    RemoveTmpFile(path);
}

// ============ UTF-8 BOM ============

TEST(Config, Utf8Bom_Skipped) {
    const std::string path = "zbase_test_config_bom.ini";
    // 文件首部加 UTF-8 BOM (EF BB BF)，第一行是 [s] 节
    std::string content =
        "\xEF\xBB\xBF[s]\n"
        "k = v\n";
    WriteTmpFile(path, content);

    z_config_t* cfg = nullptr;
    ASSERT_EQ(z_config_load(path.c_str(), &cfg), Z_OK);
    // 第一行 [s] 应该被正确解析为 section（如果 BOM 没跳过，section 名会带 BOM 字符）
    EXPECT_STREQ(z_config_get(cfg, "s", "k", ""), "v");

    z_config_free(cfg);
    RemoveTmpFile(path);
}

// ============ 空文件 / 边界 ============

TEST(Config, EmptyFile_LoadsOk) {
    const std::string path = "zbase_test_config_empty.ini";
    WriteTmpFile(path, "");

    z_config_t* cfg = nullptr;
    EXPECT_EQ(z_config_load(path.c_str(), &cfg), Z_OK);
    ASSERT_NE(cfg, nullptr);

    // 任何查询都应该返回默认值
    EXPECT_STREQ(z_config_get(cfg, "s", "k", "default"), "default");

    z_config_free(cfg);
    RemoveTmpFile(path);
}

TEST(Config, OnlyComments_LoadsOk) {
    const std::string path = "zbase_test_config_comments_only.ini";
    std::string content =
        "# all comments\n"
        "; nothing else\n"
        "# nothing\n";
    WriteTmpFile(path, content);

    z_config_t* cfg = nullptr;
    EXPECT_EQ(z_config_load(path.c_str(), &cfg), Z_OK);
    ASSERT_NE(cfg, nullptr);
    EXPECT_STREQ(z_config_get(cfg, "", "anything", "default"), "default");

    z_config_free(cfg);
    RemoveTmpFile(path);
}

TEST(Config, KeyCaseSensitive) {
    const std::string path = "zbase_test_config_case.ini";
    std::string content =
        "Key = upper\n"
        "key = lower\n";
    WriteTmpFile(path, content);

    z_config_t* cfg = nullptr;
    ASSERT_EQ(z_config_load(path.c_str(), &cfg), Z_OK);

    EXPECT_STREQ(z_config_get(cfg, "", "Key", ""), "upper");
    EXPECT_STREQ(z_config_get(cfg, "", "key", ""), "lower");

    z_config_free(cfg);
    RemoveTmpFile(path);
}

TEST(Config, SectionBeforeAnyDeclaration_GoesToEmptySection) {
    const std::string path = "zbase_test_config_no_section.ini";
    std::string content =
        "k1 = v1\n"
        "k2 = v2\n"
        "[s]\n"
        "k3 = v3\n";
    WriteTmpFile(path, content);

    z_config_t* cfg = nullptr;
    ASSERT_EQ(z_config_load(path.c_str(), &cfg), Z_OK);

    // [s] 之前的 k1/k2 归属空 section
    EXPECT_STREQ(z_config_get(cfg, "", "k1", ""), "v1");
    EXPECT_STREQ(z_config_get(cfg, "", "k2", ""), "v2");
    // [s] 节下没有 k1
    EXPECT_STREQ(z_config_get(cfg, "s", "k1", "nope"), "nope");
    EXPECT_STREQ(z_config_get(cfg, "s", "k3", ""), "v3");

    z_config_free(cfg);
    RemoveTmpFile(path);
}

// ============ C++ 包装 ============

TEST(ConfigCpp, BasicUsage) {
    const std::string path = "zbase_test_config_cpp.ini";
    std::string content =
        "name = zbase\n"
        "[s]\n"
        "port = 8080\n"
        "debug = true\n";
    WriteTmpFile(path, content);

    zbase::Config cfg(path);
    EXPECT_EQ(cfg.Get("", "name"), "zbase");
    EXPECT_EQ(cfg.Get("s", "port"), "8080");
    EXPECT_EQ(cfg.GetInt("s", "port", 0), 8080);
    EXPECT_TRUE(cfg.GetBool("s", "debug", false));
    EXPECT_FALSE(cfg.GetBool("s", "missing", false));

    // 缺省值
    EXPECT_EQ(cfg.Get("s", "missing", "fb"), "fb");
    EXPECT_EQ(cfg.GetInt("s", "missing", 999), 999);

    RemoveTmpFile(path);
}

TEST(ConfigCpp, FileNotFound_Throws) {
    try {
        zbase::Config cfg("nonexistent_zbase_cpp.ini");
        FAIL() << "应该抛异常";
    } catch (const zbase::Exception& e) {
        EXPECT_EQ(e.code(), Z_ERR_NOT_FOUND);
    }
}
