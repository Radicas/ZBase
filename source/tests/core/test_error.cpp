/**
 * @file test_error.cpp
 * @author 朱鹏 (Radica Zhu)
 * @date 2026-06-29
 * @version 0.1.0
 * @brief error 模块单元测试
 */

#include <gtest/gtest.h>
#include "zbase/error.h"
#include "zbase/zbase.h"

TEST(Error, OkIsZero) {
    EXPECT_EQ(Z_OK, 0);
}

TEST(Error, AllCodesNegativeExceptOk) {
    EXPECT_LT(Z_ERR_INVALID_ARG, 0);
    EXPECT_LT(Z_ERR_NOT_FOUND, 0);
    EXPECT_LT(Z_ERR_PERMISSION, 0);
    EXPECT_LT(Z_ERR_IO, 0);
    EXPECT_LT(Z_ERR_ENCODING, 0);
    EXPECT_LT(Z_ERR_NOMEM, 0);
    EXPECT_LT(Z_ERR_OVERFLOW, 0);
    EXPECT_LT(Z_ERR_UNSUPPORTED, 0);
    EXPECT_LT(Z_ERR_UNKNOWN, 0);
}

TEST(Error, SizeofIsInt) {
    EXPECT_EQ(sizeof(z_error_t), sizeof(int));
}

TEST(Error, MsgNotNull) {
    EXPECT_NE(z_error_msg(Z_OK), nullptr);
    EXPECT_NE(z_error_msg(Z_ERR_INVALID_ARG), nullptr);
    EXPECT_NE(z_error_msg(Z_ERR_UNKNOWN), nullptr);
}

TEST(Error, MsgIsChinese) {
    EXPECT_STREQ(z_error_msg(Z_OK), "成功");
    EXPECT_STREQ(z_error_msg(Z_ERR_INVALID_ARG), "参数非法");
    EXPECT_STREQ(z_error_msg(Z_ERR_ENCODING), "编码错误");
}

TEST(Error, MsgUnknownForOutOfRange) {
    EXPECT_NE(z_error_msg(12345), nullptr);
    EXPECT_STRNE(z_error_msg(12345), "");
}

TEST(Version, StringIsExpected) {
    EXPECT_STREQ(z_version(), "0.1.0");
}
