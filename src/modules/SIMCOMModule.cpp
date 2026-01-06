// SIMCOMModule.cpp
// Vicente Soriano - victek@gmail.com
// Implementación específica para módulos SIMCOM A7672E/A7682E

#include "SIMCOMModule.h"
#include "../Kiss_setup.h"

SIMCOMModule::SIMCOMModule(HardwareSerial* serial, int pwrKeyPin, int statusPin, int resetPin, int dtrPin)
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

SIMCOMModule::~SIMCOMModule() {
    powerOff();
}

// ========== IDENTIFICACIÓN ==========

bool SIMCOMModule::detect() {
    // SIMCOM responde MUY rápido a AT+GMM (400-600ms típico)
    char response[128];

    // Intentar varias veces con timeout corto
    for (int i = 0; i < 3; i++) {
        if (sendATCommand("AT+CGMM", response, sizeof(response), 800)) {
            if (strstr(response, "A7672E-FASE") || strstr(response, "A767")) {
                KISS_LOG("✓ Módulo SIMCOM detectado");
                return true;
            }
        }
        delay(200);
    }

    // Si no responde rápido, probablemente es Quectel
    return false;
}

// ========== CONTROL DE HARDWARE ==========

bool SIMCOMModule::powerOn() {
    // Verificar si ya está encendido (vía STATUS_PIN)
    if (_statusPin >= 0) {
        if (digitalRead(_statusPin) == HIGH) {
            KISS_LOG("Módulo SIMCOM ya encendido (STATUS=HIGH)");
            return true;
        }
    }

    KISS_LOG("Encendiendo módulo SIMCOM...");

    // SIMCOM: Pulso HIGH de 1.2s en PWRKEY
    digitalWrite(_pwrKeyPin, HIGH);
    delay(1200);
    digitalWrite(_pwrKeyPin, LOW);
    delay(3000);  // SIMCOM tarda más en arrancar

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
            KISS_LOG("✓ Módulo SIMCOM respondiendo");

            // Desactivar echo
            sendATCommand("ATE0", "OK", 1000);

            return true;
        }
        delay(500);
    }

    KISS_LOG("✗ Módulo no responde a AT");
    return false;
}

bool SIMCOMModule::powerOff() {
    KISS_LOG("Apagando módulo SIMCOM...");

    // Cerrar conexión SSL si está activa
    if (_sslConnected) {
        closeSSLConnection();
    }

    // Cerrar red si está abierta
    if (_netOpen) {
        sendATCommand("AT+NETCLOSE", "OK", 10000);
    }

    // Apagado ordenado por AT
    sendATCommand("AT+CPOF", "OK", 5000);
    delay(3000);

    // Verificar STATUS_PIN
    if (_statusPin >= 0) {
        unsigned long start = millis();
        while (millis() - start < 5000) {
            if (digitalRead(_statusPin) == LOW) {
                KISS_LOG("✓ Módulo apagado (STATUS=LOW)");
                _sslConnected = false;
                _netOpen = false;
                return true;
            }
            delay(100);
        }

        KISS_LOG("⚠ STATUS_PIN todavía HIGH, forzando apagado...");
        // Pulso largo de PWRKEY (2.5s)
        digitalWrite(_pwrKeyPin, HIGH);
        delay(2500);
        digitalWrite(_pwrKeyPin, LOW);
        delay(2000);

        if (digitalRead(_statusPin) == LOW) {
            KISS_LOG("✓ Apagado forzado exitoso");
            _sslConnected = false;
            _netOpen = false;
            return true;
        }
    }

    _sslConnected = false;
    _netOpen = false;
    return true;
}

