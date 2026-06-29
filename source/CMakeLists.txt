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

# 主库目标
add_library(zbase ${ZBASE_SOURCES})
add_library(zbase::zbase ALIAS zbase)

target_include_directories(zbase
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_features(zbase PUBLIC cxx_std_20)

# 应用编译选项
zbase_apply_compile_options(zbase)

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
    add_subdirectory(thirdparty/googletest-1.17.0)
    add_subdirectory(tests)
endif()

# 示例
if(ZBASE_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()
