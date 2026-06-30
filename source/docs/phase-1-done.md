<!--
@file phase-1-working-file.md
@author 朱鹏 (Radica Zhu)
@date 2026-06-29
@version 1.0
@brief ZBase 第一期（v0.1）工作文件，记录本期范围、注意事项、风险与下一步。
@details 本文档是"活"文档，随 v0.1 进展持续更新。完成后归档，转入 v0.2 工作文件。
-->

# ZBase 第一期工作文件（v0.1）

> 本期目标：从零搭出最小可用版动态库 —— CMake 跨平台构建 + 6 个核心模块 + C ABI + C++ 包装 + 单元测试 + 示例。
> 整体架构参见 [architecture.md](file:///c:/Repos/ZBase/source/docs/architecture.md)。

**状态**：📋 规划中（未开始编码）
**更新日期**：2026-06-29

---

## 1. 本期范围

### 1.1 必须交付（P0）

| # | 模块 | 交付内容 | 完成标志 |
|---|------|---------|---------|
| 1 | 构建 | CMake 3.20+ 跨平台构建骨架 | MSVC / MinGW / GCC / Clang 四套工具链都能 configure + build |
| 2 | 平台层 | 文件、时间、休眠、UTF-8↔UTF-16 转换的最小封装 | 单元测试覆盖；不对外暴露 |
| 3 | error | 错误码枚举 + `z_error_msg()` | 9 个错误码 + 中文描述 |
| 4 | string | split / join / replace / trim / 大小写转换 / UTF-8 长度 | 8+ 单元测试用例 |
| 5 | time | 时间戳获取 / 格式化（ISO 8601）/ 跨平台休眠 | 单元测试覆盖 |
| 6 | file | 读文本 / 写文本 / exists / mkdir / 简单目录遍历 | 单元测试覆盖含中文路径 |
| 7 | perf | 高精度计时器 / scope 计时 / 耗时汇总 dump | 单元测试 + 示例 |
| 8 | log | 分级日志（DEBUG/INFO/WARN/ERROR）/ stderr 输出 / 控制台 UTF-8 设置 | 单元测试 + 示例 |
| 9 | C ABI | 上述模块全部导出 C 接口 | `nm` / `dumpbin` 验证符号可见 |
| 10 | C++ 包装 | 上述模块的 header-only RAII 包装 | 示例可编译运行 |
| 11 | 测试 | GoogleTest 1.17.0 集成 + 各模块单测 | `ctest` 全绿 |
| 12 | 示例 | 一个综合 demo（读文件 → 字符串处理 → perf 计时 → log 输出） | 四平台都能跑 |

### 1.2 显式不做（P1+，留给 v0.2 及以后）

- 日志文件输出、滚动、异步刷盘
- 字符串正则、模板渲染
- 时间字符串解析（`strptime` 等价物）
- 配置文件解析
- 多语言绑定（Python/Rust/Go）
- CI 自动化
- 性能基准测试套件

---

## 2. 实现顺序（自下而上）

按依赖图逐层落地，每层做完单测再上。

```
Step 0  CMake 骨架       ── 验证构建链路通（仅一个空 main）
Step 1  平台层 (encoding) ── UTF-8↔UTF-16 转换先做，后续模块都要用
Step 2  error 模块        ── 错误码 + 描述（最底层）
Step 3  string 模块      ── 依赖 error
Step 4  平台层 (file/time) ── 文件句柄、时间原语
Step 5  time 模块        ── 依赖 error + 平台层时间
Step 6  file 模块        ── 依赖 platform + string + error
Step 7  perf 模块        ── 依赖 time + error
Step 8  log 模块         ── 依赖 time + error
Step 9  C ABI 导出       ── 把上述模块包成 C 接口
Step 10 C++ 包装层       ── header-only，调 C ABI
Step 11 集成测试 + 示例  ── 端到端跑通
Step 12 四平台烟测       ── Win MSVC / Win MinGW / Linux GCC / macOS Clang
```

每一步完成的标准：**单测全绿 + 至少一个调用方在用**。

---

## 3. 模块详细范围

### 3.1 error

```c
typedef enum {
    Z_OK = 0,
    Z_ERR_INVALID_ARG = -1,
    Z_ERR_NOT_FOUND = -2,
    Z_ERR_PERMISSION = -3,
    Z_ERR_IO = -4,
    Z_ERR_ENCODING = -5,
    Z_ERR_NOMEM = -6,
    Z_ERR_OVERFLOW = -7,
    Z_ERR_UNSUPPORTED = -8,
    Z_ERR_UNKNOWN = -99,
} z_error_t;

const char* z_error_msg(int err);
```

- 描述表用 `static const char* const kErrTable[]`，编译期常量。
- 不在 v0.1 做 `errno` 桥接（v0.2 再加）。

### 3.2 string

最小集：
- `z_string_split(input, sep, &out_arr, &out_count)` → 库申请，调 `z_string_free_array` 释放
- `z_string_join(arr, count, sep, &out)` → 库申请
- `z_string_replace(input, from, to, &out)` → 库申请
- `z_string_trim(input, &out)` → 库申请
- `z_string_tolower(input, &out)` / `z_string_toupper(input, &out)`
- `z_string_utf8_len(input)` → 返回字符数（不是字节数）
- `z_string_free(s)` / `z_string_free_array(arr, count)`

### 3.3 time

- `z_time_now_ms()` → `int64_t` 毫秒时间戳
- `z_time_now_us()` → `int64_t` 微秒时间戳
- `z_time_format_iso(int64_t ms, char* buf, size_t buf_size)` → "2026-06-29T12:34:56.789"
- `z_time_sleep_ms(uint32_t ms)` / `z_time_sleep_us(uint64_t us)`

### 3.4 file

- `z_file_read_text(path, &buf, &len)` → 库申请 buf，调 `z_file_free`
- `z_file_write_text(path, content, len)` → 原子写（写临时文件 + rename）
- `z_file_exists(path)` → 1/0
- `z_file_mkdir(path)` → 递归创建
- `z_file_free(p)`
- `z_dir_iter_create(path, &iter)` / `z_dir_iter_next(iter, &entry)` / `z_dir_iter_destroy(iter)`
- entry 结构含 name / is_dir / size

### 3.5 perf

- `z_perf_start(name)` / `z_perf_end(name)` → 命名打点
- `z_perf_dump()` → 输出到 stderr，按耗时降序
- `Z_PERF_SCOPE(name)` 宏 → RAII，构造时 start，析构时 end
- 内部用 `std::unordered_map<string, accumulator>`，加锁保证线程安全

### 3.6 log（最小版）

- `z_log_init(level)` → 仅设置最低输出级别，stderr 输出
- `z_log_shutdown()`
- `Z_LOG_DEBUG(fmt, ...)` / `Z_LOG_INFO` / `Z_LOG_WARN` / `Z_LOG_ERROR`
- Windows 启动时调 `SetConsoleOutputCP(CP_UTF8)`
- 不做文件输出、不做异步、不做颜色（v0.2 再加）

### 3.7 C ABI 导出

每个模块单独一个 `*_c.cpp` 文件，统一模式：

```cpp
// src/c_api/file_c.cpp
#include "zbase/file.h"
#include "core/file/file_reader.h"  // 内部 C++ 实现

ZBASE_API int z_file_read_text(const char* path, char** out_buf, size_t* out_len) {
    if (path == nullptr || out_buf == nullptr) return Z_ERR_INVALID_ARG;
    try {
        return zbase::core::file::ReadText(path, out_buf, out_len);
    } catch (const std::bad_alloc&) {
        return Z_ERR_NOMEM;
    } catch (...) {
        return Z_ERR_UNKNOWN;
    }
}
```

### 3.8 C++ 包装层

header-only，放在 `include/zbase++/`：

```cpp
// include/zbase++/file.hpp
#pragma once
#include <string>
#include <string_view>
#include <stdexcept>
#include "zbase/file.h"

namespace zbase {

class FileReader {
 public:
  explicit FileReader(std::string_view path) {
    int err = z_file_read_text(path.data(), &buf_, &len_);
    if (err != Z_OK) throw std::runtime_error(z_error_msg(err));
  }
  ~FileReader() { if (buf_) z_file_free(buf_); }
  FileReader(const FileReader&) = delete;
  FileReader& operator=(const FileReader&) = delete;
  FileReader(FileReader&& o) noexcept : buf_(o.buf_), len_(o.len_) { o.buf_ = nullptr; }
  FileReader& operator=(FileReader&& o) noexcept {
    if (this != &o) { if (buf_) z_file_free(buf_); buf_ = o.buf_; len_ = o.len_; o.buf_ = nullptr; }
    return *this;
  }
  [[nodiscard]] std::string_view View() const noexcept { return {buf_, len_}; }
  [[nodiscard]] std::string Str() const { return {buf_, len_}; }
 private:
  char* buf_ = nullptr;
  size_t len_ = 0;
};

}  // namespace zbase
```

---

## 4. 注意事项（坑位清单）

### 4.1 CMake 源码查找

项目规则强制：**禁止硬编码源文件列表**，必须用 `file(GLOB_RECURSE)`。

```cmake
file(GLOB_RECURSE ZBASE_SOURCES
    CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/core/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/platform/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/c_api/*.cpp"
)
```

注意 `CONFIGURE_DEPENDS` —— 让 CMake 在源文件新增/删除时自动重跑配置。

### 4.2 字符编码

| 编译器 | 必加选项 |
|--------|---------|
| MSVC | `/utf-8`（同时设源码和执行字符集） |
| GCC/MinGW | `-finput-charset=UTF-8 -fexec-charset=UTF-8` |
| Clang | 默认 UTF-8，无需额外选项 |

源文件统一以 **UTF-8 无 BOM** 保存（BOM 会让某些 GCC 版本报错）。

### 4.3 Windows 路径含中文

- 路径参数是 UTF-8 `const char*`
- Windows 内部转 UTF-16 后调 `CreateFileW` / `GetFileAttributesW` / `FindFirstFileW` 等 W 版 API
- 绝不调 A 版 API（系统码页依赖，中文路径会失败）

### 4.4 MSVC `/permissive-` 与 `/W4`

- 启用 `/permissive-` 严格标准 conformant 模式
- 启用 `/W4` 高警告级别
- 暴露的问题要正面解决，**禁止 `#pragma warning(disable:xxx)` 一刀切**（除非有明确理由且注释说明）

### 4.5 MinGW 兼容性

- MinGW-w64 GCC 10+ 支持 C++20
- 部分老版本 MinGW 缺 `<format>` 头，本期不用 `std::format`，用 `snprintf` / `fmt::format`（如需）
- 测试用静态链接 gtest，避免 MinGW 动态库导出符号的坑

### 4.6 内存所有权

- 库申请 → 必须库释放（`z_file_free` / `z_string_free` 等）
- 禁止调用方 `free()` 库返回的指针（MSVC debug heap、跨 CRT 都有坑）
- 句柄型接口用 create/destroy 对，destroy 内部做 `delete`

### 4.7 异常安全

- C ABI 边界 `try/catch(...)` 全包，转错误码
- 任何 C ABI 函数都不允许异常逃逸（C 调用方未定义行为）
- C++ 包装层可以选择重抛（`zbase::Exception`）或返回错误码（双接口）

### 4.8 全局状态

- 不写带全局构造函数的静态对象（动态库加载顺序问题）
- 所有需要初始化的子系统走显式 `z_*_init()` / `z_*_shutdown()` 模式
- 全局可变状态用 `std::mutex` 保护，或文档明确"非线程安全，需调用方加锁"

### 4.9 GoogleTest 集成

```cmake
# 顶层 CMakeLists.txt
if(BUILD_TESTING)
    enable_testing()
    add_subdirectory(thirdparty/googletest-1.17.0)
endif()

# tests/CMakeLists.txt
file(GLOB_RECURSE ZBASE_TEST_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
add_executable(zbase_tests ${ZBASE_TEST_SOURCES})
target_link_libraries(zbase_tests PRIVATE zbase gtest_main)
include(GoogleTest)
gtest_discover_tests(zbase_tests)
```

### 4.10 文件头与注释规范

每个 `.h` / `.cpp` / `.hpp` 文件顶部必须有 doxygen 文件说明：

```cpp
/**
 * @file file.h
 * @author 朱鹏 (Radica Zhu)
 * @date 2026-06-29
 * @version 0.1.0
 * @brief 文件操作 C ABI 接口
 * @details 提供跨平台的文本文件读写、目录遍历等能力。
 */
```

每个公开函数有 doxygen 中文注释（参数、返回值、异常）。
成员变量加 `///<` 行内注释。

### 4.11 跨平台路径分隔符

- 内部一律用 `/`（POSIX 风格），Windows API 自动接受
- `z_file_mkdir` 同时接受 `/` 和 `\`
- 输出路径统一用 `/`

### 4.12 时间精度

- Windows：`QueryPerformanceCounter`（纳秒级）
- Linux/macOS：`clock_gettime(CLOCK_MONOTONIC)`（纳秒级）
- 不要用 `time()` / `clock()`（精度不够）

### 4.13 异常边界统一包裹

所有 C ABI 函数必须用 `ZBASE_C_API_BEGIN` / `ZBASE_C_API_END` 宏包裹，自动 try/catch 转错误码，避免漏 catch：

```cpp
// include/zbase/internal/c_api_guard.hpp（内部头，不对外发布）
#define ZBASE_C_API_BEGIN try {
#define ZBASE_C_API_END(default_err)                              \
    } catch (const std::bad_alloc&) { return Z_ERR_NOMEM;       } \
      catch (const std::exception&)  { return (default_err);    } \
      catch (...)                    { return (default_err);    }
