#include <Arduino.h>
#include <ArduinoOTA.h>
#include <MD_MAX72xx.h>
// #include <SPI.h>
#include <SolarCalculator.h>
#include <Syslog.h>
#include <Timemark.h>
#if defined(ESP32)
  #include <WiFi.h>
  #include <esp_sntp.h>
#elif defined(ESP8266)
  // see https://arduino-esp8266.readthedocs.io/en/latest/ for libraries reference
  #include <ESP8266WiFi.h>
#else
  #error "Unsupported platform"
#endif
#include <WiFiManager.h>

// clang-format off
#define LOCAL_DEBUG
#include "secrets.h"
#include "local_debug.h"
#include "main.h"
#include "tests.h"
// clang-format on

#define TIMEZONE "CET-1CEST,M3.5.0,M10.5.0/3"  // Europe/Prague
#define NTP_SERVER "cz.pool.ntp.org"
double latitude = 49.0;
double longitude = 16.0;

// hardware configuration
#define HARDWARE_TYPE MD_MAX72XX::DR0CR0RR0_HW
#define MAX_DEVICES 2

// SPI hardware interface
// MD_MAX72XX mx = MD_MAX72XX(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);
// Specific SPI hardware interface
// MD_MAX72XX mx = MD_MAX72XX(HARDWARE_TYPE, SPI1, CS_PIN, MAX_DEVICES);
// Arbitrary pins (bit banged SPI, no problem with low speed devices like MAX72xx)
// MD_MAX72XX mx = MD_MAX72XX(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);
#if defined(ESP32)
  #define DATA_PIN 9
  #define CS_PIN 10
  #define CLK_PIN 11
MD_MAX72XX mx = MD_MAX72XX(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);
#elif defined(ESP8266)
  #define CS_PIN 15
MD_MAX72XX mx = MD_MAX72XX(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);
#endif

WiFiManager wifiManager;
WiFiClient wifiClient;
#if defined(SYSLOG_SERVER)
WiFiUDP udpClient;
Syslog syslog(udpClient, SYSLOG_SERVER, SYSLOG_PORT, SYSLOG_MYHOSTNAME, SYSLOG_MYAPPNAME, LOG_KERN);
#endif

time_t lastTimeSeconds = 0;

bool timeIsSyncedToNTP = false;
Timemark halfSecondIndicator(500 * MILLIS);
Timemark outOfSyncTimer(6 * HOURS_TO_MILLIS);

#if defined(ESP32)
String resetReasonAsString() {
  esp_reset_reason_t reset_reason = esp_reset_reason();
  if (reset_reason == ESP_RST_UNKNOWN) {
    return "UNKNOWN";
  } else if (reset_reason == ESP_RST_POWERON) {
    return "POWERON";
  } else if (reset_reason == ESP_RST_SW) {
    return "SW";
  } else if (reset_reason == ESP_RST_PANIC) {
    return "PANIC";
  } else if (reset_reason == ESP_RST_INT_WDT) {
    return "INT_WDT";
  } else if (reset_reason == ESP_RST_TASK_WDT) {
    return "TASK_WDT";
  } else if (reset_reason == ESP_RST_WDT) {
    return "WDT";
  } else if (reset_reason == ESP_RST_DEEPSLEEP) {
    return "DEEPSLEEP";
  } else if (reset_reason == ESP_RST_BROWNOUT) {
    return "BROWNOUT";
  } else if (reset_reason == ESP_RST_SDIO) {
    return "SDIO";
  }
  return "? (" + String(reset_reason) + ")";
};
#endif

void logResetReason() {
#if defined(ESP32)
  DEBUG_PRINT("Reset reason: %s", resetReasonAsString());
#elif defined(ESP8266)
  DEBUG_PRINT("Reset reason: %s", ESP.getResetReason().c_str());
#endif
}

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
int digit_pins_to_internal[8 * MAX_DEVICES] = {
    7, 6, 5, 4, 3, 2, 1, 0,       // IC1
    8, 9, 10, 11, 12, 13, 14, 15  // IC2
};

int segment_pins_to_internal[8 * MAX_DEVICES] = {
    1, 6, 4, 5, 3, 2, /* not connected: */ 7, 0,  // IC1
    6, 5, 4, 3, 2, 1, /* not connected: */ 7, 0   // IC2
};
// clang-format on

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

