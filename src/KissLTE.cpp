// KissLTE.cpp
// Vicente Soriano - victek@gmail.com
// ImplementaciÃ³n del sistema LTE con módulo Quectel EC200A-EU-HA
// SSL/TLS nativo mediante comandos AT+QSSL*

#include "KissLTE.h"

// ========== CONSTRUCTOR/DESTRUCTOR ==========
KissLTE::KissLTE()
  : _serial(nullptr),
    _rxPin(-1),
    _txPin(-1),
    _pwrKeyPin(-1),
    _dtrPin(-1),
    _state(STATE_OFF),
    _initialized(false),
    _powered(false),
    _debug(KISS_DEVELOPMENT_MODE),
    _timeout(1000),
    _lastActivityTime(0),
    _secureMode(true),  // Quectel siempre usa SSL
    _pinRequired(false),
    _tcpConnected(false),
    _pdpDeactivated(false),
    _signalStrength(-999),
    _lastKeepalive(0),
    _sslContextId(0),
    _sslConfigured(false),
    _sslConnected(false),
    _timeInSleep(0),
    _timeInActive(0),
    _isInSleepMode(false),
    _healthCheckFailures(0) {

  memset(_apn, 0, sizeof(_apn));
  memset(_apnUser, 0, sizeof(_apnUser));
  memset(_apnPass, 0, sizeof(_apnPass));
  memset(_pin, 0, sizeof(_pin));
  memset(_imei, 0, sizeof(_imei));
  memset(_iccid, 0, sizeof(_iccid));
  memset(_operator, 0, sizeof(_operator));
  memset(_buffer, 0, sizeof(_buffer));
  memset(_moduleModel, 0, sizeof(_moduleModel));

  // Inicializar variables heredadas de KissClient
  _connectionStartTime = 0;
  _errorCount = 0;
  _powerMode = CLIENT_POWER_BOOT;

  // Inicializar watchdog
  _lastSuccessfulComm = millis();
  _lastHealthCheck = millis();
  _lastModeChange = millis();
}

KissLTE::~KissLTE() {
  end();
}

// ========== INICIALIZACIÓN ==========
bool KissLTE::begin(HardwareSerial* serial, int rxPin, int txPin, int pwrKeyPin, int dtrPin) {
  if (_initialized) {
    KISS_LOG("KissLTE ya inicializado");
    return true;
  }

  KISS_LOG("Iniciando KissLTE para Quectel EC200A...");

  _serial = serial;
  _rxPin = rxPin;
  _txPin = txPin;
  _pwrKeyPin = pwrKeyPin;
  _dtrPin = dtrPin;

  // Configurar GPIO PWRKEY
  pinMode(_pwrKeyPin, OUTPUT);
  digitalWrite(_pwrKeyPin, LOW);

  // Configurar GPIO DTR si está definido (opcional)
  if (_dtrPin >= 0) {
    pinMode(_dtrPin, OUTPUT);
    digitalWrite(_dtrPin, LOW);
    KISS_LOGF("DTR configurado en GPIO%d", _dtrPin);
  }

  // Configurar UART para Quectel (115200 por defecto)
  _serial->begin(115200, SERIAL_8N1, _rxPin, _txPin);
  _serial->setTimeout(_timeout);

  KISS_LOGF("UART configurado: RX=%d, TX=%d, PWRKEY=%d", _rxPin, _txPin, _pwrKeyPin);

  // Cargar credenciales desde NVS
  loadCredentialsFromNVS();

  _initialized = true;
  setState(STATE_OFF);

  KISS_LOG("KissLTE inicializado para Quectel EC200A");
  return true;
}

void KissLTE::end() {
  if (!_initialized) return;

  if (_powered) {
    powerOff();
  }

  if (_serial) {
    _serial->end();
  }

  _initialized = false;
  setState(STATE_OFF);
  KISS_LOG("KissLTE detenido");
}

// ========== LOOP PRINCIPAL ==========
void KissLTE::loop() {
  if (!_initialized || !_powered) return;

  // 1. Procesar URCs pendientes (no bloqueante)
  processURCs();

  // 2. Keepalive periodico para evitar desconexion por inactividad
  sendKeepalive();

  // 3. Health check periodico
  updateHealthStatus();

  // 4. Recovery si es necesario
  if (_pdpDeactivated || _healthCheckFailures >= MAX_HEALTH_FAILURES) {
    checkAndRecover();
  }
}

// ========== PROCESAMIENTO DE URCs ==========
void KissLTE::processURCs() {
  if (!_serial || !_powered) return;

  // No bloquear - solo procesar si hay datos
  while (_serial->available()) {
    char line[128];
    int len = readLine(line, sizeof(line), 100);
    if (len <= 0) break;

    // +QSSLURC: "recv",<clientID>
    if (strstr(line, "+QSSLURC: \"recv\"")) {
      int clientID = 0;
      sscanf(line, "+QSSLURC: \"recv\",%d", &clientID);
      handleSSLRecvURC(clientID);
    }
    // +QSSLURC: "closed",<clientID>
    else if (strstr(line, "+QSSLURC: \"closed\"")) {
      int clientID = 0;
      sscanf(line, "+QSSLURC: \"closed\",%d", &clientID);
      handleSSLClosedURC(clientID);
    }
    // +QIURC: "pdpdeact",<contextID> - CRITICO
    else if (strstr(line, "+QIURC: \"pdpdeact\"")) {
      int ctxID = 0;
      sscanf(line, "+QIURC: \"pdpdeact\",%d", &ctxID);
      handlePDPDeactURC(ctxID);
    }
    // +QIURC: "closed",<connectID>
    else if (strstr(line, "+QIURC: \"closed\"")) {
      KISS_LOG("Socket cerrado por red");
      _tcpConnected = false;
    }

    SAFE_YIELD();
  }
}

void KissLTE::handleSSLRecvURC(int clientID) {
  if (_debug) {
    KISS_LOGF("SSL datos disponibles (client %d)", clientID);
  }
}

void KissLTE::handleSSLClosedURC(int clientID) {
  KISS_LOGF("SSL cerrado por servidor (client %d)", clientID);
  _sslConnected = false;
  _tcpConnected = false;
  _errorCount++;
}

void KissLTE::handlePDPDeactURC(int contextID) {
  // MUY IMPORTANTE: La red desactivo el contexto PDP
  KISS_LOGF("PDP desactivado por red (ctx %d)", contextID);

  _pdpDeactivated = true;
  _sslConnected = false;
  _tcpConnected = false;
  setState(STATE_REGISTERING);

  // Segun manual: DEBE ejecutar AT+QIDEACT para resetear
  char cmd[32];
  snprintf(cmd, sizeof(cmd), "AT+QIDEACT=%d", contextID);
  sendATCommand(cmd, "OK", 5000);
}

// ========== RECONEXION Y RECOVERY ==========
bool KissLTE::ensureConnected() {
  if (_pdpDeactivated) {
    KISS_LOG("Reactivando PDP...");
    _pdpDeactivated = false;

    if (!activatePDP()) {
      KISS_LOG("No se pudo reactivar PDP");
      return false;
    }

    if (_sslConfigured) {
      _sslConfigured = false;
    }
  }

  if (_sslConnected && !querySSLState()) {
    KISS_LOG("SSL perdido, marcando desconectado");
    _sslConnected = false;
    _tcpConnected = false;
  }

  return (_state == STATE_CONNECTED);
}

bool KissLTE::checkAndRecover() {
  // 1. Procesar URCs pendientes
  processURCs();

  // 2. Verificar si hay conexion de red
  if (!checkNetworkRegistration()) {
    KISS_LOG("Sin registro de red");
    setState(STATE_REGISTERING);

    unsigned long start = millis();
    while (millis() - start < 60000) {
      if (checkNetworkRegistration()) {
        KISS_LOG("Red recuperada");

        if (!activatePDP()) {
          KISS_LOG("No se pudo reactivar PDP");
          return false;
        }

        setState(STATE_CONNECTED);
        return true;
      }
      SAFE_DELAY(2000);
    }

    KISS_LOG("Timeout esperando red");
    return false;
  }

  // 3. Verificar PDP activo
  if (_pdpDeactivated) {
    return ensureConnected();
  }

  return true;
}

// ========== CONTROL DE ENERGÍA (ESPECÍFICO QUECTEL) ==========
bool KissLTE::powerOn() {
  if (_powered) {
    KISS_LOG("Módulo Quectel ya activado");
    return true;
  }

  KISS_LOG("Arrancando módulo Quectel EC200A...");
  setState(STATE_POWERING_ON);

  // SECUENCIA QUECTEL: PWRKEY LOW durante 1.5s
  digitalWrite(_pwrKeyPin, LOW);
  SAFE_DELAY(1500);
  digitalWrite(_pwrKeyPin, HIGH);
  SAFE_DELAY(2000);  // Esperar arranque del módulo

  // Verificar que el módulo responde
  if (!waitForModuleReady(10000)) {
    KISS_LOG("Módulo Quectel no responde");
    setState(STATE_ERROR);
    return false;
  }

  _powered = true;
  
  // Detectar modelo especí­fico
  if (!detectModule()) {
    KISS_LOG("No se confirmó módulo Quectel EC200A");
  }
  
  KISS_LOG("Módulo Quectel EC200A encendido");
  return true;
}

bool KissLTE::powerOff() {
  if (!_powered) {
    return true;
  }

  KISS_LOG("Apagando módulo Quectel...");

  // Cerrar conexiones SSL si estÃ¡n abiertas
  if (_sslConnected) {
    closeSSLConnection();
  }

  // Comando QUECTEL para apagado ordenado
  sendATCommand("AT+QPOWD=1", "OK", 5000);

  _powered = false;
  setState(STATE_OFF);
  KISS_LOG("Módulo Quectel apagado");
  return true;
}

bool KissLTE::reset() {
  KISS_LOG("Reiniciando módulo Quectel...");

  if (_powered) {
    powerOff();
    SAFE_DELAY(2000);
  }

  return powerOn();
}

bool KissLTE::pulsePoweKey() {
  // Ya implementado en powerOn()
  return true;
}

bool KissLTE::waitForModuleReady(unsigned long timeout) {
  KISS_LOG("Esperando respuesta del módulo Quectel...");

  unsigned long startTime = millis();
  clearSerialBuffer();
  
  // Dar tiempo al mÃ³dulo para inicializar
  SAFE_DELAY(3000);

  int attempts = 0;
  while (millis() - startTime < timeout) {
    if (sendATCommand("AT", "OK", 1000)) {
      KISS_LOG("Módulo Quectel responde a AT");
      return true;
    }

    attempts++;
    if (_debug && attempts % 3 == 0) {
      KISS_LOGF("Intento %d de comunicación...", attempts);
    }
    SAFE_DELAY(2000);
  }

  KISS_LOG("Timeout esperando módulo Quectel");
  return false;
}

// ========== COMANDOS AT QUECTEL ==========
bool KissLTE::sendATCommand(const char* cmd, const char* expectedResponse, unsigned long timeout) {
  if (!_serial) return false;

  clearSerialBuffer();

  if (_debug) {
    KISS_LOGF("â†’ %s", cmd);
  }

  _serial->println(cmd);
  _lastActivityTime = millis();

  bool success = waitForResponse(expectedResponse, timeout);
  if (success) {
    _lastSuccessfulComm = millis();
  }
  return success;
}

bool KissLTE::sendATCommand(const char* cmd, char* response, size_t responseSize, unsigned long timeout) {
  if (!_serial || !response) return false;

  clearSerialBuffer();

  if (_debug) {
    KISS_LOGF("â†’ %s", cmd);
  }

  _serial->println(cmd);
  _lastActivityTime = millis();

  int len = readLine(response, responseSize, timeout);
  
  if (len > 0) {
    if (_debug) {
      KISS_LOGF(" %s", response);
    }
    _lastSuccessfulComm = millis();
  }

  return len > 0;
}

bool KissLTE::waitForResponse(const char* expected, unsigned long timeout) {
  unsigned long startTime = millis();
  String response = "";

  while (millis() - startTime < timeout) {
    if (_serial->available()) {
      char c = _serial->read();
      if (c != '\r') {
        response += c;
      }

      if (response.indexOf(expected) >= 0) {
        if (_debug) {
          KISS_LOGF(" %s", response.c_str());
        }
        return true;
      }

      if (response.indexOf("ERROR") >= 0) {
        if (_debug) {
          KISS_LOGF("ERROR: %s", response.c_str());
        }
        return false;
      }
    }
    SAFE_YIELD();
  }

  if (_debug) {
    KISS_LOG("Timeout esperando respuesta");
  }
  return false;
}

int KissLTE::readLine(char* buffer, size_t size, unsigned long timeout) {
  if (!buffer || size == 0) return 0;

  unsigned long startTime = millis();
  size_t index = 0;

  while (millis() - startTime < timeout && index < size - 1) {
    if (_serial->available()) {
      char c = _serial->read();
      
      if (c == '\n') {
        break;
      }
      
      if (c != '\r') {
        buffer[index++] = c;
      }
    }
    SAFE_YIELD();
  }

  buffer[index] = '\0';
  return index;
}

void KissLTE::clearSerialBuffer() {
  while (_serial && _serial->available()) {
    _serial->read();
    SAFE_YIELD();
  }
}

void KissLTE::flushSerial() {
  if (_serial) {
    _serial->flush();
  }
}

bool KissLTE::readATResponse(const char* cmd, char* response, size_t responseSize, unsigned long timeout) {
  if (!_serial || !response || responseSize == 0) return false;

  clearSerialBuffer();

  // Wake up mÃ³dulo si tiene DTR
  if (_dtrPin >= 0) {
    digitalWrite(_dtrPin, LOW);
    SAFE_DELAY(30);
  }

  if (_debug) {
    KISS_LOGF("%s", cmd);
  }

  _serial->println(cmd);
  _lastActivityTime = millis();

  SAFE_DELAY(100);
  
  memset(response, 0, responseSize);
  
  unsigned long startTime = millis();
  size_t idx = 0;
  bool foundOK = false;
  bool foundERROR = false;
  unsigned long lastCharTime = millis();

  while (millis() - startTime < timeout && idx < responseSize - 1) {
    if (_serial->available()) {
      char c = _serial->read();
      lastCharTime = millis();

      if (c != '\r') {
        response[idx++] = c;
      }

      if (strstr(response, "OK")) foundOK = true;
      if (strstr(response, "ERROR")) foundERROR = true;

      if ((foundOK || foundERROR) && (millis() - lastCharTime > 100)) {
        break;
      }
    }
    SAFE_YIELD();
  }

  response[idx] = '\0';

  if (_debug && idx > 0) {
    KISS_LOGF("%s", response);
  }

  if (foundOK) {
    _lastSuccessfulComm = millis();
    return true;
  }

  return false;
}

// ========== GESTIÓN DE CONEXIÓN LTE ==========
bool KissLTE::connect() {
  if (!_initialized || !_powered) {
    KISS_LOG("Módulo no inicializado o apagado");
    return false;
  }

  if (_state == STATE_CONNECTED && checkNetworkRegistration()) {
    KISS_LOG("Ya conectado a red LTE");
    return true;
  }

  KISS_LOG("Conectando a red LTE Quectel...");
  setState(STATE_INITIALIZING);

  // 1. Inicializar módulo
  if (!initializeModule()) {
    KISS_LOG("Error inicializando módulo Quectel");
    setState(STATE_ERROR);
    return false;
  }

  // 2. Configurar red
  if (!configureNetwork()) {
    KISS_LOG("Error configurando red");
    setState(STATE_ERROR);
    return false;
  }

  setState(STATE_REGISTERING);
  KISS_LOG("Esperando registro en red LTE (hasta 60s)...");

  // 3. Esperar registro
  unsigned long startTime = millis();
  while (millis() - startTime < 60000) {
    if (checkNetworkRegistration()) {
      KISS_LOG("Registrado en red LTE");
      
      // 4. Activar contexto PDP (Quectel)
      if (activatePDP()) {
        setState(STATE_CONNECTED);
        updateSignalStrength();
        printModuleInfo();
        KISS_LOG("Conectado a red LTE con IP asignada");
        return true;
      } else {
        KISS_LOG("Error activando PDP");
        setState(STATE_ERROR);
        return false;
      }
    }
    SAFE_DELAY(2000);
  }

  KISS_LOG("Timeout registrando en red LTE");
  setState(STATE_ERROR);
  return false;
}

void KissLTE::disconnect() {
  if (_state != STATE_CONNECTED) {
    return;
  }

  KISS_LOG("Desconectando de red LTE...");

  if (_sslConnected) {
    closeSSLConnection();
  }

  // Desactivar PDP (Quectel)
  sendATCommand("AT+QIDEACT=1", "OK", 5000);

  setState(STATE_OFF);
  KISS_LOG("Desconectado de red LTE");
}

// ========== INICIALIZACIÓN QUECTEL ==========
bool KissLTE::initializeModule() {
  KISS_LOG("Inicializando Quectel EC200A...");

  // 1. Deshabilitar sleep mode durante inicialización
  sendATCommand("AT+QSCLK=0", "OK", 2000);
  
  // 2. Deshabilitar echo
  if (!sendATCommand("ATE0", "OK", 1000)) {
    KISS_LOG("No se pudo deshabilitar echo");
  }
  
  SAFE_DELAY(1000);
  
  // 3. Configurar funcionalidad completa
  if (!sendATCommand("AT+CFUN=1", "OK", 10000)) {
    KISS_LOG("Error configurando CFUN=1");
    return false;
  }
  
  SAFE_DELAY(3000);
  
  // 4. Verificar SIM
  if (!checkPIN()) {
    KISS_LOG("Problema con SIM, continuando...");
  }
  
  // 5. Obtener información del módulo
  sendATCommand("AT+GSN", _imei, sizeof(_imei), 2000);
  KISS_LOGF("IMEI: %s", _imei);
  
  sendATCommand("AT+QCCID", _iccid, sizeof(_iccid), 2000);
  KISS_LOGF("ICCID: %s", _iccid);
  
  // 6. Configurar SSL para Quectel
  if (!configureSSL()) {
    KISS_LOG("No se pudo configurar SSL, continuando...");
  }
  
  KISS_LOG("Quectel EC200A inicializado");
  return true;
}

bool KissLTE::configureNetwork() {
  KISS_LOG("Configurando red Quectel...");

  // 1. Habilitar notificaciones de registro
  sendATCommand("AT+CREG=2", "OK", 2000);
  sendATCommand("AT+CGREG=2", "OK", 2000);
  sendATCommand("AT+CEREG=2", "OK", 2000);
  
  // 2. Configurar modo automático 2G/3G/4G
  sendATCommand("AT+CNMP=2", "OK", 2000);
  
  // 3. Configurar APN
  if (strlen(_apn) > 0) {
    char cmd[128];
    snprintf(cmd, sizeof(cmd), "AT+QICSGP=1,1,\"%s\",\"%s\",\"%s\",1", 
             _apn, _apnUser, _apnPass);
    
    if (!sendATCommand(cmd, "OK", 3000)) {
      KISS_LOG("Error configurando APN");
      return false;
    }
    KISS_LOGF("APN configurado: %s", _apn);
  }
  
  // 4. Configurar bandas LTE Europa (B3, B7, B20)
  sendATCommand("AT+QBAND=1,3,7,20", "OK", 3000);
  
  // 5. Búsqueda automática de operador
  if (!sendATCommand("AT+COPS=0", "OK", 60000)) {
    KISS_LOG("Error en búsqueda automática de operador");
    return false;
  }
  
  KISS_LOG("Red configurada");
  return true;
}

bool KissLTE::activatePDP() {
  KISS_LOG("Activando contexto PDP Quectel...");

  char response[256];
  
  // 1. Verificar si ya está activo
  if (readATResponse("AT+QIACT?", response, sizeof(response), 2000)) {
    if (strstr(response, "+QIACT: 1,1") != nullptr) {
      KISS_LOG("Contexto PDP ya activo");
      
      // Extraer IP
      char* ipStart = strchr(response, '"');
      if (ipStart) {
        ipStart++;
        char* ipEnd = strchr(ipStart, '"');
        if (ipEnd) {
          *ipEnd = '\0';
          KISS_LOGF("IP asignada: %s", ipStart);
        }
      }
      return true;
    }
  }
  
  // 2. Activar contexto PDP
  KISS_LOG("Activando PDP...");
  if (!sendATCommand("AT+QIACT=1", "OK", 150000)) {
    KISS_LOG("Error activando PDP");
    return false;
  }
  
  KISS_LOG("PDP activado");
  SAFE_DELAY(2000);
  
  // 3. Obtener IP
  if (readATResponse("AT+QIACT?", response, sizeof(response), 2000)) {
    char* ipStart = strchr(response, '"');
    if (ipStart) {
      ipStart++;
      char* ipEnd = strchr(ipStart, '"');
      if (ipEnd) {
        *ipEnd = '\0';
        KISS_LOGF("IP asignada: %s", ipStart);
      }
    }
  }
  
  // 4. Configurar DNS (opcional)
  sendATCommand("AT+QIDNSCFG=1,\"8.8.8.8\",\"8.8.4.4\"", "OK", 3000);
  
  return true;
}

