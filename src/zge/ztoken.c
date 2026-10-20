/* Script tokenizer (original file unknown, see ztoken.h). */
#include "zge/ztoken.h"

#include "compat/crt.h"
#include "zge/zdebug.h"
#include "zge/zstring.h"

#define PRINT gZdebugPrint

static int isSeparator(char c)
{
    return c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == ',';
}

static void upperCase(char *s)
{
    uint8_t c = (uint8_t)*s;

    while (c != 0) {
        c = (uint8_t)*s;
        if (c > 0x60 && c < 0x7b) {
            *s = (char)(c - 0x20);
        }
        s++;
        c = (uint8_t)*s;
    }
}

static void printClosed(void)
{
    if (gToken.debug != 0) {
        PRINT("\n*** Closed File ***\n\n");
    }
}

/* Makes the frame the current one after it has been opened. */
static int startFrame(ZtokenFrame *f, const char *path)
{
    if (f->file == 0) {
        return 0;
    }
    f->cursor = 0;
    f->line[0] = '\0';
    f->lineNumber = 0;
    zstrcpy(f->name, path);
    if (gToken.debug != 0) {
        PRINT("\n*** Opened File %s ***\n\n", path);
    }
    return 1;
}

/* Scans an operand token: 0x.. or $.. is hexadecimal, anything else decimal. */
static void scanOperand(const char *tok, int *dst)
{
    if (zstrncmp(tok, "0X", 2) == 0 || zstrncmp(tok, "0x", 2) == 0) {
        sscanf(tok + 2, "%X", dst);
    } else if (zstrncmp(tok, "$", 1) == 0) {
        sscanf(tok + 1, "%X", dst);
    } else {
        sscanf(tok, "%d", dst);
    }
}

BMX_ADDR(0x004a1120)
int ztokenOpen(const char *path)
{
    ZtokenFrame *f = &gToken.frames[0];

    gToken.depth = 0;
    gToken.current = f;
    gToken.debug = 0;
    gToken.upperCase = 1;
    gToken.hashComments = 0;
    gToken.macros = 1;
    f->includeDir[0] = '\0';
    gToken.symbolBytes = 0;
    f->file = zfileOpen(path, 1, 0);
    return startFrame(f, path);
}

BMX_ADDR(0x004a11e0)
int ztokenClose(void)
{
    gToken.current->lineNumber = -1;
    zfileClose(gToken.current->file, 0);
    if (gToken.debug != 0) {
        PRINT("\n*** Closed File ***\n\n");
    }
    return 1;
}

