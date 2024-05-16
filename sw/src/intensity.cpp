// clang-format off
#define LOCAL_DEBUG
#include "common/prolog.h"
// clang-format on

#include "intensity.h"

#include <SolarCalculator.h>

long map_and_clamp(long x, long in_min, long in_max, long out_min, long out_max) {
  return constrain(map(x, in_min, in_max, out_min, out_max), out_min, out_max);
}

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
  int intensity =
      map_and_clamp(elevation, ELEVATION_FOR_MIN_INTENSITY, ELEVATION_FOR_MAX_INTENSITY, ELEVATION_BASED_INTENSITY_MIN, ELEVATION_BASED_INTENSITY_MAX);
  mx.control(MD_MAX72XX::INTENSITY, intensity);
  if (intensity != last_intensity) {
    DEBUG_PRINT("Sun elevation: %.1f deg -> intensity %d", elevation, intensity);
    last_intensity = intensity;
  }
}
