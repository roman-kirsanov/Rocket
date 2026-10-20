cmake_minimum_required(VERSION 3.23)

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
set(ROCKET_EMBED ${CMAKE_CURRENT_SOURCE_DIR}/cmake/Embed.cmake)
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
        COMMAND ${CMAKE_COMMAND} -D IN=${ROCKET_SHADER_LIB} -D OUT=${ROCKET_SHADER_HEADER} -D NAME=Rocket_MetalLib -P ${ROCKET_EMBED}
        DEPENDS ${ROCKET_SHADER_LIB} ${ROCKET_EMBED}
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
    set(ROCKET_PROGRAM_FILES_X86 "ProgramFiles(x86)")
    file(GLOB ROCKET_WINDOWS_SDK_BINS "$ENV{${ROCKET_PROGRAM_FILES_X86}}/Windows Kits/10/bin/10.*/x64")
    list(SORT ROCKET_WINDOWS_SDK_BINS COMPARE NATURAL ORDER DESCENDING)
    set(ROCKET_SHADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/Rocket/Paint/HLSL)
    set(ROCKET_SHADER_COMMON ${ROCKET_SHADER_DIR}/Painter.hlsli)
    set(ROCKET_SHADERS
        QuadVertex
        ColorBrush
        ImageBrush
        LinearGradientBrush
        RadialGradientBrush
        BlurFilter
        ShadowFilterDilate
        ShadowFilterBlur
        ShadowFilterComposite
    )

    find_program(ROCKET_DXC dxc HINTS $ENV{WindowsSdkVerBinPath}/x64 ${ROCKET_WINDOWS_SDK_BINS} REQUIRED)

    foreach(ROCKET_SHADER_NAME ${ROCKET_SHADERS})
        set(ROCKET_SHADER_DXIL   ${CMAKE_CURRENT_BINARY_DIR}/${ROCKET_SHADER_NAME}.dxil)
        set(ROCKET_SHADER_HEADER ${CMAKE_CURRENT_BINARY_DIR}/${ROCKET_SHADER_NAME}.dxil.hpp)

        if(ROCKET_SHADER_NAME STREQUAL "QuadVertex")
            set(ROCKET_SHADER_PROFILE vs_6_0)
        else()
            set(ROCKET_SHADER_PROFILE ps_6_0)
        endif()

        add_custom_command(
            OUTPUT ${ROCKET_SHADER_DXIL}
            COMMAND ${ROCKET_DXC} -nologo -T ${ROCKET_SHADER_PROFILE} -E main -O3 -WX -Fo ${ROCKET_SHADER_DXIL} ${ROCKET_SHADER_DIR}/${ROCKET_SHADER_NAME}.hlsl
            DEPENDS ${ROCKET_SHADER_DIR}/${ROCKET_SHADER_NAME}.hlsl ${ROCKET_SHADER_COMMON}
            COMMENT "Compiling ${ROCKET_SHADER_NAME}.hlsl"
            VERBATIM
        )

        add_custom_command(
            OUTPUT ${ROCKET_SHADER_HEADER}
            COMMAND ${CMAKE_COMMAND} -D IN=${ROCKET_SHADER_DXIL} -D OUT=${ROCKET_SHADER_HEADER} -D NAME=Rocket_DXIL_${ROCKET_SHADER_NAME} -P ${ROCKET_EMBED}
            DEPENDS ${ROCKET_SHADER_DXIL} ${ROCKET_EMBED}
            COMMENT "Embedding ${ROCKET_SHADER_NAME}.dxil into ${ROCKET_SHADER_NAME}.dxil.hpp"
            VERBATIM
        )

        list(APPEND ROCKET_SOURCE ${ROCKET_SHADER_HEADER})
    endforeach()

    list(APPEND ROCKET_SOURCE
        src/Rocket/Audio/Audio_Win32.cpp
        src/Rocket/Paint/Image_Win32.cpp
        src/Rocket/Paint/Painter_Win32.cpp
        src/Rocket/Window/App_Win32.cpp
        src/Rocket/Window/Clipboard_Win32.cpp
        src/Rocket/Window/FileDialog_Win32.cpp
        src/Rocket/Window/Window_Win32.cpp
    )
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
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

    set(ROCKET_SHADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/Rocket/Paint/GLSL)
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
            COMMAND ${CMAKE_COMMAND} -D IN=${ROCKET_SHADER_SPIRV} -D OUT=${ROCKET_SHADER_HEADER} -D NAME=Rocket_SPIRV_${ROCKET_SHADER_NAME} -P ${ROCKET_EMBED}
            DEPENDS ${ROCKET_SHADER_SPIRV} ${ROCKET_EMBED}
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

add_library(Rocket STATIC ${ROCKET_SOURCE})
add_library(Rocket::Rocket ALIAS Rocket)

if(MSVC)
    target_compile_options(Rocket
        PUBLIC
            /utf-8
            /Zc:__cplusplus
            /Zc:preprocessor
        PRIVATE
            /bigobj
    )

    target_compile_definitions(Rocket
        PRIVATE
            _CRT_SECURE_NO_WARNINGS
            UNICODE
            _UNICODE
    )

    target_compile_options(yogacore PRIVATE /utf-8)
else()
    target_compile_options(yogacore PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-frtti>)
endif()

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
            "-framework Carbon"
            "-framework Cocoa"
            "-framework Metal"
            "-framework QuartzCore"
            "-framework CoreGraphics"
            "-framework ImageIO"
    )
elseif(WIN32)
    target_sources(Rocket
        INTERFACE
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Rocket.manifest>
    )

    target_link_libraries(Rocket
        PRIVATE
            avrt
            d3d12
            dwmapi
            dxgi
            dxguid
            imm32
            mfplat
            mfreadwrite
            mfuuid
            ole32
            shcore
            shell32
            shlwapi
            user32
            uuid
            windowscodecs
            winmm
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