// ========== CONFIGURACION SSL/TLS QUECTEL ==========
bool KissLTE::configureSSL() {
  if (_sslConfigured) return true;

  KISS_LOG("Configurando SSL/TLS Quectel...");

  char cmd[64];

  // 1. TLS 1.2 (valor 4 en EC200A)
  snprintf(cmd, sizeof(cmd), "AT+QSSLCFG=\"sslversion\",%d,4", _sslContextId);
  if (!sendATCommand(cmd, "OK", 2000)) {
    KISS_LOG("Fallback a TLS 1.1");
    snprintf(cmd, sizeof(cmd), "AT+QSSLCFG=\"sslversion\",%d,3", _sslContextId);
    sendATCommand(cmd, "OK", 2000);
  }

  // 2. Nivel seguridad 0 = sin validar cert
  snprintf(cmd, sizeof(cmd), "AT+QSSLCFG=\"seclevel\",%d,0", _sslContextId);
  sendATCommand(cmd, "OK", 2000);

  // 3. Cipher suites - todos los soportados
  snprintf(cmd, sizeof(cmd), "AT+QSSLCFG=\"ciphersuite\",%d,0xFFFF", _sslContextId);
  sendATCommand(cmd, "OK", 2000);

  // 4. Ignorar tiempo local
  snprintf(cmd, sizeof(cmd), "AT+QSSLCFG=\"ignorelocaltime\",%d,1", _sslContextId);
  sendATCommand(cmd, "OK", 2000);

  // 5. SNI - CRITICO para Telegram
  snprintf(cmd, sizeof(cmd), "AT+QSSLCFG=\"sni\",%d,1", _sslContextId);
  if (!sendATCommand(cmd, "OK", 2000)) {
    KISS_LOG("SNI no soportado");
  }

  // 6. Timeout de negociacion
  snprintf(cmd, sizeof(cmd), "AT+QSSLCFG=\"negotiatetime\",%d,300", _sslContextId);
  sendATCommand(cmd, "OK", 2000);

  _sslConfigured = true;
  KISS_LOG("SSL/TLS configurado");
  return true;
}


bool KissLTE::openSSLConnection(const char* host, int port) {
  if (_sslConnected) return true;

  KISS_LOGF("Conectando SSL a %s:%d...", host, port);
  
  if (!_sslConfigured && !configureSSL()) {
    KISS_LOG("SSL no configurado");
    return false;
  }
  
  // Comando Quectel: AT+QSSLOPEN
  char cmd[128];
  snprintf(cmd, sizeof(cmd), "AT+QSSLOPEN=1,%d,0,\"%s\",%d,0", 
           _sslContextId, host, port);
  
  if (!sendATCommand(cmd, "OK", 5000)) {
    KISS_LOG("Error en AT+QSSLOPEN");
    return false;
  }
  
  // Esperar respuesta URC: +QSSLOPEN: 0,0
  char response[64];
  unsigned long start = millis();
  
  while (millis() - start < 30000) {
    if (readATResponse("", response, sizeof(response), 1000)) {
      if (strstr(response, "+QSSLOPEN:") != nullptr) {
        int clientID, result;
        if (sscanf(response, "+QSSLOPEN: %d,%d", &clientID, &result) == 2) {
          if (result == 0) {
            _sslConnected = true;
            _tcpConnected = true;
            KISS_LOG("Conexión SSL establecida");
            return true;
          } else {
            KISS_LOGF("Error SSL código %d", result);
            return false;
          }
        }
      }
    }
    SAFE_DELAY(100);
  }
  
  KISS_LOG("Timeout SSL");
  return false;
}

bool KissLTE::closeSSLConnection() {
  if (!_sslConnected) return true;

  KISS_LOG("Cerrando conexión SSL...");
  
  if (sendATCommand("AT+QSSLCLOSE=0,10", "OK", 15000)) {
    _sslConnected = false;
    _tcpConnected = false;
    KISS_LOG("Conexión SSL cerrada");
    return true;
  }
  
  KISS_LOG("Error cerrando SSL");
  return false;
}

int KissLTE::sslWrite(const uint8_t* data, size_t len) {
  if (!_sslConnected) return -1;

  char cmd[32];
  snprintf(cmd, sizeof(cmd), "AT+QSSLSEND=0,%d", len);
  
  clearSerialBuffer();
  _serial->println(cmd);
  
  // Esperar prompt '>'
  unsigned long start = millis();
  while (millis() - start < 5000) {
    if (_serial->available()) {
      char c = _serial->read();
      if (c == '>') break;
    }
  }
  
  // Enviar datos
  size_t written = _serial->write(data, len);
  _serial->flush();
  
  // Esperar confirmación
  char response[64];
  if (waitForResponse("+QSSLSEND:", 10000)) {
    if (readATResponse("", response, sizeof(response), 2000)) {
      int clientID, sentLen, ackLen;
      if (sscanf(response, "+QSSLSEND: %d,%d,%d", &clientID, &sentLen, &ackLen) == 3) {
        return ackLen;
      }
    }
  }
  
  return -1;
}

int KissLTE::sslRead(uint8_t* buffer, size_t len) {
  if (!_sslConnected) return -1;

  char cmd[32];
  snprintf(cmd, sizeof(cmd), "AT+QSSLRECV=0,%d", len);
  
  char response[1024];
  if (!readATResponse(cmd, response, sizeof(response), 5000)) {
    return -1;
  }
  
  // Parsear: +QSSLRECV: <data_length>\r\n<data>
  char* dataStart = strstr(response, "+QSSLRECV:");
  if (!dataStart) return -1;
  
  int dataLen;
  if (sscanf(dataStart, "+QSSLRECV: %d", &dataLen) != 1) {
    return -1;
  }
  
  dataStart = strchr(dataStart, '\n');
  if (!dataStart) return -1;
  dataStart++;
  
  size_t copyLen = (dataLen < (int)len) ? dataLen : len;
  memcpy(buffer, dataStart, copyLen);
  
  return copyLen;
}

int KissLTE::sslAvailable() {
  return _sslConnected ? _serial->available() : 0;
}

// ========== DETECCIÓN MÓDULO ==========
bool KissLTE::detectModule() {
  KISS_LOG("Detectando módulo Quectel...");

  char response[128];
  
  // AT+CGMM para Quectel
  if (readATResponse("AT+CGMM", response, sizeof(response), 2000)) {
    if (strstr(response, "EC200") != nullptr) {
      strncpy(_moduleModel, "EC200A-EU-HA", sizeof(_moduleModel));
      
      // Limpiar saltos de lÃ­nea
      char* nl = strchr(_moduleModel, '\r');
      if (nl) *nl = '\0';
      nl = strchr(_moduleModel, '\n');
      if (nl) *nl = '\0';
      
      KISS_LOGF("Detectado: %s", _moduleModel);
      return true;
    }
  }
  
  // ATI como fallback
  if (readATResponse("ATI", response, sizeof(response), 2000)) {
    if (strstr(response, "Quectel") != nullptr) {
      strncpy(_moduleModel, "Quectel EC200A", sizeof(_moduleModel));
      KISS_LOGF("Detectado: %s", _moduleModel);
      return true;
    }
  }
  
  KISS_LOG("No se confirmó módulo Quectel EC200A");
  return false;
}

const char* KissLTE::getModuleName() {
  return _moduleModel;
}

// ========== CONFIGURACIÓN RED ==========
bool KissLTE::setAPN(const char* apn, const char* user, const char* pass) {
  if (!apn) return false;

  strncpy(_apn, apn, sizeof(_apn) - 1);
  if (user) strncpy(_apnUser, user, sizeof(_apnUser) - 1);
  if (pass) strncpy(_apnPass, pass, sizeof(_apnPass) - 1);

  KISS_LOGF("APN configurado: %s", _apn);
  return saveCredentialsToNVS();
}

bool KissLTE::setPIN(const char* pin) {
  if (!pin) return false;
  
  strncpy(_pin, pin, sizeof(_pin) - 1);
  KISS_LOG("PIN configurado");
  return saveCredentialsToNVS();
}

