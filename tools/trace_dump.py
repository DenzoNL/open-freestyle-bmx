#!/usr/bin/env python3
"""Dump a gameplay trace written by the hook DLL (src/hook/trace.c).

usage: trace_dump.py TRACE.bin [--frames A:B] [--summary]
"""
import argparse
import struct
import sys

PAD = struct.Struct("<HBBI4I")  # flags, present, status, held, raw axes
RIDER = struct.Struct("<BB4sIh3f3f3f3f3hi")  # state, surface, levels, flags, timer, pos, vel, fwd, up, rot, spin
FRAME_HEAD = struct.Struct("<II")  # frame, random seed
CAMERA = struct.Struct("<B3f3f")  # player, eye, lookAt


def read(path):
    data = open(path, "rb").read()
    if data[:4] != b"BMXT":
        sys.exit(f"{path}: not a trace")
    version, mode, vec, ang, riders = struct.unpack_from("<5I", data, 4)
    if (version, vec, ang) != (2, 12, 6):
        sys.exit(f"{path}: unsupported trace (version {version}, vec {vec}, angle {ang})")
    print(f"{path}: {'original game' if mode == 1 else 'our port' if mode == 2 else f'mode {mode}'}")
    pos = 24
    times = []
    frames = []
    other = 0
    while pos < len(data):
        tag = data[pos]
        pos += 1
        if tag in (ord("T"), ord("t")):
            if pos + 4 > len(data):
                break
            (t,) = struct.unpack_from("<I", data, pos)
            pos += 4
            if tag == ord("T"):
                times.append(t)
            else:
                other += 1
        elif tag == ord("F"):
            size = FRAME_HEAD.size + 2 * PAD.size + CAMERA.size + riders * RIDER.size
            if pos + size > len(data):
                break  # truncated tail (game crashed or was killed)
            frame, seed = FRAME_HEAD.unpack_from(data, pos)
            p = pos + FRAME_HEAD.size
            pads = []
            for _ in range(2):
                pads.append(PAD.unpack_from(data, p))
                p += PAD.size
            cam = CAMERA.unpack_from(data, p)
            p += CAMERA.size
            rs = []
            for _ in range(riders):
                rs.append(RIDER.unpack_from(data, p))
                p += RIDER.size
            frames.append({"frame": frame, "seed": seed, "pads": pads, "camera": cam, "riders": rs,
                           "times": times})
            times = []
            pos += size
        else:
            sys.exit(f"{path}: bad tag {tag:#x} at offset {pos - 1:#x}")
    return frames, other


def f32(bits):
    return struct.unpack("<f", struct.pack("<I", bits))[0]


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("trace")
    ap.add_argument("--frames", help="A:B range of frames to print")
    ap.add_argument("--summary", action="store_true", help="only print totals")
    a = ap.parse_args()
    frames, other = read(a.trace)
    ntimes = sum(len(f["times"]) for f in frames)
    print(f"{len(frames)} frames, {ntimes} game-thread time reads, {other} from other threads")
    if a.summary or not frames:
        return
    lo, hi = 0, len(frames)
    if a.frames:
        s, e = a.frames.split(":")
        lo, hi = int(s or 0), int(e or len(frames))
    for f in frames[lo:hi]:
        pads = " ".join(f"[{p[3]:08x} {' '.join(f'{f32(x):+.2f}' for x in p[4:8])}]" for p in f["pads"])
        r = f["riders"][0]
        print(f"#{f['frame']:6} seed {f['seed']:08x} t={f['times'][:3]} pads {pads}")
        print(f"         rider0 state {r[0]} surf {r[1]} pos ({r[5]:.3f}, {r[6]:.3f}, {r[7]:.3f}) "
              f"vel ({r[8]:.3f}, {r[9]:.3f}, {r[10]:.3f}) spin {r[-1]}")


if __name__ == "__main__":
    main()
