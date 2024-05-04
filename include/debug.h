#ifdef DEBUG
  #ifdef SYSLOG_SERVER

// A UDP instance to let us send and receive packets over UDP
WiFiUDP udpClient;
// Create a new syslog instance with LOG_KERN facility
Syslog syslog(udpClient, SYSLOG_SERVER, SYSLOG_PORT, SYSLOG_MYHOSTNAME, SYSLOG_MYAPPNAME, LOG_KERN);
    #ifndef DEBUG_PRINT
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
    #endif
  #else
    #define DEBUG_PRINT(...)      \
      Serial.printf(__VA_ARGS__); \
      Serial.print('\n')
  #endif
#else
  #define DEBUG_PRINT(...)
#endif

// WiFi is connected now => all messages go to syslog too.
// But FIXME, first few UDP packets are not sent:  //  [
// 1113][E][WiFiUdp.cpp:183] endPacket(): could not send data: 12
