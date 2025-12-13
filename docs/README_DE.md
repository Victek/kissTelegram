# KissTelegram

![License](https://img.shields.io/badge/license-MIT-blue.svg)
![Platform](https://img.shields.io/badge/platform-ESP32--S3-orange.svg)
![Version](https://img.shields.io/badge/version-1.x-green.svg)
![Language](https://img.shields.io/badge/languages-7-brightgreen.svg)

**Deutsch** | [Dokumentation](GETTING_STARTED_DE.md) | [Benchmarks](BENCHMARK.md)

---

> 🚨 **ERSTE VERWENDUNG MIT ESP32-S3 UND KISSTELEGRAM?**
> **LESEN SIE DIES ZUERST:** [**GETTING_STARTED_DE.md**](GETTING_STARTED_DE.md) ⚠️
> ESP32-S3 erfordert einen **zweistufigen Upload-Prozess** aufgrund benutzerdefinierter Partitionen. Das Überspringen dieser Anleitung führt zu Boot-Fehlern!

---

## Eine robuste, unternehmensgerechte Telegram-Bot-Bibliothek für ESP32-S3

KissTelegram ist die **einzige ESP32-Telegram-Bibliothek**, die von Grund auf für Mission-Critical-Anwendungen entwickelt wurde. Im Gegensatz zu anderen Bibliotheken, die auf der Arduino-`String`-Klasse basieren (verursacht Speicherfragmentierung und Speicherlecks), verwendet KissTelegram reine `char[]`-Arrays für absolut zuverlässige Stabilität.

### Warum KissTelegram?

- Müde von verlorenen Projekten durch schwache Bibliotheken, Speicherlecks, Last-Minute-Lösungen, mangelnde Unterstützung, leere Versprechungen, Neustarts....

- Meine Vision, jetzt die Fakten:

- **Null Nachrichtenverlust**: Persistente LittleFS-Warteschlange übersteht Abstürze, Neustarts und WiFi-Fehler
- **Keine Speicherlecks**: Reine `char[]`-Implementierung, keine String-Fragmentierung
- **SSL/TLS-Sicherheit**: Sichere Verbindungen zur Telegram-API mit Zertifikatvalidierung
- **intelligente Energieverwaltung**: 6 Energiesparmodi (BOOT, LOW, IDLE, ACTIVE, TURBO, MAINTENANCE)
- **Nachrichtenpriorisierungen**: CRITICAL, HIGH, NORMAL, LOW mit intelligenter Warteschlangenverwaltung
- **Turbo-Modus**: Batch-Verarbeitung für große Nachrichtenwarteschlangen (0,9 msg/s)
- **Mehrsprachige i18n**: Kompilierzeit-Sprachauswahl (7 Sprachen, kein Laufzeit-Overhead)
- **Enterprise-OTA**: Dual-Boot-Firmware-Updates mit automatischem Rollback und Sicherheits-Gateway
- **100% Flash-Auslastung**: Benutzerdefiniertes Partitionsschema maximiert den 16MB-Flash des ESP32-S3
- **Sicherer als Espressif OTA**: PIN/PUK-Authentifizierung, Checksum-Verifizierung, 60-sekündiges Validierungsfenster
- **Unabhängig von externen Bibliotheken**: Alles von Grund auf selbst entwickelt, eigener JSON-Parser.

---

## Hardware-Anforderungen

- **ESP32-S3** mit **16MB Flash** / **8MB PSRAM**
- WiFi-Konnektivität
- Arduino IDE oder PlatformIO

---

## Installation

### Arduino IDE

1. Dieses Repository als ZIP herunterladen
2. Arduino IDE öffnen → Sketch → Include Library → Add .ZIP Library
3. Die heruntergeladene Datei auswählen

### PlatformIO

Fügen Sie zu Ihrer `platformio.ini` hinzu:

```ini
lib_deps =
    https://github.com/victek/KissTelegram.git
```

---

## Benutzerdefiniertes Partitionsschema

KissTelegram enthält ein optimiertes `partitions.csv`, das die Flash-Nutzung maximiert:

```csv
# Name,   Type, SubType, Offset,  Size
nvs,      data, nvs,     0x9000,  0x4000,
otadata,  data, ota,     0xd000,  0x2000,
app0,     app,  factory, 0x10000, 0x180000,   # 1.5MB
app1,     app,  ota_0,   0x190000,0x180000,   # 1.5MB
spiffs,   data, spiffs,  0x310000,0xCF0000,   # 13MB!
```

**13MB SPIFFS-Speicher** - das sind 8MB mehr als Espressifs Standard-Schemas!

Um dieses Partitionsschema zu verwenden:
1. Kopieren Sie `partitions.csv` in Ihr Projektverzeichnis
2. In Arduino IDE: Tools → Partition Scheme → Custom
3. In PlatformIO: `board_build.partitions = partitions.csv`

---

## Schnelleinstieg

### Grundlegendes Beispiel

```cpp
#include "KissTelegram.h"
#include "KissCredentials.h"

#define BOT_TOKEN "YOUR_BOT_TOKEN"
#define CHAT_ID "YOUR_CHAT_ID"

KissCredentials credentials;
KissTelegram bot(BOT_TOKEN);

void messageHandler(const char* chat_id, const char* text,
                    const char* command, const char* param) {
  if (strcmp(command, "/start") == 0) {
    bot.sendMessage(chat_id, "Hello! I'm alive!");
  }
}

void setup() {
  Serial.begin(115200);

  // Connect WiFi
  WiFi.begin("SSID", "PASSWORD");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  // Initialize credentials
  credentials.begin();
  credentials.setOwnerChatID(CHAT_ID);

  // Enable bot
  bot.enable();
  bot.setWifiStable();
}

void loop() {
  bot.checkMessages(messageHandler);
  bot.processQueue();
  delay(bot.getRecommendedDelay());
}
```

### OTA-Updates Beispiel

```cpp
#include "KissOTA.h"

KissOTA* otaManager;

void fileReceivedCallback(const char* file_id, size_t file_size,
                          const char* file_name) {
  if (otaManager && strstr(file_name, ".bin")) {
    otaManager->processReceivedFile(file_id, file_size, file_name);
  }
}

void setup() {
  // ... WiFi and bot setup ...

  // Initialize OTA
  otaManager = new KissOTA(&bot, &credentials);
  bot.onFileReceived(fileReceivedCallback);
}

void loop() {
  bot.checkMessages(messageHandler);
  bot.processQueue();

  if (otaManager) {
    otaManager->loop();
  }

  delay(bot.getRecommendedDelay());
}
```

**OTA-Prozess:**
1. Senden Sie `/ota` an Ihren Bot
2. Geben Sie PIN mit `/otapin IHR_PIN` ein
3. Laden Sie die `.bin`-Firmware-Datei hoch
4. Der Bot überprüft die Checksumme automatisch
5. Bestätigen Sie mit `/otaconfirm`
6. Nach dem Neustart validieren Sie mit `/otaok` innerhalb von 60 Sekunden
7. Automatisches Rollback bei Validierungsfehlschlag!

- Lesen Sie Readme_KissOTA.md in Ihrer bevorzugten Sprache, um mehr über die Lösung zu erfahren.

---

## Wichtigste Features erklärt

### 1. Persistente Nachrichtenwarteschlange

Nachrichten werden in LittleFS mit automatischem Batch-Löschen gespeichert:

```cpp
bot.queueMessage(chat_id, "Important message", KissTelegram::PRIORITY_HIGH);
```

- Übersteht Abstürze, WiFi-Unterbrechungen, Neustarts
- Wiederholte Versuche bei fehlgeschlagenen Sendevorgängen
- Intelligentes Batch-Löschen (alle 10 Nachrichten + bei leerer Warteschlange)
- Garantie für Null Nachrichtenverlust

### 2. Energieverwaltung

6 intelligente Energiesparmodi passen sich den Anforderungen Ihrer Anwendung an:

```cpp
bot.setPowerMode(KissTelegram::POWER_ACTIVE);
bot.setPowerConfig(30, 60, 10); // idle, decay, boot stable times
```

- **POWER_BOOT**: Initiale Startphase (10s)
- **POWER_LOW**: Minimale Aktivität, langsames Polling
- **POWER_IDLE**: Keine kürzliche Aktivität, reduzierte Überprüfungen
- **POWER_ACTIVE**: Normaler Betrieb
- **POWER_TURBO**: Hochgeschwindigkeits-Batch-Verarbeitung (50ms-Intervalle)
- **POWER_MAINTENANCE**: Manueller Überschreibung für Updates
- **Decay Timing für sanfte Umschaltung**

### 3. Nachrichtenpriorisierungen

Vier Prioritätsstufen stellen sicher, dass kritische Nachrichten zuerst gesendet werden:

```cpp
bot.sendMessage(chat_id, "Normal message", KissTelegram::PRIORITY_NORMAL);
bot.sendMessage(chat_id, "Alert!", KissTelegram::PRIORITY_CRITICAL);
```

Warteschlange verarbeitet: **CRITICAL → HIGH → NORMAL → LOW**
Interne Prozesse: **OTAMODE → MAINTENANCEMODE**

### 4. SSL/TLS-Sicherheit

Sichere Verbindungen mit Zertifikatvalidierung:

```cpp
bool secure = bot.isSSLSecure();
String sslInfo = bot.getSSLInfo();
bot.testSSLConnection();
```

- Automatisches sicheres/unsicheres Fallback
- Regelmäßige Ping-Überprüfungen zur Verbindungsaufrechterhaltung
- Wiederverwendbar Code speichert Verbindungs-Header für höchsten Durchsatz

### 5. Turbo-Modus

Aktiviert sich automatisch, wenn die Warteschlange > 100 Nachrichten enthält:

```cpp
bot.enableTurboMode();  // Manuelle Aktivierung
```

- Verarbeitet 10 Nachrichten pro Zyklus
- 50ms-Intervalle zwischen Batches
- Erreicht 15-20 msg/s Durchsatz
- Deaktiviert sich automatisch, wenn die Warteschlange leer ist

### 6. Betriebsmodi

Vorkonfigurierte Profile für verschiedene Szenarien:

```cpp
bot.setOperationMode(KissTelegram::MODE_RELIABILITY);
```

- **MODE_BALANCED**: Standard (Polling: 10s, Wiederholung: 3)
- **MODE_PERFORMANCE**: Schnell (Polling: 5s, Wiederholung: 2)
- **MODE_POWERSAVE**: Langsam (Polling: 30s, Wiederholung: 2)
- **MODE_RELIABILITY**: Robust (Polling: 15s, Wiederholung: 5)

### 7. Diagnosen

Umfassende Überwachung und Fehlerbehebung:

```cpp
bot.printDiagnostics();
bot.printStorageStatus();
bot.printPowerStatistics();
bot.printSystemStatus();
```

Zeigt an:
- Freier Speicher (Heap/PSRAM)
- Nachrichtenwarteschlangen-Statistiken
- Verbindungsqualität
- Energiemodus-Verlauf
- Speicherauslastung
- Betriebszeit

---

## 8. WiFi-Verwaltung
- Integrierter WiFi-Manager triggert andere Aufgaben, bis WiFi stabil ist
- Verhindert Race Conditions
- Behauptet, dass laufende Nachrichten bis zur Wiederherstellung der Verbindung im Dateisystem gespeichert werden, kann bis zu 3500 Nachrichten speichern (Standard, aber leicht erweiterbar)
- Verbindungsqualitätsüberwachung (EXCELLENT, GOOD, FAIR, POOR, DEAD) und RSSI-Ausgabepegel
- Sie müssen sich nur um Ihren Sketch kümmern, nutzen Sie KissTelegram für den Rest

---

## 9. Killer-Feature: `/estado`-Befehl

**Das mächtigste Debugging-Tool, das Sie jemals verwenden werden:**

Senden Sie `/estado` an Ihren Bot und erhalten Sie in Sekunden einen **vollständigen Gesundheitsbericht**:

```
📦 KissTelegram v1.x.x
🎯 SYSTEM-ZUVERLÄSSIGKEIT
✅ System: RELIABLE
✅ Gesendete Nachrichten: 5.234
💾 Ausstehende Nachrichten: 0
📡 WiFi-Signal: -59 dBm (Gut)
🔌 WiFi-Neukonnexionen: 2
⏱️ Betriebszeit: 86.400 Sekunden (24h)
💾 Freier Speicher: 223 KB
📊 Warteschlangen-Statistiken: Alle Systeme funktionsfähig
```

**Warum `/estado` ist unverzichtbar:**
- ✅ Sofortige Systemgesundheitsprüfung
- ✅ WiFi-Qualitätsüberwachung (Diagnosekonnektivitätsprobleme)
- ✅ Speicherleck-Erkennung (beobachten Sie freien Heap)
- ✅ Nachrichtenwarteschlangen-Status (ausstehende/fehlgeschlagene Nachrichten anzeigen)
- ✅ Betriebszeit-Tracking (Stabilitätsüberwachung)
- ✅ Verfügbar in 7 Sprachen
- ✅ Ihr erstes Tool beim Beheben von Problemen

**Pro-Tipp:** Machen Sie `/estado` zu Ihrer ersten Nachricht nach jedem Firmware-Update, um zu überprüfen, ob alles funktioniert!

---

## 10. NTP
- Eigener Code zum Synchronisieren/Resynchronisieren für SSL und Scheduler (Enterprise Edition)
---

## 11. Dokumentation (7 Sprachen)

- **[GETTING_STARTED_DE.md](GETTING_STARTED_DE.md)** - **BEGINNEN SIE HIER!** Vollständiger Leitfaden von der Erstinbetriebnahme des ESP32-S3 bis zur ersten Telegram-Nachricht
- **[README_DE.md](README_DE.md)** (diese Datei) - Feature-Übersicht, Schnelleinstieg, API-Referenz
- **[BENCHMARK.md](BENCHMARK.md)** - Technischer Vergleich mit 6 anderen Telegram-Bibliotheken (nur Englisch)
- **[README_KissOTA_XX.md](README_KissOTA_DE.md)** - OTA-Update-System-Dokumentation (7 Sprachen: EN, ES, FR, IT, DE, PT, CN)

**Wählen Sie Ihre Sprache:** Alle KissTelegram-Systemmeldungen unterstützen 7 Sprachen durch Kompilierzeit-Auswahl.


## OTA-Sicherheitsvorteile

KissTelegramm OTA ist **sicherer als Espressifs Implementierung**:

| Feature | KissTelegram OTA | Espressif OTA |
|---------|------------------|---------------|
| Authentifizierung | PIN + PUK | Keine |
| Checksum-Überprüfung | CRC32 automatisch | Manuell |
| Sicherung & Rollback | Automatisch | Manuell |
| Validierungsfenster | 60s mit `/otaok` | Keine |
| Boot-Loop-Erkennung | Ja | Nein |
| Telegram-Integration | Nativ | Benötigt benutzerdefinierten Code |
| Flash-Optimierung | 13MB SPIFFS | 5MB SPIFFS |

---

## API-Referenz

### Initialisierung

```cpp
KissTelegram(const char* token);
void enable();
void disable();
```

### Messaging

```cpp
bool sendMessage(const char* chat_id, const char* text,
                 MessagePriority priority = PRIORITY_NORMAL);
bool sendMessageDirect(const char* chat_id, const char* text);
bool queueMessage(const char* chat_id, const char* text,
                  MessagePriority priority = PRIORITY_NORMAL);
void processQueue();
```

### Konfiguration

```cpp
void setMinMessageInterval(int milliseconds);
void setMaxMessageSize(int size);
void setMaxRetryAttempts(int attempts);
void setPollingTimeout(int seconds);
void setOperationMode(OperationMode mode);
void setPowerMode(PowerMode mode);
```

### Überwachung

```cpp
int getQueueSize();
int getPendingMessages();
ConnectionQuality getConnectionQuality();
PowerMode getCurrentPowerMode();
unsigned long getUptime();
int getFreeMemory();
```

### Speicher

```cpp
void enableStorage(bool enable = true);
bool saveNow();
bool restoreFromStorage();
void clearStorage();
```

---

## Beispiele

Siehe die mitgelieferten .ino in der Bibliothek, um einige Szenarien und Features sowie meinen Codestil von KissTelegram zu erkunden. Besser noch, kommentieren Sie Ihre Sprache in [lang.h] aus, um Nachrichten von den Hauptkonstruktoren (.cpp) in Ihrer Ortssprache zu erhalten. Wenn alle Sprachen kommentiert sind, erhalten Sie Nachrichten auf Spanisch, der Standardsprache:

Code-Konventionen sind auf Englisch, aber Gedanken und Kommentare in meiner Muttersprache, verwenden Sie Ihren Online-Übersetzer, Code ist einfach, hinter dem Code steckt viel Kompliziertes ...

````cpp

// =========================================================================
// SPRACHAUSWAHL - EINE Sprache auskommentieren
// =========================================================================
// #define LANG_CN  // 中文
// #define LANG_DE  // Deutsch
// #define LANG_EN  // English
// #define LANG_FR  // Français
// #define LANG_IT  // Italiano
// #define LANG_PT  // Português
````

---
## Grundlegende Konfigurationseinstellungen
- Benennen Sie system_setup_template.h in system_setup.h in Ihrem KissTelegram-Ordner um, um mit der Kompilierung zu beginnen.
- Ersetzen Sie die folgenden Zeilen durch Ihre Einstellungen.

````cpp
#define KISS_FALLBACK_BOT_TOKEN "YOUR_TELEGRAM_BOT_TOKEN"
#define KISS_FALLBACK_CHAT_ID "YOUR_CHAT_ID number"
#define KISS_FALLBACK_WIFI_SSID "YOUR_WIFI_SSID"
#define KISS_FALLBACK_WIFI_PASSWORD "YOUR_WIFI_PASSWORD"
#define KISS_FALLBACK_OTA_PIN "0000"
#define KISS_FALLBACK_OTA_PUK "00000000"
````

## Lizenz

Dieses Projekt ist unter der MIT-Lizenz lizenziert - siehe die [LICENSE](LICENSE)-Datei für Details.

---

## Architektur, Vision, Konzept, Lösungen && Design (und der Verantwortliche für Fehlfunktionen)

**Vicente Soriano**
E-Mail: victek@gmail.com
GitHub: [victek](https://github.com/victek)

**Beiträge**
- Viele KI-Assistenten bei Übersetzungen, Code, Problemlösung und Witze.

---


## Beitragen

Beiträge sind willkommen! Bitte zögern Sie nicht, einen Pull Request einzureichen.

---

## Unterstützung

Wenn Sie diese Bibliothek nützlich finden, können Sie gerne:
- Dieses Repository als Favorit markieren
- Bugs über GitHub Issues melden
- Ihre Projekte mit KissTelegram teilen

---