```

用法：
```cpp
ZBASE_API int z_file_read_text(const char* path, char** out_buf, size_t* out_len) {
    ZBASE_C_API_BEGIN
    if (path == nullptr || out_buf == nullptr) return Z_ERR_INVALID_ARG;
    return zbase::core::file::ReadText(path, out_buf, out_len);
    ZBASE_C_API_END(Z_ERR_UNKNOWN)
}
```

### 4.14 CRT 链接约定

详见 [architecture.md §12.4](file:///c:/Repos/ZBase/source/docs/architecture.md#124-crt-链接约定windows)。
- 动态库构建（默认）：CMake 强制 `CMAKE_MSVC_RUNTIME_LIBRARY=MultiThreadedDLL`
- 静态库构建（`-DBUILD_SHARED_LIBS=OFF`）：强制 `MultiThreaded`
- 调用方必须匹配，文档明确写出

### 4.15 公共头禁用类型清单

详见 [architecture.md §13.3](file:///c:/Repos/ZBase/source/docs/architecture.md#133-公共头类型禁用清单)。CI 加一步扫描：
```bash
# scripts/check_public_headers.sh
grep -rEn '\bwchar_t\b|\blong\b|unsigned long|thread_local' include/zbase/ include/zbase++/
# 命中即失败
```

### 4.16 install 规则

详见 [architecture.md §14.2](file:///c:/Repos/ZBase/source/docs/architecture.md#142-install-规则)。v0.1 即写最小 install（headers + lib + targets），即使自己用不到。避免未来 v0.3 想用 `find_package(ZBase)` 时回填。

### 4.17 RPATH / soname / install_name

详见 [architecture.md §14.3](file:///c:/Repos/ZBase/source/docs/architecture.md#143-rpath--soname--install_name)。CMake 配置：
```cmake
set(CMAKE_INSTALL_RPATH "$ORIGIN")           # Linux
set(CMAKE_INSTALL_NAME_DIR "@rpath")         # macOS
set_target_properties(zbase PROPERTIES
    VERSION 0.1.0     # 完整版本号
    SOVERSION 0       # ABI 版本号
)
```

### 4.18 静态库 / 动态库双模式

详见 [architecture.md §14.1](file:///c:/Repos/ZBase/source/docs/architecture.md#141-静态库--动态库双模式)。通过 `BUILD_SHARED_LIBS` 切换。`ZBASE_API` 宏在静态模式下展开为空（通过 `ZBASE_STATIC_DEFINE` 控制）。

### 4.19 POD 结构体 reserved[8]

详见 [architecture.md §13.2](file:///c:/Repos/ZBase/source/docs/architecture.md#132-pod-结构体-reserved8-策略)。所有公共 POD 结构体末尾必须有 `void* reserved[8]`。
- 调用方传入时 `reserved` 必须置零
- 库校验非零返回 `Z_ERR_INVALID_ARG`
- 新增字段追加在 `reserved` 之前

### 4.20 `_v2` 函数命名约定

详见 [architecture.md §13.4](file:///c:/Repos/ZBase/source/docs/architecture.md#134-_v2-函数命名约定)。
- v0.1 写代码时就要意识到：**签名一旦发布永不修改**
- 设计签名时想三遍："参数够不够？返回类型合不合适？"
- 未来扩展走 `_v2` 后缀，老函数保留并 `ZBASE_DEPRECATED`

### 4.21 控制台编码 save/restore

`z_log_init` 中调 `SetConsoleOutputCP(CP_UTF8)` 时，**先保存原值**；`z_log_shutdown` 时恢复。避免库卸载后影响调用方控制台：
```cpp
#ifdef _WIN32
static UINT g_orig_console_cp = 0;
static UINT g_orig_output_cp = 0;

