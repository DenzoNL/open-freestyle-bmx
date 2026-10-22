/* Controller devices of the PC platform layer (C:\NgEngine\Zge\PC\pcinput.c). */
#include "zge/pc/pcinput.h"

#include "zge/zmem.h"
#include "zge/zstring.h"

BMX_ADDR(0x00480370)
struct GuiPadDevice *zpadCreateDevice(int kind)
{
    uint32_t *device = zmemFreeListAlloc(gZpadDevices);

    zmemFill(device, 0, kZpadDeviceBytes);
    device[kZpadDirectInputSlot / 4] = (uint32_t)zpadSetupDirectInput(kind, device);
    if (device[kZpadDirectInputSlot / 4] == 0) {
        zmemFreeListFree(gZpadDevices, device);
        return NULL;
    }
    device[0] = (uint32_t)kind;
    return (struct GuiPadDevice *)device;
}

BMX_ADDR(0x004803c0)
int zpadDestroyDevice(struct GuiPadDevice *device)
{
    zpadReleaseDirectInput(device);
    zmemFreeListFree(gZpadDevices, device);
    return 1;
}

BMX_ADDR(0x004803f0)
int zpadPoll(void)
{
    return zpadPollDevices();
}

BMX_ADDR(0x00480400)
struct GuiPadDevice *zpadGetDevice(int index)
{
    ZMemFreeNode *node = zmemFreeListFirst(gZpadDevices);

    for (; node != NULL; node = zmemFreeNodeNext(node), index--) {
        if (index == 0) {
            return zmemFreeNodePayload(node);
        }
    }
    return NULL;
}

enum { kDiErrorInputLost = 0x8007001e };

BMX_ADDR(0x004937f0)
int zpadReleaseDirectInput(void *device)
{
    ZpadDiRecord *record = ((ZpadDevice *)device)->record;
    ZpadDiDevice *di = record->di;

    if (di != NULL) {
        di->vtbl->unacquire(di);
        record->di->vtbl->release(record->di);
        record->di = NULL;
    }
    zmemFreeListFree(gZpadDiRecords, record);
    return 1;
}

BMX_ADDR(0x00493830)
int zrwOnActivate(void)
{
    if (gZpadDiActive == 0) {
        return 1;
    }
    if (gZpadKeyboard != NULL) {
        gZpadKeyboard->vtbl->acquire(gZpadKeyboard);
    }
    for (ZMemFreeNode *node = zmemFreeListFirst(gZpadDiRecords); node != NULL;
         node = zmemFreeNodeNext(node)) {
        ZpadDiDevice *di = ((ZpadDiRecord *)zmemFreeNodePayload(node))->di;

        if (di != NULL) {
            di->vtbl->acquire(di);
        }
    }
    return 1;
}

BMX_ADDR(0x00493890)
int zrwOnDeactivate(void)
{
    if (gZpadDiActive == 0) {
        return 1;
    }
    if (gZpadKeyboard != NULL) {
        gZpadKeyboard->vtbl->unacquire(gZpadKeyboard);
        gZpadKeyboardFlags &= ~1u;
    }
    for (ZMemFreeNode *node = zmemFreeListFirst(gZpadDiRecords); node != NULL;
         node = zmemFreeNodeNext(node)) {
        ZpadDiRecord *record = zmemFreeNodePayload(node);

        if (record->di != NULL) {
            record->di->vtbl->unacquire(record->di);
            record->flags &= ~1u;
        }
    }
    return 1;
}

/* Bit of ZpadDevice.buttons that is set while the analogue input is above zero. */
static const struct {
    uint8_t input;
    uint32_t bit;
} kPadInputBits[] = {
    {6, 0x1},    {7, 0x2},    {4, 0x4},      {5, 0x8},      {8, 0x10},   {9, 0x20},
    {10, 0x40},  {11, 0x80},  {12, 0x10000}, {13, 0x20000}, {14, 0x100}, {16, 0x400},
    {15, 0x200}, {17, 0x800}, {20, 0x4000},  {21, 0x8000},
};

