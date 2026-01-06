// QuectelModule.cpp
// Vicente Soriano - victek@gmail.com
// Implementación específica para módulos Quectel EC200A

#include "QuectelModule.h"
#include "../Kiss_setup.h"

QuectelModule::QuectelModule(HardwareSerial* serial, int pwrKeyPin, int statusPin, int resetPin, int dtrPin)
    : KissLTEModule(serial, pwrKeyPin, dtrPin), _statusPin(statusPin), _resetPin(resetPin) {

    // Configurar pines de control
    if (_statusPin >= 0) {
        pinMode(_statusPin, INPUT);
    }

    if (_resetPin >= 0) {
        pinMode(_resetPin, OUTPUT);
        digitalWrite(_resetPin, HIGH);  // RESET inactivo
    }
}

QuectelModule::~QuectelModule() {
    powerOff();
}

// ========== IDENTIFICACIÓN ==========

bool QuectelModule::detect() {
    // Enviar AT+GMM para obtener modelo
    char response[128];
    if (sendATCommand("AT+GMM", response, sizeof(response), 2000)) {
        if (strstr(response, "Quectel") || strstr(response, "EC200")) {
            KISS_LOG("✓ Módulo Quectel detectado");
            return true;
        }
    }
    return false;
}

// ========== CONTROL DE HARDWARE ==========

bool QuectelModule::powerOn() {
    // Verificar si ya está encendido (vía STATUS_PIN)
    if (_statusPin >= 0) {
        if (digitalRead(_statusPin) == HIGH) {
            KISS_LOG("Módulo Quectel ya encendido (STATUS=HIGH)");
            return true;
        }
    }

    KISS_LOG("Encendiendo módulo Quectel...");

    // QUECTEL: Pulso LOW de 1.5s en PWRKEY
    digitalWrite(_pwrKeyPin, LOW);
    delay(1500);
    digitalWrite(_pwrKeyPin, HIGH);
    delay(2000);  // Esperar arranque

    // Verificar STATUS_PIN si está disponible
    if (_statusPin >= 0) {
        unsigned long start = millis();
        while (millis() - start < 10000) {
            if (digitalRead(_statusPin) == HIGH) {
                KISS_LOG("✓ Módulo encendido (STATUS=HIGH)");
                break;
            }
            delay(100);
        }

        if (digitalRead(_statusPin) == LOW) {
            KISS_LOG("✗ STATUS_PIN no respondió");
            return false;
        }
    }

    // Verificar respuesta AT
    clearSerialBuffer();
    delay(1000);

    for (int i = 0; i < 10; i++) {
        if (sendATCommand("AT", "OK", 1000)) {
            KISS_LOG("✓ Módulo Quectel respondiendo");

            // Desactivar echo
            sendATCommand("ATE0", "OK", 1000);

            return true;
        }
        delay(500);
    }

    KISS_LOG("✗ Módulo no responde a AT");
    return false;
}

bool QuectelModule::powerOff() {
    KISS_LOG("Apagando módulo Quectel...");

    // Cerrar conexión SSL si está activa
    if (_sslConnected) {
        closeSSLConnection();
    }

    // Apagado ordenado por AT
    sendATCommand("AT+QPOWD=1", "OK", 5000);
    delay(3000);

    // Verificar STATUS_PIN
    if (_statusPin >= 0) {
        unsigned long start = millis();
        while (millis() - start < 5000) {
            if (digitalRead(_statusPin) == LOW) {
                KISS_LOG("✓ Módulo apagado (STATUS=LOW)");
                _sslConnected = false;
                _pdpActive = false;
                return true;
            }
            delay(100);
        }

        KISS_LOG("⚠ STATUS_PIN todavía HIGH, forzando apagado...");
        // Pulso largo de PWRKEY (2.5s)
        digitalWrite(_pwrKeyPin, LOW);
        delay(2500);
        digitalWrite(_pwrKeyPin, HIGH);
        delay(2000);

        if (digitalRead(_statusPin) == LOW) {
            KISS_LOG("✓ Apagado forzado exitoso");
            _sslConnected = false;
            _pdpActive = false;
            return true;
        }
    }

    _sslConnected = false;
    _pdpActive = false;
    return true;
}

