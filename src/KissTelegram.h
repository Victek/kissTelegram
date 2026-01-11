// KissTelegram.h
// Vicente Soriano - victek@gmail.com

#ifndef KISS_TELEGRAM_H
#define KISS_TELEGRAM_H

#include "Kiss_setup.h"
#include "KissTime.h"
#include "KissClient.h"
#include "KissNet.h"  // Gestor único de conectividad


#if defined(ESP_PLATFORM) || defined(ARDUINO_ARCH_ESP32)
#define KISS_USE_RTOS
#endif

struct TgAck {
  bool success;
  uint16_t httpCode;
  uint32_t retryAfter;
};

// ========== TELEGRAM RESPONSE HANDLING ==========
#define TG_TIMEOUT_MS 2000
#define TG_MAX_BODY JSON_BUFFER_SIZE

class KissTelegram {
public:
  enum MessagePriority {
    PRIORITY_LOW,
    PRIORITY_NORMAL,
    PRIORITY_HIGH,
    PRIORITY_CRITICAL
  };

  enum ConnectionQuality {
    QUALITY_DEAD,
    QUALITY_POOR,
    QUALITY_FAIR,
    QUALITY_GOOD,
    QUALITY_EXCELLENT
  };

  enum StorageMode {
    STORAGE_DISABLED,
    STORAGE_FULL
  };

  enum OperationMode {
    MODE_BALANCED,
    MODE_PERFORMANCE,
    MODE_POWERSAVE,
    MODE_RELIABILITY
  };

  enum PowerMode {
    POWER_BOOT,
    POWER_LOW,
    POWER_IDLE,
    POWER_ACTIVE,
    POWER_TURBO,
    POWER_MAINTENANCE
  };

  // ========== CALLBACKS ==========
  typedef void (*PowerModeCallback)(PowerMode oldMode, PowerMode newMode);
  typedef void (*SystemEventCallback)(const char* event, const char* data);
  typedef bool (*PowerTransitionCallback)(PowerMode from, PowerMode to);
  typedef void (*FileReceivedCallback)(const char* file_id, size_t file_size, const char* file_name);

  // ========== CONSTRUCTOR/DESTRUCTOR ==========
  KissTelegram(const char* token);
  ~KissTelegram();

  // ========== CONTROL BÁSICO ==========
  void enable();
  void disable();
  bool isEnabled();
  void setWifiStable();
  bool isWifiStable();
  bool isConnected();
  bool isConnectionAlive();
  void checkConnectionAge();

  // ========== CONFIGURACIÓN MENSAJES ==========
  void setMinMessageInterval(int milliseconds);
  int getMinMessageInterval();

  void setMaxRetryAttempts(int attempts);
  int getMaxRetryAttempts();

  // ========== CONFIGURACIÓN STORAGE ==========
  void enableStorage(bool enable = true);
  void setStorageMode(StorageMode mode);
  void setMaxQueueStorage(int maxMessages);
  int getMaxQueueStorage();

  void setAutoSaveInterval(unsigned long intervalMs);
  unsigned long getAutoSaveInterval();

  // ========== CONFIGURACIÓN AVANZADA ==========
  void setOperationMode(OperationMode mode);
  OperationMode getOperationMode();

  void setDiagnosticsVerbose(bool verbose);
  bool getDiagnosticsVerbose();

  // ========== POWER MANAGEMENT ==========
  void setPowerMode(PowerMode mode);
  PowerMode getCurrentPowerMode();
  PowerMode getTargetPowerMode();

  void setPowerConfig(int idleTimeoutSec, int decayTimeSec, int bootStableTimeSec);
  void getPowerConfig(int& idleTimeout, int& decayTime, int& bootStableTime);

  void setPowerSaving(bool enable);
  bool getPowerSaving();

  void setMaintenanceMode(bool enable, const char* reason = nullptr);
  bool isInMaintenanceMode();

  void onPowerModeChange(PowerModeCallback callback);
  void onSystemEvent(SystemEventCallback callback);
  void onPowerTransition(PowerTransitionCallback callback);
  void onFileReceived(FileReceivedCallback callback);

  void updatePowerState();
  bool shouldProcessQueue();
  bool shouldCheckMessages();
  int getRecommendedDelay();

  unsigned long getTimeInMode(PowerMode mode);
  float getPowerEfficiency();
  void resetPowerStatistics();

