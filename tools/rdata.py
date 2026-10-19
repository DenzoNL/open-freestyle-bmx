#!/usr/bin/env python3
"""Print constants from NgBMX.exe as float32/float64/int so ports can use exact literals.

Usage: tools/rdata.py NgBMX.exe 0x54d048 0x54dcc4 ...
"""
import struct
import sys

import pefile


def main():
    pe = pefile.PE(sys.argv[1], fast_load=True)
    base = pe.OPTIONAL_HEADER.ImageBase
    for a in sys.argv[2:]:
        va = int(a, 16)
        raw = pe.get_data(va - base, 8)
        f = struct.unpack_from("<f", raw)[0]
        d = struct.unpack_from("<d", raw)[0]
        u = struct.unpack_from("<I", raw)[0]
        print(f"{va:#010x}  f32={f!r:<24} ({f.hex() if False else float(f).hex()})  f64={d!r:<24}  u32={u:#010x}")


if __name__ == "__main__":
    main()
