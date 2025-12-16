/*
 * KissTelegram RTOS Example
 * 
 * Ejemplo avanzado usando tareas FreeRTOS para desacoplar KissTelegram
 * de la lógica de aplicación. Útil para sistemas complejos con múltiples
 * subsistemas concurrentes.
 * 
 * Arquitectura:
 * - Core 0: WiFi/Telegram (tarea telegramTask)
 * - Core 1: Lógica aplicación (tarea applicationTask)
 * - Comunicación entre tareas via Queue/Mutex
 * 
 * Vicente Soriano - KissTelegram v1.0
 */

#include "KissTelegram.h"
#include "KissCredentials.h"

#define BOT_TOKEN "YOUR_BOT_TOKEN"
#define CHAT_ID "YOUR_CHAT_ID"

// ============================================================================
// OBJETOS GLOBALES
// ============================================================================
KissCredentials credentials;
KissTelegram bot(BOT_TOKEN);

// ============================================================================
// FREERTOS HANDLES
// ============================================================================
TaskHandle_t telegramTaskHandle = NULL;
TaskHandle_t applicationTaskHandle = NULL;
SemaphoreHandle_t botMutex = NULL;
QueueHandle_t commandQueue = NULL;

// ============================================================================
// ESTRUCTURAS PARA COMUNICACIÓN INTER-TASK
// ============================================================================
struct Command {
  char cmd[32];
  char param[64];
  char chat_id[32];
};

// ============================================================================
// TELEGRAM TASK - Core 0 (WiFi core)
// ============================================================================
void telegramTask(void *parameter) {
  const char* TAG = "TelegramTask";
  
  LOGF("%s: Iniciada en Core %d\n", TAG, xPortGetCoreID());
  
  TickType_t xLastWakeTime = xTaskGetTickCount();
  const TickType_t xFrequency = pdMS_TO_TICKS(bot.getRecommendedDelay());
  
  while (1) {
    // Tomar mutex antes de acceder al bot
    if (xSemaphoreTake(botMutex, portMAX_DELAY)) {
      
      // Verificar mensajes entrantes
      bot.checkMessages([](const char* chat_id, const char* text, 
                            const char* command, const char* param) {
        LOGF("Mensaje recibido: %s %s\n", command, param);
        
        // Comandos críticos se procesan inmediatamente
        if (strcmp(command, "/estado") == 0) {
          bot.sendMessageDirect(chat_id, "Sistema operativo");
        }
        // Comandos que requieren lógica de aplicación → queue
        else {
          Command cmd;
          strncpy(cmd.cmd, command, sizeof(cmd.cmd) - 1);
          strncpy(cmd.param, param, sizeof(cmd.param) - 1);
          strncpy(cmd.chat_id, chat_id, sizeof(cmd.chat_id) - 1);
          
          // Enviar a application task
          if (xQueueSend(commandQueue, &cmd, 0) != pdPASS) {
            LOGF("ERROR: Queue llena, comando descartado\n");
          }
        }
      });
      
      // Procesar cola de mensajes salientes
      bot.processQueue();
      
      // Liberar mutex
      xSemaphoreGive(botMutex);
    }
    
    // Delay preciso usando vTaskDelayUntil
    vTaskDelayUntil(&xLastWakeTime, xFrequency);
    
    // Watchdog reset (si está habilitado)
    // esp_task_wdt_reset();
  }
}

// ============================================================================
// APPLICATION TASK - Core 1 (Application core)
// ============================================================================
void applicationTask(void *parameter) {
  const char* TAG = "AppTask";
  
  LOGF("%s: Iniciada en Core %d\n", TAG, xPortGetCoreID());
  
  // Variables locales para lógica de aplicación
  int sensorValue = 0;
  unsigned long lastSensorRead = 0;
  const unsigned long SENSOR_INTERVAL = 60000; // 1 minuto
  
  Command cmd;
  
  while (1) {
    // ========================================================================
    // PROCESAR COMANDOS DE TELEGRAM (sin bloquear)
    // ========================================================================
    if (xQueueReceive(commandQueue, &cmd, 0) == pdPASS) {
      LOGF("%s: Procesando comando %s\n", TAG, cmd.cmd);
      
      // Lógica de aplicación según comando
      if (strcmp(cmd.cmd, "/riego") == 0) {
        if (strcmp(cmd.param, "ON") == 0) {
          // Activar riego
          digitalWrite(VALVE_PIN, HIGH);
          
          // Responder via Telegram (con mutex)
          if (xSemaphoreTake(botMutex, pdMS_TO_TICKS(1000))) {
            bot.queueMessage(cmd.chat_id, "Riego activado", 
                            KissTelegram::PRIORITY_HIGH);
            xSemaphoreGive(botMutex);
          }
        }
        else if (strcmp(cmd.param, "OFF") == 0) {
          // Desactivar riego
          digitalWrite(VALVE_PIN, LOW);
          
          if (xSemaphoreTake(botMutex, pdMS_TO_TICKS(1000))) {
            bot.queueMessage(cmd.chat_id, "Riego desactivado", 
                            KissTelegram::PRIORITY_HIGH);
            xSemaphoreGive(botMutex);
          }
        }
      }
      else if (strcmp(cmd.cmd, "/sensor") == 0) {
        char response[64];
        snprintf(response, sizeof(response), 
                "Sensor actual: %d", sensorValue);
        
        if (xSemaphoreTake(botMutex, pdMS_TO_TICKS(1000))) {
          bot.queueMessage(cmd.chat_id, response, 
                          KissTelegram::PRIORITY_NORMAL);
          xSemaphoreGive(botMutex);
        }
      }
    }
    
    // ========================================================================
    // LÓGICA PERIÓDICA DE APLICACIÓN
    // ========================================================================
    unsigned long now = millis();
    if (now - lastSensorRead >= SENSOR_INTERVAL) {
      lastSensorRead = now;
      
      // Leer sensor (simulado)
      sensorValue = analogRead(SENSOR_PIN);
      
      // Si valor crítico, enviar alerta
      if (sensorValue > THRESHOLD) {
        char alert[64];
        snprintf(alert, sizeof(alert), 
                "ALERTA: Sensor = %d (crítico)", sensorValue);
        
        if (xSemaphoreTake(botMutex, pdMS_TO_TICKS(1000))) {
          bot.queueMessage(credentials.getOwnerChatID(), alert, 
                          KissTelegram::PRIORITY_CRITICAL);
          xSemaphoreGive(botMutex);
        }
      }
      
      LOGF("%s: Sensor leído = %d\n", TAG, sensorValue);
    }
    
    // Delay no crítico (permite otras tareas)
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

// ============================================================================
// SETUP
// ============================================================================
void setup() {
  Serial.begin(115200);
  delay(1000);
  
  LOGF("\n=== KissTelegram RTOS Example ===\n");
  LOGF("Cores disponibles: %d\n", portNUM_PROCESSORS);
  
  // Conectar WiFi
  WiFi.begin("SSID", "PASSWORD");
  LOGF("Conectando WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  LOGF("\nWiFi conectado: %s\n", WiFi.localIP().toString().c_str());
  
  // Inicializar credenciales
  credentials.begin();
  credentials.setOwnerChatID(CHAT_ID);
  
  // Habilitar bot
  bot.enable();
  bot.setWifiStable();
  bot.setOperationMode(KissTelegram::MODE_BALANCED);
  
  // ============================================================================
  // CREAR PRIMITIVAS RTOS
  // ============================================================================
  
  // Mutex para acceso al bot
  botMutex = xSemaphoreCreateMutex();
  if (botMutex == NULL) {
    LOGF("ERROR: No se pudo crear mutex\n");
    while(1) delay(1000);
  }
  
  // Queue para comandos (10 comandos max)
  commandQueue = xQueueCreate(10, sizeof(Command));
  if (commandQueue == NULL) {
    LOGF("ERROR: No se pudo crear queue\n");
    while(1) delay(1000);
  }
  
  // ============================================================================
  // CREAR TAREAS
  // ============================================================================
  
  // Tarea Telegram en Core 0 (WiFi core)
  // Stack: 8KB, Prioridad: 2 (media-alta)
  xTaskCreatePinnedToCore(
    telegramTask,           // Función
    "TelegramTask",         // Nombre
    8192,                   // Stack size (bytes)
    NULL,                   // Parámetro
    2,                      // Prioridad (0-24, mayor = más prioridad)
    &telegramTaskHandle,    // Handle
    0                       // Core 0 (WiFi)
  );
  
  if (telegramTaskHandle == NULL) {
    LOGF("ERROR: No se pudo crear TelegramTask\n");
    while(1) delay(1000);
  }
  
  // Tarea Aplicación en Core 1 (App core)
  // Stack: 4KB, Prioridad: 1 (media)
  xTaskCreatePinnedToCore(
    applicationTask,
    "ApplicationTask",
    4096,
    NULL,
    1,                      // Menor prioridad que Telegram
    &applicationTaskHandle,
    1                       // Core 1 (Aplicación)
  );
  
  if (applicationTaskHandle == NULL) {
    LOGF("ERROR: No se pudo crear ApplicationTask\n");
    while(1) delay(1000);
  }
  
  LOGF("Tareas RTOS creadas exitosamente\n");
  LOGF("Telegram: Core %d, Prioridad 2\n", 0);
  LOGF("App:      Core %d, Prioridad 1\n", 1);
  
  // Mensaje inicial
  bot.sendMessage(CHAT_ID, "Sistema RTOS iniciado");
}

// ============================================================================
// LOOP - Vacío (todo en tareas RTOS)
// ============================================================================
void loop() {
  // Loop vacío - toda la lógica está en tareas RTOS
  // Podrías usar este loop para monitoreo o watchdog si lo necesitas
  
  static unsigned long lastStats = 0;
  if (millis() - lastStats > 30000) { // Cada 30s
    lastStats = millis();
    
    // Imprimir estadísticas de tareas (opcional)
    LOGF("\n=== Task Stats ===\n");
    LOGF("Free Heap: %d bytes\n", ESP.getFreeHeap());
    LOGF("Telegram Task Stack: %d words free\n", 
         uxTaskGetStackHighWaterMark(telegramTaskHandle));
    LOGF("App Task Stack: %d words free\n", 
         uxTaskGetStackHighWaterMark(applicationTaskHandle));
  }
  
  delay(1000);
}

// ============================================================================
// NOTAS DE USO
// ============================================================================
/*
 * VENTAJAS ARQUITECTURA RTOS:
 * 
 * 1. DESACOPLAMIENTO
 *    - Telegram independiente de lógica aplicación
 *    - Cambios en uno no afectan al otro
 * 
 * 2. CONCURRENCIA REAL
 *    - Telegram polling en Core 0 (WiFi)
 *    - Lógica app en Core 1 (no bloquea WiFi)
 * 
 * 3. PRIORIDADES
 *    - Telegram prioridad 2 → respuesta rápida
 *    - App prioridad 1 → no interfiere con red
 * 
 * 4. ESCALABILIDAD
 *    - Añadir más tareas sin modificar existentes
 *    - Ej: OTA task, Sensor task, Display task
 * 
 * DESVENTAJAS:
 * 
 * 1. COMPLEJIDAD
 *    - Requiere entender mutex/queues
 *    - Debugging más difícil
 * 
 * 2. MEMORIA
 *    - Cada tarea tiene stack propio
 *    - ~12KB overhead para 2 tareas
 * 
 * 3. SINCRONIZACIÓN
 *    - Deadlocks posibles si mal diseñado
 *    - Siempre tomar mutex con timeout
 * 
 * CUANDO USAR RTOS:
 * 
 * ✓ Sistema con múltiples subsistemas concurrentes
 * ✓ Operaciones bloqueantes en aplicación
 * ✓ Necesitas priorizar tareas críticas
 * ✓ Tienes >200KB RAM libre
 * 
 * CUANDO USAR LOOP SIMPLE:
 * 
 * ✓ Aplicación single-purpose
 * ✓ Lógica secuencial simple
 * ✓ RAM limitada (<100KB libre)
 * ✓ Simplicidad sobre performance
 * 
 * CONFIGURACIÓN STACK SIZE:
 * 
 * - TelegramTask: 8KB (networking buffer overhead)
 * - ApplicationTask: 4KB (ajustar según complejidad)
 * - Monitorear con uxTaskGetStackHighWaterMark()
 * - Si < 500 words libres → aumentar stack
 * 
 * PRIORIDADES RECOMENDADAS:
 * 
 * - Critical/Network tasks: 2-3
 * - Application logic: 1
 * - Background tasks: 0
 * - Idle task: automática (FreeRTOS)
 */
