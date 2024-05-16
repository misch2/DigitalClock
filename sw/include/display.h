#include <Arduino.h>

#include "display_layout.h"
#include "externs.h"

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
void displayTime(tm rtcTime, bool showDots);
