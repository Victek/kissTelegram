# KissTelegram - RTOS vs Loop Semplice

## Guida alla Decisione: Quando usare quale approccio?

### Approccio Loop Semplice (Raccomandato per impostazione predefinita)

**Esempio tipico:**
```cpp
void loop() {
  bot.checkMessages(handler);
  bot.processQueue();
  delay(bot.getRecommendedDelay());
}
```

**✅ Usare quando:**
- Applicazione single-purpose (solo bot + logica semplice)
- RAM limitata (< 100KB libera)
- Priorità: semplicità e manutenibilità
- Team senza esperienza RTOS
- Logica sequenziale senza operazioni bloccanti lunghe

**Esempio caso d'uso:**
- Monitor temperatura semplice
- Controllo remoto on/off
- Logger dati di base
- Notificatore eventi

---

### Approccio RTOS (Avanzato)

**Esempio architettura:**
```cpp
Core 0: telegramTask()     // Networking + Telegram
Core 1: applicationTask()  // Logica applicazione
        sensorTask()       // Lettura sensori
        displayTask()      // UI locale
```

**✅ Usare quando:**
- Più sottosistemi concorrenti
- Operazioni bloccanti nell'applicazione
- Necessità di prioritizzare task critici
- RAM abbondante (> 200KB libera)
- Sistema scalabile con funzioni multiple

**Esempio caso d'uso:**
- Sistema irrigazione con VPD + AEMET + sensori multipli
- Gateway IoT con protocolli multipli
- Sistema con UI locale + Telegram + cloud
- Applicazioni con timing critico

---

## Confronto Tecnico

| Aspetto | Loop Semplice | RTOS |
|---------|-------------|------|
| **Complessità codice** | Bassa | Media-Alta |
| **Overhead RAM** | ~0KB | ~12KB (2 task) |
| **Utilizzo CPU** | Sequenziale | Parallelo reale |
| **Debugging** | Facile | Complesso |
| **Scalabilità** | Limitata | Eccellente |
| **Latenza risposta** | Variabile | Prevedibile |
| **Manutenzione** | Semplice | Richiede expertise |

---

## Architetture Ibride

### Opzione 1: Loop con callback non-bloccanti
```cpp
void loop() {
  bot.checkMessages(handler);
  bot.processQueue();
  
  // Logica non-bloccante
  if (millis() - lastSensorRead > INTERVAL) {
    readSensors();
  }
  
  delay(10); // Yield
}
```

**Vantaggi:**
- Semplicità del loop
- Migliore concorrenza del loop puro
- Nessun overhead RTOS

**Limitazioni:**
- Richiede disciplina non-bloccante
- Nessuna prioritizzazione reale

---

### Opzione 2: RTOS minimalista (1 task extra)
```cpp
// Telegram in loop() normale
void loop() {
  bot.checkMessages(handler);
  bot.processQueue();
  delay(bot.getRecommendedDelay());
}

// Solo logica pesante in task RTOS
void heavyTask(void *param) {
  while(1) {
    processComplexAlgorithm();
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}
```

**Vantaggi:**
- Meno complessità del RTOS completo
- Isola operazioni bloccanti
- Telegram continua a rispondere

---

## Migrazione: Loop → RTOS

**Passo 1: Identificare operazioni bloccanti**
```cpp
// PRIMA (bloccante)
void loop() {
  bot.checkMessages(handler);
  
  int result = longCalculation(); // Blocca 5 secondi
  
  bot.processQueue();
}

// DOPO (non-bloccante)
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

**Passo 2: Aggiungere primitive RTOS**
- Mutex per bot
- Queue per comunicazione
- Creare task

**Passo 3: Test graduale**
- Monitorare utilizzo stack
- Verificare assenza deadlock
- Misurare RAM consumata

---

## Casi d'Uso Reali

### 1. Monitor Semplice (Loop)
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

### 2. Sistema Irrigazione Intelligente (RTOS)
```cpp
Core 0: telegramTask()
  - Ricevere comandi utente
  - Inviare alert critici
  
Core 1: irrigationTask()
  - Calcolare VPD
  - Leggere sensori multipli
  - Consultare API AEMET
  - Decidere irrigazione adattiva
  
Core 1: sensorTask()
  - Leggere BME680 continuamente
  - Rilevare VOC
  - Inferenze ML
```

### 3. Gateway IoT (RTOS)
```cpp
Core 0: telegramTask()
  - Networking Telegram
  
Core 0: wifiTask()
  - Gestione fallback WiFi/LTE
  
Core 1: espNowTask()
  - Ricevere da nodi ESP-NOW
  - Instradare messaggi
  
Core 1: dataProcessingTask()
  - Aggregare dati sensori
  - Generare report
```

---

## Benchmark Prestazioni

### Latenza risposta Telegram

**Loop Semplice:**
- Caso migliore: 50ms
- Caso peggiore: 5000ms (se app blocca)
- Media: 100-500ms

**RTOS (Telegram priorità 2):**
- Caso migliore: 20ms
- Caso peggiore: 100ms
- Media: 30-50ms

### Throughput messaggi

**Loop Semplice:**
- ~0.5-1 msg/s (dipende dall'app)

**RTOS:**
- ~1-2 msg/s (indipendente dall'app)
- Modalità turbo: ~0.9 msg/s sostenuto

---

## Raccomandazione Finale

### Per l'80% dei progetti: **Loop Semplice**
- Più facile da sviluppare
- Più facile da mantenere
- Prestazioni sufficienti
- Meno bug potenziali

### Per il 20% dei progetti: **RTOS**
- Sistemi multi-sottosistema complessi
- Requisiti timing critici
- Scalabilità futura importante
- Team con expertise RTOS

### Regola d'oro:
> "Inizia semplice. Migra a RTOS solo quando **misuri** che il loop semplice 
> non soddisfa i tuoi requisiti."

---

## Risorse Aggiuntive

- **FreeRTOS ESP32 docs:** https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/freertos.html
- **Esempi KissTelegram:** Vedi cartella `examples/`
- **Tutorial ESP32 RTOS:** https://www.freertos.org/

---

## Contatto

Domande su quale approccio scegliere per il tuo progetto?

Vicente Soriano - victek@gmail.com
GitHub: https://github.com/victek/KissTelegram
