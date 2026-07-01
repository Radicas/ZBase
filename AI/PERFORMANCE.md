# PERFORMANCE

> 性能敏感模块与高频调用识别。基于代码与文档观察，未做实测 profiling。

---

## 性能敏感模块

### 1. log 模块（`src/core/log/log.cpp`）

**敏感性：高**（高频调用、持锁 IO）

- `z_log_write` 每次调用都：
  - 原子读 `g_min_level` 过滤
  - `z_time_now_ms` + `z_time_format_iso` 生成时间戳
  - `vsnprintf` 格式化
  - `fprintf(stderr)` + `fflush`（控制台同步刷盘）
  - 持 `g_file_mutex` 写文件 + `flush`（每次写后 flush，防崩溃丢日志）
- **锁粒度**：`g_file_mutex` 串行化所有文件写入，跨线程争用
- **每次 flush**：简化策略，保证崩溃不丢日志，但吞吐受限
- **滚动开销**：`RollFiles()` 持锁倒序 rename N 个文件

**优化空间**（未实现，YAGNI）：
- 异步刷盘（双缓冲 + 后台线程）—— v0.2 显式不做
- 批量写入

### 2. perf 模块（`src/core/perf/perf.cpp`）

**敏感性：中**

- `z_perf_start` / `z_perf_end` 每次都：
  - 持 `g_mutex`
  - `unordered_map` 查找 / 插入（`std::string` key，含哈希 + 比较）
  - `zbase::platform::MonoNowNs`（Windows `QueryPerformanceCounter`，POSIX `clock_gettime`）
- **锁粒度**：全局 `g_mutex`，跨线程争用
- **key 为 std::string**：每次 start/end 可能触发字符串构造/哈希

**高频使用注意**：在热路径中频繁 start/end 同名打点会累积锁竞争。

### 3. string 模块（`src/core/string/string.cpp`）

**敏感性：中**

- `z_string_split` / `z_string_join` / `z_string_replace`：
  - 内部用 `std::string` + `std::vector`
  - 出参通过 `std::malloc` 分配（C ABI 边界）
  - `DupString` 每次结果都 malloc + memcpy
- `z_string_replace` 用 `std::string::replace` 循环，O(n·m) 最坏
- `z_string_utf8_len`（static inline，纯算法，无导出）—— 高频安全

### 4. file 模块（`src/core/file/file_reader.cpp`）

**敏感性：中**

- `z_file_read_text`：
  - 先 `FileStatGet` 查属性（Windows `GetFileAttributesW`，POSIX `stat`）
  - 再 `CreateFileW` / `open` 打开
  - 单次 `ReadFile` / 循环 `read` 读全部
  - `std::malloc` 缓冲区
- `z_file_write_text`：临时文件 + rename 原子写（双倍 IO）
- **IO 密集**，无缓存层

### 5. encoding 模块（`src/platform/encoding.cpp`）

**敏感性：中**（Windows 路径转换高频）

- `NormalizePath`（`file_platform.hpp`）：Windows 下 UTF-8 → UTF-16 + `\\?\` 长路径前缀
- 每次 file / log 文件操作都需转换
- Windows 走 `MultiByteToWideChar` / `WideCharToMultiByte`
- POSIX 走纯算法（RFC 3629）

---

## 大对象

| 对象 | 位置 | 大小 | 说明 |
|------|------|------|------|
| `g_accum` | `perf.cpp` | 进程级 `unordered_map<string, Accumulator>` | 打点累计表，按 name 累加 |
| `g_pending` | `perf.cpp` | 进程级 `unordered_map<string, PendingStart>` | 进行中打点 |
| `g_file.stream` | `log.cpp` | `std::ofstream` | 日志文件流 |
| `z_config_t` 内部 map | `config.cpp` | `std::map<pair<string,string>, string>` | 配置键值表 |
| `z_dir_iter` 内部状态 | `file/dir_iter.cpp` | Unknown（未读源码） | 目录遍历句柄 |
| 文件读取缓冲 | `file_reader.cpp` | 调用方传入大小 | `std::malloc(st.size)` |

---

## 高频调用

| 调用点 | 频次 | 瓶颈 |
|--------|------|------|
| `Z_LOG_*` 宏 → `z_log_write` | 极高（业务每条日志） | 持锁 + fflush + 文件 flush |
| `z_perf_start` / `z_perf_end` | 高（性能打点） | 持锁 + map 查找 |
| `zbase::platform::NormalizePath`（Windows） | 高（每次文件/日志路径） | UTF-8→UTF-16 转换 |
| `z_time_now_ms` / `z_time_format_iso` | 高（每条日志） | 系统调用 + localtime + strftime |
| `z_string_split` / `z_string_join` | 中（字符串处理） | malloc + vector |
| `z_file_read_text` | 低（启动配置读取） | IO |

---

## IO

- **文件 IO**：`file` 模块，Windows 走 Win32 `CreateFileW`/`ReadFile`/`FindFirstFileW`，POSIX 走 `open`/`read`/`opendir`
- **控制台 IO**：`log` 模块 `fprintf(stderr)` + `fflush`，`perf` 模块 `fprintf(stderr)`
- **无缓冲层**：file 模块无 read 缓存；log 模块每次写后 flush
- **原子写**：`z_file_write_text` 用临时文件 + rename（双倍 IO，换取崩溃安全）

---

## 内存分配

- **C ABI 出参**：`std::malloc` / `std::calloc`（`z_file_read_text`、`z_string_split`、`z_string_join` 等）
- **内部容器**：`std::string`、`std::vector`、`std::unordered_map`、`std::map`
- **日志行拼接**：`z_log_write` 每次用 `std::string line_str` + `reserve` + 多次 `+=`
- **无内存池**：所有分配走默认 allocator
- **无 arena**：Unknown 是否计划引入

---

## 多线程

- **log 文件写入**：`g_file_mutex` 串行化（粒度大，跨线程争用）
- **perf 全表**：`g_mutex` 串行化（start/end/dump/reset 全部互斥）
- **log 级别过滤**：`g_min_level` 用 `std::atomic<int>` + `memory_order_relaxed`（无锁快路径）
- **无读写锁**：全部用 `std::mutex`，无 `shared_mutex`
- **无锁-free 数据结构**：Unknown

---

## 锁

| 锁 | 位置 | 保护对象 | 粒度 |
|----|------|---------|------|
| `g_mutex` | `perf.cpp` | `g_accum` + `g_pending` | 全表 |
| `g_file_mutex` | `log.cpp` | `g_file`（文件流 + 状态 + 滚动） | 单文件输出 |
| CRT 内部锁 | stderr 输出 | `fprintf` | 行级 |

**潜在竞争点**：
- `z_perf_dump` 持 `g_mutex` 期间做 `std::sort` + `fprintf`，阻塞所有 start/end
- `RollFiles` 持 `g_file_mutex` 期间做 N 次 rename + reopen，阻塞所有日志写入

---

## Cache

- **无应用层缓存**：file 模块无 read cache，config 模块无热加载缓存
- **CPU cache 友好性**：Unknown（未做 profiling）
- **错误码描述表**：编译期常量（`static const`），可能命中只读段缓存

---

## 已知性能基准（v0.2）

基准位于 `source/benchmarks/`，需 `ZBASE_BUILD_BENCHMARKS=ON` 启用：

| 基准 | 文件 | 覆盖操作 |
|------|------|---------|
| `bench_string` | `bench_string.cpp` | split / join / replace / trim（多规模 128/1024/8192） |
| `bench_file` | `bench_file.cpp` | read_text / write_text |
| `bench_log` | `bench_log.cpp` | `z_log_write` 吞吐 |
| `bench_config` | `bench_config.cpp` | load + get |

共 4 个基准、21 项测试。基线数据 Unknown（未在文档中记录具体数值）。

---

## 未确认项

- perf 模块跨线程同名嵌套打点的语义与正确性
- log 模块在极端高并发下的吞吐上限
- file 模块在大量小文件场景的性能特征
- config 模块加载大配置文件（万行级）的耗时
- Windows Defender 实时扫描对文件 IO 的实际影响
