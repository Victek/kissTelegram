// KissConfig.cpp
// Vicente Soriano - victek@gmail.com
// Gestor centralizado de configuración persistente

#include "KissConfig.h"

// ========== IMPLEMENTACIÓN PRIVADA ==========
bool KissConfig::getBool(const char* key, bool defaultValue) {
  Preferences prefs;
  if (!prefs.begin("kiss_suite", true)) return defaultValue;
  bool value = prefs.getBool(key, defaultValue);
  prefs.end();
  return value;
}

bool KissConfig::setBool(const char* key, bool value) {
  Preferences prefs;
  if (!prefs.begin("kiss_suite", false)) return false;
  prefs.putBool(key, value);
  prefs.end();
  return true;
}

int KissConfig::getInt(const char* key, int defaultValue) {
  Preferences prefs;
  if (!prefs.begin("kiss_suite", true)) return defaultValue;
  int value = prefs.getInt(key, defaultValue);
  prefs.end();
  return value;
}

bool KissConfig::setInt(const char* key, int value) {
  Preferences prefs;
  if (!prefs.begin("kiss_suite", false)) return false;
  prefs.putInt(key, value);
  prefs.end();
  return true;
}

unsigned long KissConfig::getULong(const char* key, unsigned long defaultValue) {
  Preferences prefs;
  if (!prefs.begin("kiss_suite", true)) return defaultValue;
  unsigned long value = prefs.getULong(key, defaultValue);
  prefs.end();
  return value;
}

bool KissConfig::setULong(const char* key, unsigned long value) {
  Preferences prefs;
  if (!prefs.begin("kiss_suite", false)) return false;
  prefs.putULong(key, value);
  prefs.end();
  return true;
}

bool KissConfig::setString(const char* key, const char* value) {
  Preferences prefs;
  if (!prefs.begin("kiss_suite", false)) return false;
  prefs.putString(key, value);
  prefs.end();
  return true;
}