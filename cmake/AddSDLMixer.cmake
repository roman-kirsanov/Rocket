# SDL_mixer, the Linux backend's audio. It plays and decodes with the decoders
# it carries (WAV, AIFF, VOC, AU, MP3, FLAC, Ogg Vorbis); those needing a
# system library are left out.

block()
    set(BUILD_SHARED_LIBS OFF)
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

    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/lib/sdl_mixer)
endblock()