  // ========== OPERACIONES MENSAJES ==========
  bool sendMessage(const char* chat_id, const char* text,
                   MessagePriority priority = PRIORITY_NORMAL);
  bool sendMessageDirect(const char* chat_id, const char* text);  // Sin rate-limit
  bool queueMessage(const char* chat_id, const char* text,
                    MessagePriority priority = PRIORITY_NORMAL);
  void processQueue();

  // ========== MODO TURBO ==========
  void enableTurboMode();
  void disableTurboMode();
  bool isTurboMode();

  bool checkMessages(void (*handler)(const char* chat_id, const char* text,
                                     const char* command, const char* param));

  // ========== STORAGE OPERATIONS ==========
  bool saveNow();
  bool restoreFromStorage();
  void clearStorage();
  void autoSave();

  // ========== DIAGNÓSTICOS ==========
  void printDiagnostics();
  void printStorageStatus();
  void printSystemStatus();
  void printConfiguration();
  void printPowerStatistics();

  // ========== INFORMACIÓN SISTEMA ==========
  static int getFreeMemory();
  int getQueueSize();
  bool isSSLSecure();
  ConnectionQuality getConnectionQuality();

  int getStorageUsage();
  int getFreeStorage();
  bool isStorageAvailable();

  int getPendingMessages();
  int getFailedMessages();
  unsigned long getUptime();

  String getVersion();

  // ========== GESTIÓN DE RED ==========
  String getNetworkInfo();          // Info actual de red (WiFi/LTE + señal)
  String getNetworkMode();          // Modo actual (AUTO/WIFI/LTE)
  bool setNetworkMode(const char* mode);  // Cambiar modo: "auto", "wifi", "lte"
  bool switchToLTE();               // Forzar cambio a LTE
  bool switchToWiFi();              // Forzar cambio a WiFi
  bool switchToAuto();              // Volver a modo automático

  // ========== MANTENIMIENTO ==========
  void cleanupStorage();
  void resetCounters();

  // ========== MÉTODOS FILESYSTEM ==========
  size_t getFSTotalBytes();
  size_t getFSUsedBytes();
  size_t getFSFreeBytes();
  bool isFSMounted();

  void getStorageInfo(char* buffer, size_t bufferSize);
  void printStorageInfo();
  void getDiagnosticInfo(char* buffer, size_t bufferSize);
  int getMessagesInFS();

  // ========== MÉTODOS OTA ==========
  bool requestOTAPermission(char* errorMsg, int errorMsgSize);
  bool isOTAMode();
  void setOTAMode(bool enabled);
  bool sendOTAMessage(const char* chat_id, const char* text);

  // ========== MÉTODOS SSL MEJORADOS ==========
  bool testSSLConnection();
  bool hasTimePassed(unsigned long startTime, unsigned long interval);
  bool pingTelegram();
  unsigned long lastPingTime = 0;
  void updateSSLMode();

  // ========== OPTIMIZACIÓN LTE ==========
  bool isUsingLTE();
  bool shouldUseKeepAlive();
  bool shouldCloseAfterRequest();
  int getRecommendedPingInterval();
  void optimizeConnectionForNetwork();

private:
  // ========== VARIABLES CONEXIÓN ==========
  KissNet* kissNet;  // Gestor de conectividad WiFi/LTE
  char botToken[64];
  bool enabled;
  unsigned long wifiStableTime;
  int connectionAttempts;
  int failedPings;
  ConnectionQuality currentQuality;
  long lastUpdateID;

  // ========== VARIABLES MENSAJERÍA ==========
  unsigned long lastMessageTime;
  int minMessageInterval;
  int maxRetryAttempts;

  static const int JSON_BUFFER_SIZE = 6144;  // 8192 → 6KB (suficiente para updates)
  static const int MESSAGE_BUFFER_SIZE = 1024; // 2048 → 1KB (ahorra 1KB)
  static const int COMMAND_BUFFER_SIZE = 32;   // 64 → 32 (suficiente para comandos)
  static const int PARAM_BUFFER_SIZE = 64;     // 128 → 64 (ahorra 64 bytes)

  char* jsonBuffer;
  char* messageBuffer;
  char* commandBuffer;
  char* paramBuffer;

  // ========== VARIABLES POLLING ADAPTATIVO ==========
  int pollingTimeout;
  bool adaptivePolling;

  // ========== VARIABLES STORAGE ==========
  bool storageEnabled;
  StorageMode storageMode;
  int maxQueueStorage;
  unsigned long lastSaveTime;
  unsigned long autoSaveInterval;

