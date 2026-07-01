# TASK_TEMPLATE

> 供 AI 执行具体任务时使用的模板。复制本文件内容作为任务起始上下文。

---

## Goal

<!-- 一句话描述本次任务的目标。例：为 log 模块增加按大小+按时间双维度滚动 -->

---

## Constraints

<!-- 列出必须遵守的约束。至少检查以下各项： -->

- [ ] 遵守 [CODING_RULES.md](file:///c:/Repos/ZBase/AI/CODING_RULES.md) 全部强制规则
- [ ] 不破坏 C ABI 兼容性（错误码只增不改、函数签名不破坏、导出符号 ≤ 40）
- [ ] 内存所有权遵守"库申请库释放"（详见 [ARCHITECTURE.md](file:///c:/Repos/ZBase/AI/ARCHITECTURE.md) §生命周期）
- [ ] C ABI 函数用 `ZBASE_C_API_BEGIN` / `ZBASE_C_API_END` 包裹
- [ ] 跨平台：Windows (MSVC + MinGW) / Linux / macOS 均能编译
- [ ] 字符编码：对外 UTF-8，Windows 内部转 UTF-16 调 W 版 API
- [ ] 文件名小写、doxygen 中文注释、include 用绝对路径
- [ ] CMake 用 `file(GLOB_RECURSE)` 查找源码，不硬编码
- [ ] 不引入新第三方依赖（核心库零重度依赖）
- [ ] YAGNI：不实现"未来可能用到"的功能

<!-- 补充本任务特有约束： -->

- 

---

## Files

<!-- 列出预计会修改/新建的文件。先读后改。 -->

| 文件 | 操作 | 说明 |
|------|------|------|
| `source/include/zbase/xxx.h` | 修改/新建 | C ABI 头 |
| `source/include/zbase++/xxx.hpp` | 修改/新建 | C++ 包装头 |
| `source/src/core/xxx/xxx.cpp` | 修改/新建 | 实现 |
| `source/src/platform/xxx.hpp` | 修改/新建 | 平台原语（如需） |
| `source/tests/core/test_xxx.cpp` | 修改/新建 | 单元测试 |
| `source/benchmarks/bench_xxx.cpp` | 新建（可选） | 性能基准 |
| `source/CMakeLists.txt` | 一般不改 | 源码查找用 GLOB，新文件自动纳入 |
| `source/docs/architecture.md` | 修改（如架构变更） | 同步架构文档 |

---

## Risks

<!-- 列出本任务的风险与对策。 -->

| 风险 | 影响 | 严重度 | 对策 |
|------|------|--------|------|
| 例：新增导出符号超出 40 上限 | CI 符号审计失败 | 高 | 合并接口或用句柄型 API |
| 例：Windows 中文路径乱码 | 中文场景不可用 | 高 | 走 `NormalizePath` + W 版 API |
| 例：跨堆 free | 崩溃 | 高 | 库内分配的内存必须走 `z_*_free` |
| 例：异常逃逸 C 边界 | UB | 高 | 用 `ZBASE_C_API_BEGIN/END` |
| | | | |

---

## Validation

<!-- 定义"完成"的判定标准。必须可验证。 -->

- [ ] 本地构建通过（MSVC 或 MinGW）
  ```bash
  cmake -S source -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
  cmake --build build --config Release
  ```
- [ ] 全部测试通过
  ```bash
  cd build && ctest --output-on-failure
  ```
- [ ] 新增功能有对应单元测试
- [ ] 导出符号审计通过（`nm` / `dumpbin`，全部以 `z_` 开头，≤ 40）
- [ ] 中文路径/中文日志不乱码
- [ ] 跨平台编译验证（至少 MSVC + 一个 POSIX）
- [ ] doxygen 文件头 + 函数注释齐全
- [ ] include 路径为绝对路径，无 `../`
- [ ] CMakeLists 未硬编码源文件
- [ ] 文档同步更新（architecture.md / phase-x-working-file.md）

---

## Notes

<!-- 任务执行过程中的备忘、决策记录、待确认问题。 -->

### 决策记录

| 时间 | 决策 | 理由 |
|------|------|------|
| | | |

### 待确认问题

<!-- 任何模糊点先列在这里让用户确认，不要自行猜测。参考工作区规则"执行命令时，有任何模糊的地方，需要列举问题让用户确认"。 -->

- [ ] 

### 参考文档

- [PROJECT.md](file:///c:/Repos/ZBase/AI/PROJECT.md)
- [ARCHITECTURE.md](file:///c:/Repos/ZBase/AI/ARCHITECTURE.md)
- [CODING_RULES.md](file:///c:/Repos/ZBase/AI/CODING_RULES.md)
- [BUILD.md](file:///c:/Repos/ZBase/AI/BUILD.md)
- [TESTING.md](file:///c:/Repos/ZBase/AI/TESTING.md)
- [PERFORMANCE.md](file:///c:/Repos/ZBase/AI/PERFORMANCE.md)
- [source/docs/architecture.md](file:///c:/Repos/ZBase/source/docs/architecture.md)
- [.trae/rules/code-rule.md](file:///c:/Repos/ZBase/.trae/rules/code-rule.md)
