# Regenerates the embedded sheets of the PixelDark material in
# src/Rocket/UI/Theme/PixelDark/Image from Image.aseprite whenever it or Export.py
# changes. Needs Aseprite and Python 3; without them, or when Rocket is added
# to another project with add_subdirectory(), the checked-in Image*.cpp
# / .hpp are used as they are.

block()
    if(NOT PROJECT_IS_TOP_LEVEL)
        return()
    endif()

    set(MATERIAL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/Rocket/UI/Theme/PixelDark/Image)

    find_program(ASEPRITE_EXECUTABLE
        NAMES aseprite
        HINTS /Applications/Aseprite.app/Contents/MacOS
    )

    find_package(Python3 COMPONENTS Interpreter)

    if(NOT ASEPRITE_EXECUTABLE OR NOT Python3_Interpreter_FOUND)
        message(STATUS "Aseprite or Python 3 not found; using the checked-in material sheets")
        return()
    endif()

    add_custom_command(
        OUTPUT
            ${MATERIAL_DIR}/Image.hpp
            ${MATERIAL_DIR}/Image.cpp
            ${MATERIAL_DIR}/Image@2x.hpp
            ${MATERIAL_DIR}/Image@2x.cpp
            ${MATERIAL_DIR}/Image@3x.hpp
            ${MATERIAL_DIR}/Image@3x.cpp
        COMMAND ${Python3_EXECUTABLE} ${MATERIAL_DIR}/Export.py
            --aseprite ${ASEPRITE_EXECUTABLE}
            --input ${MATERIAL_DIR}/Image.aseprite
            --output-dir ${MATERIAL_DIR}
            --name Image
            --symbol MATERIAL_PIXEL_DARK
            --scales 1 2 3
        DEPENDS ${MATERIAL_DIR}/Image.aseprite ${MATERIAL_DIR}/Export.py
        COMMENT "Generating the PixelDark material sheets from Image.aseprite"
        VERBATIM
    )
endblock()
