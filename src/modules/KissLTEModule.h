// KissLTEModule.h
// Vicente Soriano - victek@gmail.com
// Clase base abstracta para módulos LTE (Quectel, SIMCOM, etc.)
// Permite soportar múltiples fabricantes con la misma interfaz

#ifndef KISS_LTE_MODULE_H
#define KISS_LTE_MODULE_H

#include <Arduino.h>
#include <HardwareSerial.h>

/**
 * @brief Clase base abstracta para módulos LTE
 *
 * Cada fabricante (Quectel, SIMCOM, u-blox, etc.) implementa sus propios
 * comandos AT para SSL, PDP, power management, etc.
 *
 * Esta clase define la interfaz común que KissLTE usará de forma polimórfica.
 */
// Enumeración para tipos de módulos (sin RTTI)
enum ModuleType {
    MODULE_UNKNOWN = 0,
    MODULE_QUECTEL = 1,
    MODULE_SIMCOM = 2
};

class KissLTEModule {
public:
    virtual ~KissLTEModule() {}

    // ========== IDENTIFICACIÓN ==========
    /**
     * @brief Obtiene el tipo de módulo (para casting sin RTTI)
     */
    virtual ModuleType getType() const = 0;

    /**
     * @brief Detecta si este módulo responde a los comandos AT
     * @return true si el módulo se identifica correctamente
     */
    virtual bool detect() = 0;

    /**
     * @brief Obtiene el nombre del módulo (ej: "SIMCOM A7672E", "Quectel EC200A")
     */
    virtual const char* getModuleName() = 0;

    /**
     * @brief Obtiene el fabricante (ej: "SIMCOM", "Quectel")
     */
    virtual const char* getManufacturer() = 0;

    // ========== CONTROL DE HARDWARE ==========
    /**
     * @brief Secuencia de encendido físico (pulso PWRKEY)
     * @return true si el módulo enciende correctamente
     */
    virtual bool powerOn() = 0;

    /**
     * @brief Secuencia de apagado físico (pulso PWRKEY largo)
     * @return true si el módulo se apaga correctamente
     */
    virtual bool powerOff() = 0;

    /**
     * @brief Reset por hardware (si está disponible)
     */
    virtual bool hardReset() = 0;

    /**
     * @brief Duración del pulso de encendido en milisegundos
     */
    virtual unsigned long getPowerOnPulseDuration() = 0;

    /**
     * @brief Duración del pulso de apagado en milisegundos
     */
    virtual unsigned long getPowerOffPulseDuration() = 0;

    // ========== INICIALIZACIÓN Y CONFIGURACIÓN ==========
    /**
     * @brief Inicializa el módulo (configuración básica AT)
     * @return true si la inicialización es exitosa
     */
    virtual bool initialize() = 0;

    /**
     * @brief Configura el APN para datos móviles
     */
    virtual bool configureAPN(const char* apn, const char* user = "", const char* pass = "") = 0;

    /**
     * @brief Activa el contexto PDP (Packet Data Protocol)
     */
    virtual bool activatePDP() = 0;

    /**
     * @brief Desactiva el contexto PDP
     */
    virtual bool deactivatePDP() = 0;

    /**
     * @brief Verifica si el PDP está activo
     */
    virtual bool isPDPActive() = 0;

    // ========== SSL/TLS ==========
    /**
     * @brief Configura SSL/TLS (versión, ciphers, nivel de seguridad)
     */
    virtual bool configureSSL() = 0;

    /**
     * @brief Carga certificado CA raíz
     * @param certPEM Certificado en formato PEM
     * @return true si se carga correctamente
     */
    virtual bool uploadCACertificate(const char* certPEM) = 0;

    /**
     * @brief Abre conexión SSL
     */
    virtual bool openSSLConnection(const char* host, int port) = 0;

    /**
     * @brief Cierra conexión SSL
     */
    virtual bool closeSSLConnection() = 0;

    /**
     * @brief Verifica si la conexión SSL está activa
     */
    virtual bool isSSLConnected() = 0;

    /**
     * @brief Envía datos por SSL
     */
    virtual int sslWrite(const uint8_t* data, size_t len) = 0;

    /**
     * @brief Lee datos de SSL
     */
    virtual int sslRead(uint8_t* buffer, size_t len) = 0;

    /**
     * @brief Bytes disponibles en buffer SSL
     */
    virtual int sslAvailable() = 0;

    // ========== POWER MANAGEMENT ==========
    /**
     * @brief Entra en modo sleep
     * @param mode Nivel de sleep (específico de cada módulo)
     */
    virtual bool enterSleepMode(int mode) = 0;

    /**
     * @brief Sale del modo sleep
     */
    virtual bool wakeUp() = 0;

    /**
     * @brief Configura DTR para sleep automático
     */
    virtual bool configureDTRSleep(bool enable) = 0;

    // ========== INFORMACIÓN Y DIAGNÓSTICOS ==========
    /**
     * @brief Obtiene la intensidad de señal (RSSI)
     * @return Valor de señal (0-31, 99=desconocido)
     */
    virtual int getSignalStrength() = 0;

    /**
     * @brief Verifica el registro en la red
     * @return true si está registrado
     */
    virtual bool isNetworkRegistered() = 0;

    /**
     * @brief Obtiene el operador de red
     */
    virtual bool getOperator(char* operatorName, size_t size) = 0;

    /**
     * @brief Obtiene el IMEI del módulo
     */
    virtual bool getIMEI(char* imei, size_t size) = 0;

    // ========== URCs (Unsolicited Result Codes) ==========
    /**
     * @brief Procesa mensajes URC del módulo
     * @param line Línea recibida del puerto serial
     *
     * Ejemplos:
     * - Quectel: "+QSSLURC: \"recv\",0", "+QIURC: \"pdpdeact\",1"
     * - SIMCOM: "+CARECV: 0,512", "+NETCLOSE: 0"
     */
    virtual void processURC(const String& line) = 0;

    // ========== HELPERS COMUNES ==========
    /**
     * @brief Envía comando AT y espera respuesta
     */
    bool sendATCommand(const char* cmd, const char* expected = "OK", unsigned long timeout = 2000);

    /**
     * @brief Envía comando AT y captura la respuesta
     */
    bool sendATCommand(const char* cmd, char* response, size_t responseSize, unsigned long timeout = 2000);

    /**
     * @brief Lee respuesta del módulo
     */
    String readResponse(unsigned long timeout = 2000);

    /**
     * @brief Limpia el buffer serial
     */
    void clearSerialBuffer();

    /**
     * @brief Espera respuesta específica
     */
    bool waitForResponse(const char* expected, unsigned long timeout = 2000);

protected:
    /**
     * Constructor protegido (solo accesible desde clases derivadas)
     */
    KissLTEModule(HardwareSerial* serial, int pwrKeyPin, int dtrPin = -1)
        : _serial(serial), _pwrKeyPin(pwrKeyPin), _dtrPin(dtrPin) {}

    // Variables comunes a todos los módulos
    HardwareSerial* _serial;
    int _pwrKeyPin;
    int _dtrPin;

    bool _debug = false;
    int _sslContextId = 0;
};

#endif // KISS_LTE_MODULE_H
