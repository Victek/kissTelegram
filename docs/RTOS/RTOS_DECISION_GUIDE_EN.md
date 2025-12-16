# KissTelegram - RTOS vs Simple Loop

## Decision Guide: When to use each approach?

### Simple Loop Approach (Recommended by default)

**Typical example:**
```cpp
void loop() {
  bot.checkMessages(handler);
  bot.processQueue();
  delay(bot.getRecommendedDelay());
}
```

**✅ Use when:**
- Single-purpose application (only bot + simple logic)
- Limited RAM (< 100KB free)
- Priority: simplicity and maintainability
- Team without RTOS experience
- Sequential logic without long blocking operations

**Use case example:**
- Simple temperature monitor
- Remote on/off control
- Basic data logger
- Event notifier

---

### RTOS Approach (Advanced)

**Architecture example:**
```cpp
Core 0: telegramTask()     // Networking + Telegram
Core 1: applicationTask()  // Application logic
        sensorTask()       // Sensor reading
        displayTask()      // Local UI
```

**✅ Use when:**
- Multiple concurrent subsystems
- Blocking operations in application
- Need to prioritize critical tasks
- Abundant RAM (> 200KB free)
- Scalable system with multiple functions

**Use case example:**
- Irrigation system with VPD + AEMET + multiple sensors
- IoT gateway with multiple protocols
- System with local UI + Telegram + cloud
- Applications with critical timing

---

## Technical Comparison

| Aspect | Simple Loop | RTOS |
|---------|-------------|------|
| **Code complexity** | Low | Medium-High |
| **RAM overhead** | ~0KB | ~12KB (2 tasks) |
| **CPU utilization** | Sequential | Real parallel |
| **Debugging** | Easy | Complex |
| **Scalability** | Limited | Excellent |
| **Response latency** | Variable | Predictable |
| **Maintenance** | Simple | Requires expertise |

---

## Hybrid Architectures

### Option 1: Loop with non-blocking callbacks
```cpp
void loop() {
  bot.checkMessages(handler);
  bot.processQueue();
  
  // Non-blocking logic
  if (millis() - lastSensorRead > INTERVAL) {
    readSensors();
  }
  
  delay(10); // Yield
}
```

**Advantages:**
- Loop simplicity
- Better concurrency than pure loop
- No RTOS overhead

**Limitations:**
- Requires non-blocking discipline
- No real prioritization

---

### Option 2: Minimalist RTOS (1 extra task)
```cpp
// Telegram in normal loop()
void loop() {
  bot.checkMessages(handler);
  bot.processQueue();
  delay(bot.getRecommendedDelay());
}

// Only heavy logic in RTOS task
void heavyTask(void *param) {
  while(1) {
    processComplexAlgorithm();
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}
```

**Advantages:**
- Less complexity than full RTOS
- Isolates blocking operations
- Telegram keeps responding

---

## Migration: Loop → RTOS

**Step 1: Identify blocking operations**
```cpp
// BEFORE (blocking)
void loop() {
  bot.checkMessages(handler);
  
  int result = longCalculation(); // Blocks 5 seconds
  
  bot.processQueue();
}

// AFTER (non-blocking)
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

**Step 2: Add RTOS primitives**
- Mutex for bot
- Queue for communication
- Create task

**Step 3: Gradual testing**
- Monitor stack usage
- Verify no deadlocks
- Measure RAM consumed

---

## Real Use Cases

### 1. Simple Monitor (Loop)
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

### 2. Smart Irrigation System (RTOS)
```cpp
Core 0: telegramTask()
  - Receive user commands
  - Send critical alerts
  
Core 1: irrigationTask()
  - Calculate VPD
  - Read multiple sensors
  - Query AEMET API
  - Decide adaptive irrigation
  
Core 1: sensorTask()
  - Read BME680 continuously
  - Detect VOCs
  - ML inferences
```

### 3. IoT Gateway (RTOS)
```cpp
Core 0: telegramTask()
  - Telegram networking
  
Core 0: wifiTask()
  - WiFi/LTE fallback management
  
Core 1: espNowTask()
  - Receive from ESP-NOW nodes
  - Route messages
  
Core 1: dataProcessingTask()
  - Aggregate sensor data
  - Generate reports
```

---

## Performance Benchmarks

### Telegram response latency

**Simple Loop:**
- Best case: 50ms
- Worst case: 5000ms (if app blocks)
- Average: 100-500ms

**RTOS (Telegram priority 2):**
- Best case: 20ms
- Worst case: 100ms
- Average: 30-50ms

### Message throughput

**Simple Loop:**
- ~0.5-1 msg/s (depends on app)

**RTOS:**
- ~1-2 msg/s (independent of app)
- Turbo mode: ~0.9 msg/s sustained

---

## Final Recommendation

### For 80% of projects: **Simple Loop**
- Easier to develop
- Easier to maintain
- Sufficient performance
- Fewer potential bugs

### For 20% of projects: **RTOS**
- Complex multi-subsystem systems
- Critical timing requirements
- Important future scalability
- Team with RTOS expertise

### Golden rule:
> "Start simple. Migrate to RTOS only when you **measure** that simple loop 
> is not meeting your requirements."

---

## Additional Resources

- **FreeRTOS ESP32 docs:** https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/freertos.html
- **KissTelegram examples:** See `examples/` folder
- **ESP32 RTOS tutorial:** https://www.freertos.org/

---

## Contact

Questions about which approach to choose for your project?

Vicente Soriano - victek@gmail.com
GitHub: https://github.com/victek/KissTelegram
