<!--
@file architecture.md
@author 朱鹏 (Radica Zhu)
@date 2026-06-29
@version 1.0
@brief ZBase 项目架构总览文档，描述整体分层、模块依赖、接口形态与关键技术决策。
@details 本文档面向所有阅读 ZBase 源码或参与开发的工程师，提供对项目的整体概念。
-->

# ZBase 架构总览

> 朱鹏（Radica Zhu）的个人跨平台 C/C++ 通用工具动态库 —— 一套覆盖日常开发高频场景的底层工具合集，编译为跨平台动态库，让所有项目告别重复造轮子。

---

## 1. 设计目标

| 目标 | 含义 |
|------|------|
| 跨平台 | 一套代码编译 Windows (dll) / Linux (so) / macOS (dylib) |
| ABI 稳定 | 对外暴露纯 C 接口，规避 C++ 名字改编，支持多语言 FFI |
| 零重度依赖 | 核心模块无第三方依赖；测试用 GoogleTest（已内置于 `thirdparty/`） |
| 接口只增不改 | 旧版调用者可加载新版动态库而不破坏兼容性 |
| 平台差异内部消化 | 上层调用者无需关心 Windows / Linux / macOS 差异 |
| 现代内部实现 | 核心用 C++20 实现，但绝不泄漏到对外 ABI |

---

## 2. 五层架构（蛋糕模型）

依赖向内指，无环。下层永远不知道上层存在。

```
┌─────────────────────────────────────────────────────────────────┐
│  ⑤ 公共头文件                                                    │
│     include/zbase/*.h     (C ABI 头)                            │
│     include/zbase++/*.hpp (header-only C++ 包装)                │
├─────────────────────────────────────────────────────────────────┤
│  ④ C++ 包装层  (header-only, RAII)                               │
│     zbase::FileReader / PerfScope / StringView / Logger ...      │
│     依赖 ③，提供 RAII、std::string / std::filesystem 互操作        │
├─────────────────────────────────────────────────────────────────┤
│  ③ C ABI 导出层  src/c_api/                                      │
│     extern "C" + ZBASE_API                                       │
│     z_file_* / z_string_* / z_time_* / z_perf_* / z_log_*       │
│     所有异常在边界 catch，转换为错误码；内存由库管                  │
├─────────────────────────────────────────────────────────────────┤
│  ② 核心模块层  src/core/{error,string,time,file,perf,log}/      │
│     现代C++20实现，所有符号内部可见（visibility=hidden）           │
├─────────────────────────────────────────────────────────────────┤
│  ① 平台适配层  src/platform/{windows,linux,macos}/              │
│     Win32/POSIX 原语封装，无对外 API                              │
└─────────────────────────────────────────────────────────────────┘
```

### 各层职责

- **① 平台适配层**：封装 OS 差异（文件句柄、时间原语、休眠、字符编码转换、同步原语）。通过 `#ifdef _WIN32 / __linux__ / __APPLE__` 选择编译。
- **② 核心模块层**：业务逻辑实现。每个模块一个命名空间（`zbase::core::file` 等），全部 `static`/匿名命名空间内函数，不对外暴露符号。
- **③ C ABI 导出层**：薄薄一层 C 函数包装，把核心模块的能力以稳定 C 接口暴露。所有异常在此 catch，所有内存分配在此明确归属。
- **④ C++ 包装层**：header-only，调用 C ABI，对外提供 RAII 容器、`std::string` 桥接、`std::filesystem` 互操作。要求调用方与库使用相同 C++ 标准和接近的编译选项。
- **⑤ 公共头文件**：用户 `#include` 的入口，对外承诺稳定。

---

## 3. 模块依赖图

```
              error   ←  无依赖（最底层，所有模块都可用）
                ↑
              string  ←  error
                ↑
               time   ←  error
                ↑
               file   ←  platform + string + error
                ↑
               perf   ←  time + error
                ↑
               log    ←  time + error
```

**实现顺序（自下而上）**：`error → string → time → file → perf → log`。

依赖规则：
1. 下层模块不能 `#include` 上层头文件。
2. 同层模块之间尽量避免横向依赖（如 `string` 不依赖 `time`）。
3. 平台层只被 `file`/`time` 等需要 OS 原语的模块使用；`error`/`string` 纯算法模块不依赖平台层。

