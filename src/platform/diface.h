/*
 * The DirectInput 7 surface the game uses (pcinput.c), in our own types (src/platform has no windows.h): the
 * IDirectInput7A and IDirectInputDevice7A vtable layouts, DIJOYSTATE, DIDEVICEINSTANCEA, the property headers
 * and the HRESULTs. The stand-ins (input.c) implement these; BMX_INPUT_CHECK (inputcheck.c) calls the real
 * dinput.dll through the same declarations.
 */
#ifndef BMX_PLATFORM_DIFACE_H
#define BMX_PLATFORM_DIFACE_H

#include <stdint.h>

#include "platform/platform.h"

typedef int32_t BmxHresult;

enum {
    kDiOk = 0,
    kDiNoEffect = 1, /* S_FALSE: already acquired / not acquired / nothing to poll */
};

#define kDiErrOtherAppHasPrio ((BmxHresult)0x80070005) /* E_ACCESSDENIED: not foreground / taken */
#define kDiErrNotAcquired     ((BmxHresult)0x8007000c)
#define kDiErrInputLost       ((BmxHresult)0x8007001e)
#define kDiErrInvalidParam    ((BmxHresult)0x80070057)
#define kDiErrDeviceNotReg    ((BmxHresult)0x80040154)
#define kDiErrUnplugged       ((BmxHresult)0x80040209)
#define kDiErrNotFound        ((BmxHresult)0x80070002)
#define kDiErrAcquired        ((BmxHresult)0x8007aa00)
#define kDiErrUnsupported     ((BmxHresult)0x80004001)
#define kDiErrNoInterface     ((BmxHresult)0x80004002)

typedef struct BmxGuid {
    uint32_t data1;
    uint16_t data2, data3;
    uint8_t data4[8];
} BmxGuid;

/* DIPROPHEADER + payload (DIPROPDWORD, DIPROPRANGE). Properties are passed as small integers (REFGUID). */
enum { kDiPropRange = 4, kDiPropDeadZone = 5, kDiPropSaturation = 6 };
enum { kDiphDevice = 0, kDiphByOffset = 1 };

typedef struct BmxDiPropHeader {
    uint32_t size, headerSize, obj, how;
} BmxDiPropHeader;

typedef struct BmxDiPropDword {
    BmxDiPropHeader header;
    uint32_t data;
} BmxDiPropDword;

typedef struct BmxDiPropRange {
    BmxDiPropHeader header;
    int32_t min, max;
} BmxDiPropRange;

/* DIDATAFORMAT: only dwDataSize is read. */
typedef struct BmxDiDataFormat {
    uint32_t size, objSize, flags, dataSize, numObjs;
    const void *objects;
} BmxDiDataFormat;

/* DIJOYSTATE (c_dfDIJoystick). */
enum { kDiAxisX, kDiAxisY, kDiAxisZ, kDiAxisRx, kDiAxisRy, kDiAxisRz, kDiSlider0, kDiSlider1, kDiAxes };

typedef struct BmxDiJoyState {
    int32_t axis[kDiAxes]; /* lX lY lZ lRx lRy lRz rglSlider[2]: offsets 0x00..0x1c */
    uint32_t pov[4];       /* 0x20, hundredths of a degree clockwise from north, 0xffffffff centred */
    uint8_t buttons[32];   /* 0x30, 0x80 = pressed */
} BmxDiJoyState;

_Static_assert(sizeof(BmxDiJoyState) == 0x50, "DIJOYSTATE");

/* DIDEVICEINSTANCEA. */
typedef struct BmxDiDeviceInstance {
    uint32_t size;
    BmxGuid guidInstance;
    BmxGuid guidProduct;
    uint32_t devType;
    char instanceName[260];
    char productName[260];
    BmxGuid guidFFDriver;
    uint16_t usagePage, usage;
} BmxDiDeviceInstance;

