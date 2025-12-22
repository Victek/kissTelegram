# Primi Passi con KissTelegram su ESP32-S3

**Guida completa per configurare il tuo ESP32-S3 da zero fino al primo messaggio Telegram**

> ⚠️ **CRITICO**: Leggi completamente questa guida prima di caricare qualsiasi firmware. L'ESP32-S3 N16R8 richiede un **processo di caricamento in due fasi** a causa delle partizioni personalizzate. Saltare i passaggi causerà errori!

---

## Indice

1. [Prima di Iniziare](#prima-di-iniziare)
2. [Creare il Tuo Bot Telegram](#creare-il-tuo-bot-telegram)
3. [Configurazione Hardware](#configurazione-hardware)
4. [Configurazione Arduino IDE](#configurazione-arduino-ide)
5. [Primo Caricamento (Creare partizioni con Arduino IDE)](#primo-caricamento)
6. [File di Configurazione](#file-di-configurazione)
7. [Successo! E Adesso?](#successo-e-adesso)

---

## Prima di Iniziare

### Cosa Ti Serve

- **ESP32-S3 N16R8** (16MB Flash / 8MB PSRAM)
- **Due cavi USB-C** (per alternare tra le porte bootloader e OTG)
- **Arduino IDE 2.x** o superiore
- **PC Windows** (questa guida è focalizzata su Windows, adatta i percorsi per Linux/Mac)
- **Account Telegram** sul tuo telefono

### Cosa Rende Questo Diverso

Il tuo nuovo ESP32-S3 N16R8 arriva con un'app demo LED RGB integrata. KissTelegram **sostituisce completamente la tabella delle partizioni** per massimizzare la tua flash da 16MB:

| Partizione | Espressif Standard | KissTelegram Personalizzato |
|-----------|-------------------|----------------------------|
| Spazio App | 1.5 MB | 4.5 MB (3x più grande!) |
| File System | 5 MB | 13 MB (2.6x più grande!) |
| Totale Usato | 6.5 MB | 17.5 MB |

Ecco perché è richiesto il processo di due caricamenti: **la tabella delle partizioni cambia tra i caricamenti**.

---

## Creare il Tuo Bot Telegram

### Passo 1: Parla con BotFather

1. Apri Telegram sul tuo telefono
2. Cerca `@BotFather` (bot ufficiale, ha il segno di spunta blu)
3. Inizia la conversazione con `/start`
4. Crea il tuo bot con `/newbot`
5. Scegli un nome (esempio: "Il Mio Assistente Domestico")
6. Scegli un nome utente (deve terminare in `bot`, esempio: "ilmioassistente_domestico_bot")
7. **Salva il tuo Token del Bot** - assomiglia a: `1234567890:ABCdefGHIjklMNOpqrsTUVwxyz`

### Passo 2: Ottieni il Tuo ID Chat

**Metodo 1: Usando un Bot (Più Facile)**

1. Cerca `@ChatIDHelperBot` su Telegram
2. Inizia la conversazione con `/start`
3. Ti risponderà con il tuo **ID Chat** (un numero come `123456789`)
4. **Salva questo numero** - ti servirà nella configurazione

**Metodo 2: Usando il Browser Web**

1. Invia qualsiasi messaggio al tuo bot appena creato
2. Apri il browser e visita:
   ```
   https://api.telegram.org/bot<TUO_TOKEN_BOT>/getUpdates
   ```
   (Sostituisci `<TUO_TOKEN_BOT>` con il tuo token effettivo)
3. Cerca `"chat":{"id":123456789` nella risposta JSON
4. Quel numero è il tuo **ID Chat**

**✅ Ora hai:**
- Token del Bot: `1234567890:ABCdefGHIjklMNOpqrsTUVwxyz`
- ID Chat: `123456789`

Conservali al sicuro! Ne avrai bisogno presto.

---

## Configurazione Hardware

### Comprendere le Due Porte USB-C

Il tuo ESP32-S3 N16R8 ha **due porte USB-C**:

```
┌─────────────────────┐
│  ┌─┐         ESP32  │
│  │•│  ← LED Alimentazione
│  └─┘                 │
│  [USB-C]  ← PORTA DESTRA (Bootloader/Caricamento)
│                      │
│                      │
│  [USB-C]  ← PORTA SINISTRA (OTG/Funzionamento Normale)
│                      │
└─────────────────────┘
```

**PORTA DESTRA (vicino al LED Alimentazione):**
- Usata per **caricamento iniziale del firmware**
- Usata per **modalità bootloader**
- Usa questa quando Arduino IDE dice "Connessione..."

**PORTA SINISTRA (OTG):**
- Usata per **funzionamento normale** dopo il primo caricamento
- Usata per **secondo caricamento** (correzione partizione)
- Usa questa per il Monitor Seriale nel funzionamento normale

---

## Configurazione Arduino IDE

### Passo 1: Mostrare File Nascosti (Windows)

1. Apri **Esplora File**
2. Clicca sulla scheda **Visualizza** → **Mostra** → Spunta:
   - ✅ Estensioni nomi file
   - ✅ Elementi nascosti
3. Nella scheda **Filtro**: **Tutti i tipi di file**

### Passo 2: Modificare boards.txt

1. Naviga a:
   ```
   C:\Users\<TUO_NOME_UTENTE>\AppData\Local\Arduino15\packages\esp32\hardware\esp32\3.3.4\
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

### Passo 3: Configurare Arduino IDE

1. **Apri** la tua cartella sketch KissTelegram (con `.ino`, `.h`, `.cpp`, e `partitions.csv`)

2. In Arduino IDE, vai a **Strumenti** → **Scheda** → **4D Systems gen4-ESP32-S3R8n16**

3. **Configura tutte le opzioni del menu Strumenti:**

   | Impostazione | Valore |
   |---------|-------|
   | **Scheda** | 4D Systems gen4-ESP32-S3R8n16 |
   | **USB CDC On Boot** | Abilitato |
   | **Dimensione Flash** | 16MB (128Mb) |
   | **Schema Partizioni** | **Custom (4MB APP/12MB LtlFS)** ⚠️ |
   | **PSRAM** | OPI PSRAM |
   | **Velocità Caricamento** | 921600 |
   | **Cancella Tutta la Flash Prima del Caricamento Sketch** | **Abilitato** ⚠️ |

   ⚠️ **Impostazioni critiche** - controlla due volte!

4. **Strumenti** → **Monitor Seriale** → Imposta velocità a **115200**

---

## Primo Caricamento (Problemi Comuni)

### Perché Sono Necessari Due Caricamenti

**Il Problema:**
- Primo caricamento: Arduino usa la **vecchia tabella delle partizioni** per scrivere il firmware
- ESP32 si avvia: Trova la **nuova tabella delle partizioni** (da `partitions.csv`)
- **Disallineamento** tra dove è stato scritto il firmware vs dove ESP32 lo cerca
- Risultato: Errori di avvio, errori di partizione, crash

**La Soluzione:**
Due caricamenti assicurano che il firmware sia scritto nella **posizione corretta** definita dalla nuova tabella delle partizioni.

---

### Caricamento #1: Flash Iniziale

1. **Collega la porta USB-C DESTRA** (vicino al LED Alimentazione) al tuo PC

2. **Seleziona porta**: Strumenti → Porta → Seleziona la porta COM che appare

3. **Verifica impostazioni**:
   - ✅ Cancella Tutta la Flash Prima del Caricamento Sketch: **Abilitato**
   - ✅ Schema Partizioni: **Custom (4MB APP/12MB LtlFS)**
   

4. **Strumenti, Carica** o (`Ctrl+U`) (Clicca sull'opzione che preferisci)
   - ✅ Il firmware viene caricato.
   - Richiede 53.6 secondi o molto meno se usi un'alimentazione esterna per l'ESP32s3 

Continua con Caricamento #2.

---

### Caricamento #2: Caricamento Sketch

1. **Scollega la porta USB-C DESTRA**

2. **Collega la porta USB-C SINISTRA** (porta OTG) al tuo PC

3. **Seleziona nuova porta**: Strumenti → Porta → Seleziona la nuova porta COM
   - **Importante**: Il numero di porta cambierà! Cerca dati nel Monitor Seriale per confermare la porta corretta, ad esempio, premi reset ESP32s3 finché non vedi risposta di dati

4. **Verifica di nuovo le impostazioni**:
   - ✅ Cancella Tutta la Flash Prima del Caricamento Sketch: **Abilitato**
   - ✅ Schema Partizioni: **Custom (4MB APP/12MB LtlFS)**

5. **Premi di nuovo Carica** (`Ctrl+U`)

6. **Attendi ~2-3 minuti** (cancellazione + caricamento, dipende se usi alimentazione esterna)

7. **Apri Monitor Seriale** - ora dovresti vedere (se hai configurato correttamente le credenziali 
in system_setup.h (il system_setup_template rinominato)):
   ```
   ✅ KissTelegram v0.9.x
   ✅ WiFi connesso
   ✅ Bot Telegram abilitato
   ✅ Sistema pronto
   ```

8. **Controlla Telegram** - riceverai messaggio di benvenuto:
   ```
   📦 Ciao! KissTelegram è pronto.
   🔌 Build: 2025-12-12 10:30:45 (0xABCD1234)
   📡 Segnale WiFi: -59 dBm (Buono)
   ✅ 0 messaggi in coda
   ```

**Successo!** Il tuo ESP32-S3 ora esegue KissTelegram con le partizioni corrette.

---

### Caricamenti Futuri

**Buone notizie:** Dopo i due caricamenti iniziali, tutti i caricamenti futuri funzionano normalmente:

- Usa **porta USB-C SINISTRA** (OTG)
- **Non serve** "Cancella Tutta la Flash" più (a meno che tu non abbia fatto modifiche ai dati NVRAM)
- Carica una volta e funziona immediatamente

---

## File di Configurazione

### system_setup.h (Richiesto Prima del Primo Caricamento!)

**Prima di compilare:**

1. Naviga alla tua cartella KissTelegram
2. Trova `system_setup_template.h`
3. **Rinominalo** in `system_setup.h`
4. **Apri** `system_setup.h` e compila:

```cpp
// Il Tuo Bot Telegram (da BotFather)
#define KISS_FALLBACK_BOT_TOKEN "1234567890:ABCdefGHIjklMNOpqrsTUVwxyz"

// Il Tuo ID Chat (da @userinfobot)
#define KISS_FALLBACK_CHAT_ID "123456789"

// Le Tue credenziali WiFi
#define KISS_FALLBACK_WIFI_SSID "NomeDelTuoWiFi"
#define KISS_FALLBACK_WIFI_PASSWORD "PasswordDelTuoWiFi"

// Sicurezza OTA (cambia PIN/PUK predefiniti!)
#define KISS_FALLBACK_OTA_PIN "0000"        // 4 cifre
#define KISS_FALLBACK_OTA_PUK "00000000"    // 8 cifre
```

5. **Salva** il file

**⚠️ Avviso di Sicurezza:** Cambia il PIN predefinito (`0000`) e PUK (`00000000`) con i tuoi segreti personali!

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
// #define LANG_ES  // Español (Spagnolo) - PREDEFINITO se tutto è commentato
```

Scegli la tua lingua (decommenta) **prima di compilare** per messaggi localizzati.

---

## Successo! E Adesso?

### Verifica Che Tutto Funzioni

1. **Invia `/status` al tuo bot** su Telegram - riceverai un report di stato dettagliato:
   ```
   📦 KissTelegram v1.x.x
   🎯 AFFIDABILITÀ DEL SISTEMA
   ✅ Sistema: AFFIDABILE
   ✅ Messaggi inviati: 2
   💾 Messaggi in sospeso: 0
   📡 Segnale WiFi: -59 dBm (Buono)
   🔋 Tempo attivo: 123 secondi
   💾 Memoria libera: 223 KB
   ```

2. **Controlla Monitor Seriale** - non dovrebbe mostrare errori

3. **Testa comandi**:
   - `/start` - Messaggio di benvenuto
   - `/help` - Comandi disponibili
   - `/status` - Stato del sistema (controllo salute)

---

### Comprendere gli Aggiornamenti OTA

Una volta che KissTelegram è in esecuzione, puoi aggiornare il firmware **via Telegram** (nessun cavo USB!):

1. Invia `/ota` al tuo bot
2. Inserisci PIN: `/otapin 0000` (o il tuo PIN personalizzato)
3. **Invia il tuo file firmware `.bin`** (trascina e rilascia su Telegram)
4. Il bot verifica automaticamente il checksum
5. Conferma: `/otaconfirm`
6. ESP32 si riavvia con il nuovo firmware
7. **Entro 60 secondi**, invia `/otaok` per confermare che funziona
8. Se non confermi, ESP32 **torna automaticamente** al firmware precedente!

📖 **Leggi di più:** Vedi `README_KissOTA_IT.md` per la documentazione OTA completa.

---

### Esplora il Codice di Esempio

L'esempio `suite_kiss.ino` dimostra:

- ✅ Gestione WiFi con monitoraggio qualità
- ✅ Coda messaggi con priorità
- ✅ Modalità gestione energia
- ✅ Gestione comandi (`/start`, `/help`, `/status`, etc.)
- ✅ Aggiornamenti OTA via Telegram
- ✅ Recupero crash e persistenza
- ✅ Connessioni SSL/TLS sicure

**Suggerimento professionale:** Usa il comando `/status` come tuo **strumento di monitoraggio salute** - è la tua finestra sugli interni di KissTelegram!

---

### Risoluzione Problemi Comuni

**Problema: "Porta non trovata" o "Accesso negato"**
- Windows ha bloccato la porta. Scollega USB, attendi 5s, ricollega.
- Prova cavo USB diverso (alcuni sono solo per ricarica, non dati)

**Problema: "Timeout in attesa del dispositivo" durante caricamento**
- Porta USB sbagliata! Ricorda: porta DESTRA per primo caricamento, porta SINISTRA per secondo
- Tieni premuto pulsante BOOT su ESP32 mentre clicchi su Carica, rilascia dopo apparizione di "Connessione..."

**Problema: Monitor Seriale mostra caratteri casuali**
- Velocità baud errata. Imposta a **115200** nel menu a tendina del Monitor Seriale

**Problema: Il bot non risponde su Telegram**
- Verifica che `system_setup.h` abbia Token Bot e ID Chat corretti
- Verifica che le credenziali WiFi siano corrette
- Apri Monitor Seriale e cerca messaggi di connessione WiFi

**Problema: Errore di compilazione "La tabella delle partizioni non entra"**
- Non hai aggiunto la partizione personalizzata a `boards.txt` correttamente
- Oppure non hai selezionato "Custom (4MB APP/12MB LtlFS)" in Strumenti → Schema Partizioni

---

### Ottieni Più Aiuto

- 📧 **Email**: victek@gmail.com
- 📖 **Documentazione**: Vedi tutti i file `README_*.md` nella tua cartella KissTelegram
- 🐛 **Segnalazioni Bug**: Issue su GitHub (link nel README.md principale)
- 💡 **Richieste Funzionalità**: Anche benvenute via email o GitHub!

---

## Riepilogo: Il Processo Completo

```
1. Ottenere Token Bot + ID Chat da Telegram ✅
2. Modificare boards.txt (aggiungere partizione personalizzata) ✅
3. Configurare Arduino IDE (Partizione personalizzata, Cancellazione abilitata) ✅
4. Modificare system_setup.h (credenziali) ✅
5. Collegare porta USB DESTRA ✅
6. Caricamento #1 (nuove partizioni)✅
7. Scollegare DESTRA, collegare porta USB SINISTRA ✅
8. Caricamento #2 (Caricare Sketch KissTelegram) ✅
9. Ricevere messaggio di benvenuto su Telegram ✅
10. Inviare /status per verificare che tutto funzioni ✅
```

**Sei pronto per costruire progetti incredibili con KissTelegram!** 🎉
