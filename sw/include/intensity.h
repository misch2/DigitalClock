#include <Arduino.h>

#include "externs.h"

#define ELEVATION_FOR_MIN_INTENSITY -5
#define ELEVATION_FOR_MAX_INTENSITY 90

#define ELEVATION_BASED_INTENSITY_MIN 0
#define ELEVATION_BASED_INTENSITY_MAX MAX_INTENSITY

void setAutoIntensity(tm rtcTime);