/* Stick deflection (-127..127 scaled by 1/127) into the two inputs of an axis direction. */
static float axisInput(int32_t value, int positive)
{
    const float scale = 0x1.020408p-7f;

    if (positive) {
        return (value > 0) ? (float)((double)value * scale) : 0.0f;
    }
    return (value < 0) ? (float)((double)(int32_t)(0u - (uint32_t)value) * scale) : 0.0f;
}

/* Reads a joystick like device (DIJOYSTATE) into the inputs of the device block. */
static void pollJoystick(ZpadDiRecord *record, ZpadDevice *device)
{
    ZpadDiDevice *di = record->di;
    int32_t state[20] = {0};
    const uint8_t *buttons = (const uint8_t *)state + 0x30;
    int32_t result;

    if ((record->flags & 1) == 0) {
        if (di->vtbl->acquire(di) < 0) {
            return;
        }
        record->flags |= 1;
    }
    if (record->di->vtbl->poll(record->di) < 0) {
        return;
    }
    result = record->di->vtbl->getDeviceState(record->di, 0x50, state);
    if (result == (int32_t)kDiErrorInputLost && record->di->vtbl->acquire(record->di) >= 0) {
        return;
    }
    device->input[6] = axisInput(state[1], 0);
    device->input[7] = axisInput(state[1], 1);
    device->input[4] = axisInput(state[0], 0);
    device->input[5] = axisInput(state[0], 1);
    for (const int8_t *map = (const int8_t *)record->buttonMap + 8; *map != -1; map += 2) {
        ((float *)device->input)[(uint8_t)map[1]] = (buttons[map[0]] & 0x80) ? 1.0f : 0.0f;
    }
}

/* Keyboard bindings: the keyboard is read once per poll, however many devices are bound to it. */
static void pollKeys(ZpadDiRecord *record, ZpadDevice *device)
{
    if ((gZpadKeyboardOk & 1) == 0) {
        int32_t result;

        if ((gZpadKeyboardFlags & 1) == 0) {
            if (gZpadKeyboard->vtbl->acquire(gZpadKeyboard) < 0) {
                return;
            }
            gZpadKeyboardFlags |= 1;
        }
        result = gZpadKeyboard->vtbl->getDeviceState(gZpadKeyboard, 0x100, gZpadKeyState);
        if (result == (int32_t)kDiErrorInputLost && gZpadKeyboard->vtbl->acquire(gZpadKeyboard) >= 0) {
            return;
        }
        gZpadKeyboardOk |= 1;
    }
    for (const uint8_t *map = record->keyMap; *map != 0xff; map += 2) {
        if (gZpadKeyState[*map] & 0x80) {
            device->input[map[1]] = 1.0f;
        }
    }
}

BMX_ADDR(0x00493910)
int zpadPollDevices(void)
{
    gZpadKeyboardOk &= 0xfffe;
    for (ZMemFreeNode *node = zmemFreeListFirst(gZpadDiRecords); node != NULL;
         node = zmemFreeNodeNext(node)) {
        ZpadDiRecord *record = zmemFreeNodePayload(node);
        ZpadDevice *device = record->device;
        const uint32_t before = device->buttons;
        uint32_t now = 0;

        zmemFill(&device->buttons, 0, 4);
        zmemFill(device->input, 0, 0x58);
        zmemFill(&device->pad6c, 0, 4);
        if (record->di != NULL) {
            pollJoystick(record, device);
        }
        if (record->keyMap != NULL) {
            pollKeys(record, device);
        }
        for (size_t i = 0; i < sizeof kPadInputBits / sizeof kPadInputBits[0]; i++) {
            if (device->input[kPadInputBits[i].input] > 0.0f) {
                now |= kPadInputBits[i].bit;
            }
        }
        device->buttons = now;
        device->changed = now ^ before;
        device->released = (now ^ before) & before;
    }
    return 1;
}

BMX_ADDR(0x00493cc0)
int zpadSetKeyMap(void *device, const uint8_t *map)
{
    ZpadDevice *d = device;

    if (d->record == NULL || d == NULL) {
        return 0;
    }
    d->record->keyMap = map;
    return 1;
}
