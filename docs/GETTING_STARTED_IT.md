# Primi Passi con KissTelegram su ESP32-S3

**Guida completa per configurare il tuo ESP32-S3 da zero fino al primo messaggio Telegram**

> ⚠️ **CRITICO**: Leggi questa guida completamente prima di caricare qualsiasi firmware. L'ESP32-S3 N16R8 richiede un **processo di caricamento in due passaggi** a causa delle partizioni personalizzate. Saltare passaggi causerà errori!

---

## Indice

1. [Prima di Iniziare](#prima-di-iniziare)
2. [Crea il Tuo Bot Telegram](#crea-il-tuo-bot-telegram)
3. [Configurazione Hardware](#configurazione-hardware)
4. [Configurazione Arduino IDE](#configurazione-arduino-ide)
5. [Primo Caricamento (Creare partizioni con esptool)](#primo-caricamento)
6. [File di Configurazione](#file-di-configurazione)
7. [Successo! E Adesso?](#successo-e-adesso)

---

## Prima di Iniziare

### Cosa Ti Serve

- **ESP32-S3 N16R8** (16MB Flash / 8MB PSRAM)
- **Due cavi USB-C** (per alternare tra porte bootloader e OTG)
- **Arduino IDE 2.x** o superiore
- **PC Windows** (questa guida è focalizzata su Windows, adatta i percorsi per Linux/Mac)
- **Account Telegram** sul tuo telefono

### Cosa Rende Questo Diverso

Il tuo nuovo ESP32-S3 N16R8 arriva con un'app demo LED RGB integrata. KissTelegram **sostituisce completamente la tabella delle partizioni** per massimizzare i tuoi 16MB di flash:

| Partizione | Default Espressif | KissTelegram Personalizzato |
|------------|-------------------|-----------------------------|
| Spazio App | 1.5 MB | 4.5 MB (3x più grande!) |
| File System | 5 MB | 13 MB (2.6x più grande!) |
| Totale Usato | 6.5 MB | 17.5 MB |

Ecco perché il processo a due caricamenti è richiesto: **la tabella delle partizioni cambia tra i caricamenti**.

---

## Crea il Tuo Bot Telegram

### Passo 1: Parla con BotFather

1. Apri Telegram sul tuo telefono
2. Cerca `@BotFather` (bot ufficiale, ha il segno di spunta blu)
3. Inizia la conversazione con `/start`
4. Crea il tuo bot con `/newbot`
5. Scegli un nome (esempio: "Il Mio Assistente Casa")
6. Scegli un nome utente (deve finire con `bot`, esempio: "ilmio_assistente_casa_bot")
7. **Salva il tuo Bot Token** - appare così: `1234567890:ABCdefGHIjklMNOpqrsTUVwxyz`

### Passo 2: Ottieni il Tuo Chat ID

**Metodo 1: Usando un Bot (Più Facile)**

1. Cerca `@ChatIDHelperBot` in Telegram
2. Inizia la conversazione con `/start`
3. Risponderà con il tuo **Chat ID** (un numero come `123456789`)
4. **Salva questo numero** - ti servirà nella configurazione

**Metodo 2: Usando un Browser Web**

1. Invia qualsiasi messaggio al tuo bot appena creato
2. Apri il browser e visita:
   ```
   https://api.telegram.org/bot<IL_TUO_BOT_TOKEN>/getUpdates
   ```
   (Sostituisci `<IL_TUO_BOT_TOKEN>` con il tuo token reale)
3. Cerca `"chat":{"id":123456789` nella risposta JSON
4. Quel numero è il tuo **Chat ID**

**✅ Ora hai:**
- Bot Token: `1234567890:ABCdefGHIjklMNOpqrsTUVwxyz`
- Chat ID: `123456789`

Conservali al sicuro! Ti serviranno presto.

---

## Configurazione Hardware

### Comprendere le Due Porte USB-C

Il tuo ESP32-S3 N16R8 ha **due porte USB-C**:

```
┌─────────────────────┐
│  ┌─┐         ESP32  │
│  │•│  ← LED Power    │
│  └─┘                 │
│  [USB-C]  ← PORTA DESTRA (Bootloader/Caricamento)
│                      │
│                      │
│  [USB-C]  ← PORTA SINISTRA (OTG/Operazione Normale)
│                      │
└─────────────────────┘
```

**PORTA DESTRA (vicino al LED Power):**
- Usata per il **caricamento iniziale del firmware**
- Usata per la **modalità bootloader**
- Usa questa quando Arduino IDE dice "Connecting..."

**PORTA SINISTRA (OTG):**
- Usata per l'**operazione normale** dopo il primo caricamento
- Usata per il **secondo caricamento** (correzione partizione)
- Usa questa per il Monitor Seriale in operazione normale

---

## Configurazione Arduino IDE

### Passo 1: Mostra File Nascosti (Windows)

1. Apri **Esplora File**
2. Clicca sulla scheda **Visualizza** → **Mostra** → Seleziona:
   - ✅ Estensioni nomi file
   - ✅ Elementi nascosti
3. Nella scheda **Filtro**: **Tutti i tipi di file**

### Passo 2: Modificare boards.txt

1. Naviga verso:
   ```
   C:\Users\<IL_TUO_NOME_UTENTE>\AppData\Local\Arduino15\packages\esp32\hardware\esp32\3.3.4\
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

6. Se Arduino IDE era aperto, **chiudi e riavvialo**

### Passo 3: Configurare Arduino IDE

1. **Apri** la tua cartella sketch KissTelegram (con `.ino`, `.h`, `.cpp`, e `partitions.csv`)

2. In Arduino IDE, vai a **Strumenti** → **Scheda** → **4D Systems gen4-ESP32-S3R8n16**

3. **Strumenti** → **Ricarica Dati Scheda** (vedrai conferma in basso)

4. **Configura tutte le opzioni del menu Strumenti:**

   | Impostazione | Valore |
   |--------------|--------|
   | **Scheda** | 4D Systems gen4-ESP32-S3R8n16 |
   | **USB CDC On Boot** | Enabled |
   | **Flash Size** | 16MB (128Mb) |
   | **Partition Scheme** | **Custom (4MB APP/12MB LtlFS)** ⚠️ |
   | **PSRAM** | OPI PSRAM |
   | **Upload Speed** | 921600 |
   | **Erase All Flash Before Sketch Upload** | **Enabled** ⚠️ |

   ⚠️ **Impostazioni critiche** - controlla due volte!

5. **Strumenti** → **Monitor Seriale** → Imposta la velocità a **115200**

---

## Primo Caricamento (Problemi Comuni)

### Perché Sono Necessari Due Caricamenti

**Il Problema:**
- Primo caricamento: Arduino usa la **vecchia tabella di partizioni** per scrivere il firmware
- ESP32 si avvia: Trova la **nuova tabella di partizioni** (da `partitions.csv`)
- **Discrepanza** tra dove il firmware è stato scritto vs dove l'ESP32 lo cerca
- Risultato: Errori di avvio, errori di partizione, crash

**La Soluzione:**
Due caricamenti assicurano che il firmware sia scritto nella **posizione corretta** definita dalla nuova tabella di partizioni.

---

### Caricamento #1: Flash Iniziale (Masterizza Bootloader)

1. **Connetti la porta USB-C DESTRA** (vicino al LED Power) al tuo PC

2. **Seleziona la porta**: Strumenti → Porta → Seleziona la porta COM che appare

3. **Verifica le impostazioni**:
   - ✅ Erase All Flash Before Sketch Upload: **Enabled**
   - ✅ Partition Scheme: **Custom (4MB APP/12MB LtlFS)**
   

4. **Strumenti, Masterizza il Bootloader** (Clicca su questa opzione)
   - ✅ Strumenti ➡️, alla fine del menu a tendina trova 'Masterizza il Bootloader'
   - ✅ Clicca qui e scriverà la nuova partizione, usando esptool
   - Richiede 53.6 secondi e hai la nuova partizione per KissTelegram 

Continua con Caricamento #2.

---

### Caricamento #2: Caricamento Sketch

1. **Disconnetti la porta USB-C DESTRA**

2. **Connetti la porta USB-C SINISTRA** (porta OTG) al tuo PC

3. **Seleziona la nuova porta**: Strumenti → Porta → Seleziona la nuova porta COM
   - **Importante**: Il numero di porta cambierà! Cerca dati nel Monitor Seriale per confermare la porta corretta, per esempio, premi reset dell'ESP32s3 finché non vedi dati come risposta

4. **Verifica di nuovo le impostazioni**:
   - ✅ Erase All Flash Before Sketch Upload: **Enabled**
   - ✅ Partition Scheme: **Custom (4MB APP/12MB LtlFS)**

5. **Premi di nuovo Carica** (`Ctrl+U`)

6. **Aspetta ~2-3 minuti** (cancellazione + caricamento)

7. **Apri Monitor Seriale** - ora dovresti vedere (se hai configurato correttamente le credenziali 
in system_setup.h (il system_setup_template rinominato)):
   ```
   ✅ KissTelegram v0.9.x
   ✅ WiFi connesso
   ✅ Bot Telegram abilitato
   ✅ Sistema pronto
   ```

8. **Controlla Telegram** - riceverai il messaggio di benvenuto:
   ```
   📦 Ciao! KissTelegram è pronto.
   🔌 Build: 2025-12-12 10:30:45 (0xABCD1234)
   📡 Segnale WiFi: -59 dBm (Buono)
   ✅ 0 messaggi in coda
   ```

**Successo!** Il tuo ESP32-S3 ora esegue KissTelegram con le partizioni corrette.

---

### Caricamenti Futuri

**Buone notizie:** Dopo i due caricamenti iniziali, tutti i futuri caricamenti funzionano normalmente:

- Usa la **porta USB-C SINISTRA** (OTG)
- **Non serve** "Erase All Flash" più (a meno che tu non abbia fatto modifiche ai dati NVRAM)
- Carica una volta e funziona immediatamente

---

## File di Configurazione

### system_setup.h (Richiesto Prima del Primo Caricamento!)

**Prima di compilare:**

1. Naviga nella tua cartella KissTelegram
2. Trova `system_setup_template.h`
3. **Rinominalo** in `system_setup.h`
4. **Apri** `system_setup.h` e compila:

```cpp
// Il Tuo Bot Telegram (da BotFather)
#define KISS_FALLBACK_BOT_TOKEN "1234567890:ABCdefGHIjklMNOpqrsTUVwxyz"

// Il Tuo Chat ID (da @userinfobot)
#define KISS_FALLBACK_CHAT_ID "123456789"

// Le Tue credenziali WiFi
#define KISS_FALLBACK_WIFI_SSID "NomeDelTuoWiFi"
#define KISS_FALLBACK_WIFI_PASSWORD "PasswordDelTuoWiFi"

// Sicurezza OTA (cambia PIN/PUK predefiniti!)
#define KISS_FALLBACK_OTA_PIN "0000"        // 4 cifre
#define KISS_FALLBACK_OTA_PUK "00000000"    // 8 cifre
```

5. **Salva** il file

**⚠️ Avviso di Sicurezza:** Cambia il PIN predefinito (`0000`) e il PUK (`00000000`) con i tuoi segreti!

---

### lang.h (Opzionale: Scegli la Tua Lingua)

KissTelegram supporta 7 lingue per i messaggi di sistema:

```cpp
// In lang.h, decommenta UNA lingua:

// #define LANG_CN  // 中文 (Cinese)
// #define LANG_DE  // Deutsch (Tedesco)
// #define LANG_EN  // English (Inglese)
// #define LANG_FR  // Français (Francese)
// #define LANG_IT  // Italiano (Italiano)
// #define LANG_PT  // Português (Portoghese)
// #define LANG_ES  // Español (Spagnolo) - PREDEFINITO se tutti commentati
```

Scegli la tua lingua **prima di compilare** per messaggi localizzati.

---

## Successo! E Adesso?

### Verifica Che Tutto Funzioni

1. **Invia `/status` al tuo bot** in Telegram - otterrai un rapporto di stato dettagliato:
   ```
   📦 KissTelegram v1.x.x
   🎯 AFFIDABILITÀ DEL SISTEMA
   ✅ Sistema: AFFIDABILE
   ✅ Messaggi inviati: 2
   💾 Messaggi in sospeso: 0
   📡 Segnale WiFi: -59 dBm (Buono)
   🔋 Tempo di attività: 123 secondi
   💾 Memoria libera: 223 KB
   ```

2. **Controlla Monitor Seriale** - non dovrebbe mostrare errori

3. **Testa i comandi**:
   - `/start` - Messaggio di benvenuto
   - `/help` - Comandi disponibili
   - `/status` - Stato del sistema (controllo salute)

---

### Comprendere gli Aggiornamenti OTA

Una volta che KissTelegram è in esecuzione, puoi aggiornare il firmware **via Telegram** (senza cavo USB!):

1. Invia `/ota` al tuo bot
2. Inserisci PIN: `/otapin 0000` (o il tuo PIN personalizzato)
3. **Invia il tuo file firmware `.bin`** (trascina e rilascia in Telegram)
4. Il bot verifica automaticamente il checksum
5. Conferma: `/otaconfirm`
6. L'ESP32 si riavvia con il nuovo firmware
7. **Entro 60 secondi**, invia `/otaok` per confermare che funziona
8. Se non confermi, l'ESP32 **torna automaticamente** al firmware precedente!

📖 **Leggi di più:** Vedi `README_KissOTA_IT.md` per la documentazione OTA completa.

---

### Esplora il Codice di Esempio

L'esempio `suite_kiss.ino` dimostra:

- ✅ Gestione WiFi con monitoraggio qualità
- ✅ Coda messaggi con priorità
- ✅ Modi di gestione energetica
- ✅ Gestione comandi (`/start`, `/help`, `/status`, ecc.)
- ✅ Aggiornamenti OTA via Telegram
- ✅ Recupero crash e persistenza
- ✅ Connessioni sicure SSL/TLS

**Suggerimento pro:** Usa il comando `/status` come tuo **strumento di monitoraggio salute** - è la tua finestra negli interni di KissTelegram!

---

### Risoluzione Problemi Comuni

**Problema: "Porta non trovata" o "Accesso negato"**
- Windows ha bloccato la porta. Disconnetti USB, aspetta 5s, riconnetti.
- Prova un cavo USB diverso (alcuni sono solo per carica, non per dati)

**Problema: "Timeout in attesa del dispositivo" durante il caricamento**
- Porta USB sbagliata! Ricorda: porta DESTRA per primo caricamento, porta SINISTRA per secondo
- Tieni premuto il pulsante BOOT sull'ESP32 mentre clicchi su Carica, rilascia dopo che appare "Connecting..."

**Problema: Monitor Seriale mostra caratteri spazzatura**
- Baud rate sbagliato. Imposta a **115200** nel menu a tendina del Monitor Seriale

**Problema: Il bot non risponde in Telegram**
- Verifica che `system_setup.h` abbia il Bot Token e Chat ID corretti
- Verifica che le credenziali WiFi siano corrette
- Apri Monitor Seriale e cerca messaggi di connessione WiFi

**Problema: Errore di compilazione "La tabella di partizioni non si adatta"**
- Non hai aggiunto la partizione personalizzata a `boards.txt` correttamente
- O non hai selezionato "Custom (4MB APP/12MB LtlFS)" in Strumenti → Partition Scheme

---

### Ottieni Più Aiuto

- 📧 **Email**: victek@gmail.com
- 📖 **Documentazione**: Vedi tutti i file `README_*.md` nella tua cartella KissTelegram
- 🐛 **Segnalazione Bug**: GitHub issues (link nel README.md principale)
- 💡 **Richieste Funzionalità**: Anche benvenute via email o GitHub!

---

## Riepilogo: Il Processo Completo

```
1. Ottenere Bot Token + Chat ID da Telegram ✅
2. Modificare boards.txt (aggiungere partizione personalizzata) ✅
3. Configurare Arduino IDE (Partizione Custom, Erase abilitato) ✅
4. Modificare system_setup.h (credenziali) ✅
5. Connettere porta USB DESTRA ✅
6. Caricamento #1 (Masterizza Bootloader) ✅
7. Disconnettere DESTRA, connettere porta USB SINISTRA ✅
8. Caricamento #2 (Carica Sketch KissTelegram) ✅
9. Ricevere messaggio di benvenuto in Telegram ✅
10. Inviare /status per verificare che tutto funzioni ✅
```

**Sei pronto a costruire progetti incredibili con KissTelegram!** 🎉
