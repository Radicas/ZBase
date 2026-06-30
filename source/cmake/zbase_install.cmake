# ZBase install 规则
# 详见 architecture.md §14.2

include(GNUInstallDirs)

function(zbase_setup_install target)
    # 头文件（排除 internal/，详见 architecture.md §14.2）
    install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/
            DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
            FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
            PATTERN "internal" EXCLUDE)

    # 库 + 运行时
    install(TARGETS ${target}
        EXPORT ZBaseTargets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )

    install(EXPORT ZBaseTargets
        FILE ZBaseTargets.cmake
        NAMESPACE ZBase::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ZBase
    )

    include(CMakePackageConfigHelpers)
    write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/ZBaseConfigVersion.cmake"
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion
    )
    configure_package_config_file(
        "${CMAKE_SOURCE_DIR}/cmake/ZBaseConfig.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/ZBaseConfig.cmake"
        INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ZBase
    )
    install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/ZBaseConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/ZBaseConfigVersion.cmake"
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ZBase
    )

    # RPATH
    if(APPLE)
        set_target_properties(${target} PROPERTIES
            INSTALL_NAME_DIR "@rpath"
            MACOSX_RPATH ON
        )
    elseif(UNIX AND NOT APPLE)
        set_target_properties(${target} PROPERTIES
            INSTALL_RPATH "$ORIGIN"
            BUILD_RPATH "$ORIGIN"
        )
    endif()
endfunction()
