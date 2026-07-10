# BUILD

> 编译、运行、调试、测试方法。

---

## 前置环境

### 通用

- CMake ≥ 3.20
- Ninja（首选生成器）
- C++20 兼容编译器

### 各平台工具链

| 平台 | 工具链 | 备注 |
|------|--------|------|
| Windows | MSVC | 需先在 `VsDevCmd.bat` 配置的环境内构建（项目经验） |
| Windows | MinGW | 推荐 LLVM-MinGW（C++20 支持好）；CI 用 `llvm-mingw-20240619-ucrt-x86_64` |
| Linux | GCC | ≥ 10 |
| macOS | Clang | ≥ 10 |

---

## 编译

> [!IMPORTANT]
> **AI 编译规则：必须通过项目提供的 Python 脚本编译，禁止直接使用 PowerShell/Bat/Cmd 执行 cmake 命令。**
>
> 编译统一使用 `source/scripts/build_project.py`：
> ```bash
> python source/scripts/build_project.py                    # 默认 Debug + 测试
> python source/scripts/build_project.py --release          # Release 构建
> python source/scripts/build_project.py --asan              # 启用 ASan
> python source/scripts/build_project.py --no-tests          # 跳过测试
> python source/scripts/build_project.py --clean             # 清理后构建
> python source/scripts/build_project.py --release --no-tests  # Release + 跳过测试
> python source/scripts/build_project.py --benchmarks        # 包含性能基准
> ```
>
> 脚本自动完成：自动查找 VS 环境 (`VsDevCmd.bat`)、CMake 配置、编译、测试（可选）全流程。
> 如需其他选项，运行 `python source/scripts/build_project.py --help` 查看完整参数列表。

> [!CAUTION]
> **新增功能模块或结构变更时**，如需新增或修改编译脚本（如 CMakeLists 调整、新增构建参数等），
> 必须同步更新 `source/scripts/build_project.py`。临时的编译辅助脚本在使用完毕后必须删除，不得提交到仓库。


## 编译产物

| 平台 | 动态库 | 静态库 | 导入库 |
|------|--------|--------|--------|
| Windows | `zbase.dll` | `zbase.lib`（静态） | `zbase.lib`（导入） |
| Linux | `libzbase.so` | `libzbase.a` | — |
| macOS | `libzbase.dylib` | `libzbase.a` | — |

---

## 输出目录

- 默认构建目录：`build/`（仓库根下）
- 可执行程序（测试、示例）：`build/tests/`、`build/examples/`
- 测试 CTest 配置：`build/Testing/`
- 已有 IDE 产物：`source/cmake-build-debug/`、`source/out/install/x64-Debug/`（CLion / VS 生成）

---

## 编译配置细节

### 编译选项（`source/cmake/zbase_compile_options.cmake`）

通过 `zbase_apply_compile_options(target)` 函数统一应用：

**MSVC：**
- PUBLIC：`/utf-8`（生成器表达式，仅 MSVC 消费者继承）
- PRIVATE：`/W4 /permissive- /WX- /MP`
- 定义：`WIN32_LEAN_AND_MEAN` `NOMINMAX` `_CRT_SECURE_NO_WARNINGS` `UNICODE` `_UNICODE`

**GCC/Clang：**
- PUBLIC：`-finput-charset=UTF-8` `-fexec-charset=UTF-8`（生成器表达式）
- PRIVATE：`-Wall -Wextra -Wpedantic` `-fvisibility=hidden` `-fvisibility-inlines-hidden`
- 动态库模式：`POSITION_INDEPENDENT_CODE ON`

### CRT 一致性（`source/cmake/zbase_options.cmake`）

- 动态库 → `MultiThreadedDLL`
- 静态库 → `MultiThreaded`
- `CMAKE_MSVC_RUNTIME_LIBRARY` 被 CACHE FORCE 设定

### 源码查找

```cmake
file(GLOB_RECURSE ZBASE_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/platform/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/core/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/c_api/*.cpp"   # 实际不存在此目录，保留无害
)
```

> 实际 `src/c_api/` 目录不存在，GLOB 不匹配任何文件。AI 新增模块应放在 `src/core/<module>/`。

### OBJECT 库模式

- `zbase_objects`（OBJECT）编译一次
- `zbase`（SHARED/STATIC）消费 `$<TARGET_OBJECTS:zbase_objects>`
- `zbase_tests` 直接链接 `zbase_objects`，可访问未导出内部符号

---

## 运行

### 示例

构建后示例位于 `build/examples/`：

