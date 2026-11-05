#!/usr/bin/env python3
"""Export an .aseprite file to embedded C++ RGBA byte arrays.

For each scale N the sprite is exported through the Aseprite CLI as a PNG,
decoded here (stdlib only), and written as <name>[@Nx].cpp / .hpp declaring
<symbol>[_NX]_DATA (RGBA8, row-major) and <symbol>[_NX]_SIZE (Vec2).
"""

import argparse
import os
import struct
import subprocess
import sys
import tempfile
import zlib


def decode_png(path):
    with open(path, "rb") as f:
        data = f.read()
    if data[:8] != b"\x89PNG\r\n\x1a\n":
        raise ValueError(f"{path}: not a PNG")
    pos, idat, plte, trns, ihdr = 8, [], None, None, None
    while pos < len(data):
        length, ctype = struct.unpack(">I4s", data[pos:pos + 8])
        body = data[pos + 8:pos + 8 + length]
        pos += 12 + length
        if ctype == b"IHDR":
            ihdr = struct.unpack(">IIBBBBB", body)
        elif ctype == b"PLTE":
            plte = body
        elif ctype == b"tRNS":
            trns = body
        elif ctype == b"IDAT":
            idat.append(body)
        elif ctype == b"IEND":
            break
    width, height, depth, ctype, _, _, interlace = ihdr
    if depth != 8 or interlace != 0:
        raise ValueError(f"{path}: only 8-bit non-interlaced PNGs are supported")
    channels = {0: 1, 2: 3, 3: 1, 4: 2, 6: 4}[ctype]
    raw = zlib.decompress(b"".join(idat))
    stride = width * channels
    prev = bytearray(stride)
    rows = []
    pos = 0
    for _ in range(height):
        ftype = raw[pos]
        cur = bytearray(raw[pos + 1:pos + 1 + stride])
        pos += 1 + stride
        if ftype == 1:
            for i in range(channels, stride):
                cur[i] = (cur[i] + cur[i - channels]) & 0xFF
        elif ftype == 2:
            for i in range(stride):
                cur[i] = (cur[i] + prev[i]) & 0xFF
        elif ftype == 3:
            for i in range(stride):
                left = cur[i - channels] if i >= channels else 0
                cur[i] = (cur[i] + ((left + prev[i]) >> 1)) & 0xFF
        elif ftype == 4:
            for i in range(stride):
                a = cur[i - channels] if i >= channels else 0
                b = prev[i]
                c = prev[i - channels] if i >= channels else 0
                p = a + b - c
                pa, pb, pc = abs(p - a), abs(p - b), abs(p - c)
                pred = a if (pa <= pb and pa <= pc) else (b if pb <= pc else c)
                cur[i] = (cur[i] + pred) & 0xFF
        elif ftype != 0:
            raise ValueError(f"{path}: bad filter type {ftype}")
        rows.append(bytes(cur))
        prev = cur
    pixels = b"".join(rows)
    out = bytearray(width * height * 4)
    if ctype == 6:
        out[:] = pixels
    elif ctype == 2:
        out[0::4], out[1::4], out[2::4] = pixels[0::3], pixels[1::3], pixels[2::3]
        out[3::4] = b"\xff" * (width * height)
    elif ctype == 3:
        alpha = list(trns or b"") + [255] * 256
        for i, idx in enumerate(pixels):
            out[i * 4:i * 4 + 3] = plte[idx * 3:idx * 3 + 3]
            out[i * 4 + 3] = alpha[idx]
    elif ctype == 4:
        out[0::4] = out[1::4] = out[2::4] = pixels[0::2]
        out[3::4] = pixels[1::2]
    else:
        out[0::4] = out[1::4] = out[2::4] = pixels
        out[3::4] = b"\xff" * (width * height)
    return width, height, bytes(out)


def write_pair(out_dir, name, symbol, width, height, rgba):
    hpp = f"""#pragma once

#include <cstdlib>
#include <cinttypes>
#include <Rocket/Math/Vec2.hpp>

namespace Rocket {{

/** Embedded {name} image pixels, RGBA8, row-major. */
extern std::uint8_t {symbol}_DATA[];
/** Size in pixels of {symbol}_DATA. */
extern Vec2 {symbol}_SIZE;

}} /* namespace Rocket */
"""
    cpp = f"""#include "{name}.hpp"

namespace Rocket {{

std::uint8_t {symbol}_DATA[] = {{ {", ".join(map(str, rgba))} }};
Vec2 {symbol}_SIZE = {{ {width:.1f}f, {height:.1f}f }};

}} /* namespace Rocket */
"""
    with open(os.path.join(out_dir, f"{name}.hpp"), "w") as f:
        f.write(hpp)
    with open(os.path.join(out_dir, f"{name}.cpp"), "w") as f:
        f.write(cpp)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--aseprite", required=True, help="path to the aseprite executable")
    ap.add_argument("--input", required=True, help=".aseprite source file")
    ap.add_argument("--output-dir", required=True)
    ap.add_argument("--name", required=True, help="base file name, e.g. Default")
    ap.add_argument("--symbol", required=True, help="base symbol name, e.g. MATERIAL_DEFAULT")
    ap.add_argument("--scales", type=int, nargs="+", default=[1, 2, 3])
    args = ap.parse_args()

    os.makedirs(args.output_dir, exist_ok=True)
    with tempfile.TemporaryDirectory() as tmp:
        for scale in args.scales:
            png = os.path.join(tmp, f"{scale}x.png")
            cmd = [args.aseprite, "-b", args.input]
            if scale != 1:
                cmd += ["--scale", str(scale)]
            cmd += ["--save-as", png]
            subprocess.run(cmd, check=True, stdout=subprocess.DEVNULL)
            width, height, rgba = decode_png(png)
            suffix = "" if scale == 1 else f"@{scale}x"
            symbol = args.symbol if scale == 1 else f"{args.symbol}_{scale}X"
            write_pair(args.output_dir, f"{args.name}{suffix}", symbol, width, height, rgba)
            print(f"{args.name}{suffix}: {width}x{height}", file=sys.stderr)


if __name__ == "__main__":
    main()
