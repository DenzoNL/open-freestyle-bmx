/* GUI label element and its text markup (type 1/5 handler and the "!S/!T/!B" markup parser). */
#include "bmx/com/menu/gui/guilabel.h"

#include "bmx/com/menu/gui/guiinput.h"
#include "bmx/com/menu/gui/guimain.h"
#include "bmx/com/zge/ngif/ngifzmemdmm.h"
#include "bmx/com/zge/zprim.h"
#include "compat/crt.h"

BMX_ADDR(0x00436aa0)
void guiSetMarkupTarget(uint32_t id)
{
    gGuiMarkupTarget = id;
}

BMX_ADDR(0x004367c0)
const char *guiParseInt(const char *s, int *out)
{
    uint32_t value = 0;
    int sign = 1;
    while (*s == ' ' || *s == ',') {
        s++;
    }
    for (;;) {
        const char c = *s;
        if (c != '-' && (c < '0' || c > '9')) {
            break;
        }
        s++;
        if (c == '-') {
            sign = -sign;
        } else {
            value = (uint32_t)(c - '0') + value * 10;
        }
    }
    if (out != NULL) {
        *out = (int)((uint32_t)sign * value);
    }
    while (*s == ' ' || *s == ',') {
        s++;
    }
    return s;
}

/* Copies a '$'-terminated markup text into buf (stops at '$', '!' or NUL); `~` becomes an apostrophe if
 * asked. */
static const char *guiMarkupText(const char *s, char *buf, int tildeIsQuote)
{
    char *dst = buf;
    while (*s != '$' && *s != '!' && *s != '\0') {
        *dst++ = (tildeIsQuote && *s == '~') ? '\'' : *s;
        s++;
    }
    *dst = '\0';
    return s;
}

/*
 * Original code is one function split into jump-table cases: 0x4368e0 (skip "!!"), 0x4368f0 (!B),
 * 0x436951 (!S) and 0x436883 (!T); the 0x4368e6 return is shared.
 */
BMX_ADDR(0x00436840)
void guiParseMarkup(const char *s, int x, int y)
{
    char buf[200];
    if (*s != '!') {
        return;
    }
    for (;;) {
        const int cmd = (int)(signed char)s[1];
        s++;
        if ((uint32_t)(cmd - 0x21) > 0x53) {
            return;
        }
        switch (cmd) {
        case '!':
            s++;
            break;
        case 'B':
        case 'b':
            if (s[1] != '$') {
                return;
            }
            s += 2;
            s = guiMarkupText(s, buf, 0);
            if (*s != '$') {
                return;
            }
            s++;
            if (gGuiMarkupTarget != 0) {
                GuiElement *target = guiFindById(gGuiMarkupTarget);
                if (target != NULL) {
                    guiSetText(target, buf);
                }
                gGuiMarkupTarget = 0;
            }
            break;
        case 'S':
        case 's': {
            int a;
            int b;
            int c;
            s++;
            s = guiParseInt(s, &a);
            s = guiParseInt(s, &b);
            s = guiParseInt(s, &c);
            zprimDrawSprite(c, a + x, b + y);
            break;
        }
        case 'T':
        case 't': {
            int a;
            int b;
            s++;
            s = guiParseInt(s, &a);
            s = guiParseInt(s, &b);
            if (*s != '$') {
                return;
            }
            s++;
            s = guiMarkupText(s, buf, 1);
            if (*s != '$') {
                return;
            }
            s++;
            break;
        }
        default:
            return;
        }
        if (*s != '!') {
            return;
        }
    }
}