---

## 4. 对外接口形态

### 4.1 两层接口

| 层 | 用户 | 形态 | ABI |
|----|------|------|-----|
| C ABI | C 程序、Python/Rust/Go FFI、C++ 程序 | `extern "C"` 函数，`z_*` 前缀 | 稳定，跨编译器 |
| C++ 包装 | 仅 C++ 程序 | header-only `zbase::*` 类 | 不保证跨编译器 |

C++ 包装层是 C ABI 之上的"糖衣"，不引入新功能。C++ 用户也可以直接用 C ABI。

### 4.2 C ABI 风格示例

```c
/* 错误码返回 + 出参指针 */
int  z_file_read_text(const char* path, char** out_buf, size_t* out_len);

/* 库申请的内存，库负责释放 */
void z_string_free(char* s);

/* 无状态查询，直接返回值 */
int  z_file_exists(const char* path);
```

### 4.3 C++ 包装风格示例

```cpp
namespace zbase {

class FileReader {
 public:
  explicit FileReader(std::string_view path);
  ~FileReader();  // 自动 z_file_free
  FileReader(const FileReader&) = delete;
  FileReader& operator=(const FileReader&) = delete;
  FileReader(FileReader&&) noexcept;
  FileReader& operator=(FileReader&&) noexcept;

  [[nodiscard]] std::string ReadAll();  // 抛 zbase::Exception
  [[nodiscard]] int ReadAll(std::string& out) noexcept;  // 返回错误码

 private:
  char* buf_ = nullptr;
  size_t len_ = 0;
};

}  // namespace zbase
```

---

## 5. 关键技术决策

| 项 | 决策 | 原因 |
|----|------|------|
| C++ 标准 | **C++20** | MSVC 2019 16.11+ / MinGW GCC 10+ / Clang 10+ 都稳定支持；不上 C++23 是为 MinGW 兼容性 |
| 构建系统 | CMake 3.20+ + Ninja | 跨平台、生成器中立；Ninja 比 Make 快得多 |
| 源码查找 | `file(GLOB_RECURSE)` | 项目规则强制；不硬编码源文件列表 |
| 字符编码 | 对外统一 UTF-8 | 一致性 + 跨平台；Windows 内部转 UTF-16 调 W 版 API |
| 内存所有权 | 库申请库释放 | 跨堆安全（MSVC debug heap、不同 CRT 不能互释放） |
| 错误码 | `int` 返回，`Z_OK=0`，负数表错误 | 简单、可扩展、跨语言友好 |
| 符号导出 | `ZBASE_API` 宏 | 跨平台统一（dllexport / dllimport / visibility default） |
| 异常处理 | C ABI 边界全部 catch | 异常不允许跨越 C 边界（UB） |
| 全局状态 | 显式 `z_*_init`，禁用全局构造 | 避免动态库加载顺序问题 |
| 控制台编码 | 启动时 `SetConsoleOutputCP(CP_UTF8)` | Windows 中文日志不乱码 |
| 测试框架 | GoogleTest 1.17.0（已内置） | `add_subdirectory(thirdparty/googletest-1.17.0)` |

---

## 6. 目录结构

```
source/
├── CMakeLists.txt              # 顶层 CMake
├── README.md
├── docs/
│   ├── architecture.md         # 本文档
│   └── phase-1-working-file.md # 第一期工作文件
├── include/                    # 公共头（用户可见）
│   ├── zbase/                  # C ABI 头
│   │   ├── error.h
│   │   ├── string.h
│   │   ├── time.h
│   │   ├── file.h
│   │   ├── perf.h
│   │   └── log.h
│   └── zbase++/                # C++ 包装头（header-only）
│       ├── file.hpp
│       ├── string.hpp
│       ├── perf.hpp
│       └── log.hpp
├── src/                        # 内部实现
│   ├── core/
│   │   ├── error/
│   │   ├── string/
│   │   ├── time/
│   │   ├── file/
│   │   ├── perf/
│   │   └── log/
│   ├── platform/
│   │   ├── windows/
│   │   ├── linux/
│   │   └── macos/
│   └── c_api/                  # C ABI 导出实现
│       ├── error_c.cpp
│       ├── string_c.cpp
│       ├── time_c.cpp
│       ├── file_c.cpp
│       ├── perf_c.cpp
│       └── log_c.cpp
├── tests/                      # GoogleTest 单元测试
├── examples/                   # 调用示例
├── cmake/                      # CMake 工具脚本
└── thirdparty/
    └── googletest-1.17.0/      # 测试框架源码
```

---

## 7. 命名规范

| 元素 | 规范 | 示例 |
|------|------|------|
| 工程名 | 全小写 | `zbase` |
| 正式名 | 大驼峰（文档用） | `ZBase` |
| 文件名 | 全小写 | `file_c.cpp`、`perf.hpp` |
| C ABI 函数 | `z_<module>_<verb>` | `z_file_read_text` |
| C ABI 类型 | `z_<module>_<noun>_t` | `z_log_level_t` |
| 错误码 | `Z_OK` / `Z_ERR_*` | `Z_ERR_NOT_FOUND` |
| 宏 | `ZBASE_*` | `ZBASE_API` |
| C++ 命名空间 | `zbase::` | `zbase::FileReader` |
| C++ 类 | 大驼峰 | `FileReader`、`PerfScope` |
| C++ 函数/变量 | 小驼峰 / 蛇形（按 Google Style） | `ReadAll`、`buf_` |

---

## 8. 内存所有权约定

**总则：库申请的内存必须由库的释放接口释放，禁止调用者直接 `free()`。**

| 接口模式 | 调用方义务 | 示例 |
|---------|----------|------|
| `T** out` 出参 | 用完后调 `z_<module>_free(*out)` | `z_file_read_text(path, &buf, &len)` |
| 调用方预分配缓冲 | 调用方自己管 | `z_file_read_text_into(path, buf, buf_size)` |
| 句柄型（`z_dir_iter_t*`） | 用 `z_dir_iter_destroy(h)` | `z_dir_iter_next(h, &entry)` |

原因：MSVC 的 Debug/Release CRT 不同堆，跨堆 `free` 是 UB；MinGW 与 MSVC 编译的库混用时同样问题。统一走库内释放可避免。

---

## 9. 错误码体系

```c
/* include/zbase/error.h */
#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    Z_OK                =  0,
    Z_ERR_INVALID_ARG  = -1,  ///< 参数非法
    Z_ERR_NOT_FOUND    = -2,  ///< 路径不存在
    Z_ERR_PERMISSION   = -3,  ///< 权限不足
    Z_ERR_IO           = -4,  ///< IO 错误
    Z_ERR_ENCODING     = -5,  ///< 编码转换失败
    Z_ERR_NOMEM        = -6,  ///< 内存不足
    Z_ERR_OVERFLOW     = -7,  ///< 缓冲区溢出
    Z_ERR_UNSUPPORTED  = -8,  ///< 平台不支持
    Z_ERR_UNKNOWN      = -99, ///< 未知错误
} z_error_t;

/* 编译期校验：保证错误码大小与 int 一致，跨平台稳定。
 * C++ 用 static_assert，C 用 _Static_assert（C11）。*/
#ifdef __cplusplus
static_assert(sizeof(z_error_t) == sizeof(int), "z_error_t 必须是 int 大小");
#else
_Static_assert(sizeof(z_error_t) == sizeof(int), "z_error_t 必须是 int 大小");
#endif

const char* z_error_msg(int err);  /* 错误码 → 中文描述 */

#ifdef __cplusplus
}  /* extern "C" */
#endif
```

规则：
- 所有 C ABI 函数返回 `int` 错误码（除非函数语义就是返回指针，此时 `NULL` 表错误）。
- 错误码可扩展，**不可重排已有值**（ABI 兼容性）。
- 新增错误码追加在末尾（`Z_ERR_UNKNOWN = -99` 是上限锚点，新错误码插在 -8 与 -99 之间）。
- `static_assert` 在每个翻译单元首次 `#include` 此头时校验大小，防止编译器将 enum 优化为更小类型导致 ABI 漂移。

---

## 10. 字符编码策略

### 10.1 对外统一 UTF-8

