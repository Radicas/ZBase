# 性能基准模块
# 每个基准独立可执行文件，链接 zbase + benchmark::benchmark

file(GLOB_RECURSE ZBASE_BENCH_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

foreach(src ${ZBASE_BENCH_SOURCES})
    # 以文件名（去扩展名）作为可执行名
    get_filename_component(bench_name ${src} NAME_WE)
    add_executable(${bench_name} ${src})
    target_link_libraries(${bench_name} PRIVATE
        zbase::zbase
        benchmark::benchmark
        benchmark::benchmark_main
    )
    # 不在 ALL 中默认构建（不影响主构建），但被 ctest -R Benchmark 显式调用时构建
    set_target_properties(${bench_name} PROPERTIES EXCLUDE_FROM_ALL ON)
endforeach()
