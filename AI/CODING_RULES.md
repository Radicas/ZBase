# CODING_RULES

> 从既有代码与 [.trae/rules/code-rule.md](file:///c:/Repos/ZBase/.trae/rules/code-rule.md) 总结的编码风格。不凭空制定新规范。

---

## 强制规则（工作区硬约束）

1. 编码风格符合 [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html)。
2. **禁止相对路径引入头文件**：禁止 `#include "../models/xxx.h"`，必须用 `#include "core/models/xxx.h"` 风格的绝对路径。
3. C++ 使用 17/20/23 最新版本语法（项目定为 C++20）。
4. 源码必须支持 macOS、Windows、Linux 三平台。
5. 源码必须支持 MinGW 和 MSVC 两种编译器工具集。
6. 文件名统一小写，例如 `functiondata.cpp`。
7. 头文件上方加 doxygen 风格文件说明（作者、日期、版本、作用）。
8. 头文件中所有函数加中文 doxygen 注释（参数、返回值、异常）。
9. 类成员变量加注释，例如 `int a; ///< xxx作用`。
10. 源代码日志使用中文，终端打印中文不能乱码。
11. CMakeLists 禁止硬编码查找文件，必须用 `file(GLOB_RECURSE xxx "src/*.cpp")`。
12. Git 提交信息用中文，conventional commits 风格：`feat(模块): xxx` / `fix(模块): xxx` / `docs(模块): xxx` / `refactor(模块): xxx` / `test(模块): xxx` / `build(模块): xxx` / `chore(模块): xxx` / `style(模块): xxx` / `perf(模块): xxx`。

---

## 命名方式

| 元素 | 规范 | 示例 |
|------|------|------|
| 工程名 | 全小写 | `zbase` |
| 正式名 | 大驼峰（文档用） | `ZBase` |
| 文件名 | 全小写 | `file_reader.cpp`、`file_platform.hpp` |
| C ABI 函数 | `z_<module>_<verb>` | `z_file_read_text` |
| C ABI 类型 | `z_<module>_<noun>_t` | `z_log_level_t`、`z_dir_iter_entry_t` |
| 错误码 | `Z_OK` / `Z_ERR_*` | `Z_ERR_NOT_FOUND` |
| 宏 | `ZBASE_*` | `ZBASE_API`、`ZBASE_C_API_BEGIN` |
| C++ 命名空间 | `zbase::` / `zbase::core::` / `zbase::platform::` | `zbase::FileReader` |
| C++ 类 | 大驼峰 | `FileReader`、`PerfScope`、`LogFile` |
| C++ 函数 | 大驼峰 | `ReadAll`、`ThrowIfError` |
| C++ 成员变量 | 蛇形 + 尾下划线 | `buf_`、`len_`、`iter_` |
| 匿名命名空间内函数 | 大驼峰 | `DupString`、`LevelName`、`RollFiles` |
| 内部辅助函数 | 大驼峰 | `NormalizePath`、`MonoNowNs` |

---

## 文件组织

- 公共头：`include/zbase/<module>.h`（C ABI）、`include/zbase++/<module>.hpp`（C++ 包装）
- 内部头：`include/zbase/internal/*.hpp`（不发布，install 时排除 `internal/`）
- 平台头：`src/platform/*.hpp`
- 实现：`src/core/<module>/<files>.cpp`
- 测试：`tests/<core|platform|integration>/test_*.cpp`
- 基准：`benchmarks/bench_*.cpp`
- 示例：`examples/<name>/main.cpp` + `CMakeLists.txt`

每个 `.cpp` / `.h` / `.hpp` 文件顶部必须有 doxygen 文件头：

```cpp
/**
 * @file perf.cpp
 * @author 朱鹏 (Radica Zhu)
 * @date 2026-06-30
 * @version 0.1.0
 * @brief 性能统计实现
 * @details ...实现要点与设计说明...
 */
```

---

## 类设计

### C ABI 函数实现模式（强制）

所有返回 `int` 的 C ABI 函数必须用异常包裹宏：

```cpp
#include "zbase/internal/c_api_guard.hpp"

extern "C" {

int z_xxx_do_something(const char* path) {
    ZBASE_C_API_BEGIN
    if (path == nullptr) return Z_ERR_INVALID_ARG;
    // ... 业务逻辑 ...
    return Z_OK;
    ZBASE_C_API_END(Z_ERR_UNKNOWN)
}

}  // extern "C"
```

`void` 返回函数不能用 `ZBASE_C_API_END`，内部 `try/catch` 静默吞异常（见 `z_log_file_close` 实现）。

### C++ 包装类模式（RAII）

```cpp
class FileReader {
 public:
  explicit FileReader(std::string_view path);   // 构造调 C ABI，失败抛 Exception
  ~FileReader();                                 // 析构调 C ABI 释放
  FileReader(const FileReader&) = delete;        // 禁拷贝
  FileReader& operator=(const FileReader&) = delete;
  FileReader(FileReader&&) noexcept;             // 可移动
  FileReader& operator=(FileReader&&) noexcept;
  [[nodiscard]] std::string_view View() const noexcept;
 private:
  char*  buf_ = nullptr;
  size_t len_ = 0;
};
```

约定：
- 构造失败抛 `zbase::Exception`（含错误码 + 上下文）
- 析构 noexcept，调对应 `z_*_free` / `z_*_destroy`
- 删除拷贝构造与拷贝赋值
- 提供移动构造与移动赋值（noexcept）
- `[[nodiscard]]` 标注返回值的 getter

---

## 错误处理

### C ABI 层

- 函数返回 `int` 错误码（`Z_OK=0`，负数表错误）
- 错误码定义在 `include/zbase/error.h`，**只增不改不重排**
- 新增错误码追加在 `-8` 与 `-99` 之间
- 参数校验放在函数最前面，返回 `Z_ERR_INVALID_ARG`
- 内存不足返回 `Z_ERR_NOMEM`
- `static_assert(sizeof(z_error_t) == sizeof(int))` 防止 ABI 漂移

### C++ 包装层

- 失败抛 `zbase::Exception`（继承 `std::exception`，含 `code()` 与 `what()`）
- 辅助函数 `ThrowIfError(int err)` 检查并抛异常
- 调用方可选：捕获异常或使用返回错误码的重载（如 `ReadAll(std::string& out) noexcept`）

### 异常隔离

- C++ 异常**不允许**跨越 C 边界（UB），由 `ZBASE_C_API_END` 统一 catch
- `std::bad_alloc` → `Z_ERR_NOMEM`
- 其他 `std::exception` / `...` → 默认错误码（一般 `Z_ERR_UNKNOWN`）

---

## 日志方式

### 接口

- `z_log_write(level, file, line, fmt, ...)`：内部接口，建议用宏
- `Z_LOG_DEBUG` / `Z_LOG_INFO` / `Z_LOG_WARN` / `Z_LOG_ERROR`：自动填 `__FILE__` / `__LINE__`
- 4 级：`Z_LOG_LEVEL_DEBUG` / `INFO` / `WARN` / `ERROR`

### 输出格式

```
[2026-06-30T12:34:56.789] [INFO] [perf.cpp:47] 用户消息
```

### 行为

- 默认输出到 `stderr` + `fflush`
- `z_log_file_open` 后同时输出到文件（每次写后 flush，防崩溃丢日志）
- 单文件超 `max_size` 滚动：`path` → `path.1` → `path.2` → ... → `path.N`（删最旧）
- 滚动时 Windows 用 `MoveFileExW(MOVEFILE_REPLACE_EXISTING)`，POSIX 用 `rename()`
- Windows Defender 偶发锁文件，重开失败重试 3 次（每次 5ms），全失败则禁用文件输出

### 中文要求

- 日志消息内容使用中文（如 `"读取文件失败"`、`"ZBase 异常"`）
- 控制台输出前必须 `z_init_console()` 确保 Windows UTF-8 编码

---

## 内存管理

### 所有权总则

**库申请的内存必须由库的释放接口释放，禁止调用者直接 `free()`。**

原因：MSVC Debug/Release CRT 不同堆，跨堆 `free` 是 UB；MinGW 与 MSVC 混用同样问题。

### 接口模式

| 模式 | 调用方义务 | 示例 |
|------|----------|------|
| `T** out` 出参 | 用完调 `z_<module>_free(*out)` | `z_file_read_text(path, &buf, &len)` |
| 调用方预分配缓冲 | 调用方自己管 | `z_time_format_iso(ms, buf, size)` |
| 句柄型 `z_*_t*` | 用 `z_*_destroy(h)` / `z_*_free(h)` | `z_dir_iter_destroy(iter)` |
| 返回内部指针 | 不可 free，句柄销毁后失效 | `z_config_get` / `z_error_msg` |

### 内部分配

- 库内字符串分配用 `std::malloc`（C ABI 出参）或 `std::string`（内部）
- 释放对应 `std::free` 或 RAII 自动
- 测试中 `zbase_objects` 链接方式可访问内部符号，但仍应遵守所有权约定

---

## RAII 使用情况

广泛使用，是 C++ 包装层的核心模式：

- `zbase::Logger`（init/shutdown）
- `zbase::LogFile`（file_open/file_close）
- `zbase::FileReader`（read_text/free）
- `zbase::DirIterator`（create/destroy）
- `zbase::Config`（load/free）
- `std::lock_guard<std::mutex>`（perf / log 文件锁）
- `std::unique_ptr` / `std::shared_ptr`：未在已读代码中发现广泛使用（内部多用原始指针 + RAII 包装类）

---

## const 使用

- getter 返回值标注 `[[nodiscard]]` + `const` 或 `noexcept`
- 不修改对象的成员函数标 `const`（如 `View() const noexcept`、`Size() const noexcept`）
- 参数：只读指针/引用标 `const`（如 `const char* path`、`const std::string&`）
- `static inline` 纯算法函数（如 `z_string_utf8_len`）不导出符号

---

## include 风格

### 路径规范（强制）

禁止相对路径，使用基于 include 根的绝对路径：

```cpp
// 正确
#include "zbase/file.h"
#include "zbase/internal/c_api_guard.hpp"
#include "platform/file_platform.hpp"
#include "core/log/console.hpp"

// 错误（禁止）
#include "../models/xxx.h"
#include "../../include/zbase/file.h"
```

### include 顺序（实际代码观察）

1. 对应的本模块头文件（如 `perf.cpp` 先 `#include "zbase/perf.h"`）
2. 项目内其他公共头（`zbase/error.h` 等）
3. 项目内内部头（`zbase/internal/c_api_guard.hpp`、`platform/*.hpp`、`core/*.hpp`）
4. C++ 标准库（`<algorithm>`、`<cstdio>`、`<mutex>`、`<string>`、`<vector>` 等）
5. 平台头（`<Windows.h>`、`<unistd.h>` 等，按 `#ifdef` 分组）

### 头文件保护

- C 头：`#ifndef ZBASE_<MODULE>_H_` + `#define` + `#endif`
- C++ 内部头：`#ifndef ZBASE_PLATFORM_<MODULE>_HPP_` + `#define` + `#endif`
- C++ 包装头：`#pragma once`

### extern "C" 边界

- 公共 C 头用 `#ifdef __cplusplus extern "C" {` 包裹
- 实现文件中 `extern "C" { ... }` 包裹所有导出函数

---

## 注释风格

### 文件头（强制 doxygen）

```cpp
/**
 * @file perf.cpp
 * @author 朱鹏 (Radica Zhu)
 * @date 2026-06-30
 * @version 0.1.0
 * @brief 性能统计实现
 * @details v0.1 直接用 unordered_map 实现 C ABI（YAGNI ...）。
 *          线程安全（mutex 保护）。同名嵌套 start/end 用 ref_count 计数。
 */
```

### 函数注释（强制 doxygen，中文）

```cpp
/**
 * @brief 开始命名打点
 * @param name 打点名（建议 ≤ 64 字节，UTF-8）
 * @return Z_OK 成功；Z_ERR_INVALID_ARG name 为 NULL
 */
ZBASE_API int z_perf_start(const char* name);
```

### 成员变量注释

```cpp
uint64_t total_ns = 0;   ///< 累计纳秒
uint64_t count = 0;      ///< 调用次数
char*  buf_ = nullptr;   ///< 缓冲区指针（库内申请）
```

### 行内注释

- 解释"为什么"而非"做什么"
- 中文撰写
- 标注设计决策来源（如 `// 详见 architecture.md §13.2`、`// v0.1 宽容处理`）

---

## 平台条件编译

```cpp
#ifdef _WIN32
  #include <Windows.h>
  // Windows 实现（W 版 API）
#else
  #include <fcntl.h>
  #include <unistd.h>
  #include <sys/stat.h>
  // POSIX 实现
#endif
```

约定：
- 用 `_WIN32`（不是 `WIN32`）区分 Windows
- 用 `__linux__` / `__APPLE__` 区分 POSIX 子平台
- 平台差异封装在 `src/platform/` 内，不泄漏到 `src/core/`
