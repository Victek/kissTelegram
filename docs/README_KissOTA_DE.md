# KissOTA - OTA-Aktualisierungssystem für ESP32

## Inhaltsverzeichnis
- [Allgemeine Beschreibung](#allgemeine-beschreibung)
- [Hauptmerkmale](#hauptmerkmale)
- [Vergleich mit anderen OTA-Systemen](#vergleich-mit-anderen-ota-systemen)
- [Systemarchitektur](#systemarchitektur)
- [Vorteile gegenüber dem Standard-OTA von Espressif](#vorteile-gegenüber-dem-standard-ota-von-espressif)
- [Hardwareanforderungen](#hardwareanforderungen)
- [Partitionskonfiguration](#partitionskonfiguration)
- [OTA-Prozessablauf](#ota-prozessablauf)
- [Verfügbare Befehle](#verfügbare-befehle)
- [Sicherheitsmerkmale](#sicherheitsmerkmale)
- [Fehlerbehandlung und Wiederherstellung](#fehlerbehandlung-und-wiederherstellung)
- [Grundlegende Verwendung](#grundlegende-verwendung)
- [Häufig gestellte Fragen (FAQ)](#häufig-gestellte-fragen-faq)

---

## Allgemeine Beschreibung

KissOTA ist ein fortschrittliches Over-The-Air (OTA)-Aktualisierungssystem, das speziell für ESP32-S3-Geräte mit PSRAM entwickelt wurde. Im Gegensatz zum Standard-OTA-System von Espressif bietet KissOTA mehrere Sicherheitsebenen, Validierung und automatische Wiederherstellung, alles integriert mit Telegram für sichere Remote-Updates.

**Autor:** Vicente Soriano (victek@gmail.com)
**Aktuelle Version:** 0.9.0
**Lizenz:** Enthalten in der KissTelegram Suite

---

## Hauptmerkmale

### 🔒 Robuste Sicherheit
- **PIN/PUK-Authentifizierung**: Zugriffskontrolle über PIN- und PUK-Codes
- **Automatische Sperre**: Nach 3 fehlgeschlagenen PIN-Versuchen
- **CRC32-Verifizierung**: Integritätsprüfung der Firmware vor dem Flashen
- **Post-Flash-Validierung**: Erfordert manuelle Bestätigung des Benutzers

### 🛡️ Fehlerschutz
- **Automatisches Backup**: Kopie der aktuellen Firmware vor der Aktualisierung
- **Automatisches Rollback**: Wiederherstellung bei fehlerhafter neuer Firmware
- **Boot-Loop-Erkennung**: Bei 3 Fehlern in 5 Minuten automatisches Rollback
- **Factory-Partition**: Letztes Sicherheitsnetz, falls alles andere fehlschlägt
- **Globales Timeout**: 7 Minuten zur Fertigstellung des gesamten Prozesses

### 💾 Effiziente Ressourcennutzung
- **PSRAM-Puffer**: Verwendet 7-8MB PSRAM zum Herunterladen ohne Flash zu berühren
- **Speicherersparnis**: Benötigt nur Factory + App-Partitionen (ohne OTA_0/OTA_1)
- **Automatische Bereinigung**: Entfernt temporäre Dateien und alte Backups

### 📱 Telegram-Integration
- **Remote-Update**: Senden Sie die .bin direkt über Telegram
- **Echtzeit-Feedback**: Fortschritts- und Statusmeldungen
- **Mehrsprachig**: Unterstützung für 7 Sprachen (ES, EN, FR, IT, DE, PT, CN)

---

## Vergleich mit anderen OTA-Systemen

### KissOTA vs. andere beliebte Lösungen

| Merkmal | KissOTA | AsyncElegantOTA | ArduinoOTA | ESP-IDF OTA | ElegantOTA |
|----------------|---------|-----------------|------------|-------------|------------|
| **Transport** | 📱 Telegram | 🌐 HTTP Web | 🌐 HTTP Web | 🌐 HTTP | 🌐 HTTP Web |
| **Remote-Zugriff** | ✅ Global ohne Konfiguration | ❌ Nur LAN* | ❌ Nur LAN* | ❌ Nur LAN* | ❌ Nur LAN* |
| **Benötigt bekannte IP** | ❌ Nein | ✅ Ja | ✅ Ja | ✅ Ja | ✅ Ja |
| **Port-Weiterleitung** | ❌ Nicht erforderlich | ⚠️ Für Remote-Zugriff | ⚠️ Für Remote-Zugriff | ⚠️ Für Remote-Zugriff | ⚠️ Für Remote-Zugriff |
| **Webserver** | ❌ Nein | ✅ AsyncWebServer | ✅ WebServer | ✅ Konfigurierbar | ✅ WebServer |
| **Authentifizierung** | 🔒 PIN/PUK (robust) | ⚠️ Basis Benutzer/Passwort | ⚠️ Passwort optional | ⚠️ Basis | ⚠️ Basis Benutzer/Passwort |
| **Firmware-Backup** | ✅ In LittleFS | ❌ Nein | ❌ Nein | ❌ Nein | ❌ Nein |
| **Automatisches Rollback** | ✅✅✅ 3 Ebenen | ⚠️ Begrenzt (2 Part.) | ❌ Nein | ⚠️ Begrenzt (2 Part.) | ⚠️ Begrenzt (2 Part.) |
| **Manuelle Validierung** | ✅ 60s mit /otaok | ❌ Nein | ❌ Nein | ⚠️ Optional | ❌ Nein |
| **Boot-Loop-Erkennung** | ✅ Automatisch | ❌ Nein | ❌ Nein | ❌ Nein | ❌ Nein |
| **Download-Puffer** | 💾 PSRAM (8MB) | 🔥 Flash | 🔥 Flash | 🔥 Flash | 🔥 Flash |
| **Echtzeit-Fortschritt** | ✅ Telegram-Nachrichten | ✅ Web UI | ⚠️ Nur seriell | ⚠️ Konfigurierbar | ✅ Web UI |
| **Benutzeroberfläche** | 📱 Telegram-Chat | 🖥️ Webbrowser | 🖥️ Webbrowser | ⚡ Programmatisch | 🖥️ Webbrowser |
| **Abhängigkeiten** | KissTelegram | ESPAsyncWebServer | ESP mDNS | Keine (nativ) | WebServer |
| **Erforderlicher Flash** | ~3.5 MB (App) | ~7 MB (OTA_0+1) | ~7 MB (OTA_0+1) | ~7 MB (OTA_0+1) | ~7 MB (OTA_0+1) |
| **Internet-Sicherheit** | ✅ Hoch (Telegram API) | ⚠️ Anfällig bei Offenlegung | ⚠️ Anfällig bei Offenlegung | ⚠️ Anfällig bei Offenlegung | ⚠️ Anfällig bei Offenlegung |
| **Benutzerfreundlich** | ✅✅ Nur .bin senden | ✅ Intuitive Web-UI | ⚠️ Erfordert Konfiguration | ⚠️ Hohe Komplexität | ✅ Intuitive Web-UI |
| **Mehrsprachig** | ✅ 7 Sprachen | ❌ Nur Englisch | ❌ Nur Englisch | ❌ Nur Englisch | ❌ Nur Englisch |
| **Factory-Wiederherstellung** | ✅ Ja | ❌ Nein | ❌ Nein | ⚠️ Manuell | ❌ Nein |

\* *Mit Port-Weiterleitung/VPN ist Remote-Zugriff möglich, erfordert jedoch erweiterte Netzwerkkonfiguration*

### Einzigartige Vorteile von KissOTA

#### 🌍 **Wirklich globaler Zugriff**
Andere OTA-Systeme erfordern:
- Kenntnis der IP-Adresse des Geräts
- Im selben LAN-Netzwerk sein, oder
- Port-Weiterleitung konfigurieren (riskant), oder
- VPN konfigurieren (komplex)

**KissOTA:** Sie benötigen nur Telegram. Aktualisieren Sie von überall auf der Welt ohne Netzwerkkonfiguration. Da es in KissTelegram integriert ist, verwendet es SSL-Verbindung.

#### 🔒 **Sicherheit ohne Kompromisse**
Einen HTTP-Webserver ins Internet zu exponieren ist gefährlich:
- Anfällig für Brute-Force-Angriffe
- Möglicher Vektor für Web-Exploits
- Erfordert HTTPS für Sicherheit (Zertifikate usw.)

**KissOTA:** Verwendet die sichere Telegram-Infrastruktur. Ihr ESP32 exponiert niemals Ports nach außen.

#### 🛡️ **Mehrstufige Wiederherstellung**
Andere Systeme mit Rollback (ESP-IDF, AsyncElegantOTA) haben nur 2 Partitionen:
- Wenn beide Partitionen fehlschlagen → Gerät "gebricked"
- Kein Backup der funktionierenden Firmware

**KissOTA:** 3 Sicherheitsebenen:
1. **Ebene 1:** Rollback vom Backup in LittleFS
2. **Ebene 2:** Boot von der ursprünglichen Factory-Partition
3. **Ebene 3:** Automatische Boot-Loop-Erkennung

#### 💾 **Flash-Speicher-Ersparnis**
Traditionelle Systeme (Dual-Bank OTA):
```
Factory:  3.5 MB  ┐
OTA_0:    3.5 MB  ├─ 7 MB mindestens erforderlich
OTA_1:    3.5 MB  ┘
Gesamt: 10.5 MB
```

**KissOTA (Single-Bank + Backup):**
```
Factory:  3.5 MB  ┐
App:      3.5 MB  ├─ 7 MB gesamt
Backup:   ~1.1 MB │  (in LittleFS, komprimierbar)
Gesamt: ~8.1 MB   ┘
```
**Gewinn:** ~2.4 MB freier Flash für Ihre Anwendung oder Daten.

#### 🚀 **PSRAM als Puffer**
Andere Systeme laden direkt auf Flash herunter:
- **Flash-Verschleiß:** Flash hat begrenzte Schreibzyklen (~100K)
- **Risiko:** Bei fehlgeschlagenem Download wurde Flash bereits teilweise beschrieben

**KissOTA:**
- Vollständiger Download zuerst auf PSRAM
- CRC32-Verifizierung in PSRAM
- Schreibt nur auf Flash, wenn alles OK ist
- **PSRAM hat keinen Verschleiß:** Unendliche Schreibzyklen

---

## Systemarchitektur

### Partitionsstruktur

```
┌─────────────────────────────────────┐
│  Factory Partition (3.5 MB)        │ ← Original-Werksfirmware
│  - Letztes Sicherheitsnetz         │
│  - Nur-Lesen in Produktion         │
└─────────────────────────────────────┘
┌─────────────────────────────────────┐
│  App Partition (3.5 MB)             │ ← Laufende Firmware
│  - Aktuell aktive Firmware          │
│  - Wird während OTA aktualisiert    │
└─────────────────────────────────────┘
┌─────────────────────────────────────┐
│  LittleFS (verbleibend ~8-9 MB)     │
│  - /ota_backup.bin (Backup)         │ ← Backup der vorherigen Firmware
│  - Konfigurationsdateien            │
│  - Persistente Daten                │
└─────────────────────────────────────┘
┌─────────────────────────────────────┐
│  NVS (Non-Volatile Storage)         │
│  - Boot-Flags (Validierung)         │ ← OTA-Validierungsstatus
│  - PIN/PUK-Anmeldedaten             │
│  - Boot-Zähler                      │
└─────────────────────────────────────┘
┌─────────────────────────────────────┐
│  PSRAM (7-8 MB)                     │
│  - Temporärer Download-Puffer       │ ← Firmware vor dem Flashen heruntergeladen
│  - Nicht persistent (wird gelöscht) │
└─────────────────────────────────────┘
```

### Datenfluss während OTA

```
Telegram API
    │
    ▼
[Download auf PSRAM] ← 7-8 MB temporärer Puffer
    │
    ▼
[CRC32-Verifizierung]
    │
    ▼
[Backup aktuelle Firmware → LittleFS] ← /ota_backup.bin
    │
    ▼
[Flash neue Firmware → App Partition]
    │
    ▼
[ESP32-Neustart]
    │
    ▼
[Validierung 60 Sekunden] → /otaok oder automatisches Rollback
    │
    ▼
[Löscht vorherige Firmware] -> Gibt Speicherplatz der vorherigen Firmware frei

```
---

## Vorteile gegenüber dem Standard-OTA von Espressif

| Merkmal | Espressif OTA | KissOTA |
|----------------|---------------|---------|
| **Erforderlicher Flash-Speicher** | ~7 MB (OTA_0 + OTA_1) | ~3.5 MB (nur App) |
| **Firmware-Backup** | ❌ Nein | ✅ Ja, in LittleFS |
| **Automatisches Rollback** | ⚠️ Begrenzt | ✅ Vollständig + Factory |
| **Manuelle Validierung** | ❌ Nein | ✅ Ja, 60 Sekunden |
| **Authentifizierung** | ❌ Nein | ✅ PIN/PUK |
| **Boot-Loop-Erkennung** | ❌ Nein | ✅ Ja, automatisch |
| **Telegram-Integration** | ❌ Nein | ✅ Native |
| **Download-Puffer** | Flash | PSRAM (kein Flash-Verschleiß) |
| **Notfall-Wiederherstellung** | ⚠️ Nur 2 Partitionen | ✅ 3 Ebenen (App/Backup/Factory) |

---

## Hardwareanforderungen

### Mindestanforderungen
- **MCU**: ESP32-S3 (andere ESP32-Varianten können mit Anpassungen funktionieren)
- **PSRAM**: Minimum hängt von der Größe der zu ersetzenden Firmware ab, 2-8MB PSRAM (für Download-Puffer)
- **Flash**: Minimum hängt von der Größe der zu ersetzenden Firmware ab, 8-16-32MB
- **Konnektivität**: Funktionierendes WiFi

### Empfohlene Konfiguration
- **ESP32-S3-WROOM-1-N16R8**: 16MB Flash + 8MB PSRAM (ideal)
- **ESP32-S3-DevKitC-1**: Mit N16R8-Modul oder höher

---

## Partitionskonfiguration

Empfohlene Datei `partitions.csv` (N16R8):

```csv

# Name,   Type, SubType, Offset,  Size
nvs,      data, nvs,     0x9000,  0x4000,
otadata,  data, ota,     0xd000,  0x2000,
app0,     app,  factory, 0x10000, 0x180000,
app1,     app,  ota_0,   0x190000,0x180000,
spiffs,   data, spiffs,  0x310000,0xCF0000,
```

**Konfiguration in Arduino IDE:**
- Tools → Partition Scheme → Custom
- Auf die Datei `partitions.csv` zeigen

---

## OTA-Prozessablauf

### Zustandsdiagramm

```
┌─────────────┐
│  OTA_IDLE   │ ← Ausgangszustand
└──────┬──────┘
       │ /ota
       ▼
┌─────────────┐
│ WAIT_PIN    │ ← Fordert PIN an (3 Versuche)
└──────┬──────┘
       │ PIN korrekt
       ▼
┌──────────────┐
│ AUTHENTICATED│ ← PIN OK, bereit zum Empfang von .bin
└──────┬───────┘
       │ Benutzer sendet .bin
       ▼
┌──────────────┐
│ DOWNLOADING  │ ← Download auf PSRAM (Echtzeit-Fortschritt)
└──────┬───────┘
       │
       ▼
┌────────────────┐
│ VERIFY_CHECKSUM│ ← Berechnet CRC32
└──────┬─────────┘
       │
       ▼
┌──────────────┐
│ WAIT_CONFIRM │ ← Wartet auf /otaconfirm vom Benutzer
└──────┬───────┘
       │ /otaconfirm
       ▼
┌──────────────┐
│ BACKUP_CURRENT│ ← Speichert aktuelle Firmware in LittleFS
└──────┬───────┘
       │
       ▼
┌──────────────┐
│  FLASHING    │ ← Schreibt neue Firmware von PSRAM
└──────┬───────┘
       │
       ▼
┌──────────────┐
│   REBOOT     │ ← Startet ESP32 neu
└──────┬───────┘
       │
       ▼
┌──────────────┐
│  VALIDATING  │ ← 60 Sekunden für /otaok
└──────┬───────┘
       │ /otaok
       ▼
┌──────────────┐
│  COMPLETE    │ ← Firmware validiert ✅
└──────────────┘
       │ Timeout oder /otacancel
       ▼
┌──────────────┐
│  ROLLBACK    │ ← Stellt Backup automatisch wieder her
└──────────────┘
```

### Wichtige Timeouts

- **PIN-Authentifizierung**: Unbegrenzt (bis zu 3 Versuchen), bei Fehlschlag wird PIN gesperrt und benötigt PUK zur Wiederherstellung
- **Empfang der .bin**: Wartet bis zu 7 Minuten, bei Überschreitung Abbruch von /ota
- **Bestätigung**: Wartet bis zu 7 Minuten, bei Überschreitung Abbruch von /ota (wartet auf /otaconfirm)
- **Vollständiger Prozess**: 7 Minuten maximal ab /otaconfirm
- **Post-Flash-Validierung**: 60 Sekunden für /otaok

---

## Verfügbare Befehle

### `/ota`
Startet den OTA-Prozess.

**Verwendung:**
```
/ota
```

**Antwort:**
- Wenn PIN nicht gesperrt: Fordert PIN an
- Wenn PIN gesperrt: Fordert PUK an

---

### `/otapin <Code>`
Sendet den PIN-Code (4-8 Ziffern).

**Verwendung:**
```
/otapin 0000 (Standard)
```

**Antwort:**
- ✅ PIN korrekt: Status AUTHENTICATED, bereit zum Empfang von .bin
- ❌ PIN falsch: Verringert verbleibende Versuche
- 🔒 Nach 3 Fehlern: Sperrt und fordert PUK an

---

### `/otapuk <Code>`
Entsperrt das System mit dem PUK-Code.

**Verwendung:**
```
/otapuk 12345678
```

**Antwort:**
- ✅ PUK korrekt: Entsperrt PIN und fordert neuen PIN an
- ❌ PUK falsch: Bleibt gesperrt

---

### `/otaconfirm`
Bestätigt, dass Sie die heruntergeladene Firmware flashen möchten.

**Verwendung:**
```
/otaconfirm
```

**Vorbedingungen:**
- Firmware heruntergeladen und verifiziert
- CRC32-Prüfsumme OK

**Aktion:**
- Erstellt Backup der aktuellen Firmware
- Flasht neue Firmware
- Startet ESP32 neu

---

### `/otaok`
Bestätigt, dass die neue Firmware korrekt funktioniert.

**Verwendung:**
```
/otaok
```

**Vorbedingungen:**
- Muss innerhalb von 60 Sekunden nach dem Neustart gesendet werden
- Nur nach einem OTA-Flash verfügbar

**Aktion:**
- Markiert die Firmware als gültig
- Löscht das vorherige Backup
- System kehrt zum normalen Betrieb zurück

⚠️ **WICHTIG**: Wenn `/otaok` nicht innerhalb von 60 Sekunden gesendet wird, wird automatisches Rollback ausgeführt.

---

### `/otacancel`
Bricht den OTA-Prozess ab oder erzwingt Rollback.

**Verwendung:**
```
/otacancel
```

**Verhalten:**
- Während Download/Validierung: Bricht ab und bereinigt temporäre Dateien
- Während Post-Flash-Validierung: Führt sofortiges Rollback durch
- Wenn kein OTA aktiv: Meldet, dass kein Prozess aktiv ist

---

## Sicherheitsmerkmale

### 1. PIN/PUK-Authentifizierung

#### PIN und PUK (Können remote mit /changepin <alt> <neu> oder /changepuk <alt> <neu> geändert werden)

#### PIN (Personal Identification Number)
- **Länge**: 4-8 Ziffern
- **Versuche**: 3 Versuche vor Sperrung
- **Konfiguration**: Definiert in `system_setup.h` Fallback-Anmeldedaten
- **Persistenz**: Sicher in NVS gespeichert

#### PUK (PIN Unlock Key)
- **Länge**: 8 Ziffern
- **Funktion**: Entsperren nach 3 PIN-Fehlern
- **Sicherheit**: Nur der Administrator sollte ihn kennen

**Beispiel für Sperrablauf:**
```
Versuch 1: PIN falsch → "❌ PIN falsch. 2 Versuche verbleibend"
Versuch 2: PIN falsch → "❌ PIN falsch. 1 Versuch verbleibend"
Versuch 3: PIN falsch → "🔒 PIN gesperrt. Verwenden Sie /otapuk [Code]"
```

### 2. Integritätsprüfung

#### CRC32-Prüfsumme
- **Algorithmus**: CRC32 IEEE 802.3
- **Berechnung**: Über die gesamte heruntergeladene .bin-Datei
- **Validierung**: Vor Erlaubnis von /otaconfirm
- **Ablehnung**: Wenn CRC32 nicht übereinstimmt, wird Download gelöscht

**Beispielausgabe:**
```
🔍 Prüfsumme wird verifiziert...
🔍 CRC32: 0xF8CAACF6 (1.07 MB verifiziert)
✅ Prüfsumme OK
```

### 3. Post-Flash-Validierung

#### Validierungsfenster (60 Sekunden)
Nach dem Flashen der neuen Firmware:
1. ESP32 startet neu
2. Boot-Flags markieren `otaInProgress = true`
3. Benutzer hat 60 Sekunden Zeit, `/otaok` zu senden
4. Wenn keine Antwort → automatisches Rollback

**Zeitdiagramm:**
```
FLASH → NEUSTART → [60s für /otaok] → ROLLBACK bei Timeout
                         ↓
                      /otaok → ✅ Firmware validiert
```

### 4. Schutz vor unbefugter Änderung

- **Wartungsmodus**: Während OTA sind andere Befehle eingeschränkt
- **Eindeutige Chat-ID**: Nur die konfigurierte chat_id kann OTA ausführen
- **Vorherige Authentifizierung**: PIN/PUK obligatorisch vor jeder Aktion

---

## Fehlerbehandlung und Wiederherstellung

### Wiederherstellungsebenen

#### Ebene 1: Automatischer Wiederholungsversuch
**Szenarien:**
- Fehler beim Download von Telegram (max. 3 Wiederholungen)
- Netzwerk-Timeout
- Unterbrochener Download

**Aktion:**
- Bereinigt PSRAM
- Wiederholt Download automatisch
- Benachrichtigt Benutzer über Versuch

---

#### Ebene 2: Rollback vom Backup
**Szenarien:**
- Benutzer sendet `/otaok` nicht innerhalb von 60 Sekunden
- Benutzer sendet `/otacancel` während Validierung
- Boot-Loop erkannt (3+ Boots in 5 Minuten)

**Rollback-Prozess:**
1. Erkennt `bootFlags.otaInProgress == true`
2. Liest `bootFlags.backupPath` → `/ota_backup.bin`
3. Stellt Backup von LittleFS → App Partition wieder her
4. Startet ESP32 neu
5. Bereinigt Boot-Flags
6. Benachrichtigt Benutzer über Telegram

**Beispielcode:**
```cpp
bool KissOTA::restoreFromBackup() {
  if (strlen(bootFlags.backupPath) == 0) {
    return false; // Kein Backup vorhanden
  }

  // Liest /ota_backup.bin von LittleFS
  // Schreibt auf App Partition
  // Startet neu
}
```

---

#### Ebene 3: Fallback auf Factory
**Szenarien:**
- Rollback vom Backup schlägt fehl
- Datei `/ota_backup.bin` beschädigt
- Kritischer Fehler beim Flashen

**Prozess:**
1. `esp_ota_set_boot_partition(factory_partition)`
2. Startet ESP32 neu
3. Bootet von Original-Werksfirmware
4. Benachrichtigt Benutzer über kritischen Fehler

⚠️ **WICHTIG**: Dies ist die letzte Maßnahme. Die Factory-Firmware muss stabil sein und darf in der Produktion niemals geändert werden.

---

### Boot-Loop-Erkennung

**Algorithmus:**
```cpp
bool KissOTA::checkBootLoop() {
  if (bootFlags.bootCount > 3) {
    unsigned long timeSinceLastBoot = millis() - bootFlags.lastBootTime;
    if (timeSinceLastBoot < 300000) {  // 5 Minuten
      KISS_CRITICAL("🔥 BOOT LOOP: 3+ Boots in 5 Minuten");
      return true; // Rollback ausführen
    }
  }
  return false;
}
```

**Schutz:**
- Erhöht `bootFlags.bootCount` bei jedem Boot
- Wenn > 3 Boots in < 5 Minuten → Automatisches Rollback
- Bei Validierung mit `/otaok`, Zähler zurücksetzen

---

### Fehlerzustände

| Fehler | Code | Automatische Aktion |
|-------|--------|-------------------|
| **Download fehlgeschlagen** | `DOWNLOAD_FAILED` | Bis zu 3 Mal wiederholen |
| **Prüfsumme falsch** | `CHECKSUM_MISMATCH` | Download löschen, OTA abbrechen |
| **Backup-Fehler** | `BACKUP_FAILED` | OTA abbrechen, kein Risiko |
| **Flash-Fehler** | `FLASH_FAILED` | OTA abbrechen, aktuelle Firmware beibehalten |
| **Validierungs-Timeout** | `VALIDATION_TIMEOUT` | Automatisches Rollback |
| **Boot-Loop** | `BOOT_LOOP_DETECTED` | Automatisches Rollback |
| **Rollback fehlgeschlagen** | `ROLLBACK_FAILED` | Fallback auf Factory |
| **Kein Backup** | `NO_BACKUP` | Aktuelle Firmware beibehalten, warnen |

---

## Grundlegende Verwendung

### Vollständige Aktualisierung Schritt für Schritt

#### 1. Firmware vorbereiten
```bash
# Projekt in Arduino IDE kompilieren
# Die .bin wird generiert in: build/esp32.esp32.xxx/suite_kiss.ino.bin
```

#### 2. OTA starten
Von Telegram:
```
/ota
```

Bot-Antwort:
```
🔐 OTA-AUTHENTIFIZIERUNG

Geben Sie den 4-8-stelligen PIN-Code ein:
/otapin [Code]

Verbleibende Versuche: 3
```

#### 3. Mit PIN authentifizieren
```
/otapin 0000
```

Antwort:
```
✅ PIN korrekt

System bereit für OTA.
Senden Sie die .bin-Datei der neuen Firmware.
```

#### 4. Firmware senden
- Hängen Sie die `.bin`-Datei als Dokument an (nicht als Foto)
- Telegram lädt sie hoch und der Bot lädt sie automatisch herunter

Antwort während des Downloads:
```
📥 Firmware wird auf PSRAM heruntergeladen...
⏳ Fortschritt: 45%
```

#### 5. Automatische Verifizierung
```
✅ Download abgeschlossen: 1.07 MB auf PSRAM
🔍 Prüfsumme wird verifiziert...
✅ CRC32: 0xF8CAACF6

📋 FIRMWARE VERIFIZIERT

Datei: suite_kiss.ino.bin
Größe: 1.07 MB
CRC32: 0xF8CAACF6

⚠️ BESTÄTIGUNG ERFORDERLICH
Zum Flashen der Firmware:
/otaconfirm

Zum Abbrechen:
/otacancel
```

#### 6. Flash bestätigen
```
/otaconfirm
```

Antwort:
```
💾 Backup wird gestartet...
✅ Backup abgeschlossen: 1123456 Bytes

⚡ Firmware wird geflasht...
✅ FLASH ABGESCHLOSSEN

Firmware erfolgreich geschrieben.
Das Gerät wird jetzt neu gestartet.

Nach dem Neustart haben Sie 60 Sekunden Zeit zum Validieren mit /otaok
Wenn Sie nicht validieren, wird automatisches Rollback ausgeführt.
```

#### 7. Post-Neustart-Validierung
Nach dem Neustart (innerhalb von 60 Sekunden):
```
/otaok
```

Antwort:
```
✅ FIRMWARE VALIDIERT

Die neue Firmware wurde bestätigt.
System kehrt zum normalen Betrieb zurück.

Version: 0.9.0
```

---

### Beispiel für manuelles Rollback

Wenn die neue Firmware nicht gut funktioniert:

```
/otacancel
```

Antwort:
```
⚠️ ROLLBACK WIRD AUSGEFÜHRT

Vorherige Firmware wird vom Backup wiederhergestellt...
✅ Vorherige Firmware wiederhergestellt
🔄 Wird neu gestartet...

[Nach Neustart]
✅ ROLLBACK ABGESCHLOSSEN

System auf vorherige Firmware wiederhergestellt.
```

---

## Erweiterte Konfiguration

### Timeouts anpassen

In `KissOTA.h`:

```cpp
// Validierungs-Timeout (Standard 60 Sekunden)
static const int BOOT_VALIDATION_TIMEOUT = 60000;

// Globales OTA-Prozess-Timeout (Standard 7 Minuten)
static const unsigned long OTA_GLOBAL_TIMEOUT = 420000;
```

### WDT während OTA aktivieren/deaktivieren

In `system_setup.h`:

```cpp
// WDT während kritischer Operationen deaktivieren
#ifdef KISS_USE_RTOS
  KISS_PAUSE_WDT();  // Watchdog pausieren
  // ... OTA-Operation ...
  KISS_INIT_WDT();   // Watchdog reaktivieren
#endif
```

### PSRAM-Puffergröße ändern

In `KissOTA.cpp`:

```cpp
bool KissOTA::initPSRAMBuffer() {
  psramBufferSize = 8 * 1024 * 1024;  // 8 MB Standard
  // Anpassen an verfügbares PSRAM in Ihrem ESP32
}
```

---
### Standard-PIN/PUK ändern

In `system_setup.h`:

```cpp
#define KISS_FALLBACK_OTA_PIN "0000"
#define KISS_FALLBACK_OTA_PUK "00000000"
```
---

## Fehlerbehebung

### Fehler: "❌ Kein PSRAM verfügbar"
**Ursache:** ESP32 ohne PSRAM oder PSRAM nicht aktiviert

**Lösung:**
1. Überprüfen Sie, ob der ESP32-S3 physisches PSRAM hat
2. In Arduino IDE: Tools → PSRAM → "OPI PSRAM"
3. Projekt neu kompilieren

---

### Fehler: "❌ Fehler beim Erstellen des Backups"
**Ursache:** LittleFS ohne Speicherplatz oder nicht gemountet

**Lösung:**
1. Partition `spiffs` in `partitions.csv` überprüfen
2. LittleFS bei Bedarf formatieren
3. Größe der spiffs-Partition erhöhen

---

### Fehler: "🔥 BOOT LOOP: 3+ Boots in 5 Minuten"
**Ursache:** Neue Firmware schlägt konsequent fehl

**Lösung:**
- Automatisch: Rollback wird automatisch ausgeführt
- Manuell: Auf automatisches Rollback warten
- Präventiv: Firmware auf anderem ESP32 vor OTA testen

---

### Firmware wird nach /otaok nicht validiert
**Ursache:** 60-Sekunden-Timeout abgelaufen

**Lösung:**
- `/otaok` mit mehr Zeit nach Neustart senden, um stabile Verbindung zu ermöglichen
- WiFi-Konnektivität nach Neustart überprüfen
- `BOOT_VALIDATION_TIMEOUT` bei Bedarf erhöhen

---

## Beispiel-Integrationscode

### Initialisierung in `suite_kiss.ino`

```cpp
#include "KissOTA.h"

KissTelegram* bot;
KissCredentials* credentials;
KissOTA* ota;

void setup() {
  // Anmeldedaten initialisieren
  credentials = new KissCredentials();
  credentials->begin();

  // Telegram-Bot initialisieren
  bot = new KissTelegram(BOT_TOKEN);

  // OTA initialisieren
  ota = new KissOTA(bot, credentials);

  // Prüfen, ob wir von einem unterbrochenen OTA kommen
  if (ota->isFirstBootAfterOTA()) {
    ota->validateBootAfterOTA();
  }
}

void loop() {
  // Telegram-Befehle verarbeiten
  if (bot->getUpdates()) {
    for (int i = 0; i < bot->message_count; i++) {
      String command = bot->messages[i].text;

      if (command.startsWith("/ota")) {
        ota->handleOTACommand(command.c_str(), "");
      }
    }
  }

  // OTA-Schleife (verwaltet Timeouts und Zustände)
  ota->loop();
}
```

---

## Zusätzliche technische Informationen

### Format der Boot-Flags (NVS)

```cpp
struct BootFlags {
  uint32_t magic;              // 0xCAFEBABE (Validierung)
  uint32_t bootCount;          // Boot-Zähler
  uint32_t lastBootTime;       // Zeitstempel letzter Boot
  bool otaInProgress;          // true wenn Validierung erwartet wird
  bool firmwareValid;          // true wenn Firmware validiert
  char backupPath[64];         // Backup-Pfad (/ota_backup.bin)
};
```

### Typische Größen

| Element | Typische Größe |
|----------|---------------|
| Kompilierte Firmware | 1.0 - 1.5 MB |
| Backup in LittleFS | ~1.1 MB |
| PSRAM-Puffer | 7-8 MB |
| Factory-Partition | 3.5 MB |
| App-Partition | 3.5 MB |

---

## Beiträge und Support

**Autor:** Vicente Soriano
**E-Mail:** victek@gmail.com
**Projekt:** KissTelegram Suite

Zur Meldung von Fehlern oder Anfrage von Funktionen kontaktieren Sie bitte den Autor.

---

## Häufig gestellte Fragen (FAQ)

### Warum nicht AsyncElegantOTA oder ArduinoOTA verwenden?

**Kurze Antwort:** KissOTA erfordert keine Netzwerkkonfiguration und funktioniert von Anfang an global.

**Vollständige Antwort:**

AsyncElegantOTA und ArduinoOTA sind ausgezeichnet für die lokale Entwicklung, haben aber Einschränkungen in der Produktion:

1. **Komplizierter Remote-Zugriff:**
   - Sie müssen die IP-Adresse des Geräts kennen
   - Wenn es hinter einem Router/NAT ist, benötigen Sie Port-Weiterleitung
   - Port-Weiterleitung exponiert Ihren ESP32 ins Internet (Sicherheitsrisiko)
   - Alternative: VPN (komplex für Endbenutzer zu konfigurieren)

2. **Begrenzte Sicherheit:**
   - Basis-Benutzername/Passwort (anfällig für Brute-Force)
   - HTTP ohne Verschlüsselung (es sei denn, Sie konfigurieren HTTPS mit Zertifikaten)
   - Exponierter Webserver = große Angriffsfläche

3. **Kein echtes Rollback:**
   - Haben nur 2 Partitionen (OTA_0 und OTA_1)
   - Wenn beide fehlschlagen, Gerät "gebricked"
   - Kein Backup der bekannt funktionierenden Firmware

**KissOTA löst dies:**
- ✅ Globales Update ohne Konfiguration (Telegram API)
- ✅ Robuste Sicherheit (PIN/PUK + Telegram-Infrastruktur)
- ✅ Mehrstufiges Rollback (Backup + Factory + Boot-Loop-Erkennung)
- ✅ Exponiert keine Ports nach außen

**Wann was verwenden?**
- **AsyncElegantOTA:** Lokale Entwicklung, schnelles Prototyping, privates LAN
- **KissOTA:** Produktion, globale Remote-Geräte, kritische Sicherheit

---

### Funktioniert es ohne PSRAM?

**Antwort:** Die aktuelle Version von KissOTA **erfordert PSRAM** für den Download-Puffer und die Dateigültigkeitsprüfung.

**Technische Gründe:**
- Typische Firmware: 1-1.5 MB
- PSRAM-Puffer: 7-8 MB verfügbar
- Normales RAM ESP32-S3: Nur ~70-105 KB frei

**Alternativen, wenn Sie kein PSRAM haben:**
1. **Zuerst auf LittleFS herunterladen:**
   - Langsamer (Flash ist langsamer als PSRAM)
   - Unnötiger Flash-Verschleiß
   - Erfordert freien Speicherplatz auf LittleFS

2. **AsyncElegantOTA verwenden:**
   - Erfordert kein PSRAM
   - Lädt direkt auf OTA-Partition herunter

3. **PR beitragen:**
   - Wenn Sie Unterstützung für ESP32 ohne PSRAM implementieren, ist ein PR willkommen

**Empfohlene Hardware:**
- ESP32-S3-WROOM-1-N16R8 (16MB Flash + 8MB PSRAM) - ~3-6 Euro
- ESP32-S3-DevKitC-1 mit N16R8-Modul

---

### Kann ich KissOTA ohne Telegram verwenden?

**Antwort:** Technisch ja, erfordert aber Anpassungsarbeit.

Die Architektur von KissOTA hat zwei Schichten:

1. **Core OTA (Backend):**
   - Zustandsverwaltung
   - Backup/Rollback
   - Flash von PSRAM
   - CRC32-Validierung
   - Boot-Flags
   - **Dieser Teil ist unabhängig vom Transport**

2. **Telegram-Frontend:**
   - PIN/PUK-Authentifizierung
   - Download von Dateien über Telegram API
   - Fortschrittsnachrichten an Benutzer
   - **Dieser Teil hängt von KissTelegram ab**

**Für anderen Transport (HTTP, MQTT, Serial usw.):**

```cpp
// Sie müssten Ihr eigenes Frontend implementieren
class KissOTACustom : public KissOTACore {
public:
  // Abstrakte Methoden implementieren:
  virtual bool downloadFirmware(const char* source) override;
  virtual void sendProgress(int percentage) override;
  virtual void sendMessage(const char* text) override;
};
```

**Lohnt sich der Aufwand?**
- Wenn Sie bereits KissTelegram haben: **Nein, verwenden Sie KissOTA wie es ist**
- Wenn Sie Telegram nicht wollen: **Wahrscheinlich besser AsyncElegantOTA verwenden**
- Wenn Sie einen speziellen Anwendungsfall haben (z.B. industrielles MQTT): **KissTelegram hat eine Version für Unternehmen, fragen Sie nach**

---

### Was passiert, wenn ich während des Downloads die WiFi-Verbindung verliere?

**Antwort:** Das System behandelt Verbindungsabbrüche sicher.

**Szenarien:**

**1. Unterbrochener Download auf PSRAM:**
```
Zustand: DOWNLOADING → Netzwerk-Timeout
Automatische Aktion:
  1. Erkennt Timeout (nach 30 Sekunden ohne Daten)
  2. Bereinigt PSRAM-Puffer
  3. Wiederholt Download (maximal 3 Versuche)
  4. Bei 3 Fehlern: Bricht OTA ab, kehrt zu IDLE zurück
  5. Aktuelle Firmware NICHT berührt (sicher)
```

**2. Verbindungsabbruch während Flash:**
```
Zustand: FLASHING → WiFi fällt aus
Ergebnis:
  - Flash setzt fort (hängt nicht von WiFi ab)
  - Daten bereits in PSRAM
  - Flash wird normal abgeschlossen
  - Nach Neustart wartet auf /otaok (60s)
  - Wenn kein WiFi zum Senden von /otaok: Automatisches Rollback
```

**3. Verbindungsabbruch während Validierung:**
```
Zustand: VALIDATING → Kein WiFi
Ergebnis:
  - 60-Sekunden-Timer läuft
  - Wenn WiFi vor 60s zurückkehrt: Sie können /otaok senden
  - Wenn 60s ohne /otaok vergehen: Automatisches Rollback
  - System kehrt zur vorherigen Firmware zurück (sicher)
```

**Globales Timeout:** 7 Minuten ab /otaconfirm bis Abschluss. Bei Überschreitung wird OTA automatisch abgebrochen.

---

### Ist es sicher, Firmware zu flashen, die kleiner als die aktuelle ist?

**Antwort:** Ja, es ist vollkommen sicher.

**Technische Erklärung:**

Der Flash-Prozess:
1. **Löscht die Zielpartition vollständig** vor dem Schreiben
2. **Schreibt die neue Firmware** (welche Größe auch immer)
3. **Markiert die tatsächliche Größe** in den Partitionsmetadaten

**Beispiel:**
```
Aktuelle Firmware:  1.5 MB
Neue Firmware:      0.9 MB

Prozess:
1. Backup von 1.5 MB → LittleFS
2. App-Partition löschen (vollständige 3.5 MB)
3. 0.9 MB neue Firmware schreiben
4. Metadaten: size = 0.9 MB
5. Verbleibender Platz in Partition: Leer/gelöscht
```

**Kein Problem mit "Restmüll"** weil:
- ESP32 führt nur bis zum Ende der markierten Firmware aus
- Der Rest der Partition ist gelöscht
- CRC32 wird nur über die tatsächliche Größe berechnet

**Häufige Anwendungsfälle:**
- Funktionen deaktivieren, um Größe zu reduzieren
- Minimalistische Notfall-Firmware (~500 KB)
- Optimierte Kompilierung mit verschiedenen Flags

---

### Wie setze ich das System zurück, wenn alles fehlschlägt?

**Antwort:** Sie haben 3 Wiederherstellungsoptionen.

#### Option 1: Automatisches Rollback (Am häufigsten)
Wenn die neue Firmware fehlschlägt, einfach **nichts tun**:
```
1. Neue Firmware startet, schlägt aber fehl
2. ESP32 startet neu (Crash/Watchdog)
3. Zähler bootCount++ in NVS
4. Nach 3 Neustarts in 5 Minuten → Automatisches Rollback
5. System stellt vorherige Firmware von /ota_backup.bin wieder her
```

#### Option 2: Manuelles Rollback
Wenn Sie Zugang zu Telegram haben:
```
/otacancel
```
Erzwingt sofortiges Rollback vom Backup.

#### Option 3: Boot in Factory (Notfall)
Wenn alles andere fehlschlägt:

**Über Serial (erfordert physischen Zugang):**
```cpp
// In setup(), Notfallbedingung erkennen
if (digitalRead(EMERGENCY_PIN) == LOW) {  // Pin auf GND
  esp_ota_set_boot_partition(esp_partition_find_first(
    ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_FACTORY, NULL
  ));
  ESP.restart();
}
```

**Über esptool (letzter Ausweg):**
```bash
# Factory-Partition mit Original-Firmware flashen
esptool.py --port COM13 write_flash 0x10000 firmware_factory.bin
```

#### Option 4: Vollständiger Flash (Neustart)
```bash
# Gesamten Flash löschen
esptool.py --port COM13 erase_flash

# Frische Firmware + Partitionen flashen
esptool.py --port COM13 write_flash 0x0 bootloader.bin 0x8000 partitions.bin 0x10000 firmware.bin
```

**Prävention:**
- ✅ Behalten Sie immer eine stabile Factory-Firmware
- ✅ Testen Sie neue Firmware zuerst auf Entwicklungsgerät
- ✅ Ändern Sie die Factory-Partition nicht in Produktion

---

### Wie lange dauert eine vollständige OTA-Aktualisierung?

**Antwort:** Etwa 1-3 Minuten insgesamt für Firmware von ~1 MB bei perfektem WiFi.

**Typische zeitliche Aufschlüsselung:**

| Phase | Typische Dauer | Faktoren |
|------|----------------|----------|
| **Authentifizierung** | 10-30 Sekunden | Zeit des Benutzers zum Eingeben der PIN |
| **Upload zu Telegram** | 5-15 Sekunden | Internet-Geschwindigkeit des Benutzers |
| **Download auf PSRAM** | 10-20 Sekunden | WiFi-Geschwindigkeit des ESP32 + Telegram API |
| **CRC32-Verifizierung** | 2-5 Sekunden | Firmware-Größe |
| **Benutzerbestätigung** | 5-30 Sekunden | Reaktionszeit des Benutzers |
| **Backup aktuelle Firmware** | 10-30 Sekunden | LittleFS-Schreibgeschwindigkeit |
| **Flash neue Firmware** | 5-10 Sekunden | Flash-Schreibgeschwindigkeit |
| **Neustart** | 5-10 Sekunden | Boot-Zeit des ESP32 |
| **Validierung /otaok** | 5-60 Sekunden | Reaktionszeit des Benutzers |
| **GESAMT** | **~2-4 Minuten** | Variiert je nach Bedingungen |

**Faktoren, die die Geschwindigkeit beeinflussen:**

**Schneller:**
- ✅ Starkes WiFi (nahe am Router)
- ✅ Kleine Firmware (<1 MB)
- ✅ Erfahrener Benutzer (reagiert schnell)
- ✅ Nicht fragmentiertes LittleFS. KissTelegram defragmentiert das FS alle 3-5 Minuten

**Langsamer:**
- ⚠️ Schwaches oder überlastetes WiFi
- ⚠️ Große Firmware (>1.5 MB)
- ⚠️ Benutzer zögert bei Bestätigung
- ⚠️ LittleFS sehr voll

**Sicherheits-Timeout:** 7 Minuten maximal ab /otaconfirm. Bei Überschreitung wird OTA abgebrochen.

---

### Kann ich mehrere ESP32 gleichzeitig aktualisieren?

**Antwort:** Ja, aber jeder ESP32 benötigt seinen eigenen Telegram-Bot (verschiedene BOT_TOKEN).

**Telegram-Einschränkung:**
- Ein Telegram-Bot kann nur 1 Konversation gleichzeitig zuverlässig verarbeiten
- Telegram API hat Rate Limits (~30 Nachrichten/Sekunde pro Bot)

**Option 1: Ein Bot pro Gerät (Für Produktion empfohlen)**
```cpp
// ESP32 #1
#define BOT_TOKEN_1 "123456:ABC-DEF..."
KissTelegram bot1(BOT_TOKEN_1);

// ESP32 #2
#define BOT_TOKEN_2 "789012:GHI-JKL..."
KissTelegram bot2(BOT_TOKEN_2);
```

**Vorteile:**
- ✅ Vollständig unabhängige OTAs
- ✅ Keine Nachrichtenkonflikte
- ✅ Jedes Gerät hat seinen eigenen Chat

**Nachteile:**
- ⚠️ Mehr Bots zu verwalten
- ⚠️ Mehr Token zu konfigurieren

---

**Option 2: Ein Bot, mehrere Geräte sequenziell**
```cpp
// Alle verwenden dasselbe BOT_TOKEN
// Aber aktualisieren Sie sie EINZELN manuell
```

**Prozess:**
1. ESP32 #1 aktualisieren → Warten auf Abschluss
2. ESP32 #2 aktualisieren → Warten auf Abschluss
3. usw.

**Vorteile:**
- ✅ Nur ein Bot zu verwalten
- ✅ Einfacher für wenige Geräte

**Nachteile:**
- ⚠️ Einer nach dem anderen (langsam für viele Geräte)
- ⚠️ Leicht, Gerät zu verwechseln

---

**Option 3: Zentralisierte Verwaltung (KissTelegram für Unternehmen)**
```
Zentrales System mit eigener API
    ↓
Verteilt Firmware an mehrere ESP32
    ↓
Jeder ESP32 meldet Fortschritt an zentrales System
    ↓
Zentrales System benachrichtigt über Json API oder Benutzer über Telegram
```

**Gewünschte Funktionen (Aber verfügbar in Kisstelegram für Unternehmen):**
- Web/Cloud-Backend
- Gerätedatenbank
- OTA-Job-Warteschlange
- Überwachungs-Dashboard

**Beiträge willkommen** KissOTA hat sehr viele Möglichkeiten, fragen Sie nach oder machen Sie einen PR.

---

## Changelog

### v0.9.0 (Aktuell)
- ✅ Vereinfachtes Versionssystem
- ✅ Downgrade-Verifizierung entfernt
- ✅ Bereinigung von veraltetem Code
- ✅ Verbesserungen bei Log-Meldungen

### v0.1.0
- ✅ Implementierung von automatischem Backup/Rollback
- ✅ Boot-Loop-Erkennung
- ✅ Vollständige Telegram-Integration

### v0.0.2
- ✅ Vollständige Umstellung auf PSRAM
- ✅ Entfernung der OTA_0/OTA_1-Partitionen
- ✅ 60-Sekunden-Validierungssystem

### v0.0.1
- ✅ Erste Version mit Basis-OTA

---

**Dokument aktualisiert:** 12.12.2025
**Dokumentversion:** 0.9.0
**Autor** Vicente Soriano, victek@gmail.com**
**MIT-Lizenz**
