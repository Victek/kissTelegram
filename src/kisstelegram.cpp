// KissTelegram.cpp
// Vicente Soriano - victek@gmail.com

#include "KissTelegram.h"
#include "KissConfig.h"
#include "KissTime.h"
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

  // Inicializar cliente de red (WiFi o LTE según configuración)
  networkClient = nullptr;
  usingLTE = false;
  initializeNetworkClient();

  sslSecure = false;

  enabled = true;
  wifiStableTime = millis();
  currentQuality = QUALITY_FAIR;
  connectionAttempts = 0;
  failedPings = 0;
  lastUpdateID = 0;

  lastMessageTime = 0;
  minMessageInterval = 300;

  pollingTimeout = 2;
  adaptivePolling = true;

  maxRetryAttempts = 3;
  baseBackoffMs = 1000;
  maxBackoffMs = 10000;
  fileReceivedCallback = nullptr;

  jsonBuffer = new char[JSON_BUFFER_SIZE];
  messageBuffer = new char[MESSAGE_BUFFER_SIZE];
  commandBuffer = new char[COMMAND_BUFFER_SIZE];
  paramBuffer = new char[PARAM_BUFFER_SIZE];
  maxMessageSize = 256;

  storageEnabled = true;
  storageMode = STORAGE_FULL;
  maxQueueStorage = KISS_MAX_FS_QUEUE;
  lastSaveTime = 0;
  autoSaveInterval = 300000;
  storageCompression = false;
  nextMsgId = 1;

  // ⚡ Inicializar caché
  cachedPendingCount = -1;
  lastCountCheck = 0;

  operationMode = MODE_BALANCED;
  connectionTimeout = 10000;
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

  if (networkClient) {
    networkClient->disconnect();
    // Solo eliminar si es KissSSL (KissLTE es singleton)
    if (!usingLTE) {
      delete networkClient;
    }
    networkClient = nullptr;
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
  if (networkClient && networkClient->isConnected()) {
    networkClient->disconnect();
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

void KissTelegram::setMaxMessageSize(int size) {
  maxMessageSize = max(64, min(size, 4096));
}
int KissTelegram::getMaxMessageSize() {
  return maxMessageSize;
}

void KissTelegram::setMaxRetryAttempts(int attempts) {
  maxRetryAttempts = max(1, attempts);
}
int KissTelegram::getMaxRetryAttempts() {
  return maxRetryAttempts;
}

void KissTelegram::setRetryBackoff(int baseMs, int maxMs) {
  baseBackoffMs = max(100, baseMs);
  maxBackoffMs = maxMs > 0 ? max(maxBackoffMs, baseBackoffMs) : 0;
}
int KissTelegram::getRetryBackoffBase() {
  return baseBackoffMs;
}
int KissTelegram::getRetryBackoffMax() {
  return maxBackoffMs;
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

void KissTelegram::setStorageCompression(bool enable) {
  storageCompression = enable;
}
bool KissTelegram::getStorageCompression() {
  return storageCompression;
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

void KissTelegram::setConnectionTimeout(int timeoutMs) {
  connectionTimeout = max(1000, timeoutMs);
}
int KissTelegram::getConnectionTimeout() {
  return connectionTimeout;
}

void KissTelegram::setWifiStabilityThreshold(int minUptimeMs) {
  wifiStabilityThreshold = max(5000, minUptimeMs);
}
int KissTelegram::getWifiStabilityThreshold() {
  return wifiStabilityThreshold;
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
  if (networkClient) {
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

    networkClient->setPowerMode(clientMode);

    // ✅ OPTIMIZACIÓN LTE: aplicar optimizaciones de conexión
    optimizeConnectionForNetwork();

    if (diagnosticsVerbose) {
      KISS_LOGF("⚡ Power mode: %s → Consumo estimado: %d mA",
                networkClient->getClientType(),
                networkClient->getCurrentConsumption());
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
    if(!networkClient) return;
    
    // ✅ SOLO cerrar si el socket está realmente muerto
    if(networkClient->isConnected()) {
        KISS_LOG(LANG_INFO_CON_RESUME);
        return;  // ← NO cerrar conexiones vivas
    }
    
    // ❌ Solo limpiar sockets zombies
    KISS_LOG(LANG_INFO_CON_DIE);
    networkClient->stop();
}

bool KissTelegram::isConnectionAlive() {
    if(!networkClient || !networkClient->isConnected()) {
        return false;
    }

    // DISABLED: Si el socket muere, networkClient->isConnected() lo detectará

    return true;
}

bool KissTelegram::sendMessage(const char* chat_id, const char* text, MessagePriority priority) {
    if(!enabled) return false;

    unsigned long now = millis();
    unsigned long timeSinceLastMsg = safeTimeDiff(now, lastMessageTime);

    // Límite de velocidad entre mensajes
    if(lastMessageTime > 0 && timeSinceLastMsg < minMessageInterval) {
        return queueMessage(chat_id, text, priority);
    }

    // Verificar WiFi estable
    if(!isWifiStable()) {
        return queueMessage(chat_id, text, priority);
    }

    connectionAttempts++;

    //REUTILIZAR conexión existente si está viva
    if(!isConnectionAlive()) {
        if(!connectToTelegram()) {
            return queueMessage(chat_id, text, priority);
        }
    }

    // CONSTRUIR Y ENVIAR MENSAJE
    networkClient->print("POST /bot");
    networkClient->print(botToken);
    networkClient->print("/sendMessage HTTP/1.1\r\n");
    networkClient->print("Host: api.telegram.org\r\n");
    networkClient->print("Content-Type: application/json\r\n");

    // Calcular longitud del JSON
    char jsonLengthStr[10];
    int jsonLength = snprintf(nullptr, 0, "{\"chat_id\":\"%s\",\"text\":\"%s\"}", chat_id, text);
    snprintf(jsonLengthStr, sizeof(jsonLengthStr), "%d", jsonLength);

    networkClient->print("Content-Length: ");
    networkClient->print(jsonLengthStr);

    // ✅ OPTIMIZACIÓN LTE: usar Connection header apropiado
    if (shouldUseKeepAlive()) {
        networkClient->print("\r\nConnection: keep-alive\r\n\r\n");
    } else {
        networkClient->print("\r\nConnection: close\r\n\r\n");
    }

    networkClient->print("{\"chat_id\":\"");
    networkClient->print(chat_id);
    networkClient->print("\",\"text\":\"");
    networkClient->print(text);
    networkClient->print("\"}");

    //LEER RESPUESTA
    TgAck ack;
    bool success = readResponse(ack);

    // ✅ OPTIMIZACIÓN LTE: cerrar socket si es necesario
    if(success) {
        lastMessageTime = now;
        lastActivityTime = now;
        totalMessagesSent++;

        // Cerrar conexión en modo LTE low-power
        if (shouldCloseAfterRequest()) {
            if (diagnosticsVerbose) {
                KISS_LOG("📡 LTE: Cerrando socket para permitir sleep");
            }
            cleanupConnection();
        }
     } else {
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
        queueMessage(chat_id, text, priority);
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
    networkClient->print("POST /bot");
    networkClient->print(botToken);
    networkClient->print("/sendMessage HTTP/1.1\r\n");
    networkClient->print("Host: api.telegram.org\r\n");
    networkClient->print("Content-Type: application/json\r\n");

    // Calcular longitud del JSON
    char jsonLengthStr[10];
    int jsonLength = snprintf(nullptr, 0, "{\"chat_id\":\"%s\",\"text\":\"%s\"}", chat_id, text);
    snprintf(jsonLengthStr, sizeof(jsonLengthStr), "%d", jsonLength);

    networkClient->print("Content-Length: ");
    networkClient->print(jsonLengthStr);

    // ✅ OPTIMIZACIÓN LTE: usar Connection header apropiado
    if (shouldUseKeepAlive()) {
        networkClient->print("\r\nConnection: keep-alive\r\n\r\n");
    } else {
        networkClient->print("\r\nConnection: close\r\n\r\n");
    }

    networkClient->print("{\"chat_id\":\"");
    networkClient->print(chat_id);
    networkClient->print("\",\"text\":\"");
    networkClient->print(text);
    networkClient->print("\"}");

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

  networkClient->print("GET /bot");
  networkClient->print(botToken);
  networkClient->print("/getUpdates?offset=");
  networkClient->print(offsetStr);
  networkClient->print("&timeout=");
  networkClient->print(timeoutStr);
  networkClient->print("&limit=10 HTTP/1.1\r\n");
  networkClient->print("Host: api.telegram.org\r\n");

  // ✅ OPTIMIZACIÓN LTE: usar Connection header apropiado
  if (shouldUseKeepAlive()) {
      networkClient->print("Connection: keep-alive\r\n\r\n");
  } else {
      networkClient->print("Connection: close\r\n\r\n");
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

void KissTelegram::setPollingTimeout(int seconds) {
  pollingTimeout = max(0, min(seconds, 10));
}

int KissTelegram::getPollingTimeout() {
  return pollingTimeout;
}

void KissTelegram::setAdaptivePolling(bool enable) {
  adaptivePolling = enable;
}

// ========== SSL Casi inteligente.. más que yo, seguro ==========
bool KissTelegram::trySecureConnection() {
  KISS_LOG(LANG_WARN_TSSL);

  // DIAGNÓSTICO COMPLETO
  bool timeSynced = KissTime::getInstance().isTimeSynced();
  KISS_LOGF(LANG_INFO_TSYNC);
  KISS_LOGF(LANG_INFO_FRAM, ESP.getFreeHeap());

  // Configurar certificado (setCACert retorna void)
  networkClient->setCACert(TELEGRAM_ROOT_CA);
  KISS_LOG(LANG_INFO_CERT);

  // Verificar mediante el getter
  if (!networkClient) {
    KISS_CRITICAL(LANG_ERROR_SSLCLI);
    return false;
  }

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
  bool connected = networkClient->connect("api.telegram.org", 443);
  unsigned long connectTime = millis() - startTime;

  KISS_LOGF(LANG_INFO_SSL_TEST, connectTime);

  if (connected) {
    KISS_LOGF(LANG_INFO_SSL_SUCC, connectTime);

    // Verificar certificado solo si el NTP está sincronizado
    if (timeSynced) {
      if (networkClient->verify("api.telegram.org", NULL)) {
        KISS_LOG(LANG_INFO_CERT_VERI);
        sslSecure = true;
      } else {
        KISS_CRITICAL(LANG_INFO_CERT_FAIL);
        networkClient->stop();
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
  networkClient->setInsecure();

  // DIAGNÓSTICO
  KISS_LOGF(LANG_INFO_FRRAM, ESP.getFreeHeap());

  unsigned long startTime = millis();
  bool connected = networkClient->connect("api.telegram.org", 443);
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

String KissTelegram::getSSLInfo() {
  char info[256];  // Buffer fijo en lugar de String
  snprintf(info, sizeof(info), LANG_INFO_SSL);
  return String(info);  // Solo una conversión al final
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
  String searchKey = String("\"") + key + "\":";
  int pos = json.indexOf(searchKey, startPos);
  if (pos < 0) return -1;
  pos += searchKey.length();
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

  while (file.available()) {
    String line = file.readStringUntil('\n');
    line.trim();
    if (line.startsWith("NEXTMSGID:")) nextMsgId = line.substring(10).toInt();
    else if (line.startsWith("OFFSET:")) lastUpdateID = line.substring(7).toInt();
    else if (line.startsWith("ENABLED:")) enabled = (line.substring(8).toInt() == 1);
    else if (line.startsWith("MINMSGINTERVAL:")) minMessageInterval = line.substring(15).toInt();
    else if (line.startsWith("PENDINGDELETES:")) {
      pendingDeletes = line.substring(15).toInt();
    }
    else if (line.startsWith("DELETEQUEUE:")) {
      String ids = line.substring(12);
      int idx = 0;
      int start = 0;
      for (int i = 0; i <= ids.length() && idx < 15; i++) {  // 25 → 15
        if (i == ids.length() || ids.charAt(i) == ',') {
          if (i > start) {
            deleteQueue[idx++] = ids.substring(start, i).toInt();
          }
          start = i + 1;
        }
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
    if(!networkClient || !networkClient->isConnected()) {
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
  return networkClient && networkClient->isConnected();
}

bool KissTelegram::testSSLConnection() {
  KISS_LOG(LANG_INFO_SSL_TEST_0);

  if (!networkClient) {
    networkClient = new KissSSL();
  }
  KISS_LOG(LANG_INFO_SSL_TEST_1);
  bool result = networkClient->connectToTelegram();

  if (result) {
    KISS_LOG(LANG_INFO_SSL_TEST_2);
    networkClient->printInfo();
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
    return networkClient->isConnected();
  }

  lastCall = now;

  if (!networkClient) {
    networkClient = new KissSSL();
  }

  // Si el socket sigue vivo, no hacemos nada
  if (networkClient->isConnected()) {
    return true;
  }

  // AUTO-CONFIGURACIÓN INTELIGENTE basada en NTP
  bool shouldBeSecure = KissTime::getInstance().isTimeSynced();

  if (networkClient && networkClient->isSecureMode() != shouldBeSecure) {
    if (diagnosticsVerbose) {
      KISS_LOGF(LANG_INFO_SSL_INTELL,
                networkClient->isSecureMode() ? "SECURE" : "INSECURE",
                shouldBeSecure ? "SECURE" : "INSECURE");
    }
    networkClient->setSecureMode(shouldBeSecure);
  }

  bool ok = networkClient->connectToTelegram();
  if (ok) {
    KISS_LOG(LANG_INFO_SSL_SUCC_1);
    lastPingTime = millis();
    sslSecure = networkClient->isSecureMode();
    return true;
  }

  // Conexión falló
  KISS_LOG(LANG_ERROR_SSL_FAIL_1);

  // Intentar failover a LTE si estamos usando WiFi
  if (!usingLTE && isLTEAvailable()) {
    KISS_LOG("🔄 WiFi falló, intentando failover a LTE...");
    if (tryFallbackToLTE()) {
      // LTE conectado, intentar conectar a Telegram
      if (networkClient->connectToTelegram()) {
        KISS_LOG("✅ Conectado a Telegram vía LTE");
        lastPingTime = millis();
        sslSecure = networkClient->isSecureMode();
        return true;
      }
    }
  }

  return false;
}


bool KissTelegram::pingTelegram() {
  if (!networkClient || !networkClient->isConnected()) return false;

  KISS_LOG(LANG_INFO_PING_TG);

  // ✅ Enviar HTTP completo
  networkClient->print("GET /bot");
  networkClient->print(botToken);
  networkClient->print("/getMe HTTP/1.1\r\n");
  networkClient->print("Host: api.telegram.org\r\n");

  // ✅ OPTIMIZACIÓN LTE: usar Connection header apropiado
  if (shouldUseKeepAlive()) {
      networkClient->print("Connection: keep-alive\r\n");
  } else {
      networkClient->print("Connection: close\r\n");
  }

  networkClient->print("Content-Length: 0\r\n\r\n");

  // Leer y DESCARTAR toda la respuesta del servidor
  // para evitar contaminar el buffer en el siguiente checkMessages()
  unsigned long timeout = millis();
  bool foundOK = false;
  int bytesRead = 0;

  // Leer hasta que no haya más datos disponibles (con timeout de 100ms sin datos)
  while (networkClient->connected() && (millis() - timeout < 2000)) {
    if (networkClient->available()) {
      char c = networkClient->read();
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

  // Esperar hasta 500ms para que lleguen datos iniciales
  // Esto evita el race condition cuando checkMessages() se ejecuta
  // inmediatamente después de sendMessage()
  unsigned long waitStart = millis();
  while (!networkClient->available() && networkClient->connected() && (millis() - waitStart < 500)) {
    SAFE_YIELD();
  }

  // Si no llegaron datos y la conexión murió, salir
  if (!networkClient->connected() && !networkClient->available()) {
    return false;
  }

  unsigned long timeout = millis();
  int pos = 0;
  int contentLength = -1;
  int bodyStart = -1;
  bool foundContentLength = false;
  bool foundHttpCode = false;

  // Fase 1: Leer headers hasta encontrar Content-Length y "\r\n\r\n"
  while (networkClient->connected() && (millis() - timeout < 2000)) {
    if (networkClient->available()) {
      char c = networkClient->read();
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
  while (true) {
    int c = networkClient->read();
    if (c >= 0) return c;
    if (!networkClient->connected()) return -1;
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
  if (!networkClient) return;

  bool shouldBeSecure = KissTime::getInstance().isTimeSynced();

  if (networkClient->isSecureMode() != shouldBeSecure) {
    networkClient->setSecureMode(shouldBeSecure);
  }
}

// ========== OPTIMIZACIÓN LTE ==========

bool KissTelegram::isUsingLTE() {
  return usingLTE;
}

bool KissTelegram::shouldUseKeepAlive() {
  // WiFi: usar keep-alive para máxima eficiencia
  // LTE: NO usar keep-alive para permitir sleep mode
  if (usingLTE) {
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
  if (usingLTE) {
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
  if (!networkClient) return;

  if (usingLTE) {
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

    networkClient->setPowerMode(clientMode);

    // Log de consumo estimado
    if (diagnosticsVerbose) {
      int consumption = networkClient->getCurrentConsumption();
      KISS_LOGF("⚡ Consumo LTE estimado: %d mA", consumption);
    }
  } else {
    // WiFi: configuración estándar
    if (diagnosticsVerbose) {
      KISS_LOG("📶 WiFi: keep-alive estándar");
    }
  }
}

// ========== networkClient MANAGEMENT ==========

bool KissTelegram::initializeNetworkClient() {
  // PRIORIDAD 1: Usar WiFi si está conectado
  if (WiFi.status() == WL_CONNECTED) {
    KISS_LOG("📶 Inicializando con WiFi...");
    networkClient = new KissSSL();
    usingLTE = false;
    KISS_LOG("✅ WiFi inicializado");
    return true;
  }

  // PRIORIDAD 2: Si WiFi no está conectado, intentar LTE (si está disponible)
#if KISS_LTE_ENABLED
  if (isLTEAvailable()) {
    KISS_LOG("📡 WiFi no disponible, inicializando con LTE (A7672E)...");
    KissLTE& lte = KissLTE::getInstance();

    // El LTE ya debería estar inicializado en setup(), solo verificamos
    if (lte.powerOn() && lte.connect()) {
      networkClient = &lte;
      usingLTE = true;

      KISS_LOG("✅ LTE inicializado correctamente");
      return true;
    }
    KISS_LOG("❌ Error inicializando LTE");
  }
#endif

  // FALLBACK FINAL: WiFi en modo fallback (aunque no esté conectado)
  KISS_LOG("⚠️ Usando WiFi como fallback (desconectado)");
  networkClient = new KissSSL();
  usingLTE = false;
  return true;
}

bool KissTelegram::isLTEAvailable() {
#if KISS_LTE_ENABLED
  // Verificar que el APN esté configurado (no sea el placeholder)
  if (strlen(KISS_LTE_APN) == 0 || strcmp(KISS_LTE_APN, "YOUR_APN") == 0) {
    return false;
  }
  return true;
#else
  return false;
#endif
}

bool KissTelegram::tryFallbackToLTE() {
#if KISS_LTE_ENABLED
  if (usingLTE) {
    // Ya estamos usando LTE
    return false;
  }

  if (!isLTEAvailable()) {
    KISS_LOG("❌ LTE no disponible para failover");
    return false;
  }

  KISS_LOG("🔄 Intentando failover WiFi → LTE...");
  
  // Limpiar cliente WiFi anterior
  if (networkClient) {
    networkClient->disconnect();
    delete networkClient;
    networkClient = nullptr;
  }

  // Inicializar LTE
  KissLTE& lte = KissLTE::getInstance();
  if (!lte.begin()) {
    KISS_LOG("❌ Error inicializando LTE para failover");
    // Volver a crear cliente WiFi
    networkClient = new KissSSL();
    usingLTE = false;
    return false;
  }

  if (!lte.powerOn()) {
    KISS_LOG("❌ Error encendiendo módulo LTE");
    networkClient = new KissSSL();
    usingLTE = false;
    return false;
  }

  // Configurar credenciales
  if (strlen(KISS_LTE_APN) > 0) {
    lte.setAPN(KISS_LTE_APN, KISS_LTE_USER, KISS_LTE_PASS);
  }
  if (strlen(KISS_LTE_PIN) > 0) {
    lte.setPIN(KISS_LTE_PIN);
  }

  // Esperar registro en red (timeout 30s)
  KISS_LOG("⏳ Esperando registro LTE en red...");
  unsigned long startTime = millis();
  while (lte.getState() != KissLTE::STATE_CONNECTED && (millis() - startTime < KISS_WIFI_TIMEOUT_MS)) {
    delay(1000);
    KISS_LOGF("   Estado LTE: %s", lte.getStateString());
  }

  if (lte.getState() == KissLTE::STATE_CONNECTED) {
    networkClient = &lte;
    usingLTE = true;
    KISS_LOG("✅ Failover a LTE exitoso");
    
    // Log información del operador
    char operatorName[32];
    if (lte.getOperator(operatorName, sizeof(operatorName))) {
      KISS_LOGF("   Operador: %s", operatorName);
    }
    KISS_LOGF("   Señal: %d dBm", lte.getSignalStrength());
    
    return true;
  } else {
    KISS_LOG("❌ LTE no pudo registrarse en red");
    // Volver a WiFi
    networkClient = new KissSSL();
    usingLTE = false;
    return false;
  }
#else
  KISS_LOG("❌ LTE no compilado (KISS_LTE_ENABLED = false)");
  return false;
#endif
}
