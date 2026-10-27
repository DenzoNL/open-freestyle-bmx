/*
 * CRT routines with the MSVC compiler-helper calling conventions (no C prototype can express them),
 * copied instruction for instruction from the original: the code of each is in
 * work/bundles-rw/<addr>.json. crt.c puts a jump from the original address to each. See docs/NATIVE.md.
 */
        .text

/* _ftol (0x4a4a80): ST0 in, truncated int64 out in EDX:EAX (rounding mode forced to chop meanwhile). */
        .globl _bmxCrtFtol
_bmxCrtFtol:
        pushl   %ebp
        movl    %esp, %ebp
        addl    $-12, %esp
        fstcw   -2(%ebp)
        movw    -2(%ebp), %ax
        orb     $0x0c, %ah
        movw    %ax, -4(%ebp)
        fldcw   -4(%ebp)
        fistpll -12(%ebp)
        fldcw   -2(%ebp)
        movl    -12(%ebp), %eax
        movl    -8(%ebp), %edx
        leave
        ret

/* _CIsqrt (0x4a5390): ST0 in, ST0 out. */
        .globl _bmxCrtCIsqrt
_bmxCrtCIsqrt:
        fsqrt
        ret

/* __chkstk / _alloca_probe (0x4a4d50): EAX = bytes; touches the pages, moves ESP down, returns to the caller. */
        .globl _bmxCrtChkstk
_bmxCrtChkstk:
        pushl   %ecx
        cmpl    $0x1000, %eax
        leal    8(%esp), %ecx
        jb      2f
1:
        subl    $0x1000, %ecx
        subl    $0x1000, %eax
        testl   %eax, (%ecx)
        cmpl    $0x1000, %eax
        jae     1b
2:
        subl    %eax, %ecx
        movl    %esp, %eax
        testl   %eax, (%ecx)
        movl    %ecx, %esp
        movl    (%eax), %ecx
        movl    4(%eax), %eax
        pushl   %eax
        ret

/* __allmul (0x4ab3b0): 64-bit multiply, arguments on the stack (a lo/hi, b lo/hi), callee pops 16. */
        .globl _bmxCrtAllmul
_bmxCrtAllmul:
        movl    8(%esp), %eax
        movl    16(%esp), %ecx
        orl     %eax, %ecx
        movl    12(%esp), %ecx
        jnz     1f
        movl    4(%esp), %eax
        mull    %ecx
        ret     $16
1:
        pushl   %ebx
        mull    %ecx
        movl    %eax, %ebx
        movl    8(%esp), %eax
        mull    20(%esp)
        addl    %eax, %ebx
        movl    8(%esp), %eax
        mull    %ecx
        addl    %ebx, %edx
        popl    %ebx
        ret     $16

/* __aulldiv (0x4ad3f0): unsigned 64-bit divide, arguments on the stack, callee pops 16. */
        .globl _bmxCrtAulldiv
_bmxCrtAulldiv:
        pushl   %ebx
        pushl   %esi
        movl    24(%esp), %eax
        orl     %eax, %eax
        jnz     1f
        movl    20(%esp), %ecx
        movl    16(%esp), %eax
        xorl    %edx, %edx
        divl    %ecx
        movl    %eax, %ebx
        movl    12(%esp), %eax
        divl    %ecx
        movl    %ebx, %edx
        jmp     4f
1:
        movl    %eax, %ecx
        movl    20(%esp), %ebx
        movl    16(%esp), %edx
        movl    12(%esp), %eax
2:
        shrl    $1, %ecx
        rcrl    $1, %ebx
        shrl    $1, %edx
        rcrl    $1, %eax
        orl     %ecx, %ecx
        jnz     2b
        divl    %ebx
        movl    %eax, %esi
        mull    24(%esp)
        movl    %eax, %ecx
        movl    20(%esp), %eax
        mull    %esi
        addl    %ecx, %edx
        jc      3f
        cmpl    16(%esp), %edx
        ja      3f
        jc      5f
        cmpl    12(%esp), %eax
        jbe     5f
3:
        decl    %esi
5:
        xorl    %edx, %edx
        movl    %esi, %eax
4:
        popl    %esi
        popl    %ebx
        ret     $16

/* __aullrem (0x4ad460): unsigned 64-bit remainder, arguments on the stack, callee pops 16. */
        .globl _bmxCrtAullrem
_bmxCrtAullrem:
        pushl   %ebx
        movl    20(%esp), %eax
        orl     %eax, %eax
        jnz     1f
        movl    16(%esp), %ecx
        movl    12(%esp), %eax
        xorl    %edx, %edx
        divl    %ecx
        movl    8(%esp), %eax
        divl    %ecx
        movl    %edx, %eax
        xorl    %edx, %edx
        jmp     4f
1:
        movl    %eax, %ecx
        movl    16(%esp), %ebx
        movl    12(%esp), %edx
        movl    8(%esp), %eax
2:
        shrl    $1, %ecx
        rcrl    $1, %ebx
        shrl    $1, %edx
        rcrl    $1, %eax
        orl     %ecx, %ecx
        jnz     2b
        divl    %ebx
        movl    %eax, %ecx
        mull    20(%esp)
        xchgl   %ecx, %eax
        mull    16(%esp)
        addl    %ecx, %edx
        jc      3f
        cmpl    12(%esp), %edx
        ja      3f
        jc      5f
        cmpl    8(%esp), %eax
        jbe     5f
3:
        subl    16(%esp), %eax
        sbbl    20(%esp), %edx
5:
        subl    8(%esp), %eax
        sbbl    12(%esp), %edx
        negl    %edx
        negl    %eax
        sbbl    $0, %edx
4:
        popl    %ebx
        ret     $16

/* __allshl (0x4ae8d0): EDX:EAX <<= CL (count of 64 or more gives 0). Register convention, no stack arguments. */
        .globl _bmxCrtAllshl
_bmxCrtAllshl:
        cmpb    $64, %cl
        jae     2f
        cmpb    $32, %cl
        jae     1f
        shldl   %cl, %eax, %edx
        shll    %cl, %eax
        ret
1:
        movl    %eax, %edx
        xorl    %eax, %eax
        andb    $0x1f, %cl
        shll    %cl, %edx
        ret
2:
        xorl    %eax, %eax
        xorl    %edx, %edx
        ret

/*
 * _setjmp3 (0x51e008) and _longjmp (0x51e084) with the MSVC jmp_buf layout (EBP, EBX, EDI, ESI, ESP, EIP at
 * 0x00..0x14). The original's SEH registration chain and unwind handling is not needed here: the only
 * user, libpng, longjmps out of plain C frames.
 */
        .globl _bmxCrtSetjmp3
_bmxCrtSetjmp3:
        movl    4(%esp), %edx
        movl    %ebp, 0(%edx)
        movl    %ebx, 4(%edx)
        movl    %edi, 8(%edx)
        movl    %esi, 12(%edx)
        movl    %esp, 16(%edx)
        movl    (%esp), %eax
        movl    %eax, 20(%edx)
        movl    $0, 24(%edx)
        movl    $0, 28(%edx)
        movl    $0, 32(%edx)
        movl    $0, 36(%edx)
        xorl    %eax, %eax
        ret

        .globl _bmxCrtLongjmp
_bmxCrtLongjmp:
        movl    4(%esp), %edx
        movl    8(%esp), %eax
        movl    0(%edx), %ebp
        movl    4(%edx), %ebx
        movl    8(%edx), %edi
        movl    12(%edx), %esi
        cmpl    $1, %eax
        adcl    $0, %eax
        movl    16(%edx), %esp
        addl    $4, %esp
        jmp     *20(%edx)
