// clang-format off
#define LOCAL_DEBUG
#include "common/prolog.h"
// clang-format on

#include <ArduinoOTA.h>
#include <MD_MAX72xx.h>
// #include <SPI.h>
#include <Timemark.h>
#include <WiFiManager.h>

#include "common/secrets.h"
#include "display.h"
#include "display_layout.h"
#include "draw.h"
#include "intensity.h"
#include "main.h"
#include "tests.h"
#include "utils.h"

#if defined(ESP32)
  #include <esp_sntp.h>
#endif

// hardware configuration
#define HARDWARE_TYPE MD_MAX72XX::DR0CR0RR0_HW

// SPI hardware interface
// MD_MAX72XX mx = MD_MAX72XX(HARDWARE_TYPE, CS_PIN, NUM_DEVICES);
// Specific SPI hardware interface
// MD_MAX72XX mx = MD_MAX72XX(HARDWARE_TYPE, SPI1, CS_PIN, NUM_DEVICES);
// Arbitrary pins (bit banged SPI, no problem with low speed devices like MAX72xx)
// MD_MAX72XX mx = MD_MAX72XX(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, NUM_DEVICES);
#if defined(INIT_MD_MAX72XX_WITH_FULL_PINS)
MD_MAX72XX mx = MD_MAX72XX(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, NUM_DEVICES);
#else
MD_MAX72XX mx = MD_MAX72XX(HARDWARE_TYPE, CS_PIN, NUM_DEVICES);
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

int lastButtonState = LOW;
enum clock_mode_t {
  MODE_NORMAL,
  MODE_TEST,
};
clock_mode_t mode = MODE_NORMAL;
clock_mode_t lastMode = MODE_NORMAL;

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
  // for (int device = 0; device < NUM_DEVICES; device++) {
  for (int device = 0; device < NUM_DEVICES - 1; device++) {
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

  initLightSensor();
  outOfSyncTimer.start();
  displayNotSyncedYet();

#if defined(TOUCH_BUTTON_PIN)
  #if defined(ESP32)
  pinMode(TOUCH_BUTTON_PIN, INPUT_PULLDOWN);
  #elif defined(ESP8266)
  pinMode(TOUCH_BUTTON_PIN, INPUT_PULLDOWN_16);
  #endif
#endif

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

#if defined(TOUCH_BUTTON_PIN)
  if (digitalRead(TOUCH_BUTTON_PIN) == HIGH) {
    if (lastButtonState == LOW) {
      // first touch
      mode = (mode == MODE_NORMAL) ? MODE_TEST : MODE_NORMAL;
    }
    lastButtonState = HIGH;
  } else {
    lastButtonState = LOW;
  }
#endif

  if (mode != lastMode) {
    if (mode == MODE_NORMAL) {
      DEBUG_PRINT("Switching to normal mode");
      test_off();
    } else if (mode == MODE_TEST) {
      DEBUG_PRINT("Switching to test mode");
      test_all_on();
    }
    lastMode = mode;
  }
  if (mode == MODE_TEST) {
    return;
  }

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
