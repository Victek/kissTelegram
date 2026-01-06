// KissNet.cpp
// Vicente Soriano - victek@gmail.com
// Implementación del gestor principal de conectividad

#include "KissNet.h"
#include "KissLTE.h"
#include "KissSSL.h"
#include "KissCredentials.h"

// ========== CONSTRUCTOR/DESTRUCTOR ==========

KissNet::KissNet()
    : _activeNetwork(NET_NONE),
      _mode(MODE_AUTO),
      _initialized(false),
      _wifiClient(nullptr),
      _lteClient(nullptr),
      _activeClient(nullptr),
      _lastConnectionCheck(0),
      _wifiTimeout(KISS_WIFI_TIMEOUT_MS),
      _lteInitialized(false) {

    memset(_ssid, 0, sizeof(_ssid));
    memset(_password, 0, sizeof(_password));
}

KissNet::~KissNet() {
    end();
}

// ========== INICIALIZACIÓN ==========

bool KissNet::begin(const char* ssid, const char* password, NetworkMode mode) {
    if (_initialized) {
        KISS_LOG("KissNet ya inicializado");
        return true;
    }

    KISS_LOG("Iniciando KissNet...");

    _mode = mode;

    // Guardar credenciales WiFi si se proporcionan
    if (ssid && strlen(ssid) > 0) {
        strncpy(_ssid, ssid, sizeof(_ssid) - 1);
    }
    if (password && strlen(password) > 0) {
        strncpy(_password, password, sizeof(_password) - 1);
    }

    // Si no se proporcionaron credenciales, intentar cargarlas de KissCredentials
    if (strlen(_ssid) == 0) {
        KISS_LOG("Cargando credenciales WiFi desde KissCredentials...");
        // Obtener credenciales globales (asumiendo que están disponibles)
        extern KissCredentials credentials;
        const char* storedSSID = credentials.getWifiSSID();
        const char* storedPass = credentials.getWifiPassword();

        if (storedSSID && strlen(storedSSID) > 0) {
            strncpy(_ssid, storedSSID, sizeof(_ssid) - 1);
            if (storedPass) {
                strncpy(_password, storedPass, sizeof(_password) - 1);
            }
            KISS_LOGF("✓ Credenciales WiFi cargadas: %s", _ssid);
        } else {
            KISS_LOG("⚠ No hay credenciales WiFi disponibles");
        }
    }

    // Inicializar LTE hardware (sin encender módulo)
    KissLTE& lte = KissLTE::getInstance();
    if (!lte.begin()) {
        KISS_LOG("⚠ Error preparando hardware LTE");
    }

    _initialized = true;

    // Intentar conectar según el modo
    bool connected = false;

    switch (_mode) {
        case MODE_AUTO:
            // Intentar WiFi primero
            if (tryWiFi()) {
                connected = true;
            } else {
                // WiFi falló, intentar LTE
                KISS_LOG("WiFi no disponible, activando LTE...");
                connected = tryLTE();
            }
            break;

        case MODE_WIFI_ONLY:
            connected = tryWiFi();
            break;

        case MODE_LTE_ONLY:
            connected = tryLTE();
            break;

        default:
            KISS_LOG("Modo no soportado");
            break;
    }

    if (connected) {
        KISS_LOGF("✓ KissNet inicializado: %s", getConnectionInfo().c_str());
    } else {
        KISS_LOG("⚠ KissNet inicializado sin conectividad");
    }

    return _initialized;
}

void KissNet::end() {
    if (!_initialized) return;

    KISS_LOG("Deteniendo KissNet...");

    // Apagar todas las conexiones
    shutdownWiFi();
    shutdownLTE();

    _activeClient = nullptr;
    _activeNetwork = NET_NONE;
    _initialized = false;

    KISS_LOG("KissNet detenido");
}

// ========== LOOP ==========

void KissNet::loop() {
    if (!_initialized) return;

    // Verificar conexión cada 30 segundos
    unsigned long now = millis();
    if (now - _lastConnectionCheck > 30000) {
        checkConnection();
        _lastConnectionCheck = now;
    }

    // Loop del cliente activo
    if (_activeClient) {
        // Si es LTE, llamar a su loop para procesar URCs
        if (_activeNetwork == NET_LTE) {
            KissLTE& lte = KissLTE::getInstance();
            lte.loop();
        }
    }
}

// ========== ACCESO A CLIENTE ==========

KissClient* KissNet::getActiveClient() {
    return _activeClient;
}

