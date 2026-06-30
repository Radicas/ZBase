/**
 * @file config.cpp
 * @author 朱鹏 (Radica Zhu)
 * @date 2026-06-30
 * @version 0.2.0
 * @brief INI 配置文件解析实现
 * @details 内部用 std::map<std::pair<string, string>, string> 存储 (section, key) → value。
 *          解析按行处理：
 *            1. 跳过 UTF-8 BOM（EF BB BF）
 *            2. 行首 trim 后是 # 或 ; → 注释，跳过
 *            3. 行首 trim 后是 [ 开头 → 解析 section（去 [ ]，trim）
 *            4. 含 = → 拆 key / value，分别 trim
 *            5. 空行跳过
 *          内存所有权：z_config_get 返回的指针指向 map 内部 std::string 的 c_str，
 *          在 z_config_free 之前一直有效。
 */

#include "zbase/config.h"
#include "zbase/error.h"
#include "zbase/internal/c_api_guard.hpp"
#include "platform/file_platform.hpp"

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <memory>
#include <string>
#include <utility>

namespace {

/// trim 字符串首尾空白（仅 ASCII 空白：space, \t, \n, \r, \v, \f）
/// @param s 输入字符串（in-out）
void Trim(std::string& s) {
    auto is_space = [](unsigned char c) {
        return std::isspace(c) != 0;
    };
    while (!s.empty() && is_space(static_cast<unsigned char>(s.front()))) s.erase(s.begin());
    while (!s.empty() && is_space(static_cast<unsigned char>(s.back())))  s.pop_back();
}

/// 转小写（仅 ASCII，不影响 UTF-8 多字节）
/// @param s 输入字符串
/// @return 转小写后的副本
std::string ToLowerAscii(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) -> char {
                       return static_cast<char>(std::tolower(c));
                   });
    return s;
}

}  // namespace

/// 配置数据结构（实现 opaque 句柄 z_config_t 的具体类型）
/// 必须放在全局命名空间，与 config.h 中 typedef struct z_config_t z_config_t; 对齐
struct z_config_t {
    /// (section, key) → value
    std::map<std::pair<std::string, std::string>, std::string> entries;
};

extern "C" {

int z_config_load(const char* path, z_config_t** out_config) {
    ZBASE_C_API_BEGIN
    if (path == nullptr || out_config == nullptr) return Z_ERR_INVALID_ARG;
    *out_config = nullptr;

    // 读取文件到字符串（复用 file_platform 的 stat + 直接读）
    zbase::platform::FileStat st;
    int err = zbase::platform::FileStatGet(path, st);
    if (err != Z_OK) return err;
    if (!st.exists) return Z_ERR_NOT_FOUND;
    if (st.is_dir)  return Z_ERR_INVALID_ARG;

#ifdef _WIN32
    std::wstring wpath;
    err = zbase::platform::NormalizePath(path, wpath);
    if (err != Z_OK) return err;
    FILE* fp = nullptr;
    if (_wfopen_s(&fp, wpath.c_str(), L"rb") != 0 || fp == nullptr) {
        return Z_ERR_IO;
    }
#else
    FILE* fp = std::fopen(path, "rb");
    if (fp == nullptr) {
        return (errno == EACCES) ? Z_ERR_PERMISSION : Z_ERR_IO;
    }
#endif
    // 用 RAII 保证关闭
    struct FileCloser {
        FILE* f;
        ~FileCloser() { if (f) std::fclose(f); }
    } closer{fp};

    // 一次性读到 string
    std::string content;
    content.resize(static_cast<size_t>(st.size));
    size_t read_total = 0;
    while (read_total < content.size()) {
        size_t n = std::fread(&content[read_total], 1, content.size() - read_total, fp);
        if (n == 0) {
            if (std::ferror(fp)) return Z_ERR_IO;
            break;  // EOF
        }
        read_total += n;
    }
    content.resize(read_total);

    // 跳过 UTF-8 BOM
    if (content.size() >= 3 &&
        static_cast<unsigned char>(content[0]) == 0xEF &&
        static_cast<unsigned char>(content[1]) == 0xBB &&
        static_cast<unsigned char>(content[2]) == 0xBF) {
        content.erase(0, 3);
    }

    auto cfg = std::make_unique<z_config_t>();
    std::string current_section;  // 默认空 section

    size_t pos = 0;
    while (pos < content.size()) {
        // 取一行
        size_t line_end = content.find('\n', pos);
        std::string line = (line_end == std::string::npos)
                           ? content.substr(pos)
                           : content.substr(pos, line_end - pos);
        pos = (line_end == std::string::npos) ? content.size() : line_end + 1;

        // 去掉 \r
        if (!line.empty() && line.back() == '\r') line.pop_back();

        // trim 整行
        Trim(line);

        // 空行 / 注释
        if (line.empty()) continue;
        if (line.front() == '#' || line.front() == ';') continue;

        // section [xxx]
        if (line.front() == '[') {
            size_t rb = line.find(']');
            if (rb == std::string::npos) continue;  // 非法 section，跳过
            current_section = line.substr(1, rb - 1);
            Trim(current_section);
            continue;
        }

        // key = value
        size_t eq = line.find('=');
        if (eq == std::string::npos) continue;  // 非 kv 行，跳过

        std::string key = line.substr(0, eq);
        std::string val = line.substr(eq + 1);
        Trim(key);
        Trim(val);
        if (key.empty()) continue;  // 空 key，跳过

        cfg->entries[{current_section, key}] = val;
    }

    *out_config = cfg.release();
    return Z_OK;
    ZBASE_C_API_END(Z_ERR_UNKNOWN)
}

void z_config_free(z_config_t* config) {
    // delete nullptr 安全；用 try/catch 防止异常逃逸
    try {
        delete config;
    } catch (...) {
        // 静默：释放失败不影响调用方
    }
}

const char* z_config_get(z_config_t* config, const char* section, const char* key,
                         const char* default_val) {
    // 不能用 ZBASE_C_API_BEGIN（返回 const char*，不是 int）
    try {
        if (config == nullptr || key == nullptr) return default_val;
        std::string sec = (section == nullptr) ? std::string{} : std::string{section};
        auto it = config->entries.find({sec, key});
        if (it == config->entries.end()) return default_val;
        return it->second.c_str();
    } catch (...) {
        return default_val;
    }
}

int z_config_get_int(z_config_t* config, const char* section, const char* key,
                     int default_val) {
    ZBASE_C_API_BEGIN
    const char* val = z_config_get(config, section, key, nullptr);
    if (val == nullptr) return default_val;
    // 解析整数：用 strtol 容忍前导空白；遇非法字符或空串返回 default
    char* end = nullptr;
    long n = std::strtol(val, &end, 10);
    // end == val 表示 strtol 没消费任何字符（空串或纯空白）→ 非法
    if (end == val) return default_val;
    // end 后允许仅剩空白
    while (*end != '\0' && std::isspace(static_cast<unsigned char>(*end))) {
        ++end;
    }
    if (*end != '\0') return default_val;
    return static_cast<int>(n);
    ZBASE_C_API_END(default_val)
}

int z_config_get_bool(z_config_t* config, const char* section, const char* key,
                      int default_val) {
    ZBASE_C_API_BEGIN
    const char* val = z_config_get(config, section, key, nullptr);
    if (val == nullptr) return default_val;
    std::string s = ToLowerAscii(val);
    Trim(s);
    if (s == "true" || s == "1" || s == "yes" || s == "on")  return 1;
    if (s == "false" || s == "0" || s == "no" || s == "off")  return 0;
    return default_val;
    ZBASE_C_API_END(default_val)
}

}  // extern "C"
