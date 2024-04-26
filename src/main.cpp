// clang-format off
// Order of includes is important, see the https://github.com/bxparks/AceTime/discussions/86
// #include <AceTime.h>
// #include <AceTimeClock.h>
#include <Arduino.h>
#include <ArduinoOTA.h>
#include <NTPClient.h>
#include <Syslog.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <WiFiUdp.h>

// using ace_time::acetime_t;
// using ace_time::BasicZoneProcessor;
// using ace_time::TimeZone;
// using ace_time::ZonedDateTime;
// // using ace_time::clock::NtpClock;
// using ace_time::zonedb::kZoneEurope_Prague;
// clang-format on

#include "secrets.h"
#include "timezone.h" // remove FIXME

// then everything else can be included
#include "debug.h"
#include "main.h"

#ifdef USE_WDT
#include <esp_task_wdt.h>
#endif

#define SECONDS_TO_MILLIS 1000
#define MILLIS 1

WiFiManager wifiManager;
WiFiClient wifiClient;

// By default 'pool.ntp.org' is used with 60 seconds update interval and
// no offset
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);
// You can specify the time server pool and the offset, (in seconds)
// additionally you can specify the update interval (in milliseconds).
// NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", 3600, 60000);

static BasicZoneProcessor localZoneProcessor;
// static NtpClock ntpClock;

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
  // TRACE_PRINT("Stopping WDT...");
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

  timeClient.begin();
  // ntpClock.setup();
  // if (!ntpClock.isSetup()) {
  //   DEBUG_PRINT(F("Something went wrong with NTP clock."));
  //   return;
  // }
}

void loop() {
  ArduinoOTA.handle();
  wdtRefresh();
  delay(5 * MILLIS);

  timeClient.update();
  Serial.println(timeClient.getFormattedTime());
  // acetime_t nowSeconds = ntpClock.getNow();
  // auto localTz = TimeZone::forZoneInfo(&kZoneEurope_Prague, &localZoneProcessor);
  // auto localTime = ZonedDateTime::forEpochSeconds(nowSeconds, localTz);
  // DEBUG_PRINT(F("Local Time: %s:%s:%s"), localTime.hour(), localTime.minute(), localTime.second());

  delay(5000);
}
