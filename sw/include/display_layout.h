// Internal LED matrix:
//
// XXX XXX X XXX XXX   XXX XXX
// X X X X X X X X X X X X X X
// XXX XXX   XXX XXX   XXX XXX
// X X X X X X X X X X X X X X
// XXX XXX X XXX XXX   XXX XXX

#define POSITION_DIGIT1 0
#define POSITION_DIGIT2 3
#define POSITION_COLON1 6
#define POSITION_DIGIT3 7
#define POSITION_DIGIT4 10
#define POSITION_COLON2 13
#define POSITION_DIGIT5 14
#define POSITION_DIGIT6 17

// How the visible screen is mapped to the physical screen.
// While there is 20x5 pixels visible on the clock, the internal wiring is 16x6 pins (8x6 + 8x6 really)
#define CLOCK_ROWS 5
#define CLOCK_COLUMNS 20
#define PHYSICAL_DIGIT_PINS 16
#define PHYSICAL_SEGMENT_PINS 8

#define NUM_DEVICES 2

// make variables globally available
extern int map_visible_to_internal_cords[CLOCK_ROWS][CLOCK_COLUMNS];
extern int digit_pins_to_internal[];
extern int segment_pins_to_internal[];
