/*
 * The port's name as players see it (docs/RELEASE-PLAN.md D5): the window title, the Android app's name and
 * log tag, the settings and log files next to the program. Code identifiers (bmx*, BMX_* variables, the CMake
 * target bmxnative) keep their names.
 */
#ifndef BMX_COMPAT_PRODUCT_H
#define BMX_COMPAT_PRODUCT_H

#define BMX_PRODUCT_NAME "Open Freestyle BMX"
/* File names: OpenFreestyleBMX.exe (CMakeLists.txt), .ini, .log. */
#define BMX_PRODUCT_FILE "OpenFreestyleBMX"
#define BMX_PRODUCT_INI  BMX_PRODUCT_FILE ".ini"
#define BMX_PRODUCT_LOG  BMX_PRODUCT_FILE ".log"
/* The settings file's name before the rename (2026-11-07): still read when the new one is missing. */
#define BMX_PRODUCT_OLD_INI "bmxnative.ini"

#endif
