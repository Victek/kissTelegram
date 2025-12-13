// KissTime.cpp
// Vicente Soriano - victek@gmail.com

#include "KissTime.h"
#include <WiFi.h>
#include "lang.h"

KissTime::KissTime() {
  synced = false;
  lastSyncTime = 0;
  timezoneOffset = 0;
  ntpServerAddr[0] = '\0';
}

bool KissTime::begin(const char* ntpServer, long tzOffset) {
  KISS_LOG("⏰ Inicializando KissTime...");

  if (WiFi.status() != WL_CONNECTED) {
    KISS_CRITICAL("❌ WiFi no conectado - no se puede sync NTP");
    return false;
  }

  // Guardar config
  strncpy(ntpServerAddr, ntpServer, sizeof(ntpServerAddr) - 1);
  ntpServerAddr[sizeof(ntpServerAddr) - 1] = '\0';
  timezoneOffset = tzOffset;

  // Cargar último sync de NVS
  loadFromNVS();

  // Intentar sync NTP
  if (performNTPSync()) {
    KISS_LOG("✅ NTP sincronizado");
    saveToNVS();
    return true;
  }

  KISS_CRITICAL("⚠️ Sync NTP falló - usando último tiempo guardado");
  return (lastSyncTime > 0);  // OK si tenemos tiempo guardado
}

bool KissTime::performNTPSync() {
  KISS_LOGF("🌐 Conectando NTP: %s", ntpServerAddr);

  // Configurar NTP
  configTime(timezoneOffset, 0, ntpServerAddr);

  // Esperar sync (max 10 segundos)
  int attempts = 0;
  time_t now = 0;

  while (attempts < KISS_NTP_RETRY) {
    time(&now);

    if (now > 1000000000) {  // Año > 2001 = sync OK
      lastSyncTime = now;
      synced = true;
      
      // Log con buffer local
      alignas(4) char timeBuffer[32];
      struct tm timeinfo;
      localtime_r(&now, &timeinfo);
      strftime(timeBuffer, sizeof(timeBuffer), "%Y-%m-%d %H:%M:%S", &timeinfo);
      KISS_LOGF("✅ NTP sync OK: %s", timeBuffer);
      return true;
    }

    KISS_LOGF("⏳ Esperando NTP... intento %d/%d", attempts + 1, KISS_NTP_RETRY);
    SAFE_DELAY(3000);
    attempts++;
  }

  KISS_CRITICAL("❌ Timeout NTP");
  return false;
}

time_t KissTime::getCurrentTime() {
  time_t now;
  time(&now);
  return now;
}

bool KissTime::isTimeSynced() {
  return synced && (lastSyncTime > 0);
}

unsigned long KissTime::getLastSyncAge() {
  if (lastSyncTime == 0) return 0xFFFFFFFF;
  return (getCurrentTime() - lastSyncTime);
}

bool KissTime::isCertificateValid(time_t notBefore, time_t notAfter) {
  if (!isTimeSynced()) {
    KISS_CRITICAL("⚠️ No se puede validar cert - tiempo no sincronizado");
    return false;
  }

  time_t now = getCurrentTime();
  return (now >= notBefore && now <= notAfter);
}

bool KissTime::isTimeForTask(int hour, int minute) {
    if (!isTimeSynced()) return false;

    time_t now = getCurrentTime();
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);

    return (timeinfo.tm_hour == hour && timeinfo.tm_min == minute);
}

bool KissTime::isInTimeWindow(int startHour, int startMin, int endHour, int endMin) {
    if (!isTimeSynced()) return false;

    time_t now = getCurrentTime();
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);

    int currentMinutes = timeinfo.tm_hour * 60 + timeinfo.tm_min;
    int startMinutes = startHour * 60 + startMin;
    int endMinutes = endHour * 60 + endMin;

    return (currentMinutes >= startMinutes && currentMinutes <= endMinutes);
}

