#include <Arduino.h>
#if defined(ESP32)
  #include <WiFi.h>
#elif defined(ESP8266)
  #include <ESP8266WiFi.h>
#else
  #error "Unsupported platform"
#endif

// clang-format off
#define LOCAL_DEBUG
#include "secrets.h"
#include "local_debug.h"
// clang-format on

#include "utils.h"

String resetReasonAsString() {
#if defined(ESP32)
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
#elif defined(ESP8266)
  return ESP.getResetReason();
#else
  #error "Unsupported platform"
#endif
};

void logResetReason() { DEBUG_PRINT("Reset reason: %s", resetReasonAsString().c_str()); }
