/**
 * @file zbase_version.cpp
 * @author 朱鹏 (Radica Zhu)
 * @date 2026-06-29
 * @version 0.1.0
 * @brief z_version 函数实现
 */

#include "zbase/zbase.h"

extern "C"
{

    const char *z_version(void)
    {
        return ZBASE_VERSION_STRING;
    }

} // extern "C"
