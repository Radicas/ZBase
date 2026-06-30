/**
 * @file zbase.h
 * @author 朱鹏 (Radica Zhu)
 * @date 2026-06-29
 * @version 0.2.0
 * @brief ZBase 公共头：版本宏 + 符号导出宏
 * @details 所有公开头文件应通过 #include "zbase/zbase.h" 引入。
 *          宏定义放在 extern "C" 之外，因为它们是预处理指令，不参与链接。
 */

#ifndef ZBASE_ZBASE_H_
#define ZBASE_ZBASE_H_

/* 版本号宏 */
#define ZBASE_VERSION_MAJOR 0
#define ZBASE_VERSION_MINOR 2
#define ZBASE_VERSION_PATCH 0
#define ZBASE_VERSION_STRING "0.2.0"

/* 符号导出宏（详见 architecture.md §12.1） */
#if defined(ZBASE_STATIC_DEFINE)
  #define ZBASE_API
  #define ZBASE_LOCAL
#elif defined(_WIN32)
  #if defined(ZBASE_EXPORTS)
    #define ZBASE_API __declspec(dllexport)
  #else
    #define ZBASE_API __declspec(dllimport)
  #endif
  #define ZBASE_LOCAL
#elif defined(__GNUC__) || defined(__clang__)
  #define ZBASE_API __attribute__((visibility("default")))
  #define ZBASE_LOCAL  __attribute__((visibility("hidden")))
#else
  #define ZBASE_API
  #define ZBASE_LOCAL
#endif

/* 弃用宏（详见 architecture.md §13.5） */
#if defined(__cplusplus) && __cplusplus >= 201402L
  #define ZBASE_DEPRECATED(msg) [[deprecated(msg)]]
#elif defined(_MSC_VER)
  #define ZBASE_DEPRECATED(msg) __declspec(deprecated(msg))
#elif defined(__GNUC__) || defined(__clang__)
  #define ZBASE_DEPRECATED(msg) __attribute__((deprecated(msg)))
#else
  #define ZBASE_DEPRECATED(msg)
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 获取库版本字符串
 * @return 形如 "0.2.0" 的静态字符串
 */
ZBASE_API const char* z_version(void);

#ifdef __cplusplus
}
#endif

#endif  // ZBASE_ZBASE_H_
