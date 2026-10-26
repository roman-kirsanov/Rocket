# SDL_image, the Linux backend's image decoder, built on every platform so an
# app can read and write image files on the CPU. It decodes PNG and JPEG with
# its built-in stb_image everywhere, not ImageIO or WIC, so no other image
# library is needed and every platform decodes the same way.

block()
    set(BUILD_SHARED_LIBS OFF)
    set(SDLIMAGE_INSTALL ON CACHE BOOL "" FORCE)
    set(SDLIMAGE_VENDORED OFF CACHE BOOL "" FORCE)
    set(SDLIMAGE_SAMPLES OFF CACHE BOOL "" FORCE)
    set(SDLIMAGE_BACKEND_STB ON CACHE BOOL "" FORCE)
    set(SDLIMAGE_BACKEND_IMAGEIO OFF CACHE BOOL "" FORCE)
    set(SDLIMAGE_BACKEND_WIC OFF CACHE BOOL "" FORCE)
    set(SDLIMAGE_PNG_LIBPNG OFF CACHE BOOL "" FORCE)
    set(SDLIMAGE_AVIF OFF CACHE BOOL "" FORCE)
    set(SDLIMAGE_JXL OFF CACHE BOOL "" FORCE)
    set(SDLIMAGE_TIF OFF CACHE BOOL "" FORCE)
    set(SDLIMAGE_WEBP OFF CACHE BOOL "" FORCE)
    set(SDLIMAGE_SVG OFF CACHE BOOL "" FORCE)

    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/lib/sdl_image)
endblock()
