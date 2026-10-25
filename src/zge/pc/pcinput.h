/*
 * Controller devices of the PC platform layer (C:\NgEngine\Zge\PC\pcinput.c). A device is a 0x74 byte block
 * from a free list: the kind at +0, the button state the polling fills in, and the DirectInput object at
 * +0x70 (see GuiPadDevice for the part the GUI reads).
 */
#ifndef ZGE_PC_PCINPUT_H
#define ZGE_PC_PCINPUT_H

#include <stdint.h>

#include "compat/orig.h"
#include "zge/zmem.h"

struct GuiPadDevice;

enum { kZpadDeviceBytes = 0x74, kZpadDirectInputSlot = 0x70 };

/* Free list of devices (0x65e2ac). */
#define gZpadDevices ORIG_VAR(0x0065e2ac, ZMemFreeList *, gZpadDevices)

/* Creates a device of `kind` (2 keyboard, 0 pad); NULL when DirectInput cannot set it up. */
struct GuiPadDevice *zpadCreateDevice(int kind);
/* Releases the DirectInput object and the block; returns 1. */
int zpadDestroyDevice(struct GuiPadDevice *device);
/* The n-th device of the controller list, or NULL (used by the key binding screen). */
struct GuiPadDevice *zpadGetDevice(int index);
/* Polls all devices (a jump to the platform's update at 0x493910); returns what it returns. */
int zpadPoll(void);

/*
 * DirectInput side of a device (the part the packet filed under zrwcol.c). The device block keeps its
 * DirectInput record at +0x70; the polling code fills the state words and the analogue inputs.
 */
typedef struct ZpadDevice {
    uint32_t kind;               /* 0x00 */
    uint32_t pad04;              /* 0x04 */
    uint32_t buttons;            /* 0x08 bits set by the polling code */
    uint32_t changed;            /* 0x0c bits that differ from the last poll */
    uint32_t released;           /* 0x10 bits that went from set to clear */
    float input[22];             /* 0x14 analogue inputs, indexed by the slot bytes of the maps */
    uint32_t pad6c;              /* 0x6c cleared by the poll */
    struct ZpadDiRecord *record; /* 0x70 */
} ZpadDevice;

_Static_assert(sizeof(ZpadDevice) == kZpadDeviceBytes, "pad device");

/* The part of IDirectInputDevice the code uses (stdcall methods). */
typedef struct ZpadDiDevice ZpadDiDevice;
typedef struct ZpadDiVtbl {
    uint32_t pad00[2];
    uint32_t(BMX_STDCALL *release)(ZpadDiDevice *self);                                            /* 0x08 */
    uint32_t pad0c[3];                                                                             /* 0x0c */
    int32_t(BMX_STDCALL *setProperty)(ZpadDiDevice *self, uintptr_t property, const void *header); /* 0x18 */
    int32_t(BMX_STDCALL *acquire)(ZpadDiDevice *self);                                             /* 0x1c */
    int32_t(BMX_STDCALL *unacquire)(ZpadDiDevice *self);                                           /* 0x20 */
    int32_t(BMX_STDCALL *getDeviceState)(ZpadDiDevice *self, uint32_t size, void *state);          /* 0x24 */
    uint32_t pad28;                                                                                /* 0x28 */
    int32_t(BMX_STDCALL *setDataFormat)(ZpadDiDevice *self, const void *format);                   /* 0x2c */
    uint32_t pad30;                                                                                /* 0x30 */
    int32_t(BMX_STDCALL *setCooperativeLevel)(ZpadDiDevice *self, void *window, uint32_t flags);   /* 0x34 */
    uint32_t pad38;                                                                                /* 0x38 */
    int32_t(BMX_STDCALL *getDeviceInfo)(ZpadDiDevice *self, void *info);                           /* 0x3c */
    uint32_t pad40[9];                                                                             /* 0x40 */
    int32_t(BMX_STDCALL *poll)(ZpadDiDevice *self);                                                /* 0x64 */
} ZpadDiVtbl;

_Static_assert(__builtin_offsetof(ZpadDiVtbl, poll) == 0x64, "DirectInput vtable");

struct ZpadDiDevice {
    const ZpadDiVtbl *vtbl;
};

/* Record of a device in the DirectInput list: its device block and DirectInput object. */
typedef struct ZpadDiRecord {
    ZpadDevice *device;       /* 0x00 */
    ZpadDiDevice *di;         /* 0x04 */
    uint32_t flags;           /* 0x08 bit 0: acquired */
    const uint8_t *keyMap;    /* 0x0c pairs {key, input slot}, ended by 0xff; keyboard binding */
    const uint8_t *buttonMap; /* 0x10 8 byte header, then pairs {button, input slot}, ended by 0xff */
} ZpadDiRecord;

