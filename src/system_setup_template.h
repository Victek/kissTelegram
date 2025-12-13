// system_setup.h
// Vicente Soriano - victek@gmail.com
// Manager de configuración KissTelegram Suite

#ifndef SYSTEM_SETUP_H
#define SYSTEM_SETUP_H

#include <Arduino.h>
#include <LittleFS.h>

// ========== MODO DESARROLLO/PRODUCCIÓN ==========
// ========== TRUE DESARROLLO/FALSE PRODUCCIÓN
// Así paras el loro por el serial ..pero la verdad es que no molesta demasiado
#define KISS_DEVELOPMENT_MODE true

// ========== MACROS DE LOGGING UNIFICADAS ==========
#if KISS_DEVELOPMENT_MODE
#define KISS_LOG(msg) \
  do { Serial.println(msg); } while (0)
#define KISS_LOGF(fmt, ...) \
  do { \
    Serial.printf(fmt, ##__VA_ARGS__); \
    Serial.println(); \
  } while (0)
#define KISS_CRITICAL(msg) \
  do { Serial.println(msg); } while (0)
#define KISS_CRITICALF(fmt, ...) \
  do { \
    Serial.printf(fmt, ##__VA_ARGS__); \
    Serial.println(); \
  } while (0)
#else
#define KISS_LOG(msg)
#define KISS_LOGF(fmt, ...)
#define KISS_CRITICAL(msg)
#define KISS_CRITICALF(fmt, ...)
#endif

// ========== CONFIGURACIÓN FILESYSTEM LITTLEFS ==========
#define KISS_FS LittleFS
#define KISS_FS_NAME "LittleFS"

// Configuración filesystem
#define KISS_FS_FORMAT_ON_FAIL true
#define KISS_FS_MAX_FILES 10
#define KISS_FS_QUEUE_FILE "/queue_messages.log"

/* Para Platformio/For Platformio to compile with ESP_IDF version
inline void KISS_INIT_WDT() {
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
  // ESP-IDF 5.x
  esp_task_wdt_config_t wdt_config = {
    .timeout_ms = KISS_WDT_TIMEOUT_MS,
    .idle_core_mask = 0,
    .trigger_panic = false
  };
  esp_task_wdt_deinit();
  esp_task_wdt_init(&wdt_config);
#else
  // ESP-IDF 4.x
  esp_task_wdt_deinit();
  esp_task_wdt_init(KISS_WDT_TIMEOUT_MS / 1000, false);
#endif
}
*/

// ========== MACROS MEJORADAS PARA LITTLEFS ==========
inline bool KISS_INIT_FS() {
  KISS_LOGF("💾 Iniciando %s...", KISS_FS_NAME);
  bool success = KISS_FS.begin(KISS_FS_FORMAT_ON_FAIL);
  if (success) {
    size_t total = KISS_FS.totalBytes();
    size_t used = KISS_FS.usedBytes();
    KISS_LOGF("✅ %s montado (%.1f%% usado)\n", KISS_FS_NAME, (used * 100.0) / total);
  } else {
    KISS_CRITICALF("❌ Error montando %s\n", KISS_FS_NAME);
  }

  return success;
}

inline float KISS_GET_FS_USAGE() {
  size_t total = KISS_FS.totalBytes();
  size_t used = KISS_FS.usedBytes();
  return (total > 0) ? (used * 100.0) / total : 0.0;
}

inline bool KISS_CHECK_FS_HEALTH() {
  float usage = KISS_GET_FS_USAGE();
  if (usage > 80.0) {
    KISS_LOGF("⚠️ %s al %.1f%% de capacidad\n", KISS_FS_NAME, usage);
    return false;
  }
  return true;
}

// Esto lo debes rellenar si quieres que Telegram te diga algo!!
// Es conveniente que cambies los fallback del PIN y el PUK ahora ... y lo recuerdes al probar el OTA, podrás? No bebas alcohol .. y deja el móvil un rato. 
// ========== CREDENCIALES FALLBACK ==========
#define KISS_FALLBACK_BOT_TOKEN "YOUR_TELEGRAM_BOT_TOKEN"
#define KISS_FALLBACK_CHAT_ID "YOUR_CHAT_ID number"
#define KISS_FALLBACK_WIFI_SSID "YOUR_WIFI_SSID"
#define KISS_FALLBACK_WIFI_PASSWORD "YOUR_WIFI_PASSWORD"
#define KISS_FALLBACK_OTA_PIN "0000"
#define KISS_FALLBACK_OTA_PUK "00000000"

// ========== STORAGE NVS KEYS ==========
#define KISS_NVS_NAMESPACE "kiss_suite"
#define KISS_NVS_BOT_TOKEN_KEY "bot_token"
#define KISS_NVS_CHAT_ID_KEY "chat_id"
#define KISS_NVS_WIFI_SSID_KEY "wifi_ssid"
#define KISS_NVS_WIFI_PASS_KEY "wifi_pass"
#define KISS_NVS_OTA_PIN_KEY "ota_pin"
#define KISS_NVS_OTA_PUK_KEY "ota_puk"
#define KISS_NVS_OTA_LOCKED_KEY "ota_locked"

// ========== LÍMITES DE TAMAÑO CREDENCIALES ==========
#define KISS_MAX_TOKEN_LENGTH 64
#define KISS_MAX_CHAT_ID_LENGTH 20
#define KISS_MAX_SSID_LENGTH 32
#define KISS_MAX_WIFI_PASS_LENGTH 64
#define KISS_MAX_PIN_LENGTH 6
#define KISS_MAX_PUK_LENGTH 12

// ========== CONFIGURACIÓN STORAGE MEJORADO ==========
#define KISS_FS_USAGE_THRESHOLD 0.8
#define KISS_MAX_FS_QUEUE 3500

// ========== CONFIGURACIÓN RTOS ==========
#if defined(ESP_PLATFORM) || defined(ARDUINO_ARCH_ESP32)
#define KISS_USE_RTOS
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_task_wdt.h"

#define SAFE_YIELD() vTaskDelay(1)
#define SAFE_DELAY(ms) vTaskDelay(pdMS_TO_TICKS(ms))
#define KISS_WDT_TIMEOUT_MS 120000

inline void KISS_INIT_WDT() {
  esp_task_wdt_deinit();
  esp_task_wdt_config_t wdt_config = {
    .timeout_ms = KISS_WDT_TIMEOUT_MS,
    .idle_core_mask = 0,
    .trigger_panic = false
  };
  esp_task_wdt_init(&wdt_config);
}
#else
#define SAFE_YIELD() yield()
#define SAFE_DELAY(ms) delay(ms)
#define KISS_INIT_WDT()
#endif

// ========== LÍMITES DE MEMORIA ==========
#define KISS_MIN_FREE_HEAP 50000
#define KISS_CRITICAL_HEAP 20000

inline bool KISS_CHECK_HEAP() {
  int freeMem = ESP.getFreeHeap();
  if (freeMem < KISS_CRITICAL_HEAP) {
    KISS_CRITICALF("⛔ MEMORIA CRÍTICA: %d bytes\n", freeMem);
    return false;
  }
  if (freeMem < KISS_MIN_FREE_HEAP) {
    KISS_LOGF("⚠️ Memoria baja: %d bytes\n", freeMem);
  }
  return true;
}

// ========== CONFIGURACIÓN OTA - Un poco más realista ==========
#define KISS_OTA_SIZE_MARGIN (65536)     // 64KB margen partición
#define KISS_OTA_PSRAM_MARGIN (1048576)  // 1MB margen PSRAM
#define KISS_OTA_PIN_ATTEMPTS 3
#define KISS_OTA_AUTH_TIMEOUT 60000
#define KISS_OTA_CONFIRM_TIMEOUT 180000
#define KISS_OTA_CLEANUP_DELAY 180000

// ========== CONFIGURACIÓN NTP/TIEMPO ==========
#define KISS_NTP_SERVER "pool.ntp.org"
#define KISS_NTP_BACKUP "time.google.com"
#define KISS_NTP_TIMEOUT 10000
#define KISS_NTP_RETRY 4
#define KISS_TIMEZONE_OFFSET 3600  // UTC por defecto

// Ejemplos timezone:
// España: 3600 (UTC+1) o 7200 (UTC+2 verano)
// México: -21600 (UTC-6)
// Argentina: -10800 (UTC-3)

// ========== CONFIGURACIÓN SSL TELEGRAM ==========
#define KISS_SSL_CERT_VALIDATION true
#define KISS_SSL_CERT_MARGIN 86400  // 24h margen antes/después expiración

// ========== CONFIGURACIÓN SSL ESP32S3 ==========
#define KISS_SSL_RX_BUFFER_SIZE 4096
#define KISS_SSL_TX_BUFFER_SIZE 4096
#define KISS_SSL_TIMEOUT 30000
#define KISS_MIN_FREE_HEAP_SSL 60000

// ========== DEBUG mbedTLS TEMPORAL ==========
#define MBEDTLS_DEBUG_C
#define MBEDTLS_SSL_DEBUG_ALL

// Certificados en cadena raíz Let's Encrypt (ISRG Root X1)
// Válido hasta: 2035-09-30 14:01:15 GMT
// Usado por api.telegram.org
// ========== CERTIFICADO TELEGRAM - CADENA COMPLETA ==========
#define TELEGRAM_ROOT_CA \
  "-----BEGIN CERTIFICATE-----\n" \
  "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\n" \
  "-----END CERTIFICATE-----\n" \
  "-----BEGIN CERTIFICATE-----\n" \
  "MIIFBjCCAu6gAwIBAgIRAIp9PhPWLzDvI4a9KQdrNPgwDQYJKoZIhvcNAQELBQAw\n" \
  "TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh\n" \
  "cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMjQwMzEzMDAwMDAw\n" \
  "WhcNMjcwMzEyMjM1OTU5WjAzMQswCQYDVQQGEwJVUzEWMBQGA1UEChMNTGV0J3Mg\n" \
  "RW5jcnlwdDEMMAoGA1UEAxMDUjExMIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIB\n" \
  "CgKCAQEAuoe8XBsAOcvKCs3UZxD5ATylTqVhyybKUvsVAbe5KPUoHu0nsyQYOWcJ\n" \
  "DAjs4DqwO3cOvfPlOVRBDE6uQdaZdN5R2+97/1i9qLcT9t4x1fJyyXJqC4N0lZxG\n" \
  "AGQUmfOx2SLZzaiSqhwmej/+71gFewiVgdtxD4774zEJuwm+UE1fj5F2PVqdnoPy\n" \
  "6cRms+EGZkNIGIBloDcYmpuEMpexsr3E+BUAnSeI++JjF5ZsmydnS8TbKF5pwnnw\n" \
  "SVzgJFDhxLyhBax7QG0AtMJBP6dYuC/FXJuluwme8f7rsIU5/agK70XEeOtlKsLP\n" \
  "Xzze41xNG/cLJyuqC0J3U095ah2VqQIDAQABo4H4MIH1MA4GA1UdDwEB/wQEAwIB\n" \
  "hjAdBgNVHSUEFjAUBggrBgEFBQcDAgYIKwYBBQUHAwEwEgYDVR0TAQH/BAgwBgEB\n" \
  "/wIBADAdBgNVHQ4EFgQUxc9GpOr0w8B6bJXELbBeki8m47kwHwYDVR0jBBgwFoAU\n" \
  "ebRZ5nu25eQBc4AIiMgaWPbpm24wNAYIKwYBBQUHAQEEKDAmMCQGCCsGAQUFBzAB\n" \
  "hhhodHRwOi8veDEuaS5sZW5jci5vcmcvMCsGA1UdHwQkMCIwIKAeoByGGmh0dHA6\n" \
  "Ly94MS5jLmxlbmNyLm9yZy9jcmwwEwYDVR0gBAwwCjAIBgZngQwBAgEwDQYJKoZI\n" \
  "hvcNAQELBQADggIBAE7iiV0KAxyQOND1H/lxXPjDj7I3iHpvsCUf7b632IYGjukJ\n" \
  "hyzlxvTItDBPH+Ex1deTxBtvZ34t4mLaswA4FZl3gj97zxCJvHH8hD8wRKZpEPYh\n" \
  "4nG4t/f3x/GE9gLaLgH1qPGjDEA5T7QLe+L8CZf0C1lP0vJBxXYN7IqRzjLwBVIe\n" \
  "zjqUKj5WvAzXPvOeqzUTPRsaLJGk9+w8FPJHxR3WJWbKZBZgjOJp6bHKl9JhJV3O\n" \
  "RM6f8NlXE+iZNJLOy7Q2N3C+9jg+ZQHWO3s/3y8UgLRq6qkA8gTp8cBhjP5DRCB7\n" \
  "K/qhqLqy1j5wlMDvT4s/yyFPCc7DrdViLOT3lCJvZkRpOYp3RR7P8a9kT2uc1K7J\n" \
  "3ZCQsVMFlLCBKE7FjGMPmWvmqOqKVJxKclKHvxw8FJPWh1TsXCfqbfTGQOTHEEEL\n" \
  "E3DY0wkT7aHx9cJ1EyYBRj3WMJhFTbdRhKHJ6jJJJuqN2G3r/V1q7xPJxJZdDvPQ\n" \
  "LR9L9A2jO8T4NlX3p6pIPSHZKh6GfN0E+NRFJ2r0F+WzULfpzQqKnGqN0YvLLxCA\n" \
  "xB2kG0xP0iyJDLYh3MsVqBnCNhZj/+U8+qCcAjlV3D1PVmKqUhm+lmXMZqPJHlfs\n" \
  "2oqwJ6vkuLKSJpKC0xqHWDyDDELGHNLHdR2v6Y2LY0LKWRrQLhFyAqd8tKnB\n" \
  "-----END CERTIFICATE-----\n"

// ========== DETECCIÓN AUTOMÁTICA PLUGINS ==========
#define KISS_HAS_OTA true

// ========== VERSIÓN UNIFICADA ==========
// ⚠️ PARA CAMBIAR LA VERSIÓN: Edita solo esta línea ⬇️
#define KISS_SUITE_VERSION "0.9.0"
// ⚠️ Fin de configuración de versión ⬆️

// Helper para obtener versión
inline const char* KISS_GET_VERSION() {
  return KISS_SUITE_VERSION;
}

// ========== INFORMACIÓN DEL SISTEMA ==========
inline void KISS_PRINT_SYSTEM_INFO() {
  KISS_LOGF("📦 KissTelegram Suite: v%s\n", KISS_GET_VERSION());
  KISS_LOGF("💾 Filesystem: %s\n", KISS_FS_NAME);

#if KISS_HAS_OTA
  KISS_LOGF("🔧 KissOTA: [ACTIVO]\n");
#else
  KISS_LOG("🔧 KissOTA: [NO INCLUIDO]");
#endif

  KISS_LOGF("⏰ KissTime: [NTP]\n");
  KISS_LOGF("🔨 Modo: %s\n", KISS_DEVELOPMENT_MODE ? "DESARROLLO" : "PRODUCCIÓN");
  KISS_LOGF("💾 Heap libre: %d bytes\n", ESP.getFreeHeap());

  if (KISS_FS.totalBytes() > 0) {
    float usage = KISS_GET_FS_USAGE();
    KISS_LOGF("💿 %s: %.1f%% usado\n\n", KISS_FS_NAME, usage);
  }
}

// Esto es para detectar el bug conocido del IDE de arduino y Platformio cuando no actualiza la versión que estás compilando..
// Una hora viendo el mismo error, soy idiota ...  Son todos iguales...
// ========== VERIFICADOR AUTOMÁTICO DE BUILD ==========
#define BUILD_TIMESTAMP __DATE__ " " __TIME__
#define BUILD_DATE __DATE__
#define BUILD_TIME __TIME__

// Hash automático basado en timestamp de compilación al menos
inline uint32_t getBuildHash() {
  const char* ts = BUILD_TIMESTAMP;
  uint32_t hash = 5381;
  while (*ts) {
    hash = ((hash << 5) + hash) + (*ts++);
  }
  return hash;
}

// Imprimir build info en Serial
inline void KISS_PRINT_BUILD_INFO() {
  Serial.println("\n=====================================================");
  Serial.println("🔨 VERIFICADOR DE COMPILACIÓN - BUILD");
  Serial.println("=======================================================");
  Serial.printf("🕐 COMPILADO: %s\n", BUILD_TIMESTAMP);
  Serial.printf("🔢 BUILD HASH: 0x%08X\n", getBuildHash());
  Serial.printf("📦 VERSIÓN: %s\n", KISS_GET_VERSION());
  Serial.println("=======================================================");
  Serial.println("⚠️ Si la hora es la misma que la versión anterior:");
  Serial.println(" → Arduino IDE NO compiló/subió correctamente. Eliminar");
  Serial.println(" → Borrar C:/Users/name/AppData/Local/arduino/sketches");
  Serial.println(" → El sketch, ejemplo; BACHEE8B7E606F2E8B44759D16C2F4");
  Serial.println("=====================================================\n");
}

#endif  // SYSTEM_SETUP_H