/*
 * The original's globals of the engine (src/zge), with their initial values (docs/PORTING.md
 * X2a; types and declarations in src/data/globals.h).
 *
 * Generated once by tools/gen_data.py from NgBMX.exe; from here on it is source like the code.
 */
#include "data/data.h"

/* Functions the data points at that no header declares (assembler name = C name). */
void crtExit(void) __asm__(BMX_SYM("crtExit"));

/* Code nothing serves (unported, unreachable): calling one reports the address and stops. */
BMX_DATA_UNSERVED(004a4a37)
BMX_DATA_UNSERVED(004a8550)
BMX_DATA_UNSERVED(004a85f5)
BMX_DATA_UNSERVED(004ad055)
BMX_DATA_UNSERVED(004ada45)
BMX_DATA_UNSERVED(0051e7cb)
BMX_DATA_UNSERVED(0051e7df)
BMX_DATA_UNSERVED(0051fec4)
BMX_DATA_UNSERVED(0051fec8)
BMX_DATA_UNSERVED(0051ff33)
BMX_DATA_UNSERVED(0051ff37)

const struct BmxGroup_gZpadFormatJoystickGroup gZpadFormatJoystickGroup __attribute__((aligned(16))) = {
    .gZpadFormatJoystick = 24,
    .d_0054e7c9 = {0, 0, 0, 16, 0, 0, 0, 1, 0, 0, 0, 80, 0, 0, 0, 44},
    .p_0054e7dc = (void *)&bmxInputTables.p_004afc60,
};

const struct BmxGroup_gZpadFormatKeyboardGroup gZpadFormatKeyboardGroup __attribute__((aligned(16))) = {
    .gZpadFormatKeyboard = 24,
    .d_0054e7e1 = {0, 0, 0, 16, 0, 0, 0, 2, 0, 0, 0, 0, 1, 0, 0, 0, 1},
    .p_0054e7f4 = (void *)&bmxInputTables.p_004aff20,
    .d_0054e7f8 =
        {0x1a82f8bc, 0x11d23f8b, 0x600074b7, 0xc1163308, 0x1d262760, 0x11cfe957, 0xdb28d6a5, 49412,
         0x28f54685, 0x11d206fd, 0xa0007ab2, 0x963122c9, 0xd38894d1, 0x11d2c052, 0x60002f87, 0xbdb19308,
         0x6fc2cae0, 0x11d2bc78, 0xaa00a6af, 0xb6d82400, 0xd2ac28c0, 0x11d1b39b, 0x60000487, 0xbdb19308,
         0xd2ac28bf, 0x11d1b39b, 0x60000487, 0xbdb19308, 0xd2ac28be, 0x11d1b39b, 0x60000487, 0xbdb19308,
         0xd2ac28bd, 0x11d1b39b, 0x60000487, 0xbdb19308, 0x2befc277, 0x11d25497, 0xa000cbbc, 0xebe622c9,
         0xd2ac28ba, 0x11d1b39b, 0x60000487, 0xbdb19308, 0x7d43d03,  0x11d26523, 0x60001d87, 0xbdb19308,
         0xf96029a1, 0x11d24282, 0x60001787, 0xbdb19308, 0xa3afdcc7, 0x11d1d3ee, 0xa0008dbc, 0xebe622c9,
         0xf96029a2, 0x11d24282, 0x60001787, 0xbdb19308, 0xd2ac28b5, 0x11d1b39b, 0x60000487, 0xbdb19308,
         0x68a04844, 0x11d1d13d, 0xaa00a6af, 0xb6d82400, 0x2ffaaca2, 0x11d25dca, 0xaa00a6af, 0xb6d82400,
         0xf17e8673, 0x11d1c3b4, 0x60000b87, 0xbdb19308, 0xfb09565b, 0x11d23631, 0xa000b8bc, 0xebe622c9,
         0xd2ac28b2, 0x11d1b39b, 0x60000487, 0xbdb19308, 0xd2ac28b1, 0x11d1b39b, 0x60000487, 0xbdb19308,
         0xd2ac28b0, 0x11d1b39b, 0x60000487, 0xbdb19308, 0x65b76fa5, 0x11d2ff37, 0xc0004e81, 0x586ea34f,
         0x45fc707e, 0x11d21db4, 0xa000acbc, 0xebe622c9, 0x45fc707d, 0x11d21db4, 0xa000acbc, 0xebe622c9,
         0x45fc707c, 0x11d21db4, 0xa000acbc, 0xebe622c9, 0x45fc707b, 0x11d21db4, 0xa000acbc, 0xebe622c9,
         0x6621075,  0x11d1e92e, 0xc000c5a8, 0x6e72a34f, 0xd2ac28ab, 0x11d1b39b, 0x60000487, 0xbdb19308,
         0xd2ac28aa, 0x11d1b39b, 0x60000487, 0xbdb19308, 0xd2ac28a9, 0x11d1b39b, 0x60000487, 0xbdb19308,
         0x1db1ae6b, 0x11d1e92e, 0xc000c5a8, 0x6e72a34f, 0xd2ac28a8, 0x11d1b39b, 0x60000487, 0xbdb19308,
         0xd2ac28a7, 0x11d1b39b, 0x60000487, 0xbdb19308, 0xd2ac28af, 0x11d1b39b, 0x60000487, 0xbdb19308,
         0xd2ac28ad, 0x11d1b39b, 0x60000487, 0xbdb19308, 0xd2ac28ac, 0x11d1b39b, 0x60000487, 0xbdb19308,
         0xd2ac28a5, 0x11d1b39b, 0x60000487, 0xbdb19308, 0xd2ac28a4, 0x11d1b39b, 0x60000487, 0xbdb19308,
         0xd2ac28a1, 0x11d1b39b, 0x60000487, 0xbdb19308, 0xd2ac289f, 0x11d1b39b, 0x60000487, 0xbdb19308,
         0xd2ac289e, 0x11d1b39b, 0x60000487, 0xbdb19308, 0x28f97ef7, 0x11d29538, 0xc000a997, 0x586ea34f,
         0xd2ac289d, 0x11d1b39b, 0x60000487, 0xbdb19308, 0xd2ac289c, 0x11d1b39b, 0x60000487, 0xbdb19308,
         0xd2ac289b, 0x11d1b39b, 0x60000487, 0xbdb19308, 0xd2ac289a, 0x11d1b39b, 0x60000487, 0xbdb19308,
         0x81f75bc5, 0x11d24e5d, 0xa000c7bc, 0xebe622c9, 0xd2ac2899, 0x11d1b39b, 0x60000487, 0xbdb19308,
         0xd2ac2893, 0x11d1b39b, 0x60000487, 0xbdb19308, 0xd2ac2898, 0x11d1b39b, 0x60000487, 0xbdb19308,
         0xd2ac2896, 0x11d1b39b, 0x60000487, 0xbdb19308, 0x79ba9e00, 0x11d1b6ee, 0xc000be86, 0xef8fbf4f,
         0xd2ac2894, 0x11d1b39b, 0x60000487, 0xbdb19308, 0xd2ac2892, 0x11d1b39b, 0x60000487, 0xbdb19308,
         0xf17e8672, 0x11d1c3b4, 0x60000b87, 0xbdb19308, 0xd2ac2890, 0x11d1b39b, 0x60000487, 0xbdb19308,
         0xd2ac288f, 0x11d1b39b, 0x60000487, 0xbdb19308, 0xd2ac288e, 0x11d1b39b, 0x60000487, 0xbdb19308,
         0xd2ac288d, 0x11d1b39b, 0x60000487, 0xbdb19308, 0xd2ac288c, 0x11d1b39b, 0x60000487, 0xbdb19308,
         0xd2ac288b, 0x11d1b39b, 0x60000487, 0xbdb19308, 0xd2ac288a, 0x11d1b39b, 0x60000487, 0xbdb19308,
         0xd2ac2888, 0x11d1b39b, 0x60000487, 0xbdb19308, 0xd2ac2887, 0x11d1b39b, 0x60000487, 0xbdb19308,
         0xd2ac2886, 0x11d1b39b, 0x60000487, 0xbdb19308, 0xd2ac2885, 0x11d1b39b, 0x60000487, 0xbdb19308,
         0xd2ac2884, 0x11d1b39b, 0x60000487, 0xbdb19308, 0xd2ac2883, 0x11d1b39b, 0x60000487, 0xbdb19308,
         0xd2ac2882, 0x11d1b39b, 0x60000487, 0xbdb19308, 0xd2ac2881, 0x11d1b39b, 0x60000487, 0xbdb19308,
         0xfedfae25, 0x11d1e46e, 52906,      0x12ac75f8, 0xcfa7cdc2, 0x11d200a1, 54698,      0x12ac75f8,
         0xcda8d611, 0x11d2684a, 0x60001e87, 0xbdb19308, 0x4cb5622,  0x11d232e5, 0xaa00a6af, 0xb6d82400,
         0x178f2f28, 0x11d1c364, 24743,      0x12ac75f8, 0x268a0fa1, 0x11d260f2, 0xaa00a6af, 0xb6d82400,
         0x268a0fa0, 0x11d260f2, 0xaa00a6af, 0xb6d82400, 0x2a91f713, 0x11d2a4bf, 0x6000dfbb, 0xd8db3308,
         0x178f2f28, 0x11d1c364, 24743,      0x12ac75f8, 0xaa97845,  0x11d1c877, 0x60000c87, 0xbdb19308,
         0xaa97844,  0x11d1c877, 0x60000c87, 0xbdb19308, 0x865fd372, 0x11d29f67, 0x60002a87, 0xbdb19308,
         0xf14599e5, 0x11d24689, 0xaa00a6af, 0xb6d82400, 0x178f2f27, 0x11d1c364, 24743,      0x12ac75f8,
         0x6496aba2, 0x11d261b0, 0xaa00a6af, 0xb6d82400, 0x6496aba1, 0x11d261b0, 0xaa00a6af, 0xb6d82400,
         0x178f2f26, 0x11d1c364, 24743,      0x12ac75f8, 0x178f2f25, 0x11d1c364, 24743,      0x12ac75f8,
         0x178f2f24, 0x11d1c364, 24743,      0x12ac75f8, 0x6fc2cae1, 0x11d2bc78, 0xaa00a6af, 0xb6d82400,
         0xd2ac287e, 0x11d1b39b, 0x60000487, 0xbdb19308, 0xd2ac287d, 0x11d1b39b, 0x60000487, 0xbdb19308,
         0xd2ac287c, 0x11d1b39b, 0x60000487, 0xbdb19308, 0xd2ac287b, 0x11d1b39b, 0x60000487, 0xbdb19308,
         0xd2ac287a, 0x11d1b39b, 0x60000487, 0xbdb19308, 0xced153e7, 0x11d23606, 63929,      0x12ac75f8,
         0x8f2d8c9,  0x11d237c2, 63929,      0x12ac75f8, 0xd2ac2878, 0x11d1b39b, 0x60000487, 0xbdb19308,
         0x6536115a, 0x11d27b2d, 6330,       0x12ac75f8, 0x58c2b4d0, 0x11d146e7, 0xa000ac89, 0x294105c9,
         0x480ff4b0, 0x11d128b2, 0xc000f7be, 0xef8fbf4f, 0x636b9f10, 0x11d10c7d, 0x2000b295, 0x2174dcaf,
         0x56a86897, 0x11ce0ad4, 0x20003ab0, 0x70a70baf, 0xc2413342, 0x11d21c1b, 0xc000f594, 0xca8ac24f,
         0xc2413340, 0x11d21c1b, 0xc000f594, 0xca8ac24f, 0xc241333f, 0x11d21c1b, 0xc000f594, 0xca8ac24f,
         0x31efac30, 0x11d0515c, 0xaa00aaa9, 0x93be6100, 0xb0210783, 0x11d089cd, 0xa00008af, 0x16cd25c9,
         0xb0210782, 0x11d089cd, 0xa00008af, 0x16cd25c9, 0xb0210781, 0x11d089cd, 0xa00008af, 0x16cd25c9,
         0x279afa86, 0x11ce4981, 0x200021a5, 0x60e50baf, 0x279afa84, 0x11ce4981, 0x200021a5, 0x60e50baf,
         0x279afa85, 0x11ce4981, 0x200021a5, 0x60e50baf, 0x279afa83, 0x11ce4981, 0x200021a5, 0x60e50baf,
         0xb0210780, 0x11d089cd, 0xa00008af, 0x16cd25c9, 0x47d4d946, 0x11cf62e8, 0x4544bc93, 21587,
         0xf2f0ce00, 0x11cfe0af, 0xa0004e9c, 0x5e4205c9, 0xe4524541, 0x11d18ea5, 0x6000968a, 0x1114b097,
         0xe63232a0, 0x11d09dbf, 0xa000c19c, 0x5e4205c9, 0xc4a54da0, 0x11cfe0af, 0xa0004e9c, 0x5e4205c9,
         0x1fd92e0,  0x11d0a2ff, 0xa0004f9c, 0x5e4205c9, 0xf6dcc200, 0x11d0a2fe, 0xa0004f9c, 0x5e4205c9,
         0xba5a7a70, 0x11d09dbf, 0xa000c19c, 0x5e4205c9, 0x78ec89a0, 0x11cfe0af, 0xa0004e9c, 0x5e4205c9,
         0x59b95640, 0x11d09667, 32167,      0xfcab03f8, 0x7d916c0,  0x11cfe0af, 0xa0004e9c, 0x5e4205c9,
         0x1318f560, 0x11d0912c, 0xa000aa9d, 0xcb430ac9, 0x48784000, 0x11d0d219, 0xc00039ba, 0x67edd74f,
         0xb4319322, 0x11d0d20d, 0xc00039ba, 0x67edd74f, 0xf56920a0, 0x11d0d218, 0xc00039ba, 0x67edd74f,
         0x762ccda1, 0x11d0d916, 0xc00039ba, 0x67edd74f, 0x2fe8f810, 0x11d0b2a5, 34727,      0xfcab03f8,
         0x2db72491, 0x11d1652c, 43175,      0xfcab03f8, 0x2db72490, 0x11d1652c, 43175,      0xfcab03f8,
         0x1bb4af80, 0x11d0a303, 0xa0004f9c, 0x5e4205c9, 0x194c220,  0x11d0a303, 0xa0004f9c, 0x5e4205c9,
         0x26c66a70, 0x11cfb367, 0xaa0024a0, 0xac576100, 0xaf465c71, 0x11cf9588, 0xaa0020a0, 0xac576100,
         0x5454e9a0, 0x11cedb65, 0xaa001c92, 0x72496c00, 0x44eaa760, 0x11cfcb68, 0xa0004e9c, 0x5e4205c9,
         0xf1d6860,  0x11cf88d9, 0xa0004e9c, 0x5e4205c9, 0x36e95ee0, 0x11cf8577, 0x80000c96, 0x824e53c7,
         0x685bc400, 0x11cf9d2c, 0xaa00cda9, 0xe3866800, 0xd1eb6d20, 0x11d08923, 0xa000979d, 0xcb430ac9,
         0xab1c531,  0x11d14745, 41383,      0xfcab03f8, 0xab1c530,  0x11d14745, 41383,      0xfcab03f8,
         0x133efe41, 0x11d032dc, 0xa000fb9c, 0xcb430ac9, 0x133efe40, 0x11d032dc, 0xa000fb9c, 0xcb430ac9,
         0x9d460580, 0x11cfa822, 0x80000c96, 0x824e53c7, 0x2b74f7c0, 0x11cf9154, 0xaa00cda9, 0xe3866800,
         0x745a17a0, 0x11d074d3, 0xa000feb6, 0xda570fc9, 0x4d36e96f, 0x11cee325, 0x8c1bf,    0x1803e12b,
         0x4d36e96c, 0x11cee325, 0x8c1bf,    0x1803e12b, 0x4d36e96b, 0x11cee325, 0x8c1bf,    0x1803e12b,
         0xeec6993a, 0x11d2b3fd, 0xc00016a9, 0x3886b94f, 0x1de12ab1, 0x11cfc9f5, 0x4544c7bf, 21587,
         0x2538130,  0x11d0898f, 0xa000d09a, 0x356ea0c9, 0x13541c2b, 0x11d08e33, 0xa000d09a, 0x356ea0c9,
         0x13541c2a, 0x11d08e33, 0xa000d09a, 0x356ea0c9, 0x13541c29, 0x11d08e33, 0xa000d09a, 0x356ea0c9,
         0x13541c28, 0x11d08e33, 0xa000d09a, 0x356ea0c9, 0x13541c27, 0x11d08e33, 0xa000d09a, 0x356ea0c9,
         0x13541c26, 0x11d08e33, 0xa000d09a, 0x356ea0c9, 0x13541c25, 0x11d08e33, 0xa000d09a, 0x356ea0c9,
         0x13541c24, 0x11d08e33, 0xa000d09a, 0x356ea0c9, 0x13541c23, 0x11d08e33, 0xa000d09a, 0x356ea0c9,
         0x13541c22, 0x11d08e33, 0xa000d09a, 0x356ea0c9, 0x13541c21, 0x11d08e33, 0xa000d09a, 0x356ea0c9,
         0x13541c20, 0x11d08e33, 0xa000d09a, 0x356ea0c9, 0x6f1d2b83, 0x11cfd5a0, 0x4544c7bf, 21587,
         0x6f1d2b82, 0x11cfd5a0, 0x4544c7bf, 21587,      0x6f1d2b81, 0x11cfd5a0, 0x4544c7bf, 21587,
         0x6f1d2b80, 0x11cfd5a0, 0x4544c7bf, 21587,      0x6f1d2b70, 0x11cfd5a0, 0x4544c7bf, 21587},
};

