/**
 * @file error.cpp
 * @author 朱鹏 (Radica Zhu)
 * @date 2026-06-29
 * @version 0.1.0
 * @brief 错误码中文描述表
 */

#include "zbase/error.h"

namespace
{

    /// 错误码到描述的映射表
    struct ErrorEntry
    {
        int code;        ///< 错误码
        const char *msg; ///< 中文描述
    };

    const ErrorEntry kErrorTable[] = {
        {Z_OK, "成功"},
        {Z_ERR_INVALID_ARG, "参数非法"},
        {Z_ERR_NOT_FOUND, "找不到"},
        {Z_ERR_PERMISSION, "权限不足"},
        {Z_ERR_IO, "IO 错误"},
        {Z_ERR_ENCODING, "编码错误"},
        {Z_ERR_NOMEM, "内存不足"},
        {Z_ERR_OVERFLOW, "溢出"},
        {Z_ERR_UNSUPPORTED, "不支持"},
        {Z_ERR_CRASH_ALREADY_INSTALLED, "崩溃处理器已安装"},
        {Z_ERR_CRASH_INSTALL_FAILED, "崩溃处理器安装失败"},
        {Z_ERR_UNKNOWN, "未知错误"},
    };

    constexpr size_t kErrorTableSize =
        sizeof(kErrorTable) / sizeof(kErrorTable[0]);

} // namespace

extern "C"
{

    const char *z_error_msg(int err)
    {
        for (size_t i = 0; i < kErrorTableSize; ++i)
        {
            if (kErrorTable[i].code == err)
            {
                return kErrorTable[i].msg;
            }
        }
        return "未知错误码";
    }

} // extern "C"
