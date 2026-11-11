# Rocket's compile options and definitions.

block()
    target_compile_features(Rocket PUBLIC cxx_std_23)

    target_include_directories(Rocket
        PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
            $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/lib/inplace_vector/include
            ${CMAKE_CURRENT_BINARY_DIR}
    )

    set_target_properties(Rocket
        PROPERTIES
            CXX_STANDARD_REQUIRED ON
            CXX_EXTENSIONS OFF
    )

    if(ROCKET_LINUX_GLES)
        target_include_directories(Rocket
            PRIVATE
                ${CMAKE_CURRENT_SOURCE_DIR}/lib/sdl/src/video/khronos
        )
    endif()

    if(APPLE)
        target_compile_options(Rocket
            PRIVATE
                $<$<COMPILE_LANGUAGE:OBJCXX>:-fobjc-arc>
        )
    endif()

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
    endif()
endblock()