```bash
# 文件 + 字符串综合示例
./build/examples/file_string_demo/zbase_file_string_demo

# perf 统计示例
./build/examples/perf_stats/zbase_perf_stats
```

### Windows DLL 路径

运行动态库链接的可执行程序时，`zbase.dll` 需在 PATH 或 exe 同目录。CI 与 install 示例通过 `add_custom_command POST_BUILD copy_if_different` 自动拷贝。

---

## 调试

### 本地调试建议

1. 用 `CMAKE_BUILD_TYPE=Debug` 构建
2. MSVC：在 `VsDevDevCmd` 环境内用 VS 打开 `build/` 调试
3. MinGW：GDB 调试
4. 控制台中文输出：可执行程序 `main()` 开头必须调 `z_init_console()`，否则 Windows 下中文乱码

### 常见问题排查

| 现象 | 原因 | 对策 |
|------|------|------|
| MSVC 编译报错找不到符号 | 未在 `VsDevCmd.bat` 环境内构建 | 先调 VsDevCmd.bat |
| Windows 中文乱码 | 未调 `z_init_console()` | `main()` 开头加 `z_init_console();` |
| 测试链接报 dllimport 错误 | 测试未定义 `ZBASE_STATIC_DEFINE` | tests/CMakeLists.txt 已设置，勿删 |
| GoogleTest 编译为 .dll 找不到符号 | `BUILD_SHARED_LIBS=ON` 泄漏到 thirdparty | 顶层 CMake 已局部保存/恢复，勿改 |
| MSVC 编码警告 C4819 | 源文件非 UTF-8 或未加 `/utf-8` | 确保源文件 UTF-8 无 BOM + `/utf-8` |
| 跨堆 free 崩溃 | 调用方直接 `free()` 库内内存 | 改用 `z_*_free` / `z_*_destroy` |

---

## 安装

### 本地安装

```bash
cmake --install build --prefix /path/to/install
```

安装内容：
- `include/zbase/*.h`、`include/zbase++/*.hpp`（排除 `internal/`）
- `lib/zbase.{dll,lib,so,dylib}` + `lib/zbase.a`（静态模式）
- `lib/cmake/ZBase/ZBaseTargets.cmake`、`ZBaseConfig.cmake`、`ZBaseConfigVersion.cmake`

### 外部项目使用

```cmake
find_package(ZBase 0.2 REQUIRED)
target_link_libraries(my_app PRIVATE ZBase::zbase)
```

Windows 下需 POST_BUILD 拷贝 `zbase.dll`。完整示例见 [source/examples/use_as_dependency/](file:///c:/Repos/ZBase/source/examples/use_as_dependency/)。

---

## 执行测试

### 命令行

```bash
# 进入构建目录
cd build

# 运行全部测试
ctest --output-on-failure

# 运行特定模块测试
ctest -R String --output-on-failure
ctest -R File --output-on-failure
ctest -R Log --output-on-failure
ctest -R Config --output-on-failure
ctest -R Perf --output-on-failure
ctest -R Time --output-on-failure
ctest -R CppWrapper --output-on-failure
```

### 直接运行测试可执行文件

```bash
./build/tests/zbase_tests
./build/tests/zbase_tests --gtest_filter=StringSplit.*
```

### 基准测试

```bash
# 需先启用基准构建
cmake -S source -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DZBASE_BUILD_BENCHMARKS=ON
cmake --build build --config Release

# 运行基准（每个基准独立可执行）
./build/benchmarks/bench_string
./build/benchmarks/bench_file
./build/benchmarks/bench_log
./build/benchmarks/bench_config
```

---

## CI 构建

CI 配置：[.github/workflows/ci.yml](file:///c:/Repos/ZBase/.github/workflows/ci.yml)

- 触发：push 到 `main` 或任意 PR
- 矩阵：4 套工具链（Win MSVC / Win MinGW / Linux GCC / macOS Clang）
- 步骤：checkout → 安装 CMake+Ninja → 配置工具链 → CMake 配置 → 构建 → `ctest` → 符号导出审计
- 失败时上传 `LastTest.log` / `CMakeOutput.log` / `CMakeError.log` 作为 artifact（保留 7 天）

### 符号导出审计

CI 在测试后运行：
- Linux/macOS：`nm -D --defined-only libzbase.so | awk '$2=="T" && $3 ~ /^z_/'`
- Windows MSVC：`dumpbin /exports zbase.dll | findstr "z_"`

要求：导出符号全部以 `z_` 开头且数量 ≤ 40。
