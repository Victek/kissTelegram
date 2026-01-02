// KissClient.h
// Vicente Soriano - victek@gmail.com
// Interfaz abstracta unificada para clientes de red (WiFi y LTE)

#ifndef KISS_CLIENT_H
#define KISS_CLIENT_H

#include <Arduino.h>
#include "Kiss_setup.h"

// ========== POWER MODES ==========
// Sincronizado con KissTelegram::PowerMode
enum KissClientPowerMode {
  CLIENT_POWER_BOOT,         // Inicialización
  CLIENT_POWER_LOW,          // Bajo consumo, sleep activado
  CLIENT_POWER_IDLE,         // RF OFF, escucha mínima
  CLIENT_POWER_ACTIVE,       // Funcionamiento normal
  CLIENT_POWER_TURBO,        // Máximo rendimiento
  CLIENT_POWER_MAINTENANCE   // Mantenimiento, consumo mínimo
};

// ========== INTERFAZ BASE ==========
class KissClient {
public:
  virtual ~KissClient() {}

  // ========== CONEXIÓN Y ESTADO ==========
  virtual bool connectToTelegram() = 0;
  virtual bool connect(const char* host, uint16_t port) = 0;
  virtual bool connected() = 0;
  virtual bool isConnected() = 0;
  virtual void disconnect() = 0;
  virtual void stop() = 0;

  // ========== SEGURIDAD ==========
  virtual void setCACert(const char* rootCA) = 0;
  virtual void setInsecure() = 0;
  virtual void setSecureMode(bool secure) = 0;
  virtual bool isSecureMode() = 0;
  virtual bool verify(const char* fingerprint, const char* url) = 0;

  // ========== I/O BÁSICO ==========
  virtual size_t print(const char* str) = 0;
  virtual size_t print(const String& str) = 0;
  virtual size_t println(const char* str) = 0;
  virtual size_t println(const String& str) = 0;
  virtual int available() = 0;
  virtual int read() = 0;
  virtual int read(uint8_t* buffer, size_t size) = 0;

  // ========== POWER MANAGEMENT ==========
  virtual bool setPowerMode(KissClientPowerMode mode) = 0;
  virtual KissClientPowerMode getPowerMode() = 0;
  virtual int getCurrentConsumption() = 0;  // Consumo estimado en mA

  // ========== INFORMACIÓN ==========
  virtual void printInfo() = 0;
  virtual const char* getClientType() = 0;  // "WiFi" o "LTE"
  virtual int getSignalStrength() = 0;      // RSSI en dBm
  virtual const char* getConnectionInfo() = 0;

  // ========== DIAGNÓSTICOS ==========
  virtual bool isConnectionHealthy() = 0;
  virtual unsigned long getConnectionAge() = 0;
  virtual int getErrorCount() = 0;
  virtual void resetErrorCount() = 0;

protected:
  KissClientPowerMode _powerMode = CLIENT_POWER_BOOT;
  unsigned long _connectionStartTime = 0;
  int _errorCount = 0;
};

#endif // KISS_CLIENT_H