const struct BmxGroup_gZpadGuidKeyboardGroup gZpadGuidKeyboardGroup __attribute__((aligned(16))) = {
    .gZpadGuidKeyboard = {.data1 = 0x6f1d2b61,
                          .data2 = 54688,
                          .data3 = 4559,
                          .data4 = {191, 199, 68, 69, 83, 84}},
    .d_0054f308 = {0x6f1d2b60, 0x11cfd5a0, 0x4544c7bf, 21587, 0xa36d02f3, 0x11cfc9f3, 0x4544c7bf, 21587},
};

/* 0x0054f328 */
const uint32_t d_0054f328[4] = {0xa36d02f2, 0x11cfc9f3, 0x4544c7bf, 21587};

/* 0x0054f338 */
const uint32_t d_0054f338[8] = {0x55728220, 0x11cfd33c, 0x4544c7bf, 21587,
                                0xa36d02f0, 0x11cfc9f3, 0x4544c7bf, 21587};

/* 0x0054f358 */
const uint32_t d_0054f358[4] = {0xa36d02e4, 0x11cfc9f3, 0x4544c7bf, 21587};

/* 0x0054f368 */
const uint32_t d_0054f368[4] = {0xa36d02e3, 0x11cfc9f3, 0x4544c7bf, 21587};

/* 0x0054f378 */
const uint32_t d_0054f378[4] = {0xa36d02f5, 0x11cfc9f3, 0x4544c7bf, 21587};

/* 0x0054f388 */
const uint32_t d_0054f388[4] = {0xa36d02f4, 0x11cfc9f3, 0x4544c7bf, 21587};

/* 0x0054f398 */
const uint32_t d_0054f398[4] = {0xa36d02e2, 0x11cfc9f3, 0x4544c7bf, 21587};

/* 0x0054f3a8 */
const uint32_t d_0054f3a8[4] = {0xa36d02e1, 0x11cfc9f3, 0x4544c7bf, 21587};

/* 0x0054f3b8 */
const uint32_t d_0054f3b8[20] = {0xa36d02e0, 0x11cfc9f3, 0x4544c7bf, 21587,      0x57d7c6bd,
                                 0x11d32356, 0xc0009d8e, 0xae44684f, 0x57d7c6bc, 0x11d32356,
                                 0xc0009d8e, 0xae44684f, 0xe7e1f7c0, 0x11d088d2, 0xa000d09a,
                                 0x356ea0c9, 0x5944e683, 0x11cfc92e, 0x4544c7bf, 21587};

const struct BmxGroup_gZpadIidDeviceGroup gZpadIidDeviceGroup __attribute__((aligned(16))) = {
    .gZpadIidDevice = {.data1 = 0x5944e682,
                       .data2 = 51502,
                       .data3 = 4559,
                       .data4 = {191, 199, 68, 69, 83, 84}},
    .d_0054f418 = {0x5944e681, 0x11cfc92e, 0x4544c7bf, 21587, 0x5944e680, 0x11cfc92e, 0x4544c7bf, 21587,
                   0x9a4cb685, 0x11d3236d, 0xc0009d8e, 0xae44684f},
};

