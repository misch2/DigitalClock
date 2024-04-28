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

// hardware configuration
#define HARDWARE_TYPE MD_MAX72XX::DR0CR0RR0_HW
#define MAX_DEVICES 2  // FIXME tmp, should be 2
#define DATA_PIN 9
#define CS_PIN 10
#define CLK_PIN 11

#define SECONDS_TO_MILLIS 1000
#define MILLIS 1

WiFiManager wifiManager;
WiFiClient wifiClient;
time_t lastTime;
Timemark blinkingDotsIndicator(500 * MILLIS);

// SPI hardware interface
// MD_MAX72XX mx = MD_MAX72XX(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);
// Specific SPI hardware interface
// MD_MAX72XX mx = MD_MAX72XX(HARDWARE_TYPE, SPI1, CS_PIN, MAX_DEVICES);
// Arbitrary pins (bit banged SPI)
MD_MAX72XX mx = MD_MAX72XX(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);

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
  int nDelay = 300;

  Serial.println("\nBlinking");

  while (nDelay > 0) {
    Serial.printf("Blinking delay: %d\n", nDelay);
    mx.control(MD_MAX72XX::TEST, MD_MAX72XX::ON);
    delay(nDelay);
    mx.control(MD_MAX72XX::TEST, MD_MAX72XX::OFF);
    delay(nDelay);

    nDelay -= 30;
    wdtRefresh();
  }
  Serial.println("Stopped blinking");
};

void test_on_off() {
  for (int i = 0; i < 100; i++) {
    Serial.println("ON");
    for (int col = 0; col < COL_SIZE; col++) {
      mx.setColumn(0, col, 0b11111111);
    }
    delay(500);
    Serial.println("OFF");
    for (int col = 0; col < COL_SIZE; col++) {
      mx.setColumn(0, col, 0b00000000);
    }
    delay(500);
  }
};

void test_simple_on() {
  while (1) {
    mx.setColumn(0, 0, 0b11111111);
    mx.setColumn(0, 1, 0b11111111);
    mx.setColumn(0, 2, 0b11111111);
    mx.setColumn(0, 3, 0b11111111);
    mx.setColumn(0, 4, 0b11111111);
    mx.setColumn(0, 5, 0b11111111);
    mx.setColumn(0, 6, 0b11111111);
    mx.setColumn(0, 7, 0b11111111);
    delay(100000);
    // mx.setColumn(0, 0, 0x00);
    // delay(1000);
  };
};

/*

Physical LED matrix:

XXX XXX X XXX XXX   XXX XXX
X X X X X X X X X X X X X X
XXX XXX   XXX XXX   XXX XXX
X X X X X X X X X X X X X X
XXX XXX X XXX XXX   XXX XXX

Controlling pins on the LED matrix:
C0-C15 = GND, mapped to the MAX7219 digits
R0-R6  = Vcc, mapped to the MAX7219 segments

*/

#define DISPLAY_ROWS 5
#define DISPLAY_COLS 20

#define PHYSICAL_DIGIT_PINS 16
#define PHYSICAL_SEGMENT_PINS 7

// Mapping of define matrix of 8 rows and 16 columns
uint8_t map_virt_to_phys_cords[DISPLAY_ROWS][DISPLAY_COLS] = {
    {0x00, 0x11, 0x21, 0x31, 0x10, 0x40, 0x50, 0x60, 0x71, 0x81, 0x91, 0x70, 0xa0, 0xff, 0xb0, 0xc1, 0xd1, 0xe1, 0xc0, 0xf0},
    {0x02, 0xff, 0x22, 0x32, 0xff, 0x42, 0x52, 0x62, 0xff, 0x82, 0x92, 0xff, 0xa2, 0x51, 0xb2, 0xff, 0xd2, 0xe2, 0xff, 0xf2},
    {0x03, 0x13, 0x23, 0x33, 0x12, 0x43, 0xff, 0x63, 0x73, 0x83, 0x93, 0x72, 0xa3, 0xff, 0xb3, 0xc3, 0xd3, 0xe3, 0xc2, 0xf3},
    {0x04, 0xff, 0x24, 0x34, 0xff, 0x44, 0x54, 0x64, 0xff, 0x84, 0x94, 0xff, 0xa4, 0x53, 0xb4, 0xff, 0xd4, 0xe4, 0xff, 0xf4},
    {0x06, 0x16, 0x26, 0x36, 0x14, 0x46, 0x56, 0x66, 0x76, 0x86, 0x96, 0x74, 0xa6, 0xff, 0xb6, 0xc6, 0xd6, 0xe6, 0xc4, 0xf6},
};

uint8_t virtual_screen[DISPLAY_ROWS][DISPLAY_COLS];
uint8_t physical_screen[PHYSICAL_SEGMENT_PINS][PHYSICAL_DIGIT_PINS];

void debugShowVirtualScreen() {
  for (int row = 0; row < DISPLAY_ROWS; row++) {
    for (int col = 0; col < DISPLAY_COLS; col++) {
      Serial.print(virtual_screen[row][col] ? "▓▓" : "  ");
      if (col == 2 || col == 5 || col == 6 || col == 9 || col == 12 || col == 13 || col == 16) {
        Serial.print("  ");
      }
    }
    Serial.println();
  }
  Serial.println();
};

