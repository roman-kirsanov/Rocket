# The example applications in example/, built into <build>/example. Off unless
# ROCKET_EXAMPLE is on (./make.sh --example), and when Rocket is added to
# another project with add_subdirectory().

block()
    if(NOT ROCKET_EXAMPLE OR NOT PROJECT_IS_TOP_LEVEL)
        return()
    endif()

    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/example ${CMAKE_CURRENT_BINARY_DIR}/example)
endblock()
