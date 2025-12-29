// KissSSL.cpp
// Vicente Soriano - victek@gmail.com
// Wrapper SSL para Telegram con keep-alive

#include "KissSSL.h"
#include "KissTime.h"
#include "kiss_setup.h"
#include "lang.h"

KissSSL::KissSSL()
  : client(nullptr), manuallyConnected(false), secureMode(false) {
  client = new WiFiClientSecure();
  _connectionStartTime = 0;
  _errorCount = 0;
  _powerMode = CLIENT_POWER_BOOT;
}

KissSSL::~KissSSL() {
  if (client) {
    client->stop();
    delete client;
    client = nullptr;
  }
}

bool KissSSL::connectToTelegram() {
  if (isConnected()) {
    return true;
  }

  disconnect();

  if (!client) {
    client = new WiFiClientSecure();
  }

  if (secureMode) {
    KISS_LOG("🔒 Modo Seguro: Validación SSL + Hibrido");
    client->setCACert(TELEGRAM_ROOT_CA);
    client->setInsecure();  // Valida cert pero NO hostname
  } else {
    KISS_LOG("⚡ Modo Inseguro: Sin validación SSL");
    client->setInsecure();
  }

  KISS_LOG("🔌 Conectando a api.telegram.org:443...");
  unsigned long start = millis();
  bool result = client->connect("api.telegram.org", 443);
  unsigned long duration = millis() - start;

  if (result) {
    KISS_LOGF("✅ SSL conectado en %lu ms", duration);
    delay(100);

    if (!client->connected()) {
      KISS_LOG("❌ Socket muerto tras handshake");
      client->stop();
      manuallyConnected = false;
      _errorCount++;
      return false;
    }

    manuallyConnected = true;
    _connectionStartTime = millis();
    _errorCount = 0;
    return true;
  }

  // OBTENER ERROR ESPECÍFICO
  int sslError = client->lastError(nullptr, 0);
  _errorCount++;
  KISS_LOGF("❌ Conexión SSL falló (%lu ms) - Error: %d", duration, sslError);

  // CÓDIGOS DE ERROR mbedTLS comunes:
  switch (sslError) {
    case -0x2700: KISS_LOG("   → Certificado no confiable"); break;
    case -0x7200: KISS_LOG("   → Buffer insuficiente"); break;
    case -0x7900: KISS_LOG("   → Certificado expirado/inválido"); break;
    case -0x7080: KISS_LOG("   → Fallo verificación cert"); break;
    case -0x6800: KISS_LOG("   → Nombre común (CN) no coincide"); break;
    default: KISS_LOGF("   → Error desconocido: 0x%04X", -sslError);
  }

  return false;
}

bool KissSSL::isConnected() {
  if (!client) return false;
  return (manuallyConnected && client->connected());
}

void KissSSL::disconnect() {
  if (client) {
    client->stop();
  }
  manuallyConnected = false;
  _connectionStartTime = 0;
}

// --- Métodos delegados ---

bool KissSSL::connect(const char* host, uint16_t port) {
  if (!client) client = new WiFiClientSecure();
  return client->connect(host, port);
}

bool KissSSL::connected() {
  return client ? client->connected() : false;
}

void KissSSL::setCACert(const char* rootCA) {
  if (client) client->setCACert(rootCA);
}

void KissSSL::setInsecure() {
  if (client) client->setInsecure();
}

bool KissSSL::verify(const char* fingerprint, const char* url) {
  return client ? client->verify(fingerprint, url) : false;
}

size_t KissSSL::print(const char* str) {
  return client ? client->print(str) : 0;
}

size_t KissSSL::print(const String& str) {
  return client ? client->print(str) : 0;
}

size_t KissSSL::println(const char* str) {
  return client ? client->println(str) : 0;
}

size_t KissSSL::println(const String& str) {
  return client ? client->println(str) : 0;
}

int KissSSL::available() {
  return client ? client->available() : 0;
}

int KissSSL::read() {
  return client ? client->read() : -1;
}

int KissSSL::read(uint8_t* buffer, size_t size) {
  return client ? client->read(buffer, size) : 0;
}

void KissSSL::stop() {
  if (client) client->stop();
}

