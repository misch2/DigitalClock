#include <Arduino.h>
#include <MD_MAX72xx.h>

#include "display_layout.h"

#define ELEVATION_FOR_MIN_INTENSITY -5
#define ELEVATION_FOR_MAX_INTENSITY 90

#define ELEVATION_BASED_INTENSITY_MIN 0
#define ELEVATION_BASED_INTENSITY_MAX MAX_INTENSITY

void debugShowVisibleScreen();
void debugShowInternalData();
void updateInternalScreen();
void sendInternalScreenToDevice();
void sendScreenToDevice();
void clearScreen();
void displaySelftest();
void displayTextBoot();
void displayTextWiFi();
void displayTextOTA(int percent);
void displayNotSyncedYet();
void setAutoIntensity(tm rtcTime);
void displayTime(tm rtcTime, bool showDots);

extern MD_MAX72XX mx;