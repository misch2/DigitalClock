// external libraries first
#include <Arduino.h>
#include <ArduinoOTA.h>
#include <MD_MAX72xx.h>
#include <Syslog.h>
#include <Timemark.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <esp_sntp.h>
#include <esp_task_wdt.h>

// then everything else

// clang-format off
#include "secrets.h"
#include "debug.h"
#include "main.h"
// clang-format on

#define SECONDS_TO_MILLIS 1000
#define MILLIS 1

#define TIMEZONE "CET-1CEST,M3.5.0,M10.5.0/3"  // Europe/Prague
#define NTP_SERVER "cz.pool.ntp.org"

// hardware configuration
#define HARDWARE_TYPE MD_MAX72XX::DR0CR0RR0_HW
#define MAX_DEVICES 2
#define DATA_PIN 9
#define CS_PIN 10
#define CLK_PIN 11
// SPI hardware interface
// MD_MAX72XX mx = MD_MAX72XX(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);
// Specific SPI hardware interface
// MD_MAX72XX mx = MD_MAX72XX(HARDWARE_TYPE, SPI1, CS_PIN, MAX_DEVICES);
// Arbitrary pins (bit banged SPI, no problem with low speed devices like MAX72xx)
MD_MAX72XX mx = MD_MAX72XX(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);

WiFiManager wifiManager;
WiFiClient wifiClient;
time_t lastTime;
Timemark blinkingDotsIndicator(500 * MILLIS);

void wdtInit() {
#ifdef USE_WDT
  DEBUG_PRINT("Configuring WDT for %d seconds", WDT_TIMEOUT);
  esp_task_wdt_init(WDT_TIMEOUT, true);  // enable panic so ESP32 restarts
  esp_task_wdt_add(NULL);                // add current thread to WDT watch
#endif
}

void wdtRefresh() {
#ifdef USE_WDT
  // TRACE_PRINT("(WDT ping)");
  esp_task_wdt_reset();
#endif
}

void wdtStop() {
#ifdef USE_WDT
  DEBUG_PRINT("Stopping WDT...");
  esp_task_wdt_deinit();
#endif
}

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
  } else {
    return "? (" + String(reset_reason) + ")";
  }
};

String wakeupReasonAsString() {
  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
  if (wakeup_reason == ESP_SLEEP_WAKEUP_UNDEFINED) {
    return "UNDEFINED";
  } else if (wakeup_reason == ESP_SLEEP_WAKEUP_EXT0) {
    return "EXT0";
  } else if (wakeup_reason == ESP_SLEEP_WAKEUP_EXT1) {
    return "EXT1";
  } else if (wakeup_reason == ESP_SLEEP_WAKEUP_TIMER) {
    return "TIMER";
  } else if (wakeup_reason == ESP_SLEEP_WAKEUP_TOUCHPAD) {
    return "TOUCHPAD";
  } else if (wakeup_reason == ESP_SLEEP_WAKEUP_ULP) {
    return "ULP";
  } else {
    return "? (" + String(wakeup_reason) + ")";
  }
};

void logResetReason() {
  DEBUG_PRINT("Reset reason: %s", resetReasonAsString());
  String wr = wakeupReasonAsString();
  if (wr != "UNDEFINED") {
    DEBUG_PRINT("Wakeup reason: %s", wr);
  };
};

void cbSyncTime(struct timeval *tv) {  // callback function to show when NTP was synchronized
  DEBUG_PRINT("NTP time synchronized to %s", NTP_SERVER);
}

