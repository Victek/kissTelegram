# KissTelegram - RTOS vs Simple Loop

## Guía de Decisión: ¿Cuándo usar cada enfoque?

### Enfoque Simple Loop (Recomendado por defecto)

**Ejemplo típico:**
```cpp
void loop() {
  bot.checkMessages(handler);
  bot.processQueue();
  delay(bot.getRecommendedDelay());
}
```

**✅ Usar cuando:**
- Aplicación single-purpose (solo bot + lógica simple)
- RAM limitada (< 100KB libre)
- Prioridad: simplicidad y mantenibilidad
- Equipo sin experiencia en RTOS
- Lógica secuencial sin operaciones bloqueantes largas

**Ejemplo caso de uso:**
- Monitor temperatura simple
- Control on/off remoto
- Logger datos básico
- Notificador de eventos

---

### Enfoque RTOS (Avanzado)

**Ejemplo arquitectura:**
```cpp
Core 0: telegramTask()     // Networking + Telegram
Core 1: applicationTask()  // Lógica aplicación
        sensorTask()       // Lectura sensores
        displayTask()      // UI local
```

**✅ Usar cuando:**
- Múltiples subsistemas concurrentes
- Operaciones bloqueantes en aplicación
- Necesitas priorizar tareas críticas
- RAM abundante (> 200KB libre)
- Sistema escalable con múltiples funciones

**Ejemplo caso de uso:**
- Sistema riego con VPD + AEMET + sensores múltiples
- Gateway IoT con múltiples protocolos
- Sistema con UI local + Telegram + cloud
- Aplicaciones con timing crítico

---

## Comparación Técnica

| Aspecto | Simple Loop | RTOS |
|---------|-------------|------|
| **Complejidad código** | Baja | Media-Alta |
| **RAM overhead** | ~0KB | ~12KB (2 tareas) |
| **CPU utilization** | Secuencial | Paralelo real |
| **Debugging** | Fácil | Complejo |
| **Escalabilidad** | Limitada | Excelente |
| **Latencia respuesta** | Variable | Predecible |
| **Mantenimiento** | Simple | Requiere expertise |

---

## Arquitecturas Híbridas

### Opción 1: Loop con callbacks non-blocking
```cpp
void loop() {
  bot.checkMessages(handler);
  bot.processQueue();
  
  // Lógica non-blocking
  if (millis() - lastSensorRead > INTERVAL) {
    readSensors();
  }
  
  delay(10); // Yield
}
```

**Ventajas:**
- Simplicidad del loop
- Mejor concurrencia que loop puro
- Sin overhead RTOS

**Limitaciones:**
- Requiere disciplina non-blocking
- No hay priorización real

---

### Opción 2: RTOS minimalista (1 tarea extra)
```cpp
// Telegram en loop() normal
void loop() {
  bot.checkMessages(handler);
  bot.processQueue();
  delay(bot.getRecommendedDelay());
}

// Solo lógica pesada en tarea RTOS
void heavyTask(void *param) {
  while(1) {
    processComplexAlgorithm();
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}
```

**Ventajas:**
- Menor complejidad que RTOS completo
- Isola operaciones bloqueantes
- Telegram sigue respondiendo

---

## Migración: Loop → RTOS

**Paso 1: Identificar operaciones bloqueantes**
```cpp
// ANTES (bloqueante)
void loop() {
  bot.checkMessages(handler);
  
  int result = longCalculation(); // Bloquea 5 segundos
  
  bot.processQueue();
}

// DESPUÉS (non-blocking)
void loop() {
  bot.checkMessages(handler);
  bot.processQueue();
  delay(10);
}

void calculationTask(void *param) {
  while(1) {
    int result = longCalculation();
    vTaskDelay(pdMS_TO_TICKS(5000));
  }
}
```

**Paso 2: Añadir primitivas RTOS**
- Mutex para bot
- Queue para comunicación
- Crear tarea

**Paso 3: Testing gradual**
- Monitorear stack usage
- Verificar no hay deadlocks
- Medir RAM consumida

---

## Casos de Uso Reales

### 1. Monitor Simple (Loop)
```cpp
void loop() {
  bot.checkMessages(handler);
  
  if (millis() - last > 60000) {
    float temp = bme.readTemperature();
    bot.queueMessage(CHAT_ID, String(temp));
    last = millis();
  }
  
  bot.processQueue();
  delay(bot.getRecommendedDelay());
}
```

### 2. Sistema Riego Inteligente (RTOS)
```cpp
Core 0: telegramTask()
  - Recibe comandos usuario
  - Envía alertas críticas
  
Core 1: irrigationTask()
  - Calcula VPD
  - Lee múltiples sensores
  - Consulta AEMET API
  - Decide riego adaptativo
  
Core 1: sensorTask()
  - Lee BME680 continuamente
  - Detecta VOCs
  - Inferencias ML
```

### 3. Gateway IoT (RTOS)
```cpp
Core 0: telegramTask()
  - Telegram networking
  
Core 0: wifiTask()
  - Gestión WiFi/LTE fallback
  
Core 1: espNowTask()
  - Recibe de nodos ESP-NOW
  - Enruta mensajes
  
Core 1: dataProcessingTask()
  - Agrega datos sensores
  - Genera reportes
```

---

## Performance Benchmarks

### Latencia respuesta Telegram

**Simple Loop:**
- Mejor caso: 50ms
- Peor caso: 5000ms (si app bloquea)
- Promedio: 100-500ms

**RTOS (Telegram prioridad 2):**
- Mejor caso: 20ms
- Peor caso: 100ms
- Promedio: 30-50ms

### Throughput mensajes

**Simple Loop:**
- ~0.5-1 msg/s (depende de app)

**RTOS:**
- ~1-2 msg/s (independiente de app)
- Turbo mode: ~0.9 msg/s sostenido

---

## Recomendación Final

### Para 80% de proyectos: **Simple Loop**
- Más fácil desarrollar
- Más fácil mantener
- Suficiente performance
- Menos bugs potenciales

### Para 20% de proyectos: **RTOS**
- Sistemas complejos multisubsistema
- Requisitos timing críticos
- Escalabilidad futura importante
- Equipo con expertise RTOS

### Regla de oro:
> "Start simple. Migrate to RTOS only when you **measure** that simple loop 
> is not meeting your requirements."

---

## Recursos Adicionales

- **FreeRTOS ESP32 docs:** https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/freertos.html
- **KissTelegram examples:** Ver carpeta `examples/`
- **ESP32 RTOS tutorial:** https://www.freertos.org/

---

## Contacto

¿Dudas sobre qué enfoque elegir para tu proyecto?

Vicente Soriano - victek@gmail.com
GitHub: https://github.com/victek/KissTelegram
