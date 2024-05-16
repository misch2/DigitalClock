#include <Arduino.h>
#if defined(ESP32)
  #include <WiFi.h>
#elif defined(ESP8266)
  #include <ESP8266WiFi.h>
#else
  #error "Unsupported platform"
#endif

// clang-format off
#include "secrets.h"
#include "local_debug.h"
// clang-format on
