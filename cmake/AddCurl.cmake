# libcurl, built from its submodule as a static library, HTTP(S) only. TLS
# comes from the platform: Schannel on Windows, OpenSSL elsewhere (Homebrew's
# static OpenSSL on macOS, verified against the system keychain).

block()
    set(BUILD_SHARED_LIBS OFF)
    set(BUILD_STATIC_LIBS ON)
    set(BUILD_CURL_EXE OFF)
    set(BUILD_TESTING OFF)
    set(BUILD_EXAMPLES OFF)
    set(BUILD_LIBCURL_DOCS OFF)
    set(BUILD_MISC_DOCS OFF)
    set(ENABLE_CURL_MANUAL OFF)
    set(PICKY_COMPILER OFF)
    set(CURL_USE_LIBPSL OFF)
    set(CURL_USE_LIBSSH2 OFF)
    set(CURL_BROTLI OFF CACHE STRING "" FORCE)
    set(CURL_ZSTD OFF CACHE STRING "" FORCE)
    set(USE_LIBIDN2 OFF)
    set(USE_NGHTTP2 OFF)

    foreach(PROTOCOL DICT FILE FTP GOPHER IMAP IPFS LDAP MQTT POP3 RTSP SMTP TELNET TFTP WEBSOCKETS)
        set(CURL_DISABLE_${PROTOCOL} ON)
    endforeach()

    if(WIN32)
        set(CURL_USE_SCHANNEL ON)
        set(CURL_USE_OPENSSL OFF)
    else()
        set(CURL_USE_OPENSSL ON)
    endif()

    if(APPLE)
        set(USE_APPLE_SECTRUST ON)
        set(OPENSSL_USE_STATIC_LIBS ON)

        if(NOT DEFINED OPENSSL_ROOT_DIR)
            execute_process(
                COMMAND brew --prefix openssl@3
                OUTPUT_VARIABLE OPENSSL_ROOT_DIR
                OUTPUT_STRIP_TRAILING_WHITESPACE
                ERROR_QUIET
            )
        endif()
    endif()

    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/lib/curl)
endblock()
