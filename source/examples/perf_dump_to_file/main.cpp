/**
 * @file main.cpp
 * @author 朱鹏 (Radica Zhu)
 * @date 2026-07-01
 * @version 1.1.0
 * @brief ZBase perf 示例：用 perf_utils 便捷工具把性能统计 dump 到文件/字符串/日志
 * @details 演示 zbase::PerfDumpToString / PerfDumpToLog / PerfDumpToCsv / PerfDumpToText
 *          四个 header-only 便捷函数的用法。这组函数基于 z_perf_iterate 组合而成，
 *          让 C++ 用户一行调用即可完成 perf 数据的常见消费场景。
 *          典型场景：长跑服务 / CI 任务希望把 perf 结果落盘做后续分析，
 *                    而不是只看控制台输出。
 */

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "zbase/log.h"          // z_log_init / z_log_shutdown
#include "zbase/perf.h"
#include "zbase++/console.hpp"
#include "zbase++/perf.hpp"
#include "zbase++/perf_utils.hpp"

// ============================================================
// 业务函数：模拟几种典型 workload
// ============================================================

/// 字符串拼接（演示 RAII 打点）
static void BuildString(int n) {
    zbase::PerfScope scope("BuildString");
    std::string s;
    s.reserve(static_cast<size_t>(n) * 12);
    for (int i = 0; i < n; ++i) {
        s += "hello,世界";
    }
    volatile size_t len = s.size();  // 防优化
    (void)len;
}

/// 数值累加（演示宏打点）
static void ComputeNumbers(int n) {
    ZBASE_PERF_SCOPE("ComputeNumbers");
    volatile double sum = 0.0;
    for (int i = 0; i < n; ++i) {
        sum += i * 1.41421356;
    }
    (void)sum;
}

/// 向量排序（演示 C ABI 手动 start/end）
static void SortVector(int n) {
    z_perf_start("SortVector");
    std::vector<int> v;
    v.reserve(static_cast<size_t>(n));
    for (int i = n; i > 0; --i) {
        v.push_back(i);
    }
    std::sort(v.begin(), v.end());
    z_perf_end("SortVector");
}

// ============================================================
// 主流程
// ============================================================

int main() {
    zbase::InitConsole();  // 初始化控制台 UTF-8，避免中文乱码
    std::printf("===== ZBase perf 示例：perf_utils 便捷工具 =====\n\n");

    // 清空历史数据（防御性）
    zbase::PerfReset();

    // --------------------------------------------------
    // [1] 模拟业务调用，触发各函数的打点
    // --------------------------------------------------
    std::printf("[1] 模拟业务调用...\n");
    BuildString(2000);
    BuildString(5000);
    ComputeNumbers(100000);
    ComputeNumbers(50000);
    SortVector(1000);
    SortVector(5000);
    SortVector(2000);
    std::printf("  共触发 3 个函数的统计打点\n\n");

    // --------------------------------------------------
    // [2] 控制台输出（z_perf_dump → stderr，作对照）
    // --------------------------------------------------
    std::printf("[2] 控制台输出（z_perf_dump → stderr）：\n\n");
    zbase::PerfDump();

    // --------------------------------------------------
    // [3] PerfDumpToString：拿到字符串后自行处理
    // --------------------------------------------------
    std::printf("\n[3] PerfDumpToString（前 2 行预览）：\n");
    std::string text = zbase::PerfDumpToString();
    // 演示：只打印前 2 行（实际可塞日志、HTTP 响应、GUI 等）
    int line = 0;
    size_t pos = 0;
    while (pos < text.size() && line < 2) {
        size_t end = text.find('\n', pos);
        if (end == std::string::npos) break;
        std::printf("  %s\n", text.substr(pos, end - pos).c_str());
        pos = end + 1;
        ++line;
    }

    // --------------------------------------------------
    // [4] PerfDumpToLog：走 z_log_write 统一日志系统
    // --------------------------------------------------
    std::printf("\n[4] PerfDumpToLog（先 z_log_init，再输出到日志）：\n");
    z_log_init(Z_LOG_LEVEL_INFO);  // 初始化日志系统（最低 INFO 级）
    zbase::PerfDumpToLog();        // 每条 perf 记录以 INFO 级写入 stderr
    z_log_shutdown();

    // --------------------------------------------------
    // [5] PerfDumpToCsv：CSV 文件输出
    // --------------------------------------------------
    const std::string csv_path = "perf_report.csv";
    if (zbase::PerfDumpToCsv(csv_path)) {
        std::printf("\n[5] PerfDumpToCsv 已生成: %s\n", csv_path.c_str());
    } else {
        std::printf("\n[5] PerfDumpToCsv 失败: %s\n", csv_path.c_str());
    }

    // --------------------------------------------------
    // [6] PerfDumpToText：人类可读文本文件输出
    // --------------------------------------------------
    const std::string txt_path = "perf_report.txt";
    if (zbase::PerfDumpToText(txt_path)) {
        std::printf("[6] PerfDumpToText 已生成: %s\n", txt_path.c_str());
    } else {
        std::printf("[6] PerfDumpToText 失败: %s\n", txt_path.c_str());
    }

    // --------------------------------------------------
    // [7] 说明
    // --------------------------------------------------
    std::printf("\n[7] 说明\n");
    std::printf("  - 4 个便捷函数均在 zbase++/perf_utils.hpp（header-only，不占 C ABI 符号）\n");
    std::printf("  - 全部基于 z_perf_iterate，库帮排序，回调拿到 (name, total_ns, count)\n");
    std::printf("  - PerfDumpToString 最通用，拿到字符串后想干嘛干嘛\n");
    std::printf("  - PerfDumpToLog 走 z_log_write，与现有日志系统合一\n");
    std::printf("  - PerfDumpToCsv/Text 内部走 z_file_write_text，支持中文路径\n");
    std::printf("  - C 用户请直接用 z_perf_iterate 自行实现\n");

    return 0;
}
