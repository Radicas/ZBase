file(GLOB_RECURSE ZBASE_TEST_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

add_executable(zbase_tests ${ZBASE_TEST_SOURCES})

# 单测直接链接 zbase_objects，可访问未导出的内部符号（platform / core）
target_link_libraries(zbase_tests PRIVATE zbase_objects gtest gtest_main)

# ZBASE_STATIC_DEFINE 让 zbase.h 里的 ZBASE_API 在测试代码里展开为空，
# 避免 dllimport 引用未导出的内部符号
target_compile_definitions(zbase_tests PRIVATE ZBASE_STATIC_DEFINE)

target_include_directories(zbase_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

include(GoogleTest)
gtest_discover_tests(zbase_tests)
