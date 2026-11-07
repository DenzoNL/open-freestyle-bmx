"""Compares the screenshots of two builds with a tolerance (docs/PORTING.md X3, "Screenshot comparison").

    py -I tools/bmpdiff.py <dir A> <dir B> [threshold]

Takes every .bmp in dir A that dir B also has (BMX_SHOT files, e.g. from `tools/xcheck.py --shots`, moved into
one folder per build) and counts the pixels whose largest channel difference is at most the threshold (default
24: rounding, x87 vs SSE) and above it. A shot with more than 200 pixels above it is marked CHECK, with the box
around them (y counted from the bottom, BMP order). Exit status 1 when any shot is marked.
"""
import os
import struct
import sys


def load(path):
    b = open(path, "rb").read()
    (off,) = struct.unpack_from("<I", b, 10)
    w, h = struct.unpack_from("<ii", b, 18)
    (bpp,) = struct.unpack_from("<H", b, 28)
    return w, abs(h), bpp // 8, b[off:]


def compare(a, b, thr):
    wa, ha, pa, da = load(a)
    wb, hb, pb, db = load(b)
    if (wa, ha, pa) != (wb, hb, pb):
        return None
    stride = (wa * pa + 3) & ~3
    small = big = 0
    box = [wa, ha, -1, -1]
    for y in range(ha):
        ra = da[y * stride : y * stride + wa * pa]
        rb = db[y * stride : y * stride + wa * pa]
        if ra == rb:
            continue
        for x in range(wa):
            i = x * pa
            d = max(abs(ra[i] - rb[i]), abs(ra[i + 1] - rb[i + 1]), abs(ra[i + 2] - rb[i + 2]))
            if d == 0:
                continue
            if d <= thr:
                small += 1
            else:
                big += 1
                box = [min(box[0], x), min(box[1], y), max(box[2], x), max(box[3], y)]
    return small, big, box


def main():
    if len(sys.argv) < 3:
        sys.exit(__doc__)
    dir_a, dir_b = sys.argv[1], sys.argv[2]
    thr = int(sys.argv[3]) if len(sys.argv) > 3 else 24
    marked = 0
    for name in sorted(os.listdir(dir_a)):
        if not name.endswith(".bmp") or not os.path.exists(os.path.join(dir_b, name)):
            continue
        r = compare(os.path.join(dir_a, name), os.path.join(dir_b, name), thr)
        if r is None:
            print(f"CHECK {name}: sizes differ")
            marked += 1
            continue
        small, big, box = r
        flag = "CHECK" if big > 200 else "ok"
        marked += flag == "CHECK"
        where = f" in x {box[0]}..{box[2]}, y {box[1]}..{box[3]} (from the bottom)" if big else ""
        print(f"{flag:5} {name}: {small} small, {big} big{where}")
    sys.exit(1 if marked else 0)


if __name__ == "__main__":
    main()
