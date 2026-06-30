/**
 * @file config.h
 * @author 朱鹏 (Radica Zhu)
 * @date 2026-06-30
 * @version 0.2.0
 * @brief 配置文件解析 C ABI（INI 子集）
 * @details 支持 INI 风格配置文件：
 *          - `#` 或 `;` 开头为注释（行首空白被 trim）
 *          - `[section]` 声明节
 *          - `key = value` 键值对（前后空白自动 trim）
 *          - 文件首部 UTF-8 BOM（EF BB BF）会被跳过
 *          - key 区分大小写（INI 通用约定）
 *          - 不支持多行值、不支持转义（YAGNI）
 *          - 任何 [section] 之前的键值对归属空 section（""）
 *
 *          返回的字符串指针所有权：
 *          - z_config_get 返回的指针指向内部内存，调用方不可 free
 *          - 指针在 z_config_free 后失效
 *          - default_val 非 NULL 时，找不到 key 返回 default_val 本身
 */

#ifndef ZBASE_CONFIG_H_
#define ZBASE_CONFIG_H_

#include "zbase/zbase.h"

#ifdef __cplusplus
extern "C" {
#endif

/// 配置不透明句柄（内部为 std::map 的包装）
typedef struct z_config_t z_config_t;

/**
 * @brief 从文件加载配置（INI 格式）
 * @param path 配置文件路径（UTF-8）
 * @param out_config 输出配置句柄（库内申请，调 z_config_free 释放）
 * @return Z_OK 成功；Z_ERR_INVALID_ARG 参数非法；Z_ERR_NOT_FOUND 文件不存在；
 *         Z_ERR_PERMISSION；Z_ERR_IO；Z_ERR_NOMEM
 * @note 文件以二进制模式读取，UTF-8 直接处理；首部 BOM 会被跳过
 */
ZBASE_API int z_config_load(const char* path, z_config_t** out_config);

/**
 * @brief 释放配置句柄（nullptr 安全）
 * @param config 由 z_config_load 返回的句柄
 */
ZBASE_API void z_config_free(z_config_t* config);

/**
 * @brief 读取字符串配置项
 * @param config 配置句柄
 * @param section 节名（NULL 或空串表示全局节）
 * @param key 键名
 * @param default_val 默认值（找不到时返回；可 NULL，返回 NULL）
 * @return 找到则返回内部字符串指针（不可 free）；找不到则返回 default_val
 * @note config 为 NULL 时直接返回 default_val
 */
ZBASE_API const char* z_config_get(z_config_t* config, const char* section,
                                   const char* key, const char* default_val);

/**
 * @brief 读取整数配置项
 * @param config 配置句柄
 * @param section 节名
 * @param key 键名
 * @param default_val 默认值（找不到或解析失败时返回）
 * @return 解析后的整数；找不到或非法则返回 default_val
 */
ZBASE_API int z_config_get_int(z_config_t* config, const char* section,
                               const char* key, int default_val);

/**
 * @brief 读取布尔配置项
 * @param config 配置句柄
 * @param section 节名
 * @param key 键名
 * @param default_val 默认值（找不到或解析失败时返回，0 或 1）
 * @return 1 表示真；0 表示假；找不到或非法则返回 default_val
 * @note 识别字符串（大小写不敏感）：true/1/yes/on → 1；false/0/no/off → 0
 */
ZBASE_API int z_config_get_bool(z_config_t* config, const char* section,
                                const char* key, int default_val);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // ZBASE_CONFIG_H_
