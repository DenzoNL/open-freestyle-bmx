/*
 * Included by the data files (src/data/<module>.c): the game's headers, for the globals' types, the
 * declarations of the definitions and the helpers of the generated code (docs/PORTING.md X2a). Generated once
 * by tools/gen_data.py.
 */
#ifndef BMX_DATA_DATA_H
#define BMX_DATA_DATA_H

#include <stddef.h>
#include <stdint.h>

/* zrwmodel.h and zrwzgl.h name 0x697720 / 0x697724 the other way round (gZrwScale, gZrwInvScale), and
 * gZrwFlags with different types: each module's own pair is right for its code, both are seen here. */
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmacro-redefined"
#include "bmx/com/action/action.h"
#include "bmx/com/action/camera.h"
#include "bmx/com/action/comdata.h"
#include "bmx/com/action/contrler.h"
#include "bmx/com/action/fx.h"
#include "bmx/com/action/game.h"
#include "bmx/com/action/memcard.h"
#include "bmx/com/action/overlay.h"
#include "bmx/com/action/overlymp.h"
#include "bmx/com/action/physics.h"
#include "bmx/com/action/physics_air.h"
#include "bmx/com/action/physics_hit.h"
#include "bmx/com/action/physics_types.h"
#include "bmx/com/action/prop.h"
#include "bmx/com/action/replay.h"
#include "bmx/com/action/rider.h"
#include "bmx/com/menu/frontend.h"
#include "bmx/com/menu/gui/gui.h"
#include "bmx/com/menu/gui/guiinput.h"
#include "bmx/com/menu/gui/guilabel.h"
#include "bmx/com/menu/gui/guimain.h"
#include "bmx/com/menu/gui/guisfile.h"
#include "bmx/com/menu/gui/guitoggl.h"
#include "bmx/com/zge/bmx/bmxsound.h"
#include "bmx/com/zge/bmx/zoverii.h"
#include "bmx/com/zge/ngif/missingzge.h"
#include "bmx/com/zge/ngif/ngapp.h"
#include "bmx/com/zge/ngif/ngiffx.h"
#include "bmx/com/zge/ngif/ngifpsx.h"
#include "bmx/com/zge/ngif/ngifzactor.h"
#include "bmx/com/zge/ngif/ngifzbsp.h"
#include "bmx/com/zge/ngif/ngifzcollide.h"
#include "bmx/com/zge/ngif/ngifzmemdmm.h"
#include "bmx/com/zge/ngif/ngifzres.h"
#include "bmx/com/zge/ngif/ngifzroller.h"
#include "bmx/com/zge/zactor.h"
#include "bmx/com/zge/zprim.h"
#include "bmx/com/zge/zsndseq.h"
#include "bmx/com/zge/zsndsrc.h"
#include "bmx/pc/zge/sound/winapi.h"
#include "bmx/pc/zge/sound/winchan.h"
#include "bmx/pc/zge/sound/winds.h"
#include "bmx/pc/zge/sound/winload.h"
#include "bmx/pc/zge/sound/winstrm.h"
#include "compat/crt.h"
#include "compat/orig.h"
#include "compat/trace.h"
#include "compat/x87.h"
#include "rw/rwanim.h"
#include "rw/rwcamera.h"
#include "rw/rwclump.h"
#include "rw/rwcollision.h"
#include "rw/rwcore.h"
#include "rw/rwd3d.h"
#include "rw/rwd3dclip.h"
#include "rw/rwim.h"
#include "rw/rwlight.h"
#include "rw/rwmath.h"
#include "rw/rwplugins.h"
#include "rw/rwstream.h"
#include "rw/rwtexture.h"
#include "rw/rwworld.h"
#include "zge/com/zglrw/zrwcam.h"
#include "zge/com/zglrw/zrwcol.h"
#include "zge/com/zglrw/zrwdbg.h"
#include "zge/com/zglrw/zrwfile.h"
#include "zge/com/zglrw/zrwlight.h"
#include "zge/com/zglrw/zrwmat.h"
#include "zge/com/zglrw/zrwmodel.h"
#include "zge/com/zglrw/zrwmodelcreate.h"
#include "zge/com/zglrw/zrwmodelgeom.h"
#include "zge/com/zglrw/zrwmodelrender.h"
#include "zge/com/zglrw/zrwprim.h"
#include "zge/com/zglrw/zrwramdisk.h"
#include "zge/com/zglrw/zrwshadow.h"
#include "zge/com/zglrw/zrwtxr.h"
#include "zge/com/zglrw/zrwworld.h"
#include "zge/com/zglrw/zrwzgl.h"
#include "zge/pc/pcinput.h"
#include "zge/pc/pcprofile.h"
#include "zge/zanim.h"
#include "zge/zasset.h"
#include "zge/zdebug.h"
#include "zge/zfile.h"
#include "zge/zge.h"
#include "zge/zgemods/zfont.h"
#include "zge/zgemods/zobjcamera.h"
#include "zge/zgemods/zobjlight.h"
#include "zge/zgemods/zobjmodel.h"
#include "zge/zgemods/zobjpemit.h"
#include "zge/zgemods/zprop.h"
#include "zge/zgemods/zramdisk.h"
#include "zge/zgemods/zscnpts.h"
#include "zge/zgemods/ztaginf.h"
#include "zge/ziff.h"
#include "zge/zinput.h"
#include "zge/zlist.h"
#include "zge/zmath.h"
#include "zge/zmem.h"
#include "zge/zmovie.h"
#include "zge/zobj.h"
#include "zge/zprofile.h"
#include "zge/zquat.h"
#include "zge/zscene.h"
#include "zge/zstring.h"
#include "zge/ztoken.h"
#pragma clang diagnostic pop
#include "native/native.h"

/* An unported, unreached original function that a pointer in the data names: reports and stops. */
#define BMX_DATA_UNSERVED(addr)                                                                              \
    static void unserved_##addr(void)                                                                        \
    {                                                                                                        \
        bmxOrigTrap(0x##addr##u, (uint32_t)(uintptr_t)__builtin_return_address(0));                          \
    }

/* A pointer the original keeps off its natural alignment (in a packed table). */
typedef struct __attribute__((packed)) BmxUnalignedPtr {
    void *p;
} BmxUnalignedPtr;

/* An original `jmp [import slot]` thunk that ported code calls by address: the same jump, through our slot.
 * `offset` is the 32-bit one; every member of bmxImports is a pointer, so on x86_64 it is twice that. */
#if defined(__aarch64__) && defined(__APPLE__)
/* arm64 (docs/PORTING.md "X5 part 1"): x16 is the intra-procedure-call scratch register. */
#define BMX_IMPORT_THUNK(addr, offset)                                                                                                 \
    __asm__(                                                                                                                           \
        ".text\n.p2align 2\n.globl " BMX_SYM("bmxImportThunk_" #addr) "\n" BMX_SYM("bmxImportThunk_" #addr) ":\n\tadrp x16, " BMX_SYM( \
            "bmxImports") "@PAGE\n\tadd x16, x16, " BMX_SYM("bmxImports") "@PAGEOFF\n\tldr x16, [x16, "                                \
                                                                          "#2*" #offset "]\n\tbr x16\n")
#elif defined(__aarch64__)
#define BMX_IMPORT_THUNK(addr, offset)                                                                       \
    __asm__(".text\n.p2align 2\n.globl bmxImportThunk_" #addr "\nbmxImportThunk_" #addr                      \
            ":\n\tadrp x16, bmxImports\n\tadd x16, x16, :lo12:bmxImports\n\tldr x16, [x16, #2*" #offset      \
            "]\n\tbr x16\n")
#elif defined(__x86_64__)
#define BMX_IMPORT_THUNK(addr, offset)                                                                       \
    __asm__(".globl bmxImportThunk_" #addr "\nbmxImportThunk_" #addr ":\n\tjmp *bmxImports+2*" #offset       \
            "(%rip)\n")
#else
#define BMX_IMPORT_THUNK(addr, offset)                                                                       \
    __asm__(".globl _bmxImportThunk_" #addr "\n_bmxImportThunk_" #addr ":\n\tjmp *_bmxImports+" #offset "\n")
#endif

#include "data/globals.h"

#endif
