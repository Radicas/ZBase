/**
 * @file dummy.cpp
 * @author 朱鹏 (Radica Zhu)
 * @date 2026-06-29
 * @version 0.1.0
 * @brief 占位空翻译单元，仅用于 Step 0 验证 CMake 链路
 * @details Task 1 起此文件可删除。
 */
namespace zbase {
namespace platform {
// 注意：非 inline 才能让 MSVC 在 .obj 中产生符号，
// 配合 WINDOWS_EXPORT_ALL_SYMBOLS 生成 zbase.lib。
// Task 1 起此文件可删除。
void dummy_link_anchor() {}
}  // namespace platform
}  // namespace zbase
