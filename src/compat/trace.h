#ifndef BMX_COMPAT_TRACE_H
#define BMX_COMPAT_TRACE_H

/*
 * Gameplay trace hook points (hook build only). The in-game dinput.dll sets these when tracing is
 * switched on (see src/hook/trace.c); otherwise they stay NULL and cost one load per call. The
 * differential-test DLL never sets them.
 */
#ifdef BMX_HOOK
/* Called at the start of every controller poll (once per gameplay frame). */
extern void (*gBmxTracePoll)(void);
#define BMX_TRACE_POLL()                                                                                     \
    do {                                                                                                     \
        if (gBmxTracePoll != 0) {                                                                            \
            gBmxTracePoll();                                                                                 \
        }                                                                                                    \
    } while (0)
#else
#define BMX_TRACE_POLL() ((void)0)
#endif

#endif
