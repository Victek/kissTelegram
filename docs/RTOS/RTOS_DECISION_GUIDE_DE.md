# KissTelegram - RTOS vs Einfache Schleife

## Entscheidungsleitfaden: Wann welchen Ansatz verwenden?

### Einfacher Schleifen-Ansatz (Standardmäßig empfohlen)

**Typisches Beispiel:**
```cpp
void loop() {
  bot.checkMessages(handler);
  bot.processQueue();
  delay(bot.getRecommendedDelay());
}
```

**✅ Verwenden wenn:**
- Single-Purpose-Anwendung (nur Bot + einfache Logik)
- Begrenzter RAM (< 100KB frei)
- Priorität: Einfachheit und Wartbarkeit
- Team ohne RTOS-Erfahrung
- Sequentielle Logik ohne lange blockierende Operationen

**Anwendungsfall-Beispiel:**
- Einfacher Temperaturmonitor
- Fernsteuerung ein/aus
- Basis-Datenlogger
- Ereignisbenachrichtiger

---

### RTOS-Ansatz (Fortgeschritten)

**Architekturbeispiel:**
```cpp
Core 0: telegramTask()     // Netzwerk + Telegram
Core 1: applicationTask()  // Anwendungslogik
        sensorTask()       // Sensorauslesen
        displayTask()      // Lokale UI
```

**✅ Verwenden wenn:**
- Mehrere gleichzeitige Subsysteme
- Blockierende Operationen in der Anwendung
- Notwendigkeit, kritische Aufgaben zu priorisieren
- Reichlich RAM (> 200KB frei)
- Skalierbares System mit mehreren Funktionen

**Anwendungsfall-Beispiel:**
- Bewässerungssystem mit VPD + AEMET + mehreren Sensoren
- IoT-Gateway mit mehreren Protokollen
- System mit lokaler UI + Telegram + Cloud
- Anwendungen mit kritischem Timing

---

## Technischer Vergleich

| Aspekt | Einfache Schleife | RTOS |
|---------|-------------|------|
| **Code-Komplexität** | Niedrig | Mittel-Hoch |
| **RAM-Overhead** | ~0KB | ~12KB (2 Tasks) |
| **CPU-Auslastung** | Sequentiell | Echt parallel |
| **Debugging** | Einfach | Komplex |
| **Skalierbarkeit** | Begrenzt | Ausgezeichnet |
| **Antwortlatenz** | Variabel | Vorhersagbar |
| **Wartung** | Einfach | Erfordert Expertise |

---

## Hybride Architekturen

### Option 1: Schleife mit nicht-blockierenden Callbacks
```cpp
void loop() {
  bot.checkMessages(handler);
  bot.processQueue();
  
  // Nicht-blockierende Logik
  if (millis() - lastSensorRead > INTERVAL) {
    readSensors();
  }
  
  delay(10); // Yield
}
```

**Vorteile:**
- Einfachheit der Schleife
- Bessere Nebenläufigkeit als reine Schleife
- Kein RTOS-Overhead

**Einschränkungen:**
- Erfordert nicht-blockierende Disziplin
- Keine echte Priorisierung

---

### Option 2: Minimalistisches RTOS (1 zusätzliche Task)
```cpp
// Telegram in normaler loop()
void loop() {
  bot.checkMessages(handler);
  bot.processQueue();
  delay(bot.getRecommendedDelay());
}

// Nur schwere Logik in RTOS-Task
void heavyTask(void *param) {
  while(1) {
    processComplexAlgorithm();
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}
```

**Vorteile:**
- Weniger Komplexität als vollständiges RTOS
- Isoliert blockierende Operationen
- Telegram antwortet weiterhin

---

## Migration: Schleife → RTOS

**Schritt 1: Blockierende Operationen identifizieren**
```cpp
// VORHER (blockierend)
void loop() {
  bot.checkMessages(handler);
  
  int result = longCalculation(); // Blockiert 5 Sekunden
  
  bot.processQueue();
}

// NACHHER (nicht-blockierend)
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

**Schritt 2: RTOS-Primitiven hinzufügen**
- Mutex für Bot
- Queue für Kommunikation
- Task erstellen

**Schritt 3: Schrittweises Testen**
- Stack-Nutzung überwachen
- Keine Deadlocks überprüfen
- Verbrauchten RAM messen

---

## Reale Anwendungsfälle

### 1. Einfacher Monitor (Schleife)
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

### 2. Intelligentes Bewässerungssystem (RTOS)
```cpp
Core 0: telegramTask()
  - Benutzerbefehle empfangen
  - Kritische Alarme senden
  
Core 1: irrigationTask()
  - VPD berechnen
  - Mehrere Sensoren lesen
  - AEMET API abfragen
  - Adaptive Bewässerung entscheiden
  
Core 1: sensorTask()
  - BME680 kontinuierlich lesen
  - VOCs erkennen
  - ML-Inferenzen
```

### 3. IoT-Gateway (RTOS)
```cpp
Core 0: telegramTask()
  - Telegram-Netzwerk
  
Core 0: wifiTask()
  - WiFi/LTE-Fallback-Verwaltung
  
Core 1: espNowTask()
  - Von ESP-NOW-Knoten empfangen
  - Nachrichten routen
  
Core 1: dataProcessingTask()
  - Sensordaten aggregieren
  - Berichte generieren
```

---

## Leistungs-Benchmarks

### Telegram-Antwortlatenz

**Einfache Schleife:**
- Bester Fall: 50ms
- Schlechtester Fall: 5000ms (wenn App blockiert)
- Durchschnitt: 100-500ms

**RTOS (Telegram Priorität 2):**
- Bester Fall: 20ms
- Schlechtester Fall: 100ms
- Durchschnitt: 30-50ms

### Nachrichten-Durchsatz

**Einfache Schleife:**
- ~0.5-1 msg/s (abhängig von App)

**RTOS:**
- ~1-2 msg/s (unabhängig von App)
- Turbo-Modus: ~0.9 msg/s anhaltend

---

## Abschließende Empfehlung

### Für 80% der Projekte: **Einfache Schleife**
- Einfacher zu entwickeln
- Einfacher zu warten
- Ausreichende Leistung
- Weniger potenzielle Bugs

### Für 20% der Projekte: **RTOS**
- Komplexe Multi-Subsystem-Systeme
- Kritische Timing-Anforderungen
- Wichtige zukünftige Skalierbarkeit
- Team mit RTOS-Expertise

### Goldene Regel:
> "Beginnen Sie einfach. Migrieren Sie zu RTOS nur, wenn Sie **messen**, dass die 
> einfache Schleife Ihre Anforderungen nicht erfüllt."

---

## Zusätzliche Ressourcen

- **FreeRTOS ESP32 docs:** https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/freertos.html
- **KissTelegram Beispiele:** Siehe `examples/` Ordner
- **ESP32 RTOS Tutorial:** https://www.freertos.org/

---

## Kontakt

Fragen dazu, welchen Ansatz Sie für Ihr Projekt wählen sollten?

Vicente Soriano - victek@gmail.com
GitHub: https://github.com/victek/KissTelegram
