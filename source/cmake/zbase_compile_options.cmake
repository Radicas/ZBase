# ZBase 跨平台编译选项
# 提供 zbase_apply_compile_options(target) 函数

function(zbase_apply_compile_options target)
    # 公共定义：控制 ZBASE_API 宏展开
    if(ZBASE_STATIC_DEFINE)
        target_compile_definitions(${target} PUBLIC ZBASE_STATIC_DEFINE)
    endif()

    if(MSVC)
        # /utf-8 同时设源码和执行字符集
        target_compile_options(${target} PRIVATE /utf-8 /W4 /permissive- /WX-)
        # 多处理器编译加速
        target_compile_options(${target} PRIVATE /MP)
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(${target} PRIVATE
            -Wall -Wextra -Wpedantic
            -finput-charset=UTF-8 -fexec-charset=UTF-8
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
