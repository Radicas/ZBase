/**
 * @file perf_utils.hpp
 * @author 朱鹏 (Radica Zhu)
 * @date 2026-07-01
 * @version 1.0.0
 * @brief perf 模块便捷工具（C++ header-only）
 * @details 基于 z_perf_iterate 提供的高层组合工具，让 C++ 用户一行调用即可
 *          完成 perf 数据的常见消费场景：
 *            - PerfDumpToString  返回人类可读字符串（最通用，可塞日志/HTTP/GUI）
 *            - PerfDumpToLog     走 z_log_write 统一日志系统
 *            - PerfDumpToCsv     导出 CSV 文件（Excel/脚本分析）
 *            - PerfDumpToText    导出人类可读文本文件（归档/查看）
 *          全部 header-only，不占 C ABI 符号；不保证 ABI 稳定，可随版本调整。
 *          C 用户请直接使用 z_perf_iterate 自行实现。
 *          判断标准：本文件只放"基于 C ABI 的合理组合 + 仅依赖 STL/CRT/z_log_write"，
 *          不引入新依赖；HTTP/网络类工具不在此层。
 */

#pragma once

#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

#include "zbase/error.h"   // Z_OK
#include "zbase/file.h"    // z_file_write_text
#include "zbase/log.h"     // z_log_write, Z_LOG_LEVEL_INFO
#include "zbase/perf.h"    // z_perf_iterate

namespace zbase {

/// 单条 perf 数据快照（结构化）
struct PerfRecord {
    std::string name;    ///< 打点名（UTF-8）
    uint64_t    total_ns; ///< 累计耗时（纳秒）
    uint64_t    count;    ///< 调用次数
};

namespace detail {

/// z_perf_iterate 回调：收集单条记录到 vector
inline void CollectPerfRecord(const char* name, uint64_t total_ns,
                              uint64_t count, void* user_data) {
    auto* v = static_cast<std::vector<PerfRecord>*>(user_data);
    v->push_back({std::string(name), total_ns, count});
}

/// 一次性收集所有 perf 记录（按总耗时降序，库已排好序）
inline std::vector<PerfRecord> CollectAllPerf() {
    std::vector<PerfRecord> v;
    z_perf_iterate(&CollectPerfRecord, &v);
    return v;
}

/// 格式化单条记录为人类可读行（与 z_perf_dump 每行同格式）
inline std::string FormatPerfTextLine(const PerfRecord& r) {
    double ms = static_cast<double>(r.total_ns) / 1000000.0;
    char buf[256];
    std::snprintf(buf, sizeof(buf),
                  "  %-30s %12.3f 毫秒  %10llu 次  平均 %.3f 毫秒\n",
                  r.name.c_str(), ms,
                  static_cast<unsigned long long>(r.count),
                  r.count ? ms / static_cast<double>(r.count) : 0.0);
    return std::string(buf);
}

/// 格式化单条记录为 CSV 行（不含表头）
inline std::string FormatPerfCsvLine(const PerfRecord& r) {
    double ms = static_cast<double>(r.total_ns) / 1000000.0;
    char buf[256];
    std::snprintf(buf, sizeof(buf), "%s,%.3f,%llu,%.3f\n",
                  r.name.c_str(), ms,
                  static_cast<unsigned long long>(r.count),
                  r.count ? ms / static_cast<double>(r.count) : 0.0);
    return std::string(buf);
}

}  // namespace detail

/// 把 perf 统计格式化为人类可读字符串
/// @return 与 z_perf_dump 每行同格式的字符串（不含头尾装饰行）
/// @note 装饰行（====/====）省略，便于嵌入日志、HTTP 响应、GUI 等上下文
inline std::string PerfDumpToString() {
    std::string s;
    for (const auto& r : detail::CollectAllPerf()) {
        s += detail::FormatPerfTextLine(r);
    }
    return s;
}

/// 把 perf 统计通过 z_log_write 输出（每条记录一行，级别 INFO）
/// @note 需先调用 z_log_init；未初始化时 z_log_write 仍写 stderr（按 log 模块语义）
/// @note 日志来源标记为 "perf"，便于在日志中筛选举例 perf 数据
inline void PerfDumpToLog() {
    for (const auto& r : detail::CollectAllPerf()) {
        double ms = static_cast<double>(r.total_ns) / 1000000.0;
        z_log_write(Z_LOG_LEVEL_INFO, "perf", 0,
                    "%s: %.3f 毫秒 / %llu 次 / 平均 %.3f 毫秒",
                    r.name.c_str(), ms,
                    static_cast<unsigned long long>(r.count),
                    r.count ? ms / static_cast<double>(r.count) : 0.0);
    }
}

/// 把 perf 统计导出为 CSV 文件
/// @param path 文件路径（UTF-8，支持中文路径）
/// @return true 成功；false 文件写入失败
/// @details CSV 列：name,total_ms,count,avg_ms；首行为表头
inline bool PerfDumpToCsv(const std::string& path) {
    std::string content = "name,total_ms,count,avg_ms\n";
    for (const auto& r : detail::CollectAllPerf()) {
        content += detail::FormatPerfCsvLine(r);
    }
    return z_file_write_text(path.c_str(), content.c_str(), content.size()) == Z_OK;
}

/// 把 perf 统计导出为人类可读文本文件
/// @param path 文件路径（UTF-8，支持中文路径）
/// @return true 成功；false 文件写入失败
/// @details 含头尾装饰行，格式与 z_perf_dump 控制台输出一致
inline bool PerfDumpToText(const std::string& path) {
    std::string content = "===== ZBase 性能统计（文件输出）=====\n";
    content += PerfDumpToString();
    content += "====================================\n";
    return z_file_write_text(path.c_str(), content.c_str(), content.size()) == Z_OK;
}

}  // namespace zbase
