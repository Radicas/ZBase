<!--
@file phase-2-working-file.md
@author 朱鹏 (Radica Zhu)
@date 2026-06-30
@version 1.0
@brief ZBase 第二期（v0.2）工作文件，记录本期范围、注意事项、风险与下一步。
@details 本文档是"活"文档，随 v0.2 进展持续更新。完成后归档，转入 v0.3 工作文件。
-->

# ZBase 第二期工作文件（v0.2）

> 本期目标：在 v0.1 稳定基线之上，补齐跨平台 CI、日志文件输出、配置文件解析、性能基准测试四项，让 ZBase 进入"可被外部项目长期依赖"的状态。
> 整体架构参见 [architecture.md](file:///c:/Repos/ZBase/source/docs/architecture.md)；v0.1 交付参见 [phase-1-done.md](file:///c:/Repos/ZBase/source/docs/phase-1-done.md)。

**状态**：📋 规划中（未开始编码）
**更新日期**：2026-06-30

---

## 1. 本期范围

### 1.1 必须交付（P0）

| # | 模块/工程 | 交付内容 | 完成标志 |
|---|----------|---------|---------|
| 1 | CI 自动化 | GitHub Actions 四平台矩阵（Win MSVC / Win MinGW / Linux GCC / macOS Clang） | 推送即触发，全绿 |
| 2 | 跨平台烟测补全 | 在 CI 上跑 Linux GCC + macOS Clang 全量测试 | 107/107 PASS（v0.1 已有测试） |
| 3 | log 文件输出 | `z_log_file_open/close` + 按大小滚动 | 单元测试覆盖滚动逻辑 |
| 4 | log 滚动策略 | 单文件超 max_size 重命名 .1 .2 ... .max_files | 中文路径支持 |
| 5 | config 模块 | INI 风格配置加载 + get/get_int/get_bool | 单元测试覆盖中文 key/value |
| 6 | 性能基准 | Google benchmark 1.9.0 集成 + 4 个模块基准 | `ctest -R Benchmark` 跑通 |

### 1.2 显式不做（P1+，留给 v0.3 及以后）

- 日志异步刷盘（双缓冲 + 后台线程）
- 日志颜色输出
- 字符串正则、模板渲染
- 时间字符串解析（strptime 等价物）
- 配置文件写回 / 热更新
- 多语言绑定（Python/Rust/Go）
- Windows 事件日志 / Linux syslog 桥接
- JSON / YAML 配置格式

---

## 2. 实现顺序（自下而上）

```
Step 0  工作文件 + 实现计划      ── 本文档 + plans/2026-06-30-zbase-v0.2.md
Step 1  CI 自动化（GitHub Actions） ── 补全 Linux/macOS 烟测，建立回归基线
Step 2  log 文件输出 + 滚动       ── 扩展现有 log 模块
Step 3  config 模块               ── 新模块，依赖 string/error/file
Step 4  性能基准测试套件          ── Google benchmark 集成 + 4 个基准
Step 5  v0.2.0 收尾               ── 文档归档 + tag
```

每一步完成的标准：**单测全绿 + 至少一处实际验证**。

---

## 3. 模块详细范围

### 3.1 CI 自动化（GitHub Actions）

`.github/workflows/ci.yml`：

```yaml
name: CI
on:
  push:
    branches: [main]
  pull_request:

jobs:
  build:
    strategy:
      fail-fast: false
      matrix:
        os: [ubuntu-latest, macos-latest, windows-latest]
        toolchain: [default, mingw]
        exclude:
          - os: ubuntu-latest
            toolchain: mingw
          - os: macos-latest
            toolchain: mingw
    runs-on: ${{ matrix.os }}
    steps:
      - uses: actions/checkout@v4
        with:
          submodules: true
      - name: Configure
        shell: bash
        run: |
          cmake -S source -B build -G Ninja \
            -DCMAKE_BUILD_TYPE=Release \
            ${{ matrix.toolchain == 'mingw' && '-DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++' || '' }}
      - name: Build
        run: cmake --build build --config Release
      - name: Test
        working-directory: build
        run: ctest --output-on-failure
```

**坑位**：
- Windows MinGW 需要 `setup-mingw` action 或预装
- 子模块方式拉 GoogleTest 1.17.0（v0.1 是 vendor 进来的）
- Ninja 在 runner 上的预装情况：ubuntu/macos 有，windows 需 `choco install ninja`

### 3.2 log 文件输出 + 滚动

C ABI：

```c
/**
 * @brief 打开日志文件输出（同时输出到 stderr 和文件）
 * @param path 日志文件路径（UTF-8）
 * @param max_size 单文件最大字节数，超过则滚动（0 表示不滚动，无限大）
 * @param max_files 保留的历史文件数（1~99，超出部分删除最旧的）
 * @return Z_OK / Z_ERR_INVALID_ARG / Z_ERR_IO
 */
ZBASE_API int z_log_file_open(const char* path, uint64_t max_size, uint32_t max_files);

/**
 * @brief 关闭日志文件输出（stderr 仍继续）
 */
ZBASE_API void z_log_file_close(void);
```

实现要点：
- `log.cpp` 内部追加 `static std::ofstream g_log_file;` `static std::mutex g_file_mutex;` `static std::string g_file_path;` `static uint64_t g_current_size, g_max_size;` `static uint32_t g_max_files;`
- `z_log_write` 加锁后同时输出到 stderr 和（若打开）g_log_file
- 每次写后 `g_current_size += n`，若 `g_current_size >= g_max_size && g_max_size > 0` 触发滚动
- 滚动流程：关闭文件 → 倒序重命名 `path.N-1` → `path.N`（删除原 `path.N`）... → `path.1` → `path` → 重新打开 `path` → `g_current_size = 0`
- Windows 中文路径走 UTF-16 转换（复用 platform/encoding）

C++ 包装：

```cpp
class LogFile {
 public:
  LogFile(std::string_view path, uint64_t max_size = 0, uint32_t max_files = 0) {
    ThrowIfError(z_log_file_open(path.data(), max_size, max_files));
  }
  ~LogFile() { z_log_file_close(); }
  LogFile(const LogFile&) = delete;
  LogFile& operator=(const LogFile&) = delete;
};
```

### 3.3 config 配置文件解析

格式（INI 子集）：

```ini
# 注释
[section]
key = value
key2 = 中文值
int_key = 42
bool_key = true   ; 支持 true/false/1/0/yes/no/on/off（大小写不敏感）
```

C ABI：

```c
typedef struct z_config_t z_config_t;  ///< 不透明句柄

ZBASE_API int z_config_load(const char* path, z_config_t** out);
ZBASE_API void z_config_free(z_config_t* config);
ZBASE_API const char* z_config_get(z_config_t* config, const char* section, const char* key, const char* default_val);
ZBASE_API int z_config_get_int(z_config_t* config, const char* section, const char* key, int default_val);
ZBASE_API int z_config_get_bool(z_config_t* config, const char* section, const char* key, int default_val);
```

实现要点：
- 内部用 `std::map<std::pair<std::string, std::string>, std::string>` 或 `std::unordered_map`
- 解析时按行处理：`#` 或 `;` 开头是注释；`[xxx]` 是 section；`key=value` 是键值对
- key/value 前后空白自动 trim
- key 区分大小写（INI 通用约定）
- 不支持多行值、不支持转义（v0.2 YAGNI）
- 文件用二进制模式读入，UTF-8 直接处理

C++ 包装：

```cpp
class Config {
 public:
  explicit Config(std::string_view path) { ThrowIfError(z_config_load(path.data(), &handle_)); }
  ~Config() { z_config_free(handle_); }
  Config(const Config&) = delete;
  Config& operator=(const Config&) = delete;
  std::string Get(std::string_view section, std::string_view key, std::string_view default_val = "") const {
    return z_config_get(handle_, section.data(), key.data(), std::string(default_val).c_str());
  }
  int GetInt(std::string_view section, std::string_view key, int default_val = 0) const {
    return z_config_get_int(handle_, section.data(), key.data(), default_val);
  }
  bool GetBool(std::string_view section, std::string_view key, bool default_val = false) const {
    return z_config_get_bool(handle_, section.data(), key.data(), default_val ? 1 : 0) != 0;
  }
 private:
  z_config_t* handle_ = nullptr;
};
```

### 3.4 性能基准测试套件

- 第三方：`thirdparty/benchmark-1.9.0/`（vendor 进来，和 GoogleTest 一样）
- CMake 选项：`ZBASE_BUILD_BENCHMARKS`（默认 OFF）
- 目录：`source/benchmarks/`
- 基准：
  - `bench_string.cpp`：split / join / replace / trim
  - `bench_file.cpp`：read_text / write_text
  - `bench_log.cpp`：z_log_write 吞吐
  - `bench_config.cpp`：load + get
- 用 `file(GLOB_RECURSE ... CONFIGURE_DEPENDS)` 查找
- 每个基准独立可执行文件，链接 `zbase` + `benchmark::benchmark`

---

## 4. 注意事项（坑位清单）

### 4.1 YAGNI 守门（v0.2 版）

v0.1 已用 30 个导出符号（命中上限）。v0.2 新增：
- log 文件输出：2 个（z_log_file_open / z_log_file_close）
- config：5 个（z_config_load / z_config_free / z_config_get / z_config_get_int / z_config_get_bool）

合计 7 个新增，累计 37 个。**v0.2 上限 8 个新增**，留 1 个缓冲。

### 4.2 GoogleTest 子模块化

v0.1 把 GoogleTest 1.17.0 vendor 进 `thirdparty/`。CI 上需要 submodule 或 vendor 二选一。本期保持 vendor 模式，CI 用 `actions/checkout@v4` 即可，无需 submodule。

Google benchmark 1.9.0 同样 vendor 进来。

### 4.3 Windows MinGW CI 环境

GitHub Actions `windows-latest` 默认不带 MinGW-w64。两种方案：
1. `msys2/setup-msys2` action 安装 mingw-w64-gcc-g++
2. 用 LLVM-MinGW（更现代，C++20 支持好）

推荐方案 2（与本地 v0.1 烟测一致）。

### 4.4 benchmark 不影响主构建

`ZBASE_BUILD_BENCHMARKS=OFF` 默认。CI 矩阵中只在 ubuntu-latest 上开启，减少构建时间。

### 4.5 log 滚动的文件锁

Windows 上 rename 不能覆盖已存在文件，需要 `MoveFileExW(MOVEFILE_REPLACE_EXISTING)`。POSIX 用 `rename()` 直接覆盖。

### 4.6 config 解析的内存所有权

`z_config_get` 返回内部指针，调用方不能 free。指针在 `z_config_free` 后失效。文档明确说明。

### 4.7 CI 触发策略

- push 到 main 触发全矩阵
- PR 触发全矩阵
- 不做 nightly（YAGNI）
- 失败时上传 build log 和 ctest log 作为 artifact

### 4.8 benchmark 不算入 YAGNI 守门

benchmark 是测试代码，不导出符号，不计入 30 上限。

---

## 5. 风险与对策

| 风险 | 影响 | 严重度 | 对策 |
|------|------|--------|------|
| GitHub Actions runner 上 MinGW 版本不一 | CI 飘绿/飘红 | 高 | 锁定 LLVM-MinGW 17.0.6 release |
| GoogleTest vendor 体积大 | 仓库膨胀 | 中 | 已接受（v0.1 已 vendor） |
| log 滚动并发竞态 | 日志丢失 | 中 | 全程持 `g_file_mutex` |
| config 解析中文 BOM | 首行 section/key 读不到 | 中 | 读到 UTF-8 BOM 时跳过前 3 字节 |
| benchmark 版本与 GCC 不兼容 | 编译失败 | 低 | 锁定 1.9.0，已在本地预验 |
| CI 一次跑满 4 平台太慢 | 反馈延迟 | 低 | 缓存 ccache（Linux/macOS），Windows 不缓存 |

---

## 6. 下一步（立即可执行）

按顺序：

1. **创建 phase-2-working-file.md**（本文档）✅
2. **Step 1 CI 自动化**：写 `.github/workflows/ci.yml`，推送到分支看首次跑结果
3. **Step 2 log 文件输出**：实现 + 单测 + 集成测试
4. **Step 3 config 模块**：实现 + 单测
5. **Step 4 性能基准**：vendor benchmark + 4 个基准文件 + CMake 集成
6. **Step 5 v0.2.0 收尾**：归档 phase-2-working-file.md → phase-2-done.md，tag v0.2.0

### 当前阻塞项

无。

### 待用户确认事项

无（v0.2 范围已确认）。

---

## 7. 进度跟踪

| 步骤 | 内容 | 状态 | 备注 |
|------|------|------|------|
| Step 0 | 工作文件 + 实现计划 | 🟦 进行中 | 本文档 |
| Step 1 | CI 自动化 | ⬜ 未开始 | |
| Step 2 | log 文件输出 + 滚动 | ⬜ 未开始 | |
| Step 3 | config 模块 | ⬜ 未开始 | |
| Step 4 | 性能基准测试套件 | ⬜ 未开始 | |
| Step 5 | v0.2.0 收尾 | ⬜ 未开始 | |

状态图例：⬜ 未开始 / 🟦 进行中 / ✅ 完成 / ⚠️ 阻塞

---

## 8. 参考资料

- v0.1 工作文件：[phase-1-done.md](file:///c:/Repos/ZBase/source/docs/phase-1-done.md)
- 架构总览：[architecture.md](file:///c:/Repos/ZBase/source/docs/architecture.md)
- 项目编码规则：[.trae/rules/code-rule.md](file:///c:/Repos/ZBase/.trae/rules/code-rule.md)
- Google benchmark: https://github.com/google/benchmark
- GitHub Actions CMake 模板: https://docs.github.com/en/actions/automating-builds-and-tests/building-and-testing-cpp
