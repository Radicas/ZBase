# ZBase 外部项目使用示例

本文档演示如何在独立的 CMake 项目中使用 ZBase 作为依赖。

## 前置条件

1. 已编译 ZBase（见主项目 `README.md`）
2. 将 ZBase 安装到某个目录（安装前缀）

## 步骤

### 第一步：安装 ZBase

在 ZBase 项目中执行：

```bash
# Windows（PowerShell）
cmake -S source -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
cmake --install build --prefix C:/opt/zbase

# Linux/macOS
cmake -S source -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
sudo cmake --install build --prefix /usr/local
```

安装后目录结构：

```
C:/opt/zbase/
├── include/
│   ├── zbase/          # C ABI 头文件
│   │   ├── zbase.h
│   │   ├── file.h
│   │   ├── string.h
│   │   ├── log.h
│   │   ├── time.h
│   │   ├── perf.h
│   │   ├── error.h
│   │   └── config.h
│   └── zbase++/        # C++ 包装头文件（header-only）
│       ├── file.hpp
│       ├── string.hpp
│       ├── log.hpp
│       ├── time.hpp
│       ├── perf.hpp
│       ├── error.hpp
│       └── config.hpp
├── lib/
│   ├── zbase.lib       # 导入库（Windows）/ 静态库
│   └── cmake/ZBase/    # CMake 配置文件
│       ├── ZBaseConfig.cmake
│       ├── ZBaseConfigVersion.cmake
│       └── ZBaseTargets.cmake
└── bin/
    └── zbase.dll       # 动态库（Windows）
```

### 第二步：构建本示例

```bash
# Windows（PowerShell）
cd examples/use_as_dependency
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=C:/opt/zbase
cmake --build build --config Release
build/zbase_example.exe

# Linux/macOS
cd examples/use_as_dependency
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/zbase_example
```

### 第三步：理解 CMake 配置

本示例的 `CMakeLists.txt` 关键部分：

```cmake
# 查找已安装的 ZBase
find_package(ZBase 0.2 REQUIRED)

# 链接 ZBase::zbase target（自动包含头文件路径）
target_link_libraries(zbase_example PRIVATE ZBase::zbase)
```

## 使用方式对比

### C ABI（稳定接口）

适合：跨语言绑定、需要稳定 ABI、嵌入到 C 项目

```cpp
#include "zbase/file.h"
#include "zbase/string.h"

// 文件操作
int r = z_file_write_text("test.txt", "content", 7);
if (r != Z_OK) { /* 错误处理 */ }

// 字符串操作
char** parts = nullptr;
size_t count = 0;
z_string_split("a,b,c", 5, ",", &parts, &count);
z_string_free_array(parts, count);
```

### C++ 包装（header-only RAII）

适合：C++ 项目、追求简洁代码、异常处理

```cpp
#include "zbase++/file.hpp"
#include "zbase++/string.hpp"

// 文件操作（RAII，自动释放）
zbase::WriteFile("test.txt", "content");
zbase::FileReader reader("test.txt");  // 析构自动释放

// 字符串操作（返回 std::vector<std::string>）
auto parts = zbase::String::Split("a,b,c", ",");
```

## 注意事项

### Windows DLL 路径

- `find_package` 会自动处理头文件和链接库路径
- 运行时需要确保 `zbase.dll` 在以下位置之一：
  1. 与 exe 同目录（本示例的 CMakeLists.txt 已添加 post-build 拷贝）
  2. 在 `PATH` 环境变量中

### 编译选项

- ZBase 的 `ZBase::zbase` target 会自动传播 `/utf-8`（MSVC）或 `-finput-charset=UTF-8`（GCC/Clang）编译选项
- 外部项目无需额外配置字符集

### 版本兼容性

- `ZBaseConfigVersion.cmake` 配置为 `SameMajorVersion`，即 0.x 版本之间兼容
- `find_package(ZBase 0.2 REQUIRED)` 会接受 0.2.x 的任何版本

## 替代方式：FetchContent

如果不想手动 install，可以用 CMake 的 `FetchContent` 直接拉取源码编译：

```cmake
include(FetchContent)

FetchContent_Declare(
  zbase
  GIT_REPOSITORY https://github.com/your-account/ZBase.git
  GIT_TAG        v0.2.0
)
set(ZBASE_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ZBASE_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(zbase)

target_link_libraries(your_app PRIVATE zbase::zbase)
```
