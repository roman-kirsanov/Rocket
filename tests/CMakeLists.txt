include(GoogleTest)

set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)

# MSVC: googletest otherwise links the static runtime (/MT) while Rocket and
# the tests use the default DLL one (/MD), and the link fails.
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

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
            SDL3::SDL3-static
            gmock_main
    )

    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        # The snapshot tests read and write their PNGs with SDL_image.
        target_link_libraries(${TEST_NAME}
            PRIVATE
                SDL3_image::SDL3_image-static
        )
    elseif(WIN32)
        # The snapshot tests read and write their PNGs with WIC.
        target_sources(${TEST_NAME}
            PRIVATE
                ${CMAKE_CURRENT_SOURCE_DIR}/_Snapshots/Snapshot_Win32.cpp
        )

        target_link_libraries(${TEST_NAME}
            PRIVATE
                d3d12
                dxgi
                ole32
                windowscodecs
        )
    endif()

    if(WIN32)
        # One ctest test per file, not per TEST: on Windows every process
        # creates a Direct3D 12 device and compiles its pipelines (on WARP, in
        # software where there is no GPU), which costs far more than the tests
        # in it, so a file runs as one. A stuck file fails after two minutes.
        add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
        set_tests_properties(${TEST_NAME} PROPERTIES TIMEOUT 120)
    else()
        # PRE_TEST defers test enumeration to ctest time. The default
        # (POST_BUILD) launches every executable during the build, and each one
        # spins up Metal and Cocoa at static-init; with one binary per test file
        # running under a parallel build, some overran the 5s default and
        # failed the whole build.
        gtest_discover_tests(${TEST_NAME}
            DISCOVERY_MODE PRE_TEST
            DISCOVERY_TIMEOUT 60
        )
    endif()
endforeach()