void init_console_utf8() {
    g_orig_console_cp = GetConsoleCP();
    g_orig_output_cp = GetConsoleOutputCP();
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);
}
void restore_console_cp() {
    if (g_orig_console_cp)     SetConsoleCP(g_orig_console_cp);
    if (g_orig_output_cp)      SetConsoleOutputCP(g_orig_output_cp);
}
#endif
```

### 4.22 CRLF / LF 统一二进制模式

所有文件读写一律用**二进制模式**（`"wb"` / `"rb"` / `CreateFile` + `GENERIC_READ`），不做换行符转换。
- 文档明确说明："zbase 的 text 模式不转换换行符，原样保留"
- 如需转换，提供显式函数 `z_string_crlf_to_lf` / `z_string_lf_to_crlf`，不在文件读写里隐式做

### 4.23 Windows MAX_PATH 处理

`z_file_*` 在 Windows 上对长度 > 248 的路径自动加 `\\?\` 前缀并转 UTF-16：
```cpp
std::wstring path_w = utf8_to_utf16(path);
if (path_w.length() > 248 && path_w.substr(0, 4) != L"\\\\?\\") {
    path_w = L"\\\\?\\" + path_w;  // 注意：\\?\ 前缀要求路径用反斜杠
}
// 用 path_w 调 CreateFileW 等
```

### 4.24 `*_free` 函数接受 nullptr 安全

所有 `z_*_free` 函数必须处理 `nullptr` 入参，安全返回 no-op：
```c
ZBASE_API void z_file_free(void* p) {
    if (p == nullptr) return;   // 安全 no-op
    free(p);                    // 或库内分配器
}
```

调用方释放后习惯性再调一次不会崩。

### 4.25 符号导出 CI 检查

详见 [architecture.md §12.2](file:///c:/Repos/ZBase/source/docs/architecture.md#122-默认隐藏显式导出)。CI 加脚本对比实际导出符号与白名单：
```bash
# scripts/check_exported_symbols.sh
nm -D --defined-only libzbase.so | awk '$2=="T"{print $3}' | sort > /tmp/actual.txt
diff /tmp/actual.txt cmake/exported_symbols.txt
```

### 4.26 DLL 劫持防范

详见 [architecture.md §12.3](file:///c:/Repos/ZBase/source/docs/architecture.md#123-dll-劫持防范windows)。`z_log_init` 中调一次 `SetDefaultDllDirectories(LOAD_LIBRARY_SEARCH_APPLICATION_DIR | LOAD_LIBRARY_SEARCH_SYSTEM32)`，限制后续 DLL 搜索路径。

### 4.27 enum + static_assert

详见 [architecture.md §9](file:///c:/Repos/ZBase/source/docs/architecture.md#9-错误码体系)。`z_error_t` 用 `typedef enum` + `static_assert(sizeof(z_error_t)==sizeof(int))`（C++）或 `_Static_assert`（C11）。

### 4.28 YAGNI 守门规则

详见 [architecture.md §16 #8](file:///c:/Repos/ZBase/source/docs/architecture.md#16-设计原则)。v0.1 新增 C ABI 函数数量上限 **30 个**。每加一个函数前自问三遍："我现在就有这个需求吗？"

### 4.29 thread_local 限制

详见 [architecture.md §13.3](file:///c:/Repos/ZBase/source/docs/architecture.md#133-公共头类型禁用清单)。
- v0.1 完全不用 `thread_local`
- 如必须用，用 `TlsAlloc`（Windows）/ `pthread_key_create`（POSIX）自封装

---

## 5. 风险与对策

| 风险 | 影响 | 严重度 | 对策 |
|------|------|--------|------|
| CRT 静态/动态混用（库 `/MD` 调用方 `/MT`） | 崩溃 | 致命 | CMake 强制 `CMAKE_MSVC_RUNTIME_LIBRARY`；文档明确；详见 §4.14 |
| 跨堆释放 | 崩溃 | 致命 | 严格库申请库释放；`*_free` 接受 nullptr；详见 §4.6 §4.24 |
| 异常逃逸 C ABI 边界 | UB / 崩溃 | 致命 | `ZBASE_C_API_BEGIN/END` 宏强制包裹；详见 §4.13 |
| DLL 劫持 | 安全漏洞 | 致命 | `SetDefaultDllDirectories` 限制搜索；详见 §4.26 |
| 公共结构体加字段破坏 ABI | 用户编译失败 | 致命 | POD + `reserved[8]`；详见 §4.19 |
| MinGW 某些版本 C++20 标准库不全 | 编译失败 | 高 | 提前在一个最小工程验证；必要时降级到 C++17 个别特性 |
| 公共头使用 `wchar_t`/`long`/`thread_local` | 跨平台行为不一致 | 高 | CI 扫描脚本检查；详见 §4.15 |
| 符号意外导出（内部函数泄漏） | 二进制体积大、被外部依赖 | 中 | `-fvisibility=hidden` + CI 符号白名单；详见 §4.25 |
| Windows 控制台默认非 UTF-8 | 中文日志乱码 | 中 | `z_log_init` save/restore `SetConsoleOutputCP`；详见 §4.21 |
| MSVC `/permissive-` 暴露历史代码问题 | 编译失败 | 中 | 本期从零写，遵循 Google Style，问题会少 |
| `file(GLOB_RECURSE)` 不自动重配置 | 新增源文件不进构建 | 中 | 加 `CONFIGURE_DEPENDS` |
| Windows MAX_PATH = 260 限制 | 长路径文件操作失败 | 中 | 路径 > 248 加 `\\?\` 前缀；详见 §4.23 |
| CRLF/LF 隐式转换 | 跨平台文件内容不一致 | 低 | 统一二进制模式；详见 §4.22 |
| GoogleTest 编译慢 | 构建时间长 | 低 | 用 `add_subdirectory` 而非 FetchContent，源码已就位 |
| YAGNI 失守，API 膨胀 | 长期维护成本高 | 低 | v0.1 函数上限 30 个；详见 §4.28 |

---

## 6. 下一步（立即可执行）

按顺序：

1. **写实现计划（writing-plans）** ── 把本期拆成可独立执行的任务，每个任务有验收标准
2. **搭 CMake 骨架** ── Step 0，验证四套工具链都能 configure + build 一个空 main
3. **写平台层 encoding 模块** ── UTF-8↔UTF-16 转换 + 单测
4. **按 Step 1→10 顺序逐模块实现**
5. **每完成一模块**：单测全绿 → 更新本文档进度表 → 进入下一模块
6. **Step 11 端到端示例跑通**后，进入 Step 12 四平台烟测
7. 全部完成后，本文档归档（重命名为 `phase-1-done.md`），创建 v0.2 工作文件

### 当前阻塞项

无。可以开始写实现计划（writing-plans skill，独立于 Step 0~12 之外的前置活动）。

### 待用户确认事项

无（v0.1 范围、C++ 标准、测试框架均已确认）。

---

## 7. 进度跟踪

| 步骤 | 内容 | 状态 | 备注 |
|------|------|------|------|
| Step 0 | CMake 骨架 | ⬜ 未开始 | |
| Step 1 | 平台层 encoding | ⬜ 未开始 | |
| Step 2 | error 模块 | ⬜ 未开始 | |
| Step 3 | string 模块 | ⬜ 未开始 | |
| Step 4 | 平台层 file/time | ⬜ 未开始 | |
| Step 5 | time 模块 | ⬜ 未开始 | |
| Step 6 | file 模块 | ⬜ 未开始 | |
| Step 7 | perf 模块 | ⬜ 未开始 | |
| Step 8 | log 模块 | ⬜ 未开始 | |
| Step 9 | C ABI 导出 | ⬜ 未开始 | |
| Step 10 | C++ 包装层 | ⬜ 未开始 | |
| Step 11 | 集成测试 + 示例 | ⬜ 未开始 | |
| Step 12 | 四平台烟测 | ⬜ 未开始 | |

状态图例：⬜ 未开始 / 🟦 进行中 / ✅ 完成 / ⚠️ 阻塞

---

## 8. 参考资料

- 架构总览：[architecture.md](file:///c:/Repos/ZBase/source/docs/architecture.md)
- 项目编码规则：[.trae/rules/code-rule.md](file:///c:/Repos/ZBase/.trae/rules/code-rule.md)
- 项目 README：[README.md](file:///c:/Repos/ZBase/source/README.md)
- GoogleTest 源码位置：[thirdparty/googletest-1.17.0/](file:///c:/Repos/ZBase/source/thirdparty/googletest-1.17.0/)
- Google C++ Style Guide: https://google.github.io/styleguide/cppguide.html
