/**
 * @file file.h
 * @author 朱鹏 (Radica Zhu)
 * @date 2026-06-29
 * @version 0.1.0
 * @brief 文件操作 C ABI
 * @details 提供 read_text / write_text / exists / mkdir / free / dir_iter 共 8 个函数。
 *          路径全部 UTF-8，Windows 内部转 UTF-16 走 W 版 Win32 API。
 *          write_text 用"临时文件 + rename"实现原子写。
 */

#ifndef ZBASE_FILE_H_
#define ZBASE_FILE_H_

#include <stddef.h>
#include <stdint.h>
#include "zbase/zbase.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 读取文本文件全部内容
 * @param path UTF-8 路径
 * @param out_buf 输出缓冲区（库内申请，调 z_file_free 释放）
 * @param out_len 输出长度（字节数）
 * @return Z_OK 成功；Z_ERR_INVALID_ARG；Z_ERR_NOT_FOUND 文件不存在；
 *         Z_ERR_PERMISSION；Z_ERR_IO；Z_ERR_NOMEM
 * @note 二进制模式读取，不转换换行符（详见 phase-1-working-file.md §4.22）
 */
ZBASE_API int z_file_read_text(const char* path, char** out_buf, size_t* out_len);

/**
 * @brief 写文本到文件（原子写：写临时文件 + rename）
 * @param path 目标 UTF-8 路径
 * @param content 内容
 * @param len 内容长度（字节）
 * @return Z_OK 成功；Z_ERR_INVALID_ARG；Z_ERR_PERMISSION；Z_ERR_IO
 */
ZBASE_API int z_file_write_text(const char* path, const char* content, size_t len);

/**
 * @brief 判断文件/目录是否存在
 * @param path UTF-8 路径
 * @return 1 存在；0 不存在；负数错误码
 */
ZBASE_API int z_file_exists(const char* path);

/**
 * @brief 递归创建目录
 * @param path UTF-8 路径
 * @return Z_OK 成功（含已存在）；Z_ERR_INVALID_ARG；Z_ERR_PERMISSION；Z_ERR_IO
 */
ZBASE_API int z_file_mkdir(const char* path);

/**
 * @brief 释放库内申请的文件缓冲区（nullptr 安全）
 * @param p 由 z_file_read_text 申请的缓冲区
 */
ZBASE_API void z_file_free(void* p);

/// 目录遍历条目（POD，详见 architecture.md §13.2）
typedef struct {
    const char* name;   ///< 文件名（UTF-8，库内申请，iter 销毁时释放）
    int          is_dir; ///< 是否目录
    uint64_t     size;   ///< 字节数
    uint64_t     mtime;  ///< 毫秒时间戳
    void* reserved[8];   ///< 预留字段，调用方必须置零
} z_dir_iter_entry_t;

/// 不透明迭代器句柄
typedef struct z_dir_iter z_dir_iter;

/**
 * @brief 创建目录迭代器
 * @param path 目录路径（UTF-8）
 * @param out_iter 输出迭代器
 * @return Z_OK 成功；Z_ERR_INVALID_ARG；Z_ERR_NOT_FOUND；Z_ERR_PERMISSION
 */
ZBASE_API int z_dir_iter_create(const char* path, z_dir_iter** out_iter);

/**
 * @brief 获取下一条目
 * @param iter 迭代器
 * @param out_entry 输出条目（迭代器内部所有，next 调用时上一条目失效）
 * @return Z_OK 取到条目；Z_ERR_NOT_FOUND 遍历结束；其他错误码
 * @note 调用前 reserved 字段必须全部置零（架构 §13.2）
 */
ZBASE_API int z_dir_iter_next(z_dir_iter* iter, z_dir_iter_entry_t* out_entry);

/**
 * @brief 销毁迭代器（释放内部资源，iter 置 NULL 由调用方负责）
 */
ZBASE_API void z_dir_iter_destroy(z_dir_iter* iter);

#ifdef __cplusplus
}
#endif

#endif  // ZBASE_FILE_H_
