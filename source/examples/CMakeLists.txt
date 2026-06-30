# ZBase 示例
# 每个 .cpp 编译为独立可执行文件，链接 zbase
# 排除 use_as_dependency 目录，它是独立的 CMake 项目

file(GLOB_RECURSE ZBASE_EXAMPLE_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)
list(FILTER ZBASE_EXAMPLE_SOURCES EXCLUDE REGEX "${CMAKE_CURRENT_SOURCE_DIR}/use_as_dependency")

foreach(src ${ZBASE_EXAMPLE_SOURCES})
    get_filename_component(name ${src} NAME_WE)
    add_executable(${name} ${src})
    target_link_libraries(${name} PRIVATE zbase)
    # 让 demo.exe 与 zbase.dll 输出到同一目录（构建根），避免运行时 STATUS_DLL_NOT_FOUND
    set_target_properties(${name} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
    )
endforeach()
