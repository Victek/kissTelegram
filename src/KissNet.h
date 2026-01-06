// KissNet.h
// Vicente Soriano - victek@gmail.com
// Gestor principal de conectividad de red para KissTelegram
// Maneja WiFi (prioritario) y LTE (backup automático)

#ifndef KISS_NET_H
#define KISS_NET_H

#include "Kiss_setup.h"
#include "KissClient.h"
#include <WiFi.h>

// Forward declarations
class KissLTE;

/**
 * @brief Gestor principal de conectividad con failover automático
 *
 * Características:
 * - WiFi como primera opción (sin overhead)
 * - LTE como backup automático solo si WiFi falla
 * - Lazy initialization de LTE (ahorro energía)
 * - Switch manual WiFi↔LTE con comando /lte
 * - Power management coordinado
 */
class KissNet {
public:
    // Tipos de red disponibles
    enum NetworkType {
        NET_NONE,      // Sin conectividad
        NET_WIFI,      // WiFi activo
        NET_LTE,       // LTE activo
        NET_LORA       // LoRa (futuro)
    };

    // Modos de operación
    enum NetworkMode {
        MODE_AUTO,      // Automático: WiFi → LTE → LoRa
        MODE_WIFI_ONLY, // Solo WiFi (no usar LTE)
        MODE_LTE_ONLY,  // Solo LTE (apagar WiFi)
        MODE_LORA_ONLY  // Solo LoRa (futuro)
    };

    /**
     * @brief Constructor
     */
    KissNet();

    /**
     * @brief Destructor
     */
    ~KissNet();

    /**
     * @brief Inicializa el gestor de red
     * @param ssid SSID de WiFi
     * @param password Contraseña de WiFi
     * @param mode Modo de operación (por defecto AUTO)
     * @return true si se inicializó correctamente
     */
    bool begin(const char* ssid = nullptr,
               const char* password = nullptr,
               NetworkMode mode = MODE_AUTO);

    /**
     * @brief Detiene todas las conexiones
     */
    void end();

    /**
     * @brief Loop principal - mantiene conectividad
     * Llamar desde Arduino loop()
     */
    void loop();

    /**
     * @brief Obtiene el cliente activo actual
     * @return Puntero a KissClient activo (WiFi o LTE)
     */
    KissClient* getActiveClient();

    /**
     * @brief Verifica si hay conectividad
     * @return true si hay conexión activa
     */
    bool isConnected();

    /**
     * @brief Obtiene el tipo de red activa
     * @return NetworkType actual
     */
    NetworkType getActiveNetwork() { return _activeNetwork; }

    /**
     * @brief Obtiene el modo de operación
     * @return NetworkMode actual
     */
    NetworkMode getMode() { return _mode; }

    /**
     * @brief Cambia el modo de operación
     * @param mode Nuevo modo
     * @return true si cambió correctamente
     */
    bool setMode(NetworkMode mode);

    /**
     * @brief Fuerza cambio a LTE (apaga WiFi)
     * Usado por comando /lte
     * @return true si cambió a LTE
     */
    bool switchToLTE();

    /**
     * @brief Fuerza cambio a WiFi (apaga LTE)
     * @return true si cambió a WiFi
     */
    bool switchToWiFi();

    /**
     * @brief Vuelve al modo automático
     * @return true si activó modo auto
     */
    bool switchToAuto();

    /**
     * @brief Obtiene información de conectividad
     * @return String con info de red activa
     */
    String getConnectionInfo();

    /**
     * @brief Obtiene intensidad de señal
     * @return RSSI de la red activa
     */
    int getSignalStrength();

    /**
     * @brief Asegura que hay conexión activa
     * Intenta reconectar si es necesario
     * @return true si hay conexión
     */
    bool ensureConnection();

private:
    // ========== ESTADO ==========
    NetworkType _activeNetwork;
    NetworkMode _mode;
    bool _initialized;

    // ========== CLIENTES ==========
    KissClient* _wifiClient;
    KissClient* _lteClient;
    KissClient* _activeClient;

    // ========== CREDENCIALES WIFI ==========
    char _ssid[32];
    char _password[64];

    // ========== TIMEOUTS Y CONTROL ==========
    unsigned long _lastConnectionCheck;
    unsigned long _wifiTimeout;
    bool _lteInitialized;

    // ========== MÉTODOS PRIVADOS ==========

    /**
     * @brief Intenta conectar WiFi
     * @return true si conectó
     */
    bool tryWiFi();

    /**
     * @brief Intenta conectar LTE (lazy init)
     * @return true si conectó
     */
    bool tryLTE();

    /**
     * @brief Inicializa LTE por primera vez
     * @return true si inicializó
     */
    bool initLTE();

    /**
     * @brief Apaga WiFi completamente
     */
    void shutdownWiFi();

    /**
     * @brief Apaga LTE completamente
     */
    void shutdownLTE();

    /**
     * @brief Verifica y reconecta si es necesario
     */
    void checkConnection();

    /**
     * @brief Cambia el cliente activo
     * @param client Nuevo cliente activo
     * @param type Tipo de red
     */
    void switchClient(KissClient* client, NetworkType type);
};

#endif // KISS_NET_H