bool KissTime::resyncNTP() {
  KISS_LOG("🔄 Resync NTP solicitado...");

  if (WiFi.status() != WL_CONNECTED) {
    KISS_CRITICAL("❌ WiFi no disponible");
    return false;
  }

  if (performNTPSync()) {
    saveToNVS();
    return true;
  }

  return false;
}

bool KissTime::needsResync() {
  if (!isTimeSynced()) return true;
  return (getLastSyncAge() > 86400);  // > 24 horas
}

void KissTime::printStatus() {
    KISS_LOG("\n⏰ KISSTIME STATUS:");
    KISS_LOGF("   Sincronizado: %s", synced ? "✅ SÍ" : "❌ NO");

    if (synced) {
        time_t now = getCurrentTime();
        struct tm timeinfo;
        localtime_r(&now, &timeinfo);
        
        alignas(4) char timeBuffer[32];
        strftime(timeBuffer, sizeof(timeBuffer), "%Y-%m-%d %H:%M:%S", &timeinfo);
        
        KISS_LOGF("   Hora actual: %s", timeBuffer);
        KISS_LOGF("   Último sync: hace %lu seg", getLastSyncAge());
        KISS_LOGF("   Timezone: UTC%+ld", timezoneOffset / 3600);
        KISS_LOGF("   Servidor NTP: %s", ntpServerAddr);
    }

    KISS_LOG("");
}

void KissTime::getTimeString(char* buffer, size_t bufferSize) {
    // Validación robusta
    if (!buffer || bufferSize < 10) {
        if (buffer && bufferSize > 0) buffer[0] = '\0';
        return;
    }
    
    if (!isTimeSynced()) {
        const char* noSync = "NO SYNC";
        strncpy(buffer, noSync, bufferSize - 1);
        buffer[bufferSize - 1] = '\0';
        return;
    }

    time_t now = getCurrentTime();
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);
    
    // Formato seguro
    strftime(buffer, bufferSize, "%H:%M:%S", &timeinfo);
}

void KissTime::getDateString(char* buffer, size_t bufferSize) {
    if (!buffer || bufferSize < 12) {
        if (buffer && bufferSize > 0) buffer[0] = '\0';
        return;
    }
    
    if (!isTimeSynced()) {
        strncpy(buffer, "NO SYNC", bufferSize - 1);
        buffer[bufferSize - 1] = '\0';
        return;
    }

    time_t now = getCurrentTime();
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);
    strftime(buffer, bufferSize, "%Y-%m-%d", &timeinfo);
}

void KissTime::getDateTimeString(char* buffer, size_t bufferSize) {
    if (!buffer || bufferSize < 22) {
        if (buffer && bufferSize > 0) buffer[0] = '\0';
        return;
    }
    
    if (!isTimeSynced()) {
        strncpy(buffer, "NO SYNC", bufferSize - 1);
        buffer[bufferSize - 1] = '\0';
        return;
    }

    time_t now = getCurrentTime();
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);
    strftime(buffer, bufferSize, "%Y-%m-%d %H:%M:%S", &timeinfo);
}

bool KissTime::loadFromNVS() {
    if (!nvs.begin("kiss_time", true)) return false;

    lastSyncTime = nvs.getULong("last_ntp_sync", 0);
    synced = (lastSyncTime > 0);

    nvs.end();

    if (lastSyncTime > 0) {
        struct tm timeinfo;
        localtime_r(&lastSyncTime, &timeinfo);
        alignas(4) char buffer[32];
        strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &timeinfo);
        KISS_LOGF("📥 Último sincr. NVS: %s", buffer);
    }

    return (lastSyncTime > 0);
}

bool KissTime::saveToNVS() {
  if (!nvs.begin("kiss_time", false)) return false;

  nvs.putULong("last_ntp_sync", lastSyncTime);
  nvs.end();

  KISS_LOG("💾 Hora guardada en NVS");
  return true;
}