// KissCredentials.h
// Vicente Soriano - victek@gmail.com

#ifndef KISS_CREDENTIALS_H
#define KISS_CREDENTIALS_H

#include <Arduino.h>
#include <Preferences.h>
#include "kiss_setup.h"

class KissCredentials {
public:
  // ========== CONSTRUCTOR PÚBLICO ==========
  KissCredentials();  // Solo UN constructor aquí

  // ========== MÉTODOS PÚBLICOS ==========
  bool begin();
  void resetToFallback();

  // Getters
  const char* getBotToken();
  const char* getChatId();
  const char* getWifiSSID();
  const char* getWifiPassword();
  const char* getOTAPin();
  const char* getOTAPuk();

  // Setters
  bool setBotToken(const char* token);
  bool setChatId(const char* chat_id);
  bool setWifiSSID(const char* ssid);
  bool setWifiPassword(const char* password);
  bool setOTAPin(const char* pin);
  bool setOTAPuk(const char* puk);

  // Verificación
  bool areCredentialsValid();
  void printStatus();

  // Bloqueo OTA
  bool isOTALocked();
  void setOTALocked(bool locked);

private:
  // ========== VARIABLES PRIVADAS ==========
  Preferences prefs;

  char botToken[KISS_MAX_TOKEN_LENGTH];
  char chatId[KISS_MAX_CHAT_ID_LENGTH];
  char wifiSSID[KISS_MAX_SSID_LENGTH];
  char wifiPassword[KISS_MAX_WIFI_PASS_LENGTH];
  char otaPin[KISS_MAX_PIN_LENGTH];
  char otaPuk[KISS_MAX_PUK_LENGTH];
  bool otaLocked;

  // ========== MÉTODOS PRIVADOS ==========
  void loadFromNVS();
  void saveToNVS();
  bool validateToken(const char* token);
  bool validateChatId(const char* chat_id);
  bool validatePin(const char* pin);
  bool validatePuk(const char* puk);
};

#endif