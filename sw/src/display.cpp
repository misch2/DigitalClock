// clang-format off
#define LOCAL_DEBUG
#include "common/prolog.h"
// clang-format on

#include "display.h"

#include <Syslog.h>

#include "draw.h"
#include "intensity.h"

uint8_t visible_screen[CLOCK_ROWS][CLOCK_COLUMNS];
uint8_t internal_buffer[PHYSICAL_SEGMENT_PINS][PHYSICAL_DIGIT_PINS];

void debugShowVisibleScreen() {
#if defined(LOCAL_DEBUG)
  for (int row = 0; row < CLOCK_ROWS; row++) {
    for (int col = 0; col < CLOCK_COLUMNS; col++) {
      Serial.print(visible_screen[row][col] ? "▓▓" : "  ");
      if (col == 2 || col == 5 || col == 6 || col == 9 || col == 12 || col == 13 || col == 16) {
        Serial.print("  ");
      }
    }
    Serial.println();
  }
  Serial.println();
#endif
};

void debugShowInternalData() {
#if defined(LOCAL_DEBUG)
  for (int segment = 0; segment < PHYSICAL_SEGMENT_PINS; segment++) {
    Serial.printf("Segment (row) %d: ", segment);
    for (int digit = 0; digit < PHYSICAL_DIGIT_PINS; digit++) {
      if (internal_buffer[segment][digit]) {
        Serial.printf("%1x ", digit);
      } else {
        Serial.print("  ");
      }
    }
    Serial.println();
  }
  Serial.println();
#endif
};

void updateInternalScreen() {
  for (int row = 0; row < CLOCK_ROWS; row++) {
    for (int col = 0; col < CLOCK_COLUMNS; col++) {
      if (map_visible_to_internal_cords[row][col] == -1) {
        continue;
      };

      int digit = map_visible_to_internal_cords[row][col] >> 4;
      int segment = map_visible_to_internal_cords[row][col] & 0x0F;
      if (digit >= PHYSICAL_DIGIT_PINS || segment >= PHYSICAL_SEGMENT_PINS) {
        continue;
      };

      // Serial.printf("Visible screen: r%d,c%d\n", row, col);
      // Serial.printf("  -> tmp: s%d,d%d\n", segment, digit);
      if (digit < 8) {
        // first IC
        digit = digit_pins_to_internal[digit];
        segment = segment_pins_to_internal[segment];
      } else {
        // second IC
        digit = digit_pins_to_internal[digit];
        segment = segment_pins_to_internal[segment + 8];
      }
      if (digit < 0 || segment < 0) {
        continue;
      };
      // Serial.printf("     -> Internal screen: s%d,d%d\n", segment, digit);

      internal_buffer[segment][digit] = visible_screen[row][col];
    }
  }
};

void sendInternalScreenToDevice() {
  for (int digit = 0; digit < PHYSICAL_DIGIT_PINS; digit++) {
    int mask = 0;
    for (int i = 0; i < PHYSICAL_SEGMENT_PINS; i++) {
      mask <<= 1;
      mask |= internal_buffer[i][digit];
    }
    mx.setColumn(digit, mask);
  }
};

void sendScreenToDevice() {
  // debugShowVisibleScreen();
  updateInternalScreen();
  // debugShowInternalData();
  sendInternalScreenToDevice();
};

void clearScreen() {
  for (int row = 0; row < CLOCK_ROWS; row++) {
    for (int col = 0; col < CLOCK_COLUMNS; col++) {
      visible_screen[row][col] = 0;
    }
  }
}