bool KissNet::isConnected() {
    if (!_activeClient) return false;

    switch (_activeNetwork) {
        case NET_WIFI:
            return (WiFi.status() == WL_CONNECTED);

        case NET_LTE:
            return _activeClient->isConnected();

        default:
            return false;
    }
}

// ========== CAMBIO DE MODO ==========

bool KissNet::setMode(NetworkMode mode) {
    if (_mode == mode) {
        KISS_LOG("Modo ya activo");
        return true;
    }

    KISS_LOGF("Cambiando modo: %d → %d", _mode, mode);
    _mode = mode;

    // Aplicar el nuevo modo
    switch (_mode) {
        case MODE_AUTO:
            return switchToAuto();

        case MODE_WIFI_ONLY:
            return switchToWiFi();

        case MODE_LTE_ONLY:
            return switchToLTE();

        default:
            return false;
    }
}

bool KissNet::switchToLTE() {
    KISS_LOG("Cambiando a LTE...");

    // Apagar WiFi para ahorrar energía
    shutdownWiFi();

    // Activar LTE
    if (tryLTE()) {
        _mode = MODE_LTE_ONLY;
        KISS_LOG("✓ Cambiado a LTE");
        return true;
    }

    KISS_LOG("✗ No se pudo activar LTE");
    return false;
}

bool KissNet::switchToWiFi() {
    KISS_LOG("Cambiando a WiFi...");

    // Apagar LTE para ahorrar energía
    shutdownLTE();

    // Activar WiFi
    if (tryWiFi()) {
        _mode = MODE_WIFI_ONLY;
        KISS_LOG("✓ Cambiado a WiFi");
        return true;
    }

    KISS_LOG("✗ No se pudo activar WiFi");
    return false;
}

bool KissNet::switchToAuto() {
    KISS_LOG("Cambiando a modo automático...");

    _mode = MODE_AUTO;

    // Intentar WiFi primero
    if (tryWiFi()) {
        // Si hay LTE activo, apagarlo
        if (_activeNetwork == NET_LTE) {
            shutdownLTE();
        }
        return true;
    }

    // WiFi no disponible, usar LTE
    return tryLTE();
}

// ========== INFORMACIÓN ==========

String KissNet::getConnectionInfo() {
    String info = "";

    switch (_activeNetwork) {
        case NET_WIFI:
            info = "WiFi (";
            info += WiFi.SSID();
            info += ", ";
            info += WiFi.RSSI();
            info += " dBm)";
            break;

        case NET_LTE:
            if (_activeClient) {
                info = "LTE (";
                info += _activeClient->getClientType();
                info += ", ";
                info += _activeClient->getSignalStrength();
                info += "/31)";
            } else {
                info = "LTE (desconectado)";
            }
            break;

        case NET_LORA:
            info = "LoRa (no implementado)";
            break;

        default:
            info = "Sin conexión";
            break;
    }

    return info;
}

int KissNet::getSignalStrength() {
    switch (_activeNetwork) {
        case NET_WIFI:
            return WiFi.RSSI();

        case NET_LTE:
            if (_activeClient) {
                return _activeClient->getSignalStrength();
            }
            return -1;

        default:
            return -999;
    }
}

bool KissNet::ensureConnection() {
    if (isConnected()) {
        return true;
    }

    KISS_LOG("Reconectando...");

    switch (_mode) {
        case MODE_AUTO:
            // Intentar WiFi primero, luego LTE
            return tryWiFi() || tryLTE();

        case MODE_WIFI_ONLY:
            return tryWiFi();

        case MODE_LTE_ONLY:
            return tryLTE();

        default:
            return false;
    }
}

// ========== MÉTODOS PRIVADOS - WIFI ==========

bool KissNet::tryWiFi() {
    // Si ya estamos en WiFi y conectados, no hacer nada
    if (_activeNetwork == NET_WIFI && WiFi.status() == WL_CONNECTED) {
        return true;
    }

    // Verificar si tenemos credenciales
    if (strlen(_ssid) == 0) {
        KISS_LOG("✗ Sin credenciales WiFi");
        return false;
    }

    KISS_LOGF("Conectando WiFi: %s", _ssid);

    // Si ya está conectado a otra red, desconectar
    if (WiFi.status() == WL_CONNECTED) {
        WiFi.disconnect();
        delay(100);
    }

    // Intentar conectar
    WiFi.begin(_ssid, _password);

    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED) {
        if (millis() - start > _wifiTimeout) {
            KISS_LOG("✗ WiFi timeout");
            WiFi.disconnect();
            return false;
        }
        delay(100);
    }

    KISS_LOGF("✓ WiFi conectado: %s (%d dBm)",
             WiFi.localIP().toString().c_str(), WiFi.RSSI());

    // Crear cliente WiFi si no existe
    if (!_wifiClient) {
        _wifiClient = new KissSSL();
    }

    // Cambiar a WiFi
    switchClient(_wifiClient, NET_WIFI);

    return true;
}

