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
    set(ROCKET_SHADER_HEADER ${CMAKE_CURRENT_SOURCE_DIR}/src/Rocket/Paint/Metal/Painter.metal-lib.hpp)
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
