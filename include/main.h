#define VERSION "20240504.1"
#define USE_WDT 1
#define WDT_TIMEOUT 60  // seconds

#define MILLIS 1
#define SECONDS_TO_MILLIS (1000 * MILLIS)
#define MINUTES_TO_MILLIS (60 * SECONDS_TO_MILLIS)
#define HOURS_TO_MILLIS (60 * MINUTES_TO_MILLIS)

void wdtInit();
void wdtRefresh();
void wdtStop();
String resetReasonAsString();
String wakeupReasonAsString();
void logResetReason();