bool QuectelModule::hardReset() {
    if (_resetPin < 0) {
        KISS_LOG("⚠ RESET_PIN no disponible, usando PWRKEY");
        // Hard reset con PWRKEY (12 segundos)
        digitalWrite(_pwrKeyPin, LOW);
        delay(12000);
        digitalWrite(_pwrKeyPin, HIGH);
        delay(3000);
        return powerOn();
    }

    KISS_LOG("Hard reset por RESET_PIN...");

    // Pulso LOW en RESET_PIN
    digitalWrite(_resetPin, LOW);
    delay(300);
    digitalWrite(_resetPin, HIGH);
    delay(3000);

    return powerOn();
}

// ========== INICIALIZACIÓN ==========

bool QuectelModule::initialize() {
    KISS_LOG("Inicializando módulo Quectel...");

    // Comandos básicos
    if (!sendATCommand("AT", "OK", 1000)) {
        KISS_LOG("✗ Módulo no responde");
        return false;
    }

    // Desactivar echo
    sendATCommand("ATE0", "OK", 1000);

    // Habilitar URCs de red
    sendATCommand("AT+QURCCFG=\"urcport\",\"uartdebug\"", "OK", 1000);

    // Formato de error extendido
    sendATCommand("AT+CMEE=2", "OK", 1000);

    KISS_LOG("✓ Módulo Quectel inicializado");
    return true;
}

bool QuectelModule::configureAPN(const char* apn, const char* user, const char* pass) {
    if (!apn) return false;

    KISS_LOGF("Configurando APN: %s", apn);

    char cmd[128];

    // Configurar APN en contexto 1
    if (strlen(user) > 0 && strlen(pass) > 0) {
        snprintf(cmd, sizeof(cmd), "AT+QICSGP=1,1,\"%s\",\"%s\",\"%s\",1", apn, user, pass);
    } else {
        snprintf(cmd, sizeof(cmd), "AT+QICSGP=1,1,\"%s\",\"\",\"\",1", apn);
    }

    return sendATCommand(cmd, "OK", 3000);
}

bool QuectelModule::activatePDP() {
    KISS_LOG("Activando PDP...");

    char response[256];

    // Verificar si ya está activo
    if (sendATCommand("AT+QIACT?", response, sizeof(response), 2000)) {
        if (strstr(response, "+QIACT: 1,1")) {
            KISS_LOG("✓ PDP ya activo");

            // Extraer IP
            char* ipStart = strchr(response, '"');
            if (ipStart) {
                ipStart++;
                char* ipEnd = strchr(ipStart, '"');
                if (ipEnd) {
                    *ipEnd = '\0';
                    KISS_LOGF("IP: %s", ipStart);
                }
            }
            _pdpActive = true;
            return true;
        }
    }

    // Activar contexto PDP (puede tardar hasta 150s)
    if (!sendATCommand("AT+QIACT=1", "OK", 150000)) {
        KISS_LOG("✗ Error activando PDP");
        return false;
    }

    delay(2000);

    // Verificar IP asignada
    if (sendATCommand("AT+QIACT?", response, sizeof(response), 2000)) {
        char* ipStart = strchr(response, '"');
        if (ipStart) {
            ipStart++;
            char* ipEnd = strchr(ipStart, '"');
            if (ipEnd) {
                *ipEnd = '\0';
                KISS_LOGF("✓ IP asignada: %s", ipStart);
            }
        }
    }

    // Configurar DNS
    sendATCommand("AT+QIDNSCFG=1,\"8.8.8.8\",\"8.8.4.4\"", "OK", 3000);

    _pdpActive = true;
    KISS_LOG("✓ PDP activado");
    return true;
}

bool QuectelModule::deactivatePDP() {
    KISS_LOG("Desactivando PDP...");

    if (sendATCommand("AT+QIDEACT=1", "OK", 40000)) {
        _pdpActive = false;
        KISS_LOG("✓ PDP desactivado");
        return true;
    }

    return false;
}

bool QuectelModule::isPDPActive() {
    char response[128];
    if (sendATCommand("AT+QIACT?", response, sizeof(response), 2000)) {
        return (strstr(response, "+QIACT: 1,1") != nullptr);
    }
    return false;
}

// ========== SSL/TLS ==========

bool QuectelModule::configureSSL() {
    KISS_LOG("Configurando SSL/TLS...");

    char cmd[64];

    // TLS 1.2 (valor 4)
    snprintf(cmd, sizeof(cmd), "AT+QSSLCFG=\"sslversion\",%d,4", _sslContextId);
    if (!sendATCommand(cmd, "OK", 2000)) {
        KISS_LOG("Fallback a TLS 1.1");
        snprintf(cmd, sizeof(cmd), "AT+QSSLCFG=\"sslversion\",%d,3", _sslContextId);
        sendATCommand(cmd, "OK", 2000);
    }

    // Nivel de seguridad 0 (sin validación de certificado)
    snprintf(cmd, sizeof(cmd), "AT+QSSLCFG=\"seclevel\",%d,0", _sslContextId);
    sendATCommand(cmd, "OK", 2000);

    // Cipher suites (todos)
    snprintf(cmd, sizeof(cmd), "AT+QSSLCFG=\"ciphersuite\",%d,0xFFFF", _sslContextId);
    sendATCommand(cmd, "OK", 2000);

    // Ignorar tiempo local
    snprintf(cmd, sizeof(cmd), "AT+QSSLCFG=\"ignorelocaltime\",%d,1", _sslContextId);
    sendATCommand(cmd, "OK", 2000);

    // SNI (crítico para Telegram)
    snprintf(cmd, sizeof(cmd), "AT+QSSLCFG=\"sni\",%d,1", _sslContextId);
    sendATCommand(cmd, "OK", 2000);

    // Timeout de negociación
    snprintf(cmd, sizeof(cmd), "AT+QSSLCFG=\"negotiatetime\",%d,300", _sslContextId);
    sendATCommand(cmd, "OK", 2000);

    KISS_LOG("✓ SSL configurado");
    return true;
}

bool QuectelModule::uploadCACertificate(const char* certPEM) {
    KISS_LOG("Subiendo certificado CA...");

    // TODO: Implementar subida de certificado
    // Quectel requiere guardar el certificado en memoria primero con AT+QFUPL
    // Luego referenciarlo con AT+QSSLCFG="cacert"

    KISS_LOG("⚠ uploadCACertificate no implementado aún");
    return true;  // Por ahora usar seclevel=0
}

bool QuectelModule::openSSLConnection(const char* host, int port) {
    if (_sslConnected) {
        KISS_LOG("✓ SSL ya conectado");
        return true;
    }

    KISS_LOGF("Conectando SSL a %s:%d...", host, port);

    // Asegurar SSL configurado
    configureSSL();

    // Comando QSSLOPEN
    char cmd[128];
    snprintf(cmd, sizeof(cmd), "AT+QSSLOPEN=1,%d,0,\"%s\",%d,0", _sslContextId, host, port);

    if (!sendATCommand(cmd, "OK", 5000)) {
        KISS_LOG("✗ Error en QSSLOPEN");
        return false;
    }

    // Esperar URC: +QSSLOPEN: 0,0
    char response[64];
    unsigned long start = millis();

    while (millis() - start < 30000) {
        if (_serial->available()) {
            String line = readResponse(1000);

            if (line.indexOf("+QSSLOPEN:") >= 0) {
                int clientID, result;
                if (sscanf(line.c_str(), "+QSSLOPEN: %d,%d", &clientID, &result) == 2) {
                    if (result == 0) {
                        _sslConnected = true;
                        KISS_LOG("✓ SSL conectado");
                        return true;
                    } else {
                        KISS_LOGF("✗ Error SSL código %d", result);
                        return false;
                    }
                }
            }
        }
        delay(100);
    }

    KISS_LOG("✗ Timeout SSL");
    return false;
}

bool QuectelModule::closeSSLConnection() {
    if (!_sslConnected) return true;

    KISS_LOG("Cerrando SSL...");

    char cmd[32];
    snprintf(cmd, sizeof(cmd), "AT+QSSLCLOSE=%d", _sslContextId);

    if (sendATCommand(cmd, "OK", 10000)) {
        _sslConnected = false;
        KISS_LOG("✓ SSL cerrado");
        return true;
    }

    return false;
}

bool QuectelModule::isSSLConnected() {
    return _sslConnected;
}

int QuectelModule::sslWrite(const uint8_t* data, size_t len) {
    if (!_sslConnected || !data || len == 0) return 0;

    char cmd[32];
    snprintf(cmd, sizeof(cmd), "AT+QSSLSEND=%d,%d", _sslContextId, len);

    _serial->println(cmd);
    delay(50);

    // Esperar prompt '>'
    unsigned long start = millis();
    while (millis() - start < 5000) {
        if (_serial->available()) {
            char c = _serial->read();
            if (c == '>') {
                // Enviar datos
                _serial->write(data, len);
                delay(100);

                // Esperar confirmación
                if (waitForResponse("SEND OK", 10000)) {
                    return len;
                }
                return 0;
            }
        }
    }

    return 0;
}

int QuectelModule::sslRead(uint8_t* buffer, size_t len) {
    if (!_sslConnected || !buffer || len == 0) return 0;

    char cmd[32];
    snprintf(cmd, sizeof(cmd), "AT+QSSLRECV=%d,%d", _sslContextId, len);

    _serial->println(cmd);
    delay(50);

    // Leer respuesta: +QSSLRECV: <length>\n<data>
    unsigned long start = millis();
    String response = "";

    while (millis() - start < 5000) {
        if (_serial->available()) {
            char c = _serial->read();
            response += c;

            if (response.indexOf("+QSSLRECV:") >= 0) {
                int dataLen = 0;
                if (sscanf(response.c_str(), "+QSSLRECV: %d", &dataLen) == 1) {
                    if (dataLen > 0 && dataLen <= (int)len) {
                        // Leer datos
                        delay(50);
                        int bytesRead = _serial->readBytes(buffer, dataLen);
                        return bytesRead;
                    }
                }
                return 0;
            }
        }
    }

    return 0;
}

int QuectelModule::sslAvailable() {
    if (!_sslConnected) return 0;

    // Consultar estado
    char response[64];
    char cmd[32];
    snprintf(cmd, sizeof(cmd), "AT+QSSLRECV=%d,0", _sslContextId);

    if (sendATCommand(cmd, response, sizeof(response), 1000)) {
        int available = 0;
        if (sscanf(response, "+QSSLRECV: %d", &available) == 1) {
            return available;
        }
    }

    return 0;
}

// ========== POWER MANAGEMENT ==========

bool QuectelModule::enterSleepMode(int mode) {
    KISS_LOGF("Entrando en sleep mode %d", mode);

    char cmd[32];
    snprintf(cmd, sizeof(cmd), "AT+QSCLK=%d", mode);

    return sendATCommand(cmd, "OK", 2000);
}

bool QuectelModule::wakeUp() {
    if (_dtrPin >= 0) {
        digitalWrite(_dtrPin, LOW);
        delay(100);
        return true;
    }

    // Sin DTR, enviar AT
    return sendATCommand("AT", "OK", 1000);
}

bool QuectelModule::configureDTRSleep(bool enable) {
    if (_dtrPin < 0) {
        KISS_LOG("⚠ DTR no disponible");
        return false;
    }

    if (enable) {
        // Modo sleep con DTR
        return sendATCommand("AT+QSCLK=1", "OK", 2000);
    } else {
        // Deshabilitar sleep
        return sendATCommand("AT+QSCLK=0", "OK", 2000);
    }
}

// ========== INFORMACIÓN ==========

int QuectelModule::getSignalStrength() {
    char response[64];
    if (sendATCommand("AT+CSQ", response, sizeof(response), 2000)) {
        int rssi, ber;
        if (sscanf(response, "+CSQ: %d,%d", &rssi, &ber) == 2) {
            return rssi;  // 0-31, 99=desconocido
        }
    }
    return -1;
}

bool QuectelModule::isNetworkRegistered() {
    char response[64];
    if (sendATCommand("AT+CREG?", response, sizeof(response), 2000)) {
        int n, stat;
        if (sscanf(response, "+CREG: %d,%d", &n, &stat) == 2) {
            return (stat == 1 || stat == 5);  // 1=home, 5=roaming
        }
    }
    return false;
}

bool QuectelModule::getOperator(char* operatorName, size_t size) {
    if (!operatorName || size == 0) return false;

    char response[128];
    if (sendATCommand("AT+COPS?", response, sizeof(response), 3000)) {
        // +COPS: 0,0,"Operator",7
        char* start = strchr(response, '"');
        if (start) {
            start++;
            char* end = strchr(start, '"');
            if (end) {
                size_t len = end - start;
                if (len < size) {
                    strncpy(operatorName, start, len);
                    operatorName[len] = '\0';
                    return true;
                }
            }
        }
    }
    return false;
}