void debugShowPhysicalScreen() {
  for (int segment = 0; segment < PHYSICAL_SEGMENT_PINS; segment++) {
    Serial.printf("Segment (row) %d: ", segment);
    for (int digit = 0; digit < PHYSICAL_DIGIT_PINS; digit++) {
      if (physical_screen[segment][digit]) {
        Serial.printf("%1x", digit);
      } else {
        Serial.print(" ");
      }
    }
    Serial.println();
  }
  Serial.println();
};

void copyVirtualToPhysical() {
  for (int row = 0; row < DISPLAY_ROWS; row++) {
    for (int col = 0; col < DISPLAY_COLS; col++) {
      uint8_t digit = map_virt_to_phys_cords[row][col] >> 4;
      uint8_t segment = map_virt_to_phys_cords[row][col] & 0x0F;
      if (digit < PHYSICAL_DIGIT_PINS && segment < PHYSICAL_SEGMENT_PINS) {
        physical_screen[segment][digit] = virtual_screen[row][col];
      }
    }
  }
};

void showPhysicalScreen() {
  for (int digit = 0; digit < PHYSICAL_DIGIT_PINS; digit++) {
    int mask = 0;
    for (int i = 0; i < PHYSICAL_SEGMENT_PINS; i++) {
      mask <<= 1;
      mask |= physical_screen[i][digit];
    }
    mx.setColumn(digit, mask);
  }
};

void showVirtualScreen() {
  // Serial.println("Showing virtual screen");
  debugShowVirtualScreen();  // FIXME

  copyVirtualToPhysical();

  debugShowPhysicalScreen();  // FIXME
  showPhysicalScreen();
};

void clearVirtualScreen() {
  for (int row = 0; row < DISPLAY_ROWS; row++) {
    for (int col = 0; col < DISPLAY_COLS; col++) {
      virtual_screen[row][col] = 0;
    }
  }
}

uint8_t digit_pixels[10][DISPLAY_ROWS] = {{// 0
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

void drawVirtualDigit(int digit, int pos) {
  for (int row = 0; row < DISPLAY_ROWS; row++) {
    for (int col = 0; col < 3; col++) {
      virtual_screen[row][pos + col] = (digit_pixels[digit][row] >> (2 - col)) & 0x01;
    }
  }
};

void drawTime(tm rtcTime) {
  // Serial.printf("Displaying RTC time: %02d:%02d:%02d\n", rtcTime.tm_hour, rtcTime.tm_min, rtcTime.tm_sec);

  drawVirtualDigit(rtcTime.tm_hour / 10, 0);
  drawVirtualDigit(rtcTime.tm_hour % 10, 3);
  drawVirtualDigit(rtcTime.tm_min / 10, 7);
  drawVirtualDigit(rtcTime.tm_min % 10, 10);
  drawVirtualDigit(rtcTime.tm_sec / 10, 14);
  drawVirtualDigit(rtcTime.tm_sec % 10, 17);
};

void drawDots(bool onOff) {
  virtual_screen[1][6] = onOff;
  virtual_screen[3][6] = onOff;

  virtual_screen[1][13] = onOff;
  virtual_screen[3][13] = onOff;
}

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

  // mx.control(MD_MAX72XX::INTENSITY, MAX_INTENSITY);
  test_blinking();
  Serial.println("Test blinking done");
  mx.clear();
  // test_on_off();
  // test_simple_on();
  // Serial.println("Test on/off done");

  // // FIXME test
  // digitalWrite(RGB_BUILTIN, HIGH);  // Turn the RGB LED white
  // delay(200);
  // digitalWrite(RGB_BUILTIN, LOW);  // Turn the RGB LED off
  // delay(200);
  // neopixelWrite(RGB_BUILTIN, RGB_BRIGHTNESS, 0, 0);  // Red
  // delay(200);
  // neopixelWrite(RGB_BUILTIN, 0, RGB_BRIGHTNESS, 0);  // Green
  // delay(200);
  // neopixelWrite(RGB_BUILTIN, 0, 0, RGB_BRIGHTNESS);  // Blue
  // delay(200);
  // neopixelWrite(RGB_BUILTIN, 0, 0, 0);  // Off / black
  // delay(200);

  lastTime = 0;
  DEBUG_PRINT("Timezone: %s", TIMEZONE);
  DEBUG_PRINT("NTP server: %s", NTP_SERVER);
  DEBUG_PRINT("Starting NTP sync...");
  configTzTime(TIMEZONE, NTP_SERVER);
  sntp_set_time_sync_notification_cb(cbSyncTime);
}

void loop() {
  // Serial.println("Looping...");
  ArduinoOTA.handle();
  wdtRefresh();

  tm rtcTime;
  if (getLocalTime(&rtcTime)) {
    time_t curTime = mktime(&rtcTime);
    if (lastTime != curTime) {
      lastTime = curTime;
      blinkingDotsIndicator.start();  // reset

      clearVirtualScreen();
      drawTime(rtcTime);
      drawDots(true);
      showVirtualScreen();
    };
  } else {
    Serial.println("RTC time not available");
    blinkingDotsIndicator.stop();
  };

  if (blinkingDotsIndicator.expired()) {
    drawDots(false);
    showVirtualScreen();
  };

  delay(5 * MILLIS);
}
