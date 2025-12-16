/*
 * KissTelegram - Minimal RTOS Example
 * 
 * Ejemplo minimalista que muestra solo lo esencial de RTOS.
 * Ideal para empezar a experimentar con tareas concurrentes.
 * 
 * Concepto: 
 * - Loop() normal para Telegram (Core 0)
 * - 1 tarea extra para lógica pesada (Core 1)
 * 
 * Vicente Soriano - KissTelegram
 */

#include "KissTelegram.h"
#include "KissCredentials.h"

#define BOT_TOKEN "YOUR_BOT_TOKEN"
#define CHAT_ID "YOUR_CHAT_ID"

KissCredentials credentials;
KissTelegram bot(BOT_TOKEN);

// Variable compartida entre loop() y task (protegida con volatile)
volatile int sensorValue = 0;

// ============================================================================
// TAREA BACKGROUND - Lógica pesada sin bloquear Telegram
// ============================================================================
void backgroundTask(void *parameter) {
  LOGF("Background Task iniciada en Core %d\n", xPortGetCoreID());
  
  while (1) {
    // Simular operación pesada (ej: cálculo complejo, lectura sensores lentos)
    delay(100);  // Simulación
    sensorValue = analogRead(34);  // Leer sensor
    
    // Si valor crítico, enviar alerta (acceso directo al bot es thread-safe)
    if (sensorValue > 3000) {
      bot.queueMessage(CHAT_ID, "⚠️ Alerta: Sensor crítico!", 
                      KissTelegram::PRIORITY_HIGH);
    }
    
    // Esta tarea se ejecuta cada 5 segundos
    vTaskDelay(pdMS_TO_TICKS(5000));
  }
}

// ============================================================================
// HANDLER MENSAJES TELEGRAM
// ============================================================================
void messageHandler(const char* chat_id, const char* text,
                    const char* command, const char* param) {
  
  if (strcmp(command, "/start") == 0) {
    bot.sendMessage(chat_id, "Bot RTOS activo!\n/sensor - Ver valor sensor");
  }
  
  else if (strcmp(command, "/sensor") == 0) {
    char msg[64];
    snprintf(msg, sizeof(msg), "Sensor actual: %d", sensorValue);
    bot.sendMessage(chat_id, msg);
  }
  
  else if (strcmp(command, "/status") == 0) {
    char msg[128];
    snprintf(msg, sizeof(msg), 
            "Core 0 (loop): %d MHz\nCore 1 (task): %d MHz\nRAM libre: %d KB",
            ESP.getCpuFreqMHz(), ESP.getCpuFreqMHz(), ESP.getFreeHeap()/1024);
    bot.sendMessage(chat_id, msg);
  }
}

// ============================================================================
// SETUP
// ============================================================================
void setup() {
  Serial.begin(115200);
  delay(1000);
  
  LOGF("\n=== KissTelegram Minimal RTOS ===\n");
  
  // Conectar WiFi
  WiFi.begin("SSID", "PASSWORD");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  LOGF("\nWiFi OK\n");
  
  // Inicializar bot
  credentials.begin();
  credentials.setOwnerChatID(CHAT_ID);
  bot.enable();
  bot.setWifiStable();
  
  // Crear tarea background en Core 1
  xTaskCreatePinnedToCore(
    backgroundTask,      // Función
    "Background",        // Nombre
    4096,               // Stack (4KB)
    NULL,               // Parámetro
    1,                  // Prioridad
    NULL,               // No necesitamos handle
    1                   // Core 1
  );
  
  LOGF("Tarea background creada en Core 1\n");
  bot.sendMessage(CHAT_ID, "Sistema RTOS iniciado ✓");
}

// ============================================================================
// LOOP - Telegram en Core 0 (loop normal)
// ============================================================================
void loop() {
  // Telegram se ejecuta en loop() normal
  // La tarea background corre en paralelo sin bloquearlo
  
  bot.checkMessages(messageHandler);
  bot.processQueue();
  delay(bot.getRecommendedDelay());
}

/* 
 * EXPLICACIÓN:
 * 
 * 1. loop() maneja Telegram (Core 0)
 * 2. backgroundTask() maneja lógica pesada (Core 1)
 * 3. Ambos corren en PARALELO sin bloquearse
 * 
 * VENTAJAS:
 * - Telegram siempre responde rápido
 * - Operaciones pesadas no bloquean bot
 * - Código simple, fácil de entender
 * 
 * THREAD SAFETY:
 * - bot.queueMessage() es thread-safe (puede llamarse desde cualquier tarea)
 * - Variables compartidas usar 'volatile' o mutex si son complejas
 * 
 * NEXT STEPS:
 * - Añadir más tareas según necesites
 * - Usar mutex si compartes estructuras complejas
 * - Ver RTOS_DECISION_GUIDE.md para casos avanzados
 */
