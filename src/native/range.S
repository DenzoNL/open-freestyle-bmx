/*
 * Claims 0x400000..0x6b3000 for the original image. The Windows loader maps code page tables and other
 * system sections at the lowest free addresses before any of our code runs, so the range cannot be
 * reserved afterwards (VirtualAlloc fails with ERROR_INVALID_ADDRESS). Instead this exe is linked at a
 * base such that the first bytes of .text (a zero-filled block sorted first) cover exactly that range; the loader then only
 * changes its protection and fills it. (lld places .text first and keeps object order within it, so this object must be first on the link line.) See docs/NATIVE.md.
 *
 * Original image: base 0x400000, SizeOfImage 0x2b3000.
 */
        .text
        .globl _bmxOrigRange
        .balign 0x10000
_bmxOrigRange:
        .space 0x2b3000
