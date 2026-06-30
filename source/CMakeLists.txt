cmake_minimum_required(VERSION 3.20)

project(zbase
    VERSION 0.1.0
    DESCRIPTION "ZBase 个人跨平台 C/C++ 通用工具动态库"
    LANGUAGES C CXX
)

# C++20 标准（不用 23 是为了 MinGW 兼容性）
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# 默认构建为动态库，可由 -DBUILD_SHARED_LIBS=OFF 切换为静态
option(BUILD_SHARED_LIBS "构建为动态库（OFF 则静态）" ON)
option(ZBASE_BUILD_TESTS "构建单元测试" ON)
option(ZBASE_BUILD_EXAMPLES "构建示例" ON)

# 加载各模块
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(zbase_options)
include(zbase_compile_options)

# 源码查找：禁止硬编码，必须用 file(GLOB_RECURSE)
file(GLOB_RECURSE ZBASE_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/platform/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/core/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/c_api/*.cpp"
)
file(GLOB_RECURSE ZBASE_PUBLIC_HEADERS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/include/zbase/*.h"
)

# 主库目标：OBJECT 库模式（架构 §12 默认隐藏 + 显式导出）
# zbase_objects 编译一次，被 zbase（共享/静态库）和 zbase_tests（单测）共用
add_library(zbase_objects OBJECT ${ZBASE_SOURCES})

target_include_directories(zbase_objects
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_features(zbase_objects PUBLIC cxx_std_20)

# 应用编译选项
zbase_apply_compile_options(zbase_objects)

# 编译 zbase 源文件时定义 ZBASE_EXPORTS，让 ZBASE_API 在头文件里展开为 dllexport
# （OBJECT 库的 .obj 同时被 zbase.dll 链接和测试 .exe 链接；ZBASE_EXPORTS 只影响声明）
if(BUILD_SHARED_LIBS)
    target_compile_definitions(zbase_objects PRIVATE ZBASE_EXPORTS)
endif()

# 包装为最终库目标（共享或静态）
if(BUILD_SHARED_LIBS)
    add_library(zbase SHARED $<TARGET_OBJECTS:zbase_objects>)
else()
    add_library(zbase STATIC $<TARGET_OBJECTS:zbase_objects>)
endif()
add_library(zbase::zbase ALIAS zbase)

# 共享/静态库需要继承 include 路径与编译特性（仅接口属性，源已由 OBJECT 编译）
target_include_directories(zbase
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)
target_compile_features(zbase PUBLIC cxx_std_20)
# 让 zbase 消费者（如 demo.exe）继承 /utf-8 等公共编译选项
# OBJECT 库的 INTERFACE 属性不会自动传播到 $<TARGET_OBJECTS:..> 消费者
zbase_apply_compile_options(zbase)

# 动态库构建时也给 zbase 目标本身加 ZBASE_EXPORTS（防御性，避免某些 IDE 重新编译时漏掉）
if(BUILD_SHARED_LIBS)
    target_compile_definitions(zbase PRIVATE ZBASE_EXPORTS)
endif()

# 版本与 soname
set_target_properties(zbase PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 0
)

# install 规则
include(zbase_install)
zbase_setup_install(zbase)

# 测试
if(ZBASE_BUILD_TESTS)
    enable_testing()
    # GoogleTest 强制静态：phase-1-working-file.md §4.5 要求"测试用静态链接 gtest"。
    # BUILD_SHARED_LIBS=ON 会被 GoogleTest 继承导致编译为 gtest.dll，运行时找不到符号。
    # 局部保存/恢复，仅影响 googletest 子工程。
    set(_zbase_prev_build_shared ${BUILD_SHARED_LIBS})
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
    set(gtest_force_shared_crt OFF CACHE BOOL "" FORCE)
    add_subdirectory(thirdparty/googletest-1.17.0 EXCLUDE_FROM_ALL)
    set(BUILD_SHARED_LIBS ${_zbase_prev_build_shared} CACHE BOOL "" FORCE)
    add_subdirectory(tests)
endif()

# 示例
if(ZBASE_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()