/* Advances the sprite animation of a label by one frame (kGuiAnimPlaying). */
static void guiLabelAnimate(GuiElement *el)
{
    const uint16_t anim = el->animFlags;
    const int16_t idx = el->animIndex;
    if (el->animTicks < el->animHold[idx]) {
        el->animTicks++;
    } else if ((anim & kGuiAnimReverse) != 0) {
        const int16_t n = (int16_t)(idx - 1);
        el->animIndex = n;
        if (n < 0) {
            if ((anim & kGuiAnimLoop) != 0) {
                if ((anim & kGuiAnimPingPong) != 0) {
                    el->animIndex = 1;
                    el->animFlags = (uint16_t)(anim & ~kGuiAnimReverse);
                } else {
                    el->animIndex = (int16_t)(el->animCount - 1);
                }
            } else {
                el->animIndex = (int16_t)(el->animCount - 1);
                el->animFlags = (uint16_t)(anim & ~kGuiAnimPlaying);
            }
        }
        el->animTicks = 0;
    } else {
        const int16_t n = (int16_t)(idx + 1);
        el->animIndex = n;
        if (n >= (int)el->animCount) {
            if ((anim & kGuiAnimLoop) != 0) {
                if ((anim & kGuiAnimPingPong) != 0) {
                    el->animIndex = (int16_t)(n - 2);
                    el->animFlags = (uint16_t)(anim | kGuiAnimReverse);
                } else {
                    el->animIndex = 0;
                }
            } else {
                el->animIndex = (int16_t)(el->animCount - 1);
                el->animFlags = (uint16_t)(anim & ~kGuiAnimPlaying);
            }
        }
        el->animTicks = 0;
    }
    el->frame = el->animFrames[el->animIndex];
}

/*
 * Draws a label: first the sprite (animated, optionally scaled, tinted or highlighted), then its text,
 * line by line. Quirks kept from the original: the horizontal alignment of every line uses the width
 * of the whole text (from its first character), and the "!" markup lines are executed instead of drawn.
 * The one-instruction wrapper at 0x439f80 only dispatches (tick: 1, draw: body at 0x439fa1).
 */
