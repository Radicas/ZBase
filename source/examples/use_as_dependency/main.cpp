/**
 * @file main.cpp
 * @author 朱鹏 (Radica Zhu)
 * @date 2026-06-30
 * @version 1.0.0
 * @brief 示例：如何在外部项目中使用 ZBase 作为依赖
 * @details 演示两种使用方式：
 *          1. C ABI（稳定接口，适合跨语言绑定）
 *          2. C++ 包装（header-only RAII，适合 C++ 项目）
 */

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

// ============ C ABI 方式 ============
// C ABI 头文件位于 zbase/ 目录下
#include "zbase/file.h"
#include "zbase/string.h"
#include "zbase/log.h"
#include "zbase/time.h"
#include "zbase/error.h"

// ============ C++ 包装方式 ============
// C++ 包装头文件位于 zbase++/ 目录下（header-only）
#include "zbase++/file.hpp"
#include "zbase++/string.hpp"
#include "zbase++/log.hpp"
#include "zbase++/time.hpp"
#include "zbase++/config.hpp"

int main() {
    printf("===== ZBase 外部项目使用示例 =====\n");
    printf("版本: %s\n\n", z_version());

    // ============================================
    // 1. C ABI 方式（稳定 C 接口）
    // ============================================
    printf("[1] C ABI 方式\n");

    // 文件操作（C ABI）
    const char* test_path = "zbase_example_test.txt";
    int r = z_file_write_text(test_path, "hello from C ABI\n", 19);
    if (r != Z_OK) {
        printf("  文件写入失败: %s\n", z_error_msg(r));
        return 1;
    }
    printf("  z_file_write_text: OK\n");

    char* buf = nullptr;
    size_t len = 0;
    r = z_file_read_text(test_path, &buf, &len);
    if (r == Z_OK && buf) {
        printf("  z_file_read_text: %zu bytes\n", len);
        z_file_free(buf);
    }

    // 字符串操作（C ABI）
    const char* input = "apple,banana,orange";
    char** parts = nullptr;
    size_t count = 0;
    r = z_string_split(input, strlen(input), ",", &parts, &count);
    if (r == Z_OK) {
        printf("  z_string_split: %zu parts\n", count);
        for (size_t i = 0; i < count; ++i) {
            printf("    [%zu] %s\n", i, parts[i]);
        }
        z_string_free_array(parts, count);
    }

    // ============================================
    // 2. C++ 包装方式（RAII，异常处理）
    // ============================================
    printf("\n[2] C++ 包装方式\n");

    try {
        // 文件操作（C++ RAII）
        zbase::WriteFile(test_path, "hello from C++ wrapper\n");
        printf("  zbase::WriteFile: OK\n");

        zbase::FileReader reader(test_path);
        printf("  zbase::FileReader: %zu bytes\n", reader.Size());

        // 字符串操作（C++）
        auto split_result = zbase::String::Split("one|two|three", "|");
        printf("  zbase::String::Split: %zu parts\n", split_result.size());

        // 时间操作（C++）
        int64_t now = zbase::Time::NowMs();
        std::string iso = zbase::Time::FormatIso(now);
        printf("  zbase::Time::FormatIso: %s\n", iso.c_str());

        // 日志（C++ RAII）
        zbase::LogFile log_file("zbase_example.log", 1024 * 1024, 3);
        Z_LOG_INFO("示例程序启动");
        Z_LOG_INFO("C++ 包装方式演示完成");
        printf("  zbase::LogFile: OK\n");

    } catch (const zbase::Exception& e) {
        printf("  异常: %s (错误码=%d)\n", e.what(), e.code());
        return 1;
    }

    // 清理测试文件
    std::remove(test_path);
    std::remove("zbase_example.log");

    printf("\n===== 示例运行完成 =====\n");
    return 0;
}
