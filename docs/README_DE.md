# KissTelegram

![License](https://img.shields.io/badge/license-MIT-blue.svg)
![Platform](https://img.shields.io/badge/platform-ESP32--S3-orange.svg)
![Version](https://img.shields.io/badge/version-1.x-green.svg)
![Language](https://img.shields.io/badge/languages-7-brightgreen.svg)

**Deutsch** | [Dokumentation](docs/GETTING_STARTED_DE.md) | [Benchmarks](docs/BENCHMARK.md)

---

> **ERSTE VERWENDUNG VON ESP32-S3 MIT KISSTELEGRAM?**
> **LESEN SIE DIES ZUERST:** [**GETTING_STARTED_DE.md**](docs/GETTING_STARTED_DE.md)
> ESP32-S3 erfordert einen **zweistufigen Upload-Prozess** aufgrund benutzerdefinierter Partitionen. Das Ignorieren dieser Anleitung führt zu Boot-Fehlern und falschen Partitionen!

---

## Eine Robuste Enterprise-Grade-Bibliothek für Telegram-Bots auf ESP32-S3

KissTelegram ist die **einzige Telegram-Bibliothek für ESP32**, die von Grund auf für unternehmenskritische Anwendungen entwickelt wurde. Im Gegensatz zu anderen Bibliotheken, die auf Arduinos `String`-Klasse angewiesen sind (was zu Speicherfragmentierung und Lecks führt), verwendet KissTelegram reine `char[]`-Arrays für unerschütterliche Stabilität.

### Warum KissTelegram?

- Müde von verlorenen Projekten wegen schwacher Bibliotheken, Speicherlecks, Last-Minute-Lösungen, fehlendem Support, leeren Worten, Begriffen die nicht funktionieren, Neustarts....

- Das war meine Vision und Erfahrung mit anderen Bibliotheken und das ist das Ergebnis mit KissTelegram:

- **Null Nachrichtenverlust**: Persistente Warteschlange auf LittleFS, die Abstürze, Neustarts und WiFi-Ausfälle überlebt
- **Keine Speicherlecks**: Pure `char[]`-Implementierung, keine String-Fragmentierung
- **SSL/TLS-Sicherheit**: Sichere Verbindungen zur Telegram-API mit Zertifikatsvalidierung (bis 2035)
- **Intelligentes Energiemanagement**: 6 Energiemodi (BOOT, LOW, IDLE, ACTIVE, TURBO, MAINTENANCE)
- **Nachrichtenprioritäten**: CRITICAL, HIGH, NORMAL, LOW mit intelligenter Warteschlangenverwaltung
- **Turbo-Modus**: Batch-Verarbeitung für große Nachrichtenwarteschlangen (0,9 msg/s)
- **Mehrsprachiges i18n**: Sprachauswahl zur Kompilierzeit für Nachrichtenversand (7 Sprachen, null Laufzeit-Overhead)
- **Enterprise-OTA**: Dual-Boot-Firmware-Updates mit automatischem Rollback und Sicherheitsverwaltung
- **100% Flash-Auslastung**: Benutzerdefiniertes Partitionsschema, das die 16MB Flash des ESP32-S3 maximiert
- **Sicherer als Espressif OTA**: PIN/PUK-Authentifizierung, Prüfsummenverifizierung, 60s Validierungsfenster
- **Unabhängig von externen Bibliotheken**: Alles von Grund auf neu erstellt, eigener JSON-Parser für KissTelegram-Bibliotheken.

---

## Hardware-Anforderungen

- **ESP32-S3** mit **16MB Flash** / **8MB PSRAM**
- WiFi-Konnektivität
- Arduino IDE oder PlatformIO

---

## Installation

### Arduino IDE

1. Laden Sie dieses Repository als ZIP herunter
2. Öffnen Sie Arduino IDE → Sketch → Include Library → Add .ZIP Library
3. Wählen Sie die heruntergeladene Datei aus

### PlatformIO

Fügen Sie zu Ihrer `platformio.ini` hinzu:

```ini
lib_deps =
    https://github.com/victek/KissTelegram.git
```

---

## Benutzerdefiniertes Partitionsschema

KissTelegram enthält eine optimierte `partitions.csv`, die die Flash-Nutzung maximiert:

```csv
# Name,   Type, SubType, Offset,  Size
nvs,      data, nvs,     0x9000,  0x4000,
otadata,  data, ota,     0xd000,  0x2000,
app0,     app,  factory, 0x10000, 0x180000,   # 1.5MB
app1,     app,  ota_0,   0x190000,0x180000,   # 1.5MB
spiffs,   data, spiffs,  0x310000,0xCF0000,   # 13MB!
```

**13MB SPIFFS-Speicher** - Das sind 8MB mehr als Espressifs Standardschemata!

Um dieses Partitionsschema zu verwenden:
1. Kopieren Sie `partitions.csv` in Ihr Projektverzeichnis
2. In Arduino IDE: Tools ->Partition Scheme ->Custom
3. In PlatformIO: `board_build.partitions = partitions.csv`

---

## Schnellstart

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

### OTA-Update-Beispiel

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
2. Geben Sie die PIN mit `/otapin YOUR_PIN` ein
3. Laden Sie die `.bin` Firmware-Datei hoch
4. Der Bot verifiziert automatisch die Prüfsumme
5. Bestätigen Sie mit `/otaconfirm`
6. Nach dem Neustart validieren Sie mit `/otaok` innerhalb von 60 Sekunden
7. Automatisches Rollback bei fehlgeschlagener Validierung!

- Lesen Sie Readme_KissOTA.md in Ihrer bevorzugten Sprache, um mehr über die Lösung zu erfahren.

---

## Wichtige Funktionen Erklärt

### 1. Persistente Nachrichtenwarteschlange

Nachrichten werden auf LittleFS mit automatischer Batch-Löschung gespeichert:

```cpp
bot.queueMessage(chat_id, "Important message", KissTelegram::PRIORITY_HIGH);
```

- Überlebt Abstürze, WiFi-Trennungen, Neustarts
- Automatische Wiederholung fehlgeschlagener Sendevorgänge
- Intelligente Batch-Löschung (alle 10 Nachrichten + wenn Warteschlange leer ist)
- Garantie für null Nachrichtenverlust

### 2. Energieverwaltung

6 intelligente Energiemodi passen sich den Anforderungen Ihrer Anwendung an:

```cpp
bot.setPowerMode(KissTelegram::POWER_ACTIVE);
bot.setPowerConfig(30, 60, 10); // idle, decay, boot stable times
```

- **POWER_BOOT**: Anfängliche Startphase (10s)
- **POWER_LOW**: Minimale Aktivität, langsames Polling
- **POWER_IDLE**: Keine kürzliche Aktivität, reduzierte Überprüfungen
- **POWER_ACTIVE**: Normaler Betrieb
- **POWER_TURBO**: Hochgeschwindigkeits-Batch-Verarbeitung (50ms Intervalle)
- **POWER_MAINTENANCE**: Manuelles Override für Updates
- **Abkling-Timing für sanfte Übergänge**

### 3. Nachrichtenprioritäten

Vier Prioritätsstufen stellen sicher, dass kritische Nachrichten zuerst gesendet werden und niedrigere Prioritäten überspringen:

```cpp
bot.sendMessage(chat_id, "Normal message", KissTelegram::PRIORITY_NORMAL);
bot.sendMessage(chat_id, "Alert!", KissTelegram::PRIORITY_CRITICAL);
```

Warteschlange verarbeitet: **CRITICAL /HIGH /NORMAL /LOW**
Interne Prozesse: **OTAMODE /MAINTENANCEMODE**

### 4. SSL/TLS-Sicherheit

Sichere Verbindungen mit Zertifikatsvalidierung (bis 2035):

```cpp
bool secure = bot.isSSLSecure();
String sslInfo = bot.getSSLInfo();
bot.testSSLConnection();
```

- Automatischer Fallback zwischen sicher und unsicher
- Periodische Ping-Überprüfungen zur Aufrechterhaltung der Verbindung
- Wiederverwendbarer Verbindungscode spart Verbindungs-Overhead für maximale Leistung

### 5. Turbo-Modus

Wird automatisch aktiviert beim Senden von Batches mit dem Befehl /llenar:

```cpp
bot.enableTurboMode();  // Auto activation
```

- Verarbeitet 10 Nachrichten pro Zyklus
- 50ms Intervalle zwischen Batches
- Erreicht 0,9 msg/s Durchsatz
- Deaktiviert sich automatisch, wenn Warteschlange gesendet ist

### 6. Betriebsmodi

Vorkonfigurierte Profile für verschiedene Szenarien:

```cpp
bot.setOperationMode(KissTelegram::MODE_RELIABILITY);
```

- **MODE_BALANCED**: Standard (Polling: 10s, Wiederholung: 3)
- **MODE_PERFORMANCE**: Schnell (Polling: 5s, Wiederholung: 2)
- **MODE_POWERSAVE**: Langsam (Polling: 30s, Wiederholung: 2)
- **MODE_RELIABILITY**: Robust (Polling: 15s, Wiederholung: 5)

### 7. Diagnose

Vollständige Überwachung und Fehlerbehebung:

```cpp
bot.printDiagnostics();
bot.printStorageStatus();
bot.printPowerStatistics();
bot.printSystemStatus();
```

Zeigt:
- Freier Speicher (Heap/PSRAM)
- Nachrichtenwarteschlangen-Statistiken
- Verbindungsqualität
- Energiemodus-Verlauf
- Speichernutzung
- Betriebszeit

---

## 8. WiFi-Verwaltung
- Integrierter WiFi-Manager aktiviert andere Aufgaben nur, wenn WiFi stabil ist
- Verhindert Race-Conditions
- Holt laufende Nachrichten zum FS-Speicher zurück, bis Verbindung wiederhergestellt ist, kann bis zu 3500 msg halten (Standard aber leicht erweiterbar, hängt davon ab, wie viel Platz Sie nutzen möchten)
- Überwachung der Verbindungsqualität (EXCELLENT, GOOD, FAIR, POOR, DEAD) und RSSI-Ausgabepegel
- Sie müssen sich nur um Ihren Sketch kümmern, fügen Sie Ihren Code hinzu und KissTelegram übernimmt die kritischen Aufgaben, WiFi, SSL, Nachrichten, OTA, Energieverwaltung, Prioritäten.. all die Arbeit, die Sie sparen....

---

## 9. Schlüsselfunktion: `/estado` Befehl

**Das leistungsstärkste Debugging-Tool, das Sie jemals in Ihren Sketch einbauen werden**

Senden Sie `/estado` an Ihren Bot und erhalten Sie einen **vollständigen Gesundheitsbericht Ihres Sketches** in diesem Moment, (verfügbar in 7 Sprachen):

```
KissTelegram_test, [15/12/2025 13:11]
🔧 KissTelegram v0.9.0
📦 Build: Dec 15 2025 13:10:23 (0xD984C13E)

✅ SYSTEMZUVERLÄSSIGKEIT
✓ System: ZUVERLÄSSIG
✓ Gesendete Nachrichten: 940
📨 Ausstehende Nachrichten: 70
✓ Verlorene Nachrichten: 0
🗑️ Verworfene (volle Warteschlange): 0

⚠️ ÄUSSERE WIDRIGKEITEN
⚠️ Gesamtfehler: 0
🔄 Wiederhergestellt (Fallback): 0
📡 WiFi-Ausfälle: 0

📊 TECHNISCHE INFORMATIONEN
⏱️ Betriebszeit: 0h 0m
🧠 Freies RAM: 223960 bytes
💾 Freies PSRAM: 1027820 bytes
💽 Freies FS: 13549568 bytes
📦 Max. in FS: 3500 Nachrichten
⚡ Energiemodus: 3 
📶 WiFi-Signal: -64 dBm (Mäßig)
🔒 SSL: SICHER
🚀 Turbo: INAKTIV
🤖 Auto-Nachrichten: JA

```

**Warum `/estado` unverzichtbar ist:**
- Sofortige Systemgesundheitsprüfung
- WiFi-Qualitätsüberwachung (Konnektivitätsprobleme diagnostizieren)
- Speicherleck-Erkennung (freien Heap beobachten)
- Nachrichtenwarteschlangen-Status (ausstehende/fehlgeschlagene Nachrichten sehen)
- Betriebszeit-Tracking (Stabilitätsüberwachung)
- Ihr erstes Diagnose-Tool

**Profi-Tipp:** Machen Sie `/estado` zu Ihrer ersten Nachricht nach jedem Firmware-Update, um zu überprüfen, dass alles funktioniert!

---

## 10. NTP
- Eigener Code zum Synchronisieren/Resynchronisieren für SSL. GNSS, LTE und Scheduler (Enterprise Edition)
---

## 11. Dokumentation (7 Sprachen)

- **[GETTING_STARTED_DE.md](docs/GETTING_STARTED_DE.md)** - **HIER BEGINNEN!** Vollständige Anleitung vom Erhalt des ESP32-S3 bis zur ersten an Telegram gesendeten Nachricht
- **[README_DE.md](docs/README_DE.md)** (diese Datei) - Funktionsübersicht, Schnellstart, API-Referenz
- **[BENCHMARK.md](docs/BENCHMARK.md)** - Technischer Vergleich mit 6 Telegram-Bibliotheken (nur Englisch, aber selbsterklärend)
- **[README_KissOTA_XX.md](docs/README_KissOTA_DE.md)** - Großer Wert, da es die Schritte des OTA-Update-Systems detailliert (7 Sprachen: EN, ES, FR, IT, DE, PT, CN)

**Wählen Sie Ihre Sprache:** Alle Konstruktor-Nachrichten, die an Telegram gesendet werden, werden in 7 Sprachen über Sprachauswahl während der Kompilierung angezeigt (lang.h).


## OTA-Sicherheitsvorteile

KissTelegram OTA ist **viel sicherer als Espressifs Architektur und spart Platz auf Ihrem ESP32S3**:

| Funktion | KissTelegram OTA | Espressif OTA |
|---------|------------------|---------------|
| Authentifizierung | PIN + PUK | Keine |
| Prüfsummen-Bestätigung | Automatisches CRC32 | Manuell |
| Backup und Rollback | Automatisch | Manuell |
| Validierungsfenster | 60s mit `/otaok` | Keines |
| Boot-Loop-Erkennung | Ja | Nein |
| Telegram-Integration | Nativ | Erfordert benutzerdefinierten Code |
| Flash-Optimierung | 13MB SPIFFS | 5MB SPIFFS |

---

## API-Referenz

### Initialisierung

```cpp
KissTelegram(const char* token);
void enable();
void disable();
```

### Nachrichtenversand

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

Sehen Sie sich die in der Bibliothek enthaltene .ino-Datei an, um einige Szenarien und Funktionen sowie meinen Codierungsstil in KissTelegram zu erkunden. Noch besser, kommentieren Sie Ihre Sprache in [lang.h] aus, um Nachrichten von den Hauptkonstruktoren (.cpp) in Ihrer Lokalsprache zu erhalten. Wenn alle Sprachen auskommentiert sind, sind die Nachrichten auf Spanisch, der Standardsprache:

Code-Konventionen sind auf Englisch, aber Gedanken und Kommentare sind auf Spanisch, meiner Muttersprache, verwenden Sie Ihren Online-Übersetzer, der Code ist einfach, im Code ist meine Vision und das Konzept von KissTelegram...

````cpp

// =========================================================================
// LANGUAGE SELECTION - Uncomment ONE language
// =========================================================================
// #define LANG_CN  // 中文
// #define LANG_DE  // Deutsch
// #define LANG_EN  // English
// #define LANG_FR  // Français
// #define LANG_IT  // Italiano
// #define LANG_PT  // Português
````

---
## Grundlegende Konfigurationseinrichtung
- Benennen Sie system_setup_template.h in system_setup.h in Ihrem KissTelegram-Ordner um, um mit der Kompilierung zu beginnen.
- Ersetzen Sie die folgenden Zeilen durch Ihre Anmeldeinformationen.

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

## Architektur, Vision, Konzept, Lösungen und Design (und verantwortlich für jede Fehlfunktion, das bin ich...)

**Vicente Soriano**
E-Mail: victek@gmail.com
GitHub: [victek](https://github.com/victek/KissTelegram)

**Mitwirkende**
- Viele KI-Assistenten bei Übersetzungen, Code, Fehlerbehebung und viele Stunden damit verbracht zu versuchen, sie daran zu hindern, das Rad neu zu erfinden.....

---


## Beitragen

Beiträge sind willkommen! Bitte zögern Sie nicht, einen Pull Request einzureichen oder mir eine E-Mail zu senden, aber ich bevorzuge einen PR, damit andere Ihre Frage finden können.

---

## Unterstützung

Wenn Sie diese Bibliothek nützlich finden, erwägen Sie bitte:
- Diesem Repository einen Stern zu geben
- Fehler über GitHub Issues zu melden
- Ihre Projekte mit KissTelegram zu teilen
- Ihren Bekannten von den Funktionen und Lösungen dieser Bibliothek zu erzählen
- Anwendungsfälle und Erfahrungen mit KissTelegram vorzuschlagen

---
