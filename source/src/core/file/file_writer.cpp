/**
 * @file file_writer.cpp
 * @author 朱鹏 (Radica Zhu)
 * @date 2026-06-29
 * @version 0.1.0
 * @brief 文件写入实现（原子写：写临时文件 + rename）
 */

#include "zbase/file.h"
#include "zbase/error.h"
#include "zbase/internal/c_api_guard.hpp"
#include "platform/file_platform.hpp"

#include <cstdlib>
#include <cstring>
#include <string>

#ifdef _WIN32
#include <Windows.h>
#else
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#ifndef O_BINARY
#define O_BINARY 0
#endif
#endif

extern "C" {

int z_file_write_text(const char* path, const char* content, size_t len) {
    ZBASE_C_API_BEGIN
    if (path == nullptr || (content == nullptr && len > 0)) {
        return Z_ERR_INVALID_ARG;
    }
    std::string tmp = std::string(path) + ".zbase.tmp";

#ifdef _WIN32
    std::wstring wtmp;
    int err = zbase::platform::NormalizePath(tmp, wtmp);
    if (err != Z_OK) return err;
    HANDLE h = CreateFileW(wtmp.c_str(), GENERIC_WRITE, 0, nullptr,
                           CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE) {
        DWORD e = GetLastError();
        return (e == ERROR_ACCESS_DENIED) ? Z_ERR_PERMISSION : Z_ERR_IO;
    }
    DWORD written = 0;
    if (len > 0) {
        if (!WriteFile(h, content, static_cast<DWORD>(len), &written, nullptr)) {
            CloseHandle(h);
            return Z_ERR_IO;
        }
    }
    CloseHandle(h);
    if (written != len) return Z_ERR_IO;

    std::wstring wpath;
    err = zbase::platform::NormalizePath(path, wpath);
    if (err != Z_OK) return err;
    if (!MoveFileExW(wtmp.c_str(), wpath.c_str(),
                     MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        return Z_ERR_IO;
    }
    return Z_OK;
#else
    int fd = ::open(tmp.c_str(), O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, 0644);
    if (fd < 0) return (errno == EACCES) ? Z_ERR_PERMISSION : Z_ERR_IO;
    size_t total = 0;
    while (total < len) {
        ssize_t n = ::write(fd, content + total, len - total);
        if (n < 0) { ::close(fd); ::unlink(tmp.c_str()); return Z_ERR_IO; }
        total += static_cast<size_t>(n);
    }
    ::close(fd);
    if (::rename(tmp.c_str(), path) != 0) {
        ::unlink(tmp.c_str());
        return Z_ERR_IO;
    }
    return Z_OK;
#endif
    ZBASE_C_API_END(Z_ERR_UNKNOWN)
}

int z_file_exists(const char* path) {
    ZBASE_C_API_BEGIN
    if (path == nullptr) return Z_ERR_INVALID_ARG;
    zbase::platform::FileStat st;
    int err = zbase::platform::FileStatGet(path, st);
    if (err != Z_OK) return err;
    return st.exists ? 1 : 0;
    ZBASE_C_API_END(Z_ERR_UNKNOWN)
}

int z_file_mkdir(const char* path) {
    ZBASE_C_API_BEGIN
    if (path == nullptr) return Z_ERR_INVALID_ARG;
    return zbase::platform::MakeDirs(path);
    ZBASE_C_API_END(Z_ERR_UNKNOWN)
}

void z_file_free(void* p) {
    if (p == nullptr) return;
    std::free(p);
}

}  // extern "C"
