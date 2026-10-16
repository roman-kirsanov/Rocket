cmake_minimum_required(VERSION 3.23)

# macOS 15 (Sequoia) is the minimum: the window cursors use the frame, column,
# row and zoom cursor factories AppKit added in that release.
if(NOT DEFINED CMAKE_OSX_DEPLOYMENT_TARGET)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "15.0" CACHE STRING "Minimum macOS version")
endif()

project(Rocket VERSION 0.1.0 LANGUAGES C CXX)

include(CTest)
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(LUNASVG_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(FT_DISABLE_ZLIB ON CACHE BOOL "" FORCE)
set(FT_DISABLE_BZIP2 ON CACHE BOOL "" FORCE)
set(FT_DISABLE_PNG ON CACHE BOOL "" FORCE)
set(FT_DISABLE_HARFBUZZ ON CACHE BOOL "" FORCE)
set(FT_DISABLE_BROTLI ON CACHE BOOL "" FORCE)
set(ROCKET_SOURCE
    src/Rocket/Audio/Audio.cpp
    src/Rocket/Base/Date.cpp
    src/Rocket/Base/JSON.cpp
    src/Rocket/Base/Object.cpp
    src/Rocket/Base/String.cpp
    src/Rocket/Base/Time.cpp
    src/Rocket/Math/Float.cpp
    src/Rocket/Math/Mat3.cpp
    src/Rocket/Math/Vec2.cpp
    src/Rocket/Math/Vec4.cpp
    src/Rocket/Node/Document.cpp
    src/Rocket/Node/Node.cpp
    src/Rocket/Node/NodeEvent.cpp
    src/Rocket/Node/NodeValue.cpp
    src/Rocket/Paint/Brush.cpp
    src/Rocket/Paint/Color.cpp
    src/Rocket/Paint/Filter.cpp
    src/Rocket/Paint/Font.cpp
    src/Rocket/Paint/Fonts/Inter.cpp
    src/Rocket/Paint/Fonts/InterItalic.cpp
    src/Rocket/Paint/Image.cpp
    src/Rocket/Paint/Painter.cpp
    src/Rocket/Paint/Shadow.cpp
    src/Rocket/Paint/Shape.cpp
    src/Rocket/Paint/Svg.cpp
    src/Rocket/Paint/Text.cpp
    src/Rocket/UI/Document.cpp
    src/Rocket/UI/Node.cpp
    src/Rocket/UI/Popup.cpp
    src/Rocket/UI/Reconciler.cpp
    src/Rocket/UI/Splitter.cpp
    src/Rocket/UI/Window.cpp
    src/Rocket/Window/App.cpp
    src/Rocket/Window/Clipboard.cpp
    src/Rocket/Window/FileDialog.cpp
    src/Rocket/Window/Input.cpp
    src/Rocket/Window/Window.cpp
    src/Rocket/Window/WindowEvent.cpp
)

if(APPLE)
    enable_language(OBJCXX)

    set(ROCKET_SHADER_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/src/Rocket/Paint/Metal/Painter.metal)
    set(ROCKET_SHADER_TYPES  ${CMAKE_CURRENT_SOURCE_DIR}/src/Rocket/Paint/Metal/Painter.metal.hpp)
    set(ROCKET_SHADER_HEADER ${CMAKE_CURRENT_BINARY_DIR}/Painter.metal-lib.hpp)
    set(ROCKET_SHADER_AIR    ${CMAKE_CURRENT_BINARY_DIR}/Painter.air)
    set(ROCKET_SHADER_LIB    ${CMAKE_CURRENT_BINARY_DIR}/Painter.metallib)

    add_custom_command(
        OUTPUT ${ROCKET_SHADER_AIR}
        COMMAND xcrun -sdk macosx metal -c ${ROCKET_SHADER_SOURCE} -o ${ROCKET_SHADER_AIR}
        DEPENDS ${ROCKET_SHADER_SOURCE} ${ROCKET_SHADER_TYPES}
        COMMENT "Compiling Painter.metal"
        VERBATIM
    )

    add_custom_command(
        OUTPUT ${ROCKET_SHADER_LIB}
        COMMAND xcrun -sdk macosx metallib ${ROCKET_SHADER_AIR} -o ${ROCKET_SHADER_LIB}
        DEPENDS ${ROCKET_SHADER_AIR}
        VERBATIM
    )

    add_custom_command(
        OUTPUT ${ROCKET_SHADER_HEADER}
        COMMAND xxd -n Rocket_MetalLib -i ${ROCKET_SHADER_LIB} > ${ROCKET_SHADER_HEADER}
        DEPENDS ${ROCKET_SHADER_LIB}
        COMMENT "Embedding Painter.metallib into Painter.metal-lib.hpp"
        VERBATIM
    )

    list(APPEND ROCKET_SOURCE
        src/Rocket/Audio/Audio_MacOS.mm
        src/Rocket/Paint/Image_MacOS.mm
        src/Rocket/Paint/Painter_MacOS.mm
        src/Rocket/Window/App_MacOS.mm
        src/Rocket/Window/Clipboard_MacOS.mm
        src/Rocket/Window/FileDialog_MacOS.mm
        src/Rocket/Window/Window_MacOS.mm
        ${ROCKET_SHADER_HEADER}
    )
elseif(WIN32)

elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    # The SDL3 backend. SDL, SDL_image and SDL_mixer are built from their
    # submodules and linked statically; SDL_image decodes PNG and JPEG with its
    # built-in stb_image, so no other image library is needed.
    set(SDL_SHARED OFF CACHE BOOL "" FORCE)
    set(SDL_STATIC ON CACHE BOOL "" FORCE)
    set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
    set(SDL_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(SDL_INSTALL ON CACHE BOOL "" FORCE)
    set(SDLIMAGE_INSTALL ON CACHE BOOL "" FORCE)
    set(SDLIMAGE_VENDORED OFF CACHE BOOL "" FORCE)
    set(SDLIMAGE_SAMPLES OFF CACHE BOOL "" FORCE)
    set(SDLIMAGE_BACKEND_STB ON CACHE BOOL "" FORCE)
    set(SDLIMAGE_PNG_LIBPNG OFF CACHE BOOL "" FORCE)
    set(SDLIMAGE_AVIF OFF CACHE BOOL "" FORCE)
    set(SDLIMAGE_JXL OFF CACHE BOOL "" FORCE)
    set(SDLIMAGE_TIF OFF CACHE BOOL "" FORCE)
    set(SDLIMAGE_WEBP OFF CACHE BOOL "" FORCE)
    set(SDLIMAGE_SVG OFF CACHE BOOL "" FORCE)
    # SDL_mixer plays the audio and decodes it with the decoders it carries
    # (WAV, AIFF, VOC, AU, MP3, FLAC, Ogg Vorbis); those needing a system
    # library are left out.
    set(SDLMIXER_VENDORED OFF CACHE BOOL "" FORCE)
    set(SDLMIXER_SAMPLES OFF CACHE BOOL "" FORCE)
    set(SDLMIXER_INSTALL ON CACHE BOOL "" FORCE)
    set(SDLMIXER_FLAC_LIBFLAC OFF CACHE BOOL "" FORCE)
    set(SDLMIXER_MP3_MPG123 OFF CACHE BOOL "" FORCE)
    set(SDLMIXER_VORBIS_VORBISFILE OFF CACHE BOOL "" FORCE)
    set(SDLMIXER_OPUS OFF CACHE BOOL "" FORCE)
    set(SDLMIXER_WAVPACK OFF CACHE BOOL "" FORCE)
    set(SDLMIXER_MOD OFF CACHE BOOL "" FORCE)
    set(SDLMIXER_GME OFF CACHE BOOL "" FORCE)
    set(SDLMIXER_MIDI OFF CACHE BOOL "" FORCE)
    set(BUILD_SHARED_LIBS OFF)

    add_subdirectory(lib/sdl EXCLUDE_FROM_ALL)
    add_subdirectory(lib/sdl_image EXCLUDE_FROM_ALL)
    add_subdirectory(lib/sdl_mixer EXCLUDE_FROM_ALL)

    find_program(ROCKET_GLSLC glslc REQUIRED)

    set(ROCKET_SHADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/Rocket/Paint/SDL3)
    set(ROCKET_SHADER_COMMON ${ROCKET_SHADER_DIR}/Painter.glsl)
    set(ROCKET_SHADERS
        QuadVertex.vert
        ColorBrush.frag
        ImageBrush.frag
        LinearGradientBrush.frag
        RadialGradientBrush.frag
        BlurFilter.frag
        ShadowFilterDilate.frag
        ShadowFilterBlur.frag
        ShadowFilterComposite.frag
    )

    # Each shader is compiled to SPIR-V and embedded as a byte array
    # Rocket_SPIRV_<Name> in <Name>.spv.hpp, the way Painter.metallib is.
    foreach(ROCKET_SHADER ${ROCKET_SHADERS})
        get_filename_component(ROCKET_SHADER_NAME ${ROCKET_SHADER} NAME_WE)

        set(ROCKET_SHADER_SPIRV  ${CMAKE_CURRENT_BINARY_DIR}/${ROCKET_SHADER_NAME}.spv)
        set(ROCKET_SHADER_HEADER ${CMAKE_CURRENT_BINARY_DIR}/${ROCKET_SHADER_NAME}.spv.hpp)

        add_custom_command(
            OUTPUT ${ROCKET_SHADER_SPIRV}
            COMMAND ${ROCKET_GLSLC} --target-env=vulkan1.0 -O -o ${ROCKET_SHADER_SPIRV} ${ROCKET_SHADER_DIR}/${ROCKET_SHADER}
            DEPENDS ${ROCKET_SHADER_DIR}/${ROCKET_SHADER} ${ROCKET_SHADER_COMMON}
            COMMENT "Compiling ${ROCKET_SHADER}"
            VERBATIM
        )

        add_custom_command(
            OUTPUT ${ROCKET_SHADER_HEADER}
            COMMAND xxd -n Rocket_SPIRV_${ROCKET_SHADER_NAME} -i ${ROCKET_SHADER_SPIRV} ${ROCKET_SHADER_HEADER}
            DEPENDS ${ROCKET_SHADER_SPIRV}
            COMMENT "Embedding ${ROCKET_SHADER_NAME}.spv into ${ROCKET_SHADER_NAME}.spv.hpp"
            VERBATIM
        )

        list(APPEND ROCKET_SOURCE ${ROCKET_SHADER_HEADER})
    endforeach()

    list(APPEND ROCKET_SOURCE
        src/Rocket/Audio/Audio_SDL3.cpp
        src/Rocket/Paint/Image_SDL3.cpp
        src/Rocket/Paint/Painter_SDL3.cpp
        src/Rocket/Window/App_SDL3.cpp
        src/Rocket/Window/Clipboard_SDL3.cpp
        src/Rocket/Window/FileDialog_SDL3.cpp
        src/Rocket/Window/Window_SDL3.cpp
    )
endif()

add_subdirectory(lib/freetype)
add_subdirectory(lib/lunasvg)
add_subdirectory(lib/yoga/yoga)

add_library(Rocket ${ROCKET_SOURCE})
add_library(Rocket::Rocket ALIAS Rocket)

# Yoga builds itself with -fno-rtti and hidden visibility. Without RTTI, clang
# emits a private copy of libc++'s `typeinfo for std::exception` into every
# Yoga object that throws, and a static link then resolves the whole
# executable's std::exception references to that copy instead of the one in
# libc++.dylib. The result is that `catch (std::exception const&)` never
# matches a std::runtime_error (or any other libc++ exception) anywhere in the
# program. Re-enabling RTTI for Yoga makes it reference the shared typeinfo.
target_compile_options(yogacore PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-frtti>)

target_include_directories(Rocket
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/inplace_vector/include
        ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(Rocket
    PRIVATE
        freetype
        lunasvg
        yogacore
)

if(APPLE)
    target_compile_options(Rocket
        PRIVATE
            $<$<COMPILE_LANGUAGE:OBJCXX>:-fobjc-arc>
    )

    target_link_libraries(Rocket
        PRIVATE
            "-framework AVFoundation"
            "-framework Cocoa"
            "-framework Metal"
            "-framework QuartzCore"
            "-framework CoreGraphics"
            "-framework ImageIO"
    )
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_libraries(Rocket
        PRIVATE
            SDL3::SDL3-static
            SDL3_image::SDL3_image-static
            SDL3_mixer::SDL3_mixer-static
    )
endif()

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

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