bool SIMCOMModule::hardReset() {
    if (_resetPin < 0) {
        KISS_LOG("⚠ RESET_PIN no disponible, usando PWRKEY");
        // Hard reset con PWRKEY (12 segundos)
        digitalWrite(_pwrKeyPin, HIGH);
        delay(12000);
        digitalWrite(_pwrKeyPin, LOW);
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

bool SIMCOMModule::initialize() {
    KISS_LOG("Inicializando módulo SIMCOM...");

    // Comandos básicos
    if (!sendATCommand("AT", "OK", 1000)) {
        KISS_LOG("✗ Módulo no responde");
        return false;
    }

    // Desactivar echo
    sendATCommand("ATE0", "OK", 1000);

    // Habilitar URCs
    sendATCommand("AT+CREG=2", "OK", 1000);
    sendATCommand("AT+CGREG=2", "OK", 1000);
    sendATCommand("AT+CEREG=2", "OK", 1000);

    // Formato de error extendido
    sendATCommand("AT+CMEE=2", "OK", 1000);

    // Configurar hora (workaround para SSL)
    setClockTime();

    KISS_LOG("✓ Módulo SIMCOM inicializado");
    return true;
}

bool SIMCOMModule::configureAPN(const char* apn, const char* user, const char* pass) {
    if (!apn) return false;

    KISS_LOGF("Configurando APN: %s", apn);

    char cmd[128];

    // SIMCOM: AT+CNCFG=<pdpidx>,<ip_type>,"<APN>","<username>","<password>",<authentication>
    if (strlen(user) > 0 && strlen(pass) > 0) {
        snprintf(cmd, sizeof(cmd), "AT+CNCFG=0,1,\"%s\",\"%s\",\"%s\",3", apn, user, pass);
    } else {
        snprintf(cmd, sizeof(cmd), "AT+CNCFG=0,1,\"%s\"", apn);
    }

    return sendATCommand(cmd, "OK", 3000);
}

bool SIMCOMModule::activatePDP() {
    KISS_LOG("Activando red SIMCOM (NETOPEN)...");

    char response[256];

    // Verificar si ya está activa
    if (sendATCommand("AT+NETOPEN?", response, sizeof(response), 2000)) {
        if (strstr(response, "+NETOPEN: 1")) {
            KISS_LOG("✓ Red ya activa");

            // Obtener IP
            if (sendATCommand("AT+IPADDR", response, sizeof(response), 2000)) {
                char* ipStart = strstr(response, "+IPADDR: ");
                if (ipStart) {
                    ipStart += 9;
                    char* ipEnd = strchr(ipStart, '\r');
                    if (ipEnd) {
                        *ipEnd = '\0';
                        KISS_LOGF("IP: %s", ipStart);
                    }
                }
            }
            _netOpen = true;
            return true;
        }
    }

    // Abrir red (puede tardar hasta 75s)
    if (!sendATCommand("AT+NETOPEN", "OK", 75000)) {
        // Verificar si falló porque ya estaba abierta
        delay(1000);
        if (sendATCommand("AT+NETOPEN?", response, sizeof(response), 2000)) {
            if (strstr(response, "+NETOPEN: 1")) {
                KISS_LOG("✓ Red ya estaba abierta");
                _netOpen = true;
                return true;
            }
        }
        KISS_LOG("✗ Error en NETOPEN");
        return false;
    }

    delay(2000);

    // Obtener IP asignada
    if (sendATCommand("AT+IPADDR", response, sizeof(response), 2000)) {
        char* ipStart = strstr(response, "+IPADDR: ");
        if (ipStart) {
            ipStart += 9;
            char* ipEnd = strchr(ipStart, '\r');
            if (ipEnd) {
                *ipEnd = '\0';
                KISS_LOGF("✓ IP asignada: %s", ipStart);
            }
        }
    }

    _netOpen = true;
    KISS_LOG("✓ Red activada");
    return true;
}

bool SIMCOMModule::deactivatePDP() {
    KISS_LOG("Cerrando red SIMCOM (NETCLOSE)...");

    if (sendATCommand("AT+NETCLOSE", "OK", 40000)) {
        _netOpen = false;
        KISS_LOG("✓ Red cerrada");
        return true;
    }

    return false;
}

bool SIMCOMModule::isPDPActive() {
    // Usar caché para optimizar
    unsigned long now = millis();
    if ((now - _lastNetOpenCheck) < NETOPEN_CACHE_MS) {
        return _lastNetOpenState;
    }

    _lastNetOpenCheck = now;

    char response[128];
    if (sendATCommand("AT+NETOPEN?", response, sizeof(response), 2000)) {
        _lastNetOpenState = (strstr(response, "+NETOPEN: 1") != nullptr);
        return _lastNetOpenState;
    }

    _lastNetOpenState = false;
    return false;
}

// ========== SSL/TLS ==========

bool SIMCOMModule::configureSSL() {
    KISS_LOG("Configurando SSL/TLS SIMCOM...");

    char cmd[64];

    // Modo de conversión de datos (0=texto)
    snprintf(cmd, sizeof(cmd), "AT+CSSLCFG=\"convert\",2,0");
    sendATCommand(cmd, "OK", 2000);

    // TLS 1.2 (valor 3)
    snprintf(cmd, sizeof(cmd), "AT+CSSLCFG=\"sslversion\",0,3");
    sendATCommand(cmd, "OK", 2000);

    // Nivel de autenticación 0 (sin validación)
    snprintf(cmd, sizeof(cmd), "AT+CSSLCFG=\"authmode\",0,0");
    sendATCommand(cmd, "OK", 2000);

    // SNI (crítico para Telegram)
    snprintf(cmd, sizeof(cmd), "AT+CSSLCFG=\"enableSNI\",0,1");
    sendATCommand(cmd, "OK", 2000);

    // Ignorar validación de tiempo del certificado
    snprintf(cmd, sizeof(cmd), "AT+CSSLCFG=\"ignorertctime\",0,1");
    sendATCommand(cmd, "OK", 2000);

    // Timeout de negociación
    snprintf(cmd, sizeof(cmd), "AT+CSSLCFG=\"negotiatetime\",0,300");
    sendATCommand(cmd, "OK", 2000);

    KISS_LOG("✓ SSL configurado");
    return true;
}

bool SIMCOMModule::uploadCACertificate(const char* certPEM) {
    KISS_LOG("Subiendo certificado CA...");

    // TODO: Implementar subida de certificado
    // SIMCOM permite subir certificados con AT+CCERTDOWN

    KISS_LOG("⚠ uploadCACertificate no implementado aún");
    return true;  // Por ahora usar authmode=0
}

bool SIMCOMModule::openSSLConnection(const char* host, int port) {
    if (_sslConnected) {
        KISS_LOG("✓ SSL ya conectado");
        return true;
    }

    KISS_LOGF("Conectando SSL a %s:%d...", host, port);

    // Verificar que NETOPEN está activo
    if (!isPDPActive()) {
        KISS_LOG("✗ NETOPEN no activo");
        return false;
    }

    // Configurar SSL
    configureSSL();

    // Cerrar cualquier SSL previo
    sendATCommand("AT+CSSLSTOP=0", "OK", 2000);
    delay(500);

    // OPCIÓN 1: Usar AT+CAOPEN (TCP sin cifrar - funciona siempre)
    // OPCIÓN 2: Usar AT+CSSLSTART (SSL - requiere firmware con soporte)

    // Por ahora usamos CAOPEN (TCP) ya que es más confiable
    KISS_LOG("Usando CAOPEN (TCP sin cifrar)");

    // Cerrar conexión TCP previa
    sendATCommand("AT+CACLOSE=0", "OK", 2000);
    delay(500);

    // Abrir conexión TCP
    char cmd[128];
    snprintf(cmd, sizeof(cmd), "AT+CAOPEN=0,0,\"TCP\",\"%s\",%d", host, port);

    if (!sendATCommand(cmd, "OK", 5000)) {
        KISS_LOG("✗ Error en CAOPEN");
        return false;
    }

    // Esperar URC: +CAOPEN: 0,0
    unsigned long start = millis();
    while (millis() - start < 30000) {
        if (_serial->available()) {
            String line = readResponse(1000);

            if (line.indexOf("+CAOPEN:") >= 0) {
                int cid, result;
                if (sscanf(line.c_str(), "+CAOPEN: %d,%d", &cid, &result) == 2) {
                    if (result == 0) {
                        _sslConnected = true;
                        KISS_LOG("✓ TCP conectado");
                        return true;
                    } else {
                        KISS_LOGF("✗ Error TCP código %d", result);
                        return false;
                    }
                }
            }
        }
        delay(100);
    }

    KISS_LOG("✗ Timeout TCP");
    return false;
}

bool SIMCOMModule::closeSSLConnection() {
    if (!_sslConnected) return true;

    KISS_LOG("Cerrando conexión...");

    if (sendATCommand("AT+CACLOSE=0", "OK", 10000)) {
        _sslConnected = false;
        KISS_LOG("✓ Conexión cerrada");
        return true;
    }

    return false;
}

bool SIMCOMModule::isSSLConnected() {
    return _sslConnected;
}

int SIMCOMModule::sslWrite(const uint8_t* data, size_t len) {
    if (!_sslConnected || !data || len == 0) return 0;

    char cmd[32];
    snprintf(cmd, sizeof(cmd), "AT+CASEND=0,%d", len);

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
                if (waitForResponse("OK", 10000)) {
                    return len;
                }
                return 0;
            }
        }
    }

    return 0;
}

int SIMCOMModule::sslRead(uint8_t* buffer, size_t len) {
    if (!_sslConnected || !buffer || len == 0) return 0;

    char cmd[32];
    snprintf(cmd, sizeof(cmd), "AT+CARECV=0,%d", len);

    _serial->println(cmd);
    delay(50);

    // Leer respuesta: +CARECV: <length>,<data>
    unsigned long start = millis();
    String response = "";

    while (millis() - start < 5000) {
        if (_serial->available()) {
            char c = _serial->read();
            response += c;

            if (response.indexOf("+CARECV:") >= 0) {
                int dataLen = 0;
                if (sscanf(response.c_str(), "+CARECV: %d", &dataLen) == 1) {
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

int SIMCOMModule::sslAvailable() {
    if (!_sslConnected) return 0;

    // Consultar estado de la conexión
    char response[64];
    if (sendATCommand("AT+CASTATE?", response, sizeof(response), 1000)) {
        // Parsear respuesta para obtener bytes disponibles
        // +CASTATE: <cid>,<state>
        // Si state == 1, hay conexión activa
        return 0;  // Simplificado - SIMCOM no tiene forma fácil de consultar bytes disponibles
    }

    return 0;
}

// ========== POWER MANAGEMENT ==========

bool SIMCOMModule::enterSleepMode(int mode) {
    KISS_LOG("⚠ SIMCOM sleep mode es problemático, deshabilitado");

    // SIMCOM A7672E tiene problemas conocidos con sleep mode
    // Solo habilitar si es absolutamente necesario

    return false;
}

bool SIMCOMModule::wakeUp() {
    if (_dtrPin >= 0) {
        digitalWrite(_dtrPin, LOW);
        delay(80);
        digitalWrite(_dtrPin, HIGH);
        delay(100);
        return true;
    }

    // Sin DTR, enviar AT
    return sendATCommand("AT", "OK", 1000);
}

bool SIMCOMModule::configureDTRSleep(bool enable) {
    KISS_LOG("⚠ DTR sleep deshabilitado en SIMCOM (problemas conocidos)");
    return false;
}

// ========== INFORMACIÓN ==========

int SIMCOMModule::getSignalStrength() {
    char response[64];
    if (sendATCommand("AT+CSQ", response, sizeof(response), 2000)) {
        int rssi, ber;
        if (sscanf(response, "+CSQ: %d,%d", &rssi, &ber) == 2) {
            return rssi;  // 0-31, 99=desconocido
        }
    }
    return -1;
}

bool SIMCOMModule::isNetworkRegistered() {
    char response[64];
    if (sendATCommand("AT+CREG?", response, sizeof(response), 2000)) {
        int n, stat;
        if (sscanf(response, "+CREG: %d,%d", &n, &stat) == 2) {
            return (stat == 1 || stat == 5);  // 1=home, 5=roaming
        }
    }
    return false;
}

bool SIMCOMModule::getOperator(char* operatorName, size_t size) {
    if (!operatorName || size == 0) return false;

    char response[128];
    if (sendATCommand("AT+COPS?", response, sizeof(response), 3000)) {
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

bool SIMCOMModule::getIMEI(char* imei, size_t size) {
    if (!imei || size == 0) return false;

    char response[64];
    if (sendATCommand("AT+GSN", response, sizeof(response), 2000)) {
        char* start = strstr(response, "\n");
        if (start) {
            start++;
            size_t i = 0;
            while (i < size - 1 && start[i] && start[i] != '\n' && start[i] != '\r') {
                imei[i] = start[i];
                i++;
            }
            imei[i] = '\0';
            return (i == 15);
        }
    }
    return false;
}

// ========== URCs ==========

void SIMCOMModule::processURC(const String& line) {
    // +CARECV: <cid>,<length>
    if (line.indexOf("+CARECV:") >= 0) {
        int cid, len;
        if (sscanf(line.c_str(), "+CARECV: %d,%d", &cid, &len) == 2) {
            handleCARecvURC(cid, len);
        }
    }
    // +CASTATE: <cid>,<state>
    else if (line.indexOf("+CASTATE:") >= 0) {
        int cid, state;
        if (sscanf(line.c_str(), "+CASTATE: %d,%d", &cid, &state) == 2) {
            handleCAStateURC(cid, state);
        }
    }
    // +NETCLOSE: <result>
    else if (line.indexOf("+NETCLOSE:") >= 0) {
        int result;
        if (sscanf(line.c_str(), "+NETCLOSE: %d", &result) == 1) {
            handleNetCloseURC(result);
        }
    }
}

void SIMCOMModule::handleCARecvURC(int cid, int len) {
    if (_debug) {
        KISS_LOGF("Datos disponibles (cid=%d, len=%d)", cid, len);
    }
}

void SIMCOMModule::handleCAStateURC(int cid, int state) {
    if (state == 0) {
        KISS_LOGF("✗ Conexión cerrada (cid=%d)", cid);
        _sslConnected = false;
    }
}

void SIMCOMModule::handleNetCloseURC(int result) {
    KISS_LOGF("✗ Red cerrada (resultado=%d)", result);
    _netOpen = false;
    _sslConnected = false;
}

// ========== HELPERS PRIVADOS ==========

bool SIMCOMModule::waitForNetworkRegistration(unsigned long timeout) {
    unsigned long start = millis();

    while (millis() - start < timeout) {
        if (isNetworkRegistered()) {
            return true;
        }
        delay(2000);
    }

    return false;
}

bool SIMCOMModule::setClockTime() {
    // Configurar hora para evitar problemas con certificados SSL
    // Formato: AT+CCLK="YY/MM/DD,HH:MM:SS±ZZ"
    return sendATCommand("AT+CCLK=\"25/12/27,18:00:00+00\"", "OK", 2000);
}

// ========== ESPECÍFICO SIMCOM - GNSS ==========

bool SIMCOMModule::enableGNSS() {
    KISS_LOG("Habilitando GNSS SIMCOM...");

    // SIMCOM A7672E usa AT+CGNSSPWR para GNSS
    // AT+CGNSSPWR=1 encender GNSS
    if (sendATCommand("AT+CGNSSPWR=1", "OK", 5000)) {
        KISS_LOG("✓ GNSS habilitado");
        delay(1000);  // Dar tiempo al GNSS para inicializar
        return true;
    }

    KISS_LOG("✗ No se pudo habilitar GNSS");
    return false;
}

bool SIMCOMModule::getGNSSLocation(float* lat, float* lon, float* alt) {
    if (!lat || !lon) return false;

    char response[512];

    // SIMCOM A7672E: AT+CGNSSINFO
    // Formato completo de la respuesta:
    // +CGNSSINFO: <GNSS run status>,<Fix status>,<UTC date & Time>,<Latitude>,<Longitude>,
    //             <MSL Altitude>,<Speed Over Ground>,<Course Over Ground>,<Fix Mode>,
    //             <Reserved1>,<HDOP>,<PDOP>,<VDOP>,<Reserved2>,<GNSS Satellites in View>,
    //             <GNSS Satellites Used>,<GLONASS Satellites Used>,<Reserved3>,<C/N0 max>,
    //             <HPA>,<VPA>
    //
    // Ejemplo real:
    // +CGNSSINFO: 1,1,20250625120712.000,11.0110083,N,77.0132217,E,314.1,0.0,189.93,1.0,1.3,0.8,1.0,9,9,0,0,45,1.5,2.5
    //
    // Campos importantes:
    // [0] = 1 (GNSS run status, 1=running)
    // [1] = Fix mode (1=No fix, 2=2D fix, 3=3D fix)
    // [2] = UTC date & time (YYYYMMDDHHmmss.sss)
    // [3] = Latitude (decimal degrees)
    // [4] = N/S indicator
    // [5] = Longitude (decimal degrees)
    // [6] = E/W indicator
    // [7] = Altitude (MSL in meters)
    // [8] = Speed over ground (knots)
    // [9] = Course over ground (degrees)
    // [14] = Satellites in view
    // [15] = Satellites used

    if (sendATCommand("AT+CGNSSINFO", response, sizeof(response), 5000)) {
        if (strstr(response, "+CGNSSINFO:")) {
            char* infoStart = strstr(response, "+CGNSSINFO:");
            if (!infoStart) return false;

            // Variables para parsear
            int runStatus, fixMode;
            char utcDateTime[32];
            double latitude, longitude;
            char latDir, lonDir;
            float altitude = 0, speedKnots = 0, course = 0;
            float hdop = 0, pdop = 0, vdop = 0;
            int satsView = 0, satsUsed = 0;

            // Parsear la respuesta completa
            int parsed = sscanf(infoStart,
                "+CGNSSINFO: %d,%d,%[^,],%lf,%c,%lf,%c,%f,%f,%f,%*f,%f,%f,%f,%*f,%d,%d",
                &runStatus,      // [0] GNSS run status
                &fixMode,        // [1] Fix mode (1=No fix, 2=2D, 3=3D)
                utcDateTime,     // [2] UTC date & time
                &latitude,       // [3] Latitude (decimal degrees)
                &latDir,         // [4] N/S
                &longitude,      // [5] Longitude (decimal degrees)
                &lonDir,         // [6] E/W
                &altitude,       // [7] Altitude MSL (meters)
                &speedKnots,     // [8] Speed (knots)
                &course,         // [9] Course (degrees)
                // [10] Reserved - skip with %*f
                &hdop,           // [11] HDOP
                &pdop,           // [12] PDOP
                &vdop,           // [13] VDOP
                // [14] Reserved - skip with %*f
                &satsView,       // [15] Satellites in view
                &satsUsed        // [16] Satellites used
            );

            // Verificar si tenemos fix válido
            if (parsed >= 7 && fixMode >= 2) {  // Fix mode >= 2 (2D o 3D fix)
                // Las coordenadas ya vienen en formato decimal (DD.DDDDDD)
                *lat = latitude;
                *lon = longitude;

                // Aplicar dirección (N/S, E/W)
                if (latDir == 'S') *lat = -*lat;
                if (lonDir == 'W') *lon = -*lon;

                if (alt) *alt = altitude;

                if (_debug) {
                    float speedKmh = speedKnots * 1.852;  // Convertir knots a km/h
                    KISS_LOGF("GPS %dD Fix: Lat=%.7f, Lon=%.7f, Alt=%.1fm, Speed=%.1fkm/h, Sats=%d/%d, HDOP=%.1f",
                             fixMode, *lat, *lon, altitude, speedKmh, satsUsed, satsView, hdop);
                }

                return true;
            } else {
                if (_debug) {
                    if (fixMode == 1) {
                        KISS_LOGF("GNSS sin fix (Sats visibles=%d)", satsView);
                    } else {
                        KISS_LOG("GNSS: datos insuficientes");
                    }
                }
            }
        }
    }

    return false;
}

void SIMCOMModule::disableGNSS() {
    KISS_LOG("Deshabilitando GNSS...");

    // SIMCOM A7672E: AT+CGNSSPWR=0
    if (sendATCommand("AT+CGNSSPWR=0", "OK", 5000)) {
        KISS_LOG("✓ GNSS deshabilitado");
        return;
    }

    KISS_LOG("⚠ Error deshabilitando GNSS");
}
