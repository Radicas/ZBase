# ARCHITECTURE

> 架构总览。完整版见 [source/docs/architecture.md](file:///c:/Repos/ZBase/source/docs/architecture.md)。

---

## 五层架构（蛋糕模型）

依赖向内指，无环。下层永远不知道上层存在。

```
┌─────────────────────────────────────────────────────────────────┐
│  ⑤ 公共头文件                                                    │
│     include/zbase/*.h     (C ABI 头，对外稳定)                   │
│     include/zbase++/*.hpp (header-only C++ 包装)                │
├─────────────────────────────────────────────────────────────────┤
│  ④ C++ 包装层  (header-only, RAII)                               │
│     zbase::FileReader / Logger / LogFile / DirIterator ...       │
│     依赖 ③，提供 RAII、std::string / std::filesystem 互操作        │
├─────────────────────────────────────────────────────────────────┤
│  ③ C ABI 导出层  (extern "C" + ZBASE_API)                        │
│     实际位于 src/core/<module>/*.cpp 内的 extern "C" 块          │
│     z_file_* / z_string_* / z_time_* / z_perf_* / z_log_* /     │
│     z_config_* / z_dir_iter_*                                   │
│     所有异常在边界 catch，转换为错误码；内存由库管                  │
├─────────────────────────────────────────────────────────────────┤
│  ② 核心模块层  src/core/{error,string,time,file,perf,log,config}/│
│     现代C++20实现，所有符号内部可见（visibility=hidden）           │
├─────────────────────────────────────────────────────────────────┤
│  ① 平台适配层  src/platform/                                     │
│     Win32/POSIX 原语封装（file_platform / time_platform / encoding）│
│     无对外 API，通过 #ifdef _WIN32 / __linux__ / __APPLE__ 选择编译│
└─────────────────────────────────────────────────────────────────┘
```

> **注意**：架构文档描述的 `src/c_api/` 独立目录在实际代码中未实现。C ABI 函数与核心逻辑合并在 `src/core/<module>/<module>.cpp` 中（YAGNI 决策）。AI 修改时应以实际代码结构为准，不要强行拆分。

### 各层职责

- **① 平台适配层**：封装 OS 差异（文件句柄、时间原语、休眠、字符编码转换）。命名空间 `zbase::platform`。
- **② 核心模块层**：业务逻辑实现。匿名命名空间内函数，不对外暴露符号。
- **③ C ABI 导出层**：`extern "C"` + `ZBASE_API`，薄包装。所有异常在此 catch，所有内存分配在此明确归属。
- **④ C++ 包装层**：header-only，调用 C ABI，提供 RAII。命名空间 `zbase::`。
- **⑤ 公共头文件**：用户 `#include` 入口，对外承诺稳定。

---

## 模块依赖图

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
               log    ←  time + error + platform
                ↑
              config  ←  error + platform
```

依赖规则：
1. 下层模块不能 `#include` 上层头文件。
2. 同层模块之间尽量避免横向依赖（如 `string` 不依赖 `time`）。
3. 平台层只被需要 OS 原语的模块使用；`error`/`string` 纯算法模块不依赖平台层。

---

## 数据流

### 文件读取流程（示例：`z_file_read_text`）

```
调用方 (const char* path)
  → src/core/file/file_reader.cpp: z_file_read_text (extern "C", ZBASE_C_API_BEGIN)
    → zbase::platform::FileStatGet(path, st)          # 平台层查属性
    → Windows: zbase::platform::NormalizePath → CreateFileW → ReadFile
      POSIX:  ::open → ::read
    → std::malloc 缓冲区
  ← 返回 Z_OK + *out_buf + *out_len
调用方用完后调 z_file_free(buf)
```

### 日志写入流程（`z_log_write`）

```
Z_LOG_INFO(fmt, ...) 宏展开
  → z_log_write(level, __FILE__, __LINE__, fmt, ...)
    → 原子读 g_min_level 过滤
    → z_time_now_ms + z_time_format_iso 生成时间戳
    → 提取 basename
    → vsnprintf 格式化用户消息
    → fprintf(stderr, ...) + fflush        # 控制台输出
    → lock_guard(g_file_mutex)
      → 若 g_file.enabled: 拼行 → stream.write → flush
      → g_file.current_size += n
      → 若超 max_size: RollFiles()         # 滚动
```

---

## 初始化流程

### 库初始化约定

- **无全局构造**：动态库加载时不执行任何初始化逻辑，避免加载顺序问题。
- **显式 init**：需要初始化的子系统（`log`）由调用方主动调用 `z_*_init`。
- **可重复调用**：`z_log_init` / `z_init_console` 幂等，可多次调用。
- **`shutdown` 后可再次 `init`**。

### 可执行程序 main() 模式

```cpp
int main() {
    z_init_console();                    // 1. 控制台 UTF-8（Windows 防乱码）
    zbase::Logger logger(Z_LOG_LEVEL_DEBUG);  // 2. RAII 初始化日志，析构自动 shutdown
    // ... 业务 ...
    return 0;
}
```

---

## 生命周期

| 对象 | 创建 | 销毁 | 所有权 |
|------|------|------|--------|
| `zbase::Logger` | 构造调 `z_log_init` | 析构调 `z_log_shutdown` | RAII |
| `zbase::LogFile` | 构造调 `z_log_file_open` | 析构调 `z_log_file_close` | RAII |
| `zbase::FileReader` | 构造调 `z_file_read_text` | 析构调 `z_file_free` | RAII |
| `zbase::DirIterator` | 构造调 `z_dir_iter_create` | 析构调 `z_dir_iter_destroy` | RAII |
| `z_config_t*` | `z_config_load` | `z_config_free` | 调用方负责 |
| `z_dir_iter*` | `z_dir_iter_create` | `z_dir_iter_destroy` | 调用方负责 |
| `char*` (库申请) | `z_file_read_text` / `z_string_*` | `z_file_free` / `z_string_free` | 调用方负责 |
| `char**` (库申请) | `z_string_split` | `z_string_free_array` | 调用方负责 |

perf 模块的 `g_accum` / `g_pending` 是进程级全局状态，`z_perf_reset` 清空。

---

## 重要对象

### 全局状态（进程级，`src/core/`）

| 模块 | 全局对象 | 保护方式 |
|------|---------|---------|
| perf | `g_mutex` / `g_accum` / `g_pending` | `std::mutex` |
| log | `g_min_level` (atomic) / `g_file` / `g_file_mutex` | atomic + mutex |

### 关键内部类型

- `zbase::platform::FileStat`（`src/platform/file_platform.hpp`）：文件属性
- `zbase::platform` 函数：`NormalizePath` / `FileStatGet` / `MakeDirs` / `MonoNowNs` / `WallNowMs` / `SleepMs` / `Utf8ToUtf16` / `Utf16ToUtf8`
- `Accumulator` / `PendingStart`（`src/core/perf/perf.cpp` 匿名命名空间）
- `FileState`（`src/core/log/log.cpp` 匿名命名空间）
- `z_config_t`（`src/core/config/config.cpp`，实现 opaque 句柄）

---

## 线程模型

| 类别 | 约定 |
|------|------|
| 无状态函数（如 `z_string_split`） | 天然线程安全 |
| 只读全局表（错误码描述表） | 线程安全（编译期常量） |
| `log` 子系统 | `init`/`shutdown` 由调用方保证主线程单次；运行时 `z_log_write` 线程安全（atomic 级别 + mutex 文件） |
| `perf` 子系统 | `z_perf_start` / `z_perf_end` / `z_perf_dump` / `z_perf_reset` 线程安全（全局 mutex） |
| 句柄型对象（`z_dir_iter*` / `z_config_t*`） | 单句柄不可多线程并发使用；不同句柄之间互不影响 |

> **未确认**：v0.1 perf 的同名嵌套 start/end 用 `ref_count` 计数，`PendingStart` 注释标注"v0.1 不严格区分线程"。跨线程同名嵌套打点的语义未知，AI 修改时应保持现有行为。

---

## 插件关系

无插件系统。Unknown 是否计划引入。

---

## DLL / 共享库关系

### 单一动态库

- 库名 `zbase`，产物 `zbase.dll` / `libzbase.so` / `libzbase.dylib`
- 通过 `ZBASE_API` 宏控制符号导出（详见 [PROJECT.md](file:///c:/Repos/ZBase/AI/PROJECT.md) §已发现的重要事实）
- 构建库时定义 `ZBASE_EXPORTS`（`zbase_objects` PRIVATE + `zbase` PRIVATE 防御性）
- 调用方不定义 `ZBASE_EXPORTS`，头文件中 `ZBASE_API` 展开为 `dllimport`（Windows）或 `visibility default`（POSIX）
- 静态库模式定义 `ZBASE_STATIC_DEFINE`，`ZBASE_API` 展开为空

### CRT 一致性（Windows）

- `zbase_options.cmake` 强制 MSVC 运行时库一致：动态库→`MultiThreadedDLL`，静态库→`MultiThreaded`
- `CMAKE_MSVC_RUNTIME_LIBRARY` 被 CACHE FORCE 设定，避免 CRT 混用（致命风险）

### DLL 搜索路径（Windows）

- `z_log_init` 内部调 `SetDefaultDllDirectories` 限制 DLL 搜索路径（安全加固）

### RPATH

- macOS：`INSTALL_NAME_DIR=@rpath`、`MACOSX_RPATH=ON`
- Linux：`INSTALL_RPATH=$ORIGIN`、`BUILD_RPATH=$ORIGIN`

### 外部依赖使用

外部项目通过 CMake `find_package(ZBase 0.2 REQUIRED)` + `target_link_libraries(... ZBase::zbase)` 使用。Windows 下需 POST_BUILD 拷贝 `zbase.dll` 到 exe 目录。详见 [source/examples/use_as_dependency/](file:///c:/Repos/ZBase/source/examples/use_as_dependency/)。