void test_blinking()
// Uses the test function of the MAX72xx to blink the display on and off.
{
  int nDelay = 1000;

  Serial.println("\nBlinking");

  while (nDelay > 0) {
    Serial.printf("Blinking delay: %d\n", nDelay);
    mx.control(MD_MAX72XX::TEST, MD_MAX72XX::ON);
    delay(nDelay);
    mx.control(MD_MAX72XX::TEST, MD_MAX72XX::OFF);
    delay(nDelay);

    nDelay -= 500;
    wdtRefresh();
  }

  mx.control(0, MD_MAX72XX::TEST, MD_MAX72XX::ON);
  delay(1000);
  mx.control(1, MD_MAX72XX::TEST, MD_MAX72XX::ON);
  delay(1000);
  mx.control(0, MD_MAX72XX::TEST, MD_MAX72XX::OFF);
  delay(1000);
  mx.control(1, MD_MAX72XX::TEST, MD_MAX72XX::OFF);
  delay(1000);

  Serial.println("Stopped blinking");
};

// Internal LED matrix:
//
// XXX XXX X XXX XXX   XXX XXX
// X X X X X X X X X X X X X X
// XXX XXX   XXX XXX   XXX XXX
// X X X X X X X X X X X X X X
// XXX XXX X XXX XXX   XXX XXX

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
    // IC 1
    7, 6, 5, 4, 3, 2, 1, 0,
    // IC 2
    15, 14, 13, 12, 11, 10, 9, 8
    };

int segment_pins_to_internal[8 * MAX_DEVICES] = {
    // IC 1
    0, 1, 2, 3, 4, 5, 6, 7,
    // IC 2
    0, 1, 2, 3, 4, 5, 6, 7};
// clang-format on

uint8_t visible_screen[CLOCK_ROWS][CLOCK_COLUMNS];
uint8_t internal_buffer[PHYSICAL_SEGMENT_PINS][PHYSICAL_DIGIT_PINS];

void debugShowVisibleScreen() {
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
};

void debugShowInternalData() {
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
};

void updateInternalScreen() {
  // FIXME buffer overflow here?
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
  // debugShowVisibleScreen();  // FIXME

  updateInternalScreen();
  // debugShowInternalData();  // FIXME
  sendInternalScreenToDevice();
};

void clearScreen() {
  for (int row = 0; row < CLOCK_ROWS; row++) {
    for (int col = 0; col < CLOCK_COLUMNS; col++) {
      visible_screen[row][col] = 0;
    }
  }
}

uint8_t digit_pixels[10][CLOCK_ROWS] = {{// 0
                                         0b111, 0b101, 0b101, 0b101, 0b111},
                                        {// 1
                                         0b001, 0b001, 0b001, 0b001, 0b001},
                                        {// 2
                                         0b111, 0b001, 0b111, 0b100, 0b111},
                                        {// 3
                                         0b111, 0b001, 0b111, 0b001, 0b111},
                                        {// 4
                                         0b101, 0b101, 0b111, 0b001, 0b001},
                                        {// 5
                                         0b111, 0b100, 0b111, 0b001, 0b111},
                                        {// 6
                                         0b111, 0b100, 0b111, 0b101, 0b111},
                                        {// 7
                                         0b111, 0b001, 0b001, 0b001, 0b001},
                                        {// 8
                                         0b111, 0b101, 0b111, 0b101, 0b111},
                                        {// 9
                                         0b111, 0b101, 0b111, 0b001, 0b111}};

void drawDigit(int digit, int pos) {
  for (int row = 0; row < CLOCK_ROWS; row++) {
    for (int col = 0; col < 3; col++) {
      visible_screen[row][pos + col] = (digit_pixels[digit][row] >> (2 - col)) & 0x01;
    }
  }
};

void drawMinusSign(int pos) {
  for (int col = 0; col < 3; col++) {
    visible_screen[2][pos + col] = 1;
  }
};

void drawTime(tm rtcTime) {
  // Serial.printf("Displaying RTC time: %02d:%02d:%02d\n", rtcTime.tm_hour, rtcTime.tm_min, rtcTime.tm_sec);

  drawDigit(rtcTime.tm_hour / 10, 0);
  drawDigit(rtcTime.tm_hour % 10, 3);
  drawDigit(rtcTime.tm_min / 10, 7);
  drawDigit(rtcTime.tm_min % 10, 10);
  drawDigit(rtcTime.tm_sec / 10, 14);
  drawDigit(rtcTime.tm_sec % 10, 17);
};

void drawDots(bool onOff) {
  visible_screen[1][6] = onOff;
  visible_screen[3][6] = onOff;

  visible_screen[1][13] = onOff;
  visible_screen[3][13] = onOff;
}

void test01() {
  Serial.println("in test01");
  mx.clear();
  mx.setPoint(0, 0, true);
  delay(1000);
  Serial.println("test01 finished");
};

void setup() {
  Serial.begin(115200); /* prepare for possible serial debug */

  wdtStop();
  wifiManager.setHostname(HOSTNAME);
  wifiManager.setConnectRetries(3);
  wifiManager.setConnectTimeout(15);            // 15 seconds
  wifiManager.setConfigPortalTimeout(10 * 60);  // Stay 10 minutes max in the AP web portal, then reboot
  wifiManager.autoConnect();
  logResetReason();
  wdtInit();

  ArduinoOTA.setHostname(HOSTNAME);
  ArduinoOTA.begin();
  ArduinoOTA.onStart([]() {
    DEBUG_PRINT("OTA Start");
    wdtStop();
  });
  ArduinoOTA.onEnd([]() { DEBUG_PRINT("OTA End"); });

  DEBUG_PRINT("Initializing MD_MAX72XX");
  if (!mx.begin()) {
    DEBUG_PRINT("MD_MAX72XX initialization failed");
    wdtStop();
    // sleep for an hour
    delay(3600 * SECONDS_TO_MILLIS);
    // reset the ESP32
    ESP.restart();
  };

  mx.control(MD_MAX72XX::INTENSITY, 0);
  // test_blinking();

  // indicate that time is not synced yet
  // clearScreen();

  // test01();

  // for (int repeat = 0; repeat < 999; repeat++) {
  //   for (int col = 0; col < CLOCK_COLUMNS; col++) {
  //     clearScreen();
  //     for (int row = 0; row < CLOCK_ROWS; row++) {
  //       visible_screen[row][col] = 1;
  //     }
  //     Serial.printf("displaying colum %d\n", col);
  //     sendScreenToDevice();
  //     delay(200);
  //     wdtRefresh();
  //   }
  //   delay(2000);
  // };

  drawMinusSign(0);
  drawMinusSign(3);
  drawMinusSign(7);
  drawMinusSign(10);
  drawMinusSign(14);
  drawMinusSign(17);
  sendScreenToDevice();
  // delay(30000);

  lastTime = 0;
  DEBUG_PRINT("Timezone: %s", TIMEZONE);
  DEBUG_PRINT("NTP server: %s", NTP_SERVER);
  DEBUG_PRINT("Starting NTP sync...");
  configTzTime(TIMEZONE, NTP_SERVER);
  sntp_set_time_sync_notification_cb(cbSyncTime);
}

void loop() {
  ArduinoOTA.handle();

  tm rtcTime;
  if (getLocalTime(&rtcTime)) {
    wdtRefresh();
    time_t curTime = mktime(&rtcTime);
    if (lastTime != curTime) {
      lastTime = curTime;
      blinkingDotsIndicator.start();  // reset

      clearScreen();
      drawTime(rtcTime);
      drawDots(true);
      sendScreenToDevice();

      if (rtcTime.tm_hour >= 22 || rtcTime.tm_hour <= 5) {
        mx.control(MD_MAX72XX::INTENSITY, 0);
      } else if (rtcTime.tm_hour >= 20 || rtcTime.tm_hour <= 7) {
        mx.control(MD_MAX72XX::INTENSITY, 3);
      } else {
        mx.control(MD_MAX72XX::INTENSITY, 7);
      };
    };
  } else {
    blinkingDotsIndicator.stop();
    // eventually WDT will reset the device if NTP sync takes too long
  };

  if (blinkingDotsIndicator.expired()) {
    drawDots(false);
    sendScreenToDevice();
  };

  delay(5 * MILLIS);
}
