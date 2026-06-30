# ZBase 跨平台编译选项
# 提供 zbase_apply_compile_options(target) 函数

function(zbase_apply_compile_options target)
    # 公共定义：控制 ZBASE_API 宏展开
    if(ZBASE_STATIC_DEFINE)
        target_compile_definitions(${target} PUBLIC ZBASE_STATIC_DEFINE)
    endif()

    if(MSVC)
        # /utf-8 设为 PRIVATE：避免强制依赖方继承（依赖方可能用 Clang/MinGW）
        # 依赖方如需 UTF-8，应在自己的 CMakeLists.txt 中设置
        target_compile_options(${target} PRIVATE /utf-8)
        # 严格警告级别 + 标准 conformant 模式 + 多处理器编译仅对 zbase 自己生效
        target_compile_options(${target} PRIVATE /W4 /permissive- /WX- /MP)
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        # 字符集约定 PRIVATE：避免强制依赖方继承
        target_compile_options(${target} PRIVATE
            -finput-charset=UTF-8 -fexec-charset=UTF-8
        )
        # 严格警告 + 默认隐藏符号仅对 zbase 自己生效
        target_compile_options(${target} PRIVATE
            -Wall -Wextra -Wpedantic
            -fvisibility=hidden -fvisibility-inlines-hidden
        )
        if(BUILD_SHARED_LIBS)
            set_target_properties(${target} PROPERTIES POSITION_INDEPENDENT_CODE ON)
        endif()
    endif()

    if(WIN32)
        target_compile_definitions(${target} PRIVATE
            WIN32_LEAN_AND_MEAN
            NOMINMAX
            _CRT_SECURE_NO_WARNINGS
            UNICODE
            _UNICODE
        )
    endif()
endfunction()
