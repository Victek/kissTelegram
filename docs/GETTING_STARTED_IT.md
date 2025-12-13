# Guida Introduttiva a KissTelegram su ESP32-S3

**Una guida completa per configurare il tuo ESP32-S3 da zero al primo messaggio Telegram**

> ⚠️ **CRITICO**: Leggi completamente questa guida prima di caricare qualsiasi firmware. L'ESP32-S3 N16R8 richiede un **processo di caricamento in due fasi** a causa delle partizioni personalizzate. Saltare i passaggi causerà errori!

---

## Sommario

1. [Prima di Iniziare](#prima-di-iniziare)
2. [Creazione del Tuo Bot Telegram](#creazione-del-tuo-bot-telegram)
3. [Configurazione Hardware](#configurazione-hardware)
4. [Configurazione Arduino IDE](#configurazione-arduino-ide)
5. [Primo Caricamento (Problemi Comuni)](#primo-caricamento-problemi-comuni)
6. [File di Configurazione](#file-di-configurazione)
7. [Successo! Cosa Fare Dopo?](#successo-cosa-fare-dopo)

---

## Prima di Iniziare

### Cosa Ti Serve

- **ESP32-S3 N16R8** (Flash 16MB / PSRAM 8MB)
- **Due cavi USB-C** (per alternare tra la porta bootloader e OTG)
- **Arduino IDE 2.x** o più recente
- **PC Windows** (questa guida è focalizzata su Windows, adatta i percorsi per Linux/Mac)
- **Account Telegram** sul tuo telefono

### Cosa Lo Rende Diverso

Il tuo nuovo ESP32-S3 N16R8 arriva con un programma demo per LED RGB. KissTelegram **sostituirà completamente la tabella delle partizioni** per massimizzare i tuoi 16MB di flash:

| Partizione | Default Espressif | Custom KissTelegram |
|-----------|-------------------|---------------------|
| App Space | 1,5 MB | 4,5 MB (3x più grande!) |
| File System | 5 MB | 13 MB (2,6x più grande!) |
| Total Used | 6,5 MB | 17,5 MB |

Questo è il motivo per cui il processo di caricamento in due fasi è richiesto: **la tabella delle partizioni cambia tra i caricamenti**.

---

## Creazione del Tuo Bot Telegram

### Passaggio 1: Parla con BotFather

1. Apri Telegram sul tuo telefono
2. Cerca `@BotFather` (bot ufficiale, ha spunta di verifica blu)
3. Inizia una conversazione con `/start`
4. Crea il tuo bot con `/newbot`
5. Scegli un nome (esempio: "My Home Assistant")
6. Scegli un nome utente (deve terminare in `bot`, esempio: "myhome_assistant_bot")
7. **Salva il tuo Bot Token** - assomiglia a: `1234567890:ABCdefGHIjklMNOpqrsTUVwxyz`

### Passaggio 2: Ottieni il Tuo Chat ID

**Metodo 1: Usando un Bot (Più Facile)**

1. Cerca `@userinfobot` in Telegram
2. Inizia conversazione con `/start`
3. Risponderà con il tuo **Chat ID** (un numero come `123456789`)
4. **Salva questo numero** - ti servirà nella configurazione

**Metodo 2: Usando il Browser Web**

1. Invia un messaggio qualsiasi al tuo bot appena creato
2. Apri il browser e visita:
   ```
   https://api.telegram.org/bot<YOUR_BOT_TOKEN>/getUpdates
   ```
   (Sostituisci `<YOUR_BOT_TOKEN>` con il tuo token effettivo)
3. Cerca `"chat":{"id":123456789` nella risposta JSON
4. Quel numero è il tuo **Chat ID**

**✅ Ora hai:**
- Bot Token: `1234567890:ABCdefGHIjklMNOpqrsTUVwxyz`
- Chat ID: `123456789`

Mantienili al sicuro! Te ne serviranno presto.

---

## Configurazione Hardware

### Comprendere le Due Porte USB-C

Il tuo ESP32-S3 N16R8 ha **due porte USB-C**:

```
┌─────────────────────┐
│  ┌─┐         ESP32  │
│  │•│  ← Power LED    │
│  └─┘                 │
│  [USB-C]  ← RIGHT PORT (Bootloader/Upload)
│                      │
│                      │
│  [USB-C]  ← LEFT PORT (OTG/Normal Operation)
│                      │
└─────────────────────┘
```

**PORTA DESTRA (vicino al Power LED):**
- Usata per **caricamento iniziale del firmware**
- Usata per **modalità bootloader**
- Usa questa quando Arduino IDE dice "Connecting..."

**PORTA SINISTRA (OTG):**
- Usata per **normale operazione** dopo il primo caricamento
- Usata per **secondo caricamento** (correzione partizione)
- Usa questa per Serial Monitor durante il normale funzionamento

---

## Configurazione Arduino IDE

### Passaggio 1: Mostra File Nascosti (Windows)

1. Apri **Esplora File**
2. Fai clic sulla scheda **Visualizza** → **Mostra** → Seleziona:
   - ✅ Estensioni nomi file
   - ✅ Elementi nascosti
3. Nella scheda **Filtro**: **Tutti i tipi di file**

### Passaggio 2: Modifica boards.txt

1. Naviga verso:
   ```
   C:\Users\<YOUR_USERNAME>\AppData\Local\Arduino15\packages\esp32\hardware\esp32\3.3.4\
   ```
   (Sostituisci `3.3.4` con la tua versione del core ESP32 se diversa)

2. Trova e apri `boards.txt` (usa Notepad++ o qualsiasi editor di testo)

3. Premi `Ctrl+F` e cerca:
   ```
   gen4esp32_4MBapp_4MBota_7MBspiffs
   ```

4. **Immediatamente sotto quella riga**, incolla queste tre righe:
   ```
   gen4-ESP32-S3R8n16.menu.PartitionScheme.gen4esp32scheme2=Custom (4MB APP/12MB LtlFS)
   gen4-ESP32-S3R8n16.menu.PartitionScheme.gen4esp32scheme2.build.custom_partitions=partitions
   gen4-ESP32-S3R8n16.menu.PartitionScheme.gen4esp32scheme2.upload.maximum_size=4718592
   ```

5. **Salva** e chiudi `boards.txt`

6. Se Arduino IDE era aperto, **chiudilo e riavvialo**

### Passaggio 3: Configura Arduino IDE

1. **Apri** la cartella del tuo sketch KissTelegram (con `.ino`, `.h`, `.cpp`, e `partitions.csv`)

2. In Arduino IDE, vai a **Strumenti** → **Scheda** → **4D Systems gen4-ESP32-S3R8n16**

3. **Strumenti** → **Ricarica Dati Scheda** (vedrai una conferma in fondo allo schermo)

4. **Configura tutte le opzioni del menu Strumenti:**

   | Impostazione | Valore |
   |---------|-------|
   | **Scheda** | 4D Systems gen4-ESP32-S3R8n16 |
   | **USB CDC On Boot** | Enabled |
   | **Flash Size** | 16MB (128Mb) |
   | **Partition Scheme** | **Custom (4MB APP/12MB LtlFS)** ⚠️ |
   | **PSRAM** | OPI PSRAM |
   | **Upload Speed** | 921600 |
   | **Erase All Flash Before Sketch Upload** | **Enabled** ⚠️ |

   ⚠️ **Impostazioni critiche** - verifica di nuovo queste!

5. **Strumenti** → **Serial Monitor** → Imposta il baud rate a **115200**

---

## Primo Caricamento (Problemi Comuni)

### Perché Sono Necessari Due Caricamenti

**Il Problema:**
- Primo caricamento: Arduino usa **tabella delle partizioni vecchia** per scrivere il firmware
- ESP32 avvia: Trova **tabella delle partizioni nuova** (da `partitions.csv`)
- **Mancata corrispondenza** tra dove il firmware è stato scritto vs dove ESP32 lo cerca
- Risultato: Errori di avvio, errori di partizione, crash

**La Soluzione:**
Due caricamenti garantiscono che il firmware sia scritto nella **posizione corretta** definita dalla nuova tabella delle partizioni.

---

### Caricamento #1: Flash Iniziale (Attendi Errori!)

1. **Connetti la porta USB-C DESTRA** (vicino al Power LED) al tuo PC

2. **Seleziona la porta**: Strumenti → Porta → Seleziona la porta COM che appare

3. **Verifica le impostazioni**:
   - ✅ Erase All Flash Before Sketch Upload: **Enabled**
   - ✅ Partition Scheme: **Custom (4MB APP/12MB LtlFS)**

4. **Premi Upload** (`Ctrl+U` o pulsante ➡️)

5. **Attendi ~2-3 minuti** (cancellazione + caricamento)

6. **Risultato atteso**:
   ```
   ✅ Upload successful
   ```

7. **Apri Serial Monitor** - vedrai errori come:
   ```
   ❌ E (123) boot: No factory partition found
   ❌ E (456) esp_image: Image length 12345 doesn't fit in partition length 67890
   ❌ E (789) boot: Failed to verify app partition
   Guru Meditation Error: Core 0 panic'ed (LoadProhibited)
   ```

8. **Se hai configurato `system_setup.h` correttamente**: Potresti ricevere il **primo messaggio Telegram** (ma Serial mostrerà errori)

**Non farti prendere dal panico!** Questi errori sono **attesi** e **normali**. Continua al Caricamento #2.

---

### Caricamento #2: Correzione Partizione (Errori Scompaiono)

1. **Disconnetti la porta USB-C DESTRA**

2. **Connetti la porta USB-C SINISTRA** (porta OTG) al tuo PC

3. **Seleziona la nuova porta**: Strumenti → Porta → Seleziona la nuova porta COM
   - **Importante**: Il numero della porta cambierà! Guarda in Serial Monitor per i dati per confermare la porta corretta.

4. **Verifica di nuovo le impostazioni**:
   - ✅ Erase All Flash Before Sketch Upload: **Enabled**
   - ✅ Partition Scheme: **Custom (4MB APP/12MB LtlFS)**

5. **Premi Upload di nuovo** (`Ctrl+U`)

6. **Attendi ~2-3 minuti** (cancellazione + caricamento)

7. **Apri Serial Monitor** - dovresti ora vedere:
   ```
   ✅ KissTelegram v1.x.x
   ✅ WiFi connected
   ✅ Telegram bot enabled
   ✅ System ready
   ```

8. **Controlla Telegram** - riceverai il messaggio di benvenuto:
   ```
   📦 Hello! KissTelegram is ready.
   🔌 Build: 2025-12-12 10:30:45 (0xABCD1234)
   📡 WiFi Signal: -59 dBm (Good)
   ✅ 0 messages queued
   ```

**Successo!** Il tuo ESP32-S3 è ora in esecuzione con KissTelegram con partizioni corrette.

---

### Caricamenti Futuri

**Buone notizie:** Dopo i due caricamenti iniziali, tutti i caricamenti futuri funzionano normalmente:

- Usa **porta USB-C SINISTRA** (OTG)
- **Non è necessario** cancellare tutto il Flash (a meno che tu non voglia una pulizia)
- Carica una volta e funziona immediatamente

---

## File di Configurazione

### system_setup.h (Richiesto Prima del Primo Caricamento!)

**Prima di compilare:**

1. Naviga verso la cartella di KissTelegram
2. Trova `system_setup_template.h`
3. **Rinomina** in `system_setup.h`
4. **Apri** `system_setup.h` e riempi:

```cpp
// Your Telegram Bot (from BotFather)
#define KISS_FALLBACK_BOT_TOKEN "1234567890:ABCdefGHIjklMNOpqrsTUVwxyz"

// Your Chat ID (from @userinfobot)
#define KISS_FALLBACK_CHAT_ID "123456789"

// Your WiFi credentials
#define KISS_FALLBACK_WIFI_SSID "YourWiFiName"
#define KISS_FALLBACK_WIFI_PASSWORD "YourWiFiPassword"

// OTA Security (change default PIN/PUK!)
#define KISS_FALLBACK_OTA_PIN "0000"        // 4 digits
#define KISS_FALLBACK_OTA_PUK "00000000"    // 8 digits
```

5. **Salva** il file

**⚠️ Avviso di Sicurezza:** Cambia il PIN predefinito (`0000`) e PUK (`00000000`) con i tuoi segreti!

---

### lang.h (Facoltativo: Scegli La Tua Lingua)

KissTelegram supporta 7 lingue per i messaggi di sistema:

```cpp
// In lang.h, uncomment ONE language:

// #define LANG_CN  // 中文 (Chinese)
// #define LANG_DE  // Deutsch (German)
// #define LANG_EN  // English
// #define LANG_FR  // Français (French)
// #define LANG_IT  // Italiano (Italian)
// #define LANG_PT  // Português (Portuguese)
// #define LANG_ES  // Español (Spanish) - DEFAULT if all commented
```

Scegli la tua lingua **prima di compilare** per messaggi localizzati.

---

## Successo! Cosa Fare Dopo?

### Verifica che Tutto Funzioni

1. **Invia `/estado` al tuo bot** in Telegram - riceverai un rapporto di stato dettagliato:
   ```
   📦 KissTelegram v1.x.x
   🎯 SYSTEM RELIABILITY
   ✅ System: RELIABLE
   ✅ Messages sent: 2
   💾 Messages pending: 0
   📡 WiFi Signal: -59 dBm (Good)
   🔋 Uptime: 123 seconds
   💾 Free memory: 223 KB
   ```

2. **Controlla Serial Monitor** - non dovrebbe mostrare errori

3. **Prova i comandi**:
   - `/start` - Messaggio di benvenuto
   - `/help` - Comandi disponibili
   - `/estado` - Stato del sistema (health check)

---

### Comprendere gli Aggiornamenti OTA

Una volta che KissTelegram è in esecuzione, puoi aggiornare il firmware **via Telegram** (senza cavo USB!):

1. Invia `/ota` al tuo bot
2. Inserisci PIN: `/otapin 0000` (o il tuo PIN personalizzato)
3. **Invia il tuo file di firmware `.bin`** (trascina e rilascia in Telegram)
4. Il bot verifica il checksum automaticamente
5. Conferma: `/otaconfirm`
6. ESP32 si riavvia con il nuovo firmware
7. **Entro 60 secondi**, invia `/otaok` per confermare che funziona
8. Se non confermi, ESP32 **esegue automaticamente un rollback** al firmware precedente!

📖 **Leggi di più:** Vedi `README_KissOTA_IT.md` (o la tua lingua) per la documentazione completa di OTA.

---

### Esplora il Codice di Esempio

L'esempio `suite_kiss.ino` dimostra:

- ✅ Gestione WiFi con monitoraggio della qualità
- ✅ Coda di messaggi con priorità
- ✅ Modalità di gestione dell'alimentazione
- ✅ Gestione dei comandi (`/start`, `/help`, `/estado`, ecc.)
- ✅ Aggiornamenti OTA via Telegram
- ✅ Ripristino da crash e persistenza
- ✅ Connessioni sicure SSL/TLS

**Pro tip:** Usa il comando `/estado` come il tuo **strumento di monitoraggio della salute** - è la tua finestra sugli interni di KissTelegram!

---

### Risoluzione Comune dei Problemi

**Problema: "Port not found" o "Access denied"**
- Windows ha bloccato la porta. Scollega l'USB, attendi 5s, ricollega.
- Prova un cavo USB diverso (alcuni sono solo per la carica, non per i dati)

**Problema: "Timeout waiting for device" durante il caricamento**
- Porta USB sbagliata! Ricorda: porta DESTRA per il primo caricamento, porta SINISTRA per il secondo
- Tieni premuto il pulsante BOOT su ESP32 mentre fai clic su Upload, rilascia dopo che appare "Connecting..."

**Problema: Serial Monitor mostra caratteri di spazzatura**
- Baud rate sbagliato. Imposta a **115200** nel dropdown di Serial Monitor

**Problema: Il bot non risponde in Telegram**
- Verifica che `system_setup.h` abbia il Bot Token e Chat ID corretti
- Verifica che le credenziali WiFi siano corrette
- Apri Serial Monitor e cerca i messaggi di connessione WiFi

**Problema: Errore di compilazione "Partition table does not fit"**
- Non hai aggiunto la partizione personalizzata a `boards.txt` correttamente
- Oppure non hai selezionato "Custom (4MB APP/12MB LtlFS)" in Strumenti → Partition Scheme

---

### Ottieni Ulteriore Aiuto

- 📧 **Email**: victek@gmail.com
- 📖 **Documentazione**: Vedi tutti i file `README_*.md` nella cartella KissTelegram
- 🐛 **Rapporti di Bug**: Problemi GitHub (link nel README.md principale)
- 💡 **Richieste di Funzionalità**: Anche benvenute tramite email o GitHub!

---

## Sommario: Il Processo Completo

```
1. Ottieni Bot Token + Chat ID da Telegram ✅
2. Modifica boards.txt (aggiungi partizione personalizzata) ✅
3. Configura Arduino IDE (partizione personalizzata, cancellazione abilitata) ✅
4. Modifica system_setup.h (credenziali) ✅
5. Connetti porta USB DESTRA ✅
6. Caricamento #1 (attendi errori) ✅
7. Disconnetti DESTRA, connetti porta USB SINISTRA ✅
8. Caricamento #2 (errori scompaiono!) ✅
9. Ricevi il messaggio di benvenuto in Telegram ✅
10. Invia /estado per verificare che tutto funzioni ✅
```

**Sei pronto a costruire progetti straordinari con KissTelegram!** 🎉

---

**Buona programmazione!**

*Vicente Soriano - victek@gmail.com*
