/**
 * @file demo.cpp
 * @author 朱鹏 (Radica Zhu)
 * @date 2026-06-30
 * @version 0.1.0
 * @brief ZBase v0.1 综合示例：读文件 → 字符串处理 → perf 计时 → log 输出
 * @details 展示 zbase++/ C++ 包装层端到端使用。运行后会留下 perf dump
 *          与一条 ISO 时间日志。文件读写在临时目录完成，运行结束清理。
 */

#include <cstdio>
#include <cstdlib>
#include <string>

#include "zbase/zbase.h"
#include "zbase++/file.hpp"
#include "zbase++/log.hpp"
#include "zbase++/perf.hpp"
#include "zbase++/string.hpp"
#include "zbase++/time.hpp"

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    zbase::Logger logger(Z_LOG_LEVEL_DEBUG);

    Z_LOG_INFO("ZBase v%s 启动", z_version());

    // 准备测试文件
    const char* path = "zbase_demo_input.txt";
    zbase::WriteFile(path, "hello,world,你好,世界");

    try {
        ZBASE_PERF_SCOPE("demo_total");

        // 1. 读文件
        zbase::FileReader reader(path);
        Z_LOG_INFO("读到 %zu 字节", reader.Size());

        // 2. 字符串处理
        auto parts = zbase::StringSplit(reader.Str(), ",");
        Z_LOG_INFO("分隔得到 %zu 个片段", parts.size());

        // 3. 拼接回去
        std::string joined = zbase::StringJoin(parts, "|");
        Z_LOG_INFO("拼接结果: %s", joined.c_str());

        // 4. UTF-8 长度
        Z_LOG_INFO("UTF-8 字符数: %zu", zbase::StringUtf8Len(joined));
    } catch (const zbase::Exception& e) {
        Z_LOG_ERROR("ZBase 异常: %s (code=%d)", e.what(), e.code());
        std::remove(path);
        return 1;
    }

    zbase::PerfDump();
    Z_LOG_INFO("当前时间: %s", zbase::FormatIso(zbase::NowMs()).c_str());

    std::remove(path);
    return 0;
}
