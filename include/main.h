// external libraries first
#include <Arduino.h>
#include <ArduinoOTA.h>
#include <MD_MAX72xx.h>
// #include <SPI.h>
#include <Syslog.h>
#include <Timemark.h>
#if defined(ESP32)
  #include <WiFi.h>
  #include <esp_sntp.h>
  #include <esp_task_wdt.h>
#elif defined(ESP8266)
  // see https://arduino-esp8266.readthedocs.io/en/latest/ for libraries reference
  #include <ESP8266WiFi.h>
#else
  #error "Unsupported platform"
#endif
#include <WiFiManager.h>

// specific headers later
// clang-format off
#define LOCAL_DEBUG
#include "secrets.h"
#include "local_debug.h"
// clang-format on

// then everything else
#define VERSION "20240426.1"
#define USE_WDT 1
#define WDT_TIMEOUT 60  // seconds

#define MILLIS 1
#define SECONDS_TO_MILLIS (1000 * MILLIS)
#define MINUTES_TO_MILLIS (60 * SECONDS_TO_MILLIS)
#define HOURS_TO_MILLIS (60 * MINUTES_TO_MILLIS)

void wdtInit();
void wdtRefresh();
void wdtStop();
String resetReasonAsString();
String wakeupReasonAsString();
void logResetReason();