BMX_ADDR(0x00439fa1)
BMX_ADDR(0x00439f80)
int BMX_CDECL guiLabelProc(int msg, GuiElement *el, int arg3, int arg4)
{
    if (msg == kGuiMsgTick) {
        return 1;
    }
    if (msg != kGuiMsgDraw) {
        return 0;
    }
    int scaled = 0;
    int x = (int16_t)(el->slideX + el->x);
    int y = (int16_t)(el->slideY + el->y);
    int16_t w = el->w;
    int16_t h = el->h;
    if (arg3 != 0 && el->spriteFocused != NULL) {
        w = zprimSpriteFrameWidth(el->spriteFocused, el->frame);
        h = zprimSpriteFrameHeight(el->spriteFocused, el->frame);
    } else if (el->sprite != NULL) {
        w = zprimSpriteFrameWidth(el->sprite, el->frame);
        h = zprimSpriteFrameHeight(el->sprite, el->frame);
    }
    const uint16_t flags = el->flags;
    if ((flags & kGuiFlagScaled) != 0) {
        if ((flags & kGuiFlagWide) != 0) {
            w = (int16_t)(w * 160 / 100);
        } else {
            w = (int16_t)(int)((double)w * el->scaleX);
            h = (int16_t)(int)((double)h * el->scaleY);
        }
        zprimSetScale(el->scaleX, el->scaleY);
        scaled = 1;
    } else if ((flags & kGuiFlagWide) != 0) {
        w = (int16_t)(w * 160 / 100);
        zprimSetScale(1.0f, 1.0f);
        scaled = 1;
    }
    if ((el->flags & kGuiFlagCenterX) != 0) {
        x -= w / 2;
    }
    if ((el->flags & kGuiFlagCenterY) != 0) {
        y -= h / 2;
    }
    const int xEl = x;

    if ((el->animFlags & kGuiAnimDisabled) == 0) {
        if ((el->animFlags & kGuiAnimPlaying) != 0) {
            guiLabelAnimate(el);
        }
        zprimSet(8, (el->animFlags & kGuiAnimBit4) != 0);
        if ((el->animFlags & kGuiAnimBit8) != 0) {
            zprimSet(7, 1);
            zprimSet(0xf, el->alpha);
        } else {
            zprimSet(7, 0);
        }
        zprimSet(0xe, (int)el->palette);
        zprimSetPicture((arg3 != 0 && el->spriteFocused != NULL) ? el->spriteFocused : el->sprite);
        if (guiPadMultiMode() == 0 && (el->flags & kGuiFlagHighlight) != 0) {
            zprimSetColor(0xde, 0xde, 0x9c, 0x11);
            zprimSet(0x10, gGuiPulse);
        }
        if ((el->flags & kGuiFlagTinted) != 0) {
            zprimSet(0x11, (int)el->tintRgb);
            zprimSet(0x10, (int)el->tintAlpha);
        }
        zprimDrawSprite(el->frame, (int16_t)(el->spriteX + x), (int16_t)(el->spriteY + y));
        if (scaled != 0) {
            zprimStateReset();
        }
        if ((el->flags & (kGuiFlagTinted | kGuiFlagHighlight)) != 0) {
            zprimSet(0x10, 0);
        }
    }

    if ((el->textFlags & kGuiTextDisabled) == 0 && el->textHandle != 0) {
        char *const text = zmemdmmLock(el->textHandle);
        char *line = text;
        int lineY = y;
        while (strlen(line) != 0) {
            char *newline = strchr(line, '\n');
            if (newline != NULL) {
                *newline = '\0';
            }
            zprimSet(0x13, (el->textFlags & kGuiTextBit4) != 0);
            if ((el->textFlags & kGuiTextColored) != 0) {
                zprimSet(0x14, 1);
                zprimSetColor(el->textRgb[0], el->textRgb[1], el->textRgb[2], 0x16);
            } else {
                zprimSet(0x14, 0);
            }
            const int savedAlpha = zprimGet(0xf);
            const int savedTint = zprimGet(0x11);
            const int savedTintAlpha = zprimGet(0x10);
            if ((el->textFlags & kGuiTextBit8) != 0) {
                zprimSet(0x12, 1);
                zprimSet(0xf, el->alpha);
                zprimSet(0x11, (int)el->tintRgb);
                zprimSet(0x10, (int)el->tintAlpha);
            } else {
                zprimSet(0x12, 0);
            }
            zprimSet(0x17, (int)el->textStyle);
            zprimSet(0x18, el->textSpacing);
            int tx = el->textX;
            int ty = el->textY;
            zprimSetFont(el->font);
            if (line[0] == '!') {
                zprimSetPicture(el->sprite);
                zprimSet(8, 1);
                guiParseMarkup(line, tx + xEl, ty + lineY);
            } else {
                if ((el->textFlags & kGuiTextCenterX) != 0) {
                    if ((el->textFlags & kGuiTextFixedPitch) != 0) {
                        const uint32_t len = (uint32_t)strlen(text);
                        tx += (int)((uint32_t)(w - (int)len * el->textSpacing) >> 1);
                    } else {
                        tx += (w - (int)zprimTextWidth(el->font, (const uint8_t *)text)) / 2;
                    }
                }
                if ((el->textFlags & kGuiTextCenterY) != 0) {
                    ty += (h - (int)zprimFontFlags(el->font)) / 2;
                }
                if ((el->textFlags & kGuiTextRight) != 0) {
                    if ((el->textFlags & kGuiTextFixedPitch) != 0) {
                        const uint16_t span = (uint16_t)((uint32_t)strlen(text) * (uint16_t)el->textSpacing);
                        tx += w - span;
                    } else {
                        tx += w - (int)zprimTextWidth(el->font, (const uint8_t *)text);
                    }
                }
                if ((el->textFlags & kGuiTextFixedPitch) != 0) {
                    zprimDrawTextMono((const uint8_t *)line, tx + xEl, ty + lineY);
                } else {
                    zprimDrawText((const uint8_t *)line, tx + xEl, ty + lineY);
                }
            }
            zprimSet(0x18, 0);
            zprimSet(0x17, 0);
            zprimSet(0xf, savedAlpha);
            zprimSet(0x11, savedTint);
            zprimSet(0x10, savedTintAlpha);
            lineY = lineY + (int)zprimFontFlags(el->font) + 1;
            if (newline != NULL) {
                *newline = '\n';
                line = newline + 1;
            } else {
                line = strchr(line, '\0');
            }
        }
        zmemdmmUnlock(el->textHandle);
    }
    return 1;
}