bool KissLTE::checkPIN() {
  KISS_LOG("Verificando SIM...");

  char response[256];
  
  if (!sendATCommand("AT+CPIN?", response, sizeof(response), 5000)) {
    KISS_LOG("No hay respuesta AT+CPIN?, continuando...");
    _pinRequired = false;
    return true;
  }
  
  if (strstr(response, "READY") || strstr(response, "+CPIN: READY")) {
    KISS_LOG("SIM lista");
    _pinRequired = false;
    return true;
  }
  
  if (strstr(response, "SIM PIN")) {
    KISS_LOG("SIM requiere PIN");
    _pinRequired = true;
    return unlockSIM();
  }
  
  if (strstr(response, "OK")) {
    KISS_LOG("SIM lista (OK recibido)");
    _pinRequired = false;
    return true;
  }
  
  KISS_LOG("Estado SIM desconocido");
  _pinRequired = false;
  return true;
}

bool KissLTE::unlockSIM() {
  if (strlen(_pin) == 0) {
    KISS_LOG("PIN no configurado");
    return false;
  }

  KISS_LOG("Desbloqueando SIM...");

  char cmd[32];
  snprintf(cmd, sizeof(cmd), "AT+CPIN=\"%s\"", _pin);
  
  if (!sendATCommand(cmd, "OK", 5000)) {
    KISS_LOG("PIN incorrecto o error");
    return false;
  }
  
  KISS_LOG("SIM desbloqueada");
  _pinRequired = false;
  return true;
}

// ========== INFORMACIÓN MÓDULO ==========
bool KissLTE::getIMEI(char* imei, size_t size) {
  if (!imei || size == 0) return false;
  
  if (strlen(_imei) > 0) {
    strncpy(imei, _imei, size - 1);
    return true;
  }
  
  return sendATCommand("AT+GSN", imei, size, 2000);
}

bool KissLTE::getICCID(char* iccid, size_t size) {
  if (!iccid || size == 0) return false;
  
  if (strlen(_iccid) > 0) {
    strncpy(iccid, _iccid, size - 1);
    return true;
  }
  
  return sendATCommand("AT+QCCID", iccid, size, 2000);
}

bool KissLTE::getOperator(char* operatorName, size_t size) {
  if (!operatorName || size == 0) return false;

  char response[128];
  if (sendATCommand("AT+COPS?", response, sizeof(response), 5000)) {
    char* start = strchr(response, '"');
    if (start) {
      start++;
      char* end = strchr(start, '"');
      if (end) {
        size_t len = end - start;
        if (len < size) {
          strncpy(operatorName, start, len);
          operatorName[len] = '\0';
          strncpy(_operator, operatorName, sizeof(_operator) - 1);
          return true;
        }
      }
    }
  }
  
  return false;
}

// ========== CALIDAD SEÑAL ==========
int KissLTE::getSignalStrength() {
  char response[64];
  if (!readATResponse("AT+CSQ", response, sizeof(response), 2000)) {
    return -999;
  }
  
  char* start = strstr(response, "+CSQ: ");
  if (start) {
    int rssi = atoi(start + 6);
    
    if (rssi >= 0 && rssi <= 31) {
      _signalStrength = -113 + (rssi * 2);
      return _signalStrength;
    }
  }
  
  _signalStrength = -999;
  return _signalStrength;
}

KissLTE::SignalQuality KissLTE::getSignalQuality() {
  int rssi = getSignalStrength();
  
  if (rssi == -999 || rssi < -110) return SIGNAL_NONE;
  if (rssi < -95) return SIGNAL_POOR;
  if (rssi < -85) return SIGNAL_FAIR;
  if (rssi < -75) return SIGNAL_GOOD;
  return SIGNAL_EXCELLENT;
}

void KissLTE::updateSignalStrength() {
  getSignalStrength();
}

bool KissLTE::checkNetworkRegistration() {
  char response[128];
  int n, stat;
  bool registered = false;
  
  // Wake up si tiene DTR
  if (_dtrPin >= 0) {
    digitalWrite(_dtrPin, LOW);
    SAFE_DELAY(50);
  }
  
  // Verificar CREG (2G/3G)
  if (readATResponse("AT+CREG?", response, sizeof(response), 2000)) {
    char* cregPos = strstr(response, "+CREG:");
    if (cregPos && sscanf(cregPos, "+CREG: %d,%d", &n, &stat) >= 2) {
      if (stat == 1 || stat == 5) registered = true;
    }
  }
  
  // Verificar CGREG (GPRS)
    if (!registered && readATResponse("AT+CGREG?", response, sizeof(response), 2000)) {
        char* cgregPos = strstr(response, "+CGREG:");
        if (cgregPos && sscanf(cgregPos, "+CGREG: %d,%d", &n, &stat) >= 2) {
            if (stat == 1 || stat == 5) registered = true;
        }
    }
    
    // Verificar CEREG (LTE) - IMPORTANTE para Quectel
    if (!registered && readATResponse("AT+CEREG?", response, sizeof(response), 2000)) {
        char* ceregPos = strstr(response, "+CEREG:");
        if (ceregPos && sscanf(ceregPos, "+CEREG: %d,%d", &n, &stat) >= 2) {
            if (stat == 1 || stat == 5) registered = true;
        }
    }
  
  return registered;
}

// ========== NVS ==========
bool KissLTE::loadCredentialsFromNVS() {
  if (!_nvs.begin(KISS_NVS_NAMESPACE, true)) {
    return false;
  }
  
  _nvs.getString(KISS_NVS_SIM_PIN_KEY, _pin, sizeof(_pin));
  _nvs.getString(KISS_NVS_LTE_APN_KEY, _apn, sizeof(_apn));
  _nvs.getString(KISS_NVS_LTE_USER_KEY, _apnUser, sizeof(_apnUser));
  _nvs.getString(KISS_NVS_LTE_PASS_KEY, _apnPass, sizeof(_apnPass));
  
  _nvs.end();
  
  if (strlen(_apn) > 0) {
    KISS_LOGF("APN cargado: %s", _apn);
  }
  
  return true;
}

bool KissLTE::saveCredentialsToNVS() {
  if (!_nvs.begin(KISS_NVS_NAMESPACE, false)) {
    return false;
  }
  
  if (strlen(_pin) > 0) {
    _nvs.putString(KISS_NVS_SIM_PIN_KEY, _pin);
  }
  
  if (strlen(_apn) > 0) {
    _nvs.putString(KISS_NVS_LTE_APN_KEY, _apn);
    _nvs.putString(KISS_NVS_LTE_USER_KEY, _apnUser);
    _nvs.putString(KISS_NVS_LTE_PASS_KEY, _apnPass);
  }
  
  _nvs.end();
  KISS_LOG("Credenciales guardadas en NVS");
  return true;
}

// ========== MÃ‰TODOS VIRTUALES KissClient ==========
bool KissLTE::connectToTelegram() {
  if (!isConnected()) {
    return false;
  }
  
  KISS_LOG("Conectando LTE a Telegram...");
  bool result = openSSLConnection("api.telegram.org", 443);
  
  if (result) {
    _connectionStartTime = millis();
    _errorCount = 0;
    KISS_LOG("LTE conectado a Telegram (SSL)");
  } else {
    _errorCount++;
    KISS_LOG("LTE no pudo conectar a Telegram");
  }
  
  return result;
}