// NUEVOS MÉTODOS - Control de modo SSL
void KissSSL::setSecureMode(bool secure) {
  KISS_LOGF("🔧 setSecureMode() llamado: %s → %s",
            secureMode ? "SECURE" : "INSECURE",
            secure ? "SECURE" : "INSECURE");

  secureMode = secure;

  if (isConnected()) {
    KISS_LOG("🔌 Desconectando socket para aplicar cambio...");
    disconnect();
  }
}


bool KissSSL::isSecureMode() {
  return secureMode;
}

void KissSSL::printInfo() {
  KISS_LOG("\n🔍 KissSSL Info:");
  KISS_LOGF("   Tipo: %s", getClientType());
  KISS_LOGF("   Conectado: %s", isConnected() ? "SI" : "NO");
  KISS_LOGF("   Cliente: %s", client ? "ACTIVO" : "NULL");
  KISS_LOGF("   Modo SSL: %s", secureMode ? "SECURED (validación ON)" : "INSECURE (validación OFF)");
  KISS_LOGF("   Time synced: %s", KissTime::getInstance().isTimeSynced() ? "SI" : "NO");
  KISS_LOGF("   Señal WiFi: %d dBm", getSignalStrength());
  KISS_LOGF("   Consumo estimado: %d mA", getCurrentConsumption());
  KISS_LOGF("   Edad conexión: %lu s", getConnectionAge() / 1000);
}

// ========== POWER MANAGEMENT ==========
bool KissSSL::setPowerMode(KissClientPowerMode mode) {
  _powerMode = mode;

  // WiFi no tiene control directo de power mode como LTE,
  // pero podemos ajustar parámetros WiFi según el modo
  switch (mode) {
    case CLIENT_POWER_LOW:
    case CLIENT_POWER_IDLE:
      WiFi.setSleep(true);  // Activar WiFi sleep
      break;

    case CLIENT_POWER_ACTIVE:
    case CLIENT_POWER_TURBO:
      WiFi.setSleep(false);  // Desactivar WiFi sleep para máximo rendimiento
      break;

    case CLIENT_POWER_MAINTENANCE:
      // En mantenimiento, mantener conexión mínima
      WiFi.setSleep(true);
      break;

    default:
      break;
  }

  return true;
}

KissClientPowerMode KissSSL::getPowerMode() {
  return _powerMode;
}

int KissSSL::getCurrentConsumption() {
  // Estimación de consumo WiFi en mA
  if (!WiFi.isConnected()) return 0;

  switch (_powerMode) {
    case CLIENT_POWER_LOW:
    case CLIENT_POWER_IDLE:
      return 20;  // WiFi sleep activo

    case CLIENT_POWER_ACTIVE:
      return 80;  // WiFi activo normal

    case CLIENT_POWER_TURBO:
      return 120; // WiFi máximo rendimiento

    case CLIENT_POWER_MAINTENANCE:
      return 15;  // Mínimo

    default:
      return 40;  // Boot/transición
  }
}

// ========== INFORMACIÓN ==========
const char* KissSSL::getClientType() {
  return "WiFi";
}

int KissSSL::getSignalStrength() {
  if (!WiFi.isConnected()) return -999;
  return WiFi.RSSI();
}

const char* KissSSL::getConnectionInfo() {
  static char info[128];

  if (!WiFi.isConnected()) {
    snprintf(info, sizeof(info), "WiFi: Desconectado");
    return info;
  }

  snprintf(info, sizeof(info), "WiFi: %s | IP: %s | RSSI: %d dBm",
           WiFi.SSID().c_str(),
           WiFi.localIP().toString().c_str(),
           WiFi.RSSI());

  return info;
}

// ========== DIAGNÓSTICOS ==========
bool KissSSL::isConnectionHealthy() {
  if (!isConnected()) return false;
  if (!WiFi.isConnected()) return false;

  // Verificar que la señal WiFi sea aceptable (> -85 dBm)
  int rssi = WiFi.RSSI();
  if (rssi < -85) return false;

  // Verificar que el socket SSL esté vivo
  if (!client || !client->connected()) return false;

  return true;
}

unsigned long KissSSL::getConnectionAge() {
  if (_connectionStartTime == 0 || !isConnected()) return 0;
  return millis() - _connectionStartTime;
}

int KissSSL::getErrorCount() {
  return _errorCount;
}

void KissSSL::resetErrorCount() {
  _errorCount = 0;
}