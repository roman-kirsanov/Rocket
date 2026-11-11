# FreeType, the font rasterizer, with its optional dependencies left out.

block()
    set(BUILD_SHARED_LIBS OFF)
    set(FT_DISABLE_ZLIB ON CACHE BOOL "" FORCE)
    set(FT_DISABLE_BZIP2 ON CACHE BOOL "" FORCE)
    set(FT_DISABLE_PNG ON CACHE BOOL "" FORCE)
    set(FT_DISABLE_HARFBUZZ ON CACHE BOOL "" FORCE)
    set(FT_DISABLE_BROTLI ON CACHE BOOL "" FORCE)

    if(NOT PROJECT_IS_TOP_LEVEL)
        set(ROCKET_EXCLUDE EXCLUDE_FROM_ALL)
    endif()

    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/lib/freetype ${ROCKET_EXCLUDE})
endblock()