const struct BmxGroup_gZpadIidDirectInputGroup gZpadIidDirectInputGroup __attribute__((aligned(16))) = {
    .gZpadIidDirectInput = {.data1 = 0x9a4cb684,
                            .data2 = 9069,
                            .data3 = 4563,
                            .data4 = {142, 157, 0, 192, 79, 104, 68, 174}},
    .d_0054f458 =
        {0x5944e663, 0x11cfaa8a, 0x4544c7bf, 21587,      0x5944e662, 0x11cfaa8a, 0x4544c7bf, 21587,
         0x89521361, 0x11cfaa8a, 0x4544c7bf, 21587,      0x89521360, 0x11cfaa8a, 0x4544c7bf, 21587,
         0x25e609e1, 0x11cfb259, 0x4544c7bf, 21587,      0x25e609e0, 0x11cfb259, 0x4544c7bf, 21587,
         0x3d82ab43, 0x11cf62da, 0x200039ab, 0x33e471af, 0x3d82ab46, 0x11cf62da, 0x200039ab, 0x33e471af,
         0x3d82ab45, 0x11cf62da, 0x200039ab, 0x33e471af, 0x3d82ab44, 0x11cf62da, 0x200039ab, 0x33e471af,
         0x3d82ab43, 0x11cf62da, 0x200039ab, 0x33e471af, 0x3d82ab42, 0x11cf62da, 0x200039ab, 0x33e471af,
         0x3d82ab41, 0x11cf62da, 0x200039ab, 0x33e471af, 0x3d82ab40, 0x11cf62da, 0x200039ab, 0x33e471af,
         0x4516ec43, 0x11d08f20, 28059,      0xc31b78c0, 0x7f5d5ea0, 0x11d1d53a, 0xa000c082, 0x717269c9,
         0x7f0f21e1, 0x11d1bfe1, 0xa000c082, 0x717269c9, 0x7f0f21e0, 0x11d1bfe1, 0xa000c082, 0x717269c9,
         0x98116aa0, 0x11d1bdba, 0xa000c082, 0x717269c9, 0x8a63c360, 0x11d0997d, 0x80001c94, 0x7bfa0cc8,
         0x3a23eea1, 0x11d094b1, 0x200039ab, 0x33e471af, 0x3a23eea0, 0x11d094b1, 0x200039ab, 0x33e471af,
         0xa8a98ba0, 0x11cfc5e5, 0x800041b9, 0x7bfa0cc8, 0xaed22741, 0x11cfb31f, 0xaa00d59d, 0x2f1aa700,
         0xaed22742, 0x11cfb31f, 0xaa00d59d, 0x2f1aa700, 0xaed22740, 0x11cfb31f, 0xaa00d59d, 0x2f1aa700,
         0xe5745280, 0x11cfb24f, 0xaa00d59d, 0x2f1aa700, 0x3d82ab51, 0x11cf62da, 0x200039ab, 0x33e471af,
         0x3d82ab4a, 0x11cf62da, 0x200039ab, 0x33e471af, 0xe2bf56c3, 0x11cf840f, 0x4000528f, 0xa3943533,
         0xe2bf56c2, 0x11cf840f, 0x4000528f, 0xa3943533, 0xe2bf56c1, 0x11cf840f, 0x4000528f, 0xa3943533,
         0xe2bf56c0, 0x11cf840f, 0x4000528f, 0xa3943533, 0xf406b180, 0x11cf7b3b, 0x4000528f, 0xa3943533,
         0x4885ae63, 0x11cf78e8, 0x4000528f, 0xa3943533, 0xed1ec5c0, 0x11d0c0a8, 0x80001c94, 0x7bfa0cc8,
         0x537da6a0, 0x11d0ca37, 0x80001c94, 0x7bfa0cc8, 0x4885ae60, 0x11cf78e8, 0x4000528f, 0xa3943533,
         0x1630b821, 0x11cf7842, 0x4000528f, 0xa3943533, 0x1630b820, 0x11cf7842, 0x4000528f, 0xa3943533,
         0xa42790e2, 0x11cf7810, 0x4000528f, 0xa3943533, 0xa42790e1, 0x11cf7810, 0x4000528f, 0xa3943533,
         0xa42790e0, 0x11cf7810, 0x4000528f, 0xa3943533, 0xd3e16e81, 0x11cf7835, 0x4000528f, 0xa3943533,
         0x35ff44e0, 0x11cf6c7c, 0x4000528f, 0xa3943533, 0x1411843,  0x11cf7786, 0x4000528f, 0xa3943533,
         0xd3e16e80, 0x11cf7835, 0x4000528f, 0xa3943533, 0x1411842,  0x11cf7786, 0x4000528f, 0xa3943533,
         0x1411841,  0x11cf7786, 0x4000528f, 0xa3943533, 0x1411840,  0x11cf7786, 0x4000528f, 0xa3943533,
         0x10dd46a9, 0x11cf775b, 0x4000528f, 0xa3943533, 0x10dd46a8, 0x11cf775b, 0x4000528f, 0xa3943533,
         0x3d82ab50, 0x11cf62da, 0x200039ab, 0x33e471af, 0x3d82ab4f, 0x11cf62da, 0x200039ab, 0x33e471af,
         0xf6f23f45, 0x11cf7686, 0x4000528f, 0xa3943533, 0xf6f23f44, 0x11cf7686, 0x4000528f, 0xa3943533,
         0xf6f23f43, 0x11cf7686, 0x4000528f, 0xa3943533, 0xf6f23f40, 0x11cf7686, 0x4000528f, 0xa3943533,
         0xf6f23f42, 0x11cf7686, 0x4000528f, 0xa3943533, 0xf6f23f41, 0x11cf7686, 0x4000528f, 0xa3943533,
         0x3d82ab46, 0x11cf62da, 0x200039ab, 0x33e471af, 0x35ff44e1, 0x11cf6c7c, 0x4000528f, 0xa3943533,
         0x3d82ab4d, 0x11cf62da, 0x200039ab, 0x33e471af, 0x3d82ab5f, 0x11cf62da, 0x200039ab, 0x33e471af,
         0x3d82ab5e, 0x11cf62da, 0x200039ab, 0x33e471af, 0x3d82ab44, 0x11cf62da, 0x200039ab, 0x33e471af,
         0x2b957100, 0x11cf9e9a, 0x200039ab, 0x33e471af, 0xc5016cc0, 0x11ced273, 18604,      0xa12538c0,
         0x4516ec41, 0x11d08f20, 28059,      0xc31b78c0, 0x4516ec83, 0x11d08f20, 28059,      0xc31b78c0,
         0x4516ecc8, 0x11d08f20, 28059,      0xc31b78c0, 0x2bc49361, 0x11cf8327, 19116,      0xa12538c0,
         0x5434e733, 0x11d16d66, 3003,       0x5a8675f8, 0x4516ec7b, 0x11d08f20, 28059,      0xc31b78c0,
         0x4516ec79, 0x11d08f20, 28059,      0xc31b78c0, 0x242f6bc1, 0x11d03849, 28059,      0xc31b78c0,
         0x86b44e25, 0x11d19c82, 0xa0000bbb, 0xa6a081c9, 0xaf359780, 0x11cf6ba3, 19116,      0xa12538c0,
         0x59163de0, 0x11cf6d43, 19116,      0xa12538c0, 0xd5f1cae0, 0x11d14bd7, 0x600074b9, 0xf3453e08,
         0xeb16cb17, 0x11ced271, 18604,      0xa12538c0, 0xeb16cb16, 0x11ced271, 18604,      0xa12538c0,
         0xeb16cb14, 0x11ced271, 18604,      0xa12538c0, 0xeb16cb13, 0x11ced271, 18604,      0xa12538c0,
         0xeb16cb12, 0x11ced271, 18604,      0xa12538c0, 0xeb16cb11, 0x11ced271, 18604,      0xa12538c0,
         0xeb16cb10, 0x11ced271, 18604,      0xa12538c0, 0x242f6bc2, 0x11d03849, 28059,      0xc31b78c0,
         0xff6b7f79, 0x11d1a40e, 63889,      0x668e75f8, 0xeb16cb0e, 0x11ced271, 18604,      0xa12538c0,
         0xff6b7f77, 0x11d1a40e, 63889,      0x668e75f8, 0xeb16cb0d, 0x11ced271, 18604,      0xa12538c0,
         0xff6b7f75, 0x11d1a40e, 63889,      0x668e75f8, 0xeb16cb0b, 0x11ced271, 18604,      0xa12538c0,
         0xeb16cb0a, 0x11ced271, 18604,      0xa12538c0, 0xff6b7f73, 0x11d1a40e, 63889,      0x668e75f8,
         0x120f30c0, 0x11d01629, 0x80001c94, 0x7bfa0cc8, 0xeb16cb09, 0x11ced271, 18604,      0xa12538c0,
         0xeb16cb08, 0x11ced271, 18604,      0xa12538c0, 0x4516ec81, 0x11d08f20, 28059,      0xc31b78c0,
         0xeb16cb07, 0x11ced271, 18604,      0xa12538c0, 0x4516ec82, 0x11d08f20, 28059,      0xc31b78c0,
         0x4516ec77, 0x11d08f20, 28059,      0xc31b78c0, 0xa3a80d02, 0x11cf6e12, 19116,      0xa12538c0,
         0xa3a80d01, 0x11cf6e12, 19116,      0xa12538c0, 0xeb16cb04, 0x11ced271, 18604,      0xa12538c0,
         0xff6b7f70, 0x11d1a40e, 63889,      0x668e75f8, 0xc3dfbd60, 0x11d03988, 49822,      0xc31a29c0,
         0xeb16cb03, 0x11ced271, 18604,      0xa12538c0, 0x4a1b1be6, 0x11d1bfed, 0xa000d88e, 0x82a467c9,
         0xeb16cb02, 0x11ced271, 18604,      0xa12538c0, 0x549f498b, 0x11d1bfeb, 0xa000d88e, 0x82a467c9,
         0x4516ec78, 0x11d08f20, 28059,      0xc31b78c0, 0xe9e19280, 0x11cf6e05, 19116,      0xa12538c0,
         0x4516ec7c, 0x11d08f20, 28059,      0xc31b78c0, 0xeb16cb00, 0x11ced271, 18604,      0xa12538c0,
         0x5434e72d, 0x11d16d66, 3003,       0x5a8675f8, 0x4516ec40, 0x11d08f20, 28059,      0xc31b78c0,
         0xde9eaa8,  0x11d03b84, 28059,      0xc31b78c0, 0xde9eaa7,  0x11d03b84, 28059,      0xc31b78c0,
         0xde9eaa6,  0x11d03b84, 28059,      0xc31b78c0, 0xde9eaa3,  0x11d03b84, 28059,      0xc31b78c0,
         0xde9eaa2,  0x11d03b84, 28059,      0xc31b78c0, 0xde9eaa1,  0x11d03b84, 28059,      0xc31b78c0,
         0x4fa3569b, 0x11cf623f, 19116,      0xa12538c0, 0x4fa3569a, 0x11cf623f, 19116,      0xa12538c0,
         0x4fa35699, 0x11cf623f, 19116,      0xa12538c0, 0x4fa35698, 0x11cf623f, 19116,      0xa12538c0,
         0x4fa35697, 0x11cf623f, 19116,      0xa12538c0, 0x4fa35696, 0x11cf623f, 19116,      0xa12538c0,
         0x4fa35695, 0x11cf623f, 19116,      0xa12538c0, 0x4fa35694, 0x11cf623f, 19116,      0xa12538c0,
         0x4fa35693, 0x11cf623f, 19116,      0xa12538c0, 0x4fa35692, 0x11cf623f, 19116,      0xa12538c0,
         0x4fa35691, 0x11cf623f, 19116,      0xa12538c0, 0x4fa35690, 0x11cf623f, 19116,      0xa12538c0,
         0x4fa3568f, 0x11cf623f, 19116,      0xa12538c0, 0x4fa3568e, 0x11cf623f, 19116,      0xa12538c0,
         0x880baf30, 0x11d0b030, 0x6000a78e, 0x5bea9797, 0x406b2f00, 0x11d13e5a, 0xaa0040b6, 0x6af9a100,
         0xb1122b40, 0x11d15da5, 0xc000cf8f, 0x4e9bc24f, 0x2e04ffa0, 0x11d198e4, 0xa000e18c, 0xa82906c9,
         0x97f861b6, 0x11d160a1, 0xa000d08c, 0xa82906c9, 0xf0b0e8e2, 0x11d15f97, 0xa000d08c, 0xa82906c9,
         0x9d792804, 0x11d15fa8, 0xa000d08c, 0xa82906c9, 0xe05c8472, 0x11d151d4, 0xa000ce8c, 0xa82906c9,
         0xf828169c, 0x11d2a8e8, 0xa000f2a1, 0xf6ea83c9, 0x3b8a0466, 0x11d1f269, 0xc0000b88, 0xc530d94f,
         0x93869880, 0x11d136cf, 0xaa001b9b, 0xaeb8bb00, 0x7de41f80, 0x11d09d93, 0xa000ab89, 0x294105c9,
         0xffaa7540, 0x11d07aa8, 0xa000069b, 0xb8a303c9, 0x80863800, 0x11d06b06, 0xa000069b, 0xb8a303c9,
         0x86c4fa80, 0x11d08d84, 0xc000e894, 0x3741c34f, 0xddf41230, 0x11d0ec0a, 0xaa00b6a9, 0x3e99c000,
         0xba584e1,  0x11d070b6, 0xaa009d88, 0x6ab7bb00, 0xefd60cc3, 0x11d049e7, 0xaa009d88, 0x6ab7bb00,
         0xefd60cc2, 0x11d049e7, 0xaa009d88, 0x6ab7bb00, 0xefd60cc1, 0x11d049e7, 0xaa009d88, 0x6ab7bb00,
         0xefd60cc0, 0x11d049e7, 0xaa009d88, 0x6ab7bb00, 0xf5049e7d, 0x11d24861, 0xa00007a4, 0xa82906c9,
         0x7a503555, 0x11d14a83, 0xa000dba5, 0xf86703c9, 0xb0ab3b61, 0x11d133d7, 0xc00081a9, 0x74b1d74f,
         0x93281500, 0x11d08cf8, 0xa000ab89, 0x294105c9, 0x4417c146, 0x11cf33ad, 28545,      0x6e1520c0,
         0x4417c145, 0x11cf33ad, 28545,      0x6e1520c0, 0xca9c46f4, 0x11d1d3c5, 0x60005ab7, 0x12b35208,
         0x93281503, 0x11d08cf8, 0xa000ab89, 0x294105c9, 0x4417c144, 0x11cf33ad, 28545,      0x6e1520c0,
         0x4417c142, 0x11cf33ad, 28545,      0x6e1520c0},
};

/* 0x0054fee8 */
const uint32_t d_0054fee8[32] = {
    0x93281502, 0x11d08cf8, 0xa000ab89, 0x294105c9, 0x2cdcd9e0, 0x11cf25a0, 0xaa001aa3, 0x5633b900,
    0xf5049e79, 0x11d24861, 0xa00007a4, 0xa82906c9, 0xb0ab3b60, 0x11d133d7, 0xc00081a9, 0x74b1d74f,
    0x93281501, 0x11d08cf8, 0xa000ab89, 0x294105c9, 0x64108800, 0x11d0957d, 0xa000ab89, 0x294105c9,
    0xf5049e78, 0x11d24861, 0xa00007a4, 0xa82906c9, 0x8767df22, 0x11d1bacc, 0xa0006989, 0xa82906c9};

/* 0x0054ff68 */
const uint32_t d_0054ff68[8] = {0x50936643, 0x11d113e9, 0xa000aa89, 0x294105c9,
                                0x881949a1, 0x11d0d6f3, 0xa000ab89, 0x294105c9};

/* 0x0054ff88 */
const uint32_t d_0054ff88[4] = {0x84e63de0, 0x11cf46aa, 28545, 0x6e1520c0};

/* 0x0054ff98 */
const uint32_t d_0054ff98[12] = {0xa4665c60, 0x11cf2673, 0xaa001aa3, 0x5633b900, 0xf2086b20, 0x11cf259f,
                                 0xaa001aa3, 0x5633b900, 0xf5049e77, 0x11d24861, 0xa00007a4, 0xa82906c9};

/* 0x0054ffc8 */
const uint32_t d_0054ffc8[132] = {
    0xbb223240, 0x11d0e72b, 0xaa00b4a9, 0x3e99c000, 0x6aae1ec1, 0x11d0662a, 0xaa009d88, 0x6ab7bb00,
    0x3bba0080, 0x11cf2421, 0xaa001aa3, 0x5633b900, 0xc9b2d740, 0x11d13356, 0xc000cf8f, 0x4e9bc24f,
    0xaca12120, 0x11d13356, 0xc000cf8f, 0x4e9bc24f, 0x60755da0, 0x11d06a40, 0xa000069b, 0xb8a303c9,
    0x8d56c120, 0x11d06a08, 0xa000069b, 0xb8a303c9, 0x332cf160, 0x11cfda61, 0xa000069b, 0xb8a303c9,
    0x1352a560, 0x11cfda61, 0xa000069b, 0xb8a303c9, 0xfca326a0, 0x11cfda60, 0xa000069b, 0xb8a303c9,
    0xe09c77e0, 0x11cfda60, 0xa000069b, 0xb8a303c9, 0xa07a02e0, 0x11cfda60, 0xa000069b, 0xb8a303c9,
    0x92783220, 0x11cfda60, 0xa000069b, 0xb8a303c9, 0x54f39980, 0x11cfda60, 0xa000069b, 0xb8a303c9,
    0xb36d93e0, 0x11cf2b43, 0xaa00dea2, 0x5633b900, 0x6c142760, 0x11cea733, 0x200021a5, 0x60e50baf,
    0x69c11c3e, 0x11d1b46b, 0xc0007aad, 0x4e9bc24f, 0x4b9f0ee0, 0x11d00d7e, 0xa000069b, 0xb8a303c9,
    0x6c14db85, 0x11cea733, 0x200021a5, 0x60e50baf, 0x6c14db84, 0x11cea733, 0x200021a5, 0x60e50baf,
    0xc03c477e, 0x11d16519, 0xc000528c, 0xc530d94f, 0x51191f1e, 0x11d14f2b, 0xa000c38c, 0xa82906c9,
    0x730c7ffc, 0x11d15347, 0xc0004d8c, 0xc530d94f, 0xe043bc46, 0x11d15317, 0xc0004d8c, 0xc530d94f,
    0x89b2c488, 0x11d14af4, 0xc0004c8c, 0xc530d94f, 0xb9dc4790, 0x11d14af1, 0xc0004c8c, 0xc530d94f,
    0x1bab8e96, 0x4ce39cfe, 0xe9d772bc, 0x75219ae9, 0x6675a80,  0x11d23b9b, 0x60002fb9, 0x5bea9797,
    0xb2b8630,  0x11d0ad35, 0x6000a68e, 0x5bea9797, 0xda044e00, 0x11d069b2, 0xaa00d5a1, 0xbbdfb800,
    0x57805885, 0x11cf6eec, 0x23a84194, 0x270ec103, 0x6c14db81, 0x11cea733, 0x200021a5, 0x60e50baf,
    0x15e65ec0, 0x11d23b9c, 0x60002fb9, 0x5bea9797};

/* 0x005501d8 */
const uint32_t d_005501d8[4] = {0x9c59509a, 0x11d139bd, 0xc0004a8c, 0xc530d94f};

/* 0x005501e8 */
const uint32_t d_005501e8[20] = {0xb3a6f3e0, 0x11cf2b43, 0xaa00dea2, 0x5633b900, 0x6c14db80,
                                 0x11cea733, 0x200021a5, 0x60e50baf, 0x593817a0, 0x11cf7db3,
                                 0xaa00dea2, 0x5633b900, 0x3c305196, 0x11d350db, 0xc000fe9c,
                                 0xc530d94f, 0xd7b70ee0, 0x11cf4340, 0x200063b0, 0x35cdc2af};

/* 0x00550238 */
const double d_00550238 = 2.0;

const struct BmxGroup_d_00550240Group d_00550240Group __attribute__((aligned(16))) = {
    .d_00550240 = 0xffffffff,
    .p_00550244 =
        {
            unserved_0051e7cb,
            unserved_0051e7df,
        },
};

/* 0x0055024c */
const char sSunMonTueWedThuFriSat[23] = "SunMonTueWedThuFriSat";

/* 0x00550263 */
const unsigned char pad_00550263[41] = {0,   74,  97,  110, 70,  101, 98,  77,  97,  114, 65,  112, 114,
                                        77,  97,  121, 74,  117, 110, 74,  117, 108, 65,  117, 103, 83,
                                        101, 112, 79,  99,  116, 78,  111, 118, 68,  101, 99};

/* 0x0055028c */
const char sTZ[4] = "TZ";

const struct BmxGroup_d_00550290Group d_00550290Group __attribute__((aligned(16))) = {
    .d_00550290 = 0xffffffff,
    .p_00550294 =
        {
            unserved_0051fec4,
            unserved_0051fec8,
        },
    .d_0055029c = 0xffffffff,
    .p_005502a0 =
        {
            unserved_0051ff33,
            unserved_0051ff37,
        },
};

/* 0x00551000 */
unsigned char d_00551000[260];

/* 0x00551104 */
unsigned char d_00551104[260];

struct BmxGroup_d_00551208Group d_00551208Group __attribute__((aligned(16))) = {
    .p_0055130c =
        {
            unserved_004a8550,
            unserved_004ada45,
        },
};

/* 0x00551418 */
unsigned char d_00551418[260];

struct BmxGroup_d_0055151cGroup d_0055151cGroup __attribute__((aligned(16))) = {
    .p_00551620 = unserved_004a85f5,
};

/* 0x00551724 */
unsigned char d_00551724[260];

/* 0x00551828 */
unsigned char d_00551828[260];

/* 0x0055192c */
unsigned char d_0055192c[260];

/* 0x0055bf38 */
ZramdiskDeviceName gZramdiskDeviceName = "rd00";

/* 0x0055bf40 */
char sNoDevices[16] = "No devices!";

/* 0x0055bf50 */
char sCouldNotLoadMenuArtAssetFile[44] = "Could not load menu art asset file";

/* 0x0055bf7c */
char sInterfaceAssetWrapperDoesNotExist[48] = "Interface asset wrapper does not exist!";

/* 0x0055bfac */
char sInterfaceArtAlreadyLoaded[36] = "Interface art already loaded!";

/* 0x0055bfd0 */
char sCZaxisBmxSrcComZgeNgIFNgAppC[48] = "C:\\zaxis\\Bmx\\src\\com\\zge\\NgIF\\NgApp.c";

/* 0x0055c000 */
char sBinMovieINTROAVI[24] = "bin\\movie\\INTRO.AVI";

/* 0x0055c018 */
char sBinMovieZAXISAVI[24] = "bin\\movie\\ZAXIS.AVI";

/* 0x0055c030 */
char sBinMovieACCLAIMAVI[28] = "bin\\movie\\ACCLAIM.AVI";

/* 0x0055c04c */
char sDaveMirraFreestyleBMX[32] = "Dave Mirra Freestyle BMX";

/* 0x0055c06c */
char sLOADING[12] = "LOADING";

/* 0x0055c078 */
char sGlobals[12] = "Globals";

/* 0x0055c084 */
char sErrorLoadingLevel[24] = "Error loading level";

/* 0x0055c09c */
char sFailedToLoadInterfaceAssets[40] = "Failed to load interface assets";

/* 0x0055c0c4 */
char sLEGAL2[8] = "LEGAL2";

/* 0x0055c0cc */
char sLEGAL[12] = "LEGAL";

/* 0x00560030 */
BmxType_gZdebugStart gZdebugStart = zrwDbgInitialize;

/* 0x00560034 */
void *p_00560034 = zrwDbgShutdown;

struct BmxGroup_gZProfilePrintAtGroup gZProfilePrintAtGroup __attribute__((aligned(16))) = {
    .gZProfilePrintAt = ZDebugPrintBlya,
    .p_0056003c =
        {
            zrwDbgPrintFloat,
            zrwDbgPrintMatrix,
            zrwDbgPrintVec4,
            zrwDbgPrintVec3,
            zrwDbgPrintAngles,
            zrwDbgPrintAngle,
            zrwDbgSetFpsMode,
        },
};

/* 0x00560058 */
void *p_00560058 = zrwDbgAddCallback;

/* 0x0056005c */
void *p_0056005c = zrwDbgRemoveCallback;

/* 0x00560060 */
void *p_00560060 = zrwDbgAddAxes;

/* 0x00560074, also gGuiDebugPrint */
ZDebugPrintFn gZdebugPrint = DebugString;

struct BmxGroup_gZdebugIndentGroup gZdebugIndentGroup __attribute__((aligned(16))) = {
    .gZdebugIndent = zdebugIndent,
    .p_0056007c = zdebugSetIndent,
};

struct BmxGroup_gZobjCameraVtableGroup gZobjCameraVtableGroup __attribute__((aligned(16))) = {
    .gZobjCameraVtable =
        {
            .destroy = zobjCameraDestroy,
            .draw = zobjCameraDraw,
            .moved = zobjCameraMoved,
            .rotated = zobjCameraRotated,
        },
};

/* 0x00560540 */
ZVec3 gZobjCameraHome = {.z = -100.0f};

/* 0x00560550 */
char sCNgEngineZgeZgeModsZobjCameraC[40] = "C:\\NgEngine\\Zge\\ZgeMods\\zobjCamera.c";

/* 0x00560578 */
char sUnknownType[16] = "Unknown type";

/* 0x00560588 */
char sError[8] = "Error";

/* 0x00560590 */
char sCNgEngineZgeZstringC[28] = "C:\\NgEngine\\Zge\\zstring.c";

/* 0x005605ac */
char sSTRS[8] = "STRS";

/* 0x005605b4 */
char sString0xXNotFoundInSetD[36] = "String(0x%x) not found in set:%d";

/* 0x005605d8 */
char sNotFound[12] = "Not found";

/* 0x005605e4 */
char sGUIMAINBadMacroExpansion[32] = "GUIMAIN: Bad macro expansion\n";

/* 0x00560604 */
char sCNgEngineZgeZquatC[24] = "C:\\NgEngine\\Zge\\zquat.c";

/* 0x0056061c */
char sDivisionBy0[16] = "Division by 0";

/* 0x0056062c */
intptr_t gZfileRamDisk = -1;

/* 0x00560630 */
char sZfileRegisterErrorMessageS[36] = "zfileRegisterErrorMessage(): %s\n";

/* 0x00560654 */
char sAnInvalidDeviceIdentifierWasGiven[40] = "An invalid device identifier was given";

/* 0x0056067c */
char sFailedToCreateFileHandleAllocator[40] = "Failed to create file handle allocator";

/* 0x005606a4 */
char sFileSystemShouldNotBeInitialized[40] = "File system should not be initialized";

/* 0x005606cc */
char sThereAreNoDevicesConnectedToTheSystem[48] = "There are no devices connected to the system.";

/* 0x005606fc */
char sErrorInitializingPlatformFileSystem[40] = "Error initializing platform file system";

/* 0x00560724 */
char sCNgEngineZgeZfileC[24] = "C:\\NgEngine\\Zge\\zfile.c";

/* 0x0056073c */
char sErrorWhenTryingToCloseFileThroughDevice[48] = "Error when trying to close file through device.";

/* 0x0056076c */
char sInvalidFileHandle_0056076c[20] = "Invalid file handle";

/* 0x00560780 */
char sFileSystemHasNotBeenIntialized[40] = "File system has not been intialized.";

/* 0x005607a8 */
char sDeviceListMayBeCorrupt[28] = "Device list may be corrupt";

/* 0x005607c4 */
char sTheDefaultDeviceIsInvalid[32] = "The default device is invalid";

/* 0x005607e4 */
char sInvalidParameter_005607e4[20] = "Invalid parameter";

/* 0x005607f8 */
char sFailedToIdentifyDeviceFromFileName[44] = "Failed to identify device from file name";

/* 0x00560824 */
char sDeviceNameIsNotValid[28] = "Device name is not valid";

/* 0x00560840 */
char sCouldNotDetermineDeviceForFileName[44] = "Could not determine device for file name.";

/* 0x0056086c */
char sInvalidParamters[20] = "Invalid paramters";

/* 0x00560880 */
char sAnInvalidFileNameWasGiven[32] = "An invalid file name was given";

/* 0x005608a0 */
char sErrorOpeningFile[20] = "Error opening file.";

/* 0x005608b4 */
char sFileSystemShouldBeInitialized[36] = "File system should be initialized.";

/* 0x005608d8 */
char sMemoryCorruptionWhenBuildingPathName[44] = "Memory corruption when building path name";

/* 0x00560904 */
char sCNgEngineZgeZmemC[24] = "C:\\NgEngine\\Zge\\zmem.c";

/* 0x0056091c */
char sMemoryAllocationFailed[28] = "Memory allocation failed";

/* 0x00560938 */
char sCanOnlyResizeLastAllocatedBlock[40] = "Can only resize last allocated block";

/* 0x00560960 */
char sCantResize[12] = "Cant resize";

/* 0x0056096c */
char sCannotResizeMemoryWhenAllocationsAreReD[68] =
    "Cannot resize memory when allocations are re-directed to a pool.";

/* 0x005609b0 */
char sUnknownPool[16] = "Unknown pool";

/* 0x005609c0 */
char sUseZmemFreeToResizeBlockTo0[36] = "Use zmemFree to resize block to 0";

/* 0x005609e4 */
char sCouldNotInitializeStaticMemoryManager[44] = "Could not initialize static memory manager";

/* 0x00560a10 */
char sMemoryAllocationFailedOutOfRAM[40] = "Memory allocation failed - Out of RAM";

/* 0x00560a38 */
char sFreeListPoolHasBeenExhausted[36] = "Free list pool has been exhausted.";

/* 0x00560a5c */
char sAnInvalidFreeListWasDestroyed[36] = "An invalid free list was destroyed.";

/* 0x00560a80 */
char sFreeListShouldBeEmptyButFreeBlocksStill[56] = "Free list should be empty, but free blocks still exist.";

/* 0x00560ab8 */
char sFreeListIsCorruptHeadOfUsedBlockListIsN[68] =
    "Free list is corrupt: Head of used block list is not the sentinel.";

/* 0x00560afc */
char sInvalidBlockReleasedFromFreeList[40] = "Invalid block released from free list.";

/* 0x00560b24 */
char sMemoryPoolSystemHasBeenPreviouslyInitia[52] = "Memory pool system has been previously initialized.";

/* 0x00560b58 */
char sMemoryPoolSystemHasBeenPreviouslyShutdo[52] = "Memory pool system has been previously shutdown.";

/* 0x00560b8c */
char sAnInvalidMemoryPoolWasDestroyed[40] = "An invalid memory pool was destroyed.";

/* 0x00560bb4 */
char sMemoryPoolChacheHasBeenBeenExhausted[44] = "Memory pool chache has been been exhausted.";

/* 0x00560be0 */
char sMemoryBeingReleasedIsNotFromThisPool[48] = "Memory being released is not from this pool.";

/* 0x00560c10 */
char sZmemPoolFreeMemoryHasBeenPreviouslyFree[52] = "zmemPoolFree(): Memory has been previously freed.";

/* 0x00560c44 */
char sZpropPropDefHasNoGeometry[36] = "zprop : Prop Def has no geometry!!!";

/* 0x00560c68 */
char sZpropPropDefHeaderSizeInvalid[40] = "zprop : Prop Def header size invalid";

/* 0x00560c90 */
char sZpropPropDefHasNoData[32] = "zprop : Prop Def has no data!!!";

/* 0x00560cb0 */
char sCNgEngineZgeZgeModsZpropC[32] = "C:\\NgEngine\\Zge\\ZgeMods\\zprop.c";

/* 0x00560cd0 */
char sZpropUnableToCreateAnObjBasedOnThePropD[60] =
    "zprop : Unable to create an obj based on the prop def model";

/* 0x00560d0c */
char sZpropNoPropDefCorrespondsToTheProp[44] = "zprop : No prop def corresponds to the prop";

/* 0x00560d38 */
char sZpropPropHeaderSizeInvalid[36] = "zprop : Prop header size invalid";

/* 0x00560d5c */
char sZpropPropHasNoData[28] = "zprop : Prop has no data!!!";

/* 0x00560d78 */
char sZObjUsedD[16] = "ZObj: Used:%d";

/* 0x00560d88 */
char sCanTInitializeWith0[24] = "Can't initialize with 0";

/* 0x00560da0 */
char sCNgEngineZgeZobjC[24] = "C:\\NgEngine\\Zge\\zobj.c";

/* 0x00560db8 */
char sZOBJErrorFailedToAllocateObj[40] = "ZOBJ: Error - failed to allocate obj\n";

/* 0x00560de0 */
char sZOBJObjectIsAlreadyAttachedToAnother[48] = "ZOBJ: Object is already attached to another\n";

/* 0x00560e10 */
char sErrorInHierarchy[20] = "Error in hierarchy";

/* 0x00560e24 */
char sNONAMEKzobjTypeD[24] = "NONAME: kzobjType=%d\n";

/* 0x00560e3c */
char sS_00560e3c[4] = "%s\n";

/* 0x00560e40 */
char sZOBJMemAllocatedDObjectsInUseD[48] = "ZOBJ: MemAllocated:%d, Objects in use:%d\n";

struct BmxGroup_gZobjSetColorGroup gZobjSetColorGroup __attribute__((aligned(16))) = {
    .gZobjSetColor = (ZobjColorFn)zrwModelSetDrawColor,
    .p_00560ef8 =
        {
            zrwModelSetOnUpdate,
            zrwModelGeomNotCoded,
            zrwModelNextIndex,
            zrwModelLockGeometry,
            zrwModelUnlockGeometry,
            zrwModelGetVertex,
            zrwModelSetVertex,
            zrwModelDumpInfo,
        },
};

/* 0x00560f18 */
void *p_00560f18 = zrwModelPrintUsage;

/* 0x00560f1c */
char sCNgEngineZgeComZglRwZrwModelC[40] = "C:\\NgEngine\\Zge\\Com\\ZglRw\\zrwModel.c";

/* 0x00560f44 */
char sBodyPartNotMatchedHashX[32] = "BodyPart not matched Hash:%x\n";

/* 0x00560f64 */
char sPart0xXNotFoundInModelS[36] = "Part (0x%x)not found in model:%s\n";

/* 0x00560f88 */
char sMaterialIndexOutOfRange[32] = "Material index out of range";

/* 0x00561468 */
BmxType_gZrwGraphicLibraryInitialize gZrwGraphicLibraryInitialize = zrwGraphicLibraryInitialize;

/* 0x0056146c */
void *p_0056146c = zrwGraphicLibraryShutdown;

/* 0x0056147c */
BmxType_gZdebugFailHandler gZdebugFailHandler = zrwEpicFail;

/* 0x0056149c */
ZMemAllocFn gRwMalloc;

/* 0x005614a0 */
ZMemFreeFn gRwFree;

/* 0x005614a4 */
ZMemReallocFn gRwRealloc;

struct BmxGroup_gZobjLightOpsGroup gZobjLightOpsGroup __attribute__((aligned(16))) = {
    .gZobjLightOps = (void *)zobjLightDestroy,
    .p_00561860 =
        {
            zobjLightSetParam,
            zobjLightSetRotation,
        },
};

/* 0x0056187c */
char sCNgEngineZgeZgeModsZobjLightC[36] = "C:\\NgEngine\\Zge\\ZgeMods\\zobjLight.c";

struct BmxGroup_p_005618a0Group p_005618a0Group __attribute__((aligned(16))) = {
    .p_005618a0 =
        {
            &sZMD,
            zsceneLoadModelFile,
        },
};

struct BmxGroup_p_005618b0Group p_005618b0Group __attribute__((aligned(16))) = {
    .p_005618b0 =
        {
            &sNAD,
            zsceneLoadAnimFile,
            &sIFF,
            zsceneLoadAnimFile,
            &sZAL,
            zsceneLoadAnimLibrary,
        },
    .p_005618d0 = &sSCENE,
};

struct BmxGroup_d_005618d4Group d_005618d4Group __attribute__((aligned(16))) = {
    .p_005618d8 = &sMODEL,
};

struct BmxGroup_gSceneModelHandlersGroup gSceneModelHandlersGroup __attribute__((aligned(16))) = {
    .gSceneModelHandlers = (void *)((char *)&p_005618a0Group.p_005618a0 + 0),
    .p_005618e0 = &sANIM,
};

struct BmxGroup_gSceneAnimHandlersGroup gSceneAnimHandlersGroup __attribute__((aligned(16))) = {
    .gSceneAnimHandlers = (void *)((char *)&p_005618b0Group.p_005618b0 + 0),
};

struct BmxGroup_d_00561900Group d_00561900Group __attribute__((aligned(16))) = {
    .d_00561900 = 0x42474154,
    .p_00561904 = ztaginfLoad,
};

struct BmxGroup_d_00561908Group d_00561908Group __attribute__((aligned(16))) = {
    .d_00561908 = 0x54504353,
    .p_0056190c = zscnptsReadChunk,
    .d_00561910 = 0x41504453,
    .p_00561914 = zpanoramaLoadChunk,
};

struct BmxGroup_d_00561918Group d_00561918Group __attribute__((aligned(16))) = {
    .d_00561918 = 0x4c424450,
    .p_0056191c = zpropLoadDef,
};

struct BmxGroup_d_00561920Group d_00561920Group __attribute__((aligned(16))) = {
    .d_00561920 = 0x4c425250,
    .p_00561924 = zpropLoad,
};

/* 0x00561928 */
char sMODEL[8] = "MODEL";

/* 0x00561930 */
char sSCENE[8] = "SCENE";

/* 0x00561938 */
char sZAL[4] = "ZAL";

/* 0x0056193c */
char sIFF[4] = "IFF";

/* 0x00561940 */
char sNAD[4] = "NAD";

/* 0x00561944 */
char sZMD[4] = "ZMD";

/* 0x00561948 */
char sERRORUnknownTokenS[32] = "**ERROR** Unknown token:'%s'\n";

/* 0x00561968 */
char sERRORMissingData[28] = "**ERROR**: Missing data\n";

/* 0x00561984 */
char s_00561984[4] = "}";

/* 0x00561988 */
char sSUBTYPE[8] = "SUBTYPE";

/* 0x00561990 */
char sTYPE[8] = "TYPE";

/* 0x00561998 */
char sFileNameIsTooLong[24] = "File name is too long.";

/* 0x005619b0 */
char sFILE[8] = "FILE";

/* 0x005619b8 */
char sObjectNameIsTooLong[28] = "Object name is too long.";

/* 0x005619d4 */
char sCNgEngineZgeZSceneC[28] = "C:\\NgEngine\\Zge\\zScene.c";

/* 0x005619f0 */
char sNAME[8] = "NAME";

/* 0x005619f8 */
char s_005619f8[4] = "{";

/* 0x005619fc */
char sDatabaseNotFoundS[24] = "Database not found:%s";

/* 0x00561a14 */
char sSCENECOMMON[16] = "SCENE_COMMON";

/* 0x00561a24 */
char sNoSceneIsOpen[20] = "No scene is open";

/* 0x00561a38 */
char sCouldnTOpenRamdiskFile[28] = "Couldn't open ramdisk file";

/* 0x00561a54 */
char sSceneComponentFileNotFoundS[36] = "Scene component file not found:%s";

/* 0x00561a78 */
char sZrwWorldFailedToLoadScenePoint[40] = "zrwWorld: Failed to load scene point\n";

/* 0x00561aa0 */
char sZrwWorldFailedToLoadSceneTaggingInfo[48] = "zrwWorld: Failed to load scene tagging info\n";

/* 0x00561ad0 */
char sUnknownSceneEzsceneObjTypeD[32] = "Unknown scene EzsceneObjType:%d";

/* 0x00561af0 */
char sNULLAnimLibID[16] = "NULL animLibID";

/* 0x00561b00 */
char sUncoded[8] = "Uncoded";

/* 0x00561b08 */
char sZSCENEWarningObjectWasNotInSceneTypeDId[72] =
    "ZSCENE: Warning object was not in scene:Type:%d, idType:%d, ZID:0x%x\n";

/* 0x00561b50 */
char sAnimLibNotFoundS[24] = "AnimLib not found:%s";

/* 0x00561b68 */
char sERRORZSCENELoadingDatabase[36] = "**ERROR** ZSCENE: Loading database\n";

/* 0x00561b8c */
char sSceneTxt[12] = "Scene.Txt";

/* 0x00561b98 */
char sErrorLoadingS[20] = "Error loading:%s";

/* 0x00561bac */
char sZSCENECouldNotLoadS[28] = "ZSCENE: Could not load %s";

/* 0x00561bc8 */
char sSceneNotFoundS[20] = "Scene not found:%s\n";

/* 0x00561bdc */
char sThereIsAlreadyASceneOpen[32] = "There is already a scene open";

/* 0x00561bfc */
char sFailedToCreateObjectForS[32] = "Failed to create object for:%s";

/* 0x00561c1c */
char sModelNotFoundS[20] = "Model not found:%s";

/* 0x00561c30 */
char sUnknownModelExtensionS[28] = "Unknown model extension:%s";

/* 0x00561c4c */
char sModelNotInSceneS[24] = "Model not in scene:%s";

/* 0x00561c64 */
char sAnimNotFoundS[20] = "Anim not found:%s";

/* 0x00561c78 */
char sUnknownExtension[20] = "Unknown extension";

/* 0x00561c8c */
char sAnimNotInSceneS[24] = "Anim not in scene:%s";

/* 0x00561ca4 */
char s_00561ca4[4] = "\n";

/* 0x00561ca8 */
char sS_00561ca8[4] = ":%s";

/* 0x00561cac */
char sWARNINGZSCENEMotion0xXNotFoundInAnimati[68] =
    "**WARNING** ZSCENE: Motion (0x%x) not found in animation library";

/* 0x00561cf0 */
char sTableNotBigEnoughTabSizeDNumMotionsD[48] = "Table not big enough (TabSize:%d, NumMotions:%d";

/* 0x00561d20 */
char sZscnptsPointDataSizeMismatch[36] = "zscnpts : Point data size mismatch";

/* 0x00561d44 */
char sZscnptsPointHasNoData[32] = "zscnpts : Point has no data!!!";

/* 0x00561d64 */
char sCNgEngineZgeZgeModsZscnptsC[36] = "C:\\NgEngine\\Zge\\ZgeMods\\zscnpts.c";

struct BmxGroup_gZpadKeyMapFirstGroup gZpadKeyMapFirstGroup __attribute__((aligned(16))) = {
    .gZpadKeyMapFirst = 203,
    .d_00561e69 = {4,  205, 5, 200, 6,  208, 7, 18, 8, 31, 9,  33, 10, 32, 11,
                   17, 14,  4, 16,  19, 15,  5, 17, 1, 12, 28, 13, 57, 20, 255},
};

struct BmxGroup_gZpadKeyMapOtherGroup gZpadKeyMapOtherGroup __attribute__((aligned(16))) = {
    .gZpadKeyMapOther = 36,
    .d_00561e89 = {4,  38, 5,  23, 6,  37, 7,  44, 8,  45, 9,  46, 10,
                   47, 11, 48, 14, 49, 16, 50, 15, 35, 17, 57, 20, 255},
};

struct BmxGroup_gZpadDefaultButtonMapGroup gZpadDefaultButtonMapGroup __attribute__((aligned(16))) = {
    .d_00561ea9 = {0,  0, 0,  0, 0,  0, 0,  3, 8,  0, 9,  2, 10, 1,
                   11, 4, 14, 6, 16, 5, 15, 7, 17, 8, 21, 9, 20, 255},
};

struct BmxGroup_p_00561ec8Group p_00561ec8Group __attribute__((aligned(16))) = {
    .p_00561ec8 =
        {
            &sGravisGrIP,
            &sGravisGamePadPro,
        },
};

struct BmxGroup_p_00561ed8Group p_00561ed8Group __attribute__((aligned(16))) = {
    .p_00561ed8 = &p_00561ec8Group.p_00561ec8,
    .d_00561edc = {0, 0x9000803, 0xb010a02, 0x10060e04, 0x11070f05, 0x14091508, 255},
};

struct BmxGroup_p_00561ef8Group p_00561ef8Group __attribute__((aligned(16))) = {
    .p_00561ef8 =
        {
            &sMicrosoftSideWinderGamePad,
            &sMicrosoftSideWinderAutoDetect,
        },
};

struct BmxGroup_p_00561f04Group p_00561f04Group __attribute__((aligned(16))) = {
    .p_00561f04 = (void *)&d_0054e0f0,
};

struct BmxGroup_p_00561f10Group p_00561f10Group __attribute__((aligned(16))) = {
    .p_00561f10 =
        {
            &p_00561ef8Group.p_00561ef8,
            &p_00561f04Group.p_00561f04,
        },
    .d_00561f18 = {0x9030804, 0xb000a01, 0xf070e06, 0x15091408, 255},
};

struct BmxGroup_gZpadProfilesGroup gZpadProfilesGroup __attribute__((aligned(16))) = {
    .gZpadProfiles = (void *)((char *)&p_00561ed8Group.p_00561ed8 + 0),
    .p_00561f30 = &p_00561f10Group.p_00561f10,
};

/* 0x00561f38 */
char sMicrosoftSideWinderAutoDetect[36] = "Microsoft SideWinder (Auto Detect)";

/* 0x00561f5c */
char sMicrosoftSideWinderGamePad[32] = "Microsoft SideWinder game pad";

/* 0x00561f7c */
char sGravisGamePadPro[20] = "Gravis GamePad Pro";

/* 0x00561f90 */
char sGravisGrIP[12] = "Gravis GrIP";

/* 0x00561f9c */
char sDirectInputCreateFailed[28] = "DirectInputCreate() failed.";

/* 0x00561fb8 */
char sCNgEngineZgePcPcinputC[32] = "C:\\NgEngine\\Zge\\Pc\\pcinput.c";

/* 0x00561fd8 */
char sPCINPUTCouldNotCreateController[40] = "PCINPUT: Could not create controller\n";

/* 0x00562000 */
char sCouldNotCreateAnInputDevice[36] = "Could not create an input device";

/* 0x00562024 */
char sSetCooperativeLevelFailed[32] = "SetCooperativeLevel() failed";

/* 0x00562044 */
char sSetDataFormatFailed[28] = "SetDataFormat() failed";

/* 0x00563428 */
ZrwLightCreateFn gZrwLightCreate = (ZrwLightCreateFn)zrwLightCreate;

/* 0x0056342c */
ZrwLightDestroyFn gZrwLightDestroy = (ZrwLightDestroyFn)zrwLightDestroy;

/* 0x00563430 */
ZrwLightColorFn gZrwLightColor = (ZrwLightColorFn)zrwLightSetColor;

/* 0x00563434 */
ZrwLightFlagsFn gZrwLightFlags = (ZrwLightFlagsFn)zrwLightSetFlags;

/* 0x00563438 */
ZrwLightParamFn gZrwLightSetParam = (ZrwLightParamFn)zrwLightSetPosition;

/* 0x0056343c */
ZrwLightDirectionFn gZrwLightDirection = (ZrwLightDirectionFn)zrwLightSetDirection;

/* 0x00563440 */
void *p_00563440 = zrwLightSetConeAngle;

/* 0x00563444 */
void *p_00563444 = zrwLightSetRadius;

/* 0x00563448 */
char sCNgEngineZgeComZglRwZrwLightC[40] = "C:\\NgEngine\\Zge\\Com\\ZglRw\\zrwLight.c";

/* 0x00563470 */
char sCNgEngineZgeZgeC[24] = "C:\\NgEngine\\Zge\\zge.c";

/* 0x00563488 */
char sZGLDUMP[12] = "ZGL DUMP\n";

/* 0x00563494 */
char sZGEDUMP[12] = "ZGE DUMP\n";

/* 0x005634a0 */
char s_005634a0[60] = "-------------------------------------------------------\n";

/* 0x005634dc */
char sMovieInit[16] = "Movie : Init\n";

/* 0x005634ec */
char sMovieCloseAudio[24] = "Movie : Close Audio\n";

/* 0x00563504 */
char sMovieShutdown[20] = "Movie : Shutdown\n";

/* 0x00563518 */
char sMovieFillAudio[20] = "Movie : Fill Audio\n";

/* 0x0056352c */
char sMovieClose[16] = "Movie : Close\n";

/* 0x0056353c */
char sMovieEndOfMovie[24] = "Movie : End Of Movie\n";

/* 0x00563554 */
char sPanorama[12] = "Panorama";

/* 0x00563560 */
char sOpenedFileS[28] = "\n*** Opened File %s ***\n\n";

/* 0x0056357c */
char sClosedFile[24] = "\n*** Closed File ***\n\n";

/* 0x00563594 */
char sSymbolNotFoundS[24] = "Symbol not found: '%s'\n";

/* 0x005635ac */
char sD_005635ac[4] = "%d";

/* 0x005635b0 */
char sX[4] = "%X";

/* 0x005635b4 */
char s_005635b4[4] = "$";

/* 0x005635b8 */
char s0x[4] = "0x";

/* 0x005635bc */
char s0X[4] = "0X";

/* 0x005635c0 */
char sFileSLineD[20] = "File:%s,  Line:%d\n";

/* 0x005635d4 */
char sIncludedFrom[16] = "Included from:";

/* 0x005635e4 */
char sCanTOpenIncludeFileS[32] = "Can't open include file: %s\n";

/* 0x00563604 */
char sERRORMaximumIncludeNestingReached[44] = "**ERROR** Maximum include nesting reached!\n";

/* 0x00563630 */
char sDEFINE[8] = "#DEFINE";

/* 0x00563638 */
char sEndif[8] = "#endif";

/* 0x00563640 */
char sIfndef[8] = "#ifndef";

/* 0x00563648 */
char sINCLUDEPATH[16] = "#INCLUDE_PATH";

/* 0x00563658 */
char sINCLUDE[12] = "#INCLUDE";

/* 0x00563664 */
char s_00563664[4] = ";";

/* 0x00563668 */
char s_00563668[4] = "\t";

/* 0x0056366c */
char s_0056366c[4] = "*/";

/* 0x00563670 */
char s_00563670[4] = "/*";

/* 0x00563674 */
char s_00563674[8] = "////";

/* 0x0056367c */
char sOutOfSymbolSpace[24] = "Out of symbol space\n";

/* 0x00563694 */
char sReplacingSymbolS[24] = "Replacing symbol:%s\n";

/* 0x005636ac */
char sF_005636ac[4] = "%f";

/* 0x005636b0 */
char sZTOKENAssertFailedFoundSExpectingS[52] = "ZTOKEN assert failed : found '%s', expecting '%s'\n";

/* 0x005636e4 */
char sSetCapitalizeFALSE[32] = "\n*** Set Capitalize FALSE ***\n\n";

/* 0x00563704 */
char sSetCapitalizeTRUE[32] = "\n*** Set Capitalize TRUE ***\n\n";

/* 0x00563724 */
char sCNgEngineZgeZgeModsZFontC[32] = "C:\\NgEngine\\Zge\\ZgeMods\\zFont.c";

/* 0x00563744 */
char sBadFontGlyphNumber[24] = "Bad font glyph Number";

/* 0x0056375c */
char sBadFontVersion[20] = "Bad font version";

/* 0x00563904 */
BmxType_gZProfileColors gZProfileColors = {0x80808080, 0x80ffffff, 0x800000ff, 0x8000ff00, 0x80ff0000};

/* 0x00563918 */
uint32_t d_00563918 = 0x8000ff00;

/* 0x0056391c */
uint32_t d_0056391c = 0x800000ff;

/* 0x00563920 */
char sCNgEngineZgeZprofileC[28] = "C:\\NgEngine\\Zge\\zprofile.c";

/* 0x0056393c */
char sInvalidParameterToZprofileSetLayout[44] = "Invalid parameter to zprofileSetLayout().";

/* 0x00563968 */
char sFailedToAllocateObj[24] = "Failed to allocate obj\n";

/* 0x00563980 */
char sUnknownBarType[20] = "Unknown bar type";

/* 0x00563994 */
char sApp[4] = "App";

/* 0x00563998 */
char sEngCPU[8] = "EngCPU";

/* 0x005639a0 */
char sZTIMERAssertFailedUnableToRegisterUpdat[60] =
    "ZTIMER assert failed : unable to register update function\n";

/* 0x005639dc */
char sEventTypeTimerForObjD[32] = "Event type timer for obj %d\n";

/* 0x005639fc */
char sScriptTypeTimer[20] = "Script type timer\n";

/* 0x00563a10 */
char sPROFILENoHardwareCounterAvailable[40] = "PROFILE: No hardware counter available\n";

/* 0x00563a38 */
char sCNgEngineZgePcPcProfileC[40] = {67,  58,  92, 78, 103, 69, 110, 103,  105, 110, 101, 92,  90,
                                      103, 101, 92, 80, 99,  92, 112, 99,   80,  114, 111, 102, 105,
                                      108, 101, 46, 99, 0,   0,  117, -104, 0,   0,   115, -104};

struct BmxGroup_p_00563a60Group p_00563a60Group __attribute__((aligned(16))) = {
    .p_00563a60 =
        {
            crtFpInit,
            unserved_004a4a37,
            unserved_004a4a37,
        },
};

/* 0x00563a70 */
char sAssertionFailedSFileSLineD[40] = "Assertion failed: %s, file %s, line %d\n";

/* 0x00563a98 */
void *p_00563a98 = (void *)&s_0054e160;

/* 0x00563a9c */
void *p_00563a9c = &s_00561ca4;

struct BmxGroup_p_00563aa0Group p_00563aa0Group __attribute__((aligned(16))) = {
    .p_00563aa0 = (void *)&s_0054e15c,
};

/* 0x00563ab0 */
void *p_00563ab0 = crtExit;

/* 0x00563ab4 */
uint32_t d_00563ab4[3] = {2};

/* 0x00563ac0 */
char sAsin_00563ac0[8] = "asin";

/* 0x00563ac8 */
uint32_t d_00563ac8 = 5951;

/* 0x00563acc */
uint32_t d_00563acc = 6975;

struct BmxGroup_gCrtFloatHooksGroup gCrtFloatHooksGroup __attribute__((aligned(16))) = {
    .gCrtFloatHooks =
        {
            unserved_004ad055,
            unserved_004ad055,
            unserved_004ad055,
            unserved_004ad055,
            unserved_004ad055,
            unserved_004ad055,
        },
    .d_00563ae8 = {16},
};

/* 0x00563af0 */
void *p_00563af0 = &p_00563af0;

/* 0x00563af4 */
void *p_00563af4[3] = {
    &p_00563af0,
    &d_00563b08,
    &d_00563b08,
};

/* 0x00563b00 */
uint32_t d_00563b00[2] = {0xffffffff, 0xffffffff};

/* 0x00563b08 */
uint32_t d_00563b08[2050] = {240, 241};

/* 0x00565b10 */
void *p_00565b10 = &p_00563af0;

/* 0x00565b14 */
uint32_t d_00565b14 = 480;

struct BmxGroup_p_00565b18Group p_00565b18Group __attribute__((aligned(16))) = {
    .p_00565b18 = &d_00697f20,
    .p_00565b20 = &d_00697f20,
    .d_00565b24 = 257,
};

/* 0x00565b28 */
uint32_t d_00565b28[4] = {0, 0, 4096};

/* 0x00565b38 */
uint32_t d_00565b38[8] = {0, 0, 0, 2, 1};

/* 0x00565b58 */
unsigned char d_00565b58[12];

/* 0x00565b64 */
uint32_t d_00565b64[9] = {2, 2};

/* 0x00565b88 */
unsigned char d_00565b88[528];

/* 0x00565d98 */
unsigned char d_00565d98[8];

/* 0x00565da0 */
void *p_00565da0 = (void *)&sNull;

/* 0x00565da4 */
void *p_00565da4 = (void *)&s_0054e3b8;

/* 0x00565da8 */
uint32_t d_00565da8 = 0xc0000005;

/* 0x00565dac */
uint32_t d_00565dac = 11;

/* 0x00565db0 */
uint32_t d_00565db0[28] = {0, 0xc000001d, 4, 0, 0xc0000096, 4, 0, 0xc000008d, 8,
                           0, 0xc000008e, 8, 0, 0xc000008f, 8, 0, 0xc0000090, 8,
                           0, 0xc0000091, 8, 0, 0xc0000092, 8, 0, 0xc0000093, 8};

/* 0x00565e20 */
uint32_t d_00565e20 = 3;

/* 0x00565e24 */
uint32_t d_00565e24 = 7;

/* 0x00565e28 */
char s_00565e28[4] = "\n";

/* 0x00565e2c */
uint32_t d_00565e2c = 140;

/* 0x00565e30 */
uint32_t d_00565e30[2] = {0xffffffff, 2560};

/* 0x00565e38 */
uint32_t d_00565e38[4] = {0x19930520};

/* 0x00565e48 */
uint32_t d_00565e48 = 2;

struct BmxGroup_p_00565e4cGroup p_00565e4cGroup __attribute__((aligned(16))) = {
    .p_00565e4c = (void *)&sR6002FloatingPointNotLoaded,
    .d_00565e50 = 8,
    .p_00565e54 = (void *)&sR6008NotEnoughSpaceForArguments,
    .d_00565e58 = 9,
    .p_00565e5c = (void *)&sR6009NotEnoughSpaceForEnvironment,
    .d_00565e60 = 10,
    .p_00565e64 = (void *)&sAbnormalProgramTermination,
    .d_00565e68 = 16,
    .p_00565e6c = (void *)&sR6016NotEnoughSpaceForThreadData,
    .d_00565e70 = 17,
    .p_00565e74 = (void *)&sR6017UnexpectedMultithreadLockError,
    .d_00565e78 = 18,
    .p_00565e7c = (void *)&sR6018UnexpectedHeapError,
    .d_00565e80 = 19,
    .p_00565e84 = (void *)&sR6019UnableToOpenConsoleDevice,
    .d_00565e88 = 24,
    .p_00565e8c = (void *)&sR6024NotEnoughSpaceForOnexitAtexitTable,
    .d_00565e90 = 25,
    .p_00565e94 = (void *)&sR6025PureVirtualFunctionCall,
    .d_00565e98 = 26,
    .p_00565e9c = (void *)&sR6026NotEnoughSpaceForStdioInitializati,
    .d_00565ea0 = 27,
    .p_00565ea4 = (void *)&sR6027NotEnoughSpaceForLowioInitializati,
    .d_00565ea8 = 28,
    .p_00565eac = (void *)&sR6028UnableToInitializeHeap,
    .d_00565eb0 = 120,
    .p_00565eb4 = (void *)&sDOMAINError,
    .d_00565eb8 = 121,
    .p_00565ebc = (void *)&sSINGError,
    .d_00565ec0 = 122,
    .p_00565ec4 = (void *)&sTLOSSError,
    .d_00565ec8 = 252,
    .p_00565ecc = (void *)&s_0054e3e0,
    .d_00565ed0 = 255,
    .p_00565ed4 = (void *)&sRuntimeError,
};

/* 0x00565ed8 */
unsigned char d_00565ed8[8];

/* 0x00565ee0 */
unsigned char d_00565ee0[10] = {0, 0, 0, 0, 0, 0, 0, 192, 255, 255};

/* 0x00565eea */
unsigned char d_00565eea[10] = {53, 194, 104, 33, 162, 218, 15, 201, 255, 63};

/* 0x00565ef4 */
unsigned char d_00565ef4[9] = {0, 0, 0, 0, 0, 0, 240, 63, 1};

/* 0x00565efd */
unsigned char d_00565efd[19] = {8, 4, 8, 8, 8, 4, 8, 8, 0, 4, 12, 8, 0, 4, 12, 8};

/* 0x00565f10 */
uint32_t d_00565f10 = 20;

struct BmxGroup_p_00565f14Group p_00565f14Group __attribute__((aligned(16))) = {
    .p_00565f14 = (void *)&sExp,
    .d_00565f18 = 29,
    .p_00565f1c = (void *)&sPow,
    .d_00565f20 = 26,
    .p_00565f24 = (void *)&sLog,
    .d_00565f28 = 27,
    .p_00565f2c = (void *)&sLog10,
    .d_00565f30 = 31,
    .p_00565f34 = (void *)&sSinh,
    .d_00565f38 = 19,
    .p_00565f3c = (void *)&sCosh,
    .d_00565f40 = 33,
    .p_00565f44 = (void *)&sTanh,
    .d_00565f48 = 14,
    .p_00565f4c = (void *)&sAsin,
    .d_00565f50 = 13,
    .p_00565f54 = (void *)&sAcos,
    .d_00565f58 = 15,
    .p_00565f5c = (void *)&sAtan,
    .d_00565f60 = 16,
    .p_00565f64 = (void *)&sAtan2,
    .d_00565f68 = 5,
    .p_00565f6c = (void *)&sSqrt,
    .d_00565f70 = 30,
    .p_00565f74 = (void *)&sSin,
    .d_00565f78 = 18,
    .p_00565f7c = (void *)&sCos,
    .d_00565f80 = 32,
    .p_00565f84 = (void *)&sTan,
    .d_00565f88 = 12,
    .p_00565f8c = (void *)&sCeil,
    .d_00565f90 = 11,
    .p_00565f94 = (void *)&sFloor,
    .d_00565f98 = 21,
    .p_00565f9c = (void *)&sFabs,
    .d_00565fa0 = 28,
    .p_00565fa4 = (void *)&sModf,
    .d_00565fa8 = 25,
    .p_00565fac = (void *)&sLdexp,
    .d_00565fb0 = 17,
    .p_00565fb4 = (void *)&sCabs,
    .d_00565fb8 = 24,
    .p_00565fbc = (void *)&sHypot,
    .d_00565fc0 = 22,
    .p_00565fc4 = (void *)&sFmod,
    .d_00565fc8 = 23,
    .p_00565fcc = (void *)&sFrexp,
    .d_00565fd0 = 34,
    .p_00565fd4 = (void *)&sY0,
    .d_00565fd8 = 35,
    .p_00565fdc = (void *)&sY1,
    .d_00565fe0 = 36,
    .p_00565fe4 = (void *)&sYn,
};

/* 0x00565fe8 */
uint32_t d_00565fe8[2] = {0, 0x7ff00000};

/* 0x00565ff0 */
uint32_t d_00565ff0[2] = {0, 0xfff80000};

/* 0x00565ff8 */
uint32_t d_00565ff8[4] = {0xffffffff, 0x7fefffff, 0, 0x100000};

/* 0x00566008 */
uint32_t d_00566008[2] = {0, 0x80000000};

/* 0x00566010 */
uint32_t d_00566010[3] = {0, 0x80000000, 17424};

/* 0x0056601c */
uint32_t d_0056601c[3] = {1, 0x80000000, 12288};

struct BmxGroup_p_00566028Group p_00566028Group __attribute__((aligned(16))) = {
    .p_00566028 =
        {
            &s_00566032,
            &s_00566032,
        },
};

/* 0x00566032 */
char s_00566032[514] = {
    32,   0, 32,   0, 32,   0, 32,   0, 32,   0, 32,   0, 32,   0, 32,   0, 32,   0, 40,   0, 40,   0, 40, 0,
    40,   0, 40,   0, 32,   0, 32,   0, 32,   0, 32,   0, 32,   0, 32,   0, 32,   0, 32,   0, 32,   0, 32, 0,
    32,   0, 32,   0, 32,   0, 32,   0, 32,   0, 32,   0, 32,   0, 32,   0, 72,   0, 16,   0, 16,   0, 16, 0,
    16,   0, 16,   0, 16,   0, 16,   0, 16,   0, 16,   0, 16,   0, 16,   0, 16,   0, 16,   0, 16,   0, 16, 0,
    -124, 0, -124, 0, -124, 0, -124, 0, -124, 0, -124, 0, -124, 0, -124, 0, -124, 0, -124, 0, 16,   0, 16, 0,
    16,   0, 16,   0, 16,   0, 16,   0, 16,   0, -127, 0, -127, 0, -127, 0, -127, 0, -127, 0, -127, 0, 1,  0,
    1,    0, 1,    0, 1,    0, 1,    0, 1,    0, 1,    0, 1,    0, 1,    0, 1,    0, 1,    0, 1,    0, 1,  0,
    1,    0, 1,    0, 1,    0, 1,    0, 1,    0, 1,    0, 1,    0, 16,   0, 16,   0, 16,   0, 16,   0, 16, 0,
    16,   0, -126, 0, -126, 0, -126, 0, -126, 0, -126, 0, -126, 0, 2,    0, 2,    0, 2,    0, 2,    0, 2,  0,
    2,    0, 2,    0, 2,    0, 2,    0, 2,    0, 2,    0, 2,    0, 2,    0, 2,    0, 2,    0, 2,    0, 2,  0,
    2,    0, 2,    0, 2,    0, 16,   0, 16,   0, 16,   0, 16,   0, 32};

/* 0x00566234 */
uint32_t d_00566234 = 1;

/* 0x00566238 */
char s_00566238[8] = {46, 0, 0, 0, 1};

/* 0x00566240 */
char s_00566240[8] = " \t-\r]";

/* 0x00566248 */
char s_00566248[8] = "]";

/* 0x00566250 */
uint32_t d_00566250[6] = {1024, 0xfffffc01, 53, 11, 64, 1023};

/* 0x00566268 */
uint32_t d_00566268[6] = {128, 0xffffff81, 24, 8, 32, 127};

/* 0x00566280 */
uint32_t d_00566280[2] = {0x8040201};

/* 0x00566288 */
uint32_t d_00566288 = 932;

/* 0x0056628c */
uint32_t d_0056628c[3] = {0x82798260, 33};

/* 0x00566298 */
uint32_t d_00566298[56] = {
    [0] = 57254,       [2] = 42401,       [4] = 0xfce09f81,  [6] = 0xfc807e40, [8] = 936,
    [9] = 0xa3daa3c1,  [10] = 32,         [16] = 65153,      [18] = 65088,     [20] = 949,
    [21] = 0xa3daa3c1, [22] = 32,         [28] = 65153,      [30] = 65089,     [32] = 950,
    [33] = 0xa2e4a2cf, [34] = 0xa2e5001a, [35] = 0x5ba2e8,   [40] = 65153,     [42] = 0xfea17e40,
    [44] = 1361,       [45] = 0xda5eda51, [46] = 0xda5f0020, [47] = 0x32da6a,  [52] = 0xded8d381,
    [53] = 63968,      [54] = 0xfe817e31};

/* 0x00566378 */
unsigned char d_00566378[8];

/* 0x00566380 */
uint32_t d_00566380[2] = {9876};

/* 0x00566388 */
uint32_t d_00566388 = 1;

/* 0x0056638c */
uint32_t d_0056638c[89] = {22, 2,   2,  3,   2,  4,   24, 5,   13, 6,   9,  7,   12, 8,   12, 9,    12, 10,
                           7,  11,  8,  12,  22, 13,  22, 15,  2,  16,  13, 17,  18, 18,  2,  33,   13, 53,
                           2,  65,  13, 67,  2,  80,  17, 82,  13, 83,  13, 87,  22, 89,  11, 108,  13, 109,
                           32, 112, 28, 114, 9,  6,   22, 128, 10, 129, 10, 130, 9,  131, 22, 132,  13, 145,
                           41, 158, 13, 161, 2,  164, 11, 167, 13, 183, 17, 206, 2,  215, 11, 1816, 12};

/* 0x005664f0 */
uint32_t d_005664f0[88] = {
    0,          0,          0x4002a000, 0,          0,          0x4005c800, 0,          0,
    0x4008fa00, 0,          0,          0x400c9c40, 0,          0,          0x400fc350, 0,
    0,          0x4012f424, 0,          0x80000000, 0x40169896, 0,          0x20000000, 0x4019bebc,
    0,          0xc9bf0400, 0x40348e1b, 0xa1000000, 0x1bcecced, 0x404ed3c2, 0xb59ef020, 0xada82b70,
    0x40699dc5, 0x25fd5dd0, 0x4f8e1ae5, 0x4083eb19, 0x95d79671, 0x8d050e43, 0x409eaf29, 0x44a0bff9,
    0x8f1281ed, 0x40b98281, 0xa6d53cbf, 0x1f49ffcf, 0x40d3c278, 0x8ce0c66f, 0x47c980e9, 0x41a893ba,
    0x556b85bc, 0xf78d3927, 0x427ce070, 0xde8eddbc, 0xebfb9df9, 0x4351aa7e, 0xe376e6a1, 0x2f29f2cc,
    0x44268184, 0xaa171028, 0xe310aef8, 0x44fac4c5, 0xf3d4a7eb, 0x4ae1ebf7, 0x45cf957a, 0x91c7cc65,
    0xa0aea60e, 0x46a3e319, 0xc17650d,  0x75868175, 0x4d48c976, 0xa7e44258, 0x353b3993, 0x53edb2b8,
    0x5de5a74d, 0x3b5dc53d, 0x5a929e8b, 0xf0a65dff, 0x54c020a1, 0x61378ca5, 0x5a8bfdd1, 0x5d25d88b,
    0x67dbf989, 0xf3f895aa, 0xc8a2bf27, 0x6e80dd5d, 0x979bc94c, 0x52028a20, 0x7525c460};

/* 0x00566650 */
uint32_t d_00566650[88] = {
    0xcccdcccd, 0xcccccccc, 0x3ffbcccc, 0xd70a3d71, 0xa3d70a3,  0x3ff8a3d7, 0xdf3b645a, 0x6e978d4f,
    0x3ff58312, 0x652cd3c3, 0x1758e219, 0x3ff1d1b7, 0x84230fd0, 0xac471b47, 0x3feea7c5, 0x69b6a640,
    0xbd05af6c, 0x3feb8637, 0x42bc3d33, 0x94d5e57a, 0x3fe7d6bf, 0xcefdfdc2, 0x77118461, 0x3fe4abcc,
    0xe15b4c2f, 0x94bec44d, 0x3fc9e695, 0x3b53c492, 0x14cd4475, 0x3faf9abe, 0x94ba67de, 0x1ead4539,
    0x3f94cfb1, 0xe2c62324, 0x313bbabc, 0x3f7a8b61, 0xc1595561, 0x7c53b17e, 0x3f5fbb12, 0x8d2feed7,
    0x8592be06, 0x3f44fb15, 0xe9a53f24, 0xea27a539, 0x3f2aa87f, 0xe4a1ac7d, 0x467c64bc, 0x3e55ddd0,
    0xcc067b63, 0x83775423, 0x3d8191ff, 0x193afa91, 0x4325637a, 0x3cacc031, 0x38d18921, 0xb8974782,
    0x3bd7fd00, 0x85888dc,  0xe3e8b11b, 0x3b03a686, 0x424584c6, 0x7599b607, 0x3a2edb37, 0xd21c7133,
    0xee32db23, 0x395a9049, 0xc0be87a6, 0x82a5da57, 0x32b5a2a6, 0x11b268e2, 0x449f52a7, 0x2c10b759,
    0x2de44925, 0x534f3436, 0x256bceae, 0xa404598f, 0x7dc2dec0, 0x1ec6e8fb, 0x5a88e79e, 0xbf3c9157,
    0x18228350, 0x62654b4e, 0xaf8f83fd, 0x117d9406, 0x9fde2de4, 0x4c8d2ce,  0xad8a6dd};

/* 0x00650170 */
int gZdebugCreated;

/* 0x00650174 */
unsigned char d_00650174[4];

/* 0x00650178 */
unsigned char d_00650178[8];

/* 0x0065e29c, also gDebugIndent */
int gZdebugIndentLevel;

/* 0x0065e2a8 */
int gZinputReady;

/* 0x0065e2ac */
ZMemFreeList *gZpadDevices;

/* 0x0065e2b0 */
ZMemFreeList *gZassetLibraries;

/* 0x0065e2bc */
ZMathTables gZmathTables;

struct BmxGroup_gZmathOriginGroup gZmathOriginGroup __attribute__((aligned(16)));

/* 0x006702f8 */
int gZobjCameraInitialized;

/* 0x006702fc */
ZMemFreeList *gZobjCameraCaches;

/* 0x00670300 */
ZVec3 gZobjCameraTarget;

/* 0x00670310 */
unsigned char d_00670310[200];

/* 0x006703d8 */
ZstrTempBuffer gZstrTempBuffer;

/* 0x006704d8 */
ZStringSetTable gZstringSets;

/* 0x00670518 */
int gNameTablesReady;

/* 0x0067051c */
ZNameTables gNameTables;

/* 0x00670528 */
ZfileErrorBuffer gZfileErrorMessage;

/* 0x00670568 */
ZfileDevice *gZfileDefaultDevice;

/* 0x0067056c */
unsigned char d_0067056c[64];

/* 0x006705ac */
ZfileNameBuffer gZfileNameBuffer;

/* 0x00670630 */
ZfileDeviceTable gZfileDevices;

/* 0x00670674 */
int gZfileInitialized;

/* 0x00670678 */
ZMemFreeList *gZfileHandles;

/* 0x0067067c */
ZMemPool *gZfileLoadPool;

/* 0x00670680 */
ZMemInUseTable gZmemFreeListInUse;

/* 0x00670880 */
ZMemState gZmem;

/* 0x006708a8 */
ZMemFreeListTable gZmemFreeLists;

/* 0x006718a8 */
ZMemHooks gZmemHooks;

/* 0x006718b0 */
unsigned char d_006718b0[16];

/* 0x006718c0 */
unsigned char d_006718c0[2544];

/* 0x006722b0 */
int gZmemHooksInstalled;

/* 0x006722b8, also gDeferredB, gZobjTreeObjects */
ZrwList gZpropDefs;

/* 0x006722c4, also gDeferredC, gZobjProps */
ZrwList gZpropInsts;

/* 0x006722d0 */
int gZobjInitialized;

/* 0x006722d8 */
ZMemFreeList *gZobjLinkPool;

/* 0x006722dc */
ZrwUpdateNode *gZobjDebugNode;

/* 0x006722e0 */
ZrwUpdateNode *gZobjUpdateNode;

/* 0x00672308 */
int gZobjModelReady;

/* 0x006728b8 */
uint32_t gZtaginfUnused;

/* 0x006728bc */
uint32_t gZtaginfFaceCount;

/* 0x006728c0 */
struct ZbspLevelFace *gZtaginfFaces;

struct BmxGroup_gZtaginfRootGroup gZtaginfRootGroup __attribute__((aligned(16)));

/* 0x006737b8 */
ZMemFreeList *gZtaginfNodes;

/* 0x006737c0 */
unsigned char d_006737c0[4];

/* 0x006737c4 */
unsigned char d_006737c4[1724];

/* 0x00673f68 */
int gZobjLightInitialized;

/* 0x00673f70 */
ZsceneState gScene;

/* 0x00674000, also gZrwMarkers */
ZscnptsState gScnpts;

/* 0x00674028 */
int gZpadDiActive;

/* 0x0067402c */
ZMemFreeList *gZpadDiRecords;

/* 0x00674030 */
uint16_t gZpadKeyboardOk;

/* 0x00674034 */
void *gZpadWindow;

/* 0x00674038 */
ZpadDirectInput *gZpadDirectInput;

/* 0x0067403c */
ZpadGuid gZpadJoystickGuid;

/* 0x0067404c */
ZpadDiDevice *gZpadKeyboard;

/* 0x00674050 */
uint32_t gZpadKeyboardFlags;

/* 0x00674054 */
ZpadKeyState gZpadKeyState;

/* 0x006741b4 */
int gZobjPEmitInitialized;

/* 0x006746c0 */
int gZanimInitialized;

/* 0x006746c4 */
ZSkeletonItem *gZiffItemPool;

/* 0x006746c8 */
void *gZiffAnim;

/* 0x006746d0 */
ZiffState gZiff;

/* 0x006752ec */
ZramdiskNameBuffer gZramdiskName;

/* 0x0067536c */
ZMemFreeList *gZramdiskFiles;

/* 0x00675378 */
int gZgeInitialized;

/* 0x0067537c */
void *gPanorama;

/* 0x00675380 */
ZtokenState gToken;

/* 0x006763d0 */
ZFontState gFont;

/* 0x006763e0 */
unsigned char d_006763e0[4];

/* 0x006763e4 */
unsigned char d_006763e4[4];

/* 0x006763ec */
ZrwRamDiskInfoSlots gZrwRamDiskInfoSlots;

/* 0x00676410 */
ZrwFileInterface gZrwSavedFileInterface;

/* 0x00676438 */
ZProfileState gZProfile;

/* 0x006764a0 */
uint16_t gZtimerSlots;

/* 0x006764a4 */
ZtimerHeap gZtimerHeap;

/* 0x006764b4 */
ZrwList gZtimerFree;

/* 0x006764c0 */
ZrwUpdateNode *gZtimerUpdate;

/* 0x006764c4 */
uint32_t gZtimerReady;

/* 0x006764c8 */
unsigned char d_006764c8[8];

/* 0x006764d0 */
PcProfileClock gPcProfileClock;

/* 0x006764e0 */
ZtimerHeapEntry gZtimerPopped;

/* 0x006764e8 */
unsigned char d_006764e8[4];

/* 0x006764ec */
uint32_t gCrtFpStatus;

/* 0x006764f0 */
unsigned char d_006764f0[8];

/* 0x006764f8 */
unsigned char d_006764f8[4];

/* 0x006764fc */
unsigned char d_006764fc[4];

/* 0x00676500 */
unsigned char d_00676500[4];

/* 0x00676504 */
unsigned char d_00676504[4];

/* 0x00676508 */
unsigned char d_00676508[4];

/* 0x0067650c */
unsigned char d_0067650c[4];

/* 0x00676510 */
unsigned char d_00676510[4];

/* 0x00676514 */
unsigned char d_00676514[4];

/* 0x00676518 */
unsigned char d_00676518[4];

/* 0x0067651c */
unsigned char d_0067651c[8];

/* 0x00676524 */
unsigned char d_00676524[4];

/* 0x00676528 */
unsigned char d_00676528[4];

/* 0x0067652c */
unsigned char d_0067652c[8];

/* 0x00676534 */
unsigned char d_00676534[8];

/* 0x0067653c */
unsigned char d_0067653c[4];

/* 0x00676540 */
unsigned char d_00676540[4];

/* 0x00676544 */
unsigned char d_00676544[4];

/* 0x00676548 */
unsigned char d_00676548[4];

/* 0x0067654c */
unsigned char d_0067654c[4];

/* 0x00676550 */
unsigned char d_00676550[4];

/* 0x00676554 */
unsigned char d_00676554[4];

/* 0x00676558 */
unsigned char d_00676558[4];

/* 0x0067655c */
unsigned char d_0067655c[4];

/* 0x00676560 */
unsigned char d_00676560[4];

/* 0x00676564 */
unsigned char d_00676564[4];

/* 0x00676568 */
unsigned char d_00676568[4];

/* 0x0067656c */
unsigned char d_0067656c[4];

/* 0x00676570 */
unsigned char d_00676570[4];

/* 0x00676574 */
unsigned char d_00676574[4];

/* 0x00676578 */
unsigned char d_00676578[4];

/* 0x0067657c */
unsigned char d_0067657c[4];

/* 0x00676580 */
unsigned char d_00676580[4];

/* 0x00676584 */
unsigned char d_00676584[4];

/* 0x00676588 */
unsigned char d_00676588[8];

/* 0x00676590 */
unsigned char d_00676590[4];

/* 0x00676594 */
unsigned char d_00676594[260];

/* 0x00676698 */
unsigned char d_00676698[4];

/* 0x0067669c */
unsigned char d_0067669c[4];

/* 0x006766a0 */
unsigned char d_006766a0[4];

/* 0x006766a4 */
unsigned char d_006766a4[8];

/* 0x006766ac */
unsigned char d_006766ac[16];

/* 0x006766bc */
unsigned char d_006766bc[12];

/* 0x006766c8 */
unsigned char d_006766c8[2];

/* 0x006766ca */
unsigned char d_006766ca[2];

/* 0x006766cc */
unsigned char d_006766cc[28];

/* 0x006766e8 */
unsigned char d_006766e8[4];

/* 0x006766ec */
unsigned char d_006766ec[4];

/* 0x006766f0 */
unsigned char d_006766f0[4];

/* 0x006766f4 */
unsigned char d_006766f4[4];

/* 0x006766f8 */
unsigned char d_006766f8[4];

/* 0x006766fc */
unsigned char d_006766fc[4];

/* 0x00676700 */
unsigned char d_00676700[4];

/* 0x00676704 */
unsigned char d_00676704[12];
