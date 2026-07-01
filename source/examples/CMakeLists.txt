# ZBase 示例
# 每个示例独立一个子目录，目录名即示例名
# use_as_dependency 是独立 CMake 项目，不在此构建

file(GLOB ZBASE_EXAMPLE_DIRS LIST_DIRECTORIES true CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*"
)

foreach(dir ${ZBASE_EXAMPLE_DIRS})
    get_filename_component(name ${dir} NAME)
    # 跳过独立项目和非目录
    if(name STREQUAL "use_as_dependency")
        continue()
    endif()
    if(NOT IS_DIRECTORY "${dir}")
        continue()
    endif()
    if(NOT EXISTS "${dir}/CMakeLists.txt")
        continue()
    endif()
    add_subdirectory(${name})
endforeach()
