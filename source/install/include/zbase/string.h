/**
 * @file string.h
 * @author 朱鹏 (Radica Zhu)
 * @date 2026-06-29
 * @version 0.1.0
 * @brief 字符串工具 C ABI
 * @details 提供 split / join / replace / trim / tolower / toupper / free / free_array
 *          共 8 个 ZBASE_API 导出函数，外加 utf8_len 一个 static inline 函数（纯算法，
 *          无需导出，避免 YAGNI 守门超标，详见 phase-1-working-file.md §4.28）。
 *          所有返回字符串的函数都由库内申请内存，调用方必须使用 z_string_free 或
 *          z_string_free_array 释放，禁止外部 free。
 */

#ifndef ZBASE_STRING_H_
#define ZBASE_STRING_H_

#include <stddef.h>
#include "zbase/zbase.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 分隔字符串
 * @param input 输入字符串（UTF-8，不以 \0 终止也可，配合 input_len）
 * @param input_len 输入长度（字节）
 * @param sep 分隔符（不可为空串）
 * @param out_arr 输出数组（库申请，调 z_string_free_array 释放）
 * @param out_count 输出元素数量
 * @return Z_OK 成功；Z_ERR_INVALID_ARG 参数非法；Z_ERR_NOMEM 内存不足
 */
ZBASE_API int z_string_split(const char* input, size_t input_len,
                             const char* sep,
                             char*** out_arr, size_t* out_count);

/**
 * @brief 拼接字符串数组
 * @param arr 字符串数组（元素可为 NULL，按空串处理）
 * @param count 元素数量
 * @param sep 分隔符
 * @param out 输出字符串（库申请，调 z_string_free 释放）
 * @return Z_OK 成功；Z_ERR_INVALID_ARG；Z_ERR_NOMEM
 */
ZBASE_API int z_string_join(char** arr, size_t count, const char* sep, char** out);

/**
 * @brief 替换字符串中所有匹配的子串
 * @param input 输入
 * @param from 待替换子串（不可为空串）
 * @param to 替换为
 * @param out 输出（库申请）
 * @return Z_OK 成功；Z_ERR_INVALID_ARG；Z_ERR_NOMEM
 */
ZBASE_API int z_string_replace(const char* input, const char* from,
                                const char* to, char** out);

/**
 * @brief 去除首尾空白字符（空格、\t、\n、\r、\v、\f）
 * @param input 输入
 * @param out 输出（库申请）
 * @return Z_OK 成功；Z_ERR_INVALID_ARG；Z_ERR_NOMEM
 */
ZBASE_API int z_string_trim(const char* input, char** out);

/**
 * @brief 转小写（仅对 ASCII 字母生效，UTF-8 多字节字符不变）
 * @param input 输入
 * @param out 输出（库申请）
 * @return Z_OK 成功；Z_ERR_INVALID_ARG；Z_ERR_NOMEM
 */
ZBASE_API int z_string_tolower(const char* input, char** out);

/**
 * @brief 转大写（仅对 ASCII 字母生效，UTF-8 多字节字符不变）
 * @param input 输入
 * @param out 输出（库申请）
 * @return Z_OK 成功；Z_ERR_INVALID_ARG；Z_ERR_NOMEM
 */
ZBASE_API int z_string_toupper(const char* input, char** out);

/**
 * @brief 释放库内申请的字符串
 * @param s 由 z_string_* 申请的字符串指针，可为 NULL
 */
ZBASE_API void z_string_free(char* s);

/**
 * @brief 释放库内申请的字符串数组
 * @param arr 数组指针，可为 NULL
 * @param count 元素数量
 */
ZBASE_API void z_string_free_array(char** arr, size_t count);

#ifdef __cplusplus
}  // extern "C"
#endif

/**
 * @brief 计算 UTF-8 字符串的字符数（不是字节数）
 * @param input 输入（必须以 \0 终止）
 * @return 字符数；input 为 NULL 时返回 0
 * @note 非法字节按 1 字符宽容处理；纯算法，作为 static inline 不占导出符号
 */
static inline size_t z_string_utf8_len(const char* input) {
    if (input == NULL) return 0;
    size_t count = 0;
    for (size_t i = 0; input[i] != '\0'; ) {
        unsigned char c = (unsigned char)input[i];
        if (c < 0x80) i += 1;
        else if ((c & 0xE0) == 0xC0) i += 2;
        else if ((c & 0xF0) == 0xE0) i += 3;
        else if ((c & 0xF8) == 0xF0) i += 4;
        else { ++i; }  /* 非法字节，按 1 字符算（v0.1 宽容处理） */
        ++count;
    }
    return count;
}

#endif  // ZBASE_STRING_H_
