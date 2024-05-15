#if !defined(LOCAL_DEBUG_H)
  #define LOCAL_DEBUG_H

// make the global variables from main.cpp visible
extern Syslog syslog;

  #if defined(LOCAL_DEBUG)
    #if defined(SYSLOG_SERVER)
      #define DEBUG_PRINT(...)                                \
        Serial.printf(__VA_ARGS__);                           \
        Serial.print('\n');                                   \
        Serial.flush();                                       \
        if (WiFi.status() == WL_CONNECTED) {                  \
          for (int i = 0; i < 3; i++) {                       \
            if (syslog.logf(LOG_INFO, __VA_ARGS__)) {         \
              break;                                          \
            } else {                                          \
              delay(100);                                     \
            }                                                 \
          }                                                   \
        } else {                                              \
          Serial.println(" (no syslog, WiFi not connected)"); \
        }
    #else
      #define DEBUG_PRINT(...)      \
        Serial.printf(__VA_ARGS__); \
        Serial.print('\n')
    #endif
  #else
    #define DEBUG_PRINT(...)
    #error FIXME
  #endif
#endif