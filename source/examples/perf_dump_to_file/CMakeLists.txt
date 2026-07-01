# ZBase perf 示例：用 z_perf_iterate 把性能统计 dump 到文件
add_executable(perf_dump_to_file main.cpp)
target_link_libraries(perf_dump_to_file PRIVATE zbase::zbase)

# 与 zbase.dll 输出到同一目录，避免运行时 STATUS_DLL_NOT_FOUND
set_target_properties(perf_dump_to_file PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)