BMX_ADDR(0x004a1220)
int ztokenNext(char *out, int size)
{
    int comment = 0;
    int pendingInclude = 0;
    int before, i, quoted;
    ZtokenFrame *f;
    char operand[0x100];
    char symbol[0x80];
    char path[0x100];
    int a = 0, b = 0, result;

restart:
    for (;;) {
        for (;;) {
            i = 0;
            quoted = 0;
            while (isSeparator(gToken.current->line[gToken.current->cursor])) {
                gToken.current->cursor++;
            }
            f = gToken.current;
            if (f->line[f->cursor] != '\0') {
                break;
            }
            /* End of line: read the next one, or pop the include. */
            f->lineNumber++;
            f->line[0] = '\0';
            f->cursor = 0;
            if (zfileGets(f->file, f->line, 0xff) == 0) {
                if (gToken.depth == 0) {
                    return 0;
                }
                f->lineNumber = -1;
                zfileClose(f->file, 0);
                printClosed();
                gToken.depth--;
                gToken.current--;
            }
        }
        if (f->line[f->cursor] == '"') {
            uint8_t c;

            quoted = 1;
            gToken.flags |= 1;
            f->cursor++;
            c = (uint8_t)f->line[f->cursor];
            while (c != '"' && c != 0) {
                out[i++] = (char)c;
                f->cursor++;
                if (size - 1 <= i) {
                    return 0;
                }
                c = (uint8_t)f->line[f->cursor];
            }
            if (f->line[f->cursor] != '"') {
                return 0;
            }
            f->cursor++;
        } else {
            gToken.flags &= ~1u;
            /* The original does not stop at a terminating NUL here, only at a separator. */
            while (!isSeparator(f->line[f->cursor])) {
                out[i++] = f->line[f->cursor];
                f->cursor++;
                if (i == size - 1) {
                    return 0;
                }
            }
        }
        out[i] = '\0';
        if (!quoted && (out[0] == ';' || zstrncmp(out, "//", 2) == 0)) {
            /* Line comment: drop the rest of the line. */
            f = gToken.current;
            f->lineNumber++;
            f->line[0] = '\0';
            if (zfileGets(f->file, f->line, 0xff) == 0) {
                if (gToken.depth == 0) {
                    return 0;
                }
                f->lineNumber = -1;
                zfileClose(f->file, 0);
                printClosed();
                gToken.depth--;
                gToken.current--;
                goto restart;
            }
            f->cursor = 0;
            goto restart;
        }
        before = comment;
        if (gToken.hashComments != 0 && !quoted && out[0] == '#') {
            f = gToken.current;
            f->lineNumber++;
            f->line[0] = '\0';
            if (zfileGets(f->file, f->line, 0xff) == 0) {
                return 0;
            }
            f->cursor = 0;
            goto restart;
        }
        if (!quoted) {
            if (zstrcmp(out, "/*") == 0) {
                comment = before + 1;
            } else if (zstrcmp(out, "*/") == 0) {
                comment--;
            }
        }
        if (comment > 0) {
            continue;
        }
        break;
    }
    if (comment == 0) {
        if (before == 1) {
            goto restart;
        }
    } else if (comment < 0) {
        return 0;
    }
    if (gToken.upperCase != 0) {
        upperCase(out);
    }
    if (gToken.debug != 0) {
        PRINT("\t");
        PRINT(out);
        PRINT(";");
    }
    f = gToken.current;
    if (zstrcmpi(out, "#INCLUDE") == 0 && gToken.macros != 0) {
        pendingInclude = 1;
        goto restart;
    }
    if (pendingInclude == 1) {
        if (gToken.depth == kZtokenMaxDepth) {
            PRINT("**ERROR** Maximum include nesting reached!\n");
        } else {
            gToken.current++;
            gToken.depth++;
            zmemCopy(gToken.current, f, sizeof(ZtokenFrame));
            zstrFormat(operand, "%s%s", gToken.current->includeDir, out);
            gToken.current->file = zfileOpen(operand, 1, 0);
            if (startFrame(gToken.current, operand)) {
                pendingInclude = 0;
                goto restart;
            }
            PRINT("Can't open include file: %s\n", operand);
            gToken.depth--;
            gToken.current--;
        }
        out[0] = '\0';
        ztokenPrintTrace();
        return 0;
    }
    if (zstrcmpi(out, "#INCLUDE_PATH") == 0) {
        if (gToken.macros != 0 && ztokenNext(gToken.current->includeDir, 0x100) == 0) {
            gToken.current->includeDir[0] = '\0';
        }
        goto restart;
    }
    if (zstrcmpi(out, "#ifndef") == 0) {
        if (ztokenNext(out + 1, 0x80) == 0) {
            return 0;
        }
        goto restart;
    }
    if (zstrcmpi(out, "#endif") == 0) {
        goto restart;
    }
    f = gToken.current;
    if (zstrcmpi(out, "#DEFINE") == 0) {
        if (ztokenNext(out + 1, 0x80) == 0) {
            return 0;
        }
        out[0] = '$';
        if (ztokenNext(symbol, 0x80) == 0) {
            return 0;
        }
        if (gToken.macros != 0) {
            ztokenDefine(out, symbol);
        }
        goto restart;
    }
    if (out[0] != '$') {
        goto done;
    }
    switch (out[1]) {
    case '$':
        if (out[2] == '~') {
            gToken.symbolBytes = 0;
        } else if (out[2] == '$') {
            if (gToken.depth != kZtokenMaxDepth) {
                gToken.current++;
                gToken.depth++;
                zmemCopy(gToken.current, f, sizeof(ZtokenFrame));
                zstrFormat(path, "%s%s", gToken.current->includeDir, out + 3);
                gToken.current->file = zfileOpen(path, 1, 0);
                if (startFrame(gToken.current, path)) {
                    out[0] = '\0';
                    return 0;
                }
                PRINT("Can't open include file: %s\n", path);
                gToken.depth--;
                gToken.current--;
            } else {
                PRINT("**ERROR** Maximum include nesting reached!\n");
            }
        } else {
            ztokenNext(symbol, 0x80);
            ztokenDefine(out + 1, symbol);
        }
        goto restart;
    default:
        if (ztokenLookup(out) == 0) {
            PRINT("Symbol not found: '%s'\n", out);
            ztokenNext(out, size);
        }
        goto done;
    case '*':
    case '+':
    case '-':
    case '/':
        if (ztokenNext(operand, 0x80) != 0) {
            scanOperand(operand, &a);
        }
        if (ztokenNext(operand, 0x80) != 0) {
            scanOperand(operand, &b);
        }
        switch (out[1]) {
        case '*':
            result = b * a;
            break;
        case '+':
            result = a + b;
            break;
        case '-':
            result = a - b;
            break;
        default:
            result = a / b;
            break;
        }
        zstrFormat(out, "%d", result);
        break;
    }
done:
    if (gToken.debug != 0) {
        PRINT("\n");
    }
    return 1;
}

BMX_ADDR(0x004a20c0)
int ztokenDefine(const char *name, const char *value)
{
    char local[64];
    uint32_t len = gToken.symbolBytes;
    int pos;
    uint32_t w;
    const char *p;
    int i = 0;

    while (name[i] != '\0') {
        local[i] = name[i];
        i++;
    }
    local[i] = '\0';
    pos = ztokenLookup(local);
    if (pos == 0) {
        /* New entry: "$name@value". Writes stop at the table size; '$' and '@' cannot be stored. */
        if (len < kZtokenSymbolBytes) {
            w = len;
            gToken.symbols[w++] = '$';
            for (p = name + 1; *p != '\0'; p++) {
                if (w > kZtokenSymbolBytes - 1) {
                    goto full;
                }
                gToken.symbols[w++] = *p;
                if (*p == '$' || *p == '@') {
                    w = kZtokenSymbolBytes + 1;
                }
            }
            if (w < kZtokenSymbolBytes) {
                gToken.symbols[w++] = '@';
                for (p = value; *p != '\0'; p++) {
                    if (w > kZtokenSymbolBytes - 1) {
                        goto full;
                    }
                    gToken.symbols[w++] = *p;
                    if (*p == '$' || *p == '@') {
                        w = kZtokenSymbolBytes + 1;
                    }
                }
                if (w < kZtokenSymbolBytes) {
                    gToken.symbolBytes = w;
                    return 1;
                }
            }
        }
    full:
        PRINT("Out of symbol space\n");
        return 0;
    }
    /* Redefinition: replace the value in place, moving the entries behind it. */
    {
        uintptr_t oldLen = (uintptr_t)name; /* only overwritten when the old value has characters */
        uint32_t valueStart, k, newLen = 0;
        uint32_t tail;
        int shrink;

        while (gToken.symbols[pos] != '@') {
            pos++;
        }
        valueStart = (uint32_t)pos + 1;
        for (k = valueStart; k < gToken.symbolBytes && gToken.symbols[k] != '$'; k++) {
            oldLen = k + (1 - valueStart);
        }
        if (*value == '\0') {
            shrink = oldLen != 0;
        } else {
            do {
                newLen++;
            } while (value[newLen] != '\0');
            shrink = newLen < oldLen;
            if (newLen > oldLen) {
                uint32_t s;

                for (s = gToken.symbolBytes; valueStart < s; s--) {
                    gToken.symbols[newLen + s - oldLen] = gToken.symbols[s];
                }
                gToken.symbolBytes = (uint32_t)(newLen + (gToken.symbolBytes - oldLen));
                goto copy;
            }
        }
        if (shrink) {
            for (tail = valueStart; tail < gToken.symbolBytes; tail++) {
                gToken.symbols[tail] = gToken.symbols[tail + (oldLen - newLen)];
            }
            gToken.symbolBytes = (uint32_t)(newLen + (gToken.symbolBytes - oldLen));
        }
    copy:
        for (p = value, k = valueStart; *p != '\0'; p++, k++) {
            gToken.symbols[k] = *p;
        }
    }
    PRINT("Replacing symbol:%s\n", name);
    return 1;
}

BMX_ADDR(0x004a22a0)
int ztokenLookup(char *name)
{
    uint32_t pos;

    for (pos = 0; pos < gToken.symbolBytes; pos++) {
        if (gToken.symbols[pos] == '$') {
            uint32_t k = 0;

            while (pos + k < gToken.symbolBytes && gToken.symbols[pos + k] == name[k]) {
                k++;
            }
            if (name[k] == '\0' && gToken.symbols[pos + k] == '@') {
                uint32_t src = pos + 1 + k;
                int dst = 0;

                while (src < gToken.symbolBytes && gToken.symbols[src] != '$') {
                    name[dst++] = gToken.symbols[src++];
                }
                name[dst] = '\0';
                return (int)pos + 1;
            }
        }
    }
    return 0;
}

BMX_ADDR(0x004a2330)
int ztokenReadInt(int *value)
{
    char tok[0x80];
    const char *p = tok;
    const char *fmt = "%d";

    if (ztokenNext(tok, 0x80) == 0) {
        return 0;
    }
    if (zstrncmp(tok, "0X", 2) == 0 || zstrncmp(tok, "0x", 2) == 0) {
        p = tok + 2;
        fmt = "%X";
    } else if (zstrncmp(tok, "$", 1) == 0) {
        p = tok + 1;
        fmt = "%X";
    }
    return sscanf(p, fmt, value) == 1;
}

BMX_ADDR(0x004a2440)
int ztokenReadString(char *out, int size)
{
    return ztokenNext(out, size) != 0;
}

BMX_ADDR(0x004a2460)
int ztokenExpect(char *expected)
{
    char tok[0x80];

    upperCase(expected);
    if (ztokenNext(tok, 0x80) != 0) {
        if (zstrcmp(tok, expected) == 0) {
            return 1;
        }
        PRINT("ZTOKEN assert failed : found '%s', expecting '%s'\n", tok, expected);
    }
    return 0;
}

BMX_ADDR(0x004a24f0)
void ztokenPrintTrace(void)
{
    int i;

    for (i = gToken.depth; i >= 0; i--) {
        if (i != gToken.depth) {
            PRINT("Included from:");
        }
        PRINT("File:%s,  Line:%d\n", gToken.frames[i].name, gToken.frames[i].lineNumber);
    }
}