/* 0x674028 non zero once DirectInput is set up; the records' free list; the keyboard object and its state. */
#define gZpadDiActive      ORIG_VAR(0x00674028, int, gZpadDiActive)
#define gZpadDiRecords     ORIG_VAR(0x0067402c, ZMemFreeList *, gZpadDiRecords)
#define gZpadKeyboardOk    ORIG_VAR(0x00674030, uint16_t, gZpadKeyboardOk)
#define gZpadKeyboard      ORIG_VAR(0x0067404c, ZpadDiDevice *, gZpadKeyboard)
#define gZpadKeyboardFlags ORIG_VAR(0x00674050, uint32_t, gZpadKeyboardFlags)
typedef uint8_t ZpadKeyState[256];
#define gZpadKeyState ORIG_VAR(0x00674054, ZpadKeyState, gZpadKeyState)

/* Un-acquires and releases the DirectInput object of a device and frees its record; returns 1. */
int zpadReleaseDirectInput(void *device);
/* Reads every device: buttons, sticks and keys into the device blocks; returns 1. */
int zpadPollDevices(void);
/* Window activation: re-acquires / un-acquires all DirectInput objects; return 1. */
int zrwOnActivate(void);
int zrwOnDeactivate(void);
/* Binds the key map of a device; 0 when the device has no DirectInput record. */
int zpadSetKeyMap(void *device, const uint8_t *map);

/* GUID by value (16 bytes). */
typedef struct ZpadGuid {
    uint32_t data1;
    uint16_t data2, data3;
    uint8_t data4[8];
} ZpadGuid;

/*
 * A controller profile of the game pads the game knows (table at 0x561f2c, ended by a NULL entry): the
 * product names and product GUIDs (both NULL ended lists) that select it, then the button map. The record
 * keeps a pointer to the whole profile as its button map.
 */
typedef struct ZpadProfile {
    const char *const *names;
    const ZpadGuid *const *guids;
    uint8_t map[1]; /* pairs {button, input slot}, ended by 0xff */
} ZpadProfile;
#define gZpadProfiles ORIG_VAR(0x00561f2c, const ZpadProfile *, gZpadProfiles)

/*
 * Creates the DirectInput record of a device: kind 1 (keyboard) and 2 (both) bind the keyboard (created on
 * first use), kind 0 (pad) and 2 the pad, whose product name or GUID selects its button map from the
 * profiles. Without a pad the keyboard stands in for it. Returns the record, NULL if the keyboard cannot
 * be created.
 */
ZpadDiRecord *zpadSetupDirectInput(int kind, ZpadDevice *device);

/* The part of IDirectInput the code uses. */
typedef struct ZpadDirectInput ZpadDirectInput;
typedef struct ZpadDirectInputVtbl {
    uint32_t pad00[4];
    int32_t(BMX_STDCALL *enumDevices)(ZpadDirectInput *self, uint32_t type,
                                      uint32_t(BMX_STDCALL *callback)(const void *instance, void *ref),
                                      void *ref, uint32_t flags); /* 0x10 */
    uint32_t pad14[4];                                            /* 0x14 */
    int32_t(BMX_STDCALL *createDeviceEx)(ZpadDirectInput *self, const ZpadGuid *guid, const ZpadGuid *iid,
                                         ZpadDiDevice **out, void *outer); /* 0x24 */
} ZpadDirectInputVtbl;

_Static_assert(__builtin_offsetof(ZpadDirectInputVtbl, createDeviceEx) == 0x24, "IDirectInput vtable");

struct ZpadDirectInput {
    const ZpadDirectInputVtbl *vtbl;
};

/* Window of the game, the DirectInput object and the first joystick's GUID. */
#define gZpadWindow       ORIG_VAR(0x00674034, void *, gZpadWindow)
#define gZpadDirectInput  ORIG_VAR(0x00674038, ZpadDirectInput *, gZpadDirectInput)
#define gZpadJoystickGuid ORIG_VAR(0x0067403c, ZpadGuid, gZpadJoystickGuid)

/* Starts DirectInput once (the records list, the object, the joystick GUID); asserts when creation fails. */
int zpadDirectInputInitialize(void);
/* Enumeration callback: remembers the instance GUID (instance + 4) and stops (0). */
/* A stdcall callback; the label keeps the symbol undecorated for the generated hook table. */
#if defined(BMX_HOOK)
uint32_t BMX_STDCALL zpadEnumJoysticksCallback(const void *instance,
                                               void *guid) __asm__("_zpadEnumJoysticksCallback");
#else
uint32_t BMX_STDCALL zpadEnumJoysticksCallback(const void *instance, void *guid);
#endif
/*
 * Creates a device for `guid` and sets its data format and cooperative level; NULL when creation fails, or
 * when a later step fails (those assert).
 */
ZpadDiDevice *zpadCreateDiDevice(void *window, ZpadGuid guid, const void *format, uint32_t flags);

#endif
