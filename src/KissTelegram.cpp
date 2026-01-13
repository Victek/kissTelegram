// KissTelegram.cpp
// Vicente Soriano - victek@gmail.com

#include "KissTelegram.h"
#include "KissConfig.h"
#include "KissTime.h"
#include "KissNet.h"
#include <Arduino.h>
#include "lang.h"

#ifdef KISS_USE_RTOS
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#define SAFE_YIELD() vTaskDelay(1)
#else
#define SAFE_YIELD() yield()
#endif

// ========== CONSTRUCTOR ==========
KissTelegram::KissTelegram(const char* token) {
  strncpy(botToken, token, sizeof(botToken) - 1);
  botToken[sizeof(botToken) - 1] = '\0';

  // Crear KissNet para gestión unificada WiFi/LTE
  kissNet = new KissNet();

  // Inicializar KissNet (cargará credenciales automáticamente)
  kissNet->begin();

  sslSecure = false;

  enabled = true;
  wifiStableTime = millis();
  currentQuality = QUALITY_FAIR;
  connectionAttempts = 0;
  failedPings = 0;
  lastUpdateID = 0;

  lastMessageTime = 0;
  minMessageInterval = 1000;

  pollingTimeout = 2;
  adaptivePolling = true;

  maxRetryAttempts = 3;
  fileReceivedCallback = nullptr;

  jsonBuffer = new char[JSON_BUFFER_SIZE];
  messageBuffer = new char[MESSAGE_BUFFER_SIZE];
  commandBuffer = new char[COMMAND_BUFFER_SIZE];
  paramBuffer = new char[PARAM_BUFFER_SIZE];

  storageEnabled = true;
  storageMode = STORAGE_FULL;
  maxQueueStorage = KISS_MAX_FS_QUEUE;
  lastSaveTime = 0;
  autoSaveInterval = 300000;
  nextMsgId = 1;

  // ⚡ Inicializar caché
  cachedPendingCount = -1;
  lastCountCheck = 0;

  operationMode = MODE_BALANCED;
  wifiStabilityThreshold = 30000;
  diagnosticsVerbose = true;
  startTime = millis();

  currentPowerMode = POWER_BOOT;
  targetPowerMode = POWER_BOOT;
  powerModeChangeTime = millis();
  lastActivityTime = millis();
  maintenanceModeStart = 0;
  maintenanceReason[0] = '\0';

  idleTimeout = 300;
  decayTime = 10;
  bootStableTime = 30;
  powerSavingEnabled = true;
  maintenanceMode = false;

  memset(timeInMode, 0, sizeof(timeInMode));
  modeStartTime = millis();
  totalActiveTime = 0;
  totalMessagesSent = 0;
  powerModeCallback = nullptr;
  systemEventCallback = nullptr;
  powerTransitionCallback = nullptr;
  otaMode = false;

  // Turbo mode
  turboMode = false;
  originalMinInterval = 1000;
  turboProcessedTotal = 0;
  turboStartTime = 0;

  // Batch delete
  pendingDeletes = 0;
  memset(deleteQueue, 0, sizeof(deleteQueue));

  resetBuffers();
  if (!KISS_INIT_FS()) {
    KISS_CRITICAL(LANG_ERROR_CRITICAL_FS);
  }

  // Restaurar estado y flush inmediato de deleteQueue[] si hay IDs pendientes
  if (storageEnabled) {
    restoreFromLittleFS();

    // Limpiar duplicados que puedan existir de reinicios abruptos previos
    removeDuplicateMessagesFromFS();

    if (pendingDeletes > 0) {
      KISS_LOGF("🔄 Restaurados %d IDs para borrado batch", pendingDeletes);
      flushDeleteQueue();  // Borrar inmediatamente mensajes que ya se enviaron antes del reinicio
    }
  }
}

KissTelegram::~KissTelegram() {
  // Flush cualquier delete pendiente antes de guardar
  if (pendingDeletes > 0) {
    flushDeleteQueue();
  }

  if (storageEnabled) {
    saveNow();
  }

  // Desactivar turbo mode si estaba activo para reducir consumo MCU
  if (turboMode) {
    turboMode = false;
  }

  disable();

  if (kissNet) {
    kissNet->end();  // KissNet se encarga de limpiar WiFi/LTE
    delete kissNet;
    kissNet = nullptr;
  }

  if (jsonBuffer) delete[] jsonBuffer;
  if (messageBuffer) delete[] messageBuffer;
  if (commandBuffer) delete[] commandBuffer;
  if (paramBuffer) delete[] paramBuffer;

  KISS_LOG(LANG_ERROR_SYSTEM_DES);
}

void KissTelegram::onFileReceived(FileReceivedCallback callback) {
  fileReceivedCallback = callback;
}

// ========== CONFIGURACIÓN BÁSICA ==========

void KissTelegram::enable() {
  enabled = true;
}

void KissTelegram::disable() {
  enabled = false;
  KissClient* client = kissNet->getActiveClient();
  if (client && client->isConnected()) {
    client->disconnect();
  }
}

bool KissTelegram::isEnabled() {
  return enabled;
}

void KissTelegram::setWifiStable() {
  wifiStableTime = millis();
  failedPings = 0;
  currentQuality = QUALITY_FAIR;
}

bool KissTelegram::isWifiStable() {
  return enabled && isConnectionReallyStable();
}

void KissTelegram::setMinMessageInterval(int milliseconds) {
  minMessageInterval = max(0, milliseconds);
}
int KissTelegram::getMinMessageInterval() {
  return minMessageInterval;
}

void KissTelegram::setMaxRetryAttempts(int attempts) {
  maxRetryAttempts = max(1, attempts);
}
int KissTelegram::getMaxRetryAttempts() {
  return maxRetryAttempts;
}

void KissTelegram::enableStorage(bool enable) {
  // Si se desactiva storage, hacer flush de deletes pendientes primero
  if (!enable && storageEnabled && pendingDeletes > 0) {
    flushDeleteQueue();
  }
  storageEnabled = enable;
}
void KissTelegram::setStorageMode(StorageMode mode) {
  storageMode = mode;
}
void KissTelegram::setMaxQueueStorage(int maxMessages) {
  maxQueueStorage = maxMessages;
}
int KissTelegram::getMaxQueueStorage() {
  return maxQueueStorage;
}

void KissTelegram::setAutoSaveInterval(unsigned long intervalMs) {
  autoSaveInterval = intervalMs;
}
unsigned long KissTelegram::getAutoSaveInterval() {
  return autoSaveInterval;
}

void KissTelegram::applyOperationMode() {
  switch (operationMode) {
    case MODE_BALANCED:
      setPowerSaving(true);
      setMinMessageInterval(2000);
      break;
    case MODE_PERFORMANCE:
      setPowerSaving(false);
      setMinMessageInterval(1000);
      break;
    case MODE_POWERSAVE:
      setPowerSaving(true);
      setMinMessageInterval(3000);
      break;
    case MODE_RELIABILITY:
      setMaxRetryAttempts(5);
      setAutoSaveInterval(60000);
      break;
  }
}

void KissTelegram::setOperationMode(OperationMode mode) {
  operationMode = mode;
  applyOperationMode();
}
KissTelegram::OperationMode KissTelegram::getOperationMode() {
  return operationMode;
}

void KissTelegram::setDiagnosticsVerbose(bool verbose) {
  diagnosticsVerbose = verbose;
}
bool KissTelegram::getDiagnosticsVerbose() {
  return diagnosticsVerbose;
}

// ========== POWER MANAGEMENT ==========

void KissTelegram::setPowerMode(PowerMode mode) {
  PowerMode oldMode = currentPowerMode;
  currentPowerMode = mode;
  powerModeChangeTime = millis();

  // Sincronizar con KissClient (WiFi/LTE)
  KissClient* client = kissNet->getActiveClient();
  if (client) {
    KissClientPowerMode clientMode;
    switch (mode) {
      case POWER_BOOT:
        clientMode = CLIENT_POWER_BOOT;
        break;
      case POWER_LOW:
        clientMode = CLIENT_POWER_LOW;
        break;
      case POWER_IDLE:
        clientMode = CLIENT_POWER_IDLE;
        break;
      case POWER_ACTIVE:
        clientMode = CLIENT_POWER_ACTIVE;
        break;
      case POWER_TURBO:
        clientMode = CLIENT_POWER_TURBO;
        break;
      case POWER_MAINTENANCE:
        clientMode = CLIENT_POWER_MAINTENANCE;
        break;
      default:
        clientMode = CLIENT_POWER_ACTIVE;
    }

    client->setPowerMode(clientMode);

    // ✅ OPTIMIZACIÓN LTE: aplicar optimizaciones de conexión
    optimizeConnectionForNetwork();

    if (diagnosticsVerbose) {
      KISS_LOGF("⚡ Power mode: %s", client->getClientType());
    }
  }

  if (powerModeCallback) {
    powerModeCallback(oldMode, mode);
  }
}

KissTelegram::PowerMode KissTelegram::getCurrentPowerMode() {
  return currentPowerMode;
}
KissTelegram::PowerMode KissTelegram::getTargetPowerMode() {
  return targetPowerMode;
}

void KissTelegram::setPowerConfig(int idleTimeoutSec, int decayTimeSec, int bootStableTimeSec) {
  idleTimeout = max(60, idleTimeoutSec);
  decayTime = max(5, decayTimeSec);
  bootStableTime = max(10, bootStableTimeSec);
}

void KissTelegram::getPowerConfig(int& idleTimeoutOut, int& decayTimeOut, int& bootStableTimeOut) {
  idleTimeoutOut = this->idleTimeout;
  decayTimeOut = this->decayTime;
  bootStableTimeOut = this->bootStableTime;
}

void KissTelegram::setPowerSaving(bool enable) {
  powerSavingEnabled = enable;
}
bool KissTelegram::getPowerSaving() {
  return powerSavingEnabled;
}

void KissTelegram::setMaintenanceMode(bool enable, const char* reason) {
  maintenanceMode = enable;
  if (enable) {
    maintenanceModeStart = millis();
    if (reason) {
      strncpy(maintenanceReason, reason, sizeof(maintenanceReason) - 1);
      maintenanceReason[sizeof(maintenanceReason) - 1] = '\0';
    } else {
      strcpy(maintenanceReason, "No especificado");
    }
    setPowerMode(POWER_MAINTENANCE);
    KISS_LOGF(LANG_INFO_MAINTENANCE_ON, maintenanceReason);
  } else {
    maintenanceReason[0] = '\0';
    KISS_LOG(LANG_INFO_MAINTENANCE_OFF);
    updatePowerState();
  }
}

bool KissTelegram::isInMaintenanceMode() {
  return maintenanceMode;
}

void KissTelegram::onPowerModeChange(PowerModeCallback callback) {
  powerModeCallback = callback;
}
void KissTelegram::onSystemEvent(SystemEventCallback callback) {
  systemEventCallback = callback;
}
void KissTelegram::onPowerTransition(PowerTransitionCallback callback) {
  powerTransitionCallback = callback;
}

void KissTelegram::updatePowerState() {
  if (!powerSavingEnabled || maintenanceMode) return;

  unsigned long now = millis();
  int pendingCount = countPendingMessages();

  PowerMode desiredMode = currentPowerMode;

  if (!isWifiStable()) {
    desiredMode = POWER_BOOT;
  } else if (pendingCount > 0) {
    desiredMode = POWER_ACTIVE;
  } else {
    unsigned long inactiveTime = safeTimeDiff(now, lastActivityTime);
    desiredMode = (inactiveTime > (idleTimeout * 1000)) ? POWER_LOW : POWER_IDLE;
  }

  if (desiredMode != currentPowerMode) {
    setPowerMode(desiredMode);
  }

  updatePowerStatistics();
}

bool KissTelegram::shouldProcessQueue() {
  if (!powerSavingEnabled) return true;
  if (maintenanceMode) return false;

  switch (currentPowerMode) {
    case POWER_BOOT:
    case POWER_LOW:
      return false;
    default:
      return true;
  }
}

bool KissTelegram::shouldCheckMessages() {
  if (!powerSavingEnabled) return true;

  unsigned long timeSinceLastCheck = safeTimeDiff(millis(), lastMessageTime);

  switch (currentPowerMode) {
    case POWER_BOOT: return (timeSinceLastCheck > 30000);
    case POWER_LOW: return (timeSinceLastCheck > 60000);
    case POWER_IDLE: return (timeSinceLastCheck > 15000);
    case POWER_ACTIVE: return (timeSinceLastCheck > 10000);
    case POWER_TURBO: return (timeSinceLastCheck > 5000);
    default: return (timeSinceLastCheck > 10000);
  }
}

int KissTelegram::getRecommendedDelay() {
  if (!powerSavingEnabled) return 1000;

  switch (currentPowerMode) {
    case POWER_BOOT: return 5000;
    case POWER_LOW: return 10000;
    case POWER_IDLE: return 3000;
    case POWER_ACTIVE: return 1000;
    case POWER_TURBO: return 500;
    default: return 1000;
  }
}

unsigned long KissTelegram::getTimeInMode(PowerMode mode) {
  if (mode >= POWER_BOOT && mode <= POWER_MAINTENANCE) {
    return timeInMode[mode];
  }
  return 0;
}

float KissTelegram::getPowerEfficiency() {
  if (totalActiveTime == 0) return 0.0f;
  return (float)totalMessagesSent / (totalActiveTime / 1000.0f);
}

void KissTelegram::resetPowerStatistics() {
  memset(timeInMode, 0, sizeof(timeInMode));
  totalActiveTime = 0;
  totalMessagesSent = 0;
  modeStartTime = millis();
}

// ========== OPERACIONES MENSAJES ==========
void KissTelegram::cleanupConnection() {
    KissClient* client = kissNet->getActiveClient();
    if(!client) return;

    // ✅ SOLO cerrar si el socket está realmente muerto
    if(client->isConnected()) {
        KISS_LOG(LANG_INFO_CON_RESUME);
        return;  // ← NO cerrar conexiones vivas
    }

    // ❌ Solo limpiar sockets zombies
    KISS_LOG(LANG_INFO_CON_DIE);
    client->stop();
}

bool KissTelegram::isConnectionAlive() {
    KissClient* client = kissNet->getActiveClient();
    if(!client || !client->isConnected()) {
        return false;
    }

    // DISABLED: Si el socket muere, client->isConnected() lo detectará

    return true;
}

// Versión sin diagnóstico (mantener compatibilidad)
bool KissTelegram::sendMessage(const char* chat_id, const char* text, MessagePriority priority) {
    SendFailureReason reason;
    return sendMessage(chat_id, text, priority, &reason);
}

// Versión con diagnóstico detallado
bool KissTelegram::sendMessage(const char* chat_id, const char* text, MessagePriority priority, SendFailureReason* failReason) {
    if (failReason) *failReason = SEND_OK;

    if(!enabled) {
        if (failReason) *failReason = SEND_FAIL_DISABLED;
        logEvent("SEND_FAIL", "BOT_DISABLED");
        return false;
    }

    unsigned long now = millis();
    unsigned long timeSinceLastMsg = safeTimeDiff(now, lastMessageTime);

    // Límite de velocidad entre mensajes
    if(lastMessageTime > 0 && timeSinceLastMsg < minMessageInterval) {
        if (failReason) *failReason = SEND_FAIL_RATE_LIMIT;
        logEvent("SEND_QUEUED", "RATE_LIMIT");
        return queueMessage(chat_id, text, priority);
    }

    // Verificar WiFi estable
    if(!isWifiStable()) {
        if (failReason) *failReason = SEND_FAIL_WIFI_UNSTABLE;
        logEvent("SEND_QUEUED", "WIFI_UNSTABLE");
        return queueMessage(chat_id, text, priority);
    }

    connectionAttempts++;

    //REUTILIZAR conexión existente si está viva
    if(!isConnectionAlive()) {
        if(!connectToTelegram()) {
            if (failReason) *failReason = SEND_FAIL_CONNECTION;
            logEvent("SEND_FAIL", "SSL_CONNECT");
            return queueMessage(chat_id, text, priority);
        }
    }

    // CONSTRUIR Y ENVIAR MENSAJE
    KissClient* client = kissNet->getActiveClient();
    if (!client) {
        if (failReason) *failReason = SEND_FAIL_NO_CLIENT;
        logEvent("SEND_FAIL", "NO_CLIENT");
        return false;
    }

    client->print("POST /bot");
    client->print(botToken);
    client->print("/sendMessage HTTP/1.1\r\n");
    client->print("Host: api.telegram.org\r\n");
    client->print("Content-Type: application/json\r\n");

    // Calcular longitud del JSON
    char jsonLengthStr[10];
    int jsonLength = snprintf(nullptr, 0, "{\"chat_id\":\"%s\",\"text\":\"%s\"}", chat_id, text);
    snprintf(jsonLengthStr, sizeof(jsonLengthStr), "%d", jsonLength);

    client->print("Content-Length: ");
    client->print(jsonLengthStr);

    // ✅ OPTIMIZACIÓN LTE: usar Connection header apropiado
    if (shouldUseKeepAlive()) {
        client->print("\r\nConnection: keep-alive\r\n\r\n");
    } else {
        client->print("\r\nConnection: close\r\n\r\n");
    }

    client->print("{\"chat_id\":\"");
    client->print(chat_id);
    client->print("\",\"text\":\"");
    client->print(text);
    client->print("\"}");

    //LEER RESPUESTA
    TgAck ack;
    bool success = readResponse(ack);

    // ✅ OPTIMIZACIÓN LTE: cerrar socket si es necesario
    if(success) {
        lastMessageTime = now;
        lastActivityTime = now;
        totalMessagesSent++;
        if (failReason) *failReason = SEND_OK;

        // Cerrar conexión en modo LTE low-power
        if (shouldCloseAfterRequest()) {
            if (diagnosticsVerbose) {
                KISS_LOG("📡 LTE: Cerrando socket para permitir sleep");
            }
            cleanupConnection();
        }
     } else {
        // Diagnosticar tipo de error
        if (failReason) {
            if (ack.httpCode >= 400) {
                *failReason = SEND_FAIL_HTTP_ERROR;
                char httpErr[32];
                snprintf(httpErr, sizeof(httpErr), "HTTP_%d", ack.httpCode);
                logEvent("SEND_FAIL", httpErr);
            } else {
                *failReason = SEND_FAIL_READ_RESPONSE;
                logEvent("SEND_FAIL", "READ_RESPONSE");
            }
        }

        // Solo en caso de error, limpiar conexión
        if(ack.httpCode >= 400) {
            KISS_LOGF(LANG_INFO_CON_CLEAN, ack.httpCode);
            cleanupConnection();  // Error de API, reconectar
        }

        if(ack.httpCode == 429) {
            KISS_LOGF("⏰ Rate limited - retry en %lu seg", ack.retryAfter);
            if(ack.retryAfter > 0) {
                minMessageInterval = max(minMessageInterval, (int)(ack.retryAfter * 1000));
            }
        }

        // Reencolar mensaje fallido
        if (!queueMessage(chat_id, text, priority)) {
            if (failReason) *failReason = SEND_FAIL_QUEUE_FULL;
            logEvent("SEND_FAIL", "QUEUE_FULL");
        }
     }

    return success;
 }

// ========== ENVÍO DIRECTO SIN RATE-LIMIT ==========
bool KissTelegram::sendMessageDirect(const char* chat_id, const char* text) {
    if(!enabled) return false;
    if(!isWifiStable()) return false;

    // Reutilizar conexión existente si está viva
    if(!isConnectionAlive()) {
        if(!connectToTelegram()) {
            return false;
        }
    }

    // Construir y enviar mensaje
    KissClient* client = kissNet->getActiveClient();
    if (!client) return false;

    client->print("POST /bot");
    client->print(botToken);
    client->print("/sendMessage HTTP/1.1\r\n");
    client->print("Host: api.telegram.org\r\n");
    client->print("Content-Type: application/json\r\n");

    // Calcular longitud del JSON
    char jsonLengthStr[10];
    int jsonLength = snprintf(nullptr, 0, "{\"chat_id\":\"%s\",\"text\":\"%s\"}", chat_id, text);
    snprintf(jsonLengthStr, sizeof(jsonLengthStr), "%d", jsonLength);

    client->print("Content-Length: ");
    client->print(jsonLengthStr);

    // ✅ OPTIMIZACIÓN LTE: usar Connection header apropiado
    if (shouldUseKeepAlive()) {
        client->print("\r\nConnection: keep-alive\r\n\r\n");
    } else {
        client->print("\r\nConnection: close\r\n\r\n");
    }

    client->print("{\"chat_id\":\"");
    client->print(chat_id);
    client->print("\",\"text\":\"");
    client->print(text);
    client->print("\"}");

    // Leer respuesta
    TgAck ack;
    bool success = readResponse(ack);

    if(success) {
        lastMessageTime = millis();
        lastActivityTime = millis();
        totalMessagesSent++;

        // ✅ OPTIMIZACIÓN LTE: cerrar socket si es necesario
        if (shouldCloseAfterRequest()) {
            cleanupConnection();
        }
    } else {
        if(ack.httpCode >= 400) {
            cleanupConnection();
        }
    }

    return success;
}

bool KissTelegram::queueMessage(const char* chat_id, const char* text, MessagePriority priority) {
  // Verificar espacio disponible en FS (mínimo 10% libre)
  if (KISS_FS.begin(false)) {
    size_t total = KISS_FS.totalBytes();
    size_t used = KISS_FS.usedBytes();
    size_t freeSpace = total - used;
    float usagePercent = (used * 100.0f) / total;

    if (usagePercent > 90.0f) {
      KISS_CRITICALF(LANG_INFO_FS_NEAR, usagePercent);
      KISS_FS.end();
      return false;
    }

    // Verificar que hay espacio para al menos un mensaje (~200 bytes)
    if (freeSpace < 500) {
      KISS_CRITICAL(LANG_INFO_FS_FULL);
      KISS_FS.end();
      return false;
    }
    KISS_FS.end();
  }

  if (!appendMessageToFS(chat_id, text, priority)) {
    KISS_CRITICAL(LANG_ERROR_FS_SAVE);
    return false;
  }

  unsigned long totalQueued = KissConfig::getInstance().getTotalMessagesQueued();
  KissConfig::getInstance().setTotalMessagesQueued(totalQueued + 1);

  lastActivityTime = millis();
  return true;
}

void KissTelegram::processQueue() {
  if (maintenanceMode) return;
  if (!shouldProcessQueue()) return;
  if (!isWifiStable()) return;

  // Activación automática de turbo mode si hay muchos mensajes pendientes
  if (!turboMode) {
    int pending = getMessagesInFS();
    if (pending > 20) {
      KISS_LOG("🚀 Auto-activando TURBO MODE (>20 mensajes pendientes)");
      enableTurboMode();
    }
  }

  // Determinar cuántos mensajes procesar por ciclo
  int maxPerCycle = turboMode ? 10 : 5;
  int processed = 0;
  int currentInterval = turboMode ? 50 : minMessageInterval;

  while (processed < maxPerCycle) {
    unsigned long now = millis();
    unsigned long timeSinceLastMsg = safeTimeDiff(now, lastMessageTime);

    // Verificar intervalo entre mensajes
    if (lastMessageTime > 0 && timeSinceLastMsg < (unsigned long)currentInterval) {
      // En modo turbo, esperar activamente un poco
      if (turboMode && timeSinceLastMsg < 50) {
        SAFE_YIELD();
        continue;
      }
      break;  // No turbo o ya esperamos suficiente
    }

    char chat_id[20];
    char text[256];
    MessagePriority priority;
    uint32_t msgId;

    if (!getNextPendingMessage(chat_id, text, &priority, &msgId)) {
      break;  // No hay más mensajes
    }

    // Usar sendMessageDirect para evitar reencolamiento
    if (sendMessageDirect(chat_id, text)) {
      queueDeleteMessage(msgId);  // Encolar para batch delete
      processed++;

      // Flush inmediato para mensajes CRITICAL (ej: bienvenida al arrancar)
      if (priority == PRIORITY_CRITICAL && pendingDeletes > 0) {
        flushDeleteQueue();
      }

      // Actualizar contador turbo
      if (turboMode) {
        turboProcessedTotal++;
        // Log de progreso cada 25 mensajes en modo turbo
        if (turboProcessedTotal % 25 == 0) {
          unsigned long elapsed = safeTimeDiff(millis(), turboStartTime);
          float rate = (elapsed > 0) ? (turboProcessedTotal * 1000.0f / elapsed) : 0;
          KISS_LOGF(LANG_INFO_MSG_SENT, turboProcessedTotal, rate);
        }
      }

      unsigned long totalSent = KissConfig::getInstance().getTotalMessagesSent();
      KissConfig::getInstance().setTotalMessagesSent(totalSent + 1);
    } else {
      KISS_LOG(LANG_WARN_MSG_SEND);
      break;  // Error, parar este ciclo
    }

    SAFE_YIELD();  // Permitir otras tareas
  }

  // Flush batch delete si hay suficientes pendientes
  if (pendingDeletes >= BATCH_DELETE_THRESHOLD) {
    flushDeleteQueue();
  }

  // SEGURIDAD: Flush final si no quedan mensajes pendientes
  // Esto garantiza que SIEMPRE se borren todos los mensajes enviados
  if (processed > 0 && countPendingMessages() == 0 && pendingDeletes > 0) {
    flushDeleteQueue();
    if (diagnosticsVerbose) {
      KISS_LOG(LANG_INFO_FLUSH);
    }
  }

  if (processed > 0 && diagnosticsVerbose) {
    KISS_LOGF(LANG_INFO_MSG_QUEUE, processed);
  }
}

bool KissTelegram::checkMessages(void (*handler)(const char*, const char*, const char*, const char*)) {
    static unsigned long lastCheck = 0;
    // Intervalo fijo para evitar overhead
    if(!hasTimePassed(lastCheck, 3000)) return false;
    lastCheck = millis();

    if(!enabled || !isWifiStable()) return false;

    // REUTILIZAR conexión existente
    if(!isConnectionAlive()) {
        if(!connectToTelegram()) return false;
    }

  int timeout = pollingTimeout;
  if (adaptivePolling) {
    int pending = countPendingMessages();
    if (pending > 0) {
      timeout = 0;  // Hay mensajes pendientes, no esperar
    }
  }

  // Buffers para conversiones numéricas
  char offsetStr[20];
  char timeoutStr[10];

  snprintf(offsetStr, sizeof(offsetStr), "%ld", lastUpdateID + 1);
  snprintf(timeoutStr, sizeof(timeoutStr), "%d", timeout);

  KissClient* client = kissNet->getActiveClient();
  if (!client) return false;

  client->print("GET /bot");
  client->print(botToken);
  client->print("/getUpdates?offset=");
  client->print(offsetStr);
  client->print("&timeout=");
  client->print(timeoutStr);
  client->print("&limit=10 HTTP/1.1\r\n");
  client->print("Host: api.telegram.org\r\n");

  // ✅ OPTIMIZACIÓN LTE: usar Connection header apropiado
  if (shouldUseKeepAlive()) {
      client->print("Connection: keep-alive\r\n\r\n");
  } else {
      client->print("Connection: close\r\n\r\n");
  }

  if (!readResponse()) {
    cleanupConnection();
    return false;
  }

  // ✅ OPTIMIZACIÓN LTE: cerrar socket después de checkMessages si es necesario
  bool shouldCloseAfter = shouldCloseAfterRequest();

  char* updatePtr = jsonBuffer;
  bool processedAny = false;
  long highestUpdateID = lastUpdateID;

  while ((updatePtr = strstr(updatePtr, "\"update_id\":")) != NULL) {
    updatePtr += strlen("\"update_id\":");
    long currentUpdateID = strtol(updatePtr, NULL, 10);

    // VALIDACIÓN: Ignorar updates ya procesados (protección contra duplicados)
    if (currentUpdateID <= lastUpdateID) {
      // Log solo en modo verbose para debugging
      if (diagnosticsVerbose) {
        KISS_LOGF(LANG_WARN_MSG_DUP, currentUpdateID);
      }
      updatePtr += 10; // Avanzar para buscar el siguiente
      continue;
    }

    if (currentUpdateID > highestUpdateID) {
      highestUpdateID = currentUpdateID;
    }

    char* textPtr = strstr(updatePtr, "\"text\":\"");
    if (textPtr && handler) {
      textPtr += strlen("\"text\":\"");
      char* textEnd = strchr(textPtr, '\"');
      if (textEnd) {
        // PROTECCIÓN: Validar longitud antes de copiar
        int textLen = (int)(textEnd - textPtr);
        if (textLen < 0 || textLen > 1024) {
          KISS_LOGF(LANG_WARN_MSG_LEN, textLen);
          updatePtr = textEnd;
          continue;
        }

        char text[128] = { 0 };
        strncpy(text, textPtr, min(textLen, 127));
        text[127] = '\0'; // Garantizar null-termination

        if (text[0] == '/') {
          char chat_id[20] = { 0 };
          char* chatPtr = strstr(updatePtr, "\"chat\":{\"id\":");
          if (chatPtr) {
            chatPtr += strlen("\"chat\":{\"id\":");
            char* chatEnd = strchr(chatPtr, ',');
            if (chatEnd) {
              int chatLen = (int)(chatEnd - chatPtr);
              if (chatLen > 0 && chatLen < 20) {
                strncpy(chat_id, chatPtr, chatLen);
                chat_id[chatLen] = '\0';
              }
            }
          }

          // VALIDACIÓN: Solo procesar si tenemos chat_id válido
          if (chat_id[0] != '\0') {
            // ✅ FIX: Marcar update como procesado ANTES de llamar al handler
            // Esto previene reprocesamiento si WiFi falla durante el handler
            if (currentUpdateID > lastUpdateID) {
              lastUpdateID = currentUpdateID;
            }

            char command[32] = { 0 };
            char param[64] = { 0 };
            extractCommand(text, command, param);

            handler(chat_id, text, command, param);
            processedAny = true;
          } else {
            KISS_LOG(LANG_WARN_NOID);
          }
        }
      }
    }

    char* docPtr = strstr(updatePtr, "\"document\":{");
    if (docPtr && fileReceivedCallback) {
      char* fileIdPtr = strstr(docPtr, "\"file_id\":\"");
      if (fileIdPtr) {
        fileIdPtr += strlen("\"file_id\":\"");
        char* fileIdEnd = strchr(fileIdPtr, '\"');
        if (fileIdEnd) {
          // 🔥 PROTECCIÓN: Validar longitud de file_id
          int fileIdLen = (int)(fileIdEnd - fileIdPtr);
          if (fileIdLen < 0 || fileIdLen > 127) {
            KISS_LOGF(LANG_WARN_WRLEN, fileIdLen);
            updatePtr = fileIdEnd;
            continue;
          }

          char file_id[128] = { 0 };
          strncpy(file_id, fileIdPtr, fileIdLen);
          file_id[fileIdLen] = '\0';

          size_t file_size = 0;
          char* sizePtr = strstr(docPtr, "\"file_size\":");
          if (sizePtr) {
            sizePtr += strlen("\"file_size\":");
            file_size = strtoul(sizePtr, NULL, 10);
          }

          char file_name[128] = { 0 };
          char* namePtr = strstr(docPtr, "\"file_name\":\"");
          if (namePtr) {
            namePtr += strlen("\"file_name\":\"");
            char* nameEnd = strchr(namePtr, '\"');
            if (nameEnd) {
              int nameLen = (int)(nameEnd - namePtr);
              if (nameLen > 0 && nameLen < 128) {
                strncpy(file_name, namePtr, nameLen);
                file_name[nameLen] = '\0';
              }
            }
          }

          fileReceivedCallback(file_id, file_size, file_name);
          processedAny = true;
        }
      }
    }

    updatePtr = textPtr ? textPtr : updatePtr + 50;
    if (updatePtr > jsonBuffer + JSON_BUFFER_SIZE) break;
  }

  if (highestUpdateID > lastUpdateID) {
    lastUpdateID = highestUpdateID;
    if (storageEnabled && shouldSave()) {
      saveNow();
    }
  }

  // ✅ OPTIMIZACIÓN LTE: cerrar socket tras checkMessages en modo low-power
  if (shouldCloseAfter) {
    if (diagnosticsVerbose) {
      KISS_LOG("📡 LTE: Cerrando socket tras checkMessages para permitir sleep");
    }
    cleanupConnection();
  }

  return processedAny;
}

// ========== SSL Casi inteligente.. más que yo, seguro ==========
bool KissTelegram::trySecureConnection() {
  KISS_LOG(LANG_WARN_TSSL);

  // DIAGNÓSTICO COMPLETO
  bool timeSynced = KissTime::getInstance().isTimeSynced();
  KISS_LOGF(LANG_INFO_TSYNC);
  KISS_LOGF(LANG_INFO_FRAM, ESP.getFreeHeap());

  KissClient* client = kissNet->getActiveClient();
  if (!client) {
    KISS_CRITICAL(LANG_ERROR_SSLCLI);
    return false;
  }

  // Configurar certificado (setCACert retorna void)
  client->setCACert(TELEGRAM_ROOT_CA);
  KISS_LOG(LANG_INFO_CERT);

  // Test de conectividad básica primero
  KISS_LOG(LANG_INFO_TEST_BASIC);
  WiFiClient basicTest;
  unsigned long basicStart = millis();
  bool basicConnected = basicTest.connect("api.telegram.org", 443);
  unsigned long basicTime = millis() - basicStart;
  KISS_LOGF(LANG_INFO_CON_BASIC,
            basicConnected ? "OK" : "FALLÓ", basicTime);
  if (basicConnected) {
    basicTest.stop();
  }

  // Ahora SSL
  unsigned long startTime = millis();
  bool connected = client->connect("api.telegram.org", 443);
  unsigned long connectTime = millis() - startTime;

  KISS_LOGF(LANG_INFO_SSL_TEST, connectTime);

  if (connected) {
    KISS_LOGF(LANG_INFO_SSL_SUCC, connectTime);

    // Verificar certificado solo si el NTP está sincronizado
    if (timeSynced) {
      if (client->verify("api.telegram.org", NULL)) {
        KISS_LOG(LANG_INFO_CERT_VERI);
        sslSecure = true;
      } else {
        KISS_CRITICAL(LANG_INFO_CERT_FAIL);
        client->stop();
        return false;
      }
    } else {
      KISS_LOG(LANG_WARN_CERT_FAIL);
      sslSecure = false;
    }
    return true;
  } else {
    KISS_LOGF(LANG_ERROR_SSL_FAIL, connectTime);
    return false;
  }
}

bool KissTelegram::tryInsecureConnection() {
  KissClient* client = kissNet->getActiveClient();
  if (!client) return false;

  client->setInsecure();

  // DIAGNÓSTICO
  KISS_LOGF(LANG_INFO_FRRAM, ESP.getFreeHeap());

  unsigned long startTime = millis();
  bool connected = client->connect("api.telegram.org", 443);
  unsigned long connectTime = millis() - startTime;

  if (connected) {
    KISS_LOGF(LANG_WARN_INSECURE, connectTime);
    sslSecure = false;
    return true;
  } else {
    KISS_LOGF(LANG_ERROR_INSECURE, connectTime);
    return false;
  }
}

// ========== MANEJO SEGURO DE TIEMPO ==========
unsigned long KissTelegram::safeTimeDiff(unsigned long later, unsigned long earlier) {
  // Manejo robusto de overflow
  if (later >= earlier) {
    return later - earlier;
  } else {
    // Overflow detectado - calcular diferencia correcta
    return (ULONG_MAX - earlier) + later + 1;
  }
}

bool KissTelegram::hasTimePassed(unsigned long startTime, unsigned long interval) {
  unsigned long current = millis();

  if (current >= startTime) {
    // Caso normal sin overflow
    return (current - startTime) >= interval;
  } else {
    // Caso con overflow
    return ((ULONG_MAX - startTime) + current + 1) >= interval;
  }
}

// ========== STORAGE ==========

bool KissTelegram::saveNow() {
  if (!storageEnabled || storageMode == STORAGE_DISABLED) return false;

  // Flush deleteQueue[] antes de guardar para minimizar riesgo de pérdida
  if (pendingDeletes > 0) {
    flushDeleteQueue();
  }

  bool success = saveToLittleFS();
  if (success) {
    lastSaveTime = millis();
    KissConfig::getInstance().setLastSaveTime(lastSaveTime);
  }
  return success;
}

bool KissTelegram::restoreFromStorage() {
  if (!storageEnabled) return false;
  return restoreFromLittleFS();
}

void KissTelegram::clearStorage() {
  if (!storageEnabled) return;

  // Limpiar cola de deletes pendientes (ya no hacen falta)
  pendingDeletes = 0;
  memset(deleteQueue, 0, sizeof(deleteQueue));

  // Invalidar caché
  cachedPendingCount = -1;

  nextMsgId = 1;
  if (KISS_FS.begin(KISS_FS_FORMAT_ON_FAIL)) {
    KISS_FS.remove("/kisstg_state.txt");
    KISS_FS.remove(KISS_FS_QUEUE_FILE);
    KISS_FS.end();
  }
  saveToLittleFS();
  KISS_LOG(LANG_INFO_STOR_CLEAN);
}

void KissTelegram::autoSave() {
  if (!storageEnabled) return;
  if (shouldSave()) saveNow();
}

bool KissTelegram::shouldSave() {
  return hasTimePassed(lastSaveTime, 300000);  // 5 minutos
}

// ========== LITTLEFS ==========

bool KissTelegram::appendMessageToFS(const char* chat_id, const char* text, MessagePriority priority) {
  cachedPendingCount = -1;

  if (!KISS_FS.begin(KISS_FS_FORMAT_ON_FAIL)) return false;

  // Escapar texto
  char escapedText[512];  // Revertido: necesario para mensajes largos
  size_t escPos = 0;
  size_t textLen = strlen(text);
  for (size_t i = 0; i < textLen && escPos < sizeof(escapedText) - 3; i++) {
    if (text[i] == '"') {
      escapedText[escPos++] = '\\';
      escapedText[escPos++] = '"';
    } else if (text[i] == '\\') {
      escapedText[escPos++] = '\\';
      escapedText[escPos++] = '\\';
    } else if (text[i] == '\n') {
      escapedText[escPos++] = '\\';
      escapedText[escPos++] = 'n';
    } else if (text[i] == '\r') {
      escapedText[escPos++] = '\\';
      escapedText[escPos++] = 'r';
    } else {
      escapedText[escPos++] = text[i];
    }
  }
  escapedText[escPos] = '\0';

  // Crear línea JSONL
  char newLine[600];
  int len = snprintf(newLine, sizeof(newLine),
                     "{\"i\":%u,\"c\":\"%s\",\"t\":\"%s\",\"p\":%d,\"ts\":%lu}\n",
                     nextMsgId, chat_id, escapedText, (int)priority, millis());

  if (len >= (int)sizeof(newLine)) {
    KISS_CRITICAL(LANG_ERROR_MSG_TOLA);
    KISS_FS.end();
    return false;
  }

  // Append directo
  File file = KISS_FS.open(KISS_FS_QUEUE_FILE, FILE_APPEND);
  if (!file) {
    KISS_FS.end();
    return false;
  }

  size_t written = file.print(newLine);
  file.close();
  KISS_FS.end();

  if (written != strlen(newLine)) {
    KISS_CRITICAL(LANG_ERROR_MSG_WRIT);
    return false;
  }

  nextMsgId++;
  return true;
}

bool KissTelegram::deleteMessageFromFS(uint32_t messageID) {
  cachedPendingCount = -1;

  if (!KISS_FS.begin(KISS_FS_FORMAT_ON_FAIL)) return false;

  File fileIn = KISS_FS.open(KISS_FS_QUEUE_FILE, FILE_READ);
  if (!fileIn) {
    KISS_FS.end();
    return false;
  }

  File fileOut = KISS_FS.open("/queue_tmp.json", FILE_WRITE);
  if (!fileOut) {
    fileIn.close();
    KISS_FS.end();
    return false;
  }

  char line[600];
  bool found = false;

  while (fileIn.available()) {
    int len = 0;
    while (fileIn.available() && len < (int)sizeof(line) - 1) {
      char c = fileIn.read();
      if (c == '\n') break;
      line[len++] = c;
    }
    line[len] = '\0';

    if (len == 0) continue;

    char* idPtr = strstr(line, "\"i\":");
    if (idPtr) {
      uint32_t lineId = strtoul(idPtr + 4, NULL, 10);
      if (lineId == messageID) {
        found = true;
        continue;
      }
    }

    fileOut.print(line);
    fileOut.print('\n');
  }

  fileIn.close();
  fileOut.close();

  KISS_FS.remove(KISS_FS_QUEUE_FILE);
  KISS_FS.rename("/queue_tmp.json", KISS_FS_QUEUE_FILE);
  KISS_FS.end();

  return found;
}

// ========== BATCH && CACHE DELETION TASK ==========
void KissTelegram::queueDeleteMessage(uint32_t messageID) {
  if (pendingDeletes < 15) {  // 25 → 15 (threshold=10 + margen)
    deleteQueue[pendingDeletes++] = messageID;
  } else {
    // Cola llena, forzar flush
    flushDeleteQueue();
    deleteQueue[0] = messageID;
    pendingDeletes = 1;
  }
}

void KissTelegram::removeDuplicateMessagesFromFS() {
  if (!KISS_FS.begin(KISS_FS_FORMAT_ON_FAIL)) return;

  File fileIn = KISS_FS.open(KISS_FS_QUEUE_FILE, FILE_READ);
  if (!fileIn) {
    KISS_FS.end();
    return;
  }

  File fileOut = KISS_FS.open("/queue_tmp.json", FILE_WRITE);
  if (!fileOut) {
    fileIn.close();
    KISS_FS.end();
    return;
  }

  char line[600];
  uint32_t seenIds[50];  // 100 → 50 IDs (ahorra 200 bytes, suficiente para init)
  int seenCount = 0;
  int duplicatesFound = 0;

  while (fileIn.available()) {
    int len = 0;
    while (fileIn.available() && len < (int)sizeof(line) - 1) {
      char c = fileIn.read();
      if (c == '\n') break;
      line[len++] = c;
    }
    line[len] = '\0';

    if (len == 0) continue;

    // Extraer ID del mensaje
    char* idPtr = strstr(line, "\"i\":");
    if (!idPtr) {
      fileOut.print(line);
      fileOut.print('\n');
      continue;
    }

    uint32_t lineId = strtoul(idPtr + 4, NULL, 10);

    // Verificar si ya vimos este ID
    bool isDuplicate = false;
    for (int i = 0; i < seenCount; i++) {
      if (seenIds[i] == lineId) {
        isDuplicate = true;
        duplicatesFound++;
        break;
      }
    }

    if (!isDuplicate) {
      // Primera vez que vemos este ID, guardar, a ver qué está haciendo aquí
      if (seenCount < 50) {  // 100 → 50
        seenIds[seenCount++] = lineId;
      }
      fileOut.print(line);
      fileOut.print('\n');
    }
  }

  fileIn.close();
  fileOut.close();

  KISS_FS.remove(KISS_FS_QUEUE_FILE);
  KISS_FS.rename("/queue_tmp.json", KISS_FS_QUEUE_FILE);
  KISS_FS.end();

  if (duplicatesFound > 0) {
    KISS_LOGF("🧹 Eliminados %d mensajes duplicados del FS", duplicatesFound);
  }

  cachedPendingCount = -1;  // Invalidar caché
}

bool KissTelegram::flushDeleteQueue() {
  if (pendingDeletes == 0) return true;

  cachedPendingCount = -1;

  if (!KISS_FS.begin(KISS_FS_FORMAT_ON_FAIL)) return false;

  File fileIn = KISS_FS.open(KISS_FS_QUEUE_FILE, FILE_READ);
  if (!fileIn) {
    KISS_FS.end();
    pendingDeletes = 0;
    return false;
  }

  File fileOut = KISS_FS.open("/queue_tmp.json", FILE_WRITE);
  if (!fileOut) {
    fileIn.close();
    KISS_FS.end();
    pendingDeletes = 0;
    return false;
  }

  char line[600];
  int deletedCount = 0;

  while (fileIn.available()) {
    int len = 0;
    while (fileIn.available() && len < (int)sizeof(line) - 1) {
      char c = fileIn.read();
      if (c == '\n') break;
      line[len++] = c;
    }
    line[len] = '\0';

    if (len == 0) continue;

    // Buscar si esta línea está en la cola de borrado
    char* idPtr = strstr(line, "\"i\":");
    bool shouldDelete = false;

    if (idPtr) {
      uint32_t lineId = strtoul(idPtr + 4, NULL, 10);
      for (int i = 0; i < pendingDeletes; i++) {
        if (deleteQueue[i] == lineId) {
          shouldDelete = true;
          deletedCount++;
          break;
        }
      }
    }

    if (!shouldDelete) {
      fileOut.print(line);
      fileOut.print('\n');
    }
  }

  fileIn.close();
  fileOut.close();

  KISS_FS.remove(KISS_FS_QUEUE_FILE);
  KISS_FS.rename("/queue_tmp.json", KISS_FS_QUEUE_FILE);
  KISS_FS.end();

  if (diagnosticsVerbose && deletedCount > 0) {
    KISS_LOGF(LANG_INFO_MSG_DEL, deletedCount);
  }

  pendingDeletes = 0;
  return true;
}

// ========== MODO TURBO ==========
void KissTelegram::enableTurboMode() {
  if (!turboMode) {
    originalMinInterval = minMessageInterval;
    minMessageInterval = 50;  // Intervalo muy corto
    turboMode = true;
    turboProcessedTotal = 0;
    turboStartTime = millis();
    KISS_LOG(LANG_INFO_TUR_ACT);
  }
}

void KissTelegram::disableTurboMode() {
  if (turboMode) {
    minMessageInterval = originalMinInterval;
    turboMode = false;

    // Calcular estadísticas
    unsigned long turboTime = safeTimeDiff(millis(), turboStartTime);
    float rate = (turboTime > 0) ? (turboProcessedTotal * 1000.0f / turboTime) : 0;

    KISS_LOGF(LANG_INFO_TUR_DEAC, turboProcessedTotal, turboTime, rate);

    // Flush cualquier delete pendiente de una vez
    flushDeleteQueue();

    turboProcessedTotal = 0;
  }
}

bool KissTelegram::isTurboMode() {
  return turboMode;
}

bool KissTelegram::getNextPendingMessage(char* chat_id, char* text, MessagePriority* priority, uint32_t* msgId) {
  if (!KISS_FS.begin(KISS_FS_FORMAT_ON_FAIL)) return false;

  File file = KISS_FS.open(KISS_FS_QUEUE_FILE, FILE_READ);
  if (!file) {
    KISS_FS.end();
    return false;
  }

  char line[600];
  char bestLine[600] = {0};
  int bestPriority = -1;
  uint32_t bestId = 0;

  while (file.available()) {
    int len = 0;
    while (file.available() && len < (int)sizeof(line) - 1) {
      char c = file.read();
      if (c == '\n') break;
      line[len++] = c;
    }
    line[len] = '\0';

    if (len < 10) continue;

    // Obtener ID del mensaje
    char* idPtr = strstr(line, "\"i\":");
    if (!idPtr) continue;
    uint32_t lineId = strtoul(idPtr + 4, NULL, 10);

    // Saltar mensajes que ya están en cola de borrado de una pasada
    bool inDeleteQueue = false;
    for (int i = 0; i < pendingDeletes; i++) {
      if (deleteQueue[i] == lineId) {
        inDeleteQueue = true;
        break;
      }
    }
    if (inDeleteQueue) continue;

    char* pPtr = strstr(line, "\"p\":");
    if (!pPtr) continue;
    int linePriority = atoi(pPtr + 4);

    if (linePriority > bestPriority) {
      bestPriority = linePriority;
      strncpy(bestLine, line, sizeof(bestLine) - 1);
      bestId = lineId;
    }
  }

  file.close();
  KISS_FS.end();

  if (bestPriority < 0) return false;

  // Extraer chat_id
  char* cPtr = strstr(bestLine, "\"c\":\"");
  if (cPtr) {
    cPtr += 5;
    char* cEnd = strchr(cPtr, '"');
    if (cEnd) {
      int cLen = cEnd - cPtr;
      strncpy(chat_id, cPtr, min(cLen, 19));
      chat_id[min(cLen, 19)] = '\0';
    }
  }

  // Extraer text con decodificación de escapes
  char* tPtr = strstr(bestLine, "\"t\":\"");
  if (tPtr) {
    tPtr += 5;
    char temp[256];
    int tLen = 0;
    bool escaped = false;
    
    while (tPtr[tLen] && tLen < 255) {
      if (escaped) {
        escaped = false;
        tLen++;
        continue;
      }
      if (tPtr[tLen] == '\\') {
        escaped = true;
        tLen++;
        continue;
      }
      if (tPtr[tLen] == '"') break;
      tLen++;
    }
    
    strncpy(temp, tPtr, tLen);
    temp[tLen] = '\0';

    // Decodificar escapes
    char* src = temp;
    char* dst = text;
    while (*src && (dst - text) < 254) {
      if (*src == '\\' && *(src + 1)) {
        src++;
        if (*src == 'n') *dst++ = '\n';
        else if (*src == 'r') *dst++ = '\r';
        else if (*src == '"') *dst++ = '"';
        else if (*src == '\\') *dst++ = '\\';
        else *dst++ = *src;
        src++;
      } else {
        *dst++ = *src++;
      }
    }
    *dst = '\0';
  }

  *priority = (MessagePriority)bestPriority;
  *msgId = bestId;
  return true;
}

int KissTelegram::countPendingMessages() {
  unsigned long now = millis();
  if (cachedPendingCount >= 0 && safeTimeDiff(now, lastCountCheck) < 500) {
    return cachedPendingCount;
  }

  if (!KISS_FS.begin(KISS_FS_FORMAT_ON_FAIL)) {
    cachedPendingCount = 0;
    return 0;
  }

  File file = KISS_FS.open(KISS_FS_QUEUE_FILE, FILE_READ);
  if (!file) {
    KISS_FS.end();
    cachedPendingCount = 0;
    return 0;
  }

  int count = 0;
  char line[600];

  while (file.available()) {
    int len = 0;
    while (file.available() && len < (int)sizeof(line) - 1) {
      char c = file.read();
      if (c == '\n') break;
      line[len++] = c;
    }
    line[len] = '\0';

    // Contar líneas válidas con formato JSON
    if (len >= 10 && strstr(line, "\"i\":")) {
      // Verificar que no esté en deleteQueue
      char* idPtr = strstr(line, "\"i\":");
      if (idPtr) {
        uint32_t lineId = strtoul(idPtr + 4, NULL, 10);
        bool inDeleteQueue = false;
        for (int i = 0; i < pendingDeletes; i++) {
          if (deleteQueue[i] == lineId) {
            inDeleteQueue = true;
            break;
          }
        }
        if (!inDeleteQueue) {
          count++;
        }
      }
    }
  }

  file.close();
  KISS_FS.end();

  cachedPendingCount = count;
  lastCountCheck = now;
  return count;
}

// ========== JSON HELPERS ==========

int KissTelegram::findJSONValue(const String& json, const char* key, int startPos) {
  // Construir clave de búsqueda: "key":
  char searchKey[64];
  snprintf(searchKey, sizeof(searchKey), "\"%s\":", key);

  int pos = json.indexOf(searchKey, startPos);
  if (pos < 0) return -1;
  pos += strlen(searchKey);
  while (pos < json.length() && json.charAt(pos) == ' ') pos++;
  return pos;
}

bool KissTelegram::extractJSONString(const String& json, int startPos, char* buffer, size_t bufferSize) {
  if (!buffer || bufferSize == 0) return false;

  int jsonLen = json.length();
  if (startPos < 0 || startPos >= jsonLen || json.charAt(startPos) != '"') {
    buffer[0] = '\0';
    return false;
  }

  startPos++;
  size_t pos = 0;
  bool escaped = false;

  for (int i = startPos; i < jsonLen && pos < bufferSize - 1; i++) {
    char c = json.charAt(i);
    if (escaped) {
      // Manejar secuencias de escape comunes
      switch (c) {
        case 'n': buffer[pos++] = '\n'; break;
        case 'r': buffer[pos++] = '\r'; break;
        case 't': buffer[pos++] = '\t'; break;
        default:  buffer[pos++] = c; break;
      }
      escaped = false;
      continue;
    }
    if (c == '\\') {
      escaped = true;
      continue;
    }
    if (c == '"') {
      buffer[pos] = '\0';
      return true;
    }
    buffer[pos++] = c;
  }

  // Si llegamos aquí, no encontramos cierre de comillas
  buffer[pos] = '\0';  // Acaba lo que tienes
  return false;
}

int KissTelegram::extractJSONInt(const String& json, int startPos) {
  // Usar buffer fijo en lugar de String dinámico, evita fuga del heap...
  char numStr[12];  // Suficiente para int32
  int pos = 0;
  int len = json.length();
  for (int i = startPos; i < len && pos < 11; i++) {
    char c = json.charAt(i);
    if (c >= '0' && c <= '9') numStr[pos++] = c;
    else break;
  }
  numStr[pos] = '\0';
  return atoi(numStr);
}

int KissTelegram::findNextMessage(const String& json, int startPos) {
  return json.indexOf("{\"i\":", startPos);
}

bool KissTelegram::saveToLittleFS() {
  if (!KISS_FS.begin(KISS_FS_FORMAT_ON_FAIL)) return false;

  File file = KISS_FS.open("/kisstg_state.txt", FILE_WRITE);
  if (!file) {
    KISS_FS.end();
    return false;
  }

  file.printf("TIMESTAMP:%lu\n", millis());
  file.printf("NEXTMSGID:%u\n", nextMsgId);
  file.printf("OFFSET:%ld\n", lastUpdateID);
  file.printf("ENABLED:%d\n", enabled ? 1 : 0);
  file.printf("MINMSGINTERVAL:%d\n", minMessageInterval);

  // Persistir deleteQueue[] antes de reiniciar
  file.printf("PENDINGDELETES:%d\n", pendingDeletes);
  if (pendingDeletes > 0) {
    file.print("DELETEQUEUE:");
    for (int i = 0; i < pendingDeletes; i++) {    
      file.printf("%u", deleteQueue[i]);
      if (i < pendingDeletes - 1) file.print(",");
    }
    file.print("\n");
  }

  file.close();
  KISS_FS.end();
  return true;
}

bool KissTelegram::restoreFromLittleFS() {
  if (!KISS_FS.begin(KISS_FS_FORMAT_ON_FAIL)) return false;
  if (!KISS_FS.exists("/kisstg_state.txt")) {
    KISS_FS.end();
    return false;
  }

  File file = KISS_FS.open("/kisstg_state.txt", FILE_READ);
  if (!file) {
    KISS_FS.end();
    return false;
  }

  char line[256];
  while (file.available()) {
    int idx = 0;
    // Leer línea carácter por carácter
    while (file.available() && idx < sizeof(line) - 1) {
      char c = file.read();
      if (c == '\n') break;
      if (c != '\r') line[idx++] = c;
    }
    line[idx] = '\0';

    // Procesar línea
    if (strncmp(line, "NEXTMSGID:", 10) == 0) {
      nextMsgId = atoi(line + 10);
    }
    else if (strncmp(line, "OFFSET:", 7) == 0) {
      lastUpdateID = atoi(line + 7);
    }
    else if (strncmp(line, "ENABLED:", 8) == 0) {
      enabled = (atoi(line + 8) == 1);
    }
    else if (strncmp(line, "MINMSGINTERVAL:", 15) == 0) {
      minMessageInterval = atoi(line + 15);
    }
    else if (strncmp(line, "PENDINGDELETES:", 15) == 0) {
      pendingDeletes = atoi(line + 15);
    }
    else if (strncmp(line, "DELETEQUEUE:", 12) == 0) {
      // Parsear lista de IDs separados por comas
      char* ptr = line + 12;
      int queueIdx = 0;
      char numBuf[16];
      int numIdx = 0;

      while (*ptr && queueIdx < 15) {
        if (*ptr == ',' || *ptr == '\0') {
          if (numIdx > 0) {
            numBuf[numIdx] = '\0';
            deleteQueue[queueIdx++] = atoi(numBuf);
            numIdx = 0;
          }
          if (*ptr == '\0') break;
          ptr++;
        } else {
          if (numIdx < sizeof(numBuf) - 1) {
            numBuf[numIdx++] = *ptr;
          }
          ptr++;
        }
      }
      // Último número si no termina en coma
      if (numIdx > 0 && queueIdx < 15) {
        numBuf[numIdx] = '\0';
        deleteQueue[queueIdx++] = atoi(numBuf);
      }
    }
  }

  file.close();
  KISS_FS.end();
  return true;
}

// ========== INFORMACIÓN SISTEMA ==========

void KissTelegram::printDiagnostics() {
  int pending = countPendingMessages();
  KISS_LOG(LANG_INFO_DIAG_1);
  KISS_LOGF(LANG_INFO_DIAG_2);
  KISS_LOGF(LANG_INFO_DIAG_3);
  KISS_LOGF(LANG_INFO_DIAG_4);
  KISS_LOGF(LANG_INFO_DIAG_5);
}

void KissTelegram::printStorageStatus() {
  KISS_LOG(LANG_INFO_STOR_1);
  KISS_LOGF(LANG_INFO_STOR_2);
}

void KissTelegram::printSystemStatus() {
  KISS_LOG(LANG_INFO_STATS_1);
  KISS_LOGF(LANG_INFO_STATS_2);
  KISS_LOGF(LANG_INFO_STATS_3);
}

void KissTelegram::printConfiguration() {
  KISS_LOG(LANG_INFO_STATS_4);
  KISS_LOGF(LANG_INFO_STATS_5);
  KISS_LOGF(LANG_INFO_STATS_6);
}

void KissTelegram::printPowerStatistics() {
  KISS_LOG(LANG_INFO_POWER_MODE);
  const char* names[] = { "BOOT", "LOW", "IDLE", "ACTIVE", "TURBO", "MAINT" };
  KISS_LOGF(" - Modo: %s", names[currentPowerMode]);
}

int KissTelegram::getFreeMemory() {
  return ESP.getFreeHeap();
}
int KissTelegram::getQueueSize() {
  return countPendingMessages();
}
bool KissTelegram::isSSLSecure() {
  return sslSecure;
}

int KissTelegram::getStorageUsage() {
  if (!KISS_FS.begin(KISS_FS_FORMAT_ON_FAIL)) return 0;
  int used = KISS_FS.usedBytes();
  KISS_FS.end();
  return used;
}

int KissTelegram::getFreeStorage() {
  if (!KISS_FS.begin(KISS_FS_FORMAT_ON_FAIL)) return 0;
  int free = KISS_FS.totalBytes() - KISS_FS.usedBytes();
  KISS_FS.end();
  return free;
}

bool KissTelegram::isStorageAvailable() {
  return KISS_FS.begin(KISS_FS_FORMAT_ON_FAIL);
}

int KissTelegram::getPendingMessages() {
  return countPendingMessages();
}
int KissTelegram::getFailedMessages() {
  return 0;
}
int KissTelegram::getMessagesInFS() {
  return countPendingMessages();
}
unsigned long KissTelegram::getUptime() {
  return safeTimeDiff(millis(), startTime);
}
String KissTelegram::getVersion() {
  return String(KISS_GET_VERSION());
}

// ========== GESTIÓN DE RED ==========

String KissTelegram::getNetworkInfo() {
  if (!kissNet) return "Sin KissNet";
  return kissNet->getConnectionInfo();
}

String KissTelegram::getNetworkMode() {
  if (!kissNet) return "DESCONOCIDO";

  KissNet::NetworkMode mode = kissNet->getMode();
  switch (mode) {
    case KissNet::MODE_AUTO:
      return "AUTO (WiFi→LTE)";
    case KissNet::MODE_WIFI_ONLY:
      return "WIFI ONLY";
    case KissNet::MODE_LTE_ONLY:
      return "LTE ONLY";
    default:
      return "DESCONOCIDO";
  }
}

bool KissTelegram::setNetworkMode(const char* mode) {
  if (!kissNet || !mode) return false;

  KissNet::NetworkMode newMode;

  if (strcasecmp(mode, "auto") == 0) {
    newMode = KissNet::MODE_AUTO;
  } else if (strcasecmp(mode, "wifi") == 0) {
    newMode = KissNet::MODE_WIFI_ONLY;
  } else if (strcasecmp(mode, "lte") == 0) {
    newMode = KissNet::MODE_LTE_ONLY;
  } else {
    return false;  // Modo inválido
  }

  // Cambiar modo en KissNet
  bool success = kissNet->setMode(newMode);

  // Guardar en configuración persistente
  if (success) {
    KissConfig::getInstance().setNetworkMode((int)newMode);
  }

  return success;
}

bool KissTelegram::switchToLTE() {
  if (!kissNet) return false;

  bool success = kissNet->switchToLTE();

  if (success) {
    // Actualizar configuración para recordar preferencia
    KissConfig::getInstance().setNetworkMode((int)KissNet::MODE_LTE_ONLY);
  }

  return success;
}

bool KissTelegram::switchToWiFi() {
  if (!kissNet) return false;

  bool success = kissNet->switchToWiFi();

  if (success) {
    KissConfig::getInstance().setNetworkMode((int)KissNet::MODE_WIFI_ONLY);
  }

  return success;
}

bool KissTelegram::switchToAuto() {
  if (!kissNet) return false;

  bool success = kissNet->switchToAuto();

  if (success) {
    KissConfig::getInstance().setNetworkMode((int)KissNet::MODE_AUTO);
  }

  return success;
}

// ========== FIN GESTIÓN DE RED ==========

void KissTelegram::getStorageInfo(char* buffer, size_t bufferSize) {
  if (!buffer || bufferSize < 200) return;
  int pending = countPendingMessages();
  snprintf(buffer, bufferSize, LANG_INFO_FS_PEND, pending);
}

void KissTelegram::printStorageInfo() {
  char buffer[200];
  getStorageInfo(buffer, sizeof(buffer));
  KISS_LOG(buffer);
}

void KissTelegram::getDiagnosticInfo(char* buffer, size_t bufferSize) {
  if (!buffer || bufferSize < 300) return;
  snprintf(buffer, bufferSize, LANG_INFO_FS_DIAG, countPendingMessages(), ESP.getFreeHeap());
}

// ========== MANTENIMIENTO ==========
void KissTelegram::cleanupStorage() { /* No hay mensajes enviados que limpiar */
}

void KissTelegram::resetCounters() {
  connectionAttempts = 0;
  failedPings = 0;
  totalMessagesSent = 0;
  totalActiveTime = 0;
  memset(timeInMode, 0, sizeof(timeInMode));
}

void KissTelegram::checkConnectionAge() {
    static unsigned long lastActivityCheck = 0;
    if(!hasTimePassed(lastActivityCheck, 60000)) return;  // Cada minuto
    lastActivityCheck = millis();

    static unsigned long connectionStartTime = 0;

    // Si no hay conexión, resetear el contador
    KissClient* client = kissNet->getActiveClient();
    if(!client || !client->isConnected()) {
        connectionStartTime = 0;
        return;
    }

    // Si es nueva conexión, iniciar contador
    if(connectionStartTime == 0) {
        connectionStartTime = millis();
        return;
    }

    // Cerrar conexiones muy viejas (10 minutos)
    if(safeTimeDiff(millis(), connectionStartTime) > 600000) {  // 10 min
        KISS_LOG(LANG_INFO_CLOSE_CON);
        cleanupConnection();
        connectionStartTime = 0;
    }
}

// ========== FILESYSTEM ==========
size_t KissTelegram::getFSTotalBytes() {
  if (!KISS_FS.begin(KISS_FS_FORMAT_ON_FAIL)) return 0;
  size_t t = KISS_FS.totalBytes();
  KISS_FS.end();
  return t;
}

size_t KissTelegram::getFSUsedBytes() {
  if (!KISS_FS.begin(KISS_FS_FORMAT_ON_FAIL)) return 0;
  size_t u = KISS_FS.usedBytes();
  KISS_FS.end();
  return u;
}

size_t KissTelegram::getFSFreeBytes() {
  if (!KISS_FS.begin(KISS_FS_FORMAT_ON_FAIL)) return 0;
  size_t f = KISS_FS.totalBytes() - KISS_FS.usedBytes();
  KISS_FS.end();
  return f;
}

bool KissTelegram::isFSMounted() {
  return KISS_FS.begin(KISS_FS_FORMAT_ON_FAIL);
}

// ========== OTA ==========
bool KissTelegram::requestOTAPermission(char* errorMsg, int errorMsgSize) {
  if (errorMsg && errorMsgSize > 0) {
    strncpy(errorMsg, LANG_INFO_OTA_AUTHO, errorMsgSize - 1);
    errorMsg[errorMsgSize - 1] = '\0';
  }
  return true;
}

bool KissTelegram::isOTAMode() {
  return otaMode;
}
void KissTelegram::setOTAMode(bool enabled) {
  otaMode = enabled;
}

bool KissTelegram::sendOTAMessage(const char* chat_id, const char* text) {
  if (!otaMode) return false;
  // Usar sendMessageDirect para envío inmediato sin rate-limit
  // Los mensajes OTA deben llegar SIEMPRE, no pueden encolarse
  return sendMessageDirect(chat_id, text);
}

// ========== PRIVADOS ==========
bool KissTelegram::isConnectionReallyStable() {
  // Rate-limit: máximo 1 vez cada 30 s
  static unsigned long lastStableCheck = 0;
  if (!hasTimePassed(lastStableCheck, 30000)) return true;  // ← asumimos estable
  lastStableCheck = millis();

  if (WiFi.status() != WL_CONNECTED) {
    currentQuality = QUALITY_DEAD;
    return false;
  }
  if (wifiStableTime == 0) return false;

  if (!testBasicConnectivity()) {
    failedPings++;
    currentQuality = QUALITY_POOR;
    return false;
  }

  unsigned long timeStable = safeTimeDiff(millis(), wifiStableTime);
  bool stable = (timeStable > wifiStabilityThreshold);
  if (stable) {
    currentQuality = (failedPings == 0) ? QUALITY_GOOD : QUALITY_FAIR;
  }
  return stable;
}

bool KissTelegram::testBasicConnectivity() {
  WiFiClient testClient;
  if (testClient.connect("www.google.com", 80)) {
    testClient.stop();
    return true;
  }
  return false;
}

bool KissTelegram::isConnected() {
  KissClient* client = kissNet->getActiveClient();
  return client && client->isConnected();
}

bool KissTelegram::testSSLConnection() {
  KISS_LOG(LANG_INFO_SSL_TEST_0);

  KissClient* client = kissNet->getActiveClient();
  if (!client) {
    KISS_LOG(LANG_ERROR_SSL_TEST_3);
    return false;
  }

  KISS_LOG(LANG_INFO_SSL_TEST_1);
  bool result = client->connectToTelegram();

  if (result) {
    KISS_LOG(LANG_INFO_SSL_TEST_2);
    client->printInfo();
  } else {
    KISS_LOG(LANG_ERROR_SSL_TEST_3);
  }

  return result;
}

bool KissTelegram::connectToTelegram() {
  static unsigned long lastCall = 0;
  unsigned long now = millis();

  // Rate limit: máximo 1 vez cada 2 segundos
  if (now - lastCall < 2000) {
    if (diagnosticsVerbose) {
      KISS_LOGF(LANG_INFO_CON_LIM, now - lastCall);
    }
    KissClient* client = kissNet->getActiveClient();
    return client && client->isConnected();
  }

  lastCall = now;

  // Asegurar conectividad de red (KissNet maneja WiFi↔LTE automáticamente)
  if (!kissNet->ensureConnection()) {
    KISS_LOG("❌ Sin conectividad de red");
    return false;
  }

  KissClient* client = kissNet->getActiveClient();
  if (!client) {
    KISS_LOG("❌ No hay cliente activo");
    return false;
  }

  // Si el socket sigue vivo, no hacemos nada
  if (client->isConnected()) {
    return true;
  }

  // AUTO-CONFIGURACIÓN INTELIGENTE basada en NTP
  bool shouldBeSecure = KissTime::getInstance().isTimeSynced();

  if (client->isSecureMode() != shouldBeSecure) {
    if (diagnosticsVerbose) {
      KISS_LOGF(LANG_INFO_SSL_INTELL,
                client->isSecureMode() ? "SECURE" : "INSECURE",
                shouldBeSecure ? "SECURE" : "INSECURE");
    }
    client->setSecureMode(shouldBeSecure);
  }

  bool ok = client->connectToTelegram();
  if (ok) {
    KISS_LOG(LANG_INFO_SSL_SUCC_1);
    lastPingTime = millis();
    sslSecure = client->isSecureMode();
    return true;
  }

  // Conexión falló - KissNet se encarga del failover automático
  KISS_LOG(LANG_ERROR_SSL_FAIL_1);
  return false;
}


bool KissTelegram::pingTelegram() {
  KissClient* client = kissNet->getActiveClient();
  if (!client || !client->isConnected()) return false;

  KISS_LOG(LANG_INFO_PING_TG);

  // ✅ Enviar HTTP completo
  client->print("GET /bot");
  client->print(botToken);
  client->print("/getMe HTTP/1.1\r\n");
  client->print("Host: api.telegram.org\r\n");

  // ✅ OPTIMIZACIÓN LTE: usar Connection header apropiado
  if (shouldUseKeepAlive()) {
      client->print("Connection: keep-alive\r\n");
  } else {
      client->print("Connection: close\r\n");
  }

  client->print("Content-Length: 0\r\n\r\n");

  // Leer y DESCARTAR toda la respuesta del servidor
  // para evitar contaminar el buffer en el siguiente checkMessages()
  unsigned long timeout = millis();
  bool foundOK = false;
  int bytesRead = 0;

  // Leer hasta que no haya más datos disponibles (con timeout de 100ms sin datos)
  while (client->connected() && (millis() - timeout < 2000)) {
    if (client->available()) {
      char c = client->read();
      bytesRead++;

      // Solo verificar "200 OK" en los primeros 128 bytes (headers HTTP)
      if (!foundOK && bytesRead < 128) {
        static char checkBuf[4] = {0};
        checkBuf[0] = checkBuf[1];
        checkBuf[1] = checkBuf[2];
        checkBuf[2] = c;
        if (checkBuf[0] == '2' && checkBuf[1] == '0' && checkBuf[2] == '0') {
          foundOK = true;
        }
      }

      timeout = millis();  // reset por cada byte
    } else if (bytesRead > 0 && (millis() - timeout > 100)) {
      // 100ms sin datos tras recibir algo = respuesta completa
      break;
    }
    yield();
  }

  KISS_LOGF(LANG_INFO_PING_REP, bytesRead);

  // ✅ OPTIMIZACIÓN LTE: cerrar socket después de ping si es necesario
  if (shouldCloseAfterRequest()) {
    if (diagnosticsVerbose) {
      KISS_LOG("📡 LTE: Cerrando socket tras ping para permitir sleep");
    }
    cleanupConnection();
  }

  return foundOK;
}

bool KissTelegram::readResponse() {
  TgAck ack;
  return readResponse(ack);
}

bool KissTelegram::readResponse(TgAck& ack) {
  ack.success = false;
  ack.httpCode = 0;
  ack.retryAfter = 0;

  resetBuffers();

  KissClient* client = kissNet->getActiveClient();
  if (!client) return false;

  // Esperar hasta 500ms para que lleguen datos iniciales
  // Esto evita el race condition cuando checkMessages() se ejecuta
  // inmediatamente después de sendMessage()
  unsigned long waitStart = millis();
  while (!client->available() && client->connected() && (millis() - waitStart < 500)) {
    SAFE_YIELD();
  }

  // Si no llegaron datos y la conexión murió, salir
  if (!client->connected() && !client->available()) {
    return false;
  }

  unsigned long timeout = millis();
  int pos = 0;
  int contentLength = -1;
  int bodyStart = -1;
  bool foundContentLength = false;
  bool foundHttpCode = false;

  // Fase 1: Leer headers hasta encontrar Content-Length y "\r\n\r\n"
  while (client->connected() && (millis() - timeout < 2000)) {
    if (client->available()) {
      char c = client->read();
      if (pos < JSON_BUFFER_SIZE - 1) jsonBuffer[pos++] = c;
      jsonBuffer[pos] = '\0';  // null-terminate en cada iteración
      timeout = millis();

      // CAPTURAR httpCode DURANTE la lectura, no después
      if (!foundHttpCode && pos > 12) {
        char* httpLine = strstr(jsonBuffer, "HTTP/1.");
        if (httpLine) {
          sscanf(httpLine, "HTTP/1.%*d %hu", &ack.httpCode);
          foundHttpCode = true;
        }
      }

      // Buscar Content-Length SOLO si aún no lo encontramos
      // Buscar tanto con mayúsculas como minúsculas
      if (!foundContentLength && pos > 20) {
        char* clPtr = strstr(jsonBuffer, "Content-Length: ");
        if (!clPtr) clPtr = strstr(jsonBuffer, "content-length: ");
        if (!clPtr) clPtr = strstr(jsonBuffer, "Content-length: ");

        if (clPtr) {
          // Avanzar hasta después del ": " (ya sabemos que está ahí)
          clPtr += 15;  // Longitud de "Content-Length:"
          while (*clPtr == ' ') clPtr++;  // Saltar espacios
          contentLength = atoi(clPtr);
          foundContentLength = true;
        }
      }

      // Detectar fin de headers mirando los últimos 4 caracteres
      if (bodyStart == -1 && pos >= 4) {
        if (jsonBuffer[pos-4] == '\r' && jsonBuffer[pos-3] == '\n' &&
            jsonBuffer[pos-2] == '\r' && jsonBuffer[pos-1] == '\n') {
          bodyStart = pos;
        }
      }

      // Si ya tenemos el body completo según Content-Length, PARAR
      if (bodyStart > 0 && contentLength > 0) {
        int bytesRead = pos - bodyStart;
        if (bytesRead >= contentLength) {
          // Ya leímos exactamente Content-Length bytes del body
          break;
        }
      }
    } else if (pos > 0 && (millis() - timeout > 100)) {
      // Timeout de 100ms sin datos
      break;
    }
    SAFE_YIELD();
  }
  jsonBuffer[pos] = '\0';

  ack.success = (ack.httpCode == 200 && strstr(jsonBuffer, "\"ok\":true") != nullptr);

  if (!ack.success) {
    const char* ra = strstr(jsonBuffer, "\"retry_after\":");
    if (ra) sscanf(ra, "\"retry_after\":%lu", &ack.retryAfter);
  }

  return ack.success;
}

int KissTelegram::timedRead(unsigned long startTime) {
  KissClient* client = kissNet->getActiveClient();
  if (!client) return -1;

  while (true) {
    int c = client->read();
    if (c >= 0) return c;
    if (!client->connected()) return -1;
    if (millis() - startTime > TG_TIMEOUT_MS) return -1;
    SAFE_YIELD();
  }
}

void KissTelegram::resetBuffers() {
  memset(jsonBuffer, 0, JSON_BUFFER_SIZE);
  memset(messageBuffer, 0, MESSAGE_BUFFER_SIZE);
  memset(commandBuffer, 0, COMMAND_BUFFER_SIZE);
  memset(paramBuffer, 0, PARAM_BUFFER_SIZE);
}

void KissTelegram::extractCommand(const char* text, char* command, char* param) {
  command[0] = '\0';
  param[0] = '\0';
  if (!text || text[0] != '/') return;

  const char* spacePos = strchr(text, ' ');
  if (spacePos) {
    int cmdLen = spacePos - text;
    strncpy(command, text, min(cmdLen, 31));
    command[min(cmdLen, 31)] = '\0';
    strncpy(param, spacePos + 1, 63);
    param[63] = '\0';
  } else {
    strncpy(command, text, 31);
    command[31] = '\0';
  }
}

void KissTelegram::logSystemEvent(const char* event, const char* data) {
  if (systemEventCallback) systemEventCallback(event, data);
}

void KissTelegram::transitionToPowerMode(PowerMode newMode) {
  if (currentPowerMode == newMode) return;
  PowerMode oldMode = currentPowerMode;
  currentPowerMode = newMode;
  powerModeChangeTime = millis();
  modeStartTime = millis();
  notifyPowerModeChange(oldMode, newMode);
}

void KissTelegram::updatePowerStatistics() {
  unsigned long now = millis();
  unsigned long timeInCurrentMode = safeTimeDiff(now, modeStartTime);
  if (currentPowerMode >= POWER_BOOT && currentPowerMode <= POWER_MAINTENANCE) {
    timeInMode[currentPowerMode] += timeInCurrentMode;
  }
  if (currentPowerMode == POWER_ACTIVE || currentPowerMode == POWER_TURBO) {
    totalActiveTime += timeInCurrentMode;
  }
  modeStartTime = now;
}

bool KissTelegram::isTransitionComplete() {
  return (currentPowerMode == targetPowerMode);
}
bool KissTelegram::canTransitionTo(PowerMode newMode) {
  return true;
}

void KissTelegram::notifyPowerModeChange(PowerMode oldMode, PowerMode newMode) {
  if (powerModeCallback) powerModeCallback(oldMode, newMode);
}

void KissTelegram::updateSSLMode() {
  KissClient* client = kissNet->getActiveClient();
  if (!client) return;

  bool shouldBeSecure = KissTime::getInstance().isTimeSynced();

  if (client->isSecureMode() != shouldBeSecure) {
    client->setSecureMode(shouldBeSecure);
  }
}

// ========== OPTIMIZACIÓN LTE ==========

bool KissTelegram::isUsingLTE() {
  return kissNet && (kissNet->getActiveNetwork() == KissNet::NET_LTE);
}

bool KissTelegram::shouldUseKeepAlive() {
  // WiFi: usar keep-alive para máxima eficiencia
  // LTE: NO usar keep-alive para permitir sleep mode
  if (isUsingLTE()) {
    // En modo LTE, solo usar keep-alive en POWER_ACTIVE o POWER_TURBO
    return (currentPowerMode == POWER_ACTIVE || currentPowerMode == POWER_TURBO);
  }

  // WiFi siempre usa keep-alive (no tiene sleep mode significativo)
  return true;
}

bool KissTelegram::shouldCloseAfterRequest() {
  // OPTIMIZACIÓN: Como LTE es fallback de WiFi y se usa poco,
  // mantener siempre conectado en sleep mode es más eficiente
  // que re-registrarse cada vez (30-60s vs 10-12mA constantes)

  // LTE: nunca cerrar conexión (mantener en sleep mode)
  // WiFi: nunca cerrar (keep-alive siempre)
  return false;
}

int KissTelegram::getRecommendedPingInterval() {
  if (isUsingLTE()) {
    // LTE: intervalos más largos según power mode
    switch (currentPowerMode) {
      case POWER_LOW:
      case POWER_MAINTENANCE:
        return 600000;  // 10 minutos (prácticamente sin pings)

      case POWER_IDLE:
        return 300000;  // 5 minutos

      case POWER_BOOT:
        return 180000;  // 3 minutos

      case POWER_ACTIVE:
        return 120000;  // 2 minutos

      case POWER_TURBO:
        return 60000;   // 1 minuto (como WiFi)

      default:
        return 300000;  // 5 minutos por defecto
    }
  }

  // WiFi: ping cada 60 segundos (comportamiento original)
  return 60000;
}

void KissTelegram::optimizeConnectionForNetwork() {
  KissClient* client = kissNet->getActiveClient();
  if (!client) return;

  if (kissNet->getActiveNetwork() == KissNet::NET_LTE) {
    // Configurar modo de ahorro según power mode
    KissClientPowerMode clientMode;

    switch (currentPowerMode) {
      case POWER_LOW:
        clientMode = CLIENT_POWER_LOW;  // AT+CSCLK=1
        if (diagnosticsVerbose) {
          KISS_LOG("📡 LTE optimizado: SLEEP MODE activado (3-7 mA)");
        }
        break;

      case POWER_IDLE:
        clientMode = CLIENT_POWER_IDLE;  // Sleep mode con DTR o activo sin DTR
        if (diagnosticsVerbose) {
          // El mensaje se adaptará automáticamente según DTR esté o no configurado
          KISS_LOG("📡 LTE optimizado: MODO IDLE (12-18 mA según DTR)");
        }
        break;

      case POWER_ACTIVE:
      case POWER_TURBO:
        clientMode = CLIENT_POWER_ACTIVE;  // AT+CSCLK=0
        if (diagnosticsVerbose) {
          KISS_LOG("📡 LTE optimizado: MODO ACTIVO (~100 mA)");
        }
        break;

      case POWER_MAINTENANCE:
        clientMode = CLIENT_POWER_MAINTENANCE;  // Mínimo
        if (diagnosticsVerbose) {
          KISS_LOG("📡 LTE optimizado: MANTENIMIENTO (~8 mA)");
        }
        break;

      default:
        clientMode = CLIENT_POWER_BOOT;
        break;
    }

    client->setPowerMode(clientMode);

    if (diagnosticsVerbose) {
      KISS_LOG("📡 LTE: optimizaciones aplicadas");
    }
  } else {
    // WiFi: configuración estándar
    if (diagnosticsVerbose) {
      KISS_LOG("📶 WiFi: keep-alive estándar");
    }
  }
}

// ========== REPORTES DE ERROR ==========

const char* KissTelegram::getFailureReasonText(SendFailureReason reason) {
  switch (reason) {
    case SEND_OK:
      return "OK";
    case SEND_FAIL_DISABLED:
      return "Bot deshabilitado";
    case SEND_FAIL_RATE_LIMIT:
      return "Rate limit (encolado)";
    case SEND_FAIL_WIFI_UNSTABLE:
      return "WiFi inestable (encolado)";
    case SEND_FAIL_NO_CLIENT:
      return "Sin cliente activo";
    case SEND_FAIL_CONNECTION:
      return "Fallo conexión SSL";
    case SEND_FAIL_HTTP_ERROR:
      return "Error HTTP";
    case SEND_FAIL_READ_RESPONSE:
      return "Fallo leer respuesta";
    case SEND_FAIL_QUEUE_FULL:
      return "Cola llena";
    default:
      return "Desconocido";
  }
}

void KissTelegram::sendErrorReport(const char* chat_id, SendFailureReason reason, const char* context) {
  if (reason == SEND_OK) return;  // No enviar si no hay error

  char errorMsg[600];

  // Obtener información del sistema
  KissClient* client = kissNet->getActiveClient();
  const char* network = isUsingLTE() ? "LTE" : "WiFi";
  int rssi = client ? client->getSignalStrength() : -999;
  bool connected = client ? client->isConnected() : false;
  unsigned long uptime = safeTimeDiff(millis(), startTime) / 1000;
  unsigned long socketAge = client ? client->getConnectionAge() / 1000 : 0;

  snprintf(errorMsg, sizeof(errorMsg),
           "⚠️ ERROR ENVÍO MENSAJE\n\n"
           "❌ Causa: %s\n"
           "📡 Red: %s (%d dBm)\n"
           "🔌 Socket: %s\n"
           "⏱️ Socket edad: %lu seg\n"
           "🔒 SSL: %s\n"
           "💾 Memoria: %d bytes\n"
           "⏰ Uptime: %lu seg\n"
           "📊 Power Mode: %d\n"
           "💾 Cola: %d msgs\n",
           getFailureReasonText(reason),
           network,
           rssi,
           connected ? "Conectado" : "Desconectado",
           socketAge,
           sslSecure ? "Seguro" : "Inseguro",
           ESP.getFreeHeap(),
           uptime,
           currentPowerMode,
           countPendingMessages());

  // Añadir contexto adicional si se proporciona
  if (context && strlen(context) > 0) {
    size_t currentLen = strlen(errorMsg);
    snprintf(errorMsg + currentLen, sizeof(errorMsg) - currentLen,
             "\nℹ️ Contexto: %s", context);
  }

  // Añadir últimos eventos del log
  String lastEvents = getLastEvents(5);
  if (lastEvents.length() > 0) {
    size_t currentLen = strlen(errorMsg);
    snprintf(errorMsg + currentLen, sizeof(errorMsg) - currentLen,
             "\n\n📋 Últimos eventos:\n%s", lastEvents.c_str());
  }

  // Registrar el error en el log
  logEvent("ERROR_ENVIO", getFailureReasonText(reason));

  // Usar queueMessage con prioridad HIGH para asegurar el envío
  // No usar sendMessage para evitar recursión infinita
  queueMessage(chat_id, errorMsg, PRIORITY_HIGH);

  KISS_LOGF("📤 Reporte de error encolado: %s", getFailureReasonText(reason));
}

// ========== LOGGING Y DIAGNÓSTICO ==========

void KissTelegram::logEvent(const char* event, const char* details) {
  if (!KISS_FS.begin(KISS_FS_FORMAT_ON_FAIL)) return;

  File file = KISS_FS.open("/kiss_events.log", FILE_APPEND);
  if (!file) {
    KISS_FS.end();
    return;
  }

  unsigned long uptime = safeTimeDiff(millis(), startTime);
  char timestamp[32];
  snprintf(timestamp, sizeof(timestamp), "%lu", uptime / 1000);

  file.print(timestamp);
  file.print("|");
  file.print(event);
  if (details && strlen(details) > 0) {
    file.print("|");
    file.print(details);
  }
  file.println();

  file.close();
  KISS_FS.end();
}

void KissTelegram::logSocketState(const char* when) {
  KissClient* client = kissNet->getActiveClient();
  if (!client) {
    logEvent("SOCKET_CHECK", "NO_CLIENT");
    return;
  }

  bool connected = client->isConnected();
  unsigned long age = client->getConnectionAge() / 1000;
  int rssi = client->getSignalStrength();

  char details[128];
  snprintf(details, sizeof(details), "%s|%s|age=%lu|rssi=%d",
           when,
           connected ? "CONNECTED" : "DISCONNECTED",
           age,
           rssi);

  logEvent("SOCKET_STATE", details);
}

String KissTelegram::getLastEvents(int count) {
  if (!KISS_FS.begin(KISS_FS_FORMAT_ON_FAIL)) return "";

  File file = KISS_FS.open("/kiss_events.log", FILE_READ);
  if (!file) {
    KISS_FS.end();
    return "";
  }

  // Contar líneas totales
  int totalLines = 0;
  while (file.available()) {
    file.readStringUntil('\n');
    totalLines++;
  }

  // Volver al inicio y saltar hasta las últimas N líneas
  file.seek(0);
  int skipLines = max(0, totalLines - count);
  for (int i = 0; i < skipLines; i++) {
    file.readStringUntil('\n');
  }

  // Leer últimas líneas
  String result = "";
  int lineNum = 0;
  while (file.available() && lineNum < count) {
    String line = file.readStringUntil('\n');
    line.trim();

    if (line.length() > 0) {
      // Formatear: timestamp|event|details -> "evento (detalles) +XXXs"
      int firstPipe = line.indexOf('|');
      if (firstPipe > 0) {
        String timestamp = line.substring(0, firstPipe);
        String rest = line.substring(firstPipe + 1);

        int secondPipe = rest.indexOf('|');
        String event = (secondPipe > 0) ? rest.substring(0, secondPipe) : rest;
        String details = (secondPipe > 0) ? rest.substring(secondPipe + 1) : "";

        result += "+";
        result += timestamp;
        result += "s ";
        result += event;
        if (details.length() > 0 && details.length() < 40) {
          result += " (";
          result += details;
          result += ")";
        }
        result += "\n";

        lineNum++;
      }
    }
  }

  file.close();
  KISS_FS.end();

  return result;
}

void KissTelegram::clearEventLog() {
  if (!KISS_FS.begin(KISS_FS_FORMAT_ON_FAIL)) return;
  KISS_FS.remove("/kiss_events.log");
  KISS_FS.end();
  KISS_LOG("🗑️ Log de eventos limpiado");
}
