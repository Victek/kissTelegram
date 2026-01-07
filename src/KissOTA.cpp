// KissOTA.cpp
// Vicente Soriano - victek@gmail.com
// Plugin OTA para KissTelegram Suite

#include "KissOTA.h"
#include "rom/crc.h"
#include <nvs_flash.h>
#include <WiFiClientSecure.h>
#include "lang.h"

// ========== CONSTRUCTOR ==========
KissOTA::KissOTA(KissTelegram* botInstance, KissCredentials* credsInstance) {
  bot = botInstance;
  credentials = credsInstance;
  currentState = OTA_IDLE;
  stateStartTime = 0;
  otaStartTime = 0;
  pinAttempts = 0;
  pinLocked = credentials->isOTALocked();
  authStartTime = 0;
  downloadedFile[0] = '\0';
  backupFile[0] = '\0';
  downloadedSize = 0;
  downloadedChecksum = 0;
  downloadAttempts = 0;
  maxDownloadAttempts = 3;
  downloadInterrupted = false;
  currentProgress = 0;
  lastProgressReport = 0;
  lastProgressUpdate = 0;

  // Inicializar PSRAM
  psramBuffer = nullptr;
  psramBufferSize = 0;
  downloadSize = 0;

  if (!initPSRAMBuffer()) {
    KISS_CRITICAL("❌ Error inicializando buffer PSRAM");
    currentState = OTA_IDLE;
    return;
  }

  KISS_LOGF("✅ PSRAM buffer: %.2f MB para descarga firmware\n",
            psramBufferSize / 1048576.0);
  // Verificar si venimos de OTA interrumpido o boot loop
  if (loadBootFlags()) {
    if (bootFlags.otaInProgress) {
      KISS_CRITICAL("⚠️ OTA INTERRUMPIDO detectado");
      handleInterruptedOTA();
    } else if (isFirstBootAfterOTA()) {
      KISS_CRITICAL("🔄 Primera arrancada tras OTA - validando...");
      validateBootAfterOTA();
    } else if (checkBootLoop()) {
      KISS_CRITICAL("🔥 BOOT LOOP detectado - iniciando ROLLBACK");
      handleBootLoopRecovery();
    }
  }

  // Limpiar backup si venimos de rollback
  if (bootFlags.firmwareValid && strlen(bootFlags.backupPath) > 0) {
    KISS_LOG("🧹 Detectado rollback previo - limpiando backup...");
    cleanupOldBackups();
    bootFlags.backupPath[0] = '\0';
    saveBootFlags();
  }

  KISS_LOG("✅ KissOTA inicializado");
  if (pinLocked) {
    KISS_CRITICAL("🔒 OTA bloqueado - requiere PUK");
  }
}

KissOTA::~KissOTA() {
  if (currentState != OTA_IDLE) {
    cancelOTA();
  }
  freePSRAMBuffer();
}

// ========== GESTIÓN PSRAM ==========
bool KissOTA::initPSRAMBuffer() {
#if defined(BOARD_HAS_PSRAM) || defined(CONFIG_SPIRAM_SUPPORT)
  size_t totalPSRAM = heap_caps_get_total_size(MALLOC_CAP_SPIRAM);
  size_t freePSRAM = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
  KISS_LOGF("💾 PSRAM total: %.2f MB", totalPSRAM / 1048576.0);
  KISS_LOGF("💾 PSRAM libre: %.2f MB", freePSRAM / 1048576.0);
  static const size_t PSRAM_MARGIN = 1048576;

  if (freePSRAM <= PSRAM_MARGIN) {
    KISS_CRITICALF("❌ PSRAM insuficiente: %.2f MB libre, necesita >1MB margen",
                   freePSRAM / 1048576.0);
    return false;
  }
  psramBufferSize = freePSRAM - PSRAM_MARGIN;
  psramBuffer = (uint8_t*)heap_caps_malloc(psramBufferSize, MALLOC_CAP_SPIRAM);
  return (psramBuffer != nullptr);
#else
  KISS_CRITICAL("❌ Hardware sin PSRAM - OTA no disponible");
  return false;
#endif
}

void KissOTA::freePSRAMBuffer() {
  if (psramBuffer) {
    heap_caps_free(psramBuffer);
    psramBuffer = nullptr;
    psramBufferSize = 0;
    downloadSize = 0;
    KISS_LOG("🗑️ Buffer PSRAM liberado");
  }
}

// ========== INICIO DE OTA ==========
bool KissOTA::startOTA() {
  if (currentState != OTA_IDLE) {
    sendOTAMessage(LANG_OTA_RUN_RUNN);
    return false;
  }
  KISS_CRITICAL("📦 Solicitud OTA recibida...");

  // Verificar permisos con KissTelegram
  char errorMsg[256];
  if (!bot->requestOTAPermission(errorMsg, sizeof(errorMsg))) {
    sendOTAMessage(errorMsg);
    return false;
  }

  // Activar otaMode ANTES de cualquier verificación (para enviar mensajes)
  bot->setOTAMode(true);
  bot->setMaintenanceMode(true, "Proceso OTA iniciado");
  SAFE_DELAY(100); // Pequeño delay para asegurar que el bot procesa el cambio de modo

  // Verificar si existe backup anterior (bin_original.bin)
  if (KISS_FS.begin(KISS_FS_FORMAT_ON_FAIL)) {
    if (KISS_FS.exists(BIN_ORIGINAL)) {
      KISS_FS.end();
      KISS_CRITICAL("⚠️ Existe backup anterior (bin_original.bin)");
      sendOTAMessage(LANG_OTA_BACKUP_EXISTS);
      // Desactivar modes antes de salir
      bot->setOTAMode(false);
      bot->setMaintenanceMode(false);
      return false;
    }
    KISS_FS.end();
  }

  cleanupOldBackups();

  // Determinar estado inicial según bloqueo PIN
  if (pinLocked) {
    transitionState(OTA_WAIT_PUK);
    KISS_LOG("📤 Enviando mensaje PIN bloqueado...");
    sendOTAMessage(LANG_OTA_PIN_BLOCK);
  } else {
    transitionState(OTA_WAIT_PIN);
    pinAttempts = 0;
    authStartTime = millis();

    KISS_LOG("📤 Enviando mensaje inicial OTA (esperando PIN)...");
    sendOTAMessage(LANG_OTA_INI_1);
    KISS_LOG("✅ Mensaje inicial enviado");
  }
  return true;
}

// ========== GESTIÓN DE COMANDOS ==========
void KissOTA::handleOTACommand(const char* command, const char* param) {
  // Verificar timeout autenticación (60 segundos para PIN)
  if (currentState == OTA_WAIT_PIN && isAuthTimeout()) {
    sendOTAMessage(LANG_OTA_INI_TIMEOUT);
    transitionState(OTA_IDLE);
    return;
  }
  // Verificar timeout global OTA (7 minutos)
  if (currentState != OTA_IDLE && currentState != OTA_WAIT_PIN && currentState != OTA_WAIT_PUK && otaStartTime > 0) {
    if (millis() - otaStartTime > OTA_GLOBAL_TIMEOUT) {
      sendOTAMessage(LANG_OTA_GLOB_TIMEOUT);

      cancelOTA();
      return;
    }
  }

  // /otapin - Verificar PIN
  if (strcmp(command, "/otapin") == 0) {
    if (currentState != OTA_WAIT_PIN) {
      return;
    }
    if (verifyPIN(param)) {
      // PIN correcto - AQUÍ activamos maintenance mode
      bot->setMaintenanceMode(true, "OTA en progreso");
      bot->setOTAMode(true);
      otaStartTime = millis();
      sendOTAMessage(LANG_OTA_PIN_OK);

      transitionState(OTA_AUTHENTICATED);
      resetAttempts();

#ifdef KISS_USE_RTOS
      esp_task_wdt_deinit();
      KISS_LOG("⚠️ WDT desactivado para OTA");
#endif

      // Verificar espacio
      if (!checkFSSpace()) {
        cancelOTA();
        return;
      }

      // Hacer backup del firmware actual
      transitionState(OTA_BACKUP_CURRENT);
      if (!backupCurrentFirmware()) {
        sendOTAMessage(LANG_OTA_ERROR_BACKUP);
        cancelOTA();
        return;
      }



      sendOTAMessage(LANG_OTA_INI_2);
      transitionState(OTA_DOWNLOADING);

    } else {
      pinAttempts++;
      if (pinAttempts >= KISS_OTA_PIN_ATTEMPTS) {
        lockPIN();
        sendOTAMessage(LANG_OTA_PIN_BLOCKED);
        transitionState(OTA_WAIT_PUK);

      } else {
        char msg[150];
        snprintf(msg, sizeof(msg),
                 LANG_OTA_PIN_FAIL,
                 KISS_OTA_PIN_ATTEMPTS - pinAttempts);
        sendOTAMessage(msg);
      }
    }

  }

  // /otapuk - Verificar PUK
  else if (strcmp(command, "/otapuk") == 0) {
    if (currentState != OTA_WAIT_PUK) {
      return;
    }

    if (verifyPUK(param)) {
      unlockPIN();
      sendOTAMessage(LANG_OTA_PIN_UBLOCK);
      transitionState(OTA_IDLE);
    } else {
      sendOTAMessage(LANG_OTA_PUK_WRONG);
    }
  }

  // /otaconfirmdelete - Borrar backup anterior y continuar OTA
  else if (strcmp(command, "/otaconfirmdelete") == 0) {
    // Borrar bin_original.bin
    if (KISS_FS.begin(KISS_FS_FORMAT_ON_FAIL)) {
      if (KISS_FS.exists(BIN_ORIGINAL)) {
        KISS_FS.remove(BIN_ORIGINAL);
        KISS_LOG("🗑️ Backup anterior (bin_original.bin) eliminado");
        bot->sendMessage(credentials->getChatId(), LANG_OTA_BACKUP_DELETED);
      } else {
        bot->sendMessage(credentials->getChatId(), LANG_OTA_NO_BACKUP_DELETE);
      }
      KISS_FS.end();
    } else {
      bot->sendMessage(credentials->getChatId(), LANG_OTA_FS_ACCESS_ERROR);
    }

    // Resetear a IDLE para permitir nuevo /ota
    if (currentState != OTA_IDLE) {
      bot->setOTAMode(false);
      bot->setMaintenanceMode(false);
      transitionState(OTA_IDLE);
    }
  }

  // /otacancel - Cancelar OTA
  else if (strcmp(command, "/otacancel") == 0) {
    if (currentState == OTA_IDLE) {
      bot->sendMessage(credentials->getChatId(), LANG_OTA_PR_INACTI);
      return;
    }

    // Si estamos validando tras flash → rollback
    if (currentState == OTA_VALIDATING) {
      sendOTAMessage(LANG_OTA_REVERT);
      transitionState(OTA_ROLLBACK);
      emergencyRollback();
    } else {
      // Cancelación normal antes del flash
      sendOTAMessage(LANG_OTA_CANCEL);
      cancelOTA();
    }
  }

  // /otaconfirm - Confirmar flash
  else if (strcmp(command, "/otaconfirm") == 0) {
    if (currentState != OTA_WAIT_CONFIRM) {
      return;
    }

    sendOTAMessage(LANG_OTA_FLASH);
    transitionState(OTA_FLASHING);

    if (!flashNewFirmware()) {
      sendOTAMessage(LANG_OTA_FLASH_F);
      transitionState(OTA_ROLLBACK);
      emergencyRollback();
    }

    // Si flashNewFirmware() tiene éxito, se reiniciará automáticamente
  }

  // /otaok - Validar firmware nuevo
  else if (strcmp(command, "/otaok") == 0) {
    KISS_CRITICAL("🔍 Comando /otaok recibido");
    KISS_LOGF("   Estado actual: %s", getStateName(currentState));
    KISS_LOGF("   Primera arrancada: %s", isFirstBootAfterOTA() ? "SI" : "NO");

    if (currentState == OTA_VALIDATING || isFirstBootAfterOTA()) {
      KISS_CRITICAL("✅ Condiciones de validación cumplidas - marcando firmware como válido");

      markFirmwareValid();
      transitionState(OTA_CLEANUP);

      // Borrar SOLO bin_nuevo.bin, MANTENER bin_original.bin como backup
      if (!KISS_FS.begin(KISS_FS_FORMAT_ON_FAIL)) {
        KISS_CRITICAL("❌ Error montando LittleFS para limpieza");
      } else {
        if (KISS_FS.exists(BIN_NUEVO)) {
          KISS_FS.remove(BIN_NUEVO);
          KISS_LOG("🗑️ Eliminado bin_nuevo.bin (ya en factory)");
        }

        // MANTENER bin_original.bin como backup para /reverse
        if (KISS_FS.exists(BIN_ORIGINAL)) {
          KISS_LOG("💾 Manteniendo bin_original.bin como backup");
        }
        KISS_FS.end();
      }

      transitionState(OTA_COMPLETE);

      // Enviar mensaje ANTES de desactivar otaMode
      sendOTAMessage(LANG_OTA_VALIDATED);
      KISS_CRITICAL("🎉 OTA COMPLETADO - Firmware validado (backup disponible: /reverse)");

      // CRÍTICO: Verificar si estamos corriendo desde app1
      const esp_partition_t* running = esp_ota_get_running_partition();
      const esp_partition_t* factory = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_FACTORY, NULL);

      if (running && factory && running != factory) {
        // Copiar firmware actual a factory y reiniciar desde factory
        if (copyCurrentToFactory()) {
          sendOTAMessage(LANG_OTA_PREPARING_FUTURE);
          SAFE_DELAY(2000);
          ESP.restart();
        }
      }

      // Desactivar maintenance mode
      bot->setMaintenanceMode(false);
      bot->setOTAMode(false);

#ifdef KISS_USE_RTOS
      KISS_INIT_WDT();
      KISS_LOG("✅ WDT reactivado");
#endif

      transitionState(OTA_IDLE);
      otaStartTime = 0;
    } else {
      KISS_CRITICAL("❌ /otaok recibido pero no hay validación pendiente");
      sendOTAMessage(LANG_OTA_NO_PENDING);
    }
  }

  // /reverse - Revertir a firmware anterior
  else if (strcmp(command, "/reverse") == 0) {
    KISS_CRITICAL("🔄 Comando /reverse recibido");

    // Solo permitir si estamos en IDLE (firmware validado funcionando)
    if (currentState != OTA_IDLE) {
      bot->sendMessage(credentials->getChatId(), LANG_OTA_REVERSE_IDLE_ONLY);
      return;
    }

    // Verificar que existe bin_original.bin
    if (!KISS_FS.begin(KISS_FS_FORMAT_ON_FAIL)) {
      bot->sendMessage(credentials->getChatId(), LANG_OTA_FS_ACCESS_ERROR);
      KISS_CRITICAL("❌ Error accediendo FS para /reverse");
      return;
    }

    if (!KISS_FS.exists(BIN_ORIGINAL)) {
      KISS_FS.end();
      KISS_CRITICAL("❌ No existe bin_original.bin para /reverse");
      bot->sendMessage(credentials->getChatId(), LANG_OTA_NO_BACKUP);
      return;
    }

    KISS_FS.end();

    // Activar modo OTA para la reversión
    bot->setOTAMode(true);
    bot->setMaintenanceMode(true, "Reversión de firmware");

    sendOTAMessage(LANG_OTA_REVERSING);
    transitionState(OTA_REVERSE);

    if (reverseFirmware()) {
      sendOTAMessage(LANG_OTA_REVERSE_COMPLETE);
      SAFE_DELAY(2000);
      ESP.restart();
    } else {
      sendOTAMessage(LANG_OTA_REVERSE_ERROR);
      bot->setOTAMode(false);
      bot->setMaintenanceMode(false);
      transitionState(OTA_IDLE);
    }
  }

  // /newfirmok - Confirmar firmware revertido
  else if (strcmp(command, "/newfirmok") == 0) {
    KISS_CRITICAL("🔍 Comando /newfirmok recibido");

    if (currentState == OTA_WAIT_NEWFIRMOK || isFirstBootAfterOTA()) {
      KISS_CRITICAL("✅ Firmware revertido confirmado - borrando backup");

      markFirmwareValid();

      // Borrar bin_original.bin definitivamente
      if (!KISS_FS.begin(KISS_FS_FORMAT_ON_FAIL)) {
        KISS_CRITICAL("❌ Error montando LittleFS");
      } else {
        if (KISS_FS.exists(BIN_ORIGINAL)) {
          KISS_FS.remove(BIN_ORIGINAL);
          KISS_LOG("🗑️ Eliminado bin_original.bin definitivamente");
        }
        KISS_FS.end();
      }

      transitionState(OTA_IDLE);
      sendOTAMessage(LANG_OTA_NEWFIRM_CONFIRMED);
      KISS_CRITICAL("🎉 Firmware revertido confirmado - Backup borrado");
    } else {
      KISS_CRITICAL("❌ /newfirmok recibido pero no hay reversión pendiente");
      sendOTAMessage(LANG_OTA_NO_REVERSE_PENDING);
    }
  }
}

// ========== PROCESAMIENTO ARCHIVO RECIBIDO ==========
bool KissOTA::processReceivedFile(const char* file_id, size_t file_size, const char* file_name) {
  if (currentState != OTA_DOWNLOADING) {
    KISS_LOG("⚠️ Archivo recibido fuera de estado OTA_DOWNLOADING");
    return false;
  }

  // Verificar extensión .bin
  if (!file_name || strlen(file_name) < 4) {
    sendOTAMessage(LANG_OTA_NONVALID);
    return false;
  }

  const char* ext = file_name + strlen(file_name) - 4;
  if (strcasecmp(ext, ".bin") != 0) {
    char msg[200];
    snprintf(msg, sizeof(msg),
             LANG_OTA_WRONG_EXT,
             file_name);
    sendOTAMessage(msg);
    return false;
  }

  // Verificar tamaño
  if (file_size > psramBufferSize) {
    char msg[250];
    snprintf(msg, sizeof(msg),
             LANG_OTA_TOO_BIG,
             file_size / 1048576.0,
             psramBufferSize / 1048576.0);
    sendOTAMessage(msg);
    return false;
  }

  if (file_size < 100000) {
    char msg[200];
    snprintf(msg, sizeof(msg),
             LANG_OTA_TOO_SMALL,
             file_size / 1024.0);
    sendOTAMessage(msg);
  }

  KISS_LOGF("📥 Procesando archivo: %s (%.2f MB)\n",
            file_name, file_size / 1048576.0);
  // Mensaje de descarga
  char downloadMsg[200];
  snprintf(downloadMsg, sizeof(downloadMsg),
           LANG_OTA_DOWNLOADING,
           file_name, file_size / 1048576.0);
  sendOTAMessage(downloadMsg);

  // Descargar a PSRAM
  if (!downloadFirmware(file_id)) {
    sendOTAMessage(LANG_OTA_DOWN_FAIL);
    cancelOTA();
    return false;
  }

  // Verificar checksum
  transitionState(OTA_VERIFY_CHECKSUM);
  sendOTAMessage(LANG_OTA_CHKSUM_VERI);
  if (!verifyChecksum()) {
    sendOTAMessage(LANG_OTA_CHKSUM_WRONG);
    cleanupPartialDownload();
    cancelOTA();
    return false;
  }

  // Todo OK - pedir confirmación
  transitionState(OTA_WAIT_CONFIRM);
  char confirmMsg[350];
  snprintf(confirmMsg, sizeof(confirmMsg),
           LANG_OTA_FILE_VERIFIED,
           file_name,
           downloadSize / 1048576.0,
           downloadedChecksum);
  sendOTAMessage(confirmMsg);
  return true;
}

// ========== LOOP MONITORING ==========
void KissOTA::loop() {
  // 1. Verificar timeout inactividad (60 segundos para PIN)
  if (currentState == OTA_WAIT_PIN && isAuthTimeout()) {
    sendOTAMessage(LANG_OTA_PIN_OUTTIME);
    cancelOTA();
    return;
  }
  // 2. Timeout validación (60 segundos) para confirmación upgrade
  if (currentState == OTA_VALIDATING) {
    unsigned long elapsed = millis() - stateStartTime;
    if (elapsed > BOOT_VALIDATION_TIMEOUT) {
      KISS_CRITICAL("⏱️ TIMEOUT validación - ROLLBACK");
      sendOTAMessage(LANG_OTA_PIN_OUT_VAL);
      transitionState(OTA_ROLLBACK);
      emergencyRollback();
    }
  }

  // Monitorizar timeout global OTA (7 minutos)
  if (currentState != OTA_IDLE && currentState != OTA_WAIT_PIN && currentState != OTA_WAIT_PUK && currentState != OTA_VALIDATING && otaStartTime > 0) {
    if (millis() - otaStartTime > OTA_GLOBAL_TIMEOUT) {
      KISS_CRITICAL("⏱️ TIMEOUT global OTA");
      sendOTAMessage(LANG_OTA_TIMEOUT);
      cancelOTA();
    }
  }
}

// ========== AUTENTICACIÓN ==========
bool KissOTA::verifyPIN(const char* pin) {
  return (strcmp(pin, credentials->getOTAPin()) == 0);
}

bool KissOTA::verifyPUK(const char* puk) {
  return (strcmp(puk, credentials->getOTAPuk()) == 0);
}

void KissOTA::lockPIN() {
  pinLocked = true;
  credentials->setOTALocked(true);
  KISS_CRITICAL("🔒 PIN bloqueado");
}

void KissOTA::unlockPIN() {
  pinLocked = false;
  credentials->setOTALocked(false);
  pinAttempts = 0;
  KISS_LOG("🔓 PIN desbloqueado");
}

void KissOTA::resetAttempts() {
  pinAttempts = 0;
}

bool KissOTA::isAuthTimeout() {
  return (millis() - authStartTime > KISS_OTA_AUTH_TIMEOUT);
}

// ========== VERIFICACIÓN ESPACIO ==========
bool KissOTA::checkFSSpace() {
  KISS_LOG("💾 Verificando espacio LittleFS...");
  if (!KISS_FS.begin(KISS_FS_FORMAT_ON_FAIL)) {
    KISS_CRITICAL("❌ Error montando LittleFS");
    sendOTAMessage(LANG_OTA_CHECK_FS);
    return false;
  }
  size_t totalBytes = KISS_FS.totalBytes();
  size_t usedBytes = KISS_FS.usedBytes();
  size_t freeBytes = totalBytes - usedBytes;

  KISS_FS.end();

  if (freeBytes == 0 || totalBytes == 0) {
    KISS_CRITICAL("❌ Error accediendo LittleFS");
    return false;
  }

  size_t required = getRequiredSpace();
  KISS_LOGF("💾 Libre: %.1fMB / %.1fMB\n",
            freeBytes / 1048576.0, totalBytes / 1048576.0);
  if (freeBytes < required) {
    char msg[250];
    snprintf(msg, sizeof(msg),
             LANG_OTA_NOT_ENOUGH,
             required / 1048576.0,
             freeBytes / 1048576.0);
    sendOTAMessage(msg);
    return false;
  }
  KISS_LOG("✅ Espacio suficiente");
  return true;
}

size_t KissOTA::getRequiredSpace() {
  return 6291456;  // 6MB
}

// ========== BACKUP FIRMWARE ==========
bool KissOTA::backupCurrentFirmware() {
  KISS_LOG("💾 Iniciando backup...");
  const esp_partition_t* running = esp_ota_get_running_partition();
  if (!running) {
    KISS_CRITICAL("❌ No se puede obtener partición actual");
    return false;
  }

  KISS_LOGF("📦 Partición actual: %s (0x%x bytes)\n",
            running->label, running->size);

  // Abrir archivo backup en LittleFS
  if (!KISS_FS.begin(KISS_FS_FORMAT_ON_FAIL)) {
    KISS_CRITICAL("❌ Error montando LittleFS");
    return false;
  }

  strcpy(backupFile, BIN_ORIGINAL);

  // Borrar backup anterior si existe
  if (KISS_FS.exists(backupFile)) {
    KISS_FS.remove(backupFile);
  }

  File backupFileHandle = KISS_FS.open(backupFile, FILE_WRITE);
  if (!backupFileHandle) {
    KISS_CRITICAL("❌ Error creando backup");
    KISS_FS.end();
    return false;
  }

  // Buffer en heap
  const size_t CHUNK_SIZE = 32768;
  uint8_t* buffer = (uint8_t*)malloc(CHUNK_SIZE);
  if (!buffer) {
    KISS_CRITICAL("❌ Error asignando buffer");
    backupFileHandle.close();
    KISS_FS.remove(backupFile);
    KISS_FS.end();
    return false;
  }

  size_t bytesWritten = 0;
  int chunkCount = 0;

  for (size_t offset = 0; offset < running->size; offset += CHUNK_SIZE) {
    size_t remaining = running->size - offset;
    size_t toRead = remaining < CHUNK_SIZE ? remaining : CHUNK_SIZE;

    if (esp_partition_read(running, offset, buffer, toRead) != ESP_OK) {
      KISS_CRITICAL("❌ Error leyendo firmware");
      free(buffer);
      backupFileHandle.close();
      KISS_FS.remove(backupFile);
      KISS_FS.end();
      return false;
    }

    size_t written = backupFileHandle.write(buffer, toRead);
    if (written != toRead) {
      KISS_CRITICAL("❌ Error escribiendo backup");
      free(buffer);
      backupFileHandle.close();
      KISS_FS.remove(backupFile);
      KISS_FS.end();
      return false;
    }

    bytesWritten += written;
    chunkCount++;

    if (chunkCount % 10 == 0) {
      KISS_CHECK_HEAP();
      SAFE_YIELD();
    }
  }

  free(buffer);
  backupFileHandle.close();
  KISS_FS.end();
  KISS_LOGF("✅ Backup completo: %d bytes\n", bytesWritten);

  // Guardar ruta en boot flags
  strncpy(bootFlags.backupPath, backupFile, sizeof(bootFlags.backupPath) - 1);
  saveBootFlags();
  return true;
}

bool KissOTA::restoreFromBackup() {
  KISS_CRITICAL("🔄 Revertir a un firmware anterior (dual-bank)...");

  // En dual-bank con factory, el firmware original está INTACTO en app0
  // Solo necesitamos cambiar el boot partition, NO copiar nada
  const esp_partition_t* factory = esp_partition_find_first(
    ESP_PARTITION_TYPE_APP,
    ESP_PARTITION_SUBTYPE_APP_FACTORY,
    NULL);

  if (!factory) {
    KISS_CRITICAL("❌ Falta la copia de seguridad de la configuración de fábrica");
    return false;
  }

  const esp_partition_t* running = esp_ota_get_running_partition();

  KISS_LOGF("🔙 Cambiando boot: %s → %s\n",
            running ? running->label : "unknown",
            factory->label);
  // Configurar boot a factory (firmware original)
  esp_err_t err = esp_ota_set_boot_partition(factory);

  if (err != ESP_OK) {
    KISS_CRITICALF("❌ Error configurando arranque: 0x%x\n", err);
    return false;
  }
  // Marcar flags para limpieza en próximo boot
  bootFlags.magic = BOOT_MAGIC;
  bootFlags.otaInProgress = false;
  bootFlags.firmwareValid = true;
  // Mantener backupPath para limpieza posterior
  saveBootFlags();
  KISS_CRITICAL("✅ Vuelta atrás configurada - Reiniciando...");
  SAFE_DELAY(1000);
  ESP.restart();

  return true;
}


// ========== DESCARGA FIRMWARE ==========
bool KissOTA::downloadFirmware(const char* file_id) {
  downloadAttempts = 0;
  while (downloadAttempts < maxDownloadAttempts) {
    downloadAttempts++;
    KISS_LOGF("📥 Intento descarga %d/%d\n",
              downloadAttempts, maxDownloadAttempts);
    // Intentar descarga
    if (downloadFirmwareAttempt(file_id)) {
      KISS_LOG("✅ Descarga completada");
      downloadAttempts = 0;
      return true;
    }

    // Fallo detectado
    KISS_CRITICALF("❌ Descarga falló (intento %d)\n", downloadAttempts);
    // Limpiar descarga parcial
    cleanupPartialDownload();
    if (downloadAttempts < maxDownloadAttempts) {
      SAFE_DELAY(5000);
    }
  }

  KISS_CRITICAL("❌ Descarga falló definitivamente");
  return false;
}

bool KissOTA::downloadFirmwareAttempt(const char* file_id) {
  KISS_LOGF("📥 Descargando file_id: %s\n", file_id);
  if (!psramBuffer || psramBufferSize == 0) {
    KISS_CRITICAL("❌ Buffer PSRAM no inicializado");
    return false;
  }

  // PASO 1: Obtener file_path desde Telegram API
  WiFiClientSecure client;
  client.setInsecure();
  if (!client.connect("api.telegram.org", 443)) {
    KISS_CRITICAL("❌ Conexión Telegram API falló");
    return false;
  }

  // Construir request getFile
  client.print("GET /bot");
  client.print(credentials->getBotToken());
  client.print("/getFile?file_id=");
  client.print(file_id);
  client.print(" HTTP/1.1\r\n");
  client.print("Host: api.telegram.org\r\n");
  client.print("Connection: close\r\n\r\n");

  // Leer respuesta
  char response[1024];
  int pos = 0;
  unsigned long timeout = millis();
  while (client.connected() && (millis() - timeout < 10000)) {
    if (client.available()) {
      char c = client.read();
      if (pos < sizeof(response) - 1) {
        response[pos++] = c;
      }
      timeout = millis();
    }
    SAFE_YIELD();
  }
  response[pos] = '\0';
  client.stop();

  // Extraer file_path de JSON
  char* pathPtr = strstr(response, "\"file_path\":\"");
  if (!pathPtr) {
    KISS_CRITICAL("❌ No se encontró la ruta del archivo");
    return false;
  }
  pathPtr += strlen("\"file_path\":\"");
  char* pathEnd = strchr(pathPtr, '\"');
  if (!pathEnd) {
    KISS_CRITICAL("❌ Error parseando la ruta del archivo");
    return false;
  }

  char file_path[256];
  int pathLen = pathEnd - pathPtr;
  strncpy(file_path, pathPtr, pathLen);
  file_path[pathLen] = '\0';
  KISS_LOGF("📄 Ruta del archivo: %s\n", file_path);

  // PASO 2: Descargar archivo a PSRAM
  if (!client.connect("api.telegram.org", 443)) {
    KISS_CRITICAL("❌ Conexión de descarga falló");
    return false;
  }

  client.print("GET /file/bot");
  client.print(credentials->getBotToken());
  client.print("/");
  client.print(file_path);
  client.print(" HTTP/1.1\r\n");
  client.print("Host: api.telegram.org\r\n");
  client.print("Connection: close\r\n\r\n");

  // Saltar headers HTTP
  timeout = millis();
  bool headersEnd = false;
  while (client.connected() && !headersEnd && (millis() - timeout < 10000)) {
    if (client.available()) {
      char lineBuffer[128];
      int len = 0;
      
      // Leer hasta \n o fin de buffer
      while(client.available() && len < sizeof(lineBuffer) - 1) {
        char c = client.read();
        if (c == '\n') break;
        lineBuffer[len++] = c;
      }
      lineBuffer[len] = '\0';
      
      // Trim \r al final si existe
      if (len > 0 && lineBuffer[len-1] == '\r') {
        lineBuffer[--len] = '\0';
      }
      
      // Header vacío o solo \r = fin de headers
      if (len == 0) {
        headersEnd = true;
      }
      timeout = millis();
    }
    SAFE_YIELD();
  }

  if (!headersEnd) {
    KISS_CRITICAL("❌ Timeout esperando cabeceras");
    client.stop();
    return false;
  }

  // Descargar contenido a PSRAM
  downloadSize = 0;
  timeout = millis();
  KISS_LOG("📥 Descargando firmware a PSRAM...");
  while (client.connected() || client.available()) {
    if (client.available()) {
      size_t available = client.available();
      size_t toRead = min(available, psramBufferSize - downloadSize);
      if (toRead == 0) {
        KISS_CRITICAL("❌ Buffer PSRAM lleno");
        client.stop();
        return false;
      }
      size_t bytesRead = client.readBytes(psramBuffer + downloadSize, toRead);
      downloadSize += bytesRead;
      timeout = millis();
    }
    if (millis() - timeout > 30000) {
      KISS_CRITICAL("❌ Fuera de tiempo en la descarga (30s sin datos)");
      client.stop();
      return false;
    }
    SAFE_YIELD();
  }
  client.stop();
  KISS_LOGF("✅ Descarga completa: %.2f MB en PSRAM\n", downloadSize / 1048576.0);

  // Guardar info
  strcpy(downloadedFile, "[PSRAM]");
  downloadedSize = downloadSize;
  return true;
}

void KissOTA::cleanupPartialDownload() {
  KISS_LOG("🗑️ Limpiando descarga parcial...");
  downloadSize = 0;
  downloadedSize = 0;
  downloadedChecksum = 0;
  downloadedFile[0] = '\0';
  KISS_LOG("✅ Descarga parcial limpiada");
}

bool KissOTA::handleDownloadFailure() {
  cleanupPartialDownload();
  if (downloadAttempts >= maxDownloadAttempts) {
    cancelOTA();
    return false;
  }
  return true;
}

// ========== VERIFICACIÓN CHECKSUM ==========
bool KissOTA::verifyChecksum() {
  KISS_LOG("🔍 Verificando checksum...");
  if (!psramBuffer || downloadSize == 0) {
    KISS_CRITICAL("❌ No hay datos descargados");
    return false;
  }
  // CRC32 hardware ESP32
  downloadedChecksum = crc32_le(0, psramBuffer, downloadSize);
  KISS_LOGF("🔍 CRC32: 0x%08X (%.2f MB verificados)\n",
            downloadedChecksum, downloadSize / 1048576.0);

  // Guardar firmware verificado en FS como bin_nuevo.bin
  KISS_LOG("💾 Guardando firmware en FS...");
  if (!saveFirmwareToFS()) {
    KISS_CRITICAL("❌ Error guardando firmware en FS");
    return false;
  }

  return true;
}

// Guardar firmware de PSRAM a FS como bin_nuevo.bin
bool KissOTA::saveFirmwareToFS() {
  if (!KISS_FS.begin(KISS_FS_FORMAT_ON_FAIL)) {
    KISS_CRITICAL("❌ Error montando LittleFS");
    return false;
  }

  // Borrar bin_nuevo.bin anterior si existe
  if (KISS_FS.exists(BIN_NUEVO)) {
    KISS_FS.remove(BIN_NUEVO);
  }

  File file = KISS_FS.open(BIN_NUEVO, FILE_WRITE);
  if (!file) {
    KISS_CRITICAL("❌ Error creando bin_nuevo.bin");
    KISS_FS.end();
    return false;
  }

  // Escribir PSRAM → FS
  const size_t CHUNK_SIZE = 32768;
  size_t bytesWritten = 0;

  for (size_t offset = 0; offset < downloadSize; offset += CHUNK_SIZE) {
    size_t remaining = downloadSize - offset;
    size_t toWrite = remaining < CHUNK_SIZE ? remaining : CHUNK_SIZE;

    size_t written = file.write(psramBuffer + offset, toWrite);
    if (written != toWrite) {
      KISS_CRITICAL("❌ Error escribiendo en FS");
      file.close();
      KISS_FS.remove(BIN_NUEVO);
      KISS_FS.end();
      return false;
    }

    bytesWritten += written;
    SAFE_YIELD();
  }

  file.close();
  KISS_FS.end();

  KISS_LOGF("✅ Firmware guardado en FS: %.2f MB\n", bytesWritten / 1048576.0);
  return true;
}
uint32_t KissOTA::calculateCRC32(File& file) {
  uint32_t crc = 0xFFFFFFFF;
  uint8_t buffer[256];
  file.seek(0);
  while (file.available()) {
    size_t bytesRead = file.read(buffer, sizeof(buffer));
    for (size_t i = 0; i < bytesRead; i++) {
      uint8_t byte = buffer[i];
      crc = crc ^ byte;
      for (int j = 0; j < 8; j++) {
        if (crc & 1) {
          crc = (crc >> 1) ^ 0xEDB88320;
        } else {
          crc = crc >> 1;
        }
      }
    }
    SAFE_YIELD();
  }
  return ~crc;
}

// ========== FLASH FIRMWARE DIRECTO ==========
bool KissOTA::flashNewFirmware() {
  KISS_CRITICAL("🔥 Grabando firmware directamente en factory - NO APAGAR");

  // Abrir bin_nuevo.bin desde FS
  if (!KISS_FS.begin(KISS_FS_FORMAT_ON_FAIL)) {
    KISS_CRITICAL("❌ Error montando LittleFS");
    return false;
  }

  if (!KISS_FS.exists(BIN_NUEVO)) {
    KISS_CRITICAL("❌ No se encuentra bin_nuevo.bin");
    KISS_FS.end();
    return false;
  }

  File firmwareFile = KISS_FS.open(BIN_NUEVO, FILE_READ);
  if (!firmwareFile) {
    KISS_CRITICAL("❌ Error abriendo bin_nuevo.bin");
    KISS_FS.end();
    return false;
  }

  size_t firmwareSize = firmwareFile.size();
  KISS_LOGF("📦 Flasheando %.2f MB desde FS...\n", firmwareSize / 1048576.0);

  // OBTENER PARTICIÓN ACTUAL Y DESTINO OTA
  const esp_partition_t* running = esp_ota_get_running_partition();
  const esp_partition_t* target = esp_ota_get_next_update_partition(NULL);

  if (!target) {
    KISS_CRITICAL("❌ Partición OTA destino no encontrada");
    firmwareFile.close();
    KISS_FS.end();
    return false;
  }

  // CRÍTICO: Verificar que NO escribimos en partición activa
  if (target == running) {
    KISS_CRITICAL("❌ ERROR: Destino es la partición actual");
    firmwareFile.close();
    KISS_FS.end();
    return false;
  }

  // VERIFICAR TAMAÑO
  if (firmwareSize > target->size) {
    KISS_CRITICALF("❌ Firmware demasiado grande: %d > %d\n", firmwareSize, target->size);
    firmwareFile.close();
    KISS_FS.end();
    return false;
  }

  // 1. DESACTIVAR WDT SI ESTÁ ACTIVO
#ifdef KISS_USE_RTOS
  esp_task_wdt_deinit();
  KISS_LOG("⚠️ WDT desactivado para flash");
#endif

  // 2. BORRAR EN BLOQUES PEQUEÑOS CON YIELD
  // Redondear hacia arriba al múltiplo de 4KB (tamaño sector flash)
  size_t eraseSize = ((firmwareSize + 4095) / 4096) * 4096;
  KISS_LOGF("🧹 Borrando %d bytes en %s...\n", eraseSize, target->label);

  // Borrar en bloques de 64KB con yield entre cada bloque
  const size_t ERASE_BLOCK = 65536; // 64KB
  size_t eraseOffset = 0;
  esp_err_t err = ESP_OK;

  while (eraseOffset < eraseSize && err == ESP_OK) {
    size_t toErase = (eraseSize - eraseOffset) < ERASE_BLOCK ? (eraseSize - eraseOffset) : ERASE_BLOCK;
    err = esp_partition_erase_range(target, eraseOffset, toErase);
    eraseOffset += toErase;
    SAFE_YIELD(); // Yield después de cada bloque
  }

  if (err != ESP_OK) {
    KISS_CRITICALF("❌ Error borrando partición: 0x%x\n", err);
    firmwareFile.close();
    KISS_FS.end();
    return false;
  }

  KISS_LOG("✅ Espacio borrado");

  // 3. ESCRIBIR NUEVO FIRMWARE DESDE FS
  const size_t FLASH_WRITE_SIZE = 4096;
  uint8_t* buffer = (uint8_t*)malloc(FLASH_WRITE_SIZE);
  if (!buffer) {
    KISS_CRITICAL("❌ Error asignando buffer");
    firmwareFile.close();
    KISS_FS.end();
    return false;
  }

  size_t bytesWritten = 0;
  int writeCount = 0;

  KISS_LOG("📝 Escribiendo firmware...");

  while (firmwareFile.available()) {
    size_t toRead = FLASH_WRITE_SIZE;
    size_t bytesRead = firmwareFile.read(buffer, toRead);

    if (bytesRead > 0) {
      err = esp_partition_write(target, bytesWritten, buffer, bytesRead);
      if (err != ESP_OK) {
        KISS_CRITICALF("❌ Error escribiendo en offset %d: 0x%x\n", bytesWritten, err);
        free(buffer);
        firmwareFile.close();
        KISS_FS.end();
        return false;
      }
      bytesWritten += bytesRead;
      writeCount++;

      // YIELD cada 2 escrituras (8KB) para evitar WDT
      if (writeCount % 2 == 0) {
        SAFE_YIELD();
      }
    }
  }

  free(buffer);
  firmwareFile.close();
  KISS_FS.end();

  KISS_LOGF("✅ Escritura completada: %d bytes\n", bytesWritten);

  // 3. VERIFICACIÓN OMITIDA (para evitar WDT)
  KISS_LOG("⏭️ Verificación omitida (optimización)");

  // 4. ENVIAR MENSAJE ANTES DE REINICIAR
  sendOTAMessage(LANG_OTA_BEF_REBOOT);

  KISS_LOGF("✅ Grabación en %s completada\n", target->label);

  // 5. CONFIGURAR BOOT A LA NUEVA PARTICIÓN
  KISS_LOGF("🔧 Configurando boot a %s...\n", target->label);
  err = esp_ota_set_boot_partition(target);
  if (err != ESP_OK) {
    KISS_CRITICALF("❌ Error configurando boot: 0x%x\n", err);
    return false;
  }

  // 6. ACTUALIZAR BOOT FLAGS
  bootFlags.magic = BOOT_MAGIC;
  bootFlags.otaInProgress = false;
  bootFlags.firmwareValid = false;
  bootFlags.bootCount = 1;
  saveBootFlags();

  // 7. DELAY para que Telegram envíe los mensajes
  KISS_LOG("💤 Esperando 5s para envío mensajes...");
  SAFE_DELAY(5000);

  // 8. REINICIAR
  KISS_LOGF("🔄 Reiniciando a %s...\n", target->label);
  SAFE_DELAY(2000);
  ESP.restart();
  return true;
}

bool KissOTA::verifyFlash(const esp_partition_t* partition) {

  KISS_LOG("🔍 Iniciando verificación flash vs FS...");

  // Abrir bin_nuevo.bin desde FS para comparar
  if (!KISS_FS.begin(KISS_FS_FORMAT_ON_FAIL)) {
    KISS_CRITICAL("❌ Error montando FS para verificación");
    return false;
  }

  File firmwareFile = KISS_FS.open(BIN_NUEVO, FILE_READ);
  if (!firmwareFile) {
    KISS_FS.end();
    KISS_CRITICAL("❌ No se puede abrir bin_nuevo.bin para verificación");
    return false;
  }

  size_t firmwareSize = firmwareFile.size();

  const size_t VERIFY_CHUNK = 4096;
  uint8_t* flashBuffer = (uint8_t*)malloc(VERIFY_CHUNK);
  uint8_t* fileBuffer = (uint8_t*)malloc(VERIFY_CHUNK);

  if (!flashBuffer || !fileBuffer) {
    KISS_CRITICAL("❌ No hay memoria para la verificación");
    if (flashBuffer) free(flashBuffer);
    if (fileBuffer) free(fileBuffer);
    firmwareFile.close();
    KISS_FS.end();
    return false;
  }

  bool verificationOK = true;
  size_t bytesVerified = 0;

  while (bytesVerified < firmwareSize && verificationOK) {
    size_t remaining = firmwareSize - bytesVerified;
    size_t toVerify = (remaining < VERIFY_CHUNK) ? remaining : VERIFY_CHUNK;

    // Leer de flash
    esp_err_t err = esp_partition_read(partition, bytesVerified, flashBuffer, toVerify);
    if (err != ESP_OK) {
      KISS_CRITICALF("❌ Error leyendo flash: 0x%x\n", err);
      verificationOK = false;
      break;
    }

    // Leer de archivo FS
    size_t bytesRead = firmwareFile.read(fileBuffer, toVerify);
    if (bytesRead != toVerify) {
      KISS_CRITICAL("❌ Error leyendo bin_nuevo.bin");
      verificationOK = false;
      break;
    }

    // Comparar flash vs archivo
    if (memcmp(flashBuffer, fileBuffer, toVerify) != 0) {
      KISS_CRITICALF("❌ Diferencia en offset %d\n", bytesVerified);
      verificationOK = false;
      break;
    }

    bytesVerified += toVerify;
    SAFE_YIELD();
  }

  free(flashBuffer);
  free(fileBuffer);
  firmwareFile.close();
  KISS_FS.end();

  if (verificationOK) {
    KISS_LOG("✅ Verificación completada exitosamente");
  } else {
    KISS_CRITICAL("❌ Verificación falló");
  }
  return verificationOK;
}

bool KissOTA::validateNewFirmware() {
  transitionState(OTA_VALIDATING);
  return true;
}

// ========== BOOT FLAGS ==========
void KissOTA::saveBootFlags() {
  Preferences prefs;
  KISS_LOG("💾 Guardando boot flags...");
  esp_err_t err = nvs_flash_init();
  if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    KISS_LOG("⚠️ NVS requiere reinicialización");
    nvs_flash_erase();
    err = nvs_flash_init();
  }

  if (err != ESP_OK) {
    KISS_CRITICALF("❌ Error inicializando NVS: 0x%x\n", err);
  }

  // Intentar abrir el namespace
  if (!prefs.begin("kiss_ota", false)) {
    KISS_CRITICAL("❌ Error abriendo NVS - reintentando...");
    // Reintentar con delay
    prefs.end();
    delay(100);

    if (!prefs.begin("kiss_ota", false)) {
      KISS_CRITICAL("❌ Error crítico guardando banderas de arranque - NVS no disponible");
      return;
    }

    KISS_LOG("✅ NVS abierto en segundo intento");
  }

  prefs.putUInt("magic", bootFlags.magic);
  prefs.putUInt("bootCount", bootFlags.bootCount);
  prefs.putUInt("lastBoot", bootFlags.lastBootTime);
  prefs.putBool("otaProgress", bootFlags.otaInProgress);
  prefs.putBool("fwValid", bootFlags.firmwareValid);
  prefs.putString("backup", bootFlags.backupPath);
  prefs.end();

  KISS_LOGF("✅ Banderas de arranque guardadas (magic=0x%X, otaInProgress=%d, fwValid=%d)\n",
            bootFlags.magic, bootFlags.otaInProgress, bootFlags.firmwareValid);
}

bool KissOTA::loadBootFlags() {
  Preferences prefs;

  if (!prefs.begin("kiss_ota", true)) {
    return false;
  }
  bootFlags.magic = prefs.getUInt("magic", 0);
  if (bootFlags.magic != BOOT_MAGIC) {
    prefs.end();
    // Primera vez - inicializar
    bootFlags.magic = BOOT_MAGIC;
    bootFlags.bootCount = 0;
    bootFlags.lastBootTime = 0;
    bootFlags.otaInProgress = false;
    bootFlags.firmwareValid = true;
    bootFlags.backupPath[0] = '\0';
    saveBootFlags();
    return false;
  }

  bootFlags.bootCount = prefs.getUInt("bootCount", 0);
  bootFlags.lastBootTime = prefs.getUInt("lastBoot", 0);
  bootFlags.otaInProgress = prefs.getBool("otaProgress", false);
  bootFlags.firmwareValid = prefs.getBool("fwValid", true);

  size_t len = prefs.getString("backup", bootFlags.backupPath, 
                               sizeof(bootFlags.backupPath));
  if (len == 0) {
    bootFlags.backupPath[0] = '\0';  // Vacío si no existe
  }
  prefs.end();
  saveBootFlags();

  return true;
}

void KissOTA::clearBootFlags() {
  bootFlags.otaInProgress = false;
  bootFlags.firmwareValid = true;
  bootFlags.bootCount = 0;
  bootFlags.backupPath[0] = '\0';
  saveBootFlags();
  KISS_LOG("🗑️ Banderas de arranque limpiadas");
}

bool KissOTA::isFirstBootAfterOTA() {
  return (!bootFlags.firmwareValid && strlen(bootFlags.backupPath) > 0);
}

void KissOTA::markFirmwareValid() {
  bootFlags.firmwareValid = true;
  bootFlags.bootCount = 0;
  saveBootFlags();
  KISS_LOGF("✅ Firmware v%s marcado como válido", KISS_GET_VERSION());
}

bool KissOTA::checkBootLoop() {
  if (bootFlags.bootCount > 3) {
    unsigned long timeSinceLastBoot = millis() - bootFlags.lastBootTime;
    if (timeSinceLastBoot < 300000) {  // 5 minutos
      KISS_CRITICAL("🔥 BOOT LOOP: 3+ arranques en 5 minutos");
      return true;
    }
  }
  return false;
}

// ========== RECUPERACIÓN ==========

void KissOTA::handleInterruptedOTA() {
  KISS_CRITICAL("🔧 Limpiando OTA interrumpido...");
  // Limpiar descarga parcial
  cleanupPartialDownload();
  // 🔥 CAMBIO 2: Limpiar backup si existe
  if (KISS_FS.begin(KISS_FS_FORMAT_ON_FAIL)) {
    if (strlen(bootFlags.backupPath) > 0 && KISS_FS.exists(bootFlags.backupPath)) {
      KISS_FS.remove(bootFlags.backupPath);
      KISS_LOGF("🗑️ Backup eliminado: %s\n", bootFlags.backupPath);
    }
    KISS_FS.end();
  }

  // Limpiar flags
  bootFlags.otaInProgress = false;
  saveBootFlags();

  // Informar usuario
  bot->queueMessage(credentials->getChatId(),
                    LANG_OTA_QUEUE_MESS,
                    KissTelegram::PRIORITY_HIGH);

  KISS_LOG("✅ OTA interrumpido limpiado");
}

void KissOTA::validateBootAfterOTA() {
  KISS_CRITICAL("🔄 Validando firmware nuevo...");
  // CAMBIO 4: Si venimos de rollback, limpiar backup
  if (bootFlags.firmwareValid && strlen(bootFlags.backupPath) > 0) {
    KISS_LOG("🧹 Detectada vuelta atrás previa - limpiando copia de seguridad...");
    if (KISS_FS.begin(KISS_FS_FORMAT_ON_FAIL)) {
      if (KISS_FS.exists(bootFlags.backupPath)) {
        KISS_FS.remove(bootFlags.backupPath);
        KISS_LOGF("🗑️ Copia de seguridad de vuelta atrás eliminada: %s\n", bootFlags.backupPath);
      }
      KISS_FS.end();
    }
    bootFlags.backupPath[0] = '\0';
    saveBootFlags();
  }


  // Activar maintenance mode durante validación
  bot->setMaintenanceMode(true, "Validación OTA");
  bot->setOTAMode(true);
  transitionState(OTA_VALIDATING);
  stateStartTime = millis();

  // Mensaje al usuario
  sendOTAMessage(LANG_OTA_MAINT_MODE);
  // Marcar en flags que estamos esperando validación

  bootFlags.firmwareValid = false;
  bootFlags.bootCount = 1;
  saveBootFlags();
}

void KissOTA::handleBootLoopRecovery() {
  KISS_CRITICAL("⚠️ Recuperación de lazo de arranque...");

  // Verificar si existe bin_original.bin
  if (!KISS_FS.begin(KISS_FS_FORMAT_ON_FAIL)) {
    KISS_CRITICAL("❌ Error montando FS - no se puede recuperar");
    bot->queueMessage(credentials->getChatId(), LANG_OTA_CRITI_ERROR,
                      KissTelegram::PRIORITY_CRITICAL);
    return;
  }

  if (KISS_FS.exists(BIN_ORIGINAL)) {
    KISS_FS.end();
    KISS_CRITICAL("✅ Backup encontrado - intentando restaurar...");
    if (reverseFirmware()) {
      KISS_CRITICAL("✅ Restauración iniciada - reiniciando");
      SAFE_DELAY(2000);
      ESP.restart();
    }
  } else {
    KISS_FS.end();
    KISS_CRITICAL("❌ No hay backup disponible");
  }

  // Si llegamos aquí, el rollback falló
  bot->queueMessage(credentials->getChatId(), LANG_OTA_CRITI_ERROR,
                    KissTelegram::PRIORITY_CRITICAL);
}

void KissOTA::emergencyRollback() {
  KISS_CRITICAL("🔥 INICIANDO RECUPERACIÓN DE EMERGENCIA");

  // Verificar si existe bin_original.bin
  if (!KISS_FS.begin(KISS_FS_FORMAT_ON_FAIL)) {
    KISS_CRITICAL("❌ Error montando FS - recuperación imposible");
    sendOTAMessage(LANG_OTA_ERROR_RECOVER);
    bot->setMaintenanceMode(false);
    bot->setOTAMode(false);
    transitionState(OTA_IDLE);
    otaStartTime = 0;
    return;
  }

  if (KISS_FS.exists(BIN_ORIGINAL)) {
    KISS_FS.end();
    KISS_CRITICAL("✅ Backup encontrado - restaurando...");
    sendOTAMessage(LANG_OTA_RESTORING_PREVIOUS);

    if (reverseFirmware()) {
      KISS_CRITICAL("✅ Firmware restaurado - reiniciando");
      SAFE_DELAY(2000);
      ESP.restart();
    } else {
      KISS_CRITICAL("❌ Recuperación falló - SISTEMA INESTABLE");
      sendOTAMessage(LANG_OTA_ERROR_RECOVER);
      bot->setMaintenanceMode(false);
      bot->setOTAMode(false);
      transitionState(OTA_IDLE);
      otaStartTime = 0;
    }
  } else {
    KISS_FS.end();
    KISS_CRITICAL("❌ No hay backup (bin_original.bin) disponible");
    sendOTAMessage(LANG_OTA_RECOVER_ERROR);
    bot->setMaintenanceMode(false);
    bot->setOTAMode(false);
    transitionState(OTA_IDLE);
    otaStartTime = 0;
  }
}

// Revertir a firmware anterior (bin_original.bin)
bool KissOTA::reverseFirmware() {
  KISS_CRITICAL("🔄 Revirtiendo a firmware anterior...");

  // Abrir bin_original.bin
  if (!KISS_FS.begin(KISS_FS_FORMAT_ON_FAIL)) {
    KISS_CRITICAL("❌ Error montando LittleFS");
    return false;
  }

  File originalFile = KISS_FS.open(BIN_ORIGINAL, FILE_READ);
  if (!originalFile) {
    KISS_CRITICAL("❌ Error abriendo bin_original.bin");
    KISS_FS.end();
    return false;
  }

  size_t firmwareSize = originalFile.size();
  KISS_LOGF("📦 Cargando firmware original: %.2f MB\n", firmwareSize / 1048576.0);

  // Obtener partición factory
  const esp_partition_t* factory = esp_partition_find_first(
    ESP_PARTITION_TYPE_APP,
    ESP_PARTITION_SUBTYPE_APP_FACTORY,
    NULL);

  if (!factory) {
    KISS_CRITICAL("❌ No se encuentra partición factory");
    originalFile.close();
    KISS_FS.end();
    return false;
  }

  // Borrar factory
  KISS_LOG("🧹 Borrando partición factory...");
  esp_err_t err = esp_partition_erase_range(factory, 0, factory->size);
  if (err != ESP_OK) {
    KISS_CRITICALF("❌ Error borrando partición: 0x%x", err);
    originalFile.close();
    KISS_FS.end();
    return false;
  }

  // Escribir bin_original.bin → factory
  const size_t CHUNK_SIZE = 4096;
  uint8_t* buffer = (uint8_t*)malloc(CHUNK_SIZE);
  if (!buffer) {
    KISS_CRITICAL("❌ Error asignando buffer");
    originalFile.close();
    KISS_FS.end();
    return false;
  }

  size_t bytesWritten = 0;
  int writeCount = 0;

  KISS_LOG("📝 Restaurando firmware original...");

  while (originalFile.available()) {
    size_t bytesRead = originalFile.read(buffer, CHUNK_SIZE);

    if (bytesRead > 0) {
      err = esp_partition_write(factory, bytesWritten, buffer, bytesRead);
      if (err != ESP_OK) {
        KISS_CRITICALF("❌ Error escribiendo offset %d: 0x%x", bytesWritten, err);
        free(buffer);
        originalFile.close();
        KISS_FS.end();
        return false;
      }
      bytesWritten += bytesRead;
      writeCount++;

      if (writeCount % 10 == 0) {
        SAFE_YIELD();
      }
    }
  }

  free(buffer);
  originalFile.close();
  KISS_FS.end();

  KISS_LOGF("✅ Firmware original restaurado: %d bytes\n", bytesWritten);

  // Borrar otadata para forzar boot a factory
  KISS_LOG("🔧 Borrando otadata...");
  const esp_partition_t* otadata = esp_partition_find_first(
    ESP_PARTITION_TYPE_DATA,
    ESP_PARTITION_SUBTYPE_DATA_OTA,
    NULL);

  if (otadata) {
    err = esp_partition_erase_range(otadata, 0, otadata->size);
    if (err != ESP_OK) {
      KISS_CRITICALF("⚠️ Advertencia borrando otadata: 0x%x", err);
    } else {
      KISS_LOG("✅ Otadata borrada");
    }
  }

  // Marcar flags para validación
  bootFlags.firmwareValid = false;
  bootFlags.bootCount = 1;
  saveBootFlags();

  KISS_CRITICAL("✅ Reversión completada");
  return true;
}

// ========== CLEANUP ==========
void KissOTA::cleanupFiles() {
  KISS_LOG("🗑️ Limpiando archivos OTA...");
  if (!KISS_FS.begin(KISS_FS_FORMAT_ON_FAIL)) {
    KISS_CRITICAL("❌ Error montando sistema de archivos");
    return;
  }
  int filesDeleted = 0;

  // Eliminar backup
  if (strlen(bootFlags.backupPath) > 0 && KISS_FS.exists(bootFlags.backupPath)) {
    KISS_FS.remove(bootFlags.backupPath);
    KISS_LOGF("🗑️ Eliminado: %s\n", bootFlags.backupPath);
    filesDeleted++;
  }
  KISS_FS.end();

  // Limpiar boot flags
  clearBootFlags();
  KISS_LOGF("✅ %d archivos eliminados\n", filesDeleted);
}

void KissOTA::cleanupOldBackups() {
  KISS_LOG("🧹 Limpiando copias de seguridad antiguas...");
  if (!KISS_FS.begin(KISS_FS_FORMAT_ON_FAIL)) {
    KISS_CRITICAL("❌ Error montando sistema de archivos para limpieza");
    return;
  }
  int filesDeleted = 0;

  // Eliminar /ota_backup.bin si existe
  if (KISS_FS.exists("/ota_backup.bin")) {
    KISS_FS.remove("/ota_backup.bin");
    KISS_LOG("🗑️ Copia de seguridad antigua /ota_backup.bin eliminada");
    filesDeleted++;
  }

  KISS_FS.end();

  if (filesDeleted > 0) {
    KISS_LOGF("✅ %d backup(s) antiguo(s) eliminado(s)\n", filesDeleted);
  } else {
    KISS_LOG("ℹ️  No hay backups antiguos que limpiar");
  }
}

// ========== HELPERS ==========
// Copiar firmware actual a partición factory (app0)
bool KissOTA::copyCurrentToFactory() {
  const esp_partition_t* running = esp_ota_get_running_partition();
  const esp_partition_t* factory = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_FACTORY, NULL);

  if (!running || !factory) {
    return false;
  }

  if (running == factory) {
    return true;
  }

  // Buffer para copiar en bloques
  const size_t BUFFER_SIZE = 4096;
  uint8_t* buffer = (uint8_t*)malloc(BUFFER_SIZE);
  if (!buffer) {
    return false;
  }

  // Borrar factory primero
  esp_err_t err = esp_partition_erase_range(factory, 0, factory->size);
  if (err != ESP_OK) {
    free(buffer);
    return false;
  }

  // Copiar en bloques
  size_t totalCopied = 0;
  size_t sizeToCopy = running->size;

  while (totalCopied < sizeToCopy) {
    size_t chunkSize = (sizeToCopy - totalCopied) > BUFFER_SIZE ? BUFFER_SIZE : (sizeToCopy - totalCopied);

    err = esp_partition_read(running, totalCopied, buffer, chunkSize);
    if (err != ESP_OK) {
      free(buffer);
      return false;
    }

    err = esp_partition_write(factory, totalCopied, buffer, chunkSize);
    if (err != ESP_OK) {
      free(buffer);
      return false;
    }

    totalCopied += chunkSize;
    SAFE_YIELD();
  }

  free(buffer);

  // Cambiar boot partition a factory
  err = esp_ota_set_boot_partition(factory);
  if (err != ESP_OK) {
    return false;
  }

  return true;
}

void KissOTA::sendOTAMessage(const char* text) {
  bool sent = bot->sendOTAMessage(credentials->getChatId(), text);
  if (!sent) {
    KISS_CRITICAL("⚠️ Falló envío mensaje OTA - reintentando...");
    SAFE_DELAY(500);
    sent = bot->sendOTAMessage(credentials->getChatId(), text);
    if (sent) {
      KISS_LOG("✅ Reintento exitoso");
    } else {
      KISS_CRITICAL("❌ Falló reintento - mensaje no enviado");
    }
  }
}

void KissOTA::transitionState(OTAState newState) {
  OTAState oldState = currentState;
  currentState = newState;
  stateStartTime = millis();

  KISS_LOGF("🔄 Estado OTA: %s → %s\n",
            getStateName(oldState), getStateName(newState));
}

const char* KissOTA::getStateName(OTAState state) {
  const char* names[] = {
    "IDLE", "WAIT_PIN", "PIN_LOCKED", "WAIT_PUK", "AUTHENTICATED",
    "SPACE_CHECK", "BACKUP_CURRENT", "BACKUP_PROGRESS",
    "DOWNLOADING", "DOWNLOAD_PROGRESS", "VERIFY_CHECKSUM",
    "WAIT_CONFIRM", "FLASHING", "FLASH_PROGRESS",
    "VALIDATING", "ROLLBACK", "REVERSE", "WAIT_NEWFIRMOK",
    "CLEANUP", "COMPLETE"
  };

  if (state >= 0 && state < (sizeof(names) / sizeof(names[0]))) {
    return names[state];
  }

  return "UNKNOWN";
}

void KissOTA::cancelOTA() {
  KISS_LOG("❌ Cancelando OTA...");
  if (currentState == OTA_DOWNLOADING || currentState == OTA_DOWNLOAD_PROGRESS || currentState == OTA_WAIT_CONFIRM) {
    cleanupPartialDownload();
  }

  // Desactivar maintenance mode
  bot->setMaintenanceMode(false);
  bot->setOTAMode(false);
#ifdef KISS_USE_RTOS
  KISS_INIT_WDT();
  KISS_LOG("✅ WDT reactivado");
#endif

  transitionState(OTA_IDLE);
  otaStartTime = 0;
  KISS_LOG("✅ OTA cancelado");
}