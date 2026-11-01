/*
 * The original's _rwdseg (RenderWare data), 0x006a0000..0x006a0109, as one object with the original's
 * layout (src/data/layout.h, docs/PORTING.md X1b): code relies on globals being next to each other (the
 * save data copied as one range, tables used past their declared ends), so the members keep their
 * original offsets; ORIG_VAR(addr) reaches into it (src/compat/orig.h).
 *
 * Generated once by tools/gen_data.py from NgBMX.exe; from here on it is source like the code.
 */
#include "data/data.h"

struct BmxRwBss bmxRwBss __attribute__((aligned(4096)));
