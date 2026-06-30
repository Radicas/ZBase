/**
 * @file file_reader.cpp
 * @author 朱鹏 (Radica Zhu)
 * @date 2026-06-29
 * @version 0.1.0
 * @brief 文件读取实现
 */

#include "zbase/file.h"
#include "zbase/error.h"
#include "zbase/internal/c_api_guard.hpp"
#include "platform/encoding.hpp"
#include "platform/file_platform.hpp"

#include <cstdlib>
#include <cstring>

#ifdef _WIN32
#include <Windows.h>
#else
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#ifndef O_BINARY
#define O_BINARY 0
#endif
#endif

extern "C" {

int z_file_read_text(const char* path, char** out_buf, size_t* out_len) {
    ZBASE_C_API_BEGIN
    if (path == nullptr || out_buf == nullptr || out_len == nullptr) {
        return Z_ERR_INVALID_ARG;
    }
    *out_buf = nullptr;
    *out_len = 0;

    zbase::platform::FileStat st;
    int err = zbase::platform::FileStatGet(path, st);
    if (err != Z_OK) return err;
    if (!st.exists) return Z_ERR_NOT_FOUND;
    if (st.is_dir) return Z_ERR_INVALID_ARG;

#ifdef _WIN32
    std::wstring wpath;
    err = zbase::platform::NormalizePath(path, wpath);
    if (err != Z_OK) return err;
    HANDLE h = CreateFileW(wpath.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
                           OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE) {
        DWORD e = GetLastError();
        return (e == ERROR_ACCESS_DENIED) ? Z_ERR_PERMISSION : Z_ERR_IO;
    }
    DWORD size = static_cast<DWORD>(st.size);
    char* buf = static_cast<char*>(std::malloc(size ? size : 1));
    if (buf == nullptr) { CloseHandle(h); return Z_ERR_NOMEM; }
    DWORD read_bytes = 0;
    if (!ReadFile(h, buf, size, &read_bytes, nullptr)) {
        CloseHandle(h);
        std::free(buf);
        return Z_ERR_IO;
    }
    CloseHandle(h);
    *out_buf = buf;
    *out_len = read_bytes;
    return Z_OK;
#else
    int fd = ::open(path, O_RDONLY | O_BINARY);
    if (fd < 0) {
        return (errno == EACCES) ? Z_ERR_PERMISSION : Z_ERR_IO;
    }
    char* buf = static_cast<char*>(std::malloc(st.size ? st.size : 1));
    if (buf == nullptr) { ::close(fd); return Z_ERR_NOMEM; }
    size_t total = 0;
    while (total < st.size) {
        ssize_t n = ::read(fd, buf + total, st.size - total);
        if (n < 0) { ::close(fd); std::free(buf); return Z_ERR_IO; }
        if (n == 0) break;
        total += static_cast<size_t>(n);
    }
    ::close(fd);
    *out_buf = buf;
    *out_len = total;
    return Z_OK;
#endif
    ZBASE_C_API_END(Z_ERR_UNKNOWN)
}

}  // extern "C"
