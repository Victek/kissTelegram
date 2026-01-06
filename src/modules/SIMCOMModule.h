// SIMCOMModule.h
// Vicente Soriano - victek@gmail.com
// Implementación específica para módulos SIMCOM (A7672E, A7682E, etc.)

#ifndef SIMCOM_MODULE_H
#define SIMCOM_MODULE_H

#include "KissLTEModule.h"

/**
 * @brief Implementación para módulos SIMCOM
 *
 * Soporta:
 * - SIMCOM A7672E (Cat-1 LTE, SSL nativo)
 * - SIMCOM A7682E (similar)
 *
 * Comandos AT específicos:
 * - SSL: AT+CSSL*
 * - Network: AT+NETOPEN/AT+NETCLOSE
 * - Power: AT+CPOF
 * - Sleep: AT+CSCLK (problemático, usar con precaución)
 */
class SIMCOMModule : public KissLTEModule {
public:
    SIMCOMModule(HardwareSerial* serial, int pwrKeyPin, int statusPin = -1, int resetPin = -1, int dtrPin = -1);
    ~SIMCOMModule() override;

    // ========== IDENTIFICACIÓN ==========
    ModuleType getType() const override { return MODULE_SIMCOM; }
    bool detect() override;
    const char* getModuleName() override { return "SIMCOM A7672E"; }
    const char* getManufacturer() override { return "SIMCOM"; }

    // ========== CONTROL DE HARDWARE ==========
    bool powerOn() override;
    bool powerOff() override;
    bool hardReset() override;
    unsigned long getPowerOnPulseDuration() override { return 1200; }   // 1.2s HIGH
    unsigned long getPowerOffPulseDuration() override { return 2500; }  // 2.5s HIGH

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

    // ========== ESPECÍFICO SIMCOM - GNSS ==========
    bool enableGNSS();
    bool getGNSSLocation(float* lat, float* lon, float* alt = nullptr);
    void disableGNSS();

private:
    int _statusPin;
    int _resetPin;
    bool _sslConnected = false;
    bool _netOpen = false;

    // Caché para optimizar consultas NETOPEN
    bool _lastNetOpenState = false;
    unsigned long _lastNetOpenCheck = 0;
    static const unsigned long NETOPEN_CACHE_MS = 5000;

    // Callbacks para URCs
    void handleCARecvURC(int cid, int len);
    void handleCAStateURC(int cid, int state);
    void handleNetCloseURC(int result);

    // Helpers
    bool waitForNetworkRegistration(unsigned long timeout = 30000);
    bool setClockTime();  // Workaround para certificados SSL
};

#endif // SIMCOM_MODULE_H