  // ========== VARIABLES OPERACIÓN ==========
  OperationMode operationMode;
  int wifiStabilityThreshold;
  bool diagnosticsVerbose;
  unsigned long startTime;

  // ========== VARIABLES POWER MANAGEMENT ==========
  PowerMode currentPowerMode;
  PowerMode targetPowerMode;
  unsigned long powerModeChangeTime;
  unsigned long lastActivityTime;
  unsigned long maintenanceModeStart;
  char maintenanceReason[48];  // 128 → 48 bytes (ahorra 80 bytes)

  int idleTimeout;
  int decayTime;
  int bootStableTime;
  bool powerSavingEnabled;
  bool maintenanceMode;

  unsigned long timeInMode[6];
  unsigned long modeStartTime;
  unsigned long totalActiveTime;
  unsigned long totalMessagesSent;

  PowerModeCallback powerModeCallback;
  SystemEventCallback systemEventCallback;
  PowerTransitionCallback powerTransitionCallback;
  FileReceivedCallback fileReceivedCallback;

  // ========== VARIABLES OTA ==========
  bool otaMode;

  // ========== VARIABLES TURBO MODE ==========
  bool turboMode;
  int originalMinInterval;
  int turboProcessedTotal;      // Total procesados en esta sesión turbo
  unsigned long turboStartTime; // Cuando empezó turbo mode

  // ========== BATCH DELETE ==========
  static const int BATCH_DELETE_THRESHOLD = 10;  // Limpiar cada 10 mensajes (balance I/O vs seguridad)
  int pendingDeletes;
  uint32_t deleteQueue[15];  // 25 → 15 IDs (threshold=10 + margen=5)

  // ========== VARIABLES SSL ==========
  bool sslSecure;

  // ========== VARIABLES LITTLEFS ==========
  uint32_t nextMsgId;

  // ⚡ Caché para optimizar countPendingMessages()
  mutable int cachedPendingCount;
  mutable unsigned long lastCountCheck;

  // ========== MÉTODOS LITTLEFS ==========
  bool appendMessageToFS(const char* chat_id, const char* text, MessagePriority priority);
  bool deleteMessageFromFS(uint32_t messageID);
  void queueDeleteMessage(uint32_t messageID);  // Encolar para batch delete
  bool flushDeleteQueue();  // Ejecutar batch delete
  bool getNextPendingMessage(char* chat_id, char* text, MessagePriority* priority, uint32_t* msgId);
  int countPendingMessages();
  void removeDuplicateMessagesFromFS();  // Limpiar mensajes duplicados por ID

  bool saveToLittleFS();
  bool restoreFromLittleFS();

  // ========== HELPERS JSON MANUAL ==========
  int findJSONValue(const String& json, const char* key, int startPos);
  bool extractJSONString(const String& json, int startPos, char* buffer, size_t bufferSize);
  int extractJSONInt(const String& json, int startPos);
  int findNextMessage(const String& json, int startPos);

  // ========== MÉTODOS CONEXIÓN ==========
  bool isConnectionReallyStable();
  bool testBasicConnectivity();
  bool connectToTelegram();
  bool trySecureConnection();
  bool tryInsecureConnection();
  bool readResponse();
  bool readResponse(TgAck& ack);
  void cleanupConnection();
  int timedRead(unsigned long startTime);
  void resetBuffers();
  void extractCommand(const char* text, char* command, char* param);
  void logSystemEvent(const char* event, const char* data = nullptr);

  // ========== MÉTODOS UTILIDAD ==========
  unsigned long safeTimeDiff(unsigned long later, unsigned long earlier);

  // ========== MÉTODOS POWER MANAGEMENT ==========
  void transitionToPowerMode(PowerMode newMode);
  void applyOperationMode();
  void updatePowerStatistics();
  bool isTransitionComplete();
  bool canTransitionTo(PowerMode newMode);
  void notifyPowerModeChange(PowerMode oldMode, PowerMode newMode);

  // ========== MÉTODOS STORAGE ==========
  bool shouldSave();

  // ========== MÉTODOS COMANDOS INTERNOS ==========
  bool handleLTECommand();  // Procesar comando /lte

  // HELPER
  void printNumber(long value) {
    char buffer[20];
    snprintf(buffer, sizeof(buffer), "%ld", value);
    KissClient* client = kissNet->getActiveClient();
    if (client) client->print(buffer);
  }
};

#endif