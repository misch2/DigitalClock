#if defined(ESP32)
  #define HOSTNAME "esp30-clock"
#elif defined(ESP8266)
  #define HOSTNAME "esp28-clock"
#endif

#define SYSLOG_SERVER "logserver.localnet"
#define SYSLOG_PORT 514
#define SYSLOG_MYAPPNAME "clock"
#define SYSLOG_MYHOSTNAME HOSTNAME