void KissNet::shutdownWiFi() {
    if (_activeNetwork == NET_WIFI) {
        _activeClient = nullptr;
        _activeNetwork = NET_NONE;
    }

    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);

    if (_wifiClient) {
        delete _wifiClient;
        _wifiClient = nullptr;
    }

    KISS_LOG("WiFi apagado");
}

// ========== MÉTODOS PRIVADOS - LTE ==========

bool KissNet::tryLTE() {
    KissLTE& lte = KissLTE::getInstance();

    // Si LTE ya está conectado, usar
    if (_activeNetwork == NET_LTE && lte.isConnected()) {
        return true;
    }

    // Inicializar LTE si es la primera vez
    if (!_lteInitialized) {
        if (!initLTE()) {
            return false;
        }
    }

    // Verificar si el hardware está disponible (no deshabilitado por fallos)
    if (!lte.isHardwareAvailable() && !lte.isHardwarePowered()) {
        // Primera vez: intentar encender
        KISS_LOG("Iniciando detección de hardware LTE...");
        if (!lte.powerOn()) {
            KISS_LOG("✗ Hardware LTE no detectado o deshabilitado");
            return false;
        }
    }

    // Verificar si ya está encendido
    if (!lte.isHardwarePowered()) {
        KISS_LOG("Encendiendo módulo LTE...");
        if (!lte.powerOn()) {
            KISS_LOG("✗ No se pudo encender LTE");
            return false;
        }
    }

    // Asegurar conexión PDP
    if (!lte.ensureConnected()) {
        KISS_LOG("✗ No se pudo conectar a red LTE");
        return false;
    }

    KISS_LOGF("✓ LTE conectado: %s", lte.getConnectionInfo());

    // Cambiar a LTE
    switchClient(&lte, NET_LTE);

    return true;
}

bool KissNet::initLTE() {
    if (_lteInitialized) {
        return true;
    }

    KISS_LOG("Inicializando LTE por primera vez...");

    KissLTE& lte = KissLTE::getInstance();

    // begin() ya se llamó en KissNet::begin()
    // Solo marcar como inicializado
    _lteInitialized = true;
    _lteClient = &lte;

    KISS_LOG("✓ LTE inicializado (lazy)");
    return true;
}

void KissNet::shutdownLTE() {
    if (_activeNetwork == NET_LTE) {
        _activeClient = nullptr;
        _activeNetwork = NET_NONE;
    }

    if (_lteInitialized) {
        KissLTE& lte = KissLTE::getInstance();
        lte.powerOff();  // Apaga físicamente el módulo
        KISS_LOG("LTE apagado");
    }
}

// ========== GESTIÓN DE CONEXIÓN ==========

void KissNet::checkConnection() {
    if (!_initialized) return;

    // Verificar estado de la conexión actual
    bool connected = isConnected();

    if (connected) {
        // Todo OK
        return;
    }

    KISS_LOG("⚠ Conexión perdida, intentando reconectar...");

    // Intentar reconectar según el modo
    switch (_mode) {
        case MODE_AUTO:
            // Intentar WiFi primero
            if (tryWiFi()) {
                KISS_LOG("✓ Reconectado a WiFi");
            } else if (tryLTE()) {
                KISS_LOG("✓ Failover a LTE");
            } else {
                KISS_LOG("✗ Sin conectividad");
            }
            break;

        case MODE_WIFI_ONLY:
            if (tryWiFi()) {
                KISS_LOG("✓ Reconectado a WiFi");
            }
            break;

        case MODE_LTE_ONLY:
            if (tryLTE()) {
                KISS_LOG("✓ Reconectado a LTE");
            }
            break;

        default:
            break;
    }
}

void KissNet::switchClient(KissClient* client, NetworkType type) {
    if (_activeClient != client) {
        _activeClient = client;
        _activeNetwork = type;

        KISS_LOGF("→ Cliente activo: %s",
                 type == NET_WIFI ? "WiFi" :
                 type == NET_LTE ? "LTE" : "Ninguno");
    }
}