#define FONT_DIGITS_OFFSET 0
#define FONT_CHAR_MINUS_OFFSET 10
#define FONT_CHAR_O_OFFSET 11
#define FONT_CHAR_T_OFFSET 12
#define FONT_CHAR_A_OFFSET 13
#define FONT_CHAR_B_OFFSET 14
#define FONT_CHAR_W_OFFSET 15
#define FONT_CHAR_I_OFFSET 16
#define FONT_CHAR_F_OFFSET 17

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

int last_intensity = -1;
void displayTime(tm rtcTime, bool showDots) {
  clearScreen();
  drawTime(rtcTime);
  drawColons(showDots);
  sendScreenToDevice();

  time_t utcTime = mktime(&rtcTime);
  double azimuth, elevation;
  calcHorizontalCoordinates(utcTime, latitude, longitude, azimuth, elevation);
  // DEBUG_PRINT("Azimuth: %f, Elevation: %f", azimuth, elevation);
  // DEBUG_PRINT("UTC Time: %lld", utcTime);

  // // TODO use a light sensor or a sun position calculation to adjust the intensity
  // if (rtcTime.tm_hour >= 22 || rtcTime.tm_hour <= 5) {
  //   mx.control(MD_MAX72XX::INTENSITY, 0);
  // } else if (rtcTime.tm_hour >= 20 || rtcTime.tm_hour <= 7) {
  //   mx.control(MD_MAX72XX::INTENSITY, 3);
  // } else {
  //   mx.control(MD_MAX72XX::INTENSITY, 7);
  // };

  // Negative degrees value is used to set the minimal brightness only when the sun is below the horizon:
  int intensity = map(elevation, -10, 90, 0, MAX_INTENSITY);
  mx.control(MD_MAX72XX::INTENSITY, intensity);
  if (intensity != last_intensity) {
    DEBUG_PRINT("Sun elevation: %.1f deg -> intensity %d", elevation, intensity);
    last_intensity = intensity;
  }
}

void cbSyncTimeESP32(struct timeval *tv) {  // callback function to show when NTP was synchronized
  DEBUG_PRINT("NTP time synchronized to %s", NTP_SERVER);
  timeIsSyncedToNTP = true;
  outOfSyncTimer.start();
}

void cbSyncTimeESP8266(bool from_sntp) {  // callback function to show when NTP was synchronized
  if (from_sntp) {
    DEBUG_PRINT("NTP time synchronized to %s", NTP_SERVER);
    timeIsSyncedToNTP = true;
    outOfSyncTimer.start();
  } else {
    DEBUG_PRINT("NTP time synchronized to local time");
  };
}

void setupHelperForPinsAndSegments() {
  DEBUG_PRINT("Setting up pins and segments");

  Serial.println("All segments lit, identifying digits 0-15");
  mx.control(MD_MAX72XX::INTENSITY, 0);
  for (int i = 0; i < PHYSICAL_DIGIT_PINS; i++) {
    Serial.printf("Logical digit %d\n", i);
    mx.clear();
    mx.setColumn(digit_pins_to_internal[i], 0b11111111);
    delay(100);
  }

  Serial.println("All digits lit, identifying segments 0-7");
  // for (int device = 0; device < MAX_DEVICES; device++) {
  for (int device = 0; device < MAX_DEVICES - 1; device++) {
    for (int i = 0; i < PHYSICAL_SEGMENT_PINS; i++) {
      Serial.printf("Device %d segment %d\n", device, i);
      mx.clear();
      if (device == 0) {
        mx.setRow(device, segment_pins_to_internal[i], 0b11111111);
      } else {
        mx.setRow(device, segment_pins_to_internal[i + 8], 0b11111111);
      };
      delay(1000);
    }
  }
}

void setupHelperForSegments() {
  DEBUG_PRINT("Setting up segments on block A");

  for (int seg0 = 1; seg0 <= 6; seg0 += 4) {
    for (int seg1 = 6; seg1 <= 6; seg1++) {
      if (seg0 == seg1) {
        continue;
      }
      for (int seg2 = 4; seg2 <= 5; seg2++) {
        if (seg0 == seg2 || seg1 == seg2) {
          continue;
        }
        for (int seg3 = 1; seg3 <= 6; seg3++) {
          if (seg0 == seg3 || seg1 == seg3 || seg2 == seg3) {
            continue;
          }
          for (int seg4 = 1; seg4 <= 6; seg4++) {
            if (seg0 == seg4 || seg1 == seg4 || seg2 == seg4 || seg3 == seg4) {
              continue;
            }
            for (int seg5 = 1; seg5 <= 6; seg5++) {
              if (seg0 == seg5 || seg1 == seg5 || seg2 == seg5 || seg3 == seg5 || seg4 == seg5) {
                continue;
              }
              segment_pins_to_internal[0] = seg0;
              segment_pins_to_internal[1] = seg1;
              segment_pins_to_internal[2] = seg2;
              segment_pins_to_internal[3] = seg3;
              segment_pins_to_internal[4] = seg4;
              segment_pins_to_internal[5] = seg5;

              // valid combos:
              // 1, 6, 4, 2, 3, 5  : NE
              // 1, 6, 4, 5, 3, 2
              // 2, 6, 4, 5, 3, 1  : NE
              // 6, 5, 4, 2, 3, 1

              // 1, 6, 5, ?? 2, 3, 4 ???
              // 5, 6, 4, ???

              // 1, 6, 4, 5, 3, 2  akosor
              // 5, 6, 4,

              Serial.printf("[2345] Mapping: %d, %d, %d, %d, %d, %d\n", seg0, seg1, seg2, seg3, seg4, seg5);
              clearScreen();
              drawSymbol(FONT_DIGITS_OFFSET + 4, POSITION_DIGIT1);
              drawSymbol(FONT_DIGITS_OFFSET + 7, POSITION_DIGIT2);
              drawColons(true);
              sendScreenToDevice();
              delay(2000);
            }
          }
        }
      }
    }
  }
}

void setup() {
  Serial.begin(115200); /* prepare for possible serial debug */
  Serial.println("Starting...");

  DEBUG_PRINT("Initializing MD_MAX72XX");
  if (!mx.begin()) {
    DEBUG_PRINT("MD_MAX72XX initialization failed");
    // halt the program
    while (1) delay(1000);
  };

  // // // FIXME
  // while (1) {
  //   setupHelperForPinsAndSegments();
  // }
  // setupHelperForSegments();

  DEBUG_PRINT("Display selftest");
  mx.control(MD_MAX72XX::INTENSITY, MAX_INTENSITY);
  displaySelftest();
  displayTextWiFi();

  wifiManager.setHostname(HOSTNAME);
  wifiManager.setConnectRetries(3);
  wifiManager.setConnectTimeout(15);            // 15 seconds
  wifiManager.setConfigPortalTimeout(10 * 60);  // Stay 10 minutes max in the AP web portal, then reboot
  wifiManager.autoConnect();

  logResetReason();

  ArduinoOTA.setHostname(HOSTNAME);
  ArduinoOTA.begin();
  ArduinoOTA.onStart([]() {
    DEBUG_PRINT("OTA Start");
    displayTextOTA(0);
  });
  ArduinoOTA.onEnd([]() {
    DEBUG_PRINT("OTA End");
    displayTextBoot();
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    if (total > 0) {
      displayTextOTA(progress / (total / 100));
    }
  });

  outOfSyncTimer.start();
  displayNotSyncedYet();

  DEBUG_PRINT("SW version: %s", VERSION);
  DEBUG_PRINT("Timezone: %s", TIMEZONE);
  DEBUG_PRINT("NTP server: %s", NTP_SERVER);
  DEBUG_PRINT("Starting NTP sync...");
  configTzTime(TIMEZONE, NTP_SERVER);

#if defined(ESP32)
  sntp_set_time_sync_notification_cb(cbSyncTimeESP32);
#elif defined(ESP8266)
  settimeofday_cb(cbSyncTimeESP8266);
#endif
}

void loop() {
  ArduinoOTA.handle();

  if (outOfSyncTimer.expired()) {
    timeIsSyncedToNTP = false;
    DEBUG_PRINT("NTP time out of sync");
  };

  tm rtcTime;
  if (getLocalTime(&rtcTime)) {
    time_t curTimeSeconds = mktime(&rtcTime);
    if (lastTimeSeconds != curTimeSeconds) {  // every start of a new second
      lastTimeSeconds = curTimeSeconds;
      if (timeIsSyncedToNTP) {
        displayTime(rtcTime, true);
        halfSecondIndicator.start();
      } else {
        // display time but without blinking dots
        displayTime(rtcTime, false);
      };
    };
  };

  if (halfSecondIndicator.expired()) {
    displayTime(rtcTime, false);
  };
}