所有 C ABI 函数的 `const char*` 字符串参数（路径、日志消息、字符串内容）一律按 UTF-8 解析。

### 10.2 Windows 内部转 UTF-16

```cpp
// src/platform/windows/encoding.cpp
std::wstring utf8_to_utf16(std::string_view s);  // 调 MultiByteToWideChar
std::string  utf16_to_utf8(std::wstring_view s); // 调 WideCharToMultiByte
```

文件操作在 Windows 上：路径 UTF-8 → UTF-16 → 调 `CreateFileW` / `GetFileAttributesW` / `FindFirstFileW` 等 W 版 API。绝不调 ANSI 版（A 后缀）。

### 10.3 控制台输出

```cpp
// z_log_init 内部
#ifdef _WIN32
::SetConsoleOutputCP(CP_UTF8);
::SetConsoleCP(CP_UTF8);
#endif
```

### 10.4 源码字符集

- 所有源文件以 UTF-8 (无 BOM) 保存
- MSVC 加 `/utf-8` 编译选项（同时设源码和执行字符集为 UTF-8）
- GCC/Clang 加 `-finput-charset=UTF-8 -fexec-charset=UTF-8`

---

## 11. 线程安全约定

| 类别 | 约定 |
|------|------|
| 无状态函数（如 `z_string_split`） | 天然线程安全 |
| 有状态但只读的全局表（错误码描述表） | 线程安全（编译期常量） |
| 需要初始化的子系统（`log`、`perf`） | `init`/`shutdown` 由调用方保证主线程单次调用；运行时调用线程安全 |
| 句柄型对象（`z_dir_iter_t*`） | 单句柄不可多线程并发使用；不同句柄之间互不影响 |

---

## 12. 符号导出宏

```c
/* include/zbase/zbase_def.h */
#if defined(_WIN32)
  #ifdef ZBASE_BUILDING
    #define ZBASE_API __declspec(dllexport)
  #else
    #define ZBASE_API __declspec(dllimport)
  #endif
#else
  #define ZBASE_API __attribute__((visibility("default")))
#endif

#define ZBASE_LOCAL __attribute__((visibility("hidden")))  /* Linux/macOS 内部符号 */
```

构建库时定义 `ZBASE_BUILDING`；调用方不定义。

---

## 13. 版本规划

### v0.1（最小可用版）—— **当前焦点，详见 phase-1-working-file.md**
- CMake 跨平台构建骨架
- 平台层最小封装
- 核心模块：error / string / time / file / perf / 最小 log（仅 stderr）
- C ABI 导出 + C++ 包装层
- GoogleTest 单元测试
- 一个示例程序

### v0.2（完善基础）
- 日志文件输出 + 滚动
- 时间格式化（strftime 风格）
- 字符串高级操作（正则、trim 全家桶）
- CI 自动化（GitHub Actions 多平台）

### v0.3（生态扩展）
- 配置文件解析（INI / TOML）
- 路径处理增强
- 命令行参数解析
- 多语言绑定示例（Python ctypes / Rust bindgen）

---

## 14. 设计原则

1. **最小可用优先**：先做高频刚需，按需迭代，拒绝功能膨胀。
2. **谁申请谁释放**：库内申请的内存必须由库内释放接口释放。
3. **接口只增不改**：保证 ABI 向前兼容，旧版本程序可加载新版本库。
4. **平台差异内部消化**：上层调用者无需关心 Windows/Linux/macOS 区别。
5. **YAGNI**：不做"未来可能用到"的功能；做出来后真有需求再加。
6. **C ABI 优先**：所有功能先有 C 接口，C++ 包装只是糖衣。

---

## 17. 参考资料

- Google C++ Style Guide: https://google.github.io/styleguide/cppguide.html
- 项目编码规则：[.trae/rules/code-rule.md](file:///c:/Repos/ZBase/.trae/rules/code-rule.md)
- 项目 README：[README.md](file:///c:/Repos/ZBase/source/README.md)
- GoogleTest 1.17.0 源码：[thirdparty/googletest-1.17.0/](file:///c:/Repos/ZBase/source/thirdparty/googletest-1.17.0/)
- 第一期工作文件：[phase-1-working-file.md](file:///c:/Repos/ZBase/source/docs/phase-1-working-file.md)
