#include <Arduino.h>
#include <SolarCalculator.h>
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

#include "intensity.h"

int last_intensity = -1;
void setAutoIntensity(tm rtcTime) {
  time_t utcTime = mktime(&rtcTime);
  double latitude = LATITUDE;
  double longitude = LONGITUDE;
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
  int intensity = map(constrain(elevation, ELEVATION_FOR_MIN_INTENSITY, ELEVATION_FOR_MAX_INTENSITY),  // value needs to be capped!
                      ELEVATION_FOR_MIN_INTENSITY, ELEVATION_FOR_MAX_INTENSITY,                        // map from
                      ELEVATION_BASED_INTENSITY_MIN, ELEVATION_BASED_INTENSITY_MAX                     // map to
  );
  mx.control(MD_MAX72XX::INTENSITY, intensity);
  if (intensity != last_intensity) {
    DEBUG_PRINT("Sun elevation: %.1f deg -> intensity %d", elevation, intensity);
    last_intensity = intensity;
  }
}
