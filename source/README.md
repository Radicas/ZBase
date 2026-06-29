# ZBase

> 朱鹏（Radica Zhu）的个人跨平台 C/C++ 通用工具动态库

一套覆盖日常开发高频场景的底层工具合集，编译为跨平台动态库，让所有项目告别重复造轮子。

---

## 特性

- **跨平台**：一套代码编译 Windows (dll)、Linux (so)、macOS (dylib)
- **纯 C 接口**：对外暴露稳定 C ABI，规避 C++ 名字改编，支持多语言 FFI 调用
- **零重度依赖**：核心模块无第三方依赖，开箱即用
- **统一规范**：统一命名前缀、统一错误码体系、统一内存管理约定
- **动态库发布**：库升级只需替换动态库文件，业务代码零改动

---

## 模块说明

| 模块 | 头文件 | 功能描述 |
|------|--------|---------|
| 文件 | `zbase/file.h` | 文件读写、目录遍历、路径处理、文件操作 |
| 字符串 | `zbase/string.h` | 分割、拼接、替换、编码转换、格式化 |
| 性能统计 | `zbase/perf.h` | 高精度计时、代码块打点、耗时汇总 |
| 日志 | `zbase/log.h` | 分级日志、控制台+滚动文件输出 |
| 时间 | `zbase/time.h` | 时间戳、格式化、跨平台休眠 |
| 错误码 | `zbase/error.h` | 全局统一错误码 + 错误描述 |

---

## 目录结构

```
zbase/
├── include/
│   └── zbase/          # 对外暴露的头文件（稳定接口）
├── src/                # 内部实现源码
│   ├── file/
│   ├── string/
│   ├── perf/
│   ├── log/
│   ├── time/
│   └── platform/       # 各平台差异化适配代码
│       ├── windows/
│       ├── linux/
│       └── macos/
├── cmake/              # CMake 工具脚本
├── tests/              # 单元测试
├── examples/           # 调用示例代码
├── docs/               # 接口文档
└── CMakeLists.txt
```

---

## 快速开始

### 编译

```bash
# 克隆项目
git clone https://github.com/yourname/zbase.git
cd zbase

# 创建构建目录
mkdir build && cd build

# CMake 配置
cmake .. -DCMAKE_BUILD_TYPE=Release

# 编译
cmake --build .
```

编译产物：
- Windows：`zbase.dll` + `zbase.lib`
- Linux：`libzbase.so`
- macOS：`libzbase.dylib`

### 使用

```c
#include <zbase/file.h>
#include <zbase/perf.h>
#include <zbase/log.h>

int main() {
    // 初始化日志
    z_log_init("app.log", Z_LOG_LEVEL_INFO);

    // 性能打点
    z_perf_start("read_file");

    // 读取文件
    char* content = NULL;
    int err = z_file_read_text("config.txt", &content);
    if (err != Z_OK) {
        Z_LOG_ERROR("读取文件失败: %s", z_error_msg(err));
        return -1;
    }

    z_perf_end("read_file");

    // 使用 content...

    // 释放（谁申请谁释放原则）
    z_string_free(content);

    // 打印性能统计
    z_perf_dump();

    return 0;
}
```

---

## 命名规范

- 库名：`zbase`（全小写，工程/库文件名）
- 正式名称：`ZBase`（大驼峰，文档/展示用）
- 接口前缀：`z_` 或 `zbase_`
- 头文件：`zbase/xxx.h`
- 错误码前缀：`Z_`

---

## 设计原则

1. **最小可用优先**：先做高频刚需功能，按需迭代，拒绝功能膨胀
2. **谁申请谁释放**：库内申请的内存必须由库内释放接口释放
3. **接口只增不改**：保证 ABI 向前兼容，旧版本程序可加载新版本库
4. **平台差异内部消化**：上层调用者无需关心 Windows/Linux/macOS 区别

---

## 版本规划

### v0.1（最小可用版）
- [x] 项目结构搭建
- [ ] 文件基础操作模块
- [ ] 字符串常用工具模块
- [ ] 高精度性能统计模块
- [ ] CMake 跨平台编译支持

### v0.2（完善基础）
- [ ] 日志模块
- [ ] 时间工具模块
- [ ] 统一错误码体系
- [ ] 单元测试框架

### v0.3（跨平台适配）
- [ ] Linux 平台适配
- [ ] macOS 平台适配
- [ ] CI 自动化多平台编译

---

## License

MIT
