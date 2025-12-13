# KissOTA - Sistema di Aggiornamento OTA per ESP32

## Indice
- [Descrizione Generale](#descrizione-generale)
- [Caratteristiche Principali](#caratteristiche-principali)
- [Confronto con Altri Sistemi OTA](#confronto-con-altri-sistemi-ota)
- [Architettura del Sistema](#architettura-del-sistema)
- [Vantaggi rispetto all'OTA Standard di Espressif](#vantaggi-rispetto-allota-standard-di-espressif)
- [Requisiti Hardware](#requisiti-hardware)
- [Configurazione delle Partizioni](#configurazione-delle-partizioni)
- [Flusso del Processo OTA](#flusso-del-processo-ota)
- [Comandi Disponibili](#comandi-disponibili)
- [Caratteristiche di Sicurezza](#caratteristiche-di-sicurezza)
- [Gestione degli Errori e Recupero](#gestione-degli-errori-e-recupero)
- [Uso Base](#uso-base)
- [Domande Frequenti (FAQ)](#domande-frequenti-faq)

---

## Descrizione Generale

KissOTA è un sistema avanzato di aggiornamento Over-The-Air (OTA) progettato specificamente per dispositivi ESP32-S3 con PSRAM. A differenza del sistema OTA standard di Espressif, KissOTA fornisce molteplici livelli di sicurezza, validazione e recupero automatico, tutto integrato con Telegram per aggiornamenti remoti sicuri.

**Autore:** Vicente Soriano (victek@gmail.com)
**Versione attuale:** 0.9.0
**Licenza:** Inclusa in KissTelegram Suite

---

## Caratteristiche Principali

### 🔒 Sicurezza Robusta
- **Autenticazione PIN/PUK**: Controllo di accesso tramite codici PIN e PUK
- **Blocco automatico**: Dopo 3 tentativi falliti di PIN
- **Verifica CRC32**: Validazione dell'integrità del firmware prima del flash
- **Validazione post-flash**: Richiede conferma manuale dell'utente

### 🛡️ Protezione contro i Guasti
- **Backup automatico**: Copia del firmware attuale prima dell'aggiornamento
- **Rollback automatico**: Ripristino se il nuovo firmware fallisce
- **Rilevamento boot loop**: Se fallisce 3 volte in 5 minuti, rollback automatico
- **Partizione factory**: Rete di sicurezza finale se tutto il resto fallisce
- **Timeout globale**: 7 minuti per completare tutto il processo

### 💾 Uso Efficiente delle Risorse
- **Buffer PSRAM**: Usa 7-8MB di PSRAM per scaricare senza toccare la flash
- **Risparmio di spazio**: Necessita solo partizioni factory + app (senza OTA_0/OTA_1)
- **Pulizia automatica**: Elimina file temporanei e backup vecchi

### 📱 Integrazione con Telegram
- **Aggiornamento remoto**: Invia il .bin direttamente tramite Telegram
- **Feedback in tempo reale**: Messaggi di progresso e stato
- **Multi-lingua**: Supporto per 7 lingue (ES, EN, FR, IT, DE, PT, CN)

---

## Confronto con Altri Sistemi OTA

### KissOTA vs Altre Soluzioni Popolari

| Caratteristica | KissOTA | AsyncElegantOTA | ArduinoOTA | ESP-IDF OTA | ElegantOTA |
|----------------|---------|-----------------|------------|-------------|------------|
| **Trasporto** | 📱 Telegram | 🌐 HTTP Web | 🌐 HTTP Web | 🌐 HTTP | 🌐 HTTP Web |
| **Accesso remoto** | ✅ Globale senza config | ❌ Solo LAN* | ❌ Solo LAN* | ❌ Solo LAN* | ❌ Solo LAN* |
| **Richiede IP conosciuto** | ❌ No | ✅ Sì | ✅ Sì | ✅ Sì | ✅ Sì |
| **Port forwarding** | ❌ Non necessario | ⚠️ Per accesso remoto | ⚠️ Per accesso remoto | ⚠️ Per accesso remoto | ⚠️ Per accesso remoto |
| **Server web** | ❌ No | ✅ AsyncWebServer | ✅ WebServer | ✅ Configurabile | ✅ WebServer |
| **Autenticazione** | 🔒 PIN/PUK (robusto) | ⚠️ Utente/pass base | ⚠️ Password opzionale | ⚠️ Base | ⚠️ Utente/pass base |
| **Backup firmware** | ✅ In LittleFS | ❌ No | ❌ No | ❌ No | ❌ No |
| **Rollback automatico** | ✅✅✅ 3 livelli | ⚠️ Limitato (2 part.) | ❌ No | ⚠️ Limitato (2 part.) | ⚠️ Limitato (2 part.) |
| **Validazione manuale** | ✅ 60s con /otaok | ❌ No | ❌ No | ⚠️ Opzionale | ❌ No |
| **Rilevamento boot loop** | ✅ Automatico | ❌ No | ❌ No | ❌ No | ❌ No |
| **Buffer di download** | 💾 PSRAM (8MB) | 🔥 Flash | 🔥 Flash | 🔥 Flash | 🔥 Flash |
| **Progresso in tempo reale** | ✅ Messaggi Telegram | ✅ Web UI | ⚠️ Solo seriale | ⚠️ Configurabile | ✅ Web UI |
| **Interfaccia utente** | 📱 Chat Telegram | 🖥️ Browser web | 🖥️ Browser web | ⚡ Programmatica | 🖥️ Browser web |
| **Dipendenze** | KissTelegram | ESPAsyncWebServer | ESP mDNS | Nessuna (nativo) | WebServer |
| **Flash richiesta** | ~3.5 MB (app) | ~7 MB (OTA_0+1) | ~7 MB (OTA_0+1) | ~7 MB (OTA_0+1) | ~7 MB (OTA_0+1) |
| **Sicurezza su internet** | ✅ Alta (Telegram API) | ⚠️ Vulnerabile se esposto | ⚠️ Vulnerabile se esposto | ⚠️ Vulnerabile se esposto | ⚠️ Vulnerabile se esposto |
| **Facile da usare** | ✅✅ Solo inviare .bin | ✅ UI web intuitiva | ⚠️ Richiede configurazione | ⚠️ Complessità alta | ✅ UI web intuitiva |
| **Multi-lingua** | ✅ 7 lingue | ❌ Solo inglese | ❌ Solo inglese | ❌ Solo inglese | ❌ Solo inglese |
| **Recupero factory** | ✅ Sì | ❌ No | ❌ No | ⚠️ Manuale | ❌ No |

\* *Con port forwarding/VPN è possibile l'accesso remoto, ma richiede configurazione avanzata di rete*

### Vantaggi Unici di KissOTA

#### 🌍 **Accesso Veramente Globale**
Altri sistemi OTA richiedono:
- Conoscere l'IP del dispositivo
- Essere sulla stessa rete LAN, o
- Configurare port forwarding (rischioso), o
- Configurare VPN (complesso)

**KissOTA:** Hai solo bisogno di Telegram. Aggiorna da qualsiasi parte del mondo senza configurazione di rete. Essendo integrato in KissTelegram usa connessione SSL.

#### 🔒 **Sicurezza senza Compromessi**
Esporre un server web HTTP a internet è pericoloso:
- Vulnerabile ad attacchi brute force
- Possibile vettore di exploit web
- Richiede HTTPS per la sicurezza (certificati, ecc.)

**KissOTA:** Usa l'infrastruttura sicura di Telegram. Il tuo ESP32 non espone mai porte all'esterno.

#### 🛡️ **Recupero Multilivello**
Altri sistemi con rollback (ESP-IDF, AsyncElegantOTA) hanno solo 2 partizioni:
- Se entrambe le partizioni falliscono → dispositivo "bricked"
- Senza backup del firmware funzionante

**KissOTA:** 3 livelli di sicurezza:
1. **Livello 1:** Rollback da backup in LittleFS
2. **Livello 2:** Boot da partizione factory originale
3. **Livello 3:** Rilevamento boot loop automatico

#### 💾 **Risparmio di Spazio Flash**
Sistemi tradizionali (dual-bank OTA):
```
Factory:  3.5 MB  ┐
OTA_0:    3.5 MB  ├─ 7 MB minimo richiesto
OTA_1:    3.5 MB  ┘
Totale: 10.5 MB
```

**KissOTA (single-bank + backup):**
```
Factory:  3.5 MB  ┐
App:      3.5 MB  ├─ 7 MB totale
Backup:   ~1.1 MB │  (in LittleFS, comprimibile)
Totale: ~8.1 MB   ┘
```
**Guadagno:** ~2.4 MB di flash libera per la tua applicazione o dati.

#### 🚀 **PSRAM come Buffer**
Altri sistemi scaricano direttamente sulla flash:
- **Usura della flash:** La flash ha cicli limitati di scrittura (~100K)
- **Rischio:** Se il download fallisce, la flash è già stata scritta parzialmente

**KissOTA:**
- Download completo su PSRAM prima
- Verifica CRC32 su PSRAM
- Scrive sulla flash solo se tutto è OK
- **PSRAM senza usura:** Cicli di scrittura infiniti

---

## Architettura del Sistema

### Struttura delle Partizioni

```
┌─────────────────────────────────────┐
│  Factory Partition (3.5 MB)        │ ← Firmware originale di fabbrica
│  - Rete di sicurezza finale         │
│  - Solo lettura in produzione       │
└─────────────────────────────────────┘
┌─────────────────────────────────────┐
│  App Partition (3.5 MB)             │ ← Firmware in esecuzione
│  - Firmware attivo corrente          │
│  - Si aggiorna durante OTA          │
└─────────────────────────────────────┘
┌─────────────────────────────────────┐
│  LittleFS (restante ~8-9 MB)        │
│  - /ota_backup.bin (backup)         │ ← Backup del firmware precedente
│  - File di configurazione           │
│  - Dati persistenti                 │
└─────────────────────────────────────┘
┌─────────────────────────────────────┐
│  NVS (Non-Volatile Storage)         │
│  - Boot flags (validazione)         │ ← Stato di validazione OTA
│  - Credenziali PIN/PUK              │
│  - Contatore di avvii               │
└─────────────────────────────────────┘
┌─────────────────────────────────────┐
│  PSRAM (7-8 MB)                     │
│  - Buffer di download temporaneo    │ ← Firmware scaricato prima del flash
│  - Non persistente (si cancella)    │
└─────────────────────────────────────┘
```

### Flusso di Dati Durante OTA

```
Telegram API
    │
    ▼
[Download su PSRAM] ← buffer temporaneo 7-8 MB
    │
    ▼
[Verifica CRC32]
    │
    ▼
[Backup firmware attuale → LittleFS] ← /ota_backup.bin
    │
    ▼
[Flash nuovo firmware → App Partition]
    │
    ▼
[Riavvio dell'ESP32]
    │
    ▼
[Validazione 60 secondi] → /otaok o rollback automatico
    │
    ▼
[Cancella firmware precedente] -> Recupera lo spazio del firmware precedente

```
---

## Vantaggi rispetto all'OTA Standard di Espressif

| Caratteristica | Espressif OTA | KissOTA |
|----------------|---------------|---------|
| **Spazio Flash richiesto** | ~7 MB (OTA_0 + OTA_1) | ~3.5 MB (solo app) |
| **Backup del firmware** | ❌ No | ✅ Sì, in LittleFS |
| **Rollback automatico** | ⚠️ Limitato | ✅ Completo + factory |
| **Validazione manuale** | ❌ No | ✅ Sì, 60 secondi |
| **Autenticazione** | ❌ No | ✅ PIN/PUK |
| **Rilevamento boot loop** | ❌ No | ✅ Sì, automatico |
| **Integrazione Telegram** | ❌ No | ✅ Nativa |
| **Buffer di download** | Flash | PSRAM (non consuma flash) |
| **Recupero di emergenza** | ⚠️ Solo 2 partizioni | ✅ 3 livelli (app/backup/factory) |

---

## Requisiti Hardware

### Minimo Richiesto
- **MCU**: ESP32-S3 (altre varianti ESP32 possono funzionare con adattamenti)
- **PSRAM**: Minimo dipende dalla dimensione del firmware da sostituire 2-8MB PSRAM (per buffer di download)
- **Flash**: Minimo dipende dalla dimensione del firmware da sostituire 8-16-32MB
- **Connettività**: WiFi funzionale

### Configurazione Consigliata
- **ESP32-S3-WROOM-1-N16R8**: 16MB Flash + 8MB PSRAM (ideale)
- **ESP32-S3-DevKitC-1**: Con modulo N16R8 o superiore

---

## Configurazione delle Partizioni

File `partitions.csv` consigliato (N16R8):

```csv

# Name,   Type, SubType, Offset,  Size
nvs,      data, nvs,     0x9000,  0x4000,
otadata,  data, ota,     0xd000,  0x2000,
app0,     app,  factory, 0x10000, 0x180000,
app1,     app,  ota_0,   0x190000,0x180000,
spiffs,   data, spiffs,  0x310000,0xCF0000,
```

**Configurazione in Arduino IDE:**
- Tools → Partition Scheme → Custom
- Puntare al file `partitions.csv`

---

## Flusso del Processo OTA

### Diagramma degli Stati

```
┌─────────────┐
│  OTA_IDLE   │ ← Stato iniziale
└──────┬──────┘
       │ /ota
       ▼
┌─────────────┐
│ WAIT_PIN    │ ← Richiede PIN (3 tentativi)
└──────┬──────┘
       │ PIN corretto
       ▼
┌──────────────┐
│ AUTHENTICATED│ ← PIN OK, pronto per ricevere .bin
└──────┬───────┘
       │ Utente invia .bin
       ▼
┌──────────────┐
│ DOWNLOADING  │ ← Download su PSRAM (progresso in tempo reale)
└──────┬───────┘
       │
       ▼
┌────────────────┐
│ VERIFY_CHECKSUM│ ← Calcola CRC32
└──────┬─────────┘
       │
       ▼
┌──────────────┐
│ WAIT_CONFIRM │ ← Aspetta /otaconfirm dall'utente
└──────┬───────┘
       │ /otaconfirm
       ▼
┌──────────────┐
│ BACKUP_CURRENT│ ← Salva firmware attuale in LittleFS
└──────┬───────┘
       │
       ▼
┌──────────────┐
│  FLASHING    │ ← Scrive nuovo firmware da PSRAM
└──────┬───────┘
       │
       ▼
┌──────────────┐
│   REBOOT     │ ← Riavvia ESP32
└──────┬───────┘
       │
       ▼
┌──────────────┐
│  VALIDATING  │ ← 60 secondi per /otaok
└──────┬───────┘
       │ /otaok
       ▼
┌──────────────┐
│  COMPLETE    │ ← Firmware validato ✅
└──────────────┘
       │ Timeout o /otacancel
       ▼
┌──────────────┐
│  ROLLBACK    │ ← Ripristina backup automaticamente
└──────────────┘
```

### Timeout Importanti

- **Autenticazione PIN**: Illimitato (fino a 3 tentativi), se fallisce, PIN bloccato e richiede PUK per ripristinarlo
- **Ricezione del .bin**: Aspetta fino a 7 minuti, se supera cancella /ota
- **Conferma**: Aspetta fino a 7 minuti, se supera cancella /ota (aspetta /otaconfirm)
- **Processo completo**: 7 minuti massimo da /otaconfirm
- **Validazione post-flash**: 60 secondi per /otaok

---

## Comandi Disponibili

### `/ota`
Avvia il processo OTA.

**Uso:**
```
/ota
```

**Risposta:**
- Se PIN non bloccato: Richiede PIN
- Se PIN bloccato: Richiede PUK

---

### `/otapin <codice>`
Invia il codice PIN (4-8 cifre).

**Uso:**
```
/otapin 0000 (predefinito)
```

**Risposta:**
- ✅ PIN corretto: Stato AUTHENTICATED, pronto per ricevere .bin
- ❌ PIN errato: Riduce tentativi rimanenti
- 🔒 Dopo 3 errori: Blocca e richiede PUK

---

### `/otapuk <codice>`
Sblocca il sistema con il codice PUK.

**Uso:**
```
/otapuk 12345678
```

**Risposta:**
- ✅ PUK corretto: Sblocca PIN e richiede nuovo PIN
- ❌ PUK errato: Rimane bloccato

---

### `/otaconfirm`
Conferma che desideri flashare il firmware scaricato.

**Uso:**
```
/otaconfirm
```

**Precondizioni:**
- Firmware scaricato e verificato
- Checksum CRC32 OK

**Azione:**
- Crea backup del firmware attuale
- Flasha nuovo firmware
- Riavvia l'ESP32

---

### `/otaok`
Valida che il nuovo firmware funziona correttamente.

**Uso:**
```
/otaok
```

**Precondizioni:**
- Deve essere inviato entro 60 secondi dopo il riavvio
- Disponibile solo dopo un flash OTA

**Azione:**
- Marca il firmware come valido
- Elimina il backup precedente
- Il sistema torna all'operazione normale

⚠️ **IMPORTANTE**: Se non si invia `/otaok` entro 60 secondi, verrà eseguito un rollback automatico.

---

### `/otacancel`
Cancella il processo OTA o forza il rollback.

**Uso:**
```
/otacancel
```

**Comportamento:**
- Durante download/validazione: Cancella e pulisce file temporanei
- Durante validazione post-flash: Esegue rollback immediato
- Se non c'è OTA attivo: Informa che non c'è processo attivo

---

## Caratteristiche di Sicurezza

### 1. Autenticazione PIN/PUK

#### PIN e PUK (Si possono cambiare da remoto con /changepin <vecchio> <nuovo> o /changepuk <vecchio> <nuovo>)

#### PIN (Personal Identification Number)
- **Lunghezza**: 4-8 cifre
- **Tentativi**: 3 tentativi prima del blocco
- **Configurazione**: Definito in `system_setup.h` Credenziali Fallback
- **Persistenza**: Salvato in modo sicuro in NVS

#### PUK (PIN Unlock Key)
- **Lunghezza**: 8 cifre
- **Funzione**: Sbloccare dopo 3 errori di PIN
- **Sicurezza**: Solo l'amministratore dovrebbe conoscerlo

**Esempio di flusso di blocco:**
```
Tentativo 1: PIN errato → "❌ PIN errato. 2 tentativi rimanenti"
Tentativo 2: PIN errato → "❌ PIN errato. 1 tentativo rimanente"
Tentativo 3: PIN errato → "🔒 PIN bloccato. Usa /otapuk [codice]"
```

### 2. Verifica dell'Integrità

#### Checksum CRC32
- **Algoritmo**: CRC32 IEEE 802.3
- **Calcolo**: Su tutto il file .bin scaricato
- **Validazione**: Prima di permettere /otaconfirm
- **Rifiuto**: Se CRC32 non corrisponde, il download viene eliminato

**Esempio di output:**
```
🔍 Verificando checksum...
🔍 CRC32: 0xF8CAACF6 (1.07 MB verificati)
✅ Checksum OK
```

### 3. Validazione Post-Flash

#### Finestra di Validazione (60 secondi)
Dopo aver flashato il nuovo firmware:
1. ESP32 si riavvia
2. Boot flags marcano `otaInProgress = true`
3. L'utente ha 60 secondi per inviare `/otaok`
4. Se non risponde → rollback automatico

**Diagramma temporale:**
```
FLASH → RIAVVIO → [60s per /otaok] → ROLLBACK se timeout
                         ↓
                      /otaok → ✅ Firmware validato
```

### 4. Protezione contro Modifiche Non Autorizzate

- **Modalità manutenzione**: Durante OTA, altri comandi sono limitati
- **Chat ID unico**: Solo il chat_id configurato può eseguire OTA
- **Autenticazione preliminare**: PIN/PUK obbligatori prima di qualsiasi azione

---

## Gestione degli Errori e Recupero

### Livelli di Recupero

#### Livello 1: Riprova Automatico
**Scenari:**
- Errore nel download da Telegram (max 3 tentativi)
- Timeout di rete
- Download interrotto

**Azione:**
- Pulisce PSRAM
- Riprova download automaticamente
- Notifica all'utente del tentativo

---

#### Livello 2: Rollback da Backup
**Scenari:**
- L'utente non invia `/otaok` entro 60 secondi
- L'utente invia `/otacancel` durante la validazione
- Boot loop rilevato (3+ avvii in 5 minuti)

**Processo di Rollback:**
1. Rileva `bootFlags.otaInProgress == true`
2. Legge `bootFlags.backupPath` → `/ota_backup.bin`
3. Ripristina backup da LittleFS → App Partition
4. Riavvia ESP32
5. Pulisce boot flags
6. Notifica all'utente tramite Telegram

**Codice di esempio:**
```cpp
bool KissOTA::restoreFromBackup() {
  if (strlen(bootFlags.backupPath) == 0) {
    return false; // Nessun backup
  }

  // Legge /ota_backup.bin da LittleFS
  // Scrive in App Partition
  // Riavvia
}
```

---

#### Livello 3: Fallback a Factory
**Scenari:**
- Rollback da backup fallisce
- File `/ota_backup.bin` corrotto
- Errore critico nella flash

**Processo:**
1. `esp_ota_set_boot_partition(factory_partition)`
2. Riavvia ESP32
3. Avvia da firmware originale di fabbrica
4. Notifica errore critico all'utente

⚠️ **IMPORTANTE**: Questa è l'ultima risorsa. Il firmware factory deve essere stabile e mai modificato in produzione.

---

### Rilevamento Boot Loop

**Algoritmo:**
```cpp
bool KissOTA::checkBootLoop() {
  if (bootFlags.bootCount > 3) {
    unsigned long timeSinceLastBoot = millis() - bootFlags.lastBootTime;
    if (timeSinceLastBoot < 300000) {  // 5 minuti
      KISS_CRITICAL("🔥 BOOT LOOP: 3+ avvii in 5 minuti");
      return true; // Eseguire rollback
    }
  }
  return false;
}
```

**Protezione:**
- Incrementa `bootFlags.bootCount` ad ogni avvio
- Se > 3 avvii in < 5 minuti → Rollback automatico
- Validando con `/otaok`, resetta il contatore

---

### Stati di Errore

| Errore | Codice | Azione Automatica |
|-------|--------|-------------------|
| **Download fallito** | `DOWNLOAD_FAILED` | Riprovare fino a 3 volte |
| **Checksum errato** | `CHECKSUM_MISMATCH` | Eliminare download, cancellare OTA |
| **Errore backup** | `BACKUP_FAILED` | Cancellare OTA, non rischiare |
| **Errore flash** | `FLASH_FAILED` | Cancellare OTA, mantenere firmware attuale |
| **Timeout validazione** | `VALIDATION_TIMEOUT` | Rollback automatico |
| **Boot loop** | `BOOT_LOOP_DETECTED` | Rollback automatico |
| **Rollback fallito** | `ROLLBACK_FAILED` | Fallback a factory |
| **Senza backup** | `NO_BACKUP` | Mantenere firmware attuale, avvisare |

---

## Uso Base

### Aggiornamento Completo Passo dopo Passo

#### 1. Preparare il Firmware
```bash
# Compilare il progetto in Arduino IDE
# Il .bin viene generato in: build/esp32.esp32.xxx/suite_kiss.ino.bin
```

#### 2. Avviare OTA
Da Telegram:
```
/ota
```

Risposta del bot:
```
🔐 AUTENTICAZIONE OTA

Inserisci il codice PIN di 4-8 cifre:
/otapin [codice]

Tentativi rimanenti: 3
```

#### 3. Autenticarsi con PIN
```
/otapin 0000
```

Risposta:
```
✅ PIN corretto

Sistema pronto per OTA.
Invia il file .bin del nuovo firmware.
```

#### 4. Inviare il Firmware
- Allega il file `.bin` come documento (non come foto)
- Telegram lo carica e il bot lo scarica automaticamente

Risposta durante il download:
```
📥 Scaricando firmware su PSRAM...
⏳ Progresso: 45%
```

#### 5. Verifica Automatica
```
✅ Download completo: 1.07 MB su PSRAM
🔍 Verificando checksum...
✅ CRC32: 0xF8CAACF6

📋 FIRMWARE VERIFICATO

File: suite_kiss.ino.bin
Dimensione: 1.07 MB
CRC32: 0xF8CAACF6

⚠️ CONFERMA RICHIESTA
Per flashare il firmware:
/otaconfirm

Per cancellare:
/otacancel
```

#### 6. Confermare Flash
```
/otaconfirm
```

Risposta:
```
💾 Avviando backup...
✅ Backup completo: 1123456 byte

⚡ Flashando firmware...
✅ FLASH COMPLETATO

Firmware scritto correttamente.
Il dispositivo si riavvierà ora.

Dopo il riavvio avrai 60 secondi per validare con /otaok
Se non validi verrà eseguito un rollback automatico.
```

#### 7. Validazione Post-Riavvio
Dopo il riavvio (entro 60 secondi):
```
/otaok
```

Risposta:
```
✅ FIRMWARE VALIDATO

Il nuovo firmware è stato confermato.
Il sistema torna all'operazione normale.

Versione: 0.9.0
```

---

### Esempio di Rollback Manuale

Se il nuovo firmware non funziona bene:

```
/otacancel
```

Risposta:
```
⚠️ ESEGUENDO ROLLBACK

Ripristinando firmware precedente da backup...
✅ Firmware precedente ripristinato
🔄 Riavviando...

[Dopo il riavvio]
✅ ROLLBACK COMPLETATO

Sistema ripristinato al firmware precedente.
```

---

## Configurazione Avanzata

### Personalizzare i Timeout

In `KissOTA.h`:

```cpp
// Timeout di validazione (predefinito 60 secondi)
static const int BOOT_VALIDATION_TIMEOUT = 60000;

// Timeout globale del processo OTA (predefinito 7 minuti)
static const unsigned long OTA_GLOBAL_TIMEOUT = 420000;
```

### Attivare/Disattivare WDT Durante OTA

In `system_setup.h`:

```cpp
// Disattivare WDT durante operazioni critiche
#ifdef KISS_USE_RTOS
  KISS_PAUSE_WDT();  // Pausa watchdog
  // ... operazione OTA ...
  KISS_INIT_WDT();   // Riattiva watchdog
#endif
```

### Cambiare Dimensione del Buffer PSRAM

In `KissOTA.cpp`:

```cpp
bool KissOTA::initPSRAMBuffer() {
  psramBufferSize = 8 * 1024 * 1024;  // 8 MB predefinito
  // Regolare secondo PSRAM disponibile nel tuo ESP32
}
```

---
### Cambiare PIN/PUK predefinito

In `system_setup.h`:

```cpp
#define KISS_FALLBACK_OTA_PIN "0000"
#define KISS_FALLBACK_OTA_PUK "00000000"
```
---

## Risoluzione dei Problemi

### Errore: "❌ Nessuna PSRAM disponibile"
**Causa:** ESP32 senza PSRAM o PSRAM non abilitata

**Soluzione:**
1. Verificare che l'ESP32-S3 abbia PSRAM fisica
2. In Arduino IDE: Tools → PSRAM → "OPI PSRAM"
3. Ricompilare il progetto

---

### Errore: "❌ Errore creando backup"
**Causa:** LittleFS senza spazio o non montato

**Soluzione:**
1. Verificare partizione `spiffs` in `partitions.csv`
2. Formattare LittleFS se necessario
3. Aumentare dimensione della partizione spiffs

---

### Errore: "🔥 BOOT LOOP: 3+ avvii in 5 minuti"
**Causa:** Il nuovo firmware fallisce costantemente

**Soluzione:**
- Automatica: Il rollback si esegue da solo
- Manuale: Attendere il rollback automatico
- Preventiva: Testare il firmware su un altro ESP32 prima dell'OTA

---

### Il firmware non si valida dopo /otaok
**Causa:** Timeout di 60 secondi scaduto

**Soluzione:**
- Inviare `/otaok` lasciando più tempo dopo il riavvio per permettere connessione stabile
- Verificare connettività WiFi dopo il riavvio
- Aumentare `BOOT_VALIDATION_TIMEOUT` se necessario

---

## Codice di Esempio di Integrazione

### Inizializzazione in `suite_kiss.ino`

```cpp
#include "KissOTA.h"

KissTelegram* bot;
KissCredentials* credentials;
KissOTA* ota;

void setup() {
  // Inizializzare credenziali
  credentials = new KissCredentials();
  credentials->begin();

  // Inizializzare bot Telegram
  bot = new KissTelegram(BOT_TOKEN);

  // Inizializzare OTA
  ota = new KissOTA(bot, credentials);

  // Verificare se veniamo da un OTA interrotto
  if (ota->isFirstBootAfterOTA()) {
    ota->validateBootAfterOTA();
  }
}

void loop() {
  // Processare comandi Telegram
  if (bot->getUpdates()) {
    for (int i = 0; i < bot->message_count; i++) {
      String command = bot->messages[i].text;

      if (command.startsWith("/ota")) {
        ota->handleOTACommand(command.c_str(), "");
      }
    }
  }

  // Loop di OTA (gestisce timeout e stati)
  ota->loop();
}
```

---

## Informazioni Tecniche Aggiuntive

### Formato di Boot Flags (NVS)

```cpp
struct BootFlags {
  uint32_t magic;              // 0xCAFEBABE (validazione)
  uint32_t bootCount;          // Contatore di avvii
  uint32_t lastBootTime;       // Timestamp ultimo boot
  bool otaInProgress;          // true se in attesa di validazione
  bool firmwareValid;          // true se firmware validato
  char backupPath[64];         // Percorso del backup (/ota_backup.bin)
};
```

### Dimensioni Tipiche

| Elemento | Dimensione Tipica |
|----------|---------------|
| Firmware compilato | 1.0 - 1.5 MB |
| Backup in LittleFS | ~1.1 MB |
| Buffer PSRAM | 7-8 MB |
| Partizione Factory | 3.5 MB |
| Partizione App | 3.5 MB |

---

## Contributi e Supporto

**Autore:** Vicente Soriano
**Email:** victek@gmail.com
**Progetto:** KissTelegram Suite

Per segnalare bug o richiedere funzionalità, contattare l'autore.

---

## Domande Frequenti (FAQ)

### Perché non usare AsyncElegantOTA o ArduinoOTA?

**Risposta Breve:** KissOTA non richiede configurazione di rete e funziona globalmente dal primo momento.

**Risposta Completa:**

AsyncElegantOTA e ArduinoOTA sono eccellenti per lo sviluppo locale, ma hanno limitazioni in produzione:

1. **Accesso Remoto Complicato:**
   - Devi conoscere l'IP del dispositivo
   - Se è dietro router/NAT, serve port forwarding
   - Port forwarding espone il tuo ESP32 a internet (rischio di sicurezza)
   - Alternativa: VPN (complesso da configurare per utenti finali)

2. **Sicurezza Limitata:**
   - Utente/password base (vulnerabile a brute force)
   - HTTP senza crittografia (a meno che configuri HTTPS con certificati)
   - Server web esposto = superficie di attacco ampia

3. **Senza Rollback Reale:**
   - Hanno solo 2 partizioni (OTA_0 e OTA_1)
   - Se entrambe falliscono, dispositivo "bricked"
   - Nessun backup del firmware funzionante conosciuto

**KissOTA risolve questo:**
- ✅ Aggiornamento globale senza configurare nulla (Telegram API)
- ✅ Sicurezza robusta (PIN/PUK + infrastruttura Telegram)
- ✅ Rollback multilivello (backup + factory + rilevamento boot loop)
- ✅ Non espone porte all'esterno

**Quando usare ciascuno?**
- **AsyncElegantOTA:** Sviluppo locale, prototipazione rapida, LAN privata
- **KissOTA:** Produzione, dispositivi remoti globali, sicurezza critica

---

### Funziona senza PSRAM?

**Risposta:** La versione attuale di KissOTA **richiede PSRAM** per il buffer di download e verifica della validità del file.

**Ragioni tecniche:**
- Firmware tipico: 1-1.5 MB
- Buffer PSRAM: 7-8 MB disponibili
- RAM normale ESP32-S3: Solo ~70-105 KB liberi

**Alternative se non hai PSRAM:**
1. **Scaricare su LittleFS prima:**
   - Più lento (flash è più lenta di PSRAM)
   - Consuma flash inutilmente
   - Richiede spazio libero in LittleFS

2. **Usare AsyncElegantOTA:**
   - Non richiede PSRAM
   - Scarica direttamente su partizione OTA

3. **Contribuire un PR:**
   - Se implementi supporto per ESP32 senza PSRAM, benvenuta la PR

**Hardware consigliato:**
- ESP32-S3-WROOM-1-N16R8 (16MB Flash + 8MB PSRAM) - ~3-6 €
- ESP32-S3-DevKitC-1 con modulo N16R8

---

### Posso usare KissOTA senza Telegram?

**Risposta:** Tecnicamente sì, ma richiede lavoro di adattamento.

L'architettura di KissOTA ha due livelli:

1. **Core OTA (backend):**
   - Gestione degli stati
   - Backup/rollback
   - Flash da PSRAM
   - Validazione CRC32
   - Boot flags
   - **Questa parte è indipendente dal trasporto**

2. **Frontend Telegram:**
   - Autenticazione PIN/PUK
   - Download di file da Telegram API
   - Messaggi di progresso all'utente
   - **Questa parte dipende da KissTelegram**

**Per usare altro trasporto (HTTP, MQTT, Serial, ecc.):**

```cpp
// Dovresti implementare il tuo frontend personalizzato
class KissOTACustom : public KissOTACore {
public:
  // Implementare metodi astratti:
  virtual bool downloadFirmware(const char* source) override;
  virtual void sendProgress(int percentage) override;
  virtual void sendMessage(const char* text) override;
};
```

**Vale la pena lo sforzo?**
- Se hai già KissTelegram: **No, usa KissOTA così com'è**
- Se non vuoi Telegram: **Probabilmente meglio usare AsyncElegantOTA**
- Se hai un caso d'uso specifico (es: MQTT industriale): **KissTelegram ha una versione per Aziende, chiedi**

---

### Cosa succede se perdo la connessione WiFi durante il download?

**Risposta:** Il sistema gestisce disconnessioni in modo sicuro.

**Scenari:**

**1. Download su PSRAM interrotto:**
```
Stato: DOWNLOADING → Timeout di rete
Azione automatica:
  1. Rileva timeout (dopo 30 secondi senza dati)
  2. Pulisce buffer PSRAM
  3. Riprova download (massimo 3 tentativi)
  4. Se 3 errori: Cancella OTA, torna a IDLE
  5. Firmware attuale NON toccato (sicuro)
```

**2. Disconnessione durante flash:**
```
Stato: FLASHING → WiFi cade
Risultato:
  - Flash continua (non dipende da WiFi)
  - Dati già in PSRAM
  - Flash completa normalmente
  - Al riavvio, aspetterà /otaok (60s)
  - Se non c'è WiFi per inviare /otaok: Rollback automatico
```

**3. Disconnessione durante validazione:**
```
Stato: VALIDATING → Senza WiFi
Risultato:
  - Timer di 60 secondi in esecuzione
  - Se WiFi torna prima di 60s: Puoi inviare /otaok
  - Se passano 60s senza /otaok: Rollback automatico
  - Sistema torna al firmware precedente (sicuro)
```

**Timeout globale:** 7 minuti da /otaconfirm per completare tutto. Se si supera, OTA si cancella automaticamente.

---

### È sicuro flashare firmware più piccolo dell'attuale?

**Risposta:** Sì, è completamente sicuro.

**Spiegazione tecnica:**

Il processo di flash sempre:
1. **Cancella completamente la partizione destinazione** prima di scrivere
2. **Scrive il nuovo firmware** (qualunque sia la dimensione)
3. **Marca la dimensione reale** nei metadati della partizione

**Esempio:**
```
Firmware attuale:  1.5 MB
Firmware nuovo:   0.9 MB

Processo:
1. Backup di 1.5 MB → LittleFS
2. Cancellare partizione app (3.5 MB completi)
3. Scrivere 0.9 MB nuovi
4. Metadati: size = 0.9 MB
5. Spazio restante in partizione: Vuoto/cancellato
```

**Nessun problema di "spazzatura residua"** perché:
- ESP32 esegue solo fino alla fine del firmware marcato
- Il resto della partizione è cancellato
- CRC32 si calcola solo sulla dimensione reale

**Casi d'uso comuni:**
- Disattivare funzionalità per ridurre dimensione
- Firmware di emergenza minimalista (~500 KB)
- Compilazione ottimizzata con flag diversi

---

### Come resettare il sistema se tutto fallisce?

**Risposta:** Hai 3 opzioni di recupero.

#### Opzione 1: Rollback Automatico (Più Comune)
Se il nuovo firmware fallisce, semplicemente **non fare nulla**:
```
1. Firmware nuovo si avvia ma fallisce
2. ESP32 si riavvia (crash/watchdog)
3. Contatore bootCount++ in NVS
4. Dopo 3 riavvii in 5 minuti → Rollback automatico
5. Sistema ripristina firmware precedente da /ota_backup.bin
```

#### Opzione 2: Rollback Manuale
Se hai accesso a Telegram:
```
/otacancel
```
Forza rollback immediato da backup.

#### Opzione 3: Boot in Factory (Emergenza)
Se tutto il resto fallisce:

**Via Serial (richiede accesso fisico):**
```cpp
// In setup(), rileva condizione di emergenza
if (digitalRead(EMERGENCY_PIN) == LOW) {  // Pin a GND
  esp_ota_set_boot_partition(esp_partition_find_first(
    ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_FACTORY, NULL
  ));
  ESP.restart();
}
```

**Via esptool (ultima risorsa):**
```bash
# Flashare factory partition con firmware originale
esptool.py --port COM13 write_flash 0x10000 firmware_factory.bin
```

#### Opzione 4: Flash Completo (Tabula Rasa)
```bash
# Cancella tutta la flash
esptool.py --port COM13 erase_flash

# Flasha firmware nuovo + partizioni
esptool.py --port COM13 write_flash 0x0 bootloader.bin 0x8000 partitions.bin 0x10000 firmware.bin
```

**Prevenzione:**
- ✅ Mantieni sempre un firmware factory stabile
- ✅ Testa firmware nuovi su dispositivo di sviluppo prima
- ✅ Non modificare la partizione factory in produzione

---

### Quanto tempo richiede un aggiornamento OTA completo?

**Risposta:** Circa 1-3 minuti totali per firmware di ~1 MB se il WiFi è perfetto.

**Ripartizione temporale tipica:**

| Fase | Durata Tipica | Fattori |
|------|----------------|----------|
| **Autenticazione** | 10-30 secondi | Tempo dell'utente scrivendo PIN |
| **Caricamento su Telegram** | 5-15 secondi | Velocità internet dell'utente |
| **Download su PSRAM** | 10-20 secondi | Velocità WiFi dell'ESP32 + Telegram API |
| **Verifica CRC32** | 2-5 secondi | Dimensione del firmware |
| **Conferma utente** | 5-30 secondi | Tempo di reazione dell'utente |
| **Backup firmware attuale** | 10-30 secondi | Velocità di scrittura LittleFS |
| **Flash nuovo firmware** | 5-10 secondi | Velocità di scrittura flash |
| **Riavvio** | 5-10 secondi | Tempo di boot dell'ESP32 |
| **Validazione /otaok** | 5-60 secondi | Tempo di reazione dell'utente |
| **TOTALE** | **~2-4 minuti** | Varia secondo condizioni |

**Fattori che influenzano la velocità:**

**Più veloce:**
- ✅ WiFi forte (vicino al router)
- ✅ Firmware piccolo (<1 MB)
- ✅ Utente esperto (risponde velocemente)
- ✅ LittleFS non frammentato. KissTelegram deframmenta il FS ogni 3-5 minuti

**Più lento:**
- ⚠️ WiFi debole o congestionato
- ⚠️ Firmware grande (>1.5 MB)
- ⚠️ Utente esitante nel confermare
- ⚠️ LittleFS molto pieno

**Timeout di sicurezza:** 7 minuti massimo da /otaconfirm. Se si supera, OTA si cancella.

---

### Posso aggiornare più ESP32 simultaneamente?

**Risposta:** Sì, ma ogni ESP32 necessita del proprio bot Telegram (diverso BOT_TOKEN).

**Limitazione di Telegram:**
- Un bot Telegram può processare solo 1 conversazione simultanea in modo affidabile
- Telegram API ha limiti di frequenza (~30 messaggi/secondo per bot)

**Opzione 1: Un Bot per Dispositivo (Consigliato per Produzione)**
```cpp
// ESP32 #1
#define BOT_TOKEN_1 "123456:ABC-DEF..."
KissTelegram bot1(BOT_TOKEN_1);

// ESP32 #2
#define BOT_TOKEN_2 "789012:GHI-JKL..."
KissTelegram bot2(BOT_TOKEN_2);
```

**Vantaggi:**
- ✅ OTA completamente indipendenti
- ✅ Nessun conflitto di messaggi
- ✅ Ogni dispositivo ha la propria chat

**Svantaggi:**
- ⚠️ Più bot da gestire
- ⚠️ Più token da configurare

---

**Opzione 2: Un Bot, Più Dispositivi Sequenzialmente**
```cpp
// Tutti usano lo stesso BOT_TOKEN
// Ma aggiornali UNO ALLA VOLTA manualmente
```

**Processo:**
1. Aggiorna ESP32 #1 → Aspetta il completamento
2. Aggiorna ESP32 #2 → Aspetta il completamento
3. ecc.

**Vantaggi:**
- ✅ Solo un bot da gestire
- ✅ Più semplice per pochi dispositivi

**Svantaggi:**
- ⚠️ Uno alla volta (lento per molti dispositivi)
- ⚠️ Facile sbagliare dispositivo

---

**Opzione 3: Gestione Centralizzata (KissTelegram per Aziende)**
```
Sistema centrale con API propria
    ↓
Distribuisce firmware a più ESP32
    ↓
Ogni ESP32 riporta progresso al sistema centrale
    ↓
Sistema centrale notifica tramite Json API o utente via Telegram
```

**Caratteristiche gradite (Ma disponibili in Kisstelegram per aziende):**
- Backend web/cloud
- Database di dispositivi
- Coda di lavori OTA
- Dashboard di monitoraggio

**Contributi benvenuti** KissOTA ha moltissime possibilità, chiedi o fai un PR.

---

## Changelog

### v0.9.0 (Attuale)
- ✅ Sistema di versione semplificato
- ✅ Eliminata verifica di downgrade
- ✅ Pulizia di codice obsoleto
- ✅ Miglioramenti nei messaggi di log

### v0.1.0
- ✅ Implementazione di backup/rollback automatico
- ✅ Rilevamento boot loop
- ✅ Integrazione completa con Telegram

### v0.0.2
- ✅ Refactoring completo a PSRAM
- ✅ Eliminazione partizioni OTA_0/OTA_1
- ✅ Sistema di validazione di 60 secondi

### v0.0.1
- ✅ Versione iniziale con OTA base

---

**Documento aggiornato:** 12/12/2025
**Versione del documento:** 0.9.0
**Autore** Vicente Soriano, victek@gmail.com**
**Licenza MIT**
