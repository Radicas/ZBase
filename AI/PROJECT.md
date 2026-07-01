# PROJECT

> 项目事实卡。仅记录事实，不猜测。

---

## 项目目标

ZBase 是朱鹏（Radica Zhu）的个人跨平台 C/C++ 通用工具动态库。一套覆盖日常开发高频场景的底层工具合集，编译为跨平台动态库，让所有项目告别重复造轮子。

核心设计原则：
1. 最小可用优先（YAGNI）
2. 谁申请谁释放（库内申请的内存必须由库内释放接口释放）
3. 接口只增不改（保证 ABI 向前兼容）
4. 平台差异内部消化（上层调用者无需关心 OS 差异）
5. C ABI 优先（所有功能先有 C 接口，C++ 包装只是糖衣）

---

## 技术栈

| 项 | 内容 |
|----|------|
| 语言 | C++20（对外 C ABI 兼容 C11） |
| 构建系统 | CMake 3.20+ + Ninja |
| 编译器 | MSVC、MinGW (GCC)、Linux GCC、macOS Clang |
| 测试框架 | GoogleTest 1.17.0（vendor 进 `source/thirdparty/`） |
| 性能基准 | Google benchmark 1.9.0（vendor，opt-in） |
| CI | GitHub Actions（4 平台矩阵） |
| 版本控制 | Git |
| 许可证 | MIT |

---

## 编译方式

- CMake + Ninja 为首选生成器
- 默认构建动态库（`BUILD_SHARED_LIBS=ON`），可切换静态
- 源码查找使用 `file(GLOB_RECURSE ...)`，禁止硬编码文件列表
- 详见 [BUILD.md](file:///c:/Repos/ZBase/AI/BUILD.md)

---

## 目录结构

```
ZBase/
├── .github/workflows/ci.yml       # CI 配置（4 平台矩阵）
├── .trae/rules/                   # 工作区规则（编码、提交信息）
├── AI/                            # 本目录（AI 上下文）
└── source/                        # 源码根
    ├── CMakeLists.txt             # 顶层 CMake
    ├── README.md                  # 用户 README
    ├── include/                   # 公共头文件（稳定接口）
    │   ├── zbase/                 # C ABI 头
    │   │   ├── internal/          # 内部头（不发布）
    │   │   │   └── c_api_guard.hpp
    │   │   ├── zbase.h            # 版本宏 + 符号导出宏
    │   │   ├── error.h
    │   │   ├── string.h
    │   │   ├── time.h
    │   │   ├── file.h
    │   │   ├── perf.h
    │   │   ├── log.h
    │   │   └── config.h
    │   └── zbase++/               # C++ 包装头（header-only）
    │       ├── error.hpp
    │       ├── file.hpp
    │       ├── string.hpp
    │       ├── time.hpp
    │       ├── perf.hpp
    │       ├── log.hpp
    │       ├── config.hpp
    │       └── console.hpp
    ├── src/                       # 内部实现
    │   ├── core/                  # 核心模块层（C++20，符号隐藏）
    │   │   ├── error/
    │   │   ├── string/
    │   │   ├── time/
    │   │   ├── file/
    │   │   ├── perf/
    │   │   ├── log/
    │   │   └── config/
    │   └── platform/              # 平台适配层（OS 原语封装）
    │       ├── file_platform.{cpp,hpp}
    │       ├── time_platform.{cpp,hpp}
    │       └── encoding.{cpp,hpp}
    ├── tests/                     # GoogleTest 单元测试
    │   ├── core/
    │   ├── platform/
    │   └── integration/
    ├── benchmarks/                # Google benchmark 性能基准
    ├── examples/                  # 调用示例
    │   ├── file_string_demo/
    │   ├── perf_stats/
    │   └── use_as_dependency/     # 独立 CMake 项目，演示外部依赖
    ├── cmake/                     # CMake 工具脚本
    │   ├── zbase_options.cmake
    │   ├── zbase_compile_options.cmake
    │   ├── zbase_install.cmake
    │   └── ZBaseConfig.cmake.in
    ├── docs/                      # 架构与阶段文档
    │   ├── architecture.md
    │   ├── phase-1-done.md
    │   ├── phase-2-done.md
    │   └── superpowers/plans/
    └── thirdparty/                # 第三方依赖（vendor）
        ├── googletest-1.17.0/
        └── benchmark-1.9.0/
```

> **注意**：架构文档 `architecture.md §6` 描述的目录结构包含 `src/c_api/` 子目录，但实际实现未采用此分层（YAGNI 决策，见 `phase-1-working-file.md §4.28`）。C ABI 函数直接在 `src/core/<module>/` 的 `.cpp` 文件中以 `extern "C"` 块实现。以实际代码为准。

---

## 主要模块

| 模块 | 头文件 | C ABI 前缀 | 功能 |
|------|--------|-----------|------|
| error | `zbase/error.h` | `z_error_*` | 错误码枚举 + 中文描述 |
| string | `zbase/string.h` | `z_string_*` | split / join / replace / trim / 大小写 / UTF-8 长度 |
| time | `zbase/time.h` | `z_time_*` | now_ms / now_us / format_iso / sleep_ms / sleep_us |
| file | `zbase/file.h` | `z_file_*` / `z_dir_iter_*` | read_text / write_text / exists / mkdir / dir_iter |
| perf | `zbase/perf.h` | `z_perf_*` | 命名打点 start/end / dump / reset |
| log | `zbase/log.h` | `z_log_*` | 4 级日志 + init/shutdown + 文件滚动 |
| config | `zbase/config.h` | `z_config_*` | INI 配置加载 + get/get_int/get_bool |
| version | `zbase/zbase.h` | `z_version` / `z_init_console` | 版本字符串 + 控制台 UTF-8 初始化 |

模块依赖方向（自下而上）：`error → string → time → file → perf → log → config`

---

## 外部依赖

| 依赖 | 版本 | 用途 | 引入方式 |
|------|------|------|---------|
| GoogleTest | 1.17.0 | 单元测试 | vendor 进 `source/thirdparty/` |
| Google benchmark | 1.9.0 | 性能基准 | vendor，opt-in（`ZBASE_BUILD_BENCHMARKS=ON`） |

核心库无第三方依赖。

---

## 平台

| 平台 | 动态库产物 | 工具链 |
|------|----------|--------|
| Windows | `zbase.dll` + `zbase.lib` | MSVC、MinGW (LLVM-MinGW) |
| Linux | `libzbase.so` | GCC |
| macOS | `libzbase.dylib` | Clang |

源文件统一 UTF-8 (无 BOM) 保存。

---

## 已发现的重要事实

1. **版本号**：当前 `0.2.0`（`ZBASE_VERSION_STRING`，定义于 `include/zbase/zbase.h`）。
2. **导出符号守门**：v0.1 上限 30 个，v0.2 上限 8 个新增（累计 37）。CI 有符号导出审计步骤（`nm` / `dumpbin`），要求导出符号全部以 `z_` 开头且数量 ≤ 40。
3. **错误码不可重排**：`Z_OK=0`，负数表错误，新增错误码追加在 `-8` 与 `-99` 之间，已有值不可变更（ABI 兼容性）。
4. **内存所有权**：库申请的内存必须由库的释放接口释放（`z_file_free` / `z_string_free` / `z_string_free_array` / `z_config_free` / `z_dir_iter_destroy`），禁止调用方 `free()`。原因：MSVC Debug/Release CRT 不同堆，跨堆 free 是 UB。
5. **字符编码**：对外统一 UTF-8；Windows 内部转 UTF-16 调 W 版 Win32 API（绝不调 A 后缀）；MSVC 加 `/utf-8`，GCC/Clang 加 `-finput-charset=UTF-8 -fexec-charset=UTF-8`。
6. **C ABI 异常隔离**：所有返回 `int` 的 C ABI 函数体必须用 `ZBASE_C_API_BEGIN` / `ZBASE_C_API_END` 宏包裹，防止 C++ 异常逃逸到 C 调用方（UB）。`void` 返回函数内部 try/catch 静默吞异常。
7. **OBJECT 库模式**：`zbase_objects` 编译一次，被 `zbase`（共享/静态库）和 `zbase_tests`（单测）共用。测试链接 `zbase_objects` 可访问未导出的内部符号。
8. **GoogleTest / benchmark 强制静态**：避免 `BUILD_SHARED_LIBS=ON` 被 thirdparty 继承导致生成 `.dll` 运行时找不到符号。CMake 中局部保存/恢复 `BUILD_SHARED_LIBS`。
9. **MSVC 环境要求**：构建前需通过 `VsDevCmd.bat` 配置构建变量（项目记忆中的经验）。
10. **控制台编码**：`z_init_console()` 在 Windows 下调用 `SetConsoleOutputCP(CP_UTF8)`，POSIX 下为空操作。所有可执行程序（测试、示例）应在 `main()` 开头调用。
11. **install 目标**：CMake install 导出 `ZBase::zbase` target，外部项目通过 `find_package(ZBase)` 使用。
12. **C++ 包装层不保证 ABI 稳定**：`zbase++/*.hpp` 是 header-only，要求调用方与库使用相同 C++ 标准和接近的编译选项。
