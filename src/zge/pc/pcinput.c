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
