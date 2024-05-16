// clang-format off
#define LOCAL_DEBUG
#include "common/prolog.h"
// clang-format on

#include "display_layout.h"

// Mapping of clock pixels to the internal wiring.
// Values are in the format of 0xXY where X is the column ("digit" in the MAX7219 terminology) and Y is the row ("segment").
// For example the 0x21 means pin #2 on the "digits" connector and pin #1 on the "segments" connector.
// clang-format off
int map_visible_to_internal_cords[CLOCK_ROWS][CLOCK_COLUMNS] = {
//      0     1     2     3     4     5     6     7     8     9    10    11    12    13    14    15    16    17    18    19
    {0x00, 0x11, 0x21, 0x31, 0x10, 0x40, 0x50, 0x60, 0x71, 0x81, 0x91, 0x70, 0xa0,   -1, 0xb0, 0xc1, 0xd1, 0xe1, 0xc0, 0xf0}, // 0
    {0x02, -1,   0x22, 0x32,   -1, 0x42, 0x52, 0x62,   -1, 0x82, 0x92,   -1, 0xa2, 0x51, 0xb2,   -1, 0xd2, 0xe2,   -1, 0xf2}, // 1
    {0x03, 0x13, 0x23, 0x33, 0x12, 0x43,   -1, 0x63, 0x73, 0x83, 0x93, 0x72, 0xa3,   -1, 0xb3, 0xc3, 0xd3, 0xe3, 0xc2, 0xf3}, // 2
    {0x04, -1,   0x24, 0x34,   -1, 0x44, 0x54, 0x64,   -1, 0x84, 0x94,   -1, 0xa4, 0x53, 0xb4,   -1, 0xd4, 0xe4,   -1, 0xf4}, // 3
    {0x05, 0x15, 0x25, 0x35, 0x14, 0x45, 0x55, 0x65, 0x75, 0x85, 0x95, 0x74, 0xa5,   -1, 0xb5, 0xc5, 0xd5, 0xe5, 0xc4, 0xf5}, // 4
};

// But the internal wiring is not as simple as that. The MAX7219 has 8 pins for the "digits" and 8 pins for the "segments". So the wiring is split between these
// two ICs and each of them takes care of a single 8x6 matrix. For easier debugging of the "birds nest" of wires, we can remap each output pin:
int digit_pins_to_internal[8 * NUM_DEVICES] = {
    7, 6, 5, 4, 3, 2, 1, 0,       // IC1
    8, 9, 10, 11, 12, 13, 14, 15  // IC2
};

int segment_pins_to_internal[8 * NUM_DEVICES] = {
    1, 6, 4, 5, 3, 2, /* not connected: */ 7, 0,  // IC1
    6, 5, 4, 3, 2, 1, /* not connected: */ 7, 0   // IC2
};
// clang-format on
