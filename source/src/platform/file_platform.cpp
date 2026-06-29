/**
 * @file file_platform.cpp
 * @author 朱鹏 (Radica Zhu)
 * @date 2026-06-29
 * @version 0.1.0
 * @brief 文件操作平台原语实现
 */

#include "platform/file_platform.hpp"
#include "platform/encoding.hpp"

#ifdef _WIN32
#include <Windows.h>
#else
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#endif

namespace zbase {
namespace platform {

#ifdef _WIN32

int NormalizePath(const std::string& utf8_path, std::wstring& out) {
    int err = Utf8ToWString(utf8_path, out);
    if (err != Z_OK) return err;
    // 路径长度 > 248 自动加 \\?\ 前缀（详见 phase-1-working-file.md §4.23）
    if (out.size() > 248 && out.substr(0, 4) != L"\\\\?\\") {
        // \\?\ 前缀要求路径用反斜杠
        for (auto& c : out) if (c == L'/') c = L'\\';
        out = L"\\\\?\\" + out;
    }
    return Z_OK;
}

int NormalizePath(const std::string& utf8_path, std::string& out) {
    out = utf8_path;
    return Z_OK;
}

int FileStatGet(const std::string& utf8_path, FileStat& out) {
    std::wstring wpath;
    int err = NormalizePath(utf8_path, wpath);
    if (err != Z_OK) return err;
    WIN32_FILE_ATTRIBUTE_DATA info{};
    if (!GetFileAttributesExW(wpath.c_str(), GetFileExInfoStandard, &info)) {
        DWORD e = GetLastError();
        if (e == ERROR_FILE_NOT_FOUND || e == ERROR_PATH_NOT_FOUND) {
            out.exists = false;
            return Z_OK;
        }
        return (e == ERROR_ACCESS_DENIED) ? Z_ERR_PERMISSION : Z_ERR_IO;
    }
    out.exists = true;
    out.is_dir = (info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
    out.size = (static_cast<uint64_t>(info.nFileSizeHigh) << 32) | info.nFileSizeLow;
    // FILETIME 是 100ns 间隔，转毫秒；FILETIME 起点 1601-01-01，需减去到 1970 的偏移
    uint64_t ft = (static_cast<uint64_t>(info.ftLastWriteTime.dwHighDateTime) << 32)
                  | info.ftLastWriteTime.dwLowDateTime;
    out.mtime = ft / 10000ULL - 11644473600000ULL;
    return Z_OK;
}

int MakeDirs(const std::string& utf8_path) {
    std::wstring wpath;
    int err = NormalizePath(utf8_path, wpath);
    if (err != Z_OK) return err;
    // 逐级创建：扫描路径分隔符位置，对每一级尝试 mkdir
    for (size_t i = 1; i < wpath.size(); ++i) {
        if (wpath[i] == L'\\' || wpath[i] == L'/') {
            std::wstring sub = wpath.substr(0, i);
            DWORD attrs = GetFileAttributesW(sub.c_str());
            if (attrs == INVALID_FILE_ATTRIBUTES) {
                if (!CreateDirectoryW(sub.c_str(), nullptr)) {
                    DWORD e = GetLastError();
                    if (e != ERROR_ALREADY_EXISTS) {
                        return (e == ERROR_ACCESS_DENIED) ? Z_ERR_PERMISSION : Z_ERR_IO;
                    }
                }
            }
        }
    }
    DWORD attrs = GetFileAttributesW(wpath.c_str());
    if (attrs == INVALID_FILE_ATTRIBUTES) {
        if (!CreateDirectoryW(wpath.c_str(), nullptr)) {
            DWORD e = GetLastError();
            return (e == ERROR_ALREADY_EXISTS) ? Z_OK
                 : (e == ERROR_ACCESS_DENIED) ? Z_ERR_PERMISSION : Z_ERR_IO;
        }
    }
    return Z_OK;
}

#else  // POSIX

int NormalizePath(const std::string& utf8_path, std::string& out) {
    out = utf8_path;
    return Z_OK;
}

int NormalizePath(const std::string& utf8_path, std::wstring& out) {
    return Utf8ToWString(utf8_path, out);
}

int FileStatGet(const std::string& utf8_path, FileStat& out) {
    struct stat st;
    if (::stat(utf8_path.c_str(), &st) != 0) {
        if (errno == ENOENT) { out.exists = false; return Z_OK; }
        if (errno == EACCES) return Z_ERR_PERMISSION;
        return Z_ERR_IO;
    }
    out.exists = true;
    out.is_dir = S_ISDIR(st.st_mode);
    out.size = static_cast<uint64_t>(st.st_size);
    out.mtime = static_cast<uint64_t>(st.st_mtime) * 1000ULL;
    return Z_OK;
}

int MakeDirs(const std::string& utf8_path) {
    const std::string& s = utf8_path;
    for (size_t i = 1; i < s.size(); ++i) {
        if (s[i] == '/') {
            std::string sub = s.substr(0, i);
            if (::mkdir(sub.c_str(), 0755) != 0 && errno != EEXIST) {
                return (errno == EACCES) ? Z_ERR_PERMISSION : Z_ERR_IO;
            }
        }
    }
    if (::mkdir(s.c_str(), 0755) != 0 && errno != EEXIST) {
        return (errno == EACCES) ? Z_ERR_PERMISSION : Z_ERR_IO;
    }
    return Z_OK;
}

#endif

}  // namespace platform
}  // namespace zbase
