# LunaSVG, the SVG renderer.

block()
    set(BUILD_SHARED_LIBS OFF)
    set(LUNASVG_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/lib/lunasvg)
endblock()
