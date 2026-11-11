# LunaSVG, the SVG renderer.

block()
    set(BUILD_SHARED_LIBS OFF)
    set(LUNASVG_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

    if(NOT PROJECT_IS_TOP_LEVEL)
        set(ROCKET_EXCLUDE EXCLUDE_FROM_ALL)
    endif()

    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/lib/lunasvg ${ROCKET_EXCLUDE})
endblock()
