// QuectelModule.h
// Vicente Soriano - victek@gmail.com
// Implementación específica para módulos Quectel (EC200A, EC200U, etc.)

#ifndef QUECTEL_MODULE_H
#define QUECTEL_MODULE_H

#include "KissLTEModule.h"

/**
 * @brief Implementación para módulos Quectel
 *
 * Soporta:
 * - Quectel EC200A-EU (Cat-1 LTE, SSL nativo)
 * - Quectel EC200U (similar)
 *
 * Comandos AT específicos:
 * - SSL: AT+QSSL*
 * - PDP: AT+QIACT/AT+QIDEACT
 * - Power: AT+QPOWD
 * - Sleep: AT+QSCLK
 */
class QuectelModule : public KissLTEModule {
public:
    QuectelModule(HardwareSerial* serial, int pwrKeyPin, int statusPin = -1, int resetPin = -1, int dtrPin = -1);
    ~QuectelModule() override;

    // ========== IDENTIFICACIÓN ==========
    ModuleType getType() const override { return MODULE_QUECTEL; }
    bool detect() override;
    const char* getModuleName() override { return "Quectel EC200A"; }
    const char* getManufacturer() override { return "Quectel"; }

    // ========== CONTROL DE HARDWARE ==========
    bool powerOn() override;
    bool powerOff() override;
    bool hardReset() override;
    unsigned long getPowerOnPulseDuration() override { return 1500; }   // 1.5s LOW
    unsigned long getPowerOffPulseDuration() override { return 2500; }  // 2.5s LOW

    // ========== INICIALIZACIÓN ==========
    bool initialize() override;
    bool configureAPN(const char* apn, const char* user = "", const char* pass = "") override;
    bool activatePDP() override;
    bool deactivatePDP() override;
    bool isPDPActive() override;

    // ========== SSL/TLS ==========
    bool configureSSL() override;
    bool uploadCACertificate(const char* certPEM) override;
    bool openSSLConnection(const char* host, int port) override;
    bool closeSSLConnection() override;
    bool isSSLConnected() override;
    int sslWrite(const uint8_t* data, size_t len) override;
    int sslRead(uint8_t* buffer, size_t len) override;
    int sslAvailable() override;

    // ========== POWER MANAGEMENT ==========
    bool enterSleepMode(int mode) override;
    bool wakeUp() override;
    bool configureDTRSleep(bool enable) override;

    // ========== INFORMACIÓN ==========
    int getSignalStrength() override;
    bool isNetworkRegistered() override;
    bool getOperator(char* operatorName, size_t size) override;
    bool getIMEI(char* imei, size_t size) override;

    // ========== URCs ==========
    void processURC(const String& line) override;

    // ========== ESPECÍFICO QUECTEL ==========
    bool enableGNSS();
    bool getGNSSLocation(float* lat, float* lon, float* alt = nullptr);
    void disableGNSS();

private:
    int _statusPin;
    int _resetPin;
    bool _sslConnected = false;
    bool _pdpActive = false;

    // Callbacks para URCs
    void handleSSLRecvURC(int clientID);
    void handleSSLClosedURC(int clientID);
    void handlePDPDeactURC(int contextID);

    // Helpers
    bool waitForNetworkRegistration(unsigned long timeout = 30000);
    bool querySSLState();
};

#endif // QUECTEL_MODULE_H
