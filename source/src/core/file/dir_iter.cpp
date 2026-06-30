/**
 * @file dir_iter.cpp
 * @author 朱鹏 (Radica Zhu)
 * @date 2026-06-29
 * @version 0.1.0
 * @brief 目录遍历实现
 * @details Windows: FindFirstFileW / FindNextFileW
 *          POSIX: opendir / readdir
 *          条目名内部转为 UTF-8 输出，迭代器销毁时释放。
 */

#include "zbase/file.h"
#include "zbase/error.h"
#include "platform/encoding.hpp"
#include "platform/file_platform.hpp"

#include <cstring>
#include <new>
#include <string>

#ifdef _WIN32
#include <Windows.h>
#else
#include <dirent.h>
#include <sys/stat.h>
#endif

struct z_dir_iter {
#ifdef _WIN32
    HANDLE find_handle = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATAW find_data;
    bool first = true;
#else
    DIR* dir = nullptr;
    std::string base_path;
#endif
    std::string current_name;  ///< 当前条目名（UTF-8）
};

extern "C" {

int z_dir_iter_create(const char* path, z_dir_iter** out_iter) {
    if (path == nullptr || out_iter == nullptr) return Z_ERR_INVALID_ARG;
    *out_iter = nullptr;

    zbase::platform::FileStat st;
    int err = zbase::platform::FileStatGet(path, st);
    if (err != Z_OK) return err;
    if (!st.exists) return Z_ERR_NOT_FOUND;
    if (!st.is_dir) return Z_ERR_INVALID_ARG;

    z_dir_iter* it = new (std::nothrow) z_dir_iter();
    if (it == nullptr) return Z_ERR_NOMEM;

#ifdef _WIN32
    std::wstring wpath;
    err = zbase::platform::NormalizePath(std::string(path) + "/*", wpath);
    if (err != Z_OK) { delete it; return err; }
    it->find_handle = FindFirstFileW(wpath.c_str(), &it->find_data);
    if (it->find_handle == INVALID_HANDLE_VALUE) {
        delete it;
        return Z_ERR_IO;
    }
    it->first = true;
#else
    it->dir = ::opendir(path);
    if (it->dir == nullptr) {
        delete it;
        return Z_ERR_IO;
    }
    it->base_path = path;
#endif

    *out_iter = it;
    return Z_OK;
}

int z_dir_iter_next(z_dir_iter* iter, z_dir_iter_entry_t* out_entry) {
    if (iter == nullptr || out_entry == nullptr) return Z_ERR_INVALID_ARG;
    // 校验 reserved 必须置零（详见 architecture.md §13.2）
    for (size_t i = 0; i < 8; ++i) {
        if (out_entry->reserved[i] != nullptr) return Z_ERR_INVALID_ARG;
    }

    std::memset(out_entry, 0, sizeof(*out_entry));

#ifdef _WIN32
    while (true) {
        if (!iter->first) {
            if (!FindNextFileW(iter->find_handle, &iter->find_data)) {
                DWORD e = GetLastError();
                if (e == ERROR_NO_MORE_FILES) return Z_ERR_NOT_FOUND;
                return Z_ERR_IO;
            }
        }
        iter->first = false;

        std::wstring wname(iter->find_data.cFileName);
        std::string name;
        zbase::platform::WStringToUtf8(wname, name);
        iter->current_name = std::move(name);

        out_entry->name = iter->current_name.c_str();
        out_entry->is_dir = (iter->find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0 ? 1 : 0;
        out_entry->size = (static_cast<uint64_t>(iter->find_data.nFileSizeHigh) << 32)
                          | iter->find_data.nFileSizeLow;
        uint64_t ft = (static_cast<uint64_t>(iter->find_data.ftLastWriteTime.dwHighDateTime) << 32)
                      | iter->find_data.ftLastWriteTime.dwLowDateTime;
        out_entry->mtime = ft / 10000ULL - 11644473600000ULL;
        return Z_OK;
    }
#else
    while (true) {
        struct dirent* ent = ::readdir(iter->dir);
        if (ent == nullptr) return Z_ERR_NOT_FOUND;
        iter->current_name = ent->d_name;
        std::string full = iter->base_path + "/" + ent->d_name;
        zbase::platform::FileStat st;
        int err = zbase::platform::FileStatGet(full, st);
        if (err != Z_OK || !st.exists) continue;
        out_entry->name = iter->current_name.c_str();
        out_entry->is_dir = st.is_dir ? 1 : 0;
        out_entry->size = st.size;
        out_entry->mtime = st.mtime;
        return Z_OK;
    }
#endif
}

void z_dir_iter_destroy(z_dir_iter* iter) {
    if (iter == nullptr) return;
#ifdef _WIN32
    if (iter->find_handle != INVALID_HANDLE_VALUE) {
        FindClose(iter->find_handle);
    }
#else
    if (iter->dir != nullptr) {
        ::closedir(iter->dir);
    }
#endif
    delete iter;
}

}  // extern "C"
