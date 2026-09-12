include(GoogleTest)

set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)

file(GLOB_RECURSE TEST_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*_Test.cpp)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../lib/googletest ${CMAKE_CURRENT_BINARY_DIR}/googletest EXCLUDE_FROM_ALL)

foreach(TEST_SOURCE ${TEST_SOURCES})
    file(RELATIVE_PATH TEST_NAME ${CMAKE_CURRENT_SOURCE_DIR} ${TEST_SOURCE})
    string(REPLACE ".cpp" "" TEST_NAME ${TEST_NAME})
    string(REPLACE "/" "." TEST_NAME ${TEST_NAME})

    add_executable(${TEST_NAME} ${TEST_SOURCE})

    target_include_directories(${TEST_NAME}
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}
    )

    target_compile_definitions(${TEST_NAME}
        PRIVATE
            ROCKET_SOURCE_DIR=\"${PROJECT_SOURCE_DIR}\"
            ROCKET_BINARY_DIR=\"${PROJECT_BINARY_DIR}\"
    )

    target_link_libraries(${TEST_NAME}
        PRIVATE
            Rocket::Rocket
            gmock_main
    )

    # PRE_TEST defers test enumeration to ctest time. The default (POST_BUILD)
    # launches every executable during the build, and each one spins up Metal
    # and Cocoa at static-init; with one binary per test file running under a
    # parallel build, some overran the 5s default and failed the whole build.
    gtest_discover_tests(${TEST_NAME}
        DISCOVERY_MODE PRE_TEST
        DISCOVERY_TIMEOUT 60
    )
endforeach()
