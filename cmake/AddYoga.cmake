# Yoga, the flexbox layout engine.

block()
    set(BUILD_SHARED_LIBS OFF)

    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/lib/yoga/yoga)

    if(MSVC)
        target_compile_options(yogacore PRIVATE /utf-8)
    else()
        # Yoga builds itself with -fno-rtti and hidden visibility. Without RTTI,
        # clang emits a private copy of libc++'s `typeinfo for std::exception`
        # into every Yoga object that throws, and a static link then resolves
        # the whole executable's std::exception references to that copy instead
        # of the one in libc++.dylib. The result is that
        # `catch (std::exception const&)` never matches a std::runtime_error (or
        # any other libc++ exception) anywhere in the program. Re-enabling RTTI
        # for Yoga makes it reference the shared typeinfo.
        target_compile_options(yogacore PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-frtti>)
    endif()
endblock()