bool QuectelModule::getIMEI(char* imei, size_t size) {
    if (!imei || size == 0) return false;

    char response[64];
    if (sendATCommand("AT+GSN", response, sizeof(response), 2000)) {
        // Buscar línea con solo números (15 dígitos)
        char* start = strstr(response, "\n");
        if (start) {
            start++;
            // Copiar hasta encontrar \n o \r
            size_t i = 0;
            while (i < size - 1 && start[i] && start[i] != '\n' && start[i] != '\r') {
                imei[i] = start[i];
                i++;
            }
            imei[i] = '\0';
            return (i == 15);  // IMEI debe tener 15 dígitos
        }
    }
    return false;
}

// ========== URCs ==========

void QuectelModule::processURC(const String& line) {
    // +QSSLURC: "recv",<clientID>
    if (line.indexOf("+QSSLURC: \"recv\"") >= 0) {
        int clientID = 0;
        sscanf(line.c_str(), "+QSSLURC: \"recv\",%d", &clientID);
        handleSSLRecvURC(clientID);
    }
    // +QSSLURC: "closed",<clientID>
    else if (line.indexOf("+QSSLURC: \"closed\"") >= 0) {
        int clientID = 0;
        sscanf(line.c_str(), "+QSSLURC: \"closed\",%d", &clientID);
        handleSSLClosedURC(clientID);
    }
    // +QIURC: "pdpdeact",<contextID>
    else if (line.indexOf("+QIURC: \"pdpdeact\"") >= 0) {
        int ctxID = 0;
        sscanf(line.c_str(), "+QIURC: \"pdpdeact\",%d", &ctxID);
        handlePDPDeactURC(ctxID);
    }
}

void QuectelModule::handleSSLRecvURC(int clientID) {
    if (_debug) {
        KISS_LOGF("SSL datos disponibles (client %d)", clientID);
    }
}

void QuectelModule::handleSSLClosedURC(int clientID) {
    KISS_LOGF("✗ SSL cerrado por servidor (client %d)", clientID);
    _sslConnected = false;
}

void QuectelModule::handlePDPDeactURC(int contextID) {
    KISS_LOGF("✗ PDP desactivado por red (ctx %d)", contextID);
    _pdpActive = false;
    _sslConnected = false;

    // Resetear contexto
    char cmd[32];
    snprintf(cmd, sizeof(cmd), "AT+QIDEACT=%d", contextID);
    sendATCommand(cmd, "OK", 5000);
}

// ========== ESPECÍFICO QUECTEL - GNSS ==========

bool QuectelModule::enableGNSS() {
    KISS_LOG("Habilitando GNSS...");

    // Configurar constelaciones (GPS + Galileo)
    sendATCommand("AT+QGPSCFG=\"gnssconfig\",4", "OK", 2000);

    // Salida NMEA
    sendATCommand("AT+QGPSCFG=\"outport\",\"uartdebug\"", "OK", 2000);

    // Encender GNSS
    if (sendATCommand("AT+QGPS=1", "OK", 5000)) {
        KISS_LOG("✓ GNSS habilitado");
        return true;
    }

    return false;
}

bool QuectelModule::getGNSSLocation(float* lat, float* lon, float* alt) {
    if (!lat || !lon) return false;

    char response[256];
    if (sendATCommand("AT+QGPSLOC=2", response, sizeof(response), 5000)) {
        // +QGPSLOC: <UTC>,<latitude>,<longitude>,<hdop>,<altitude>,<fix>,<cog>,<spkm>,<spkn>,<date>,<nsat>

        // Parsear respuesta (simplificado)
        float latVal, lonVal, altVal = 0;
        if (sscanf(response, "+QGPSLOC: %*[^,],%f,%f,%*[^,],%f", &latVal, &lonVal, &altVal) >= 2) {
            *lat = latVal;
            *lon = lonVal;
            if (alt) *alt = altVal;
            return true;
        }
    }

    return false;
}

void QuectelModule::disableGNSS() {
    KISS_LOG("Deshabilitando GNSS...");
    sendATCommand("AT+QGPSEND", "OK", 5000);
}

// ========== HELPERS PRIVADOS ==========

bool QuectelModule::waitForNetworkRegistration(unsigned long timeout) {
    unsigned long start = millis();

    while (millis() - start < timeout) {
        if (isNetworkRegistered()) {
            return true;
        }
        delay(2000);
    }

    return false;
}

bool QuectelModule::querySSLState() {
    char response[64];
    char cmd[32];
    snprintf(cmd, sizeof(cmd), "AT+QSSLSTATE=%d", _sslContextId);

    return sendATCommand(cmd, response, sizeof(response), 2000);
}
