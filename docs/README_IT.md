# KissTelegram

![License](https://img.shields.io/badge/license-MIT-blue.svg)
![Platform](https://img.shields.io/badge/platform-ESP32--S3-orange.svg)
![Version](https://img.shields.io/badge/version-1.x-green.svg)
![Language](https://img.shields.io/badge/languages-7-brightgreen.svg)

**[English](README.md)** | [Documentazione](GETTING_STARTED_IT.md) | [Benchmark](BENCHMARK.md)

---

> 🚨 **PRIMO UTILIZZO DI ESP32-S3 CON KISSTELEGRAM?**
> **LEGGI PRIMA:** [**GETTING_STARTED_IT.md**](GETTING_STARTED_IT.md) ⚠️
> ESP32-S3 richiede un **processo di caricamento in due fasi** a causa delle partizioni personalizzate. Saltare questa guida causerà errori di boot!

---

## Una libreria Telegram Bot robusta e di livello aziendale per ESP32-S3

KissTelegram è l'**unica libreria Telegram per ESP32** costruita da zero per applicazioni mission-critical. A differenza di altre librerie che si affidano alla classe Arduino `String` (causando frammentazione della memoria e perdite), KissTelegram utilizza array `char[]` puri per stabilità assoluta.

### Perché KissTelegram?

- Stufo di progetti persi a causa di librerie deboli, perdite di memoria, soluzioni dell'ultimo minuto, mancanza di supporto, parole chiave vuote, riavvii....

- La mia visione, adesso i fatti:

- **Zero perdita di messaggi**: La coda persistente LittleFS sopravvive ai crash, ai riavvii e ai guasti WiFi
- **Nessuna perdita di memoria**: Implementazione pura con `char[]`, nessuna frammentazione di String
- **Sicurezza SSL/TLS**: Connessioni sicure all'API Telegram con convalida dei certificati
- **Gestione intelligente dell'alimentazione**: 6 modalità di potenza (BOOT, LOW, IDLE, ACTIVE, TURBO, MAINTENANCE)
- **Priorità dei messaggi**: CRITICAL, HIGH, NORMAL, LOW con gestione intelligente della coda
- **Turbo Mode**: Elaborazione batch per code di messaggi grandi (0,9 msg/s)
- **i18n multilingue**: Selezione della lingua al momento della compilazione (7 lingue, senza sovraccarico runtime)
- **OTA aziendale**: Aggiornamenti firmware dual-boot con rollback automatico e gateway di sicurezza
- **Utilizzo Flash 100%**: Schema di partizione personalizzato che massimizza il flash ESP32-S3 da 16MB
- **Più sicuro dell'OTA di Espressif**: Autenticazione PIN/PUK, verifica del checksum, finestra di convalida di 60s
- **Indipendente da librerie esterne**: Tutto fatto da zero, parser Json proprio.

---

## Requisiti hardware

- **ESP32-S3** con **16MB Flash** / **8MB PSRAM**
- Connettività WiFi
- Arduino IDE o PlatformIO

---

## Installazione

### Arduino IDE

1. Scarica questo repository come ZIP
2. Apri Arduino IDE → Sketch → Include Library → Add .ZIP Library
3. Seleziona il file scaricato

### PlatformIO

Aggiungi al tuo `platformio.ini`:

```ini
lib_deps =
    https://github.com/victek/KissTelegram.git
```

---

## Schema di partizione personalizzato

KissTelegram include un `partitions.csv` ottimizzato che massimizza l'utilizzo del flash:

```csv
# Name,   Type, SubType, Offset,  Size
nvs,      data, nvs,     0x9000,  0x4000,
otadata,  data, ota,     0xd000,  0x2000,
app0,     app,  factory, 0x10000, 0x180000,   # 1.5MB
app1,     app,  ota_0,   0x190000,0x180000,   # 1.5MB
spiffs,   data, spiffs,  0x310000,0xCF0000,   # 13MB!
```

**13MB di storage SPIFFS** - è 8MB in più rispetto agli schemi predefiniti di Espressif!

Per utilizzare questo schema di partizione:
1. Copia `partitions.csv` nella directory del tuo progetto
2. In Arduino IDE: Tools → Partition Scheme → Custom
3. In PlatformIO: `board_build.partitions = partitions.csv`

---

## Avvio rapido

### Esempio di base

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

### Esempio di aggiornamenti OTA

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

**Processo OTA:**
1. Invia `/ota` al tuo bot
2. Inserisci il PIN con `/otapin YOUR_PIN`
3. Carica il file firmware `.bin`
4. Il bot verifica il checksum automaticamente
5. Conferma con `/otaconfirm`
6. Dopo il riavvio, convalida con `/otaok` entro 60 secondi
7. Rollback automatico se la convalida fallisce!

- Leggi Readme_KissOTA.md nella tua lingua preferita per saperne di più sulla soluzione.

---

## Spiegazione delle principali caratteristiche

### 1. Coda di messaggi persistente

I messaggi sono memorizzati in LittleFS con eliminazione batch automatica:

```cpp
bot.queueMessage(chat_id, "Important message", KissTelegram::PRIORITY_HIGH);
```

- Sopravvive a crash, disconnessioni WiFi, riavvii
- Ritenta automaticamente i messaggi non inviati
- Eliminazione batch intelligente (ogni 10 messaggi + al svuotamento della coda)
- Garanzia di zero perdita di messaggi

### 2. Gestione dell'alimentazione

6 modalità di potenza intelligenti si adattano alle esigenze della tua applicazione:

```cpp
bot.setPowerMode(KissTelegram::POWER_ACTIVE);
bot.setPowerConfig(30, 60, 10); // idle, decay, boot stable times
```

- **POWER_BOOT**: Fase di avvio iniziale (10s)
- **POWER_LOW**: Attività minima, polling lento
- **POWER_IDLE**: Nessuna attività recente, controlli ridotti
- **POWER_ACTIVE**: Operazione normale
- **POWER_TURBO**: Elaborazione batch ad alta velocità (intervalli di 50ms)
- **POWER_MAINTENANCE**: Override manuale per aggiornamenti
- **Decay Timing per commutazione fluida**

### 3. Priorità dei messaggi

Quattro livelli di priorità garantiscono che i messaggi critici vengono inviati per primi:

```cpp
bot.sendMessage(chat_id, "Normal message", KissTelegram::PRIORITY_NORMAL);
bot.sendMessage(chat_id, "Alert!", KissTelegram::PRIORITY_CRITICAL);
```

La coda elabora: **CRITICAL → HIGH → NORMAL → LOW**
Processi interni: **OTAMODE → MAINTENANCEMODE**

### 4. Sicurezza SSL/TLS

Connessioni sicure con convalida dei certificati:

```cpp
bool secure = bot.isSSLSecure();
String sslInfo = bot.getSSLInfo();
bot.testSSLConnection();
```

- Fallback sicuro/non sicuro automatico
- Controlli ping periodici per mantenere la connessione
- Il codice di connessione riutilizzabile salva l'intestazione della connessione per il massimo throughput

### 5. Turbo Mode

Si attiva automaticamente quando la coda > 100 messaggi:

```cpp
bot.enableTurboMode();  // Manual activation
```

- Elabora 10 messaggi per ciclo
- Intervalli di 50ms tra i batch
- Raggiunge un throughput di 15-20 msg/s
- Si disattiva automaticamente quando la coda viene inviata

### 6. Modalità operative

Profili preconfigurati per diversi scenari:

```cpp
bot.setOperationMode(KissTelegram::MODE_RELIABILITY);
```

- **MODE_BALANCED**: Predefinita (polling: 10s, retry: 3)
- **MODE_PERFORMANCE**: Veloce (polling: 5s, retry: 2)
- **MODE_POWERSAVE**: Lenta (polling: 30s, retry: 2)
- **MODE_RELIABILITY**: Robusta (polling: 15s, retry: 5)

### 7. Diagnostica

Monitoraggio e debug completi:

```cpp
bot.printDiagnostics();
bot.printStorageStatus();
bot.printPowerStatistics();
bot.printSystemStatus();
```

Visualizza:
- Memoria libera (heap/PSRAM)
- Statistiche della coda di messaggi
- Qualità della connessione
- Cronologia delle modalità di potenza
- Utilizzo dell'archiviazione
- Uptime

---

## 8. Gestione WiFi
- WiFi Manager integrato attiva altre attività all'inizio finché WiFi non è stabile
- Previene race condition
- Invia i messaggi in corso all'archiviazione FS fino a quando la connessione non viene ripristinata, può conservare fino a 3500 msg (predefinito ma facilmente espandibile)
- Monitoraggio della qualità della connessione (EXCELLENT, GOOD, FAIR, POOR, DEAD) e livello di output RSSI
- Devi solo prenderti cura del tuo sketch, usa KissTelegram per il resto

---

## 9. Killer Feature: Comando `/estado`

**Lo strumento di debug più potente che tu abbia mai utilizzato:**

Invia `/estado` al tuo bot e ottieni un **report di salute completo** in pochi secondi:

```
📦 KissTelegram v1.x.x
🎯 SYSTEM RELIABILITY
✅ System: RELIABLE
✅ Messages sent: 5,234
💾 Messages pending: 0
📡 WiFi Signal: -59 dBm (Good)
🔌 WiFi reconnections: 2
⏱️ Uptime: 86,400 seconds (24h)
💾 Free memory: 223 KB
📊 Queue statistics: All systems operational
```

**Perché `/estado` è essenziale:**
- ✅ Controllo istantaneo della salute del sistema
- ✅ Monitoraggio della qualità WiFi (diagnostica dei problemi di connettività)
- ✅ Rilevamento delle perdite di memoria (monitoraggio dell'heap libero)
- ✅ Stato della coda di messaggi (visualizza messaggi in sospeso/non inviati)
- ✅ Tracciamento dell'uptime (monitoraggio della stabilità)
- ✅ Disponibile in 7 lingue
- ✅ Il tuo primo strumento quando si risolvono i problemi

**Pro tip:** Rendi `/estado` il tuo primo messaggio dopo ogni aggiornamento del firmware per verificare che tutto funzioni!

---

## 10. NTP
- Codice proprio per sincronizzazione/risincronizzazione per SSL e Scheduler (Edizione Enterprise)
---

## 11. Documentazione (7 lingue)

- **[GETTING_STARTED_IT.md](GETTING_STARTED_IT.md)** - **INIZIA DA QUI!** Guida completa dallo scatolone di ESP32-S3 al primo messaggio Telegram
- **[README_IT.md](README_IT.md)** (questo file) - Panoramica delle funzionalità, avvio rapido, riferimento API
- **[BENCHMARK.md](BENCHMARK.md)** - Confronto tecnico rispetto a 6 altre librerie Telegram (solo in inglese)
- **[README_KissOTA_XX.md](README_KissOTA_EN.md)** - Documentazione del sistema di aggiornamento OTA (7 lingue: EN, ES, FR, IT, DE, PT, CN)

**Scegli la tua lingua:** Tutti i messaggi del sistema KissTelegram supportano 7 lingue tramite selezione al momento della compilazione.


## Vantaggi della sicurezza OTA

KissTelegram OTA è **più sicuro dell'implementazione OTA di Espressif**:

| Feature | KissTelegram OTA | Espressif OTA |
|---------|------------------|---------------|
| Authentication | PIN + PUK | None |
| Checksum Verification | CRC32 automatic | Manual |
| Backup & Rollback | Automatic | Manual |
| Validation Window | 60s with `/otaok` | None |
| Boot Loop Detection | Yes | No |
| Telegram Integration | Native | Requires custom code |
| Flash Optimization | 13MB SPIFFS | 5MB SPIFFS |

---

## Riferimento API

### Inizializzazione

```cpp
KissTelegram(const char* token);
void enable();
void disable();
```

### Messaggistica

```cpp
bool sendMessage(const char* chat_id, const char* text,
                 MessagePriority priority = PRIORITY_NORMAL);
bool sendMessageDirect(const char* chat_id, const char* text);
bool queueMessage(const char* chat_id, const char* text,
                  MessagePriority priority = PRIORITY_NORMAL);
void processQueue();
```

### Configurazione

```cpp
void setMinMessageInterval(int milliseconds);
void setMaxMessageSize(int size);
void setMaxRetryAttempts(int attempts);
void setPollingTimeout(int seconds);
void setOperationMode(OperationMode mode);
void setPowerMode(PowerMode mode);
```

### Monitoraggio

```cpp
int getQueueSize();
int getPendingMessages();
ConnectionQuality getConnectionQuality();
PowerMode getCurrentPowerMode();
unsigned long getUptime();
int getFreeMemory();
```

### Archiviazione

```cpp
void enableStorage(bool enable = true);
bool saveNow();
bool restoreFromStorage();
void clearStorage();
```

---

## Esempi

Vedi i file .ino inclusi nella libreria per esplorare alcuni scenari e funzionalità e il mio stile di codice di KissTelegram. Meglio ancora, decommenta la tua lingua in [lang.h] per ricevere messaggi dai costruttori principali (.cpp) nella tua lingua locale, se tutte le lingue sono commentate ricevi i messaggi in spagnolo, la lingua predefinita:

Le convenzioni di codice sono in inglese, ma i pensieri e i commenti nella mia lingua madre, usa il tuo traduttore online, il codice è facile, dietro il codice c'è molto più complicato ...

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
## Impostazioni di configurazione di base
- Rinomina system_setup_template.h a system_setup.h nella tua cartella KissTelegram per avviare la compilazione.
- Sostituisci le seguenti righe con le tue impostazioni.

````cpp
#define KISS_FALLBACK_BOT_TOKEN "YOUR_TELEGRAM_BOT_TOKEN"
#define KISS_FALLBACK_CHAT_ID "YOUR_CHAT_ID number"
#define KISS_FALLBACK_WIFI_SSID "YOUR_WIFI_SSID"
#define KISS_FALLBACK_WIFI_PASSWORD "YOUR_WIFI_PASSWORD"
#define KISS_FALLBACK_OTA_PIN "0000"
#define KISS_FALLBACK_OTA_PUK "00000000"
````

## Licenza

Questo progetto è sotto licenza MIT - consulta il file [LICENSE](LICENSE) per i dettagli.

---

## Architettura, Visione, Concetto, Soluzioni && Design (e il responsabile di qualsiasi malfunzionamento)

**Vicente Soriano**
Email: victek@gmail.com
GitHub: [victek](https://github.com/victek)

**Contributori**
- Molti assistenti IA in Traduzioni, Codice, Risoluzione dei problemi e battute.

---


## Contributi

I contributi sono benvenuti! Sentiti libero di inviare una Pull Request.

---

## Supporto

Se trovi questa libreria utile, considera di:
- Mettere una stella a questo repository
- Segnalare bug tramite GitHub Issues
- Condividere i tuoi progetti che utilizzano KissTelegram

---