bool KissLTE::connect(const char* host, uint16_t port) {
  return openSSLConnection(host, port);
}

bool KissLTE::connected() {
  return _sslConnected;
}

bool KissLTE::isConnected() {
  return _state == STATE_CONNECTED && checkNetworkRegistration();
}

void KissLTE::stop() {
  closeSSLConnection();
}

// Security methods
void KissLTE::setCACert(const char* rootCA) {
  KISS_LOG("Quectel: setCACert() - usando seclevel=0 (sin validaciÓn CA)");
}

void KissLTE::setInsecure() {
  _secureMode = false;
  KISS_LOG("Quectel: Modo inseguro (sin SSL) - NO RECOMENDADO");
}

bool KissLTE::verify(const char* fingerprint, const char* url) {
  return true;
}

void KissLTE::setSecureMode(bool secure) {
  _secureMode = secure;
}

bool KissLTE::isSecureMode() {
  return _secureMode;
}

// I/O methods
size_t KissLTE::print(const char* str) {
  if (!_sslConnected) return 0;
  int result = sslWrite((const uint8_t*)str, strlen(str));
  return (result > 0) ? result : 0;
}

size_t KissLTE::print(const String& str) {
  return print(str.c_str());
}

size_t KissLTE::println(const char* str) {
  size_t len = print(str);
  len += print("\r\n");
  return len;
}

size_t KissLTE::println(const String& str) {
  return println(str.c_str());
}

int KissLTE::available() {
  return sslAvailable();
}

int KissLTE::read() {
  uint8_t byte;
  if (sslRead(&byte, 1) == 1) {
    return byte;
  }
  return -1;
}

int KissLTE::read(uint8_t* buffer, size_t size) {
  return sslRead(buffer, size);
}

// ========== POWER MANAGEMENT ==========
bool KissLTE::setPowerMode(KissClientPowerMode mode) {
  updatePowerStatistics();
  _powerMode = mode;
  
  bool enteringSleep = (mode == CLIENT_POWER_LOW || 
                        mode == CLIENT_POWER_IDLE || 
                        mode == CLIENT_POWER_MAINTENANCE);
  
  _isInSleepMode = enteringSleep;
  
  switch (mode) {
    case CLIENT_POWER_BOOT:
      return setFunctionalityLevel(1);
      
    case CLIENT_POWER_LOW:
      return enterSleepMode(1);  // AT+QSCLK=1
      
    case CLIENT_POWER_IDLE:
      return enterSleepMode(0);  // Sin sleep durante inicialización
      
    case CLIENT_POWER_ACTIVE:
      if (!enterSleepMode(0)) return false;
      return setFunctionalityLevel(1);
      
    case CLIENT_POWER_TURBO:
      if (!enterSleepMode(0)) return false;
      return setFunctionalityLevel(1);
      
    case CLIENT_POWER_MAINTENANCE:
      return setFunctionalityLevel(0);  // AT+CFUN=0
      
    default:
      return false;
  }
}

KissClientPowerMode KissLTE::getPowerMode() {
  return _powerMode;
}

int KissLTE::getCurrentConsumption() {
  if (!_powered) return 0;
  
  switch (_powerMode) {
    case CLIENT_POWER_LOW:
      return (_state == STATE_CONNECTED) ? 10 : 5;
      
    case CLIENT_POWER_IDLE:
      return (_dtrPin >= 0) ? 12 : 18;
      
    case CLIENT_POWER_ACTIVE:
      if (_state == STATE_CONNECTED) {
        return _sslConnected ? 150 : 100;
      }
      return 80;
      
    case CLIENT_POWER_TURBO:
      return 200;
      
    case CLIENT_POWER_MAINTENANCE:
      return 8;
      
    default:
      return 50;
  }
}

// ========== INFORMACIÓN ==========
const char* KissLTE::getClientType() {
  return "LTE-Quectel";
}

const char* KissLTE::getConnectionInfo() {
  static char info[128];
  
  if (_state != STATE_CONNECTED) {
    snprintf(info, sizeof(info), "LTE: %s", getStateString());
    return info;
  }
  
  snprintf(info, sizeof(info), "LTE: %s | RSSI: %d dBm | SSL: %s",
           strlen(_operator) > 0 ? _operator : "Conectado",
           _signalStrength,
           _sslConnected ? "SI" : "NO");
  
  return info;
}

// ========== DIAGNÃ“STICOS ==========
bool KissLTE::isConnectionHealthy() {
  if (!isConnected()) return false;
  if (_state != STATE_CONNECTED) return false;
  if (_signalStrength < -100) return false;
  if (!checkNetworkRegistration()) return false;
  if (_sslConnected) { return true; }
  return true;
}

unsigned long KissLTE::getConnectionAge() {
  if (_connectionStartTime == 0 || !isConnected()) return 0;
  return millis() - _connectionStartTime;
}

int KissLTE::getErrorCount() {
  return _errorCount;
}

void KissLTE::resetErrorCount() {
  _errorCount = 0;
}

// ========== DIAGNÃ“STICOS ESPECÃFICOS ==========
void KissLTE::printInfo() {
  KISS_LOG("\n  KissLTE Info (Quectel):");
  KISS_LOGF("   Estado: %s", getStateString());
  KISS_LOGF("   Conectado: %s", isConnected() ? "SI" : "NO");
  KISS_LOGF("   SSL activo: %s", _sslConnected ? "SI" : "NO");
  KISS_LOGF("   Señal LTE: %d dBm", getSignalStrength());
  KISS_LOGF("   Consumo: %d mA", getCurrentConsumption());
  
  if (strlen(_operator) > 0) {
    KISS_LOGF("   Operador: %s", _operator);
  }
  if (strlen(_imei) > 0) {
    KISS_LOGF("   IMEI: %s", _imei);
  }
}

void KissLTE::printStatus() {
  KISS_LOG("\n========== ESTADO KissLTE ==========");
  KISS_LOGF("Estado: %s", getStateString());
  KISS_LOGF("Inicializado: %s", _initialized ? "Sí" : "NO");
  KISS_LOGF("Encendido: %s", _powered ? "SÍ" : "NO");
  KISS_LOGF("Conectado a red: %s", isConnected() ? "SÍ" : "NO");
  KISS_LOGF("SSL conectado: %s", _sslConnected ? "SÍ" : "NO");
  
  if (_signalStrength != -999) {
    KISS_LOGF("Señal: %d dBm", _signalStrength);
  }
  
  KISS_LOG("====================================\n");
}

void KissLTE::printModuleInfo() {
  KISS_LOG("\n========== INFO QUECTEL EC200A ==========");
  
  if (strlen(_imei) > 0) {
    KISS_LOGF("IMEI: %s", _imei);
  }
  
  if (strlen(_iccid) > 0) {
    KISS_LOGF("ICCID: %s", _iccid);
  }
  
  char operatorName[64];
  if (getOperator(operatorName, sizeof(operatorName))) {
    KISS_LOGF("Operador: %s", operatorName);
  }
  
  int rssi = getSignalStrength();
  if (rssi != -999) {
    KISS_LOGF("RSSI: %d dBm", rssi);
  }
  
  KISS_LOG("========================================\n");
}

// ========== GET STATE ==========
KissLTE::ConnectionState KissLTE::getState() {
    return _state;
}

// ========== GET STATE STRING ==========
const char* KissLTE::getStateString() {
  switch (_state) {
    case STATE_OFF: return "Apagado";
    case STATE_POWERING_ON: return "Encendiendo";
    case STATE_INITIALIZING: return "Inicializando";
    case STATE_REGISTERING: return "Registrando";
    case STATE_CONNECTED: return "Conectado";
    case STATE_ERROR: return "Error";
    default: return "Desconocido";
  }
}

// ========== CONFIGURACIÓN ==========
void KissLTE::setDebug(bool enable) {
  _debug = enable;
}

void KissLTE::setTimeout(unsigned long timeout) {
  _timeout = timeout;
  if (_serial) {
    _serial->setTimeout(timeout);
  }
}

unsigned long KissLTE::getTimeout() {
  return _timeout;
}

// ========== ESTADÍSTICAS SLEEP/ACTIVO ==========
void KissLTE::updatePowerStatistics() {
  unsigned long now = millis();
  unsigned long elapsed = now - _lastModeChange;
  
  if (_isInSleepMode) {
    _timeInSleep += elapsed;
  } else {
    _timeInActive += elapsed;
  }
  
  _lastModeChange = now;
}

unsigned long KissLTE::getTimeInSleepMode() {
  unsigned long now = millis();
  unsigned long currentElapsed = now - _lastModeChange;
  
  if (_isInSleepMode) {
    return _timeInSleep + currentElapsed;
  }
  return _timeInSleep;
}

unsigned long KissLTE::getTimeInActiveMode() {
  unsigned long now = millis();
  unsigned long currentElapsed = now - _lastModeChange;
  
  if (!_isInSleepMode) {
    return _timeInActive + currentElapsed;
  }
  return _timeInActive;
}

unsigned long KissLTE::getCurrentModeTime() {
  return millis() - _lastModeChange;
}

float KissLTE::getSleepEfficiency() {
  unsigned long totalSleep = getTimeInSleepMode();
  unsigned long totalActive = getTimeInActiveMode();
  unsigned long totalTime = totalSleep + totalActive;
  
  if (totalTime == 0) return 0.0f;
  return (totalSleep * 100.0f) / totalTime;
}

void KissLTE::resetPowerStatistics() {
  _timeInSleep = 0;
  _timeInActive = 0;
  _lastModeChange = millis();
}

// ========== WATCHDOG Y RECOVERY ==========
bool KissLTE::healthCheck() {
  if (sendATCommand("AT", "OK", 1000)) {
    _lastSuccessfulComm = millis();
    _healthCheckFailures = 0;
    return true;
  }
  
  _healthCheckFailures++;
  return false;
}

void KissLTE::updateHealthStatus() {
  if (!_powered) return;
  
  unsigned long now = millis();
  if (now - _lastHealthCheck < 30000) return;
  
  _lastHealthCheck = now;
  
  if (!healthCheck()) {
    if (_healthCheckFailures >= MAX_HEALTH_FAILURES) {
      KISS_LOG("Módulo no responde, recovery necesario");
    }
  }
}

bool KissLTE::isHealthy() {
  if (!_powered) return false;
  
  unsigned long timeSinceLastComm = millis() - _lastSuccessfulComm;
  if (timeSinceLastComm > 300000) return false;
  
  return _healthCheckFailures < MAX_HEALTH_FAILURES;
}

unsigned long KissLTE::getLastSuccessfulComm() {
  return _lastSuccessfulComm;
}

void KissLTE::resetHealthMonitor() {
  _lastSuccessfulComm = millis();
  _lastHealthCheck = millis();
  _healthCheckFailures = 0;
}

bool KissLTE::softReset() {
  KISS_LOG("Soft reset Quectel...");
  
  // Quectel: AT+CRESET
  KISS_LOG("Intentando AT+CRESET...");
  if (sendATCommand("AT+CRESET", "OK", 5000)) {
    setState(STATE_INITIALIZING);
    SAFE_DELAY(3000);
    
    if (waitForModuleReady(10000)) {
      resetHealthMonitor();
      KISS_LOG("Reset exitoso");
      return initializeModule();
    }
  }
  
  KISS_LOG("Soft reset falló");
  return false;
}

bool KissLTE::hardReset() {
  KISS_LOG("Hard reset Quectel (PWRKEY 12s)...");
  
  digitalWrite(_pwrKeyPin, HIGH);
  SAFE_DELAY(12000);
  digitalWrite(_pwrKeyPin, LOW);
  
  setState(STATE_POWERING_ON);
  SAFE_DELAY(5000);
  
  if (waitForModuleReady(15000)) {
    _powered = true;
    resetHealthMonitor();
    KISS_LOG("Hard reset exitoso");
    return initializeModule();
  }
  
  KISS_LOG("Hard reset falló");
  return false;
}

bool KissLTE::recoveryReset() {
  KISS_LOG("RECOVERY QUECTEL...");
  
  // Nivel 1: Soft Reset
  KISS_LOG("Nivel 1: Soft reset");
  if (softReset()) {
    KISS_LOG("Recovery con soft reset");
    return true;
  }
  
  SAFE_DELAY(2000);
  
  // Nivel 2: Hard Reset
  KISS_LOG("Nivel 2: Hard reset");
  if (hardReset()) {
    KISS_LOG("Recovery con hard reset");
    return true;
  }
  
  SAFE_DELAY(2000);
  
  // Nivel 3: Power Cycle
  KISS_LOG("Nivel 3: Power cycle");
  if (reset()) {
    resetHealthMonitor();
    KISS_LOG("Recovery con power cycle");
    return true;
  }
  
  KISS_LOG("RECOVERY FALLÓ");
  return false;
}

// ========== POWER MANAGEMENT AT COMMANDS ==========
bool KissLTE::enterSleepMode(int mode) {
  if (mode < 0 || mode > 2) return false;
  
  char cmd[32];
  snprintf(cmd, sizeof(cmd), "AT+QSCLK=%d", mode);
  
  if (sendATCommand(cmd, "OK", 2000)) {
    KISS_LOGF("Sleep mode: %d", mode);
    return true;
  }
  
  return false;
}

bool KissLTE::setFunctionalityLevel(int level) {
  if (level < 0 || level > 4) return false;
  
  char cmd[32];
  snprintf(cmd, sizeof(cmd), "AT+CFUN=%d", level);
  
  if (sendATCommand(cmd, "OK", 5000)) {
    return true;
  }
  
  return false;
}

bool KissLTE::powerDownModule() {
  return sendATCommand("AT+QPOWD=1", "OK", 5000);
}

// ========== DTR CONTROL ==========
void KissLTE::setDTR(bool active) {
  if (_dtrPin < 0) return;
  digitalWrite(_dtrPin, active ? LOW : HIGH);
}

void KissLTE::wakeFromSleep() {
  if (_dtrPin < 0) return;
  
  digitalWrite(_dtrPin, LOW);
  delay(80);
  digitalWrite(_dtrPin, HIGH);
  
  if (_debug) {
    KISS_LOG("Módulo despertado");
  }
}

// ========== TCP/IP WRAPPERS (para compatibilidad) ==========
bool KissLTE::openTCPConnection(const char* host, int port) {
  // En Quectel, siempre usamos SSL
  return openSSLConnection(host, port);
}

bool KissLTE::closeTCPConnection() {
  return closeSSLConnection();
}

bool KissLTE::isTCPConnected() {
  return _sslConnected;
}

int KissLTE::tcpWrite(const uint8_t* data, size_t len) {
  return sslWrite(data, len);
}

int KissLTE::tcpRead(uint8_t* buffer, size_t len) {
  return sslRead(buffer, len);
}

int KissLTE::tcpAvailable() {
  return sslAvailable();
}

// ========== HELPERS ==========
void KissLTE::setState(ConnectionState newState) {
  _state = newState;
}

void KissLTE::keepalive() {
  if (!_powered || (_state != STATE_CONNECTED && _state != STATE_REGISTERING)) {
    return;
  }
  
  unsigned long now = millis();
  if (now - _lastKeepalive < KEEPALIVE_INTERVAL) {
    return;
  }
  
  if (sendATCommand("AT", "OK", 1000)) {
    _lastKeepalive = now;
    _lastSuccessfulComm = now;
  }
}

bool KissLTE::expectResponse(const char* expected, unsigned long timeout) {
  return waitForResponse(expected, timeout);
}

// ========== TCP/IP MÉTODOS (NO USAR, usar SSL directamente) ==========
// Estos mÃ©todos están aquí­ solo para compatibilidad con la interfaz
// Pero en Quectel EC200A siempre usamos SSL nativo
// ========== SSL MEJORADO ==========
bool KissLTE::querySSLState() {
  if (!_sslConnected) return false;

  char response[128];
  if (readATResponse("AT+QSSLSTATE=0", response, sizeof(response), 2000)) {
    if (strstr(response, "CONNECTED")) {
      return true;
    }
  }

  _sslConnected = false;
  _tcpConnected = false;
  return false;
}

void KissLTE::logLastError() {
  char response[128];
  if (readATResponse("AT+QIGETERROR", response, sizeof(response), 2000)) {
    KISS_LOGF("Error: %s", response);
  }
}

void KissLTE::logSSLError(int err) {
  const char* msg;
  switch (err) {
    case 550: msg = "Socket ocupado"; break;
    case 551: msg = "Socket invalido"; break;
    case 552: msg = "Sin memoria"; break;
    case 553: msg = "DNS fallo"; break;
    case 554: msg = "TCP fallo"; break;
    case 555: msg = "SSL handshake fallo"; break;
    case 556: msg = "Timeout"; break;
    case 557: msg = "Certificado invalido"; break;
    case 558: msg = "Certificado expirado"; break;
    case 559: msg = "Cert no confiable"; break;
    default:  msg = "Desconocido"; break;
  }
  KISS_LOGF("SSL[%d]: %s", err, msg);
}

void KissLTE::sendKeepalive() {
  if (!_sslConnected) return;

  unsigned long now = millis();
  // Cada 5 minutos
  if (now - _lastKeepalive < KEEPALIVE_INTERVAL) return;

  if (querySSLState()) {
    _lastKeepalive = now;
    _lastSuccessfulComm = now;
    if (_debug) {
      KISS_LOG("Keepalive OK");
    }
  } else {
    KISS_LOG("Keepalive fallo - conexion perdida");
  }
}

bool KissLTE::closeSSLConnectionFast() {
  if (!_sslConnected) return true;

  KISS_LOG("Cerrando SSL (fast)...");

  // timeout=0 = cierre inmediato sin esperar ACK
  sendATCommand("AT+QSSLCLOSE=0,0", "OK", 5000);

  _sslConnected = false;
  _tcpConnected = false;
  KISS_LOG("SSL cerrado");
  return true;
}

bool KissLTE::openSSLConnectionWithRetry(const char* host, int port, int maxRetries) {
  for (int attempt = 1; attempt <= maxRetries; attempt++) {
    KISS_LOGF("SSL intento %d/%d", attempt, maxRetries);

    unsigned long start = millis();

    if (openSSLConnection(host, port)) {
      unsigned long elapsed = millis() - start;
      KISS_LOGF("SSL OK en %lums", elapsed);
      return true;
    }

    if (attempt < maxRetries) {
      // Backoff exponencial: 2s, 4s, 8s...
      unsigned long backoff = (1UL << attempt) * 1000UL;
      if (backoff > 10000) backoff = 10000;
      KISS_LOGF("Reintento en %lums", backoff);
      SAFE_DELAY(backoff);

      if (!checkAndRecover()) {
        KISS_LOG("Red perdida durante reintentos");
        return false;
      }
    }
  }

  KISS_LOG("SSL fallo tras todos los intentos");
  return false;
}

// ========== GNSS (GPS/Galileo/GLONASS/BeiDou) ==========
bool KissLTE::initGNSS() {
  KISS_LOG("Iniciando GNSS...");

  // 1. Configurar constelaciones (GPS + Galileo para Europa)
  if (!sendATCommand("AT+QGPSCFG=\"gnssconfig\",4", "OK", 2000)) {
    KISS_LOG("No se pudo configurar constelaciones");
  }

  // 2. Salida NMEA por puerto AT
  sendATCommand("AT+QGPSCFG=\"outport\",\"uartdebug\"", "OK", 2000);

  // 3. Habilitar adquisicion via AT+QGPSGNMEA
  sendATCommand("AT+QGPSCFG=\"nmeasrc\",1", "OK", 2000);

  // 4. Tipo de sentencias (GGA+RMC)
  sendATCommand("AT+QGPSCFG=\"gpsnmeatype\",3", "OK", 2000);

  // 5. Encender GNSS (modo continuo)
  if (!sendATCommand("AT+QGPS=1", "OK", 5000)) {
    KISS_LOG("No se pudo encender GNSS");
    return false;
  }

  KISS_LOG("GNSS iniciado");
  return true;
}

bool KissLTE::getGNSSLocation(float* lat, float* lon, float* alt, float* speed) {
  if (!lat || !lon) return false;

  char response[256];
  if (!readATResponse("AT+QGPSLOC=2", response, sizeof(response), 5000)) {
    return false;
  }

  char* loc = strstr(response, "+QGPSLOC:");
  if (!loc) {
    if (strstr(response, "516")) {
      KISS_LOG("GNSS buscando satelites...");
    }
    return false;
  }

  char utc[16], latStr[16], lonStr[16];
  float hdop, altitude, cog, spkm;
  int fix;

  if (sscanf(loc, "+QGPSLOC: %[^,],%[^,],%[^,],%f,%f,%d,%f,%f",
             utc, latStr, lonStr, &hdop, &altitude, &fix, &cog, &spkm) >= 6) {

    *lat = nmeaToDecimal(latStr);
    *lon = nmeaToDecimal(lonStr);

    if (alt) *alt = altitude;
    if (speed) *speed = spkm;

    return true;
  }

  return false;
}

float KissLTE::nmeaToDecimal(const char* nmea) {
  if (!nmea || strlen(nmea) < 6) return 0.0f;

  float value = atof(nmea);
  int degrees = (int)(value / 100);
  float minutes = value - (degrees * 100);
  float decimal = degrees + (minutes / 60.0f);

  char hem = nmea[strlen(nmea) - 1];
  if (hem == 'S' || hem == 'W') {
    decimal = -decimal;
  }

  return decimal;
}

void KissLTE::stopGNSS() {
  sendATCommand("AT+QGPSEND", "OK", 2000);
  KISS_LOG("GNSS detenido");
}

bool KissLTE::enableAGPS() {
  sendATCommand("AT+QGPSCFG=\"agpsposmode\",33554432", "OK", 2000);
  return sendATCommand("AT+QAGPS=1", "OK", 5000);
}
