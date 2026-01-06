// KissLTE.cpp
// Vicente Soriano - victek@gmail.com
// Implementación del gestor LTE genérico con lazy initialization

#include "KissLTE.h"
#include "modules/QuectelModule.h"
#include "modules/SIMCOMModule.h"

// ========== CONSTRUCTOR/DESTRUCTOR ==========

KissLTE::KissLTE()
  : _serial(nullptr),
    _rxPin(-1),
    _txPin(-1),
    _pwrKeyPin(-1),
    _statusPin(-1),
    _resetPin(-1),
    _dtrPin(-1),
    _module(nullptr),
    _moduleDetected(false),
    _state(STATE_OFF),
    _initialized(false),
    _debug(KISS_DEVELOPMENT_MODE),
    _hardwareAvailable(false),
    _consecutiveFailures(0) {

  memset(_apn, 0, sizeof(_apn));
  memset(_apnUser, 0, sizeof(_apnUser));
  memset(_apnPass, 0, sizeof(_apnPass));

  // Inicializar variables heredadas de KissClient
  _connectionStartTime = 0;
  _errorCount = 0;
  _powerMode = CLIENT_POWER_BOOT;
}

KissLTE::~KissLTE() {
  end();
}

// ========== INICIALIZACIÓN (LAZY) ==========

bool KissLTE::begin(HardwareSerial* serial, int rxPin, int txPin,
                    int pwrKeyPin, int statusPin, int resetPin, int dtrPin) {
  if (_initialized) {
    KISS_LOG("KissLTE ya inicializado");
    return true;
  }

  KISS_LOG("Preparando hardware LTE (sin encender módulo)...");

  _serial = serial;
  _rxPin = rxPin;
  _txPin = txPin;
  _pwrKeyPin = pwrKeyPin;
  _statusPin = statusPin;
  _resetPin = resetPin;
  _dtrPin = dtrPin;

  // Configurar pines de control
  pinMode(_pwrKeyPin, OUTPUT);
  digitalWrite(_pwrKeyPin, LOW);  // Estado inicial

  if (_statusPin >= 0) {
    pinMode(_statusPin, INPUT);
    KISS_LOGF("STATUS_PIN configurado en GPIO%d", _statusPin);
  }

  if (_resetPin >= 0) {
    pinMode(_resetPin, OUTPUT);
    digitalWrite(_resetPin, HIGH);  // RESET inactivo
    KISS_LOGF("RESET_PIN configurado en GPIO%d", _resetPin);
  }

  if (_dtrPin >= 0) {
    pinMode(_dtrPin, OUTPUT);
    digitalWrite(_dtrPin, LOW);
    KISS_LOGF("DTR_PIN configurado en GPIO%d", _dtrPin);
  }

  // Configurar UART (NO enviar comandos aún)
  _serial->begin(115200, SERIAL_8N1, _rxPin, _txPin);
  _serial->setTimeout(2000);

  KISS_LOGF("UART configurado: RX=%d, TX=%d, PWRKEY=%d", _rxPin, _txPin, _pwrKeyPin);

  // Cargar credenciales desde NVS
  loadCredentialsFromNVS();

  // Verificar si el módulo está apagado
  if (isHardwarePowered()) {
    KISS_LOG("⚠ Módulo LTE ya estaba encendido");
    // No apagarlo automáticamente, dejarlo como está
  } else {
    KISS_LOG("✓ Módulo LTE verificado apagado (STATUS=LOW)");
  }

  _initialized = true;
  setState(STATE_OFF);

  KISS_LOG("✓ Hardware LTE preparado (lazy init - módulo apagado)");
  return true;
}

void KissLTE::end() {
  if (!_initialized) return;

  // Apagar módulo si está encendido
  if (isHardwarePowered()) {
    powerOff();
  }

  // Liberar módulo
  destroyModule();

  // Cerrar UART
  if (_serial) {
    _serial->end();
  }

  _initialized = false;
  setState(STATE_OFF);
  KISS_LOG("KissLTE detenido");
}

// ========== CONTROL DE HARDWARE ==========

bool KissLTE::isHardwarePowered() {
  if (_statusPin < 0) {
    // Sin STATUS_PIN, intentar detectar por AT
    if (_module) {
      return _module->sendATCommand("AT", "OK", 500);
    }
    return false;
  }

  return (digitalRead(_statusPin) == HIGH);
}

bool KissLTE::isHardwareAvailable() const {
  return _hardwareAvailable;
}

bool KissLTE::powerOn() {
  if (!_initialized) {
    KISS_LOG("✗ KissLTE no inicializado");
    return false;
  }

  // Verificar si ya se alcanzó el límite de fallos
  if (_consecutiveFailures >= MAX_CONSECUTIVE_FAILURES) {
    KISS_LOG("✗ LTE deshabilitado tras múltiples fallos");
    return false;
  }

  // Verificar si ya está encendido
  if (isHardwarePowered()) {
    KISS_LOG("Módulo LTE ya encendido");

    // Si no tenemos módulo detectado, detectarlo ahora
    if (!_moduleDetected) {
      bool success = detectModule() && createModule();
      if (success) {
        _hardwareAvailable = true;
        _consecutiveFailures = 0;
      } else {
        _consecutiveFailures++;
      }
      return success;
    }

    return true;
  }

  KISS_LOG("Encendiendo módulo LTE...");
  setState(STATE_POWERING_ON);

  // Detectar módulo intentando respuesta rápida (SIMCOM)
  // SIMCOM responde en 400-600ms, Quectel tarda más
  bool detectedBeforePowerOn = detectModule();

  if (!detectedBeforePowerOn) {
    KISS_LOG("Módulo apagado, procediendo a encender...");

    // Intentar encender (pulso genérico que funciona para ambos)
    // SIMCOM: HIGH 1.2s, Quectel: LOW 1.5s
    // Usamos LOW 1.5s que es compatible con ambos
    digitalWrite(_pwrKeyPin, LOW);
    delay(1500);
    digitalWrite(_pwrKeyPin, HIGH);
    delay(3000);  // Esperar arranque

    // Verificar STATUS_PIN
    if (_statusPin >= 0) {
      unsigned long start = millis();
      while (millis() - start < 10000) {
        if (digitalRead(_statusPin) == HIGH) {
          KISS_LOG("✓ Módulo encendido (STATUS=HIGH)");
          break;
        }
        delay(100);
      }

      if (!isHardwarePowered()) {
        KISS_LOG("✗ Módulo no encendió (STATUS=LOW)");
        setState(STATE_ERROR);
        _consecutiveFailures++;
        return false;
      }
    }
  }

  // Ahora detectar tipo de módulo
  setState(STATE_INITIALIZING);

  if (!_moduleDetected) {
    if (!detectModule()) {
      KISS_LOG("✗ No se pudo detectar tipo de módulo");
      setState(STATE_ERROR);
      _consecutiveFailures++;
      return false;
    }
  }

  // Crear instancia del módulo específico
  if (!createModule()) {
    KISS_LOG("✗ No se pudo crear módulo");
    setState(STATE_ERROR);
    _consecutiveFailures++;
    return false;
  }

  KISS_LOGF("✓ Módulo LTE encendido: %s", getModuleName());
  setState(STATE_CONNECTED);
  _hardwareAvailable = true;
  _consecutiveFailures = 0;  // Resetear contador tras éxito
  return true;
}

bool KissLTE::powerOff() {
  if (!_initialized) return true;

  KISS_LOG("Apagando módulo LTE...");

  // Si tenemos módulo, usar su método de apagado
  if (_module) {
    bool result = _module->powerOff();
    destroyModule();

    if (result) {
      setState(STATE_OFF);
      return true;
    }
  }

  // Fallback: pulso largo PWRKEY (2.5s)
  digitalWrite(_pwrKeyPin, LOW);
  delay(2500);
  digitalWrite(_pwrKeyPin, HIGH);
  delay(2000);

  // Verificar STATUS_PIN
  if (_statusPin >= 0) {
    unsigned long start = millis();
    while (millis() - start < 5000) {
      if (digitalRead(_statusPin) == LOW) {
        KISS_LOG("✓ Módulo apagado (STATUS=LOW)");
        setState(STATE_OFF);
        return true;
      }
      delay(100);
    }
    KISS_LOG("⚠ STATUS_PIN todavía HIGH después de apagar");
  }

  setState(STATE_OFF);
  return true;
}

bool KissLTE::hardReset() {
  KISS_LOG("Hard reset módulo LTE...");

  if (_module) {
    bool result = _module->hardReset();
    if (result) {
      setState(STATE_CONNECTED);
      return true;
    }
  }

  // Fallback: usar RESET_PIN si está disponible
  if (_resetPin >= 0) {
    digitalWrite(_resetPin, LOW);
    delay(300);
    digitalWrite(_resetPin, HIGH);
    delay(3000);
    return powerOn();
  }

  // Sin RESET_PIN, usar pulso largo PWRKEY (12s)
  digitalWrite(_pwrKeyPin, LOW);
  delay(12000);
  digitalWrite(_pwrKeyPin, HIGH);
  delay(3000);

  return powerOn();
}

// ========== DETECCIÓN DE MÓDULO ==========

bool KissLTE::detectModule() {
  if (_moduleDetected) return true;

  KISS_LOG("Detectando tipo de módulo LTE...");

  // Limpiar buffer serial
  while (_serial->available()) _serial->read();
  delay(500);

  // Enviar AT+GMM (obtener modelo)
  // SIMCOM responde MUY rápido (400-600ms)
  // Quectel tarda más (1-2s)

  _serial->println("AT+GMM");
  unsigned long start = millis();
  char response[256];
  int idx = 0;
  bool gotResponse = false;

  while (millis() - start < 2000) {
    if (_serial->available()) {
      char c = _serial->read();
      if (c != '\r' && idx < sizeof(response) - 1) {
        response[idx++] = c;
        response[idx] = '\0';  // Mantener null-terminated
      }

      if (strstr(response, "OK") || strstr(response, "SIMCOM") ||
          strstr(response, "Quectel") || strstr(response, "A767") ||
          strstr(response, "EC200")) {
        gotResponse = true;
        break;
      }
    }
    delay(10);
  }

  unsigned long responseTime = millis() - start;

  if (!gotResponse) {
    KISS_LOG("✗ Módulo no responde a AT+GMM");
    return false;
  }

  // Analizar respuesta y tiempo
  if (strstr(response, "SIMCOM") || strstr(response, "A767") || responseTime < 1000) {
    KISS_LOG("✓ Detectado: SIMCOM (respuesta rápida)");
    _moduleDetected = true;
    return true;
  }

  if (strstr(response, "Quectel") || strstr(response, "EC200")) {
    KISS_LOG("✓ Detectado: Quectel");
    _moduleDetected = true;
    return true;
  }

  KISS_LOGF("⚠ Respuesta ambigua (tiempo=%lums): %s", responseTime, response);

  // Por defecto asumir Quectel si no está claro
  KISS_LOG("Asumiendo Quectel por defecto");
  _moduleDetected = true;
  return true;
}

bool KissLTE::createModule() {
  if (_module) {
    KISS_LOG("✓ Módulo ya creado");
    return true;
  }

  // Enviar AT+GMM para identificar
  _serial->println("AT+GMM");
  delay(500);

  char response[256];
  int idx = 0;
  while (_serial->available() && idx < sizeof(response) - 1) {
    char c = _serial->read();
    if (c != '\r') {
      response[idx++] = c;
    }
  }
  response[idx] = '\0';

  if (strstr(response, "SIMCOM") || strstr(response, "A767")) {
    KISS_LOG("Creando SIMCOMModule...");
    _module = new SIMCOMModule(_serial, _pwrKeyPin, _statusPin, _resetPin, _dtrPin);
  } else {
    KISS_LOG("Creando QuectelModule...");
    _module = new QuectelModule(_serial, _pwrKeyPin, _statusPin, _resetPin, _dtrPin);
  }

  if (!_module) {
    KISS_LOG("✗ Error creando módulo");
    return false;
  }

  // Inicializar módulo
  if (!_module->initialize()) {
    KISS_LOG("✗ Error inicializando módulo");
    destroyModule();
    return false;
  }

  // Configurar APN si está definido
  if (strlen(_apn) > 0) {
    _module->configureAPN(_apn, _apnUser, _apnPass);
  }

  KISS_LOGF("✓ Módulo creado e inicializado: %s", _module->getModuleName());
  return true;
}

void KissLTE::destroyModule() {
  if (_module) {
    delete _module;
    _module = nullptr;
    _moduleDetected = false;
    KISS_LOG("Módulo liberado");
  }
}

// ========== LOOP Y MANTENIMIENTO ==========

void KissLTE::loop() {
  // Solo procesar si el módulo está encendido y creado
  if (!_initialized || !_module) return;
  if (!isHardwarePowered()) return;

  // Procesar URCs del módulo
  static char line[256];
  static int idx = 0;

  while (_serial->available()) {
    char c = _serial->read();
    if (c == '\n') {
      line[idx] = '\0';
      if (idx > 0) {
        _module->processURC(String(line));  // processURC espera String, conversión temporal
      }
      idx = 0;
    } else if (c != '\r' && idx < sizeof(line) - 1) {
      line[idx++] = c;
    }
  }
}

bool KissLTE::healthCheck() {
  if (!_module) return false;
  return _module->sendATCommand("AT", "OK", 1000);
}

bool KissLTE::ensureConnected() {
  if (!_module) return false;

  // Verificar PDP activo
  if (!_module->isPDPActive()) {
    KISS_LOG("PDP inactivo, reactivando...");
    return _module->activatePDP();
  }

  return true;
}

// ========== MÉTODOS VIRTUALES KissClient ==========

bool KissLTE::connectToTelegram() {
  if (!_module) {
    KISS_LOG("✗ Módulo no inicializado");
    return false;
  }

  return _module->openSSLConnection("api.telegram.org", 443);
}

bool KissLTE::connect(const char* host, uint16_t port) {
  if (!_module) return false;
  return _module->openSSLConnection(host, port);
}

bool KissLTE::connected() {
  if (!_module) return false;
  return _module->isSSLConnected();
}

bool KissLTE::isConnected() {
  if (!_module) return false;
  return _module->isPDPActive() && _module->isSSLConnected();
}

void KissLTE::disconnect() {
  if (_module) {
    _module->closeSSLConnection();
  }
}

void KissLTE::stop() {
  powerOff();
}

void KissLTE::setCACert(const char* rootCA) {
  if (_module) {
    _module->uploadCACertificate(rootCA);
  }
}

void KissLTE::setInsecure() {
  // Los módulos ya usan authmode=0 por defecto
}

bool KissLTE::verify(const char* fingerprint, const char* url) {
  return true;  // No implementado
}

void KissLTE::setSecureMode(bool secure) {
  // Siempre en modo seguro
}

bool KissLTE::isSecureMode() {
  return true;
}

size_t KissLTE::print(const char* str) {
  if (!_module || !_module->isSSLConnected()) return 0;
  return _module->sslWrite((const uint8_t*)str, strlen(str));
}

size_t KissLTE::print(const String& str) {
  return print(str.c_str());
}

size_t KissLTE::println(const char* str) {
  size_t n = print(str);
  n += print("\r\n");
  return n;
}

size_t KissLTE::println(const String& str) {
  return println(str.c_str());
}

int KissLTE::available() {
  if (!_module) return 0;
  return _module->sslAvailable();
}

int KissLTE::read() {
  if (!_module) return -1;
  uint8_t b;
  if (_module->sslRead(&b, 1) == 1) {
    return b;
  }
  return -1;
}

int KissLTE::read(uint8_t* buffer, size_t size) {
  if (!_module) return 0;
  return _module->sslRead(buffer, size);
}

void KissLTE::printInfo() {
  KISS_LOGF("KissLTE: %s, Estado: %s",
           getModuleName(), getStateString());
}

// ========== POWER MANAGEMENT ==========

bool KissLTE::setPowerMode(KissClientPowerMode mode) {
  _powerMode = mode;

  if (!_module) return false;

  // Mapear a sleep mode del módulo
  switch (mode) {
    case CLIENT_POWER_LOW:
    case CLIENT_POWER_MAINTENANCE:
      return _module->enterSleepMode(1);
    case CLIENT_POWER_ACTIVE:
    case CLIENT_POWER_TURBO:
      return _module->wakeUp();
    default:
      return true;
  }
}

KissClientPowerMode KissLTE::getPowerMode() {
  return _powerMode;
}

// ========== INFORMACIÓN ==========

const char* KissLTE::getClientType() {
  if (_module) {
    return _module->getManufacturer();
  }
  return "LTE";
}

int KissLTE::getSignalStrength() {
  if (!_module) return -1;
  return _module->getSignalStrength();
}

const char* KissLTE::getConnectionInfo() {
  static char info[64];
  snprintf(info, sizeof(info), "%s - %s", getModuleName(), getStateString());
  return info;
}

// ========== DIAGNÓSTICOS ==========

bool KissLTE::isConnectionHealthy() {
  return healthCheck();
}

unsigned long KissLTE::getConnectionAge() {
  if (_connectionStartTime == 0) return 0;
  return millis() - _connectionStartTime;
}

int KissLTE::getErrorCount() {
  return _errorCount;
}

void KissLTE::resetErrorCount() {
  _errorCount = 0;
}

// ========== MÉTODOS ESPECÍFICOS LTE ==========

const char* KissLTE::getStateString() {
  switch (_state) {
    case STATE_OFF: return "OFF";
    case STATE_POWERING_ON: return "POWERING_ON";
    case STATE_INITIALIZING: return "INITIALIZING";
    case STATE_REGISTERING: return "REGISTERING";
    case STATE_CONNECTED: return "CONNECTED";
    case STATE_ERROR: return "ERROR";
    default: return "UNKNOWN";
  }
}

const char* KissLTE::getModuleName() {
  if (_module) {
    return _module->getModuleName();
  }
  return "Unknown";
}

bool KissLTE::setAPN(const char* apn, const char* user, const char* pass) {
  if (!apn) return false;

  strncpy(_apn, apn, sizeof(_apn) - 1);
  if (user) strncpy(_apnUser, user, sizeof(_apnUser) - 1);
  if (pass) strncpy(_apnPass, pass, sizeof(_apnPass) - 1);

  saveCredentialsToNVS();

  if (_module) {
    return _module->configureAPN(_apn, _apnUser, _apnPass);
  }

  return true;
}

bool KissLTE::getIMEI(char* imei, size_t size) {
  if (!_module) return false;
  return _module->getIMEI(imei, size);
}

bool KissLTE::getOperator(char* operatorName, size_t size) {
  if (!_module) return false;
  return _module->getOperator(operatorName, size);
}

// ========== GNSS ==========

bool KissLTE::initGNSS() {
  if (!_module) return false;

  // Usar getType() en lugar de dynamic_cast (sin RTTI)
  if (_module->getType() == MODULE_QUECTEL) {
    QuectelModule* quectel = static_cast<QuectelModule*>(_module);
    return quectel->enableGNSS();
  }

  if (_module->getType() == MODULE_SIMCOM) {
    SIMCOMModule* simcom = static_cast<SIMCOMModule*>(_module);
    return simcom->enableGNSS();
  }

  return false;
}

bool KissLTE::getGNSSLocation(float* lat, float* lon, float* alt) {
  if (!_module) return false;

  if (_module->getType() == MODULE_QUECTEL) {
    QuectelModule* quectel = static_cast<QuectelModule*>(_module);
    return quectel->getGNSSLocation(lat, lon, alt);
  }

  if (_module->getType() == MODULE_SIMCOM) {
    SIMCOMModule* simcom = static_cast<SIMCOMModule*>(_module);
    return simcom->getGNSSLocation(lat, lon, alt);
  }

  return false;
}

void KissLTE::stopGNSS() {
  if (!_module) return;

  if (_module->getType() == MODULE_QUECTEL) {
    QuectelModule* quectel = static_cast<QuectelModule*>(_module);
    quectel->disableGNSS();
    return;
  }

  if (_module->getType() == MODULE_SIMCOM) {
    SIMCOMModule* simcom = static_cast<SIMCOMModule*>(_module);
    simcom->disableGNSS();
    return;
  }
}

// ========== CONFIGURACIÓN ==========

void KissLTE::setDebug(bool enable) {
  _debug = enable;
}

// ========== HELPERS PRIVADOS ==========

bool KissLTE::loadCredentialsFromNVS() {
  if (!_nvs.begin(KISS_NVS_NAMESPACE, true)) {
    return false;
  }

  _nvs.getString(KISS_NVS_LTE_APN_KEY, _apn, sizeof(_apn));
  _nvs.getString(KISS_NVS_LTE_USER_KEY, _apnUser, sizeof(_apnUser));
  _nvs.getString(KISS_NVS_LTE_PASS_KEY, _apnPass, sizeof(_apnPass));

  _nvs.end();

  // Usar valores por defecto si están vacíos
  if (strlen(_apn) == 0) {
    strncpy(_apn, KISS_LTE_APN, sizeof(_apn) - 1);
    strncpy(_apnUser, KISS_LTE_USER, sizeof(_apnUser) - 1);
    strncpy(_apnPass, KISS_LTE_PASS, sizeof(_apnPass) - 1);
  }

  return true;
}

bool KissLTE::saveCredentialsToNVS() {
  if (!_nvs.begin(KISS_NVS_NAMESPACE, false)) {
    return false;
  }

  _nvs.putString(KISS_NVS_LTE_APN_KEY, _apn);
  _nvs.putString(KISS_NVS_LTE_USER_KEY, _apnUser);
  _nvs.putString(KISS_NVS_LTE_PASS_KEY, _apnPass);

  _nvs.end();
  return true;
}

void KissLTE::setState(ConnectionState newState) {
  if (_state != newState) {
    _state = newState;
    if (_debug) {
      KISS_LOGF("Estado LTE: %s", getStateString());
    }
  }
}
