# Embeds a binary file into a C++ header as a byte array, so a compiled
# shader ships inside the library instead of beside it.
#
#   cmake -D IN=<file> -D OUT=<header> -D NAME=<symbol> -P Embed.cmake
#
# The header defines `NAME` (the bytes, 4-byte aligned, as Vulkan-style and
# Direct3D shader bytecode requires) and `NAME_len` (their count).

file(READ ${IN} EMBED_HEX HEX)
file(SIZE ${IN} EMBED_SIZE)

string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1," EMBED_BYTES "${EMBED_HEX}")
string(REGEX REPLACE "(0x..,0x..,0x..,0x..,0x..,0x..,0x..,0x..,0x..,0x..,0x..,0x..,0x..,0x..,0x..,0x..,)" "\\1\n    " EMBED_BYTES "${EMBED_BYTES}")

file(WRITE ${OUT}
    "#pragma once\n"
    "\n"
    "alignas(4) static unsigned char const ${NAME}[] = {\n"
    "    ${EMBED_BYTES}\n"
    "};\n"
    "\n"
    "static unsigned int const ${NAME}_len = ${EMBED_SIZE};\n"
)
