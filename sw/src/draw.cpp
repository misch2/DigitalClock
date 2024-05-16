// clang-format off
#define LOCAL_DEBUG
#include "common/prolog.h"
// clang-format on

#include "draw.h"

uint8_t digit_pixels[19][CLOCK_ROWS] = {
    // FONT_DIGITS_OFFSET
    {0b111, 0b101, 0b101, 0b101, 0b111},  // 0
    {0b001, 0b001, 0b001, 0b001, 0b001},  // 1
    {0b111, 0b001, 0b111, 0b100, 0b111},  // 2
    {0b111, 0b001, 0b111, 0b001, 0b111},  // 3
    {0b101, 0b101, 0b111, 0b001, 0b001},  // 4
    {0b111, 0b100, 0b111, 0b001, 0b111},  // 5
    {0b111, 0b100, 0b111, 0b101, 0b111},  // 6
    {0b111, 0b001, 0b001, 0b001, 0b001},  // 7
    {0b111, 0b101, 0b111, 0b101, 0b111},  // 8
    {0b111, 0b101, 0b111, 0b001, 0b111},  // 9
    // FONT_CHAR_MINUS_OFFSET
    {0b000, 0b000, 0b111, 0b000, 0b000},  // -
    // FONT_CHAR_O_OFFSET
    {0b111, 0b101, 0b101, 0b101, 0b111},  // O
    {0b111, 0b010, 0b010, 0b010, 0b010},  // T
    {0b111, 0b101, 0b111, 0b101, 0b101},  // A
    {0b111, 0b101, 0b111, 0b101, 0b111},  // B
    {0b101, 0b101, 0b101, 0b101, 0b111},  // W
    {0b001, 0b001, 0b001, 0b001, 0b001},  // {0b111, 0b010, 0b010, 0b010, 0b111},  // I
    {0b111, 0b100, 0b111, 0b100, 0b100},  // F
    {}};

void drawSymbol(int symbolFontOffset, int pos) {
  for (int row = 0; row < CLOCK_ROWS; row++) {
    for (int col = 0; col < 3; col++) {
      visible_screen[row][pos + col] = (digit_pixels[symbolFontOffset][row] >> (2 - col)) & 0x01;
    }
  }
};

void drawMinusSign(int pos) { drawSymbol(FONT_CHAR_MINUS_OFFSET, pos); };

void drawTime(tm rtcTime) {
  // Serial.printf("Displaying RTC time: %02d:%02d:%02d\n", rtcTime.tm_hour, rtcTime.tm_min, rtcTime.tm_sec);

  // if (rtcTime.tm_hour >= 10) {  // do not display leading zero for hours
  drawSymbol(FONT_DIGITS_OFFSET + rtcTime.tm_hour / 10, POSITION_DIGIT1);
  // }
  drawSymbol(FONT_DIGITS_OFFSET + rtcTime.tm_hour % 10, POSITION_DIGIT2);
  drawSymbol(FONT_DIGITS_OFFSET + rtcTime.tm_min / 10, POSITION_DIGIT3);
  drawSymbol(FONT_DIGITS_OFFSET + rtcTime.tm_min % 10, POSITION_DIGIT4);
  drawSymbol(FONT_DIGITS_OFFSET + rtcTime.tm_sec / 10, POSITION_DIGIT5);
  drawSymbol(FONT_DIGITS_OFFSET + rtcTime.tm_sec % 10, POSITION_DIGIT6);
};

void drawColons(bool onOff) {
  visible_screen[1][POSITION_COLON1] = onOff;
  visible_screen[3][POSITION_COLON1] = onOff;

  visible_screen[1][POSITION_COLON2] = onOff;
  visible_screen[3][POSITION_COLON2] = onOff;
}
