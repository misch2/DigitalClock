#define DEBUG

#define HOSTNAME "clock01"
#define SYSLOG_SERVER "logserver.localnet"
#define SYSLOG_PORT 514
#define SYSLOG_MYAPPNAME "clock"
#define SYSLOG_MYHOSTNAME HOSTNAME

// https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv
#define TIMEZONE "CET-1CEST,M3.5.0,M10.5.0/3"  // Europe/Prague
#define NTP_SERVER "pool.ntp.org"
