# Compiles the painter's shaders and embeds them as byte arrays, so they ship
# inside Rocket:
#
#   macOS    Painter.metal -> Painter.metallib -> Rocket_MetalLib in Painter.metal-lib.hpp
#   Windows  <Name>.hlsl   -> <Name>.dxil      -> Rocket_DXIL_<Name> in <Name>.dxil.hpp
#   Linux    <Name>.vert/.frag -> <Name>.spv   -> Rocket_SPIRV_<Name> in <Name>.spv.hpp

block()
    if(APPLE)
        set(SHADER_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/src/Rocket/Paint/Metal/Painter.metal)
        set(SHADER_TYPES  ${CMAKE_CURRENT_SOURCE_DIR}/src/Rocket/Paint/Metal/Painter.metal.hpp)
        set(SHADER_HEADER ${CMAKE_CURRENT_BINARY_DIR}/Painter.metal-lib.hpp)
        set(SHADER_AIR    ${CMAKE_CURRENT_BINARY_DIR}/Painter.air)
        set(SHADER_LIB    ${CMAKE_CURRENT_BINARY_DIR}/Painter.metallib)

        add_custom_command(
            OUTPUT ${SHADER_AIR}
            COMMAND xcrun -sdk macosx metal -c ${SHADER_SOURCE} -o ${SHADER_AIR}
            DEPENDS ${SHADER_SOURCE} ${SHADER_TYPES}
            COMMENT "Compiling Painter.metal"
            VERBATIM
        )

        add_custom_command(
            OUTPUT ${SHADER_LIB}
            COMMAND xcrun -sdk macosx metallib ${SHADER_AIR} -o ${SHADER_LIB}
            DEPENDS ${SHADER_AIR}
            VERBATIM
        )

        add_custom_command(
            OUTPUT ${SHADER_HEADER}
            COMMAND ${CMAKE_COMMAND} -D IN=${SHADER_LIB} -D OUT=${SHADER_HEADER} -D NAME=Rocket_MetalLib -P ${ROCKET_EMBED}
            DEPENDS ${SHADER_LIB} ${ROCKET_EMBED}
            COMMENT "Embedding Painter.metallib into Painter.metal-lib.hpp"
            VERBATIM
        )

        target_sources(Rocket PRIVATE ${SHADER_HEADER})
    elseif(WIN32 OR CMAKE_SYSTEM_NAME STREQUAL "Linux")
        set(SHADERS
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

        if(WIN32)
            set(PROGRAM_FILES_X86 "ProgramFiles(x86)")
            file(GLOB WINDOWS_SDK_BINS "$ENV{${PROGRAM_FILES_X86}}/Windows Kits/10/bin/10.*/x64")
            list(SORT WINDOWS_SDK_BINS COMPARE NATURAL ORDER DESCENDING)
            find_program(ROCKET_DXC dxc HINTS $ENV{WindowsSdkVerBinPath}/x64 ${WINDOWS_SDK_BINS} REQUIRED)

            set(SHADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/Rocket/Paint/HLSL)
            set(SHADER_COMMON ${SHADER_DIR}/Painter.hlsli)
            set(SHADER_FORMAT dxil)
            set(SHADER_SYMBOL Rocket_DXIL)
        else()
            find_program(ROCKET_GLSLC glslc REQUIRED)

            set(SHADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/Rocket/Paint/GLSL)
            set(SHADER_COMMON ${SHADER_DIR}/Painter.glsl)
            set(SHADER_FORMAT spv)
            set(SHADER_SYMBOL Rocket_SPIRV)
        endif()

        foreach(SHADER_NAME ${SHADERS})
            set(SHADER_OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${SHADER_NAME}.${SHADER_FORMAT})
            set(SHADER_HEADER ${CMAKE_CURRENT_BINARY_DIR}/${SHADER_NAME}.${SHADER_FORMAT}.hpp)

            if(WIN32)
                if(SHADER_NAME STREQUAL "QuadVertex")
                    set(SHADER_PROFILE vs_6_0)
                else()
                    set(SHADER_PROFILE ps_6_0)
                endif()

                set(SHADER_FILE ${SHADER_NAME}.hlsl)
                set(SHADER_COMMAND ${ROCKET_DXC} -nologo -T ${SHADER_PROFILE} -E main -O3 -WX -Fo ${SHADER_OUTPUT} ${SHADER_DIR}/${SHADER_FILE})
            else()
                if(SHADER_NAME STREQUAL "QuadVertex")
                    set(SHADER_FILE ${SHADER_NAME}.vert)
                else()
                    set(SHADER_FILE ${SHADER_NAME}.frag)
                endif()

                set(SHADER_COMMAND ${ROCKET_GLSLC} --target-env=vulkan1.0 -O -o ${SHADER_OUTPUT} ${SHADER_DIR}/${SHADER_FILE})
            endif()

            add_custom_command(
                OUTPUT ${SHADER_OUTPUT}
                COMMAND ${SHADER_COMMAND}
                DEPENDS ${SHADER_DIR}/${SHADER_FILE} ${SHADER_COMMON}
                COMMENT "Compiling ${SHADER_FILE}"
                VERBATIM
            )

            add_custom_command(
                OUTPUT ${SHADER_HEADER}
                COMMAND ${CMAKE_COMMAND} -D IN=${SHADER_OUTPUT} -D OUT=${SHADER_HEADER} -D NAME=${SHADER_SYMBOL}_${SHADER_NAME} -P ${ROCKET_EMBED}
                DEPENDS ${SHADER_OUTPUT} ${ROCKET_EMBED}
                COMMENT "Embedding ${SHADER_NAME}.${SHADER_FORMAT} into ${SHADER_NAME}.${SHADER_FORMAT}.hpp"
                VERBATIM
            )

            target_sources(Rocket PRIVATE ${SHADER_HEADER})
        endforeach()
    endif()
endblock()
