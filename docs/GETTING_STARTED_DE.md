# Erste Schritte mit KissTelegram auf ESP32-S3

**Ein umfassender Leitfaden zum Einrichten des ESP32-S3 von Grund auf bis zur ersten Telegram-Nachricht**

> ⚠️ **KRITISCH**: Lesen Sie diesen Leitfaden vollständig durch, bevor Sie eine Firmware hochladen. Der ESP32-S3 N16R8 benötigt einen **zweistufigen Upload-Prozess** aufgrund von benutzerdefinierten Partitionen. Das Überspringen von Schritten führt zu Fehlern!

---

## Inhaltsverzeichnis

1. [Bevor Sie beginnen](#bevor-sie-beginnen)
2. [Erstellen Ihres Telegram-Bots](#erstellen-ihres-telegram-bots)
3. [Hardware-Setup](#hardware-setup)
4. [Arduino IDE-Konfiguration](#arduino-ide-konfiguration)
5. [Erster Upload (häufige Probleme)](#erster-upload-häufige-probleme)
6. [Konfigurationsdateien](#konfigurationsdateien)
7. [Erfolg! Was kommt als nächstes?](#erfolg-was-kommt-als-nächstes)

---

## Bevor Sie beginnen

### Was Sie benötigen

- **ESP32-S3 N16R8** (16MB Flash / 8MB PSRAM)
- **Zwei USB-C-Kabel** (zum Wechseln zwischen Bootloader- und OTG-Ports)
- **Arduino IDE 2.x** oder neuer
- **Windows-PC** (dieser Leitfaden konzentriert sich auf Windows, passen Sie die Pfade für Linux/Mac an)
- **Telegram-Konto** auf Ihrem Telefon

### Das Besondere daran

Ihr neuer ESP32-S3 N16R8 kommt mit einem Demo-RGB-LED-Programm. KissTelegram wird **die Partitionstabelle komplett ersetzen**, um Ihren 16MB-Flash zu maximieren:

| Partition | Espressif-Standard | KissTelegram Benutzerdefiniert |
|-----------|-------------------|---------------------|
| App-Speicher | 1,5 MB | 4,5 MB (3x größer!) |
| Dateisystem | 5 MB | 13 MB (2,6x größer!) |
| Insgesamt verwendet | 6,5 MB | 17,5 MB |

Dies ist der Grund, warum der zweistufige Upload-Prozess erforderlich ist: **Die Partitionstabelle ändert sich zwischen Uploads**.

---

## Erstellen Ihres Telegram-Bots

### Schritt 1: Mit BotFather sprechen

1. Öffnen Sie Telegram auf Ihrem Telefon
2. Suchen Sie nach `@BotFather` (offizieller Bot mit blauem Verifikationshäkchen)
3. Starten Sie ein Gespräch mit `/start`
4. Erstellen Sie Ihren Bot mit `/newbot`
5. Wählen Sie einen Namen (Beispiel: "Mein Home Assistant")
6. Wählen Sie einen Benutzernamen (muss mit `bot` enden, Beispiel: "myhome_assistant_bot")
7. **Speichern Sie Ihr Bot-Token** - sieht so aus: `1234567890:ABCdefGHIjklMNOpqrsTUVwxyz`

### Schritt 2: Holen Sie sich Ihre Chat-ID

**Methode 1: Einen Bot verwenden (einfachste)**

1. Suchen Sie nach `@userinfobot` in Telegram
2. Starten Sie ein Gespräch mit `/start`
3. Es antwortet mit Ihrer **Chat-ID** (eine Nummer wie `123456789`)
4. **Speichern Sie diese Nummer** - Sie benötigen sie später in der Konfiguration

**Methode 2: Webbrowser verwenden**

1. Senden Sie eine beliebige Nachricht an Ihren neu erstellten Bot
2. Öffnen Sie den Browser und besuchen Sie:
   ```
   https://api.telegram.org/bot<YOUR_BOT_TOKEN>/getUpdates
   ```
   (Ersetzen Sie `<YOUR_BOT_TOKEN>` durch Ihr aktuelles Token)
3. Suchen Sie nach `"chat":{"id":123456789` in der JSON-Antwort
4. Diese Nummer ist Ihre **Chat-ID**

**✅ Jetzt haben Sie:**
- Bot-Token: `1234567890:ABCdefGHIjklMNOpqrsTUVwxyz`
- Chat-ID: `123456789`

Bewahren Sie diese sicher auf! Sie werden sie bald benötigen.

---

## Hardware-Setup

### Verständnis der zwei USB-C-Ports

Ihr ESP32-S3 N16R8 hat **zwei USB-C-Ports**:

```
┌─────────────────────┐
│  ┌─┐         ESP32  │
│  │•│  ← Power LED    │
│  └─┘                 │
│  [USB-C]  ← RECHTER PORT (Bootloader/Upload)
│                      │
│                      │
│  [USB-C]  ← LINKER PORT (OTG/Normal Operation)
│                      │
└─────────────────────┘
```

**RECHTER PORT (neben Power-LED):**
- Wird für den **ersten Firmware-Upload** verwendet
- Wird für den **Bootloader-Modus** verwendet
- Verwenden Sie diesen, wenn Arduino IDE "Verbinden..." anzeigt

**LINKER PORT (OTG):**
- Wird für den **Normalbetrieb** nach dem ersten Upload verwendet
- Wird für den **zweiten Upload** (Partitionsfixierung) verwendet
- Verwenden Sie diesen für Serial Monitor im Normalbetrieb

---

## Arduino IDE-Konfiguration

### Schritt 1: Versteckte Dateien anzeigen (Windows)

1. Öffnen Sie **Datei-Explorer**
2. Klicken Sie auf **Ansicht** → **Anzeigen** → Aktivieren Sie:
   - ✅ Dateinamenerweiterungen
   - ✅ Versteckte Elemente
3. Im **Filter**-Tab: **Alle Dateitypen**

### Schritt 2: boards.txt ändern

1. Navigieren Sie zu:
   ```
   C:\Users\<YOUR_USERNAME>\AppData\Local\Arduino15\packages\esp32\hardware\esp32\3.3.4\
   ```
   (Ersetzen Sie `3.3.4` durch Ihre ESP32-Kernversion, falls unterschiedlich)

2. Finden und öffnen Sie `boards.txt` (verwenden Sie Notepad++ oder einen beliebigen Texteditor)

3. Drücken Sie `Strg+F` und suchen Sie nach:
   ```
   gen4esp32_4MBapp_4MBota_7MBspiffs
   ```

4. **Unmittelbar unter dieser Zeile** fügen Sie diese drei Zeilen ein:
   ```
   gen4-ESP32-S3R8n16.menu.PartitionScheme.gen4esp32scheme2=Custom (4MB APP/12MB LtlFS)
   gen4-ESP32-S3R8n16.menu.PartitionScheme.gen4esp32scheme2.build.custom_partitions=partitions
   gen4-ESP32-S3R8n16.menu.PartitionScheme.gen4esp32scheme2.upload.maximum_size=4718592
   ```

5. **Speichern** und schließen Sie `boards.txt`

6. Falls Arduino IDE offen war, **schließen und starten Sie sie neu**

### Schritt 3: Arduino IDE konfigurieren

1. **Öffnen** Sie Ihren KissTelegram-Skizzenordner (mit `.ino`, `.h`, `.cpp` und `partitions.csv`)

2. Wechseln Sie in Arduino IDE zu **Tools** → **Board** → **4D Systems gen4-ESP32-S3R8n16**

3. **Tools** → **Board-Daten neu laden** (Sie sehen eine Bestätigung unten auf dem Bildschirm)

4. **Konfigurieren Sie alle Tools-Menü-Optionen:**

   | Einstellung | Wert |
   |---------|-------|
   | **Board** | 4D Systems gen4-ESP32-S3R8n16 |
   | **USB CDC On Boot** | Aktiviert |
   | **Flash-Größe** | 16MB (128Mb) |
   | **Partitionsschema** | **Custom (4MB APP/12MB LtlFS)** ⚠️ |
   | **PSRAM** | OPI PSRAM |
   | **Upload-Geschwindigkeit** | 921600 |
   | **Erase All Flash Before Sketch Upload** | **Aktiviert** ⚠️ |

   ⚠️ **Kritische Einstellungen** - überprüfen Sie diese doppelt!

5. **Tools** → **Serial Monitor** → Stellen Sie die Baudrate auf **115200** ein

---

## Erster Upload (häufige Probleme)

### Warum zwei Uploads erforderlich sind

**Das Problem:**
- Erster Upload: Arduino verwendet die **alte Partitionstabelle** zum Schreiben der Firmware
- ESP32 startet: Findet die **neue Partitionstabelle** (aus `partitions.csv`)
- **Nichtübereinstimmung** zwischen dem Ort, wo die Firmware geschrieben wurde, und dem Ort, an den der ESP32 sucht
- Ergebnis: Boot-Fehler, Partitionsfehler, Abstürze

**Die Lösung:**
Zwei Uploads stellen sicher, dass die Firmware an die **richtige Stelle** geschrieben wird, die von der neuen Partitionstabelle definiert ist.

---

### Upload #1: Initiales Flash (erwarten Sie Fehler!)

1. **Verbinden Sie den RECHTEN USB-C-Port** (neben Power-LED) mit Ihrem PC

2. **Wählen Sie den Port**: Tools → Port → Wählen Sie den COM-Port, der angezeigt wird

3. **Überprüfen Sie die Einstellungen**:
   - ✅ Erase All Flash Before Sketch Upload: **Aktiviert**
   - ✅ Partitionsschema: **Custom (4MB APP/12MB LtlFS)**

4. **Drücken Sie Upload** (`Strg+U` oder ➡️ Schaltfläche)

5. **Warten Sie ~2-3 Minuten** (Löschen + Hochladen)

6. **Erwartetes Ergebnis**:
   ```
   ✅ Upload erfolgreich
   ```

7. **Öffnen Sie Serial Monitor** - Sie sehen Fehler wie:
   ```
   ❌ E (123) boot: No factory partition found
   ❌ E (456) esp_image: Image length 12345 doesn't fit in partition length 67890
   ❌ E (789) boot: Failed to verify app partition
   Guru Meditation Error: Core 0 panic'ed (LoadProhibited)
   ```

8. **Falls Sie `system_setup.h` richtig konfiguriert haben**: Sie können die **erste Telegram-Nachricht** erhalten (aber Serial zeigt Fehler)

**Keine Angst!** Diese Fehler sind **erwartet** und **normal**. Fahren Sie mit Upload #2 fort.

---

### Upload #2: Partitionsfixierung (Fehler verschwinden)

1. **Trennen Sie den RECHTEN USB-C-Port**

2. **Verbinden Sie den LINKEN USB-C-Port** (OTG-Port) mit Ihrem PC

3. **Wählen Sie den neuen Port**: Tools → Port → Wählen Sie den neuen COM-Port
   - **Wichtig**: Die Port-Nummer wird sich ändern! Schauen Sie im Serial Monitor nach Daten, um den richtigen Port zu bestätigen.

4. **Überprüfen Sie die Einstellungen nochmals**:
   - ✅ Erase All Flash Before Sketch Upload: **Aktiviert**
   - ✅ Partitionsschema: **Custom (4MB APP/12MB LtlFS)**

5. **Drücken Sie erneut Upload** (`Strg+U`)

6. **Warten Sie ~2-3 Minuten** (Löschen + Hochladen)

7. **Öffnen Sie Serial Monitor** - Sie sollten nun sehen:
   ```
   ✅ KissTelegram v1.x.x
   ✅ WiFi connected
   ✅ Telegram bot enabled
   ✅ System ready
   ```

8. **Überprüfen Sie Telegram** - Sie erhalten die Willkommensnachricht:
   ```
   📦 Hello! KissTelegram is ready.
   🔌 Build: 2025-12-12 10:30:45 (0xABCD1234)
   📡 WiFi Signal: -59 dBm (Good)
   ✅ 0 messages queued
   ```

**Erfolg!** Ihr ESP32-S3 läuft nun KissTelegram mit korrekten Partitionen.

---

### Zukünftige Uploads

**Gute Nachrichten:** Nach den zwei initialen Uploads funktionieren alle zukünftigen Uploads normal:

- Verwenden Sie **LINKEN USB-C-Port** (OTG)
- **Nicht erforderlich**, "Erase All Flash" zu verwenden (es sei denn, Sie möchten einen sauberen Zustand)
- Laden Sie einmal hoch und es funktioniert sofort

---

## Konfigurationsdateien

### system_setup.h (erforderlich vor dem ersten Upload!)

**Vor dem Kompilieren:**

1. Navigieren Sie zu Ihrem KissTelegram-Ordner
2. Finden Sie `system_setup_template.h`
3. **Benennen Sie es um** in `system_setup.h`
4. **Öffnen Sie** `system_setup.h` und füllen Sie aus:

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

5. **Speichern Sie** die Datei

**⚠️ Sicherheitswarnung:** Ändern Sie die Standard-PIN (`0000`) und PUK (`00000000`) in Ihre eigenen Geheimnisse!

---

### lang.h (Optional: Wählen Sie Ihre Sprache)

KissTelegram unterstützt 7 Sprachen für Systemmeldungen:

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

Wählen Sie Ihre Sprache **vor dem Kompilieren**, um lokalisierte Meldungen zu erhalten.

---

## Erfolg! Was kommt als nächstes?

### Überprüfen Sie, ob alles funktioniert

1. **Senden Sie `/estado` an Ihren Bot** in Telegram - Sie erhalten einen detaillierten Statusbericht:
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

2. **Überprüfen Sie Serial Monitor** - sollte keine Fehler anzeigen

3. **Testen Sie Befehle**:
   - `/start` - Willkommensnachricht
   - `/help` - Verfügbare Befehle
   - `/estado` - Systemstatus (Gesundheitsprüfung)

---

### Verständnis von OTA-Updates

Sobald KissTelegram läuft, können Sie die Firmware **über Telegram** aktualisieren (kein USB-Kabel erforderlich!):

1. Senden Sie `/ota` an Ihren Bot
2. Geben Sie die PIN ein: `/otapin 0000` (oder Ihre benutzerdefinierte PIN)
3. **Senden Sie Ihre `.bin`-Firmware-Datei** (Drag & Drop in Telegram)
4. Bot überprüft die Checksumme automatisch
5. Bestätigen Sie: `/otaconfirm`
6. ESP32 startet mit neuer Firmware neu
7. **Innerhalb von 60 Sekunden** senden Sie `/otaok`, um zu bestätigen, dass es funktioniert
8. Falls Sie nicht bestätigen, führt der ESP32 **automatisch einen Rollback** zur vorherigen Firmware durch!

📖 **Lesen Sie mehr:** Siehe `README_KissOTA_EN.md` (oder Ihre Sprache) für vollständige OTA-Dokumentation.

---

### Erkunden Sie den Beispiel-Code

Das `suite_kiss.ino`-Beispiel zeigt:

- ✅ WiFi-Verwaltung mit Qualitätsüberwachung
- ✅ Nachrichtenwarteschlange mit Prioritäten
- ✅ Stromverwaltungsmodi
- ✅ Befehlsbehandlung (`/start`, `/help`, `/estado`, usw.)
- ✅ OTA-Updates über Telegram
- ✅ Absturzwiederherstellung und Persistenz
- ✅ SSL/TLS sichere Verbindungen

**Pro-Tipp:** Verwenden Sie den Befehl `/estado` als Ihr **Gesundheitsüberwachungs-Tool** - es ist Ihr Fenster in KissTelegrams Innere!

---

### Häufige Fehlerbehebung

**Problem: "Port nicht gefunden" oder "Zugriff verweigert"**
- Windows hat den Port blockiert. Trennen Sie USB, warten Sie 5 Sekunden, stecken Sie es wieder ein.
- Versuchen Sie ein anderes USB-Kabel (einige sind nur Ladegeräte, keine Daten)

**Problem: "Timeout beim Warten auf Gerät" während des Uploads**
- Falscher USB-Port! Denken Sie daran: RECHTER Port für ersten Upload, LINKER Port für zweiten
- Halten Sie die BOOT-Taste am ESP32 gedrückt, während Sie Upload anklicken, lassen Sie los, nachdem "Verbindung..." angezeigt wird

**Problem: Serial Monitor zeigt Müllzeichen**
- Falsche Baudrate. Stellen Sie auf **115200** in der Serial Monitor-Dropdown-Liste ein

**Problem: Bot antwortet nicht in Telegram**
- Überprüfen Sie, dass `system_setup.h` das richtige Bot-Token und die Chat-ID hat
- Überprüfen Sie, dass WiFi-Anmeldedaten korrekt sind
- Öffnen Sie Serial Monitor und suchen Sie nach WiFi-Verbindungsmeldungen

**Problem: "Partition table does not fit" Kompilierungsfehler**
- Sie haben die benutzerdefinierte Partition nicht richtig zu `boards.txt` hinzugefügt
- Oder Sie haben nicht "Custom (4MB APP/12MB LtlFS)" in Tools → Partition Scheme ausgewählt

---

### Erhalten Sie weitere Hilfe

- 📧 **E-Mail**: victek@gmail.com
- 📖 **Dokumentation**: Siehe alle `README_*.md`-Dateien in Ihrem KissTelegram-Ordner
- 🐛 **Fehlerberichte**: GitHub-Probleme (Link in der Haupt-README.md)
- 💡 **Feature-Anfragen**: Auch willkommen per E-Mail oder GitHub!

---

## Zusammenfassung: Der komplette Prozess

```
1. Bot-Token + Chat-ID von Telegram abrufen ✅
2. boards.txt ändern (benutzerdefinierte Partition hinzufügen) ✅
3. Arduino IDE konfigurieren (benutzerdefinierte Partition, Löschen aktiviert) ✅
4. system_setup.h bearbeiten (Anmeldedaten) ✅
5. RECHTEN USB-Port verbinden ✅
6. Upload #1 (erwarten Sie Fehler) ✅
7. RECHTEN trennen, LINKEN USB-Port verbinden ✅
8. Upload #2 (Fehler weg!) ✅
9. Willkommensnachricht in Telegram empfangen ✅
10. /estado senden, um zu überprüfen, dass alles funktioniert ✅
```

**Sie sind bereit, erstaunliche Projekte mit KissTelegram zu erstellen!** 🎉

---

**Viel Erfolg beim Programmieren!**

*Vicente Soriano - victek@gmail.com*
