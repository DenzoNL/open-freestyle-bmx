/*
 * Script tokenizer (the original file is unknown; its assert text says ZTOKEN). Reads whitespace and
 * comma separated tokens from text files, with quoted strings, ; and // line comments, nested
 * block comments, include files (#INCLUDE, #INCLUDE_PATH) and a small macro language: $name symbols
 * defined with #DEFINE and expanded in place, plus $*, $+, $-, $/ arithmetic on two operands.
 */
#ifndef ZGE_ZTOKEN_H
#define ZGE_ZTOKEN_H

#include <stdint.h>

#include "compat/orig.h"
#include "zge/zfile.h"

enum { kZtokenMaxDepth = 3, kZtokenLineBytes = 0x100, kZtokenSymbolBytes = 0x400 };

/* One open file of the include stack (0x30c bytes). */
typedef struct ZtokenFrame {
    ZfileHandle *file;           /* 0x000 */
    char name[0x100];            /* 0x004 */
    char line[kZtokenLineBytes]; /* 0x104 the current line, as read (newline included) */
    uint16_t cursor;             /* 0x204 offset into line */
    uint8_t pad206[2];
    int32_t lineNumber;     /* 0x208 */
    char includeDir[0x100]; /* 0x20c prefix for #INCLUDE file names */
} ZtokenFrame;

BMX_LAYOUT32_ASSERT(sizeof(ZtokenFrame) == 0x30c, "token frame layout");

/* Globals from 0x675380: the frames, then the settings and the symbol table. */
typedef struct ZtokenState {
    ZtokenFrame frames[kZtokenMaxDepth + 1]; /* 0x675380 */
    ZtokenFrame *current;                    /* 0x675fb0 */
    int32_t depth;                           /* 0x675fb4 */
    int32_t debug;                           /* 0x675fb8 echo every token */
    int32_t upperCase;                       /* 0x675fbc fold tokens to upper case */
    int32_t hashComments;                    /* 0x675fc0 lines starting with # are comments */
    int32_t macros;                          /* 0x675fc4 process #INCLUDE / $ macros */
    char symbols[kZtokenSymbolBytes];        /* 0x675fc8 "$name@value" entries back to back */
    uint32_t symbolBytes;                    /* 0x6763c8 used part of symbols */
    uint32_t flags;                          /* 0x6763cc bit 0: last token was quoted */
} ZtokenState;

#define gToken ORIG_VAR(0x00675380, ZtokenState, gToken)

/* Opens a file (frame 0), resetting the settings and the symbol table; returns 1 on success. */
int ztokenOpen(const char *path);
int ztokenClose(void);
/* Reads the next token into out (at most size - 1 characters); 0 at the end of the file or on error. */
int ztokenNext(char *out, int size);
/* Defines (or redefines) "$name" = value; 0 when the symbol table is full. */
int ztokenDefine(const char *name, const char *value);
/* Replaces name ("$name") by its value in place; returns the table position + 1, or 0 when undefined. */
int ztokenLookup(char *name);
/* Reads an integer token (decimal, 0x.. or $.. hex). */
int ztokenReadInt(int *value);
/* Reads a token for its text only. */
int ztokenReadString(char *out, int size);
/* Reads a token and checks it against `expected` (which is upper cased in place). */
int ztokenExpect(char *expected);
/* Prints "File:..,  Line:.." for every open file. */
void ztokenPrintTrace(void);

#endif
