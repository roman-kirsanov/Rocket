# Everything Rocket links: the dependencies built from lib/ and the system
# libraries of each platform backend.

block()
    target_link_libraries(Rocket
        PRIVATE
            SDL3::SDL3-static
            CURL::libcurl_static
            freetype
            lunasvg
            yogacore
    )

    if(APPLE)
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
        # The manifest is linked into every executable that links Rocket.
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
                SDL3_image::SDL3_image-static
                SDL3_mixer::SDL3_mixer-static
        )
    endif()
endblock()
