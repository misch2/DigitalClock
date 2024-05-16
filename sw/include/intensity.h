#include "common/externs.h"

#define ELEVATION_FOR_MIN_INTENSITY -5
#define ELEVATION_FOR_MAX_INTENSITY 90

#define ELEVATION_BASED_INTENSITY_MIN 0
#define ELEVATION_BASED_INTENSITY_MAX MAX_INTENSITY

long map_and_clamp(long x, long in_min, long in_max, long out_min, long out_max);
void setAutoIntensity(tm rtcTime);
