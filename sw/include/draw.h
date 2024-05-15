#include <Arduino.h>

#include "display_layout.h"

// make the global variables from main.cpp visible
extern uint8_t visible_screen[CLOCK_ROWS][CLOCK_COLUMNS];

#define FONT_DIGITS_OFFSET 0
#define FONT_CHAR_MINUS_OFFSET 10
#define FONT_CHAR_O_OFFSET 11
#define FONT_CHAR_T_OFFSET 12
#define FONT_CHAR_A_OFFSET 13
#define FONT_CHAR_B_OFFSET 14
#define FONT_CHAR_W_OFFSET 15
#define FONT_CHAR_I_OFFSET 16
#define FONT_CHAR_F_OFFSET 17

void drawSymbol(int symbolFontOffset, int pos);
void drawMinusSign(int pos);
void drawTime(tm rtcTime);
void drawColons(bool onOff);
