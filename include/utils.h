#include <Arduino.h>

void wdtInit();
void wdtRefresh();
void wdtStop();
String resetReasonAsString();
String wakeupReasonAsString();
void logResetReason();