_Static_assert(sizeof(BmxDiDeviceInstance) == 0x244, "DIDEVICEINSTANCEA");

typedef struct BmxDiDevice BmxDiDevice;
typedef struct BmxDiDeviceVtbl {
    BmxHresult(BMX_WINAPI *queryInterface)(BmxDiDevice *self, const BmxGuid *iid, void **out);
    uint32_t(BMX_WINAPI *addRef)(BmxDiDevice *self);
    uint32_t(BMX_WINAPI *release)(BmxDiDevice *self);
    void *getCapabilities, *enumObjects, *getProperty;
    BmxHresult(BMX_WINAPI *setProperty)(BmxDiDevice *self, uintptr_t property, const BmxDiPropHeader *header);
    BmxHresult(BMX_WINAPI *acquire)(BmxDiDevice *self);
    BmxHresult(BMX_WINAPI *unacquire)(BmxDiDevice *self);
    BmxHresult(BMX_WINAPI *getDeviceState)(BmxDiDevice *self, uint32_t size, void *state);
    void *getDeviceData;
    BmxHresult(BMX_WINAPI *setDataFormat)(BmxDiDevice *self, const BmxDiDataFormat *format);
    void *setEventNotification;
    BmxHresult(BMX_WINAPI *setCooperativeLevel)(BmxDiDevice *self, void *window, uint32_t flags);
    void *getObjectInfo;
    BmxHresult(BMX_WINAPI *getDeviceInfo)(BmxDiDevice *self, BmxDiDeviceInstance *info);
    void *runControlPanel, *initialize, *createEffect, *enumEffects, *getEffectInfo, *getForceFeedbackState,
        *sendForceFeedbackCommand, *enumCreatedEffectObjects, *escape;
    BmxHresult(BMX_WINAPI *poll)(BmxDiDevice *self);
    void *sendDeviceData, *enumEffectsInFile, *writeEffectToFile;
} BmxDiDeviceVtbl;

_Static_assert(__builtin_offsetof(BmxDiDeviceVtbl, getDeviceInfo) == 0x3c * (sizeof(void *) / 4), "vtbl");
_Static_assert(__builtin_offsetof(BmxDiDeviceVtbl, poll) == 0x64 * (sizeof(void *) / 4), "vtbl");

struct BmxDiDevice {
    const BmxDiDeviceVtbl *vtbl;
};

typedef uint32_t(BMX_WINAPI *BmxDiEnumCallback)(const BmxDiDeviceInstance *instance, void *ref);

typedef struct BmxDi BmxDi;
typedef struct BmxDiVtbl {
    BmxHresult(BMX_WINAPI *queryInterface)(BmxDi *self, const BmxGuid *iid, void **out);
    uint32_t(BMX_WINAPI *addRef)(BmxDi *self);
    uint32_t(BMX_WINAPI *release)(BmxDi *self);
    void *createDevice;
    BmxHresult(BMX_WINAPI *enumDevices)(BmxDi *self, uint32_t type, BmxDiEnumCallback callback, void *ref,
                                        uint32_t flags);
    void *getDeviceStatus, *runControlPanel, *initialize, *findDevice;
    BmxHresult(BMX_WINAPI *createDeviceEx)(BmxDi *self, const BmxGuid *guid, const BmxGuid *iid,
                                           BmxDiDevice **out, void *outer);
} BmxDiVtbl;

_Static_assert(__builtin_offsetof(BmxDiVtbl, createDeviceEx) == 0x24 * (sizeof(void *) / 4), "vtbl");

struct BmxDi {
    const BmxDiVtbl *vtbl;
};

typedef BmxHresult(BMX_WINAPI *BmxDirectInputCreateExFn)(void *instance, uint32_t version, const BmxGuid *iid,
                                                         BmxDi **out, void *outer);

/* GUID_SysKeyboard. */
extern const BmxGuid kBmxGuidSysKeyboard;

#endif
