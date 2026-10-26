cmake_minimum_required(VERSION 3.25)

if(NOT DEFINED CMAKE_OSX_DEPLOYMENT_TARGET)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "15.0" CACHE STRING "Minimum macOS version")
endif()

project(Rocket VERSION 0.1.0 LANGUAGES C CXX)

if(APPLE)
    enable_language(OBJCXX)
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Linux only: draw with OpenGL ES 3.0 instead of SDL_GPU, for devices with no
# Vulkan driver (the Mali GPUs of the AmberELEC handhelds).
option(ROCKET_GLES "Draw with OpenGL ES 3.0 instead of SDL_GPU (Linux)" OFF)

if(ROCKET_GLES AND NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
    message(FATAL_ERROR "ROCKET_GLES is for Linux only")
endif()

set(ROCKET_EMBED ${CMAKE_CURRENT_SOURCE_DIR}/cmake/Embed.cmake)
set(ROCKET_SOURCE
    src/Rocket/Audio/Audio.cpp
    src/Rocket/Base/Date.cpp
    src/Rocket/Base/JSON.cpp
    src/Rocket/Base/Object.cpp
    src/Rocket/Base/String.cpp
    src/Rocket/Base/Time.cpp
    src/Rocket/Fetch/Fetch.cpp
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
    src/Rocket/Window/AppEvent.cpp
    src/Rocket/Window/Clipboard.cpp
    src/Rocket/Window/FileDialog.cpp
    src/Rocket/Window/Gamepad_SDL3.cpp
    src/Rocket/Window/Input.cpp
    src/Rocket/Window/Window.cpp
    src/Rocket/Window/WindowEvent.cpp
)

if(APPLE)
    list(APPEND ROCKET_SOURCE
        src/Rocket/Audio/Audio_MacOS.mm
        src/Rocket/Paint/Image_MacOS.mm
        src/Rocket/Paint/Painter_MacOS.mm
        src/Rocket/Window/App_MacOS.mm
        src/Rocket/Window/Clipboard_MacOS.mm
        src/Rocket/Window/FileDialog_MacOS.mm
        src/Rocket/Window/Window_MacOS.mm
    )
elseif(WIN32)
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
    if(ROCKET_GLES)
        list(APPEND ROCKET_SOURCE
            src/Rocket/Paint/Image_GLES.cpp
            src/Rocket/Paint/Painter_GLES.cpp
        )
    else()
        list(APPEND ROCKET_SOURCE
            src/Rocket/Paint/Image_SDL3.cpp
            src/Rocket/Paint/Painter_SDL3.cpp
        )
    endif()

    list(APPEND ROCKET_SOURCE
        src/Rocket/Audio/Audio_SDL3.cpp
        src/Rocket/Window/App_SDL3.cpp
        src/Rocket/Window/Clipboard_SDL3.cpp
        src/Rocket/Window/FileDialog_SDL3.cpp
        src/Rocket/Window/Window_SDL3.cpp
    )
endif()

add_library(Rocket STATIC ${ROCKET_SOURCE})
add_library(Rocket::Rocket ALIAS Rocket)

include(CTest)
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/Shaders.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/CFlags.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/LFlags.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/AddCurl.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/AddFreetype.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/AddLunaSVG.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/AddYoga.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/AddSDL.cmake)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/AddSDLImage.cmake)
    include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/AddSDLMixer.cmake)
endif()

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/Install.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/Testing.cmake)
