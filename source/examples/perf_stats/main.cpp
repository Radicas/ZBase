/**
 * @file main.cpp
 * @author 朱鹏 (Radica Zhu)
 * @date 2026-06-30
 * @version 1.0.0
 * @brief ZBase perf 模块示例：统计多个函数的调用次数与总耗时
 * @details 演示典型场景：客户有一批业务函数，希望在程序结束时
 *          汇总每个函数的「调用次数 + 总耗时 + 平均耗时」。
 *          展示三种用法：
 *            1. C++ RAII（zbase::PerfScope）—— 最推荐，零侵入
 *            2. C++ 宏（ZBASE_PERF_SCOPE）—— 简化写法
 *            3. C ABI（z_perf_start / z_perf_end）—— 手动控制
 *          运行后通过 z_perf_dump() 输出统计表。
 */

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "zbase/perf.h"
#include "zbase++/perf.hpp"
#include "zbase++/console.hpp"

// ============================================================
// 业务函数 1：字符串处理（演示 RAII 用法）
// ============================================================
static void ProcessString(int n) {
    zbase::PerfScope scope("ProcessString");  // 进入打点

    std::string s;
    for (int i = 0; i < n; ++i) {
        s += "hello,世界";
    }
    // 故意做点工作：统计 UTF-8 字节数
    volatile size_t len = s.size();
    (void)len;
}

// ============================================================
// 业务函数 2：数值计算（演示宏用法）
// ============================================================
static void ComputeNumbers(int n) {
    ZBASE_PERF_SCOPE("ComputeNumbers");  // 进入打点（宏版）

    volatile double sum = 0.0;
    for (int i = 0; i < n; ++i) {
        sum += i * 1.41421356;
    }
    (void)sum;
}

// ============================================================
// 业务函数 3：C ABI 方式手动打点（演示手动 start/end）
// ============================================================
static void CStyleWork(int n) {
    z_perf_start("CStyleWork");

    std::vector<int> v;
    v.reserve(n);
    for (int i = 0; i < n; ++i) {
        v.push_back(i);
    }

    z_perf_end("CStyleWork");  // 必须配对，否则不计入统计
}

// ============================================================
// 业务函数 4：含子调用（演示嵌套场景下的独立统计）
// ============================================================
static void InnerWork() {
    zbase::PerfScope scope("InnerWork");
    volatile double x = 0.0;
    for (int i = 0; i < 1000; ++i) {
        x += i * 0.5;
    }
    (void)x;
}

static void OuterWork() {
    zbase::PerfScope scope("OuterWork");  // 外层计时
    InnerWork();                          // 内层独立计时（不影响外层）
    InnerWork();                          // 同名打点累计 2 次
}

int main() {
    zbase::InitConsole();  // 初始化控制台 UTF-8，避免中文乱码
    std::printf("===== ZBase perf 模块示例：函数调用统计 =====\n\n");

    // 清空历史数据（防御性，避免上次运行残留）
    zbase::PerfReset();

    // --------------------------------------------------
    // 模拟客户业务：调用各函数若干次
    // --------------------------------------------------
    std::printf("[1] 模拟业务调用...\n");

    // ProcessString 调用 3 次
    ProcessString(1000);
    ProcessString(5000);
    ProcessString(2000);

    // ComputeNumbers 调用 2 次
    ComputeNumbers(100000);
    ComputeNumbers(50000);

    // CStyleWork 调用 4 次
    CStyleWork(100);
    CStyleWork(500);
    CStyleWork(200);
    CStyleWork(800);

    // OuterWork 调用 1 次（内部会调 InnerWork 2 次）
    OuterWork();

    std::printf("  共触发 5 个函数的统计打点\n\n");

    // --------------------------------------------------
    // 输出统计结果（按总耗时降序，输出到 stderr）
    // --------------------------------------------------
    std::printf("[2] 性能统计结果（输出到 stderr）：\n\n");
    zbase::PerfDump();

    // --------------------------------------------------
    // 说明
    // --------------------------------------------------
    std::printf("\n[3] 说明\n");
    std::printf("  - 调用次数：同名打点每次 end 后自动累加\n");
    std::printf("  - 总耗时：纳秒精度累加，输出转毫秒\n");
    std::printf("  - 平均耗时：总耗时 / 调用次数\n");
    std::printf("  - 嵌套场景：外层与内层各自独立计时，互不干扰\n");

    return 0;
}
