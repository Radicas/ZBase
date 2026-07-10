/**
 * @file crash.h
 * @author 朱鹏 (Radica Zhu)
 * @date 2026-07-10
 * @version 0.2.0
 * @brief 崩溃堆栈捕获 C ABI
 * @details 安装全局崩溃处理器，在进程异常终止时自动将堆栈回溯信息写入文件。
 *          支持文本和 JSON 两种输出格式。
 *          符号解析精度取决于调试信息嵌入情况：
 *          - 嵌入调试信息（MSVC /Z7, GCC -g）：完整函数名 + 源文件 + 行号
 *          - 无调试信息：回退到模块名 + 偏移地址
 *          详见 AI/ARCHITECTURE.md 新增 crash 模块说明。
 */

#ifndef ZBASE_CRASH_H_
#define ZBASE_CRASH_H_

#include "zbase/zbase.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 崩溃处理器配置
 */
typedef struct z_crash_config_t {
    const char* output_dir;   ///< 崩溃文件输出目录（NULL 表示可执行文件所在目录）
    const char* app_name;     ///< 应用程序名称，用于文件命名（NULL 默认为 "zbase_app"）
} z_crash_config_t;

/**
 * @brief 安装全局崩溃处理器
 * @param config 配置参数（可为 NULL，使用默认值）
 * @return Z_OK 成功；Z_ERR_CRASH_ALREADY_INSTALLED 已安装；Z_ERR_CRASH_INSTALL_FAILED 安装失败
 * @note 安装后自动捕获 SIGSEGV/SIGABRT/SIGFPE/SIGILL/SIGBUS (POSIX)
 *       和未处理异常 (Windows SEH)。
 *       崩溃时在工作目录生成 <app>_crash_<timestamp>.txt 和 <app>_crash_<timestamp>.json。
 *       可重复调用：再次调用前会自动卸载旧处理器。
 */
ZBASE_API int z_crash_install(const z_crash_config_t* config);

/**
 * @brief 卸载崩溃处理器，恢复系统默认行为
 * @note 调用后崩溃不再被捕获。线程安全，可重复调用。
 */
ZBASE_API void z_crash_uninstall(void);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // ZBASE_CRASH_H_
