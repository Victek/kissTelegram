// KissOTA.h
// Vicente Soriano - victek@gmail.com
// Plugin OTA para KissTelegram Suite - Versión PSRAM con Flash Directo

#ifndef KISS_OTA_H
#define KISS_OTA_H

#include "Kiss_setup.h"
#include "KissTelegram.h"
#include "KissCredentials.h"
#include <Preferences.h>
#include <esp_ota_ops.h>
#include <esp_partition.h>

class KissOTA {
public:
  enum OTAState {
    OTA_IDLE,               // Inactivo
    OTA_WAIT_PIN,           // Esperando PIN
    OTA_PIN_LOCKED,         // PIN bloqueado - necesita PUK
    OTA_WAIT_PUK,           // Esperando PUK
    OTA_AUTHENTICATED,      // Autenticado OK
    OTA_SPACE_CHECK,        // Verificando espacio
    OTA_BACKUP_CURRENT,     // Haciendo backup firmware actual
    OTA_BACKUP_PROGRESS,    // Progreso backup
    OTA_DOWNLOADING,        // Descargando nuevo .bin
    OTA_DOWNLOAD_PROGRESS,  // Progreso descarga
    OTA_VERIFY_CHECKSUM,    // Verificando checksum
    OTA_WAIT_CONFIRM,       // Esperando confirmaciÃ³n usuario
    OTA_FLASHING,           // Flasheando firmware
    OTA_FLASH_PROGRESS,     // Progreso flash
    OTA_VALIDATING,         // Validando nuevo firmware (60s timeout)
    OTA_ROLLBACK,           // Haciendo rollback
    OTA_REVERSE,            // Revirtiendo a firmware anterior (comando /reverse)
    OTA_WAIT_NEWFIRMOK,     // Esperando confirmación tras /reverse
    OTA_CLEANUP,            // Limpiando archivos temporales
    OTA_COMPLETE            // Completado OK
  };

  // Constructor
  KissOTA(KissTelegram* botInstance, KissCredentials* credsInstance);
  ~KissOTA();

  // Control OTA
  bool startOTA();
  void handleOTACommand(const char* command, const char* param);
  void cancelOTA();
  void loop();

  // Procesamiento de archivos recibidos
  bool processReceivedFile(const char* file_id, size_t file_size, const char* file_name);

  // Estado
  OTAState getState() {
    return currentState;
  }
  bool isActive() {
    return currentState != OTA_IDLE;
  }
  bool isLocked() {
    return pinLocked;
  }
  bool isInitialized() const {
    return psramBuffer != nullptr;
  }

private:
  // ========== INSTANCIAS ==========
  KissTelegram* bot;
  KissCredentials* credentials;

  // ========== ESTADO OTA ==========
  OTAState currentState;
  unsigned long stateStartTime;
  unsigned long otaStartTime;  // Para timeout global de 7 minutos

  // ========== AUTENTICACIÓN ==========
  int pinAttempts;
  bool pinLocked;
  unsigned long authStartTime;

  // ========== ARCHIVOS OTA ==========
  char downloadedFile[64];
  char backupFile[64];
  size_t downloadedSize;
  uint32_t downloadedChecksum;

  // ========== REINTENTOS ==========
  int downloadAttempts;
  int maxDownloadAttempts;
  bool downloadInterrupted;

  // ========== PROGRESO ==========
  int currentProgress;
  unsigned long lastProgressReport;
  unsigned long lastProgressUpdate;

  // ========== GESTIÓN PSRAM ==========
  uint8_t* psramBuffer;    // Buffer descarga nuevo firmware
  size_t psramBufferSize;  // TamaÃ±o total disponible (7-8MB)
  size_t downloadSize;     // Bytes descargados

  bool initPSRAMBuffer();
  void freePSRAMBuffer();

  // ========== ARCHIVOS OTA ==========
  static constexpr const char* BIN_ORIGINAL = "/bin_original.bin";  // Backup firmware actual
  static constexpr const char* BIN_NUEVO = "/bin_nuevo.bin";        // Nuevo firmware descargado

  // ========== TIMEOUTS ==========
  static const uint32_t BOOT_MAGIC = 0xCAFEBABE;
  static const int BOOT_VALIDATION_TIMEOUT = 60000;        // 60 segundos para /otaok
  static const unsigned long OTA_GLOBAL_TIMEOUT = 420000;  // 7 minutos para todo el proceso

  struct BootFlags {
    uint32_t magic;
    uint32_t bootCount;
    uint32_t lastBootTime;
    bool otaInProgress;
    bool firmwareValid;
    bool reverseInProgress;  // Bandera para /reverse - evita reprocesar comando
    char backupPath[64];
  };

  BootFlags bootFlags;

  // ========== MÉTODOS AUTENTICACIÓN ==========
  bool verifyPIN(const char* pin);
  bool verifyPUK(const char* puk);
  void lockPIN();
  void unlockPIN();
  void resetAttempts();
  bool isAuthTimeout();

  // ========== MÉTODOS ESPACIO ==========
  bool checkFSSpace();
  size_t getRequiredSpace();

  // ========== MÉTODOS BACKUP ==========
  bool backupCurrentFirmware();
  bool restoreFromBackup();

  // ========== MÉTODOS DESCARGA ==========
  bool downloadFirmware(const char* file_id);
  bool downloadFirmwareAttempt(const char* file_id);
  void cleanupPartialDownload();
  bool handleDownloadFailure();

  // ========== MÉTODOS VERIFICACIÓN ==========
  bool verifyChecksum();
  bool saveFirmwareToFS();  // Guarda firmware de PSRAM a FS como bin_nuevo.bin
  uint32_t calculateCRC32(File& file);

  // ========== MÉTODOS FLASH DIRECTO ==========
  bool flashNewFirmware();
  bool verifyFlash(const esp_partition_t* partition);
  bool validateNewFirmware();

  // ========== MÃ‰TODOS BOOT FLAGS ==========
  void saveBootFlags();
  bool loadBootFlags();
  void clearBootFlags();
  bool isFirstBootAfterOTA();
  void markFirmwareValid();
  bool checkBootLoop();

  // ========== MÉTODOS RECUPERACIÓN ==========
  void handleInterruptedOTA();
  void validateBootAfterOTA();
  void handleBootLoopRecovery();
  void emergencyRollback();
  bool reverseFirmware();  // Revierte a firmware anterior usando bin_original.bin

  // ========== MÉTODOS CLEANUP ==========
  void cleanupFiles();
  void cleanupOldBackups();

  // ========== HELPERS ==========
  bool copyCurrentToFactory();  // Copia firmware actual a factory (app0) para permitir futuros OTAs
  void sendOTAMessage(const char* text);
  void transitionState(OTAState newState);
  const char* getStateName(OTAState state);
};

#endif  // KISS_OTA_H