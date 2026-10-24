# Installs the library, its headers and a CMake package, so another project
# can use find_package(Rocket) and link Rocket::Rocket.

block()
    install(TARGETS Rocket yogacore
        EXPORT RocketTargets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )

    install(DIRECTORY src/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        FILES_MATCHING
            PATTERN "*.hpp"
            PATTERN "*.h"
    )

    install(EXPORT RocketTargets
        NAMESPACE Rocket::
        FILE RocketTargets.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Rocket
    )

    write_basic_package_version_file(
        ${CMAKE_CURRENT_BINARY_DIR}/RocketConfigVersion.cmake
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion
    )

    configure_package_config_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/RocketConfig.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/RocketConfig.cmake
        INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Rocket
    )

    install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/RocketConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/RocketConfigVersion.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Rocket
    )
endblock()
