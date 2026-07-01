# AI 目录

> 本目录是 **专门给 AI Agent 阅读的项目知识库**，不是用户文档。
> 所有 AI（Claude Code、Cursor、ChatGPT、Gemini、Codex、Cline 等）在修改 ZBase 代码前应优先阅读此目录。

---

## 用途

建立一套稳定的 AI 开发上下文，使 AI 在开始修改代码之前能够：

1. 快速理解项目目标、架构与约束
2. 遵守既有编码规范与设计决策
3. 减少错误修改与重复造轮子
4. 保持与项目长期维护方向一致

---

## 阅读顺序

| 顺序 | 文档 | 用途 |
|------|------|------|
| 1 | [PROJECT.md](file:///c:/Repos/ZBase/AI/PROJECT.md) | 项目目标、技术栈、目录结构、重要事实 |
| 2 | [ARCHITECTURE.md](file:///c:/Repos/ZBase/AI/ARCHITECTURE.md) | 五层架构、模块依赖、数据流、线程模型 |
| 3 | [CODING_RULES.md](file:///c:/Repos/ZBase/AI/CODING_RULES.md) | 从既有代码总结的编码风格与约束 |
| 4 | [BUILD.md](file:///c:/Repos/ZBase/AI/BUILD.md) | 编译、运行、调试、测试方法 |
| 5 | [PERFORMANCE.md](file:///c:/Repos/ZBase/AI/PERFORMANCE.md) | 性能敏感模块与高频调用识别 |
| 6 | [TESTING.md](file:///c:/Repos/ZBase/AI/TESTING.md) | 测试框架与执行方式 |
| 7 | [MEMORY.md](file:///c:/Repos/ZBase/AI/MEMORY.md) | 长期经验积累（初始为空） |
| 8 | [TASK_TEMPLATE.md](file:///c:/Repos/ZBase/AI/TASK_TEMPLATE.md) | 执行具体任务时的模板 |

---

## 阅读原则

- **不应修改未知模块**：在不理解模块依赖与所有权约定前，禁止改动。
- **不应猜测设计意图**：无法确认的事实一律标注 `Unknown`，不编造。
- **事实优先**：本目录所有内容必须基于项目实际代码与文档生成。
- **不修改业务代码**：AI 在仅被要求"建立上下文"时，不得修改任何源代码。
- **保持长期可维护**：本目录随项目演进持续更新，过时内容应及时修订。

---

## 与其他文档的关系

| 文档 | 位置 | 说明 |
|------|------|------|
| 用户 README | [source/README.md](file:///c:/Repos/ZBase/source/README.md) | 面向用户的项目介绍 |
| 架构总览 | [source/docs/architecture.md](file:///c:/Repos/ZBase/source/docs/architecture.md) | 完整架构文档（人类阅读） |
| 编码规则 | [.trae/rules/code-rule.md](file:///c:/Repos/ZBase/.trae/rules/code-rule.md) | 工作区强制规则 |
| Git 提交规则 | [.trae/rules/git-commit-message.md](file:///c:/Repos/ZBase/.trae/rules/git-commit-message.md) | 提交信息风格 |
| 阶段归档 | [source/docs/phase-1-done.md](file:///c:/Repos/ZBase/source/docs/phase-1-done.md)、[phase-2-done.md](file:///c:/Repos/ZBase/source/docs/phase-2-done.md) | 历史版本交付记录 |

AI 文档与上述文档如有冲突，以源代码与 `source/docs/architecture.md` 为准。
