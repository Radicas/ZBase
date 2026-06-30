cmake_minimum_required(VERSION 3.20)

project(zbase-use-as-dependency
    VERSION 1.0.0
    DESCRIPTION "示例：如何在外部项目中使用 ZBase 作为依赖"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 告诉 CMake 去哪找 ZBase 的安装目录
# - Windows: CMAKE_PREFIX_PATH 需要包含 ZBase 的 install 前缀（如 C:/opt/zbase）
# - Linux/macOS: 默认会搜索标准路径，也可以通过 -DCMAKE_PREFIX_PATH 指定
#
# 构建前确保已安装 ZBase：
#   cmake --install <zbase_build_dir> --prefix <install_prefix>
find_package(ZBase 0.2 REQUIRED)

add_executable(zbase_example)
target_sources(zbase_example PRIVATE main.cpp)

# 链接 ZBase::zbase target（自动继承 include 路径和编译选项如 /utf-8）
target_link_libraries(zbase_example PRIVATE ZBase::zbase)

# Windows 下需要把 zbase.dll 拷贝到 exe 目录
if(WIN32)
    add_custom_command(TARGET zbase_example POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:ZBase::zbase>
            $<TARGET_FILE_DIR:zbase_example>
        COMMENT "拷贝 zbase.dll 到输出目录"
    )
endif()

# 运行时 UTF-8 支持（与 ZBase 一致）
if(MSVC)
    target_compile_options(zbase_example PRIVATE /utf-8)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(zbase_example PRIVATE
        -finput-charset=UTF-8 -fexec-charset=UTF-8
    )
endif()
