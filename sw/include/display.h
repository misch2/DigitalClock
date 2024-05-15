#include <Arduino.h>
#include <MD_MAX72xx.h>

#include "display_layout.h"

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