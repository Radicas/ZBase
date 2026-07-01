# TESTING

> 测试框架与执行方式。

---

## 测试框架

**GoogleTest 1.17.0**（vendor 进 `source/thirdparty/googletest-1.17.0/`）

- 通过 `add_subdirectory(thirdparty/googletest-1.17.0 EXCLUDE_FROM_ALL)` 引入
- 强制静态链接：顶层 CMake 局部保存/恢复 `BUILD_SHARED_LIBS=OFF`，避免 `gtest.dll` 运行时找不到符号
- `gtest_force_shared_crt OFF`
- 使用 `gtest_discover_tests(zbase_tests)` 注册到 CTest

### 链接方式

测试可执行文件 `zbase_tests` 直接链接 `zbase_objects`（OBJECT 库），**可访问未导出的内部符号**：

```cmake
target_link_libraries(zbase_tests PRIVATE zbase_objects gtest gtest_main)
target_compile_definitions(zbase_tests PRIVATE ZBASE_STATIC_DEFINE)
target_include_directories(zbase_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
```

- `ZBASE_STATIC_DEFINE` 让 `zbase.h` 的 `ZBASE_API` 展开为空，避免 dllimport 引用未导出符号
- include `src/` 路径使测试可访问平台层与核心层内部头

---

## 测试目录结构

```
source/tests/
├── CMakeLists.txt
├── test_init.cpp                    # 初始化相关测试
├── core/                            # 核心模块单测
│   ├── test_error.cpp
│   ├── test_string.cpp
│   ├── test_time.cpp
│   ├── test_file.cpp
│   ├── test_perf.cpp
│   ├── test_log.cpp
│   ├── test_config.cpp
│   └── test_cpp_wrapper.cpp         # C++ 包装层冒烟测试
├── platform/                        # 平台层测试
│   ├── test_file_platform.cpp
│   ├── test_time_platform.cpp
│   └── test_encoding.cpp
└── integration/                     # 集成测试
    └── test_demo.cpp
```

源码查找：`file(GLOB_RECURSE ZBASE_TEST_SOURCES CONFIGURE_DEPENDS "tests/*.cpp")`

---

## 如何执行测试

### 命令行（CTest）

```bash
# 构建（默认 ZBASE_BUILD_TESTS=ON）
cmake -S source -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release

# 运行全部测试
cd build
ctest --output-on-failure

# 运行特定测试套件
ctest -R String --output-on-failure
ctest -R File --output-on-failure
ctest -R Log --output-on-failure
ctest -R Config --output-on-failure
ctest -R Perf --output-on-failure
ctest -R Time --output-on-failure
ctest -R CppWrapper --output-on-failure
ctest -R Encoding --output-on-failure
```

### 直接运行测试可执行文件

```bash
# 全部
./build/tests/zbase_tests

# 过滤
./build/tests/zbase_tests --gtest_filter=StringSplit.*
./build/tests/zbase_tests --gtest_filter=-FileWrite*   # 排除

# 列出所有用例
./build/tests/zbase_tests --gtest_list_tests
```

### CI 执行

GitHub Actions 矩阵在构建后执行 `ctest --output-on-failure`，4 平台（Win MSVC / Win MinGW / Linux GCC / macOS Clang）全量运行。失败时上传 `build/Testing/Temporary/LastTest.log`。

---

## 是否存在自动测试

**是。**

- 本地：`ctest` 一键运行
- CI：每次 push 到 `main` 或 PR 自动触发 4 平台矩阵
- 测试发现：`gtest_discover_tests` 在构建时自动注册所有 `TEST` / `TEST_F` 到 CTest

### 已知测试规模

| 版本 | 测试用例数 | 状态 |
|------|----------|------|
| v0.1 | 107/107 PASS | Win MSVC + Win MinGW 本地全绿 |
| v0.2 | 139/139 PASS | Win MSVC 本地全绿；CI 4 平台全绿 |

---

## 哪些模块缺少测试

### 已有测试覆盖

| 模块 | 测试文件 | 覆盖情况 |
|------|---------|---------|
| error | `test_error.cpp` | 错误码 + 描述 |
| string | `test_string.cpp` | split / join / replace / trim / 大小写 / UTF-8 长度 |
| time | `test_time.cpp` | now_ms / now_us / format_iso / sleep |
| file | `test_file.cpp` | read / write / exists / mkdir / dir_iter（含中文路径） |
| perf | `test_perf.cpp` | start / end / dump / reset |
| log | `test_log.cpp` | init / write / 文件输出 / 滚动 |
| config | `test_config.cpp` | load / get / get_int / get_bool（含中文 key/value） |
| C++ 包装 | `test_cpp_wrapper.cpp` | FileReader / StringSplit / DirIterator / PerfScope / Logger / Time |
| 平台层 | `test_file_platform.cpp` / `test_time_platform.cpp` / `test_encoding.cpp` | NormalizePath / FileStatGet / MonoNowNs / UTF-8↔UTF-16 |

### 测试覆盖较弱或缺失

| 模块/场景 | 缺失情况 | 风险 |
|----------|---------|------|
| `z_init_console` 重复调用幂等性 | Unknown 是否有显式测试 | 低 |
| `z_log_file_open` 重复调用（先关闭旧文件） | Unknown | 中 |
| log 滚动并发竞态 | 未发现并发测试 | 中 |
| perf 跨线程同名嵌套 start/end | 未发现跨线程测试 | 中（语义未确认） |
| `z_dir_iter` reserved 字段置零约定 | Unknown | 中 |
| Windows 长路径（>248）`\\?\` 前缀 | Unknown | 中 |
| 错误码新增后的 ABI 兼容性回归 | 无自动化校验 | 低 |
| 性能回归 | 基准测试存在但未纳入 CI 通过条件 | 中 |
| 内存泄漏检测 | 未发现 ASan / Valgrind 集成 | 中 |
| 异常路径（`ZBASE_C_API_END` 触发场景） | 未发现强制异常注入测试 | 中 |

### 未纳入 CI 的验证

- **内存安全**：CI 未跑 ASan / UBSan / Valgrind
- **性能基准**：`ZBASE_BUILD_BENCHMARKS=ON` 默认关闭，CI 未启用（文档建议仅在 ubuntu-latest 开启以减构建时间，但当前 ci.yml 未启用）
- **静态分析**：未发现 clang-tidy / clang-analyzer / PVS-Studio 集成
- **代码覆盖率**：未发现 gcov / lcov / coverage 报告

---

## 测试编写约定

### 文件头（强制 doxygen）

```cpp
/**
 * @file test_string.cpp
 * @author 朱鹏 (Radica Zhu)
 * @date 2026-06-29
 * @version 0.1.0
 * @brief 字符串工具单元测试
 */
```

### 测试命名

- `TEST(SuiteName, CaseName)`，SuiteName 用模块名大驼峰（如 `StringSplit`、`FileWrite`、`CppWrapper`）
- 中文 case 名允许（如 `EXPECT_NE(std::string(e.what()).find("找不到"), std::string::npos)`）

### 测试清理

测试中创建的临时文件应在测试结束 `std::remove`，避免污染工作目录。已有测试遵循此约定。

### main 入口

`gtest_main` 已链接，无需手写 `main()`。但若测试涉及控制台中文输出，应在 `TEST` 内或 `main()` 调 `z_init_console()`（实际由 `z_log_init` 间接调用，多数测试通过 `zbase::Logger` RAII 触发）。
