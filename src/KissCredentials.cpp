// KissCredentials.cpp
// Vicente Soriano - victek@gmail.com

#include "KissCredentials.h"
#include "kiss_setup.h"
#include "lang.h"

// ========== CONSTRUCTOR ==========
KissCredentials::KissCredentials() {
  // Inicialización de variables
  botToken[0] = '\0';
  chatId[0] = '\0';
  wifiSSID[0] = '\0';
  wifiPassword[0] = '\0';
  otaPin[0] = '\0';
  otaPuk[0] = '\0';
  otaLocked = false;
}

// ========== MÉTODOS PÚBLICOS ==========

bool KissCredentials::begin() {
  loadFromNVS();

  // Verificar si tenemos credenciales válidas
  if (!areCredentialsValid()) {
    KISS_LOG("⚠️ Credenciales inválidas o vacías - usando fallback");
    resetToFallback();
    return false;
  }

  KISS_LOG("✅ Credenciales cargadas correctamente");
  return true;
}

void KissCredentials::resetToFallback() {
  KISS_LOG("🔄 Restableciendo credenciales a valores fallback");

  strncpy(botToken, KISS_FALLBACK_BOT_TOKEN, sizeof(botToken) - 1);
  strncpy(chatId, KISS_FALLBACK_CHAT_ID, sizeof(chatId) - 1);
  strncpy(wifiSSID, KISS_FALLBACK_WIFI_SSID, sizeof(wifiSSID) - 1);
  strncpy(wifiPassword, KISS_FALLBACK_WIFI_PASSWORD, sizeof(wifiPassword) - 1);
  strncpy(otaPin, KISS_FALLBACK_OTA_PIN, sizeof(otaPin) - 1);
  strncpy(otaPuk, KISS_FALLBACK_OTA_PUK, sizeof(otaPuk) - 1);

  // Asegurar terminación nula
  botToken[sizeof(botToken) - 1] = '\0';
  chatId[sizeof(chatId) - 1] = '\0';
  wifiSSID[sizeof(wifiSSID) - 1] = '\0';
  wifiPassword[sizeof(wifiPassword) - 1] = '\0';
  otaPin[sizeof(otaPin) - 1] = '\0';
  otaPuk[sizeof(otaPuk) - 1] = '\0';

  saveToNVS();
  KISS_LOG("✅ Credenciales fallback establecidas");
}

// ========== GETTERS ==========

const char* KissCredentials::getBotToken() {
  return botToken;
}

const char* KissCredentials::getChatId() {
  return chatId;
}

const char* KissCredentials::getWifiSSID() {
  return wifiSSID;
}

const char* KissCredentials::getWifiPassword() {
  return wifiPassword;
}

const char* KissCredentials::getOTAPin() {
  return otaPin;
}

const char* KissCredentials::getOTAPuk() {
  return otaPuk;
}

// ========== SETTERS ==========

bool KissCredentials::setBotToken(const char* token) {
  if (!validateToken(token)) {
    KISS_LOG("❌ Token de bot inválido");
    return false;
  }

  strncpy(botToken, token, sizeof(botToken) - 1);
  botToken[sizeof(botToken) - 1] = '\0';
  saveToNVS();
  KISS_LOG("✅ Token de bot actualizado");
  return true;
}

bool KissCredentials::setChatId(const char* chat_id) {
  if (!validateChatId(chat_id)) {
    KISS_LOG("❌ Chat ID inválido");
    return false;
  }

  strncpy(chatId, chat_id, sizeof(chatId) - 1);
  chatId[sizeof(chatId) - 1] = '\0';
  saveToNVS();
  KISS_LOG("✅ Chat ID actualizado");
  return true;
}

bool KissCredentials::setWifiSSID(const char* ssid) {
  if (ssid == nullptr || strlen(ssid) == 0 || strlen(ssid) > KISS_MAX_SSID_LENGTH - 1) {
    KISS_LOG("❌ SSID inválido");
    return false;
  }

  strncpy(wifiSSID, ssid, sizeof(wifiSSID) - 1);
  wifiSSID[sizeof(wifiSSID) - 1] = '\0';
  saveToNVS();
  KISS_LOG("✅ WiFi SSID actualizado");
  return true;
}

bool KissCredentials::setWifiPassword(const char* password) {
  if (password == nullptr || strlen(password) > KISS_MAX_WIFI_PASS_LENGTH - 1) {
    KISS_LOG("❌ Password de WiFi inválido");
    return false;
  }

  strncpy(wifiPassword, password, sizeof(wifiPassword) - 1);
  wifiPassword[sizeof(wifiPassword) - 1] = '\0';
  saveToNVS();
  KISS_LOG("✅ Password de WiFi actualizado");
  return true;
}

bool KissCredentials::setOTAPin(const char* pin) {
  if (!validatePin(pin)) {
    KISS_LOG("❌ PIN OTA inválido");
    return false;
  }

  strncpy(otaPin, pin, sizeof(otaPin) - 1);
  otaPin[sizeof(otaPin) - 1] = '\0';
  saveToNVS();
  KISS_LOG("✅ PIN OTA actualizado");
  return true;
}

bool KissCredentials::setOTAPuk(const char* puk) {
  if (!validatePuk(puk)) {
    KISS_LOG("❌ PUK OTA inválido");
    return false;
  }

  strncpy(otaPuk, puk, sizeof(otaPuk) - 1);
  otaPuk[sizeof(otaPuk) - 1] = '\0';
  saveToNVS();
  KISS_LOG("✅ PUK OTA actualizado");
  return true;
}

// ========== VERIFICACIÓN ==========

bool KissCredentials::areCredentialsValid() {
  bool valid = (strlen(botToken) > 10 &&  // Mínimo longitud token
                strlen(chatId) > 5 &&     // Mínimo longitud chat ID
                strlen(wifiSSID) > 0 &&   // SSID no vacío
                strlen(otaPin) >= 4 &&    // PIN mínimo 4 dígitos
                strlen(otaPuk) >= 8);     // PUK mínimo 8 dígitos

  if (!valid) {
    KISS_LOG("⚠️ Credenciales incompletas o inválidas");
  }

  return valid;
}

void KissCredentials::printStatus() {
  Serial.println("\n🔑 ESTADO CREDENCIALES:");
  Serial.printf(" - Bot Token: %s\n",
                strlen(botToken) > 10 ? "✅ CONFIGURADO" : "❌ NO CONFIGURADO");
  Serial.printf(" - Chat ID: %s\n",
                strlen(chatId) > 5 ? "✅ CONFIGURADO" : "❌ NO CONFIGURADO");
  Serial.printf(" - WiFi SSID: %s\n",
                strlen(wifiSSID) > 0 ? "✅ CONFIGURADO" : "❌ NO CONFIGURADO");
  Serial.printf(" - WiFi Password: %s\n",
                strlen(wifiPassword) > 0 ? "✅ CONFIGURADO" : "❌ NO CONFIGURADO");
  Serial.printf(" - OTA PIN: %s\n",
                strlen(otaPin) >= 4 ? "✅ CONFIGURADO" : "❌ NO CONFIGURADO");
  Serial.printf(" - OTA PUK: %s\n",
                strlen(otaPuk) >= 8 ? "✅ CONFIGURADO" : "❌ NO CONFIGURADO");
  Serial.printf(" - OTA Locked: %s\n", otaLocked ? "🔒 SI" : "🔓 NO");
}

// ========== BLOQUEO OTA ==========

bool KissCredentials::isOTALocked() {
  return otaLocked;
}

void KissCredentials::setOTALocked(bool locked) {
  otaLocked = locked;
  prefs.putBool(KISS_NVS_OTA_LOCKED_KEY, locked);
  KISS_LOGF("🔒 Estado OTA: %s", locked ? "BLOQUEADO" : "DESBLOQUEADO");
}

// ========== MÉTODOS PRIVADOS ==========

void KissCredentials::loadFromNVS() {
  // Cargar desde NVS
  if (!prefs.begin(KISS_NVS_NAMESPACE, true)) return;
  size_t tokenLen = prefs.getString(KISS_NVS_BOT_TOKEN_KEY, botToken, sizeof(botToken));
  size_t chatLen = prefs.getString(KISS_NVS_CHAT_ID_KEY, chatId, sizeof(chatId));
  size_t ssidLen = prefs.getString(KISS_NVS_WIFI_SSID_KEY, wifiSSID, sizeof(wifiSSID));
  size_t passLen = prefs.getString(KISS_NVS_WIFI_PASS_KEY, wifiPassword, sizeof(wifiPassword));
  size_t pinLen = prefs.getString(KISS_NVS_OTA_PIN_KEY, otaPin, sizeof(otaPin));
  size_t pukLen = prefs.getString(KISS_NVS_OTA_PUK_KEY, otaPuk, sizeof(otaPuk));

  otaLocked = prefs.getBool(KISS_NVS_OTA_LOCKED_KEY, false);

  prefs.end();

  KISS_LOGF("📥 Credenciales cargadas desde NVS - Token:%d, Chat:%d, SSID:%d",
            tokenLen, chatLen, ssidLen);
}

void KissCredentials::saveToNVS() {
  Preferences prefs;
  if (!prefs.begin(KISS_NVS_NAMESPACE, false)) return;
  
  // Guardar en NVS
  prefs.putString(KISS_NVS_BOT_TOKEN_KEY, botToken);
  prefs.putString(KISS_NVS_CHAT_ID_KEY, chatId);
  prefs.putString(KISS_NVS_WIFI_SSID_KEY, wifiSSID);
  prefs.putString(KISS_NVS_WIFI_PASS_KEY, wifiPassword);
  prefs.putString(KISS_NVS_OTA_PIN_KEY, otaPin);
  prefs.putString(KISS_NVS_OTA_PUK_KEY, otaPuk);
  prefs.putBool(KISS_NVS_OTA_LOCKED_KEY, otaLocked);

  prefs.end();

  KISS_LOG("💾 Credenciales guardadas en NVS");
}

bool KissCredentials::validateToken(const char* token) {
  if (token == nullptr || strlen(token) < 10) {
    return false;
  }

  // Verificar formato básico de token de bot (número:hash)
  bool hasColon = false;
  bool hasNumbers = false;

  for (size_t i = 0; i < strlen(token); i++) {
    if (token[i] == ':') hasColon = true;
    if (token[i] >= '0' && token[i] <= '9') hasNumbers = true;
  }

  return (hasColon && hasNumbers);
}

bool KissCredentials::validateChatId(const char* chat_id) {
  if (chat_id == nullptr || strlen(chat_id) < 5) {
    return false;
  }

  // Verificar que solo contiene números (para chat IDs simples)
  for (size_t i = 0; i < strlen(chat_id); i++) {
    if (chat_id[i] < '0' || chat_id[i] > '9') {
      return false;
    }
  }

  return true;
}

bool KissCredentials::validatePin(const char* pin) {
  if (pin == nullptr || strlen(pin) < 4 || strlen(pin) > KISS_MAX_PIN_LENGTH - 1) {
    return false;
  }

  // Verificar que solo contiene números
  for (size_t i = 0; i < strlen(pin); i++) {
    if (pin[i] < '0' || pin[i] > '9') {
      return false;
    }
  }

  return true;
}

bool KissCredentials::validatePuk(const char* puk) {
  if (puk == nullptr || strlen(puk) < 8 || strlen(puk) > KISS_MAX_PUK_LENGTH - 1) {
    return false;
  }

  // Verificar que solo contiene números
  for (size_t i = 0; i < strlen(puk); i++) {
    if (puk[i] < '0' || puk[i] > '9') {
      return false;
    }
  }

  return true;
}