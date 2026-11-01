/*
 * The original's .data, zero-initialised tail, 0x0056a000..0x00698f90, as one object with the original's
 * layout (src/data/layout.h, docs/PORTING.md X1b): code relies on globals being next to each other (the
 * save data copied as one range, tables used past their declared ends), so the members keep their
 * original offsets; ORIG_VAR(addr) reaches into it (src/compat/orig.h).
 *
 * Generated once by tools/gen_data.py from NgBMX.exe; from here on it is source like the code.
 */
#include "data/data.h"

struct BmxBss bmxBss __attribute__((aligned(4096)));
