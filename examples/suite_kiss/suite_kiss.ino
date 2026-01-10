//suite_kiss.ino
// Vicente Soriano - victek@gmail.com
// Some demos for KissTelegram
// Build: 05/01/2026

// ==========================================================================
// LANGUAGE SELECTION FOR TELEGRAM messages, see lang.h
// To change language, edit lang.h file before compile
// When all commented fix Spanish as language default
// ==========================================================================
// SELECCIÓN IDIOMA PARA MENSAJES ENVIADOS A TELEGRAM, ver lang.h
// Para cambiar idioma descomenta en lang.h antes de compilar
// Idioma español por defecto (si todos están comentados)
// ==========================================================================

#include "lang.h"
#include "KissTelegram.h"
#include "KissConfig.h"
#include "KissCredentials.h"
#include "KissTime.h"

#ifdef KISS_HAS_OTA
#include "KissOTA.h"
#endif

#ifdef KISS_HAS_LTE
#include "KissLTE.h"
#endif

// ========== INSTANCIAS GLOBALES ==========
KissTelegram* bot = nullptr;
KissCredentials credentials;
KissConfig& config = KissConfig::getInstance();

#ifdef KISS_HAS_OTA
KissOTA* ota = nullptr;
#endif

#ifdef KISS_HAS_LTE
KissLTE& lte = KissLTE::getInstance();
#endif

// ========== ESTADO CONEXIÓN ==========
enum ConnectionMode {
  CONN_NONE,
  CONN_WIFI,
  CONN_LTE
};

ConnectionMode currentConnection = CONN_NONE;

// ========== ESTADÍSTICAS ==========
struct SystemStats {
  unsigned long startTime;
  unsigned long lastMessageTime;
  unsigned long lastWifiDrop;
  int messageCount;
  int errorCount;
  int wifiDropouts;
  int queueFullCount;
  int fallbackEvents;
  int lteActivations;
};

SystemStats stats;
bool wifiAlreadyStable = false;

void setup() {
  Serial.begin(115200);
  delay(2000);
  Serial.setDebugOutput(true);
  KISS_PRINT_BUILD_INFO();

  // DEBUG: Ver particiones disponibles
  const esp_partition_t* running = esp_ota_get_running_partition();
  const esp_partition_t* update = esp_ota_get_next_update_partition(NULL);

  KISS_LOGF("🔍 Booteando desde: %s (0x%x)", running->label, running->address);
  if (update) {
    KISS_LOGF("🎯 Partición OTA disponible: %s (0x%x)", update->label, update->address);
  } else {
    KISS_CRITICAL("❌ No hay partición OTA disponible");
  }

  Serial.println("\n\n");
  Serial.println("🚀 INICIANDO KISSTELEGRAM SUITE...");
  Serial.println("====================================");

  // INICIALIZAR NVS
  Serial.println("🔧 Inicializando NVS...");
  if (!KISS_FS.begin(KISS_FS_FORMAT_ON_FAIL)) {
    Serial.println("❌ ERROR CRÍTICO - FS no disponible");
    Serial.println("⚠️ Sistema detenido/muerto");
    while (1) {
      delay(1000);
      Serial.print(".");
    }
  }

  KISS_INIT_WDT();
  KISS_PRINT_SYSTEM_INFO();

  // Cargar credenciales
  credentials.begin();

#ifdef KISS_HAS_LTE
  // Inicializar LTE (en standby)
  Serial.println("📡 Preparando módulo LTE...");
  #ifdef KISS_LTE_DTR_PIN
    // Inicializar con DTR para sleep mode confiable
    lte.begin(&Serial1, KISS_LTE_RX_PIN, KISS_LTE_TX_PIN,
              KISS_LTE_PWRKEY_PIN, KISS_LTE_DTR_PIN);
    Serial.printf("✅ DTR habilitado en GPIO%d para sleep mode\n", KISS_LTE_DTR_PIN);
  #else
    // Inicializar sin DTR (sleep mode deshabilitado)
    lte.begin(&Serial1, KISS_LTE_RX_PIN, KISS_LTE_TX_PIN, KISS_LTE_PWRKEY_PIN);
    Serial.println("⚠️ Sin DTR - sleep mode deshabilitado");
  #endif

  lte.setAPN(KISS_LTE_APN, KISS_LTE_USER, KISS_LTE_PASS);
  Serial.println("✅ LTE preparado (standby)\n");
#endif

  // Intentar WiFi primero
  Serial.printf("📡 Conectando WiFi: %s\n", credentials.getWifiSSID());
  WiFi.begin(credentials.getWifiSSID(), credentials.getWifiPassword());

  Serial.print("⏳");
  unsigned long wifiStart = millis();
  bool wifiConnected = false;

  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - wifiStart > KISS_WIFI_TIMEOUT_MS) {
      Serial.println("\n⚠️ Timeout WiFi");
      wifiConnected = false;
      break;
    }
    SAFE_DELAY(500);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    unsigned long wifiTime = millis() - wifiStart;
    Serial.printf("\n✅ WiFi conectado en %lu ms - IP: %s\n",
                  wifiTime, WiFi.localIP().toString().c_str());
    currentConnection = CONN_WIFI;
  }

#ifdef KISS_HAS_LTE
  // Fallback a LTE si WiFi falló
  if (!wifiConnected) {
    Serial.println("📡 Activando fallback LTE...");

    if (lte.powerOn()) {
      Serial.println("✅ LTE encendido");
      currentConnection = CONN_LTE;
      stats.lteActivations++;

      // IMPORTANTE: KissNet maneja la conexión automáticamente
      Serial.println("🔧 LTE listo - KissNet gestionará la conexión");
    } else {
      Serial.println("❌ Error: sin conexión disponible");
      currentConnection = CONN_NONE;
      return;
    }
  }
#else
  if (!wifiConnected) {
    Serial.println("❌ Sin conexión WiFi - sistema detenido");
    return;
  }
#endif

  // INICIALIZAR KISSTIME PRIMERO (necesario para SSL)
  Serial.println("⏰ Sincronizando reloj NTP...");
  bool ntpSynced = KissTime::getInstance().begin();
  if (ntpSynced) {
    Serial.println("✅ Reloj sincronizado");
    KissTime::getInstance().printStatus();
  } else {
    Serial.println("⚠️ Reloj no sincronizado - modo INSECURE");
  }

  // CREAR BOT DESPUÉS de establecer conexión (WiFi o LTE)
  // Esto permite que initializeNetworkClient() detecte correctamente la red activa
  Serial.println("🤖 Inicializando KissTelegram...");
  bot = new KissTelegram(credentials.getBotToken());

  // RESTAURAR MENSAJES PENDIENTES DE LITTLEFS
  Serial.println("🔥 Restaurando mensajes pendientes...");
  if (bot->restoreFromStorage()) {
    int restored = bot->getMessagesInFS();
    Serial.printf("✅ %d mensajes restaurados desde LittleFS\n", restored);

    // Activar turbo mode automáticamente si hay muchos mensajes pendientes
    if (restored > 10) {
      Serial.println("🚀 Activando TURBO MODE automático (>10 mensajes pendientes)");
      bot->enableTurboMode();
    }
  } else {
    Serial.println("ℹ️ Sin mensajes pendientes - sistema fresco");
  }

#ifdef KISS_HAS_OTA
  // Inicializar KissOTA DESPUÉS de KissTime para que SSL esté configurado
  ota = new KissOTA(bot, &credentials);
  // Conectar callback de archivos
  bot->onFileReceived([](const char* file_id, size_t file_size, const char* file_name) {
    if (ota && ota->isActive()) {
      ota->processReceivedFile(file_id, file_size, file_name);
    }
  });
  KISS_LOG("✅ KissOTA disponible");
#endif

  bot->enableStorage(true);
  bot->setStorageMode(KissTelegram::STORAGE_FULL);
  bot->setMinMessageInterval(1000);

  // Estabilizar y activar
  Serial.println("⏳ Estabilizando (2 seg)...");
  SAFE_DELAY(2000);

  // Activar bot
  bot->setWifiStable();
  bot->enable();
  Serial.println("🔒 SSL configurado según estado NTP\n");

  // Inicializar stats
  memset(&stats, 0, sizeof(stats));
  stats.startTime = millis();

  Serial.println("✅ Sistema operativo listo\n");
}

void testPrioridades() {
  KISS_LOG("\n📊 INICIANDO TEST DE PRIORIDADES");
  KISS_LOG("==========================================");

  int initialQueueSize = bot->getQueueSize();
  int initialFSSize = bot->getMessagesInFS();
  KissTelegram::PowerMode initialPowerMode = bot->getCurrentPowerMode();

  // Mensajes LOW (no deberá activar power management) PRIORITY_LOW
  KISS_LOG("\n😴 TEST 1: Mensajes LOW (modo normal)");
  for (int i = 1; i <= 5; i++) {
    char msg[50];
    snprintf(msg, sizeof(msg), "🧪 LOW Test %d - Power: %d", i, bot->getCurrentPowerMode());
    bot->queueMessage(credentials.getChatId(), msg, KissTelegram::PRIORITY_LOW);
    delay(500);
  }

  // Mensajes CRITICOS (deberán activar TURBO inmediatamente) PRIORITY_CRITICAL
  KISS_LOG("\n🚨 TEST 2: Mensajes CRITICAL (activar TURBO)");
  for (int i = 1; i <= 4; i++) {
    char msg[60];
    snprintf(msg, sizeof(msg), "🚨 CRITICAL Test %d - Power: %d", i, bot->getCurrentPowerMode());
    bot->queueMessage(credentials.getChatId(), msg, KissTelegram::PRIORITY_CRITICAL);
    delay(300);
  }

  // Mezcla de prioridades
  KISS_LOG("\n🔄 TEST 3: Mezcla de prioridades");
  bot->queueMessage(credentials.getChatId(), "📊 NORMAL mezclado", KissTelegram::PRIORITY_NORMAL);
  delay(200);
  bot->queueMessage(credentials.getChatId(), "🚨 OTRO CRITICO", KissTelegram::PRIORITY_CRITICAL);
  delay(200);
  bot->queueMessage(credentials.getChatId(), "😴 FINAL BAJO", KissTelegram::PRIORITY_LOW);

  // ESTADÍSTICAS FINALES POR SERIAL
  KISS_LOG("\n🔄 PRIORITY TEST:");
  KISS_LOGF(" - Initial Messages in FS: %d", initialFSSize);
  KISS_LOGF(" - Filled Messages in FS: %d", bot->getMessagesInFS());
  KISS_LOGF(" - Initial Power Mode: %d", initialPowerMode);
  KISS_LOGF(" - Ending Power Mode: %d", bot->getCurrentPowerMode());
  KISS_LOGF(" - Queued Messages: %d", bot->getMessagesInFS() - initialFSSize);

  // RESUMEN POR TELEGRAM
  char summary[400];
  snprintf(summary, sizeof(summary),
           "🧪 INICIANDO TEST PRIORIDADES\n\n"
           "📊 Estadísticas:\n"
           " - Desde FS: %d\n"
           " - Modo energía, de: %d a %d\n"
           " - CRITICOS: 5 a enviar\n"
           " - NORMALES: 1 a enviar\n"
           " - BAJA PRIORIDAD: 6 a enviar\n\n"
           " - Ordenando Mensajes, mira serial para Log",
           bot->getMessagesInFS(), initialPowerMode, bot->getCurrentPowerMode());
  bot->sendMessage(credentials.getChatId(), summary);

  KISS_LOG("📊 TEST DE PRIORIDADES FINALIZADO");
  KISS_LOG("==========================================\n");
}

void testFallbackScenario(int numeroMensajes = 15) {
  Serial.println("\n🎯 INICIANDO TEST DE FALLBACK...");
  Serial.printf("📨 Encolando %d mensajes...\n", numeroMensajes);

  int messagesSent = 0;
  for (int i = 0; i < numeroMensajes; i++) {
    char testMsg[100];
    snprintf(testMsg, sizeof(testMsg), "🧪 Mensaje Test. %d/%d", i + 1, numeroMensajes);

    if (bot->queueMessage(credentials.getChatId(), testMsg, KissTelegram::PRIORITY_NORMAL)) {
      messagesSent++;
      if (messagesSent % 50 == 0) {
        Serial.printf("✅ Encolados: %d/%d\n", messagesSent, numeroMensajes);
      }
    } else {
      Serial.printf("❌ Error en mensaje %d\n", i + 1);
      break;
    }
    SAFE_YIELD();  // Solo yield, sin delay
  }

  Serial.printf("📊 Resultado: %d/%d mensajes encolados\n", messagesSent, numeroMensajes);
  Serial.printf("💾 LittleFS: %d mensajes pendientes\n", bot->getMessagesInFS());

  // Activar modo turbo para procesar rápido
  Serial.println("🚀 Activando TURBO MODE para envío rápido...");
  bot->enableTurboMode();

  bot->printStorageStatus();
}

void handleMessage(const char* chat_id, const char* text,
                   const char* command, const char* param) {
  Serial.printf("📨 Comando recibido: %s %s\n", command, param);

  // ✅ Comandos OTA críticos bypassean debounce anti-spam
  // Esto permite que /otaok funcione inmediatamente tras boot del nuevo firmware
  bool isOTACritical = (strcmp(command, "/otaok") == 0 ||
                        strcmp(command, "/otacancel") == 0);

  static unsigned long lastHandle = 0;
  if (!isOTACritical && millis() - lastHandle < 2000) {
    Serial.println("⏰ Demasiado rápido - ignorado");
    bot->sendMessage(chat_id, "⚠️ Comando muy rápido\nVuelve a enviar");
    return;
  }
  lastHandle = millis();

  if (strcmp(command, "/help") == 0 || strcmp(command, "/start") == 0) {
    char helpMsg[1200];
    snprintf(helpMsg, sizeof(helpMsg),
             "📦 KissTelegram Suite v%s\n\n"
             "📋 Comandos básicos:\n"
             "/config - Config a monitor serie\n"
             "/estado - Estado sistema + LittleFS\n"
             "/stats - Estadísticas acumuladas\n"
             "/lte - Gestión de red WiFi/LTE\n"
             "/debug - Info detallada\n"
             "/debugfs - Debug del FS\n"
             "/memoria - Info memoria\n"
             "/storage - Estado LittleFS\n"
             "/llenar [N] - Encolar N mensajes\n"
             "/turbo - Toggle modo turbo\n"
             "/forceprocess - Forzar envío\n"
             "/borrar confirmar - Eliminar todo\n"
             "/prioridades - Test sorting\n"
             "/testlittlefs - Test persistencia\n\n"
             "🔒 SSL:\n"
             "/testssl - Probar conexion SSL\n\n"
             "⏰ Tiempo:\n"
             "/time - Hora actual\n"
             "/timestatus - Estado reloj\n"
             "/resync - Forzar sync NTP\n\n"
             "🔧 Sistema:\n"
             "/parar - Desactivar auto-msgs\n"
             "/activar - Reactivar auto-msgs\n"
             "/changepin <viejo> <nuevo>\n"
             "/changepuk <viejo> <nuevo>\n"
             "/credentials - Ver credenciales\n"
             "/resetcredentials CONFIRMAR\n\n"
#ifdef KISS_HAS_OTA
             "🔧 OTA:\n"
             "/ota - Iniciar OTA\n"
             "/otapin <code>\n"
             "/otapuk <code>\n"
             "/otaconfirm\n"
             "/otaok\n"
             "/otacancel\n"
             "/otaconfirmdelete\n"
             "/reverse\n\n"
#endif
             "/help - Esta ayuda",
             KISS_GET_VERSION());
    bot->sendMessage(chat_id, helpMsg);
  } else if (strcmp(command, "/estado") == 0) {
    unsigned long totalQueued = config.getTotalMessagesQueued();
    unsigned long totalSent = config.getTotalMessagesSent();
    int pendingFS = bot->getMessagesInFS();
    int lostMessages = stats.errorCount - stats.fallbackEvents;
    bool systemReliable = (stats.queueFullCount == 0 && lostMessages == 0);
    const char* reliabilityEmoji = systemReliable ? "✅" : "⚠️";
    const char* reliabilityText = systemReliable ? _(STATUS_RELIABLE) : _(STATUS_CHECK);

    // Obtener modo de conexión y RSSI
    const char* connectionMode;
    int32_t rssi;
    const char* signalQuality;

    if (currentConnection == CONN_WIFI) {
      connectionMode = "WiFi";
      rssi = WiFi.RSSI();

      if (rssi >= -50) {
        signalQuality = _(WIFI_EXCELLENT);
      } else if (rssi >= -60) {
        signalQuality = _(WIFI_GOOD);
      } else if (rssi >= -70) {
        signalQuality = _(WIFI_FAIR);
      } else {
        signalQuality = _(WIFI_WEAK);
      }
    }
#ifdef KISS_HAS_LTE
    else if (currentConnection == CONN_LTE) {
      connectionMode = "LTE";
      rssi = lte.getSignalStrength();

      if (rssi >= -75) {
        signalQuality = "Excelente";
      } else if (rssi >= -85) {
        signalQuality = "Buena";
      } else if (rssi >= -95) {
        signalQuality = "Regular";
      } else {
        signalQuality = "Débil";
      }
    }
#endif
    else {
      connectionMode = "Ninguna";
      rssi = 0;
      signalQuality = "N/A";
    }

    // ✅ Preparar buffers para time/date
    char timeStr[32] = "NO SYNC";
    char dateStr[32] = "";
    if (KissTime::getInstance().isTimeSynced()) {
      KissTime::getInstance().getTimeString(timeStr, sizeof(timeStr));
      KissTime::getInstance().getDateString(dateStr, sizeof(dateStr));
    }
    // Buffer más grande para incluir stats LTE
    char status[1500];

    // Construir mensaje base
    int offset = snprintf(status, sizeof(status),
             "📦 KissTelegram v%s\n"
             "📨 Build: %s (0x%08X)\n\n"

             "%s\n"  // TITLE_RELIABILITY
             "%s %s: %s\n"  // SYSTEM
             "✅ %s: %lu\n"  // SENT
             "💾 %s: %d\n"  // PENDING
             "❌ %s: %d\n"  // LOST
             "📦 %s: %d\n\n"  // DISCARDED

             "%s\n"  // TITLE_EXTERNAL
             "⚠️ %s: %d\n"  // ERRORS
             "🔄 %s: %d\n"  // RECOVERED
             "📡 %s: %d\n\n"  // WIFI_DROPS

             "%s\n"  // TITLE_TECHNICAL
             "⏱️ %s: %luh %lum\n"  // UPTIME
             "💾 %s: %d bytes\n"  // FREE_RAM
             "💾 %s: %d bytes\n"  // FREE_PSRAM
             "💿 %s: %d bytes\n"  // FREE_FS
             "💾 %s: %d %s\n"  // MAX_FS
             "🔋 %s: %d \n"  // POWER_MODE
             "🌐 Conexión: %s\n"  // CONNECTION_MODE
             "📡 RSSI: %d dBm (%s)\n",  // SIGNAL

             KISS_GET_VERSION(),
             BUILD_TIMESTAMP, getBuildHash(),

             _(STATUS_TITLE_RELIABILITY),
             reliabilityEmoji, _(STATUS_SYSTEM), reliabilityText,
             _(STATUS_SENT), totalSent,
             _(STATUS_PENDING), pendingFS,
             _(STATUS_LOST), lostMessages,
             _(STATUS_DISCARDED), stats.queueFullCount,

             _(STATUS_TITLE_EXTERNAL),
             _(STATUS_ERRORS), stats.errorCount,
             _(STATUS_RECOVERED), stats.fallbackEvents,
             _(STATUS_WIFI_DROPS), stats.wifiDropouts,

             _(STATUS_TITLE_TECHNICAL),
             _(STATUS_UPTIME), (millis() - stats.startTime) / 3600000, ((millis() - stats.startTime) % 3600000) / 60000,
             _(STATUS_FREE_RAM), ESP.getFreeHeap(),
             _(STATUS_FREE_PSRAM), ESP.getFreePsram(),
             _(STATUS_FREE_FS), bot->getFreeStorage(),
             _(STATUS_MAX_FS), KISS_MAX_FS_QUEUE, _(STATUS_MESSAGES),
             _(STATUS_POWER_MODE), bot->getCurrentPowerMode(),
             connectionMode,
             rssi, signalQuality);

    // ✅ Añadir estadísticas LTE si se está usando LTE
#ifdef KISS_HAS_LTE
    if (currentConnection == CONN_LTE) {
      int32_t lteRssi = lte.getSignalStrength();
      offset += snprintf(status + offset, sizeof(status) - offset,
                        "\n\n📡 MÓDULO LTE\n"
                        "📶 Señal: %d dBm\n"
                        "🔌 Modo: %s",
                        lteRssi,
                        lte.isConnected() ? "Conectado" : "Standby");
    }
#endif

    offset += snprintf(status + offset, sizeof(status) - offset,
             "\n🔒 %s: %s\n"  // SSL
             "🚀 %s: %s\n"  // TURBO
             "🤖 %s: %s",  // AUTO_MSGS
             _(STATUS_SSL), bot->isSSLSecure() ? _(STATUS_SSL_SECURE) : _(STATUS_SSL_INSECURE),
             _(STATUS_TURBO), bot->isTurboMode() ? _(STATUS_ACTIVE) : _(STATUS_INACTIVE),
             _(STATUS_AUTO_MSGS), config.getAutoMessagesEnabled() ? _(STATUS_YES) : _(STATUS_NO));

    bot->sendMessage(chat_id, status);
  } else if (strcmp(command, "/config") == 0) {
    config.printConfig();
    bot->sendMessage(chat_id, "⚙️ Configuración enviada al monitor serie");
  } else if (strcmp(command, "/debug") == 0) {
    Serial.println("\n🔍 SOLICITUD DEBUG DETALLADA");
    bot->printDiagnostics();
    bot->printStorageStatus();
    Serial.printf("📊 Stats: Mensajes=%d, Errores=%d, Dropouts=%d, Fallbacks=%d\n",
                  stats.messageCount, stats.errorCount, stats.wifiDropouts, stats.fallbackEvents);
    Serial.printf("⏰ Uptime: %lu segundos\n", (millis() - stats.startTime) / 1000);
    bot->sendMessage(chat_id, "🔍 Debug completo enviado al monitor serie");
  } else if (strcmp(command, "/debugfs") == 0) {
    if (!KISS_FS.begin(KISS_FS_FORMAT_ON_FAIL)) {
      bot->sendMessage(chat_id, "❌ Error montando FS");
      return;
    }

    File file = KISS_FS.open(KISS_FS_QUEUE_FILE, FILE_READ);
    if (!file) {
      bot->sendMessage(chat_id, "❌ No existe archivo de cola");
      KISS_FS.end();
      return;
    }

    char content[1024];
    size_t bytesRead = file.readBytes(content, sizeof(content) - 1);
    content[bytesRead] = '\0';
    file.close();
    KISS_FS.end();

    // Mostrar primeros 500 caracteres
    Serial.println("📄 CONTENIDO LITTLEFS:");
    if (bytesRead > 500) {
      char temp = content[500];
      content[500] = '\0';
      Serial.println(content);
      content[500] = temp;
    } else {
      Serial.println(content);
    }

    // Contar mensajes s:0 y s:1
    int pending = 0;
    char* pos = content;
    while (true) {
      pos = strstr(pos, "\"s\":0");  //string pero se destruye al terminar la expr.
      if (!pos) break;
      pending++;
      pos += 5;
    }

    char result[200];
    snprintf(result, sizeof(result),
             "📊 ANÁLISIS LITTLEFS:\n\n"
             "Pendientes: %d\n\n"
             "ℹ️ Solo se guardan mensajes\n"
             "pendientes. Los confirmados\n"
             "por Telegram se borran.\n\n"
             "Ver serial para JSON",
             pending);
    bot->sendMessage(chat_id, result);
  } else if (strcmp(command, "/memoria") == 0) {
    char memInfo[200];
    snprintf(memInfo, sizeof(memInfo),
             "💾 INFORMACIÓN MEMORIA\n\n"
             "Libre: %d bytes\n"
             "Mínimo: %d bytes\n"
             "Máximo alloc: %d bytes\n"
             "Tamaño heap: %d bytes\n"
             "LittleFS usado: %d bytes",
             ESP.getFreeHeap(),
             ESP.getMinFreeHeap(),
             ESP.getMaxAllocHeap(),
             ESP.getHeapSize(),
             bot->getStorageUsage());
    bot->sendMessage(chat_id, memInfo);
  } else if (strcmp(command, "/storage") == 0) {
    char storageInfo[300];
    bot->getStorageInfo(storageInfo, sizeof(storageInfo));
    bot->sendMessage(chat_id, storageInfo);

    bot->printStorageStatus();
    Serial.println("💾 Info almac. enviada también al monitor serie");
  } else if (strcmp(command, "/turbo") == 0) {
    if (bot->isTurboMode()) {
      bot->disableTurboMode();
      bot->sendMessage(chat_id,
       "🐢 Modo TURBO desactivado\n"
       "Ver serial para estadísticas");
    } else {
      int pending = bot->getMessagesInFS();
      char turboMsg[200];
      snprintf(turboMsg, sizeof(turboMsg),
               "🚀 Modo TURBO activado\n\n"
               "⚡ Intervalo: 50ms\n"
               "📦 Batch: 10 msgs/ciclo\n"
               "🗑️ Delete batch: 10 msgs\n"
               "📨 Pendientes: %d",
               pending);
      bot->enableTurboMode();
      bot->sendMessage(chat_id, turboMsg);
    }
  } else if (strcmp(command, "/llenar") == 0) {
    int cantidad = 20;
    if (strlen(param) > 0) {
      cantidad = atoi(param);
      cantidad = max(5, min(cantidad, 1000));
    }

    char msg[150];
    snprintf(msg, sizeof(msg), "🔄 Encolando %d mensajes...\n🚀 Turbo mode se activará automáticamente", cantidad);
    bot->sendMessage(chat_id, msg);

    testFallbackScenario(cantidad);

    snprintf(msg, sizeof(msg), 
            "✅ Comienza la prueba..\n"
            "💾 LittleFS: %d pendientes\n"
            "🚀 Turbo: %s",
             bot->getMessagesInFS(),
             bot->isTurboMode() ? "ACTIVO" : "OFF");
    bot->sendMessage(chat_id, msg);
  } else if (strcmp(command, "/forceprocess") == 0) {
    int before = bot->getMessagesInFS();
    bot->processQueue();
    int after = bot->getMessagesInFS();

    char msg[100];
    snprintf(msg, sizeof(msg), "🔄 Procesamiento forzado\nAntes: %d → Después: %d", before, after);
    bot->sendMessage(chat_id, msg);
  } else if (strcmp(command, "/borrar") == 0) {
    if (strlen(param) == 0) {
      int pending = bot->getMessagesInFS();
      if (pending > 0) {
        char confirmMsg[150];
        snprintf(confirmMsg, sizeof(confirmMsg), 
        "⚠️ CONFIRMAR BORRADO\n\n"
        "Hay %d mensajes pendientes\n\n"
        "Escribe: /borrar confirmar",
        pending);
        bot->sendMessage(chat_id, confirmMsg);
      } else {
        bot->sendMessage(chat_id, "ℹ️ No hay mensajes pendientes");
      }
    } else if (strcmp(param, "confirmar") == 0) {
      int deletedCount = bot->getMessagesInFS();
      bot->clearStorage();

      char resultMsg[150];
      snprintf(resultMsg, sizeof(resultMsg),
               "🗑️ BORRADO COMPLETADO\n\n"
               "Eliminados: %d mensajes\n"
               "LittleFS limpiado\n",
               deletedCount);
      bot->sendMessage(chat_id, resultMsg);
      Serial.printf("🗑️ Usuario borró %d mensajes pendientes\n", deletedCount);
    } else {
      bot->sendMessage(chat_id, "❌ Usa: /borrar confirmar");
    }
  } else if (strcmp(command, "/prioridades") == 0) {
    bot->sendMessage(chat_id, "🧪 INICIANDO TEST DE PRIORIDADES...");
    testPrioridades();
  } else if (strcmp(command, "/testlittlefs") == 0) {
    bot->sendMessage(chat_id, "🧪 TEST LITTLEFS\n"
                              "Generando 10 mensajes...");

    for (int i = 1; i <= 10; i++) {
      char msg[50];
      snprintf(msg, sizeof(msg), "Test LittleFS %d/10", i);
      bot->queueMessage(credentials.getChatId(), msg, KissTelegram::PRIORITY_NORMAL);
    }

    int fsCount = bot->getMessagesInFS();

    char result[250];
    snprintf(result, sizeof(result),
     "✅ 10 mensajes generados\n\n"
     "💾 Pendientes en FS: %d\n"
     "🔄 Reinicia el ESP32 y verifica\n"
     "que los mensajes persisten\n"
     "con /estado");
     bot->sendMessage(chat_id, result);
  } else if (strcmp(command, "/stats") == 0) {
    unsigned long totalQueued = config.getTotalMessagesQueued();
    unsigned long totalSent = config.getTotalMessagesSent();
    unsigned long pendingFS = bot->getMessagesInFS();

    float successRate = (totalQueued > 0) ? (totalSent * 100.0) / totalQueued : 0;

    char statsMsg[400];
    snprintf(statsMsg, sizeof(statsMsg), 
    "📊 ESTADÍSTICAS LIFETIME\n\n"
    "📨 Total encolados: %lu\n"
    "✅ Total enviados: %lu\n"
    "💾 Pendientes FS: %lu\n"
    "🔄 En tránsito: %lu\n"
    "📈 Tasa éxito: %.1f%%\n\n"
    "⏰ Uptime: %lu seg (%luh %lum)\n"
    "💾 Memoria: %d bytes\n"
    "🔋 Power Mode: %d",
             totalQueued,
             totalSent,
             pendingFS,
             totalQueued - totalSent,
             successRate,
             (millis() - stats.startTime) / 1000,
             (millis() - stats.startTime) / 3600000,
             ((millis() - stats.startTime) % 3600000) / 60000,
             KissTelegram::getFreeMemory(),
             bot->getCurrentPowerMode());
    bot->sendMessage(chat_id, statsMsg);

#ifdef KISS_HAS_LTE
  } else if (strcmp(command, "/lte") == 0) {
    // Obtener información de red desde KissTelegram
    String networkInfo = bot->getNetworkInfo();
    String networkMode = bot->getNetworkMode();

    char lteMsg[800];
    snprintf(lteMsg, sizeof(lteMsg),
             "📡 GESTIÓN DE RED\n\n"
             "🌐 Red actual:\n%s\n\n"
             "⚙️ Modo de red:\n%s\n\n"
             "🔄 Comandos disponibles:\n"
             "/ltewifi - Forzar WiFi\n"
             "/ltelte - Forzar LTE\n"
             "/lteauto - Modo automático\n\n"
             "💡 El modo se guarda en NVS\n"
             "y persiste tras reiniciar",
             networkInfo.c_str(),
             networkMode.c_str());

    bot->sendMessage(chat_id, lteMsg);
  } else if (strcmp(command, "/ltewifi") == 0) {
    if (bot->switchToWiFi()) {
      bot->sendMessage(chat_id, "✅ Cambiando a WiFi...\n🔄 Reconectando");
    } else {
      bot->sendMessage(chat_id, "❌ Error cambiando a WiFi");
    }
  } else if (strcmp(command, "/ltelte") == 0) {
    if (bot->switchToLTE()) {
      bot->sendMessage(chat_id, "✅ Cambiando a LTE...\n🔄 Reconectando");
    } else {
      bot->sendMessage(chat_id, "❌ Error cambiando a LTE");
    }
  } else if (strcmp(command, "/lteauto") == 0) {
    if (bot->switchToAuto()) {
      bot->sendMessage(chat_id, "✅ Modo automático\n📡 WiFi → LTE fallback");
    } else {
      bot->sendMessage(chat_id, "❌ Error cambiando a modo auto");
    }
#endif

  } else if (strcmp(command, "/time") == 0) {
    if (KissTime::getInstance().isTimeSynced()) {
      char timeStr[32], dateStr[32];
      KissTime::getInstance().getTimeString(timeStr, sizeof(timeStr));
      KissTime::getInstance().getDateString(dateStr, sizeof(dateStr));

      char timeMsg[150];
      snprintf(timeMsg, sizeof(timeMsg),
               "⏰ HORA ACTUAL\n\n"
               "📅 Fecha: %s\n"
               "🕐 Hora: %s\n"
               "⏱️ Último sync: hace %lu seg",
               dateStr, timeStr,
               KissTime::getInstance().getLastSyncAge());
      bot->sendMessage(chat_id, timeMsg);
    } else {
      bot->sendMessage(chat_id, "❌ Reloj no sincronizado\nUsa /resync");
    }
  } else if (strcmp(command, "/timestatus") == 0) {
    KissTime::getInstance().printStatus();
    bot->sendMessage(chat_id, "⏰ Estado reloj enviado al monitor serie");

    char timeStr[32], dateStr[32];

    if (KissTime::getInstance().isTimeSynced()) {
      KissTime::getInstance().getTimeString(timeStr, sizeof(timeStr));
      KissTime::getInstance().getDateString(dateStr, sizeof(dateStr));

      char msg[150];
      snprintf(msg, sizeof(msg),
               "⏰ *ESTADO RELOJ*\n\n"
               "🕐 Hora: %s\n"
               "📅 Fecha: %s\n"
               "✅ Sincronizado con NTP",
               timeStr, dateStr);
      bot->sendMessage(chat_id, msg);
    } else {
      bot->sendMessage(chat_id, "❌ Reloj no sincronizado");
    }
  } else if (strcmp(command, "/resync") == 0) {
    bot->sendMessage(chat_id, "🔄 Sincronizando NTP...");

    if (KissTime::getInstance().resyncNTP()) {
      char timeStr[32];
      KissTime::getInstance().getTimeString(timeStr, sizeof(timeStr));

      char msg[100];
      snprintf(msg, sizeof(msg),
               "✅ NTP sincronizado\n\n"
               "🕐 Hora: %s",
               timeStr);
      bot->sendMessage(chat_id, msg);
    } else {
      bot->sendMessage(chat_id, "❌ Error en sync NTP\nVerifica WiFi");
    }
  } else if (strcmp(command, "/testssl") == 0) {
    Serial.println("\n🔒 SOLICITUD PRUEBA SSL...");
    bool sslResult = bot->testSSLConnection();

    if (sslResult) {
      bot->sendMessage(chat_id, "✅ PRUEBA SSL EXITOSA\n\nVer monitor serie para detalles");
    } else {
      bot->sendMessage(chat_id, "❌ PRUEBA SSL FALLIDA\n\nVer monitor serie para errores");
    }
  } else if (strcmp(command, "/parar") == 0) {
    config.setAutoMessagesEnabled(false);
    bot->sendMessage(chat_id, "🚫 Mensajes automáticos desactivados por comando");
    Serial.println("🚫 Mensajes automáticos desactivados por comando");
  } else if (strcmp(command, "/activar") == 0) {
    config.setAutoMessagesEnabled(true);
    bot->sendMessage(chat_id, "✅ Mensajes automáticos activados por comando");
    Serial.println("✅ Mensajes automáticos activados por comando");
  } else if (strcmp(command, "/changepin") == 0) {
    char oldPin[10], newPin[10];
    if (sscanf(param, "%s %s", oldPin, newPin) == 2) {
      if (strcmp(oldPin, credentials.getOTAPin()) == 0) {
        if (credentials.setOTAPin(newPin)) {
          bot->sendMessage(chat_id, "✅ PIN OTA cambiado correctamente");
        } else {
          bot->sendMessage(chat_id, "❌ Error: PIN inválido (4-6 dígitos)");
        }
      } else {
        bot->sendMessage(chat_id, "❌ PIN actual incorrecto");
      }
    } else {
      bot->sendMessage(chat_id, "Uso: /changepin <viejo> <nuevo>");
    }
  } else if (strcmp(command, "/changepuk") == 0) {
    char oldPuk[15], newPuk[15];
    if (sscanf(param, "%s %s", oldPuk, newPuk) == 2) {
      if (strcmp(oldPuk, credentials.getOTAPuk()) == 0) {
        if (credentials.setOTAPuk(newPuk)) {
          bot->sendMessage(chat_id, "✅ PUK OTA cambiado correctamente");
        } else {
          bot->sendMessage(chat_id, "❌ Error: PUK inválido (8-12 dígitos)");
        }
      } else {
        bot->sendMessage(chat_id, "❌ PUK actual incorrecto");
      }
    } else {
      bot->sendMessage(chat_id, "Uso: /changepuk <viejo> <nuevo>");
    }
  } else if (strcmp(command, "/credentials") == 0) {
    credentials.printStatus();
    bot->sendMessage(chat_id, "🔑 Info credenciales enviada al monitor serie");
  } else if (strcmp(command, "/resetcredentials") == 0) {
    if (strcmp(param, "CONFIRMAR") == 0) {
      credentials.resetToFallback();
      bot->sendMessage(chat_id, "🔄 Credenciales reseteadas a default\n ⚠️ Reiniciando...");
      SAFE_DELAY(2000);
      ESP.restart();
    } else {
      bot->sendMessage(chat_id,
                       "⚠️ Esto resetea todas las credenciales\n"
                       "Confirma con: /resetcredentials CONFIRMAR");
    }
  }
#ifdef KISS_HAS_OTA
  else if (strcmp(command, "/ota") == 0) {
    if (ota->isActive()) {
      bot->sendMessage(chat_id, "⚠️ OTA ya en progreso");
    } else {
      ota->startOTA();
    }
  } else if (strcmp(command, "/otaok") == 0) {
    // ✅ ACK inmediato: comando recibido
    bot->sendMessage(chat_id, "✅ Comando recibido");
    Serial.println("🔄 Procesando /otaok - validación de firmware");
    ota->handleOTACommand(command, param);
  } else if (strcmp(command, "/otapin") == 0 || strcmp(command, "/otapuk") == 0 || strcmp(command, "/otaconfirm") == 0 || strcmp(command, "/otacancel") == 0 || strcmp(command, "/otaconfirmdelete") == 0 || strcmp(command, "/reverse") == 0) {
    ota->handleOTACommand(command, param);
  }
#endif
  else if (command[0] != '\0') {
    Serial.printf("❌ Comando desconocido: %s\n", command);
    bot->sendMessage(chat_id, "❌ Comando no reconocido. Usa /help para ayuda.");
  }
}

void sendTestMessage(const char* message) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("❌ WiFi no conectado");
    stats.errorCount++;
    return;
  }

  if (!wifiAlreadyStable) {
    Serial.println("🎯 Configurando WiFi estable (PRIMERA VEZ)");
    bot->setWifiStable();
    bot->enable();
    wifiAlreadyStable = true;

    Serial.println("⏳ Esperando estabilización WiFi (5 seg)...");
    for (int i = 0; i < 5; i++) {
      SAFE_DELAY(1000);
      Serial.print(".");
    }
    Serial.println("✅ WiFi estabilizado");
  }

  char fullMessage[300];
  snprintf(fullMessage, sizeof(fullMessage),
           "%s\n\n"
           "⏰ Uptime: %lu seg\n"
           "💾 Memoria: %d bytes\n"
           "📨 Enviados: %d\n"
           "💾 LittleFS: %d pendientes\n"
           "❌ Errores: %d\n"
           "🔄 Fallo de respaldo: %d",
           message,
           (millis() - stats.startTime) / 1000,
           KissTelegram::getFreeMemory(),
           stats.messageCount,
           bot->getMessagesInFS(),
           stats.errorCount,
           stats.fallbackEvents);

  if (bot->sendMessage(credentials.getChatId(), fullMessage, KissTelegram::PRIORITY_NORMAL)) {
    stats.messageCount++;
    stats.lastMessageTime = millis();
    Serial.printf("✅ Mensaje automático %d enviado\n", stats.messageCount);
  } else {
    stats.errorCount++;
    Serial.printf("❌ Error mensaje %d - encolando...\n", stats.messageCount);

    if (bot->queueMessage(credentials.getChatId(), fullMessage, KissTelegram::PRIORITY_NORMAL)) {
      Serial.println("📨 Mensaje encolado para reintento");
    } else {
      stats.queueFullCount++;
      Serial.println("❌ Cola llena - mensaje descartado");
    }
  }
}

void checkConnectionStability() {
  static bool wasConnected = false;
  bool isConnected = (WiFi.status() == WL_CONNECTED);

  // ========== WiFi se desconectó mientras estaba en modo WiFi ==========
  if (currentConnection == CONN_WIFI && wasConnected && !isConnected) {
    stats.wifiDropouts++;
    stats.lastWifiDrop = millis();
    Serial.println("\n⚠️ WiFi DESCONECTADO");
    bot->disable();
    wifiAlreadyStable = false;

#ifdef KISS_HAS_LTE
    // Activar fallback a LTE
    Serial.println("📡 Activando fallback LTE...");
    if (lte.powerOn()) {
      Serial.println("✅ LTE activado como fallback");
      currentConnection = CONN_LTE;
      stats.lteActivations++;

      bot->enable();
      bot->queueMessage(credentials.getChatId(),
                       "📡 Sistema usa LTE por fallo WiFi",
                       KissTelegram::PRIORITY_HIGH);
    } else {
      Serial.println("❌ Error: LTE no disponible");
      currentConnection = CONN_NONE;
    }
#endif
  }

#ifdef KISS_HAS_LTE
  // ========== Estamos en LTE, verificar si WiFi volvió ==========
  else if (currentConnection == CONN_LTE) {
    // WiFi volvió - prioridad WiFi
    if (!wasConnected && isConnected) {
      Serial.println("\n✅ WiFi detectado, cambiando a WiFi...");

      // Poner LTE en sleep mode (NO desconectar para failover instantáneo)
      // Mantiene red registrada pero consume solo 10-12mA
      Serial.println("💤 LTE → modo sleep (listo para failover)");
      lte.setPowerMode(CLIENT_POWER_IDLE);  // AT+CSCLK=1

      currentConnection = CONN_WIFI;
      bot->setWifiStable();
      wifiAlreadyStable = true;

      bot->queueMessage(credentials.getChatId(),
                       "✅ WiFi restaurado",
                       KissTelegram::PRIORITY_HIGH);
    }
    // Verificar que LTE sigue operativo
    else if (!lte.isConnected()) {
      Serial.println("\n⚠️ LTE desconectado, verificando estado...");
      if (!lte.powerOn()) {
        Serial.println("❌ LTE no disponible");
        currentConnection = CONN_NONE;
      }
    }
  }
#endif

  // ========== WiFi reconectado en modo WiFi inicial ==========
  if (!wasConnected && isConnected && currentConnection == CONN_WIFI) {
    Serial.println("📡 WiFi reconectado");
    bot->setWifiStable();
    bot->enable();
    wifiAlreadyStable = true;

    // MENSAJE INICIO CON BUILD INFO AUTOMÁTICO
    // Obtener RSSI (señal WiFi)
    int32_t rssi = WiFi.RSSI();
    const char* signalQuality;
    if (rssi >= -50) {
      signalQuality = _(WIFI_EXCELLENT);
    } else if (rssi >= -60) {
      signalQuality = _(WIFI_GOOD);
    } else if (rssi >= -70) {
      signalQuality = _(WIFI_FAIR);
    } else {
      signalQuality = _(WIFI_WEAK);
    }

    char reconMsg[450];
    snprintf(reconMsg, sizeof(reconMsg),
             _(HELLO_MESSAGE),
             BUILD_TIMESTAMP,
             getBuildHash(),
             KISS_GET_VERSION(),
             stats.wifiDropouts,
             (millis() - stats.startTime) / 1000,
             ESP.getFreeHeap(),
             bot->getMessagesInFS(),
             rssi,
             signalQuality);
    bot->queueMessage(credentials.getChatId(), reconMsg, KissTelegram::PRIORITY_CRITICAL);
  }

  wasConnected = isConnected;
}


void loop() {

#ifdef KISS_HAS_LTE
  // Procesar URCs del módulo LTE
  lte.loop();
#endif

#ifdef KISS_HAS_OTA
  if (ota && ota->isActive()) {
    ota->loop();
  }
#endif

  // AUTO-RESYNC NTP cada hora
  static unsigned long lastTimeCheck = 0;
  if (bot && bot->hasTimePassed(lastTimeCheck, 3600000)) {
    if (KissTime::getInstance().needsResync()) {
      Serial.println("🔄 Resync NTP automático...");
      KissTime::getInstance().resyncNTP();
    }
    lastTimeCheck = millis();
  }

  // AUTO-SAVE cada 5 minutos
  static unsigned long lastAutoSave = 0;
  if (bot && bot->hasTimePassed(lastAutoSave, 300000)) {
    bot->autoSave();
    lastAutoSave = millis();
    Serial.println("💾 Auto-guardado ejecutado");
  }

  // Actualizar power state regularmente (cada 10 segundos, no 5)
  static unsigned long lastPowerUpdate = 0;
  if (bot && bot->hasTimePassed(lastPowerUpdate, 10000)) {
    bot->updatePowerState();
    lastPowerUpdate = millis();
  }

  checkConnectionStability();

  // Reducir delay de 1000ms a 100ms cuando hay cola
  // Esto permite procesar mensajes más rápido
  int pendingCount = bot ? bot->getMessagesInFS() : 0;
  int loopDelay = (pendingCount > 0) ? 100 : 500;  // Rápido si hay cola, lento si no

  // En modo turbo, delay aún más corto
  if (bot && bot->isTurboMode()) {
    loopDelay = 10;
  }

  if (bot) {
    // Procesar cola (ahora procesa hasta 5-10 mensajes por llamada)
    bot->processQueue();

    // Desactivar turbo mode automáticamente cuando la cola se vacía
    if (bot->isTurboMode() && pendingCount == 0) {
      bot->disableTurboMode();
      Serial.println("✅ Cola vacía - Turbo mode desactivado");
    }

    if (bot->isWifiStable()) {
      bot->checkMessages(handleMessage);
    }
  }

  static unsigned long lastAutoMessage = 0;
  if (config.getAutoMessagesEnabled() && bot && bot->hasTimePassed(lastAutoMessage, 300000)) {
    sendTestMessage("⏰ Mensaje automático cada 5 min");
    lastAutoMessage = millis();
  }

  // Diagnóstico menos frecuente cuando hay cola activa
  static unsigned long lastDiagnostic = 0;
  unsigned long diagInterval = (pendingCount > 0) ? 1800000 : 900000;  // 30min si hay cola, 15min si no
  if (bot && bot->hasTimePassed(lastDiagnostic, diagInterval)) {
    Serial.println("\n--- DIAGNÓSTICO AUTOMÁTICO ---");
    bot->printDiagnostics();
    bot->printStorageStatus();
    Serial.printf("📊 Stats: Msgs=%d, Errors=%d, Dropouts=%d, Fallbacks=%d\n",
                  stats.messageCount, stats.errorCount, stats.wifiDropouts, stats.fallbackEvents);
    Serial.printf("💾 LittleFS: %d mensajes pendientes\n", bot->getMessagesInFS());
    Serial.printf("⏰ Uptime: %lu min\n", (millis() - stats.startTime) / 60000);
    Serial.println("--------------------------------\n");
    lastDiagnostic = millis();
  }

  static unsigned long lastMemCheck = 0;
  if (bot && bot->hasTimePassed(lastMemCheck, 600000)) {
    int freeMem = ESP.getFreeHeap();
    int minMem = ESP.getMinFreeHeap();
    Serial.printf("💾 Memoria: %d bytes (mínimo: %d)\n", freeMem, minMem);

    if (freeMem < 50000) {
      Serial.println("⚠️ ALERTA: Memoria baja");
      char alert[150];
      snprintf(alert, sizeof(alert),
               "⚠️ MEMORIA BAJA\n\n"
               "Libre: %d bytes\n"
               "Mínimo: %d bytes\n"
               "💾 Pendientes: %d msg",
               freeMem, minMem, bot->getMessagesInFS());
      bot->queueMessage(credentials.getChatId(), alert, KissTelegram::PRIORITY_HIGH);
    }
    lastMemCheck = millis();
  }

  static unsigned long lastHeartbeat = 0;
  if (bot && bot->hasTimePassed(lastHeartbeat, 30000)) {
    Serial.print("💚");
    lastHeartbeat = millis();
  }

  // ✅ OPTIMIZACIÓN LTE: ping adaptativo según tipo de red y power mode
  static unsigned long lastPingTime = 0;
  if (bot && bot->isWifiStable() && pendingCount == 0) {
    // Obtener intervalo recomendado según red (WiFi: 60s, LTE: 2-10 min según modo)
    int pingInterval = bot->getRecommendedPingInterval();

    if (bot->hasTimePassed(lastPingTime, pingInterval)) {
      // Solo hacer ping si hay conexión activa
      // En modo LTE low-power, la conexión estará cerrada y esto se saltará
      if (bot->isConnected()) {
        if (!bot->pingTelegram()) {
          KISS_LOG("💓 Ping falló, socket muerto");
        }
      } else {
        // No hay conexión, resetear timer para no intentar constantemente
        if (bot->isUsingLTE()) {
          KISS_LOG("📡 LTE: Sin conexión activa (sleep mode activo)");
        }
      }
      lastPingTime = millis();
    }
  }

  bot->checkConnectionAge();

#ifdef KISS_HAS_LTE
  // ========== PASS-THROUGH SERIAL LTE (DEBUG) ==========
  // Reenviar comandos desde Serial (Monitor IDE) a Serial1 (LTE)
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();

    // Comandos especiales para control de pines
    if (cmd.equalsIgnoreCase("PWRKEY_ON")) {
      digitalWrite(KISS_LTE_PWRKEY_PIN, HIGH);
      Serial.println("✅ PWRKEY → HIGH");
    } else if (cmd.equalsIgnoreCase("PWRKEY_OFF")) {
      digitalWrite(KISS_LTE_PWRKEY_PIN, LOW);
      Serial.println("✅ PWRKEY → LOW");
    } else if (cmd.equalsIgnoreCase("PWRKEY_PULSE")) {
      digitalWrite(KISS_LTE_PWRKEY_PIN, HIGH);
      delay(1500);
      digitalWrite(KISS_LTE_PWRKEY_PIN, LOW);
      Serial.println("✅ PWRKEY → PULSE (1.5s)");
    } else if (cmd.equalsIgnoreCase("DTR_HIGH")) {
      digitalWrite(KISS_LTE_DTR_PIN, HIGH);
      Serial.println("✅ DTR → HIGH (sleep allowed)");
    } else if (cmd.equalsIgnoreCase("DTR_LOW")) {
      digitalWrite(KISS_LTE_DTR_PIN, LOW);
      Serial.println("✅ DTR → LOW (wake up)");
    } else if (cmd.equalsIgnoreCase("HELP")) {
      Serial.println("\n========== COMANDOS LTE DEBUG ==========");
      Serial.println("Comandos AT: Escribe cualquier comando AT");
      Serial.println("  Ejemplo: AT, AT+CSQ, AT+COPS?");
      Serial.println("\nControl PWRKEY:");
      Serial.println("  PWRKEY_ON    - Pin HIGH");
      Serial.println("  PWRKEY_OFF   - Pin LOW");
      Serial.println("  PWRKEY_PULSE - Pulso 1.5s");
      Serial.println("\nControl DTR:");
      Serial.println("  DTR_HIGH - Sleep permitido");
      Serial.println("  DTR_LOW  - Wake up");
      Serial.println("\nUtilidad:");
      Serial.println("  HELP - Esta ayuda");
      Serial.println("========================================\n");
    } else if (cmd.length() > 0) {
      // Comando AT normal - enviarlo al LTE
      Serial1.println(cmd);
      Serial.printf("→ LTE: %s\n", cmd.c_str());
    }
  }

  // Reenviar respuestas del LTE a Serial
  while (Serial1.available()) {
    char c = Serial1.read();
    Serial.write(c);
  }
#endif

  SAFE_DELAY(loopDelay);
}