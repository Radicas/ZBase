# ZBase 消毒剂选项
# 提供 zbase_enable_sanitizers(target) 函数
# 当前仅支持 MSVC（项目定位为 Windows + MSVC 优先）

option(ZBASE_ENABLE_ASAN "启用 AddressSanitizer 内存错误检测（仅 MSVC）" OFF)
option(ZBASE_ENABLE_ANALYZE "启用 MSVC 静态代码分析 /analyze" OFF)

# ASan 通过全局编译标志注入（MSVC 要求同一可执行文件内所有 .obj 一致，
# 包括 gtest 等第三方库也必须带 ASan，所以不能用 per-target 方式）
if(ZBASE_ENABLE_ASAN AND MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /fsanitize=address")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /fsanitize=address")
    message(STATUS "已全局启用 AddressSanitizer（/fsanitize=address）")
elseif(ZBASE_ENABLE_ASAN AND NOT MSVC)
    message(WARNING "AddressSanitizer 当前仅支持 MSVC，已跳过")
endif()

function(zbase_enable_sanitizers target)
    if(MSVC)
        if(ZBASE_ENABLE_ANALYZE)
            target_compile_options(${target} PRIVATE /analyze /analyze:autolog-)
            message(STATUS "已为目标 ${target} 启用静态分析 /analyze")
        endif()
    else()
        if(ZBASE_ENABLE_ANALYZE)
            message(WARNING "静态分析 /analyze 当前仅支持 MSVC，已跳过")
        endif()
    endif()
endfunction()
