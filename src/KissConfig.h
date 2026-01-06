// KissConfig.h
// Vicente Soriano - victek@gmail.com
// Gestor centralizado de configuración persistente

#ifndef KISS_CONFIG_H
#define KISS_CONFIG_H

#include "Kiss_setup.h"
#include <Preferences.h>

class KissConfig {
public:
  static KissConfig& getInstance() {
    static KissConfig instance;
    return instance;
  }

  // 🔧 CONFIGURACIÓN SISTEMA
  bool getAutoMessagesEnabled() {
    return getBool("auto_msgs", true);
  }
  bool setAutoMessagesEnabled(bool enabled) {
    return setBool("auto_msgs", enabled);
  }

  // ⚡ POWER MANAGEMENT
  int getPowerSavingLevel() {
    return getInt("power_level", 2);
  }
  bool setPowerSavingLevel(int level) {
    return setInt("power_level", level);
  }

  // 🔔 NOTIFICACIONES
  bool getErrorNotifications() {
    return getBool("error_notify", true);
  }
  bool setErrorNotifications(bool enabled) {
    return setBool("error_notify", enabled);
  }

  // 💾 CONFIGURACIÓN LITTLEFS
  int getMaxStorageMessages() {
    return getInt("max_storage", 1500);
  }
  bool setMaxStorageMessages(int max) {
    return setInt("max_storage", max);
  }

  int getCleanupIntervalHours() {
    return getInt("cleanup_hrs", 3);
  }
  bool setCleanupIntervalHours(int hours) {
    return setInt("cleanup_hrs", hours);
  }

  int getCleanupSentThreshold() {
    return getInt("cleanup_sent", 200);
  }
  bool setCleanupSentThreshold(int threshold) {
    return setInt("cleanup_sent", threshold);
  }

  // 🔄 REINTENTOS Y TIMEOUTS
  int getMessageRetryAttempts() {
    return getInt("retry_attempts", 5);
  }
  bool setMessageRetryAttempts(int attempts) {
    return setInt("retry_attempts", attempts);
  }

  int getWiFiReconnectTimeout() {
    return getInt("wifi_timeout", 30000);
  }
  bool setWiFiReconnectTimeout(int ms) {
    return setInt("wifi_timeout", ms);
  }

  int getMinMessageInterval() {
    return getInt("msg_interval", 1000);
  }
  bool setMinMessageInterval(int ms) {
    return setInt("msg_interval", ms);
  }

  // 📡 CONFIGURACIÓN DE RED (KissNet)
  // Modos: 0=AUTO (WiFi→LTE), 1=WIFI_ONLY, 2=LTE_ONLY
  int getNetworkMode() {
    return getInt("net_mode", 0);  // Por defecto AUTO
  }
  bool setNetworkMode(int mode) {
    return setInt("net_mode", mode);
  }

  // 🔥 TIMESTAMPS DE MANTENIMIENTO
  unsigned long getLastCleanupTime() {
    return getULong("last_cleanup", 0);
  }
  bool setLastCleanupTime(unsigned long timestamp) {
    return setULong("last_cleanup", timestamp);
  }

  unsigned long getLastSaveTime() {
    return getULong("last_save", 0);
  }
  bool setLastSaveTime(unsigned long timestamp) {
    return setULong("last_save", timestamp);
  }

  // 📊 ESTADÍSTICAS ACUMULATIVAS
  unsigned long getTotalMessagesSent() {
    return getULong("total_sent", 0);
  }
  bool setTotalMessagesSent(unsigned long count) {
    return setULong("total_sent", count);
  }

  unsigned long getTotalMessagesQueued() {
    return getULong("total_queued", 0);
  }
  bool setTotalMessagesQueued(unsigned long count) {
    return setULong("total_queued", count);
  }


  // UTILIDADES
  bool resetToDefaults() {
  Preferences prefs;  // ✅ Local
  prefs.begin("kiss_suite", false);
  prefs.clear();
  prefs.end();
  return true;
}

  void printConfig() {
    KISS_LOG("\n⚙️ CONFIGURACIÓN KISSTELEGRAM:");
    KISS_LOGF(" - Auto mensajes: %s", getAutoMessagesEnabled() ? "ON" : "OFF");
    KISS_LOGF(" - Power level: %d", getPowerSavingLevel());
    KISS_LOGF(" - Max storage: %d", getMaxStorageMessages());
    KISS_LOGF(" - Cleanup: cada %dh", getCleanupIntervalHours());
    KISS_LOGF(" - Cleanup threshold: %d mensajes", getCleanupSentThreshold());
    KISS_LOGF(" - Reintentos: %d", getMessageRetryAttempts());
    KISS_LOGF(" - WiFi timeout: %dms", getWiFiReconnectTimeout());
    KISS_LOGF(" - Intervalo mensajes: %dms", getMinMessageInterval());

    const char* netModes[] = {"AUTO (WiFi→LTE)", "WIFI ONLY", "LTE ONLY"};
    int mode = getNetworkMode();
    if (mode >= 0 && mode <= 2) {
      KISS_LOGF(" - Modo de red: %s", netModes[mode]);
    }

    KISS_LOGF(" - Último cleanup: hace %lu seg", (millis() - getLastCleanupTime()) / 1000);
    KISS_LOGF(" - Último save: hace %lu seg", (millis() - getLastSaveTime()) / 1000);
    KISS_LOGF(" - Notificaciones: %s", getErrorNotifications() ? "ON" : "OFF");
    KISS_LOG("\n📊 ESTADÍSTICAS ACUMULATIVAS:");
    KISS_LOGF(" - Total encolados: %lu", getTotalMessagesQueued());
    KISS_LOGF(" - Total enviados: %lu", getTotalMessagesSent());
    KISS_LOG("");
  }

private:
  KissConfig() {}

  bool getBool(const char* key, bool defaultValue);
  bool setBool(const char* key, bool value);
  int getInt(const char* key, int defaultValue);
  bool setInt(const char* key, int value);
  unsigned long getULong(const char* key, unsigned long defaultValue);
  bool setULong(const char* key, unsigned long value);
  String getString(const char* key, const char* defaultValue);
  bool setString(const char* key, const char* value);

};

#endif