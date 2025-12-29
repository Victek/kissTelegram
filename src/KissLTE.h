// KissLTE.h
// Vicente Soriano - victek@gmail.com
// Sistema de conexión LTE con módulo Quectel EC200A-EU-HA
// SSL/TLS nativo, Cat-1 LTE, comandos AT+QSSL*
// GNSS integrado (GPS/Galileo/GLONASS/BeiDou)

#ifndef KISS_LTE_H
#define KISS_LTE_H

#include "kiss_setup.h"
#include "KissClient.h"
#include <HardwareSerial.h>
#include <Preferences.h>

class KissLTE : public KissClient {
public:
  // Singleton pattern
  static KissLTE& getInstance() {
    static KissLTE instance;
    return instance;
  }

  // Estados de conexión
  enum ConnectionState {
    STATE_OFF,
    STATE_POWERING_ON,
    STATE_INITIALIZING,
    STATE_REGISTERING,
    STATE_CONNECTED,
    STATE_ERROR
  };

  // Calidad de señal
  enum SignalQuality {
    SIGNAL_NONE,
    SIGNAL_POOR,
    SIGNAL_FAIR,
    SIGNAL_GOOD,
    SIGNAL_EXCELLENT
  };

  // ========== INICIALIZACIÓN Y CONTROL ==========
  bool begin(HardwareSerial* serial = &Serial1,
             int rxPin = KISS_LTE_RX_PIN,
             int txPin = KISS_LTE_TX_PIN,
             int pwrKeyPin = KISS_LTE_PWRKEY_PIN,
             int dtrPin = -1);

  void end();
  bool powerOn();
  bool powerOff();
  bool reset();

  // ========== LOOP PRINCIPAL - LLAMAR EN loop() ==========
  void loop();                        // Procesa URCs, keepalive, health check

  // ========== WATCHDOG Y RECOVERY ==========
  bool softReset();
  bool hardReset();
  bool recoveryReset();

  bool healthCheck();
  void updateHealthStatus();
  bool isHealthy();
  unsigned long getLastSuccessfulComm();
  void resetHealthMonitor();

  // ========== URCs Y RECONEXIÓN ==========
  void processURCs();                 // Procesar mensajes no solicitados
  bool ensureConnected();             // Verificar y reconectar si necesario
  bool checkAndRecover();             // Recovery completo

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

  // ========== MÉTODOS ESPECÍFICOS LTE ==========
  bool connect();
  ConnectionState getState();

  // ========== COMANDOS AT BÁSICOS ==========
  bool sendATCommand(const char* cmd, const char* expectedResponse = "OK",
                     unsigned long timeout = 1000);
  bool sendATCommand(const char* cmd, char* response, size_t responseSize,
                     unsigned long timeout = 1000);

  // ========== CONFIGURACIÓN RED ==========
  bool setAPN(const char* apn, const char* user = "", const char* pass = "");
  bool setPIN(const char* pin);
  bool checkPIN();
  bool unlockSIM();

  // ========== INFORMACIÓN MÓDULO ==========
  bool getIMEI(char* imei, size_t size);
  bool getICCID(char* iccid, size_t size);
  bool getOperator(char* operatorName, size_t size);

  // ========== CALIDAD SEÑAL ==========
  SignalQuality getSignalQuality();
  bool checkNetworkRegistration();

  // ========== TCP/IP ==========
  bool openTCPConnection(const char* host, int port);
  bool closeTCPConnection();
  bool isTCPConnected();

  int tcpWrite(const uint8_t* data, size_t len);
  int tcpRead(uint8_t* buffer, size_t len);
  int tcpAvailable();

  // ========== SSL MEJORADO ==========
  bool openSSLConnectionWithRetry(const char* host, int port, int maxRetries = 3);
  bool closeSSLConnectionFast();
  bool querySSLState();
  void sendKeepalive();

  // ========== DIAGNÓSTICOS SSL ==========
  void logLastError();
  void logSSLError(int err);

  // ========== GNSS (GPS/Galileo/GLONASS/BeiDou) ==========
  bool initGNSS();
  bool getGNSSLocation(float* lat, float* lon, float* alt = nullptr, float* speed = nullptr);
  void stopGNSS();
  bool enableAGPS();

  // ========== DIAGNÓSTICOS LTE ==========
  void printStatus();
  void printModuleInfo();
  const char* getStateString();

  // ========== CONFIGURACIÓN ==========
  void setDebug(bool enable);
  void setTimeout(unsigned long timeout);
  unsigned long getTimeout();

  // ========== ESTADÍSTICAS SLEEP/ACTIVO ==========
  unsigned long getTimeInSleepMode();
  unsigned long getTimeInActiveMode();
  unsigned long getCurrentModeTime();
  float getSleepEfficiency();
  void resetPowerStatistics();

  // ========== HELPERS ==========
  bool waitForResponse(const char* expected, unsigned long timeout = 1000);
  void clearSerialBuffer();
  void keepalive();

private:
  KissLTE();
  ~KissLTE();

  KissLTE(const KissLTE&) = delete;
  KissLTE& operator=(const KissLTE&) = delete;

  // ========== VARIABLES HARDWARE ==========
  HardwareSerial* _serial;
  int _rxPin;
  int _txPin;
  int _pwrKeyPin;
  int _dtrPin;

  // ========== VARIABLES ESTADO ==========
  ConnectionState _state;
  bool _initialized;
  bool _powered;
  bool _debug;
  unsigned long _timeout;
  unsigned long _lastActivityTime;

  // ========== VARIABLES SEGURIDAD ==========
  bool _secureMode;

  // ========== VARIABLES RED ==========
  char _apn[64];
  char _apnUser[32];
  char _apnPass[32];
  char _pin[8];
  bool _pinRequired;
  bool _tcpConnected;
  bool _pdpDeactivated;

  // ========== VARIABLES INFORMACIÓN ==========
  char _imei[16];
  char _iccid[24];
  char _operator[32];
  int _signalStrength;
  char _moduleModel[32];

  // ========== SSL/TLS QUECTEL ==========
  int _sslContextId;
  bool _sslConfigured;
  bool _sslConnected;

  // ========== VARIABLES WATCHDOG ==========
  unsigned long _lastSuccessfulComm;
  unsigned long _lastHealthCheck;
  int _healthCheckFailures;
  static const int MAX_HEALTH_FAILURES = 3;
  unsigned long _lastKeepalive;
  static const unsigned long KEEPALIVE_INTERVAL = 300000;  // 5 min

  // ========== ESTADÍSTICAS POWER ==========
  unsigned long _timeInSleep;
  unsigned long _timeInActive;
  unsigned long _lastModeChange;
  bool _isInSleepMode;

  // ========== BUFFER COMUNICACIÓN ==========
  static const size_t BUFFER_SIZE = 1024;
  char _buffer[BUFFER_SIZE];

  // ========== NVS ==========
  Preferences _nvs;

  // ========== MÉTODOS PRIVADOS ==========
  bool pulsePoweKey();
  bool waitForModuleReady(unsigned long timeout = 10000);
  bool initializeModule();
  bool configureNetwork();
  bool activatePDP();

  bool loadCredentialsFromNVS();
  bool saveCredentialsToNVS();

  void setState(ConnectionState newState);
  void updateSignalStrength();

  bool enterSleepMode(int mode);
  bool setFunctionalityLevel(int level);
  bool powerDownModule();

  void updatePowerStatistics();

  void setDTR(bool active);
  void wakeFromSleep();

  // Helpers AT
  int readLine(char* buffer, size_t size, unsigned long timeout);
  bool expectResponse(const char* expected, unsigned long timeout = 1000);
  void flushSerial();
  bool readATResponse(const char* cmd, char* response, size_t responseSize, unsigned long timeout);

  bool detectModule();
  const char* getModuleName();

  // ========== SSL/TLS NATIVO ==========
  bool configureSSL();
  bool openSSLConnection(const char* host, int port);
  bool closeSSLConnection();
  int sslWrite(const uint8_t* data, size_t len);
  int sslRead(uint8_t* buffer, size_t len);
  int sslAvailable();

  // ========== URC HANDLERS ==========
  void handleSSLRecvURC(int clientID);
  void handleSSLClosedURC(int clientID);
  void handlePDPDeactURC(int contextID);

  // ========== GNSS HELPERS ==========
  float nmeaToDecimal(const char* nmea);
};

#endif // KISS_LTE_H