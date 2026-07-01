# ZBase 综合示例：文件读写 + 字符串处理 + perf 计时 + log 输出
add_executable(file_string_demo main.cpp)
target_link_libraries(file_string_demo PRIVATE zbase::zbase)

# 与 zbase.dll 输出到同一目录，避免运行时 STATUS_DLL_NOT_FOUND
set_target_properties(file_string_demo PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)
