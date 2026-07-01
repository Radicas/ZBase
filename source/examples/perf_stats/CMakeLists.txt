# ZBase perf 示例：统计多个函数的调用次数与总耗时
add_executable(perf_stats main.cpp)
target_link_libraries(perf_stats PRIVATE zbase::zbase)

# 与 zbase.dll 输出到同一目录，避免运行时 STATUS_DLL_NOT_FOUND
set_target_properties(perf_stats PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)
