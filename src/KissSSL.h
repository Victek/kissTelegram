#ifndef KISS_SSL_H
#define KISS_SSL_H

#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <WiFi.h>
#include "KissClient.h"

class KissSSL : public KissClient {
public:
  KissSSL();
  ~KissSSL();

  // ========== MÉTODOS VIRTUALES KissClient ==========
  bool connectToTelegram() override;
  bool connect(const char* host, uint16_t port) override;
  bool connected() override;
  bool isConnected() override;
  void disconnect() override;

  void setCACert(const char* rootCA) override;
  void setInsecure() override;
  bool verify(const char* fingerprint, const char* url) override;
  void setSecureMode(bool secure) override;
  bool isSecureMode() override;

  size_t print(const char* str) override;
  size_t print(const String& str) override;
  size_t println(const char* str) override;
  size_t println(const String& str) override;

  int available() override;
  int read() override;
  int read(uint8_t* buffer, size_t size) override;
  void stop() override;
  void printInfo() override;

  // ========== POWER MANAGEMENT ==========
  bool setPowerMode(KissClientPowerMode mode) override;
  KissClientPowerMode getPowerMode() override;
  int getCurrentConsumption() override;

  // ========== INFORMACIÓN ==========
  const char* getClientType() override;
  int getSignalStrength() override;
  const char* getConnectionInfo() override;

  // ========== DIAGNÓSTICOS ==========
  bool isConnectionHealthy() override;
  unsigned long getConnectionAge() override;
  int getErrorCount() override;
  void resetErrorCount() override;

  // ========== MÉTODOS LEGACY (mantener compatibilidad) ==========
  void toggleSecureMode();

private:
  WiFiClientSecure* client;
  bool manuallyConnected;
  bool secureMode;
};

#endif