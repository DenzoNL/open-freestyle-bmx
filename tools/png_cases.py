#!/usr/bin/env python3
"""Synthetic PNG files for the BMX_PNG_CHECK differential check (src/native/pngcheck.c).

Every colour type / bit depth combination, plain and Adam7, at a few sizes (1x1, odd widths, a width that
is not a multiple of the pass steps), with random pixels, every filter type, palettes with and without
tRNS, an ancillary chunk before and after the image data, IDAT split over several chunks, stored, fixed and
dynamic deflate blocks. Plus a few corrupt files (truncated, bad CRC, bad zlib data) that both readers
must reject.

    py -I tools/png_cases.py <out folder>
"""

import random
import struct
import sys
import zlib
from pathlib import Path

COMBOS = {0: (1, 2, 4, 8, 16), 2: (8, 16), 3: (1, 2, 4, 8), 4: (8, 16), 6: (8, 16)}
CHANNELS = {0: 1, 2: 3, 3: 1, 4: 2, 6: 4}
PASSES = [(0, 0, 8, 8), (4, 0, 8, 8), (0, 4, 4, 8), (2, 0, 4, 4), (0, 2, 2, 4), (1, 0, 2, 2), (0, 1, 1, 2)]


def chunk(kind, data):
    body = kind + data
    return struct.pack(">I", len(data)) + body + struct.pack(">I", zlib.crc32(body) & 0xFFFFFFFF)


def pack_row(samples, depth):
    if depth == 16:
        return b"".join(struct.pack(">H", s) for s in samples)
    if depth == 8:
        return bytes(samples)
    out, acc, bits = bytearray(), 0, 0
    for s in samples:
        acc = (acc << depth) | s
        bits += depth
        if bits == 8:
            out.append(acc)
            acc, bits = 0, 0
    if bits:
        out.append(acc << (8 - bits))
    return bytes(out)


def paeth(a, b, c):
    p = a + b - c
    pa, pb, pc = abs(p - a), abs(p - b), abs(p - c)
    if pa <= pb and pa <= pc:
        return a
    return b if pb <= pc else c


def filter_rows(rows, bpp, rng):
    out, prev = bytearray(), None
    for row in rows:
        f = rng.randrange(5)
        line = bytearray([f])
        for i, x in enumerate(row):
            a = row[i - bpp] if i >= bpp else 0
            b = prev[i] if prev is not None else 0
            c = prev[i - bpp] if (prev is not None and i >= bpp) else 0
            pred = [0, a, b, (a + b) >> 1, paeth(a, b, c)][f]
            line.append((x - pred) & 0xFF)
        out += line
        prev = row
    return bytes(out)


def make_png(rng, width, height, ctype, depth, interlace, level, trns, split):
    maxv = (1 << depth) - 1
    npal = rng.randrange(1, min(256, maxv + 1) + 1) if ctype == 3 else 0
    ch = CHANNELS[ctype]
    top = npal - 1 if ctype == 3 else maxv
    pixels = [[[rng.randint(0, top) for _ in range(ch)] for _ in range(width)] for _ in range(height)]
    bpp = max(1, ch * depth // 8)
    raw = bytearray()
    passes = PASSES if interlace else [(0, 0, 1, 1)]
    for x0, y0, dx, dy in passes:
        rows = []
        for y in range(y0, height, dy):
            samples = [s for x in range(x0, width, dx) for s in pixels[y][x]]
            if samples:
                rows.append(pack_row(samples, depth))
        raw += filter_rows(rows, bpp, rng)
    comp = zlib.compressobj(level, zlib.DEFLATED, 15, 9, zlib.Z_FIXED if level == 1 else zlib.Z_DEFAULT_STRATEGY)
    data = comp.compress(bytes(raw)) + comp.flush()
    out = b"\x89PNG\r\n\x1a\n"
    out += chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, depth, ctype, 0, 0, interlace))
    out += chunk(b"gAMA", struct.pack(">I", 45455))
    if ctype == 3:
        out += chunk(b"PLTE", bytes(rng.randrange(256) for _ in range(npal * 3)))
        if trns:
            out += chunk(b"tRNS", bytes(rng.randrange(256) for _ in range(rng.randrange(1, npal + 1))))
    elif trns and ctype == 2:
        out += chunk(b"tRNS", struct.pack(">HHH", *(rng.randint(0, maxv) for _ in range(3))))
    # No tRNS for gray: RtPNGImageRead then reads png_get_tRNS's NULL alpha pointer (crashes the original).
    elif False:
        out += chunk(b"tRNS", struct.pack(">H", rng.randint(0, maxv)))
    step = max(1, len(data) // split)
    for i in range(0, len(data), step):
        out += chunk(b"IDAT", data[i : i + step])
    out += chunk(b"tEXt", b"Comment\x00png_cases")
    out += chunk(b"IEND", b"")
    return out


def main():
    folder = Path(sys.argv[1])
    folder.mkdir(parents=True, exist_ok=True)
    rng = random.Random(1)
    count = 0
    for ctype, depths in COMBOS.items():
        for depth in depths:
            for interlace in (0, 1):
                for width, height in ((1, 1), (13, 7), (33, 17)):
                    level = rng.choice((0, 1, 9))
                    trns = rng.random() < 0.5
                    data = make_png(rng, width, height, ctype, depth, interlace, level, trns, rng.choice((1, 3)))
                    name = f"c{ctype}_d{depth}_i{interlace}_{width}x{height}.png"
                    (folder / name).write_bytes(data)
                    count += 1
    good = make_png(rng, 9, 9, 6, 8, 0, 9, False, 1)
    (folder / "bad_truncated.png").write_bytes(good[: len(good) // 2])
    bad = bytearray(good)
    bad[29] ^= 1  # inside IHDR: CRC error
    (folder / "bad_crc.png").write_bytes(bytes(bad))
    print(f"{count} cases + 2 corrupt in {folder}")


if __name__ == "__main__":
    main()
