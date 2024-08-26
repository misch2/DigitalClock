#define VERSION "20240826.2"

#define MILLIS 1
#define SECONDS_TO_MILLIS (1000 * MILLIS)
#define MINUTES_TO_MILLIS (60 * SECONDS_TO_MILLIS)
#define HOURS_TO_MILLIS (60 * MINUTES_TO_MILLIS)

String resetReasonAsString();
String wakeupReasonAsString();
void logResetReason();
