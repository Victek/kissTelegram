// KissTime.h
// Vicente Soriano - victek@gmail.com
// Sistema de reloj NTP con persistencia NVS

#ifndef KISS_TIME_H
#define KISS_TIME_H

#include "system_setup.h"
#include <Preferences.h>

class KissTime {
public:
  static KissTime& getInstance() {
    static KissTime instance;
    return instance;
  }

  // Inicialización - sync NTP tras WiFi estable
  bool begin(const char* ntpServer = KISS_NTP_SERVER,
             long timezoneOffset = KISS_TIMEZONE_OFFSET);

  // 🕐 TIEMPO ACTUAL
  time_t getCurrentTime();
  bool isTimeSynced();
  unsigned long getLastSyncAge();  // Segundos desde último sync

  // 🔒 VALIDACIÓN SSL
  bool isCertificateValid(time_t notBefore, time_t notAfter);

  // ⏰ SCHEDULER
  bool isTimeForTask(int hour, int minute);
  bool isInTimeWindow(int startHour, int startMin, int endHour, int endMin);

  // 🔄 RESYNC
  bool resyncNTP();
  bool needsResync();  // true si > 24h sin sync

  // 📊 INFORMACIÓN - versión char[]
  void printStatus();
  void getTimeString(char* buffer, size_t bufferSize);
  void getDateString(char* buffer, size_t bufferSize);
  void getDateTimeString(char* buffer, size_t bufferSize);
  
private:
  KissTime();

  Preferences nvs;

  bool synced;
  time_t lastSyncTime;
  long timezoneOffset;
  char ntpServerAddr[64];

  // Helpers
  bool performNTPSync();
  bool loadFromNVS();
  bool saveToNVS();
};

#endif  // KISS_TIME_H