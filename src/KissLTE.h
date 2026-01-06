// KissLTE.h
// Vicente Soriano - victek@gmail.com
// Gestor LTE genérico con detección automática de módulo
// Soporta: Quectel EC200A, SIMCOM A7672E/A7682E
// Auto-detección: AT+GMM (respuesta rápida=SIMCOM, lenta=Quectel)

#ifndef KISS_LTE_H
#define KISS_LTE_H

#include "Kiss_setup.h"
#include "KissClient.h"
#include "modules/KissLTEModule.h"
#include <HardwareSerial.h>
#include <Preferences.h>

// Forward declarations
class QuectelModule;
class SIMCOMModule;

/**
 * @brief Gestor principal LTE con lazy initialization
 *
 * Características:
 * - Singleton pattern
 * - Lazy init: begin() solo configura pines, NO enciende módulo
 * - Auto-detección: powerOn() detecta Quectel/SIMCOM
 * - Delegación: Todos los comandos AT van al módulo específico
 * - Control hardware: STATUS_PIN, RESET_PIN, PWRKEY, DTR
 */
class KissLTE : public KissClient {
public:
  // Singleton pattern
  static KissLTE& getInstance() {
    static KissLTE instance;
    return instance;
  }

  // Estados de conexión
  enum ConnectionState {
    STATE_OFF,            // Módulo apagado físicamente (STATUS=LOW)
    STATE_POWERING_ON,    // Iniciando encendido
    STATE_INITIALIZING,   // Respondiendo a comandos AT
    STATE_REGISTERING,    // Buscando red
    STATE_CONNECTED,      // Conectado y con IP
    STATE_ERROR           // Error crítico
  };

  // ========== INICIALIZACIÓN (LAZY) ==========
  /**
   * @brief Prepara hardware SIN encender módulo
   * @param serial Puerto serial (por defecto Serial1)
   * @param rxPin GPIO RX
   * @param txPin GPIO TX
   * @param pwrKeyPin GPIO PWRKEY
   * @param statusPin GPIO STATUS (HIGH=ON, LOW=OFF)
   * @param resetPin GPIO RESET (opcional)
   * @param dtrPin GPIO DTR para sleep (opcional)
   * @return true si se configuró correctamente
   */
  bool begin(HardwareSerial* serial = &Serial1,
             int rxPin = KISS_LTE_RX_PIN,
             int txPin = KISS_LTE_TX_PIN,
             int pwrKeyPin = KISS_LTE_PWRKEY_PIN,
             int statusPin = KISS_LTE_STATUS_PIN,
             int resetPin = KISS_LTE_RESET_PIN,
             int dtrPin = KISS_LTE_DTR_PIN);

  /**
   * @brief Limpia recursos y apaga módulo
   */
  void end();

  // ========== CONTROL DE HARDWARE ==========
  /**
   * @brief Enciende módulo y auto-detecta tipo (SIMCOM/Quectel)
   * @return true si encendió y detectó correctamente
   */
  bool powerOn();

  /**
   * @brief Apaga módulo físicamente (pulso PWRKEY + verificación STATUS)
   * @return true si se apagó correctamente
   */
  bool powerOff();

  /**
   * @brief Reset completo del módulo
   * @return true si se reseteó correctamente
   */
  bool hardReset();

  /**
   * @brief Verifica si el módulo está encendido físicamente
   * @return true si STATUS_PIN = HIGH
   */
  bool isHardwarePowered();

  /**
   * @brief Verifica si el hardware LTE está disponible
   * @return true si el módulo respondió alguna vez
   */
  bool isHardwareAvailable() const;

  // ========== LOOP Y MANTENIMIENTO ==========
  /**
   * @brief Loop principal - procesa URCs y mantiene conexión
   * Llamar desde Arduino loop()
   */
  void loop();

  /**
   * @brief Health check del módulo
   * @return true si el módulo responde
   */
  bool healthCheck();

  /**
   * @brief Asegura que la conexión está activa
   * @return true si conectado o reconectó exitosamente
   */
  bool ensureConnected();

  // ========== MÉTODOS VIRTUALES KissClient ==========
  bool connectToTelegram() override;
  bool connect(const char* host, uint16_t port) override;
  bool connected() override;
  bool isConnected() override;
  void disconnect() override;
  void stop() override;

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
  void printInfo() override;

  // ========== POWER MANAGEMENT ==========
  bool setPowerMode(KissClientPowerMode mode) override;
  KissClientPowerMode getPowerMode() override;

  // ========== INFORMACIÓN ==========
  const char* getClientType() override;
  int getSignalStrength() override;
  const char* getConnectionInfo() override;

  // ========== DIAGNÓSTICOS ==========
  bool isConnectionHealthy() override;
  unsigned long getConnectionAge() override;
  int getErrorCount() override;
  void resetErrorCount() override;

  // ========== MÉTODOS ESPECÍFICOS LTE ==========
  ConnectionState getState() { return _state; }
  const char* getStateString();
  const char* getModuleName();

  /**
   * @brief Configura APN para datos móviles
   */
  bool setAPN(const char* apn, const char* user = "", const char* pass = "");

  /**
   * @brief Obtiene IMEI del módulo
   */
  bool getIMEI(char* imei, size_t size);

  /**
   * @brief Obtiene operador de red
   */
  bool getOperator(char* operatorName, size_t size);

  // ========== GNSS (si el módulo lo soporta) ==========
  bool initGNSS();
  bool getGNSSLocation(float* lat, float* lon, float* alt = nullptr);
  void stopGNSS();

  // ========== CONFIGURACIÓN ==========
  void setDebug(bool enable);

private:
  KissLTE();
  ~KissLTE();

  KissLTE(const KissLTE&) = delete;
  KissLTE& operator=(const KissLTE&) = delete;

  // ========== HARDWARE ==========
  HardwareSerial* _serial;
  int _rxPin;
  int _txPin;
  int _pwrKeyPin;
  int _statusPin;
  int _resetPin;
  int _dtrPin;

  // ========== MÓDULO DINÁMICO ==========
  KissLTEModule* _module = nullptr;  // Puntero al módulo específico (Quectel o SIMCOM)
  bool _moduleDetected = false;

  // ========== ESTADO ==========
  ConnectionState _state;
  bool _initialized;
  bool _debug;
  bool _hardwareAvailable;  // true si el módulo respondió alguna vez
  int _consecutiveFailures;  // Contador de fallos consecutivos
  static const int MAX_CONSECUTIVE_FAILURES = 3;  // Máximo de fallos antes de deshabilitar

  // ========== CONFIGURACIÓN RED ==========
  char _apn[64];
  char _apnUser[32];
  char _apnPass[32];

  // ========== NVS ==========
  Preferences _nvs;

  // ========== MÉTODOS PRIVADOS ==========
  /**
   * @brief Detecta tipo de módulo (SIMCOM o Quectel)
   * @return true si se detectó correctamente
   */
  bool detectModule();

  /**
   * @brief Crea instancia del módulo específico
   */
  bool createModule();

  /**
   * @brief Libera módulo actual
   */
  void destroyModule();

  /**
   * @brief Carga credenciales desde NVS
   */
  bool loadCredentialsFromNVS();

  /**
   * @brief Guarda credenciales en NVS
   */
  bool saveCredentialsToNVS();

  /**
   * @brief Actualiza estado interno
   */
  void setState(ConnectionState newState);
};

#endif // KISS_LTE_H
