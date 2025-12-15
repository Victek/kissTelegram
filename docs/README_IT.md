# KissTelegram

![License](https://img.shields.io/badge/license-MIT-blue.svg)
![Platform](https://img.shields.io/badge/platform-ESP32--S3-orange.svg)
![Version](https://img.shields.io/badge/version-1.x-green.svg)
![Language](https://img.shields.io/badge/languages-7-brightgreen.svg)

**Italiano** | [Documentazione](docs/GETTING_STARTED_IT.md) | [Benchmarks](docs/BENCHMARK.md)

---

> **PRIMA VOLTA CON ESP32-S3 E KISSTELEGRAM?**
> **LEGGI PRIMA QUESTO:** [**GETTING_STARTED_IT.md**](docs/GETTING_STARTED_IT.md)
> ESP32-S3 richiede un **processo di caricamento in due fasi** a causa delle partizioni personalizzate. Ignorare questa guida causerà errori di avvio e partizioni sbagliate!

---

## Una Libreria Robusta di Livello Aziendale per Bot Telegram su ESP32-S3

KissTelegram è la **unica libreria Telegram per ESP32** costruita da zero per applicazioni mission-critical. A differenza di altre librerie che dipendono dalla classe `String` di Arduino (causando frammentazione della memoria e perdite), KissTelegram utilizza array puri `char[]` per una stabilità incrollabile.

### Perché KissTelegram?

- Stanco di progetti persi a causa di librerie deboli, perdite di memoria, soluzioni dell'ultimo minuto, mancanza di supporto, parole vuote, termini che non funzionano, riavvii....

- Questa era la mia visione ed esperienza con altre librerie e questo è il risultato con KissTelegram:

- **Zero Perdita di Messaggi**: Coda persistente su LittleFS che sopravvive a crash, riavvii e guasti WiFi
- **Nessuna Perdita di Memoria**: Implementazione pura `char[]`, nessuna frammentazione String
- **Sicurezza SSL/TLS**: Connessioni sicure all'API Telegram con validazione certificati (fino al 2035)
- **Gestione Intelligente dell'Energia**: 6 modalità di alimentazione (BOOT, LOW, IDLE, ACTIVE, TURBO, MAINTENANCE)
- **Priorità dei Messaggi**: CRITICAL, HIGH, NORMAL, LOW con gestione intelligente della coda
- **Modalità Turbo**: Elaborazione batch per grandi code di messaggi (0,9 msg/s)
- **i18n Multilingue**: Selezione lingua a compile-time per invio messaggi (7 lingue, zero overhead runtime)
- **OTA Aziendale**: Aggiornamenti firmware dual-boot con rollback automatico e gestione sicurezza
- **Utilizzo Flash al 100%**: Schema partizioni personalizzato che massimizza i 16MB flash dell'ESP32-S3
- **Più Sicuro dell'OTA Espressif**: Autenticazione PIN/PUK, verifica checksum, finestra validazione 60s
- **Indipendente da librerie esterne**: Tutto costruito da zero, parser JSON proprio per le librerie KissTelegram.

---

## Requisiti Hardware

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

## Schema Partizioni Personalizzato

KissTelegram include un `partitions.csv` ottimizzato che massimizza l'uso della flash:

```csv
# Name,   Type, SubType, Offset,  Size
nvs,      data, nvs,     0x9000,  0x4000,
otadata,  data, ota,     0xd000,  0x2000,
app0,     app,  factory, 0x10000, 0x180000,   # 1.5MB
app1,     app,  ota_0,   0x190000,0x180000,   # 1.5MB
spiffs,   data, spiffs,  0x310000,0xCF0000,   # 13MB!
```

**13MB di storage SPIFFS** - Sono 8MB in più rispetto agli schemi predefiniti di Espressif!

Per usare questo schema di partizioni:
1. Copia `partitions.csv` nella tua directory di progetto
2. In Arduino IDE: Tools ->Partition Scheme ->Custom
3. In PlatformIO: `board_build.partitions = partitions.csv`

---

## Avvio Rapido

### Esempio Base

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

### Esempio Aggiornamento OTA

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
4. Il bot verifica automaticamente il checksum
5. Conferma con `/otaconfirm`
6. Dopo il riavvio, valida con `/otaok` entro 60 secondi
7. Rollback automatico se la validazione fallisce!

- Leggi Readme_KissOTA.md nella tua lingua preferita per saperne di più sulla soluzione.

---

## Funzionalità Chiave Spiegate

### 1. Coda Messaggi Persistente

I messaggi sono memorizzati su LittleFS con cancellazione automatica batch:

```cpp
bot.queueMessage(chat_id, "Important message", KissTelegram::PRIORITY_HIGH);
```

- Sopravvive a crash, disconnessioni WiFi, riavvii
- Riprova automatica degli invii falliti
- Cancellazione batch intelligente (ogni 10 messaggi + quando la coda è vuota)
- Garanzia zero perdita messaggi

### 2. Gestione dell'Energia

6 modalità di alimentazione intelligenti si adattano alle esigenze della tua applicazione:

```cpp
bot.setPowerMode(KissTelegram::POWER_ACTIVE);
bot.setPowerConfig(30, 60, 10); // idle, decay, boot stable times
```

- **POWER_BOOT**: Fase avvio iniziale (10s)
- **POWER_LOW**: Attività minima, polling lento
- **POWER_IDLE**: Nessuna attività recente, controlli ridotti
- **POWER_ACTIVE**: Operazione normale
- **POWER_TURBO**: Elaborazione batch ad alta velocità (intervalli 50ms)
- **POWER_MAINTENANCE**: Override manuale per aggiornamenti
- **Timing di decadimento per transizioni fluide**

### 3. Priorità dei Messaggi

Quattro livelli di priorità assicurano che i messaggi critici siano inviati per primi, saltando quelli a priorità inferiore:

```cpp
bot.sendMessage(chat_id, "Normal message", KissTelegram::PRIORITY_NORMAL);
bot.sendMessage(chat_id, "Alert!", KissTelegram::PRIORITY_CRITICAL);
```

La coda elabora: **CRITICAL /HIGH /NORMAL /LOW**
Processi interni: **OTAMODE /MAINTENANCEMODE**

### 4. Sicurezza SSL/TLS

Connessioni sicure con validazione certificati (fino al 2035):

```cpp
bool secure = bot.isSSLSecure();
String sslInfo = bot.getSSLInfo();
bot.testSSLConnection();
```

- Fallback automatico tra sicuro e insicuro
- Controlli ping periodici per mantenere la connessione
- Codice connessione riutilizzabile risparmia overhead di connessione per massime prestazioni

### 5. Modalità Turbo

Attivata automaticamente quando si inviano batch usando il comando /llenar:

```cpp
bot.enableTurboMode();  // Auto activation
```

- Elabora 10 messaggi per ciclo
- Intervalli di 50ms tra i batch
- Raggiunge throughput di 0,9 msg/s
- Si disattiva automaticamente quando la coda è inviata

### 6. Modalità di Operazione

Profili preconfigurati per diversi scenari:

```cpp
bot.setOperationMode(KissTelegram::MODE_RELIABILITY);
```

- **MODE_BALANCED**: Predefinito (polling: 10s, riprova: 3)
- **MODE_PERFORMANCE**: Veloce (polling: 5s, riprova: 2)
- **MODE_POWERSAVE**: Lento (polling: 30s, riprova: 2)
- **MODE_RELIABILITY**: Robusto (polling: 15s, riprova: 5)

### 7. Diagnostica

Monitoraggio e debugging completi:

```cpp
bot.printDiagnostics();
bot.printStorageStatus();
bot.printPowerStatistics();
bot.printSystemStatus();
```

Mostra:
- Memoria libera (heap/PSRAM)
- Statistiche coda messaggi
- Qualità connessione
- Cronologia modalità alimentazione
- Utilizzo storage
- Tempo di attività

---

## 8. Gestione WiFi
- Gestore WiFi integrato attiva altre attività solo quando WiFi è stabile
- Previene condizioni di gara
- Recupera messaggi in corso verso storage FS fino al ripristino della connessione, può contenere fino a 3500 msg (predefinito ma facilmente espandibile, dipende da quanto spazio vuoi usare)
- Monitoraggio qualità connessione (EXCELLENT, GOOD, FAIR, POOR, DEAD) e livello output RSSI
- Devi solo occuparti del tuo sketch, aggiungi il tuo codice e KissTelegram gestisce i compiti critici, WiFi, SSL, Messaggi, OTA, Gestione Energia, Priorità.. tutto il lavoro che risparmi....

---

## 9. Funzionalità Chiave: Comando `/estado`

**Lo strumento di debug più potente che includerai mai nel tuo sketch**

Invia `/estado` al tuo bot e ottieni un **rapporto completo sulla salute del tuo sketch** in quel momento, (disponibile in 7 lingue):

```
KissTelegram_test, [15/12/2025 13:11]
🔧 KissTelegram v0.9.0
📦 Build: Dec 15 2025 13:10:23 (0xD984C13E)

✅ AFFIDABILITÀ SISTEMA
✓ Sistema: AFFIDABILE
✓ Messaggi inviati: 940
📨 Messaggi in attesa: 70
✓ Messaggi persi: 0
🗑️ Scartati (coda piena): 0

⚠️ AVVERSITÀ ESTERNE
⚠️ Errori totali: 0
🔄 Recuperati (fallback): 0
📡 Cadute WiFi: 0

📊 INFORMAZIONI TECNICHE
⏱️ Tempo di attività: 0h 0m
🧠 RAM libera: 223960 bytes
💾 PSRAM libera: 1027820 bytes
💽 FS libero: 13549568 bytes
📦 Max. in FS: 3500 Messaggi
⚡ Modalità Energia: 3 
📶 Segnale WiFi: -64 dBm (Discreto)
🔒 SSL: SICURO
🚀 Turbo: INATTIVO
🤖 Auto-messaggi: SÌ

```

**Perché `/estado` è essenziale:**
- Controllo istantaneo della salute del sistema
- Monitoraggio qualità WiFi (diagnostica problemi di connettività)
- Rilevamento perdite memoria (osserva heap libero)
- Stato coda messaggi (vedi messaggi in attesa/falliti)
- Tracciamento tempo di attività (monitoraggio stabilità)
- Il tuo primo strumento diagnostico

**Suggerimento professionale:** Fai di `/estado` il tuo primo messaggio dopo ogni aggiornamento firmware per verificare che tutto funzioni!

---

## 10. NTP
- Codice proprio per sincronizzare/risincronizzare per SSL. GNSS, LTE e Scheduler (Edizione Enterprise)
---

## 11. Documentazione (7 Lingue)

- **[GETTING_STARTED_IT.md](docs/GETTING_STARTED_IT.md)** - **INIZIA QUI!** Guida completa da quando ricevi l'ESP32-S3 al primo messaggio inviato a Telegram
- **[README_IT.md](docs/README_IT.md)** (questo file) - Panoramica funzionalità, avvio rapido, riferimento API
- **[BENCHMARK.md](docs/BENCHMARK.md)** - Confronto tecnico con 6 librerie Telegram (solo inglese, ma auto-esplicativo)
- **[README_KissOTA_XX.md](docs/README_KissOTA_IT.md)** - Grande valore perché dettaglia i passaggi del sistema di aggiornamento OTA (7 lingue: EN, ES, FR, IT, DE, PT, CN)

**Scegli la tua lingua:** Tutti i messaggi del costruttore inviati a Telegram sono visualizzati in 7 lingue tramite selezione lingua durante la compilazione (lang.h).


## Vantaggi di Sicurezza OTA

KissTelegram OTA è **molto più sicuro dell'architettura Espressif e risparmia spazio sul tuo ESP32S3**:

| Funzionalità | KissTelegram OTA | Espressif OTA |
|---------|------------------|---------------|
| Autenticazione | PIN + PUK | Nessuna |
| Conferma Checksum | CRC32 automatico | Manuale |
| Backup e Rollback | Automatico | Manuale |
| Finestra Validazione | 60s con `/otaok` | Nessuna |
| Rilevamento Loop Avvio | Sì | No |
| Integrazione Telegram | Nativa | Richiede codice personalizzato |
| Ottimizzazione Flash | 13MB SPIFFS | 5MB SPIFFS |

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

### Storage

```cpp
void enableStorage(bool enable = true);
bool saveNow();
bool restoreFromStorage();
void clearStorage();
```

---

## Esempi

Consulta il .ino incluso nella libreria per esplorare alcuni scenari e funzionalità e il mio stile di codifica in KissTelegram. Ancora meglio, decommenta la tua lingua in [lang.h] per ricevere messaggi dai costruttori principali (.cpp) nella tua lingua locale, se tutte le lingue sono commentate i messaggi sono in spagnolo, la lingua predefinita:

Le convenzioni di codice sono in inglese, ma i pensieri e i commenti sono in spagnolo, la mia lingua madre, usa il tuo traduttore online, il codice è facile, all'interno del codice c'è la mia visione e il concetto di KissTelegram...

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
## Configurazione Base
- Rinomina system_setup_template.h in system_setup.h nella tua cartella KissTelegram per iniziare la compilazione.
- Sostituisci le seguenti righe con le tue credenziali.

````cpp
#define KISS_FALLBACK_BOT_TOKEN "YOUR_TELEGRAM_BOT_TOKEN"
#define KISS_FALLBACK_CHAT_ID "YOUR_CHAT_ID number"
#define KISS_FALLBACK_WIFI_SSID "YOUR_WIFI_SSID"
#define KISS_FALLBACK_WIFI_PASSWORD "YOUR_WIFI_PASSWORD"
#define KISS_FALLBACK_OTA_PIN "0000"
#define KISS_FALLBACK_OTA_PUK "00000000"
````

## Licenza

Questo progetto è sotto licenza MIT - vedi il file [LICENSE](LICENSE) per i dettagli.

---

## Architettura, Visione, Concetto, Soluzioni e Design (e responsabile di qualsiasi malfunzionamento, sono io...)

**Vicente Soriano**
Email: victek@gmail.com
GitHub: [victek](https://github.com/victek/KissTelegram)

**Contributori**
- Molti assistenti AI in Traduzioni, Codice, Risoluzione problemi e molte ore cercando di impedire loro di reinventare la ruota.....

---


## Contribuire

I contributi sono benvenuti! Sentiti libero di inviare una Pull Request o inviarmi un'email, ma preferisco un PR così altri possono trovare la tua domanda.

---

## Supporto

Se trovi utile questa libreria, considera di:
- Dare una stella a questo repository
- Segnalare bug tramite GitHub Issues
- Condividere i tuoi progetti che usano KissTelegram
- Parlare ai tuoi conoscenti delle funzionalità e soluzioni di questa libreria
- Proporre casi d'uso ed esperienze con KissTelegram

---