void displaySelftest() {
  mx.control(MD_MAX72XX::TEST, MD_MAX72XX::ON);
  delay(1000);
  mx.control(MD_MAX72XX::TEST, MD_MAX72XX::OFF);

  // for (int i = 0; i <= 9; i++) {
  //   clearScreen();
  //   drawSymbol(FONT_DIGITS_OFFSET + i, POSITION_DIGIT1);
  //   drawSymbol(FONT_DIGITS_OFFSET + i, POSITION_DIGIT2);
  //   drawSymbol(FONT_DIGITS_OFFSET + i, POSITION_DIGIT3);
  //   drawSymbol(FONT_DIGITS_OFFSET + i, POSITION_DIGIT4);
  //   drawSymbol(FONT_DIGITS_OFFSET + i, POSITION_DIGIT5);
  //   drawSymbol(FONT_DIGITS_OFFSET + i, POSITION_DIGIT6);
  //   sendScreenToDevice();
  //   delay(200);
  // }
}

void displayTextBoot() {
  clearScreen();
  drawSymbol(FONT_CHAR_B_OFFSET, POSITION_DIGIT1);
  drawSymbol(FONT_CHAR_O_OFFSET, POSITION_DIGIT2);
  drawSymbol(FONT_CHAR_O_OFFSET, POSITION_DIGIT3);
  drawSymbol(FONT_CHAR_T_OFFSET, POSITION_DIGIT4);
  sendScreenToDevice();
}

void displayTextWiFi() {
  clearScreen();
  drawSymbol(FONT_CHAR_W_OFFSET, POSITION_DIGIT1);
  drawSymbol(FONT_CHAR_I_OFFSET, POSITION_DIGIT2);
  drawSymbol(FONT_CHAR_F_OFFSET, POSITION_DIGIT3);
  drawSymbol(FONT_CHAR_I_OFFSET, POSITION_DIGIT4);
  sendScreenToDevice();
}

void displayTextOTA(int percent) {
  clearScreen();
  drawSymbol(FONT_CHAR_O_OFFSET, POSITION_DIGIT1);
  drawSymbol(FONT_CHAR_T_OFFSET, POSITION_DIGIT2);
  drawSymbol(FONT_CHAR_A_OFFSET, POSITION_DIGIT3);
  if (percent >= 100) {
    drawSymbol(FONT_DIGITS_OFFSET + 1, POSITION_DIGIT4);
    percent -= 100;
  };
  drawSymbol(FONT_DIGITS_OFFSET + percent / 10, POSITION_DIGIT5);
  drawSymbol(FONT_DIGITS_OFFSET + percent % 10, POSITION_DIGIT6);
  sendScreenToDevice();
  mx.control(MD_MAX72XX::INTENSITY, MAX_INTENSITY);
};

// Display this pattern to indicate that time has not been synced yet:
// "-- -- --"
void displayNotSyncedYet() {
  clearScreen();
  drawMinusSign(POSITION_DIGIT1);
  drawMinusSign(POSITION_DIGIT2);
  drawMinusSign(POSITION_DIGIT3);
  drawMinusSign(POSITION_DIGIT4);
  drawMinusSign(POSITION_DIGIT5);
  drawMinusSign(POSITION_DIGIT6);
  sendScreenToDevice();
}

void displayTime(tm rtcTime, bool showDots) {
  setAutoIntensity(rtcTime);
  clearScreen();
  drawTime(rtcTime);
  drawColons(showDots);
  sendScreenToDevice();
}

void displayAllDigits(int offset) {
  drawSymbol(FONT_DIGITS_OFFSET + (offset + 0) % 10, POSITION_DIGIT1);
  drawSymbol(FONT_DIGITS_OFFSET + (offset + 1) % 10, POSITION_DIGIT2);
  drawSymbol(FONT_DIGITS_OFFSET + (offset + 2) % 10, POSITION_DIGIT3);
  drawSymbol(FONT_DIGITS_OFFSET + (offset + 3) % 10, POSITION_DIGIT4);
  drawSymbol(FONT_DIGITS_OFFSET + (offset + 4) % 10, POSITION_DIGIT5);
  drawSymbol(FONT_DIGITS_OFFSET + (offset + 5) % 10, POSITION_DIGIT6);
  drawColons(false);
  sendScreenToDevice();
};
