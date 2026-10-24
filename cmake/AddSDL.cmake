# SDL 3, built from its submodule and linked statically. Gamepads are read
# through SDL on every platform; Linux uses all of it as the backend, the
# other platforms build only its gamepad part.

block()
    set(BUILD_SHARED_LIBS OFF)
    set(SDL_SHARED OFF CACHE BOOL "" FORCE)
    set(SDL_STATIC ON CACHE BOOL "" FORCE)
    set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
    set(SDL_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(SDL_INSTALL ON CACHE BOOL "" FORCE)

    if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
        set(SDL_AUDIO OFF CACHE BOOL "" FORCE)
        set(SDL_VIDEO OFF CACHE BOOL "" FORCE)
        set(SDL_GPU OFF CACHE BOOL "" FORCE)
        set(SDL_RENDER OFF CACHE BOOL "" FORCE)
        set(SDL_CAMERA OFF CACHE BOOL "" FORCE)
        set(SDL_POWER OFF CACHE BOOL "" FORCE)
        set(SDL_DIALOG OFF CACHE BOOL "" FORCE)
        set(SDL_TRAY OFF CACHE BOOL "" FORCE)
    endif()

    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/lib/sdl)
endblock()
