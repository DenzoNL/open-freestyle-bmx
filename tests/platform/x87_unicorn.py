#!/usr/bin/env python3
"""Checks compat/x87soft.c's arithmetic against the x87 as Unicorn emulates it (docs/PORTING.md "X5 part 2").

Unicorn's x87 is QEMU's softfloat floatx80 with precision and rounding control: a good reference for
FADD / FSUB / FMUL / FSQRT, the stores and the NaN rules, and so for bmxX87Rotate / bmxX87MatrixMultiply /
bmxX87HalfPiScaled. Its FSIN / FCOS / FPTAN / FPATAN call the host's double libm, so the transcendental
helpers are not compared here (tests/platform/x87_mpfr.c and test_x87.c on x86 cover them).

Builds the real sequences (tests/platform/x87_hwshim.c) for i386, the soft ones as a host library, and
compares them on random and edge operands under every precision / rounding control. Run in `nix develop`:
    python3 tests/platform/x87_unicorn.py [cases]
"""

import ctypes
import os
import random
import struct
import subprocess
import sys

import unicorn
from unicorn import x86_const as X

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
OUT = os.path.join(ROOT, "build", "x87")
BASE = 0x100000
STACK = 0x800000
DATA = 0x600000
RET = 0x700000


def build():
    os.makedirs(OUT, exist_ok=True)
    cc = os.environ.get("OPENDMFBMX_CROSS_CLANG", "clang")
    obj, elf = (os.path.join(OUT, n) for n in ("shim.o", "shim.elf"))
    subprocess.check_call([cc, "-target", "i686-linux-gnu", "-O2", "-msse2", "-mfpmath=sse", "-ffreestanding",
                           "-fno-pic", "-I", os.path.join(ROOT, "src"), "-DBMX_HOOK=1", "-c",
                           os.path.join(ROOT, "tests/platform/x87_hwshim.c"), "-o", obj])
    subprocess.check_call(["ld.lld", "-m", "elf_i386", "--no-pie", "-static", "-e", "0", "--image-base",
                           hex(BASE), "-Ttext", hex(BASE + 0x1000), obj, "-o", elf])
    syms = {}
    for line in subprocess.check_output(["llvm-nm", elf], text=True).splitlines():
        parts = line.split()
        if len(parts) == 3:
            syms[parts[2]] = int(parts[0], 16)
    lib = os.path.join(OUT, "libx87soft.dylib" if sys.platform == "darwin" else "libx87soft.so")
    subprocess.check_call(["cc", "-O2", "-shared", "-fPIC", "-ffp-contract=off", "-I", os.path.join(ROOT, "src"),
                           os.path.join(ROOT, "src/compat/x87soft.c"), "-o", lib])
    data = open(elf, "rb").read()
    phoff, = struct.unpack_from("<I", data, 0x1C)
    phentsize, phnum = struct.unpack_from("<HH", data, 0x2A)
    segments = []
    for i in range(phnum):
        ptype, off, vaddr, _, filesz, _ = struct.unpack_from("<6I", data, phoff + i * phentsize)
        if ptype == 1:
            segments.append((vaddr, data[off:off + filesz]))
    return segments, syms, lib


class Hw:
    def __init__(self, segments, syms):
        self.mu = unicorn.Uc(unicorn.UC_ARCH_X86, unicorn.UC_MODE_32)
        self.mu.mem_map(BASE, 0x100000)
        for vaddr, blob in segments:
            self.mu.mem_write(vaddr, blob)
        self.mu.mem_map(DATA, 0x10000)
        self.mu.mem_map(RET, 0x1000)
        self.mu.mem_map(STACK - 0x10000, 0x10000)
        self.syms = syms

    def call(self, name, *ptrs):
        sp = STACK - 0x100
        self.mu.mem_write(sp, struct.pack("<I" + "I" * len(ptrs), RET, *ptrs))
        self.mu.reg_write(X.UC_X86_REG_ESP, sp)
        self.mu.emu_start(self.syms[name], RET)

    def write(self, off, data):
        self.mu.mem_write(DATA + off, data)

    def read(self, off, n):
        return bytes(self.mu.mem_read(DATA + off, n))


class Reg(ctypes.Structure):
    _fields_ = [("m", ctypes.c_uint64), ("se", ctypes.c_uint16)]


def soft_lib(path):
    lib = ctypes.CDLL(path)
    lib.bmxX87SoftArith.restype = Reg
    lib.bmxX87SoftArith.argtypes = [ctypes.c_int, Reg, Reg]
    lib.bmxX87SoftStoreFloat.restype = ctypes.c_float
    lib.bmxX87SoftStoreFloat.argtypes = [Reg]
    lib.bmxX87SoftStoreDouble.restype = ctypes.c_double
    lib.bmxX87SoftStoreDouble.argtypes = [Reg]
    lib.bmxX87SoftRotate.argtypes = [ctypes.POINTER(ctypes.c_float)] * 2 + [ctypes.c_float] * 2 + \
        [ctypes.POINTER(ctypes.c_float)] * 2
    lib.bmxX87SoftMatrixMultiply.argtypes = [ctypes.POINTER(ctypes.c_float)] * 3
    lib.bmxX87SoftHalfPiScaled.restype = ctypes.c_double
    lib.bmxX87SoftHalfPiScaled.argtypes = [ctypes.c_int, ctypes.c_float]
    return lib


CWS = [0x007f | pc << 8 | rc << 10 for pc in (0, 2, 3) for rc in range(4)]


def rand_reg(rng):
    kind = rng.random()
    sign = rng.getrandbits(1) << 15
    if kind < 0.03:
        return 0, sign
    if kind < 0.05:
        return 1 << 63, sign | 0x7FFF
    if kind < 0.09:  # NaNs, signalling or quiet
        return (1 << 63) | rng.getrandbits(62) | (rng.getrandbits(1) << 62) | 1, sign | 0x7FFF
    if kind < 0.12:  # extended denormal
        return rng.getrandbits(62) | 1, sign
    if kind < 0.30:  # a float
        return (1 << 63) | (rng.getrandbits(23) << 40), sign | (16383 + rng.randint(-150, 130))
    if kind < 0.50:  # a double
        return (1 << 63) | (rng.getrandbits(52) << 11), sign | (16383 + rng.randint(-1080, 1030))
    if kind < 0.55:  # the edges of the extended range
        return (1 << 63) | rng.getrandbits(63), sign | rng.choice([1, 2, 0x7FFD, 0x7FFE])
    return (1 << 63) | rng.getrandbits(63), sign | (16383 + rng.randint(-70, 70))


def near(rng, r):
    """An operand close to r (cancellation in FSUB / FADD)."""
    m, se = r
    if (se & 0x7FFF) in (0, 0x7FFF):
        return rand_reg(rng)
    m = (m + rng.randint(-1 << rng.randint(0, 40), 1 << rng.randint(0, 40))) & ((1 << 64) - 1)
    m |= 1 << 63
    return m, (se & 0x7FFF) | (rng.getrandbits(1) << 15)


def rand_float(rng):
    """The 4 bytes of a float (bytes, so NaN payloads reach both sides unchanged)."""
    k = rng.random()
    if k < 0.1:
        return struct.pack("<I", rng.getrandbits(32))
    if k < 0.15:
        return struct.pack("<I", rng.choice([0, 0x80000000, 0x7F800000, 0xFF800000, 0x7FC00000, 0xFFC00000,
                                             0x7FA00000, 1, 0x80000001, 0x7F7FFFFF, 0xFF7FFFFF]))
    return struct.pack("<f", rng.uniform(-2, 2) * 2.0 ** rng.randint(-20, 20))


def fbits(v):
    return struct.pack("<f", v)


def main():
    cases = int(sys.argv[1]) if len(sys.argv) > 1 else 20000
    segments, syms, libpath = build()
    hw = Hw(segments, syms)
    lib = soft_lib(libpath)
    cw_var = ctypes.c_uint16.in_dll(lib, "bmxX87SoftCw")
    rng = random.Random(1)
    fails = 0

    def fail(msg):
        nonlocal fails
        fails += 1
        if fails <= 30:
            print("FAIL", msg)

    # single instructions
    for i in range(cases):
        cw = rng.choice(CWS)
        op = i % 4
        a = rand_reg(rng)
        b = near(rng, a) if rng.random() < 0.3 else rand_reg(rng)
        if op == 3 and a[1] & 0x8000 and rng.random() < 0.8:
            a = (a[0], a[1] & 0x7FFF)
        hw.write(0, struct.pack("<H", cw))
        hw.write(0x10, struct.pack("<QH", *a))
        hw.write(0x20, struct.pack("<QH", *b))
        hw.write(0x30, struct.pack("<i", op))
        hw.call("hwOp", DATA, DATA + 0x10, DATA + 0x20, DATA + 0x30, DATA + 0x40)
        out = hw.read(0x40, 22)
        cw_var.value = cw
        r = lib.bmxX87SoftArith(op, Reg(*a), Reg(*b))
        got = struct.pack("<QH", r.m, r.se) + fbits(lib.bmxX87SoftStoreFloat(r)) + \
            struct.pack("<d", lib.bmxX87SoftStoreDouble(r))
        if got != out:
            fail("op %d cw %04x a %04x:%016x b %04x:%016x hw %s soft %s" % (op, cw, a[1], a[0], b[1], b[0],
                                                                           out.hex(), got.hex()))
    print("arithmetic: %d cases" % cases)

    # the helpers
    for i in range(cases // 4):
        cw = rng.choice(CWS + [0x027F, 0x047F] * 4)
        cw_var.value = cw
        hw.write(0, struct.pack("<H", cw))
        raw = b"".join(rand_float(rng) for _ in range(32))
        # Unicorn's FLD m32 keeps a signalling NaN signalling (QEMU 5.0 softfloat; the SDM and later QEMU
        # quieten it), so the FLD-loaded operands (rotate's four, the matrix's a) get no signalling NaNs. The
        # matrix's b are FMUL / FADD memory operands, where a signalling NaN stays signalling on the x87 too.
        raw = bytearray(raw)
        for k in range(16):
            w, = struct.unpack_from("<I", raw, 4 * k)
            if (w & 0x7F800000) == 0x7F800000 and (w & 0x7FFFFF) and not (w & 0x400000):
                struct.pack_into("<I", raw, 4 * k, w | 0x400000)
        raw = bytes(raw)
        hw.write(0x10, raw)
        hw.call("hwRotate", DATA, DATA + 0x10, DATA + 0x100)
        fa = (ctypes.c_float * 32).from_buffer_copy(raw)
        f0, f1 = ctypes.c_float.from_buffer_copy(raw, 0), ctypes.c_float.from_buffer_copy(raw, 4)
        r = (ctypes.c_float * 2)()
        fp = ctypes.POINTER(ctypes.c_float)
        lib.bmxX87SoftRotate(ctypes.cast(r, fp), ctypes.cast(ctypes.byref(r, 4), fp), f0, f1,
                             ctypes.cast(ctypes.byref(fa, 8), fp), ctypes.cast(ctypes.byref(fa, 12), fp))
        if hw.read(0x100, 8) != bytes(r):
            fail("rotate cw %04x in %s hw %s soft %s" % (cw, bytes(fa)[:16].hex(), hw.read(0x100, 8).hex(),
                                                         bytes(r).hex()))
        hw.write(0x200, bytes(64))
        hw.call("hwMatrix", DATA, DATA + 0x10, DATA + 0x200)
        o = (ctypes.c_float * 16)()
        lib.bmxX87SoftMatrixMultiply(ctypes.cast(o, fp), ctypes.cast(fa, fp), ctypes.cast(ctypes.byref(fa, 64), fp))
        if hw.read(0x200, 64) != bytes(o):
            fail("matrix cw %04x hw %s soft %s" % (cw, hw.read(0x200, 64).hex(), bytes(o).hex()))
        hw.call("hwHalfPi", DATA, DATA + 0x10, DATA + 0x300)
        want = hw.read(0x300, 16)
        got = struct.pack("<dd", lib.bmxX87SoftHalfPiScaled(0, f0), lib.bmxX87SoftHalfPiScaled(1, f0))
        if want != got:
            fail("halfpi cw %04x scale %s hw %s soft %s" % (cw, raw[:4].hex(), want.hex(), got.hex()))
    print("rotate / matrix / halfpi: %d cases" % (cases // 4))
    print("%d failures" % fails)
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(main())
