# Erste Schritte mit KissTelegram auf ESP32-S3

**Vollständige Anleitung zum Einrichten Ihres ESP32-S3 von Grund auf bis zur ersten Telegram-Nachricht**

> ⚠️ **KRITISCH**: Lesen Sie diese Anleitung vollständig, bevor Sie Firmware hochladen. Der ESP32-S3 N16R8 erfordert einen **zweistufigen Upload-Prozess** aufgrund benutzerdefinierter Partitionen. Das Überspringen von Schritten führt zu Fehlern!

---

## Inhaltsverzeichnis

1. [Bevor Sie Beginnen](#bevor-sie-beginnen)
2. [Erstellen Sie Ihren Telegram-Bot](#erstellen-sie-ihren-telegram-bot)
3. [Hardware-Konfiguration](#hardware-konfiguration)
4. [Arduino IDE Konfiguration](#arduino-ide-konfiguration)
5. [Erster Upload (Partitionen mit Arduino IDE erstellen)](#erster-upload)
6. [Konfigurationsdateien](#konfigurationsdateien)
7. [Erfolg! Was Kommt Als Nächstes?](#erfolg-was-kommt-als-nächstes)

---

## Bevor Sie Beginnen

### Was Sie Benötigen

- **ESP32-S3 N16R8** (16MB Flash / 8MB PSRAM)
- **Zwei USB-C Kabel** (um zwischen Bootloader- und OTG-Ports zu wechseln)
- **Arduino IDE 2.x** oder höher
- **Windows-PC** (diese Anleitung ist auf Windows ausgerichtet, passen Sie Pfade für Linux/Mac an)
- **Telegram-Konto** auf Ihrem Telefon

### Was Dies Anders Macht

Ihr neuer ESP32-S3 N16R8 wird mit einer integrierten RGB-LED-Demo-App geliefert. KissTelegram **ersetzt die Partitionstabelle vollständig**, um Ihren 16MB Flash zu maximieren:

| Partition | Standard Espressif | KissTelegram Angepasst |
|-----------|-------------------|------------------------|
| App-Speicher | 1.5 MB | 4.5 MB (3x größer!) |
| Dateisystem | 5 MB | 13 MB (2.6x größer!) |
| Gesamt Verwendet | 6.5 MB | 17.5 MB |

Deshalb ist der zweistufige Upload-Prozess erforderlich: **Die Partitionstabelle ändert sich zwischen den Uploads**.

---

## Erstellen Sie Ihren Telegram-Bot

### Schritt 1: Sprechen Sie mit BotFather

1. Öffnen Sie Telegram auf Ihrem Telefon
2. Suchen Sie nach `@BotFather` (offizieller Bot, hat blaues Häkchen)
3. Starten Sie die Unterhaltung mit `/start`
4. Erstellen Sie Ihren Bot mit `/newbot`
5. Wählen Sie einen Namen (Beispiel: "Mein Heim-Assistent")
6. Wählen Sie einen Benutzernamen (muss auf `bot` enden, Beispiel: "meinheim_assistent_bot")
7. **Speichern Sie Ihr Bot-Token** - sieht so aus: `1234567890:ABCdefGHIjklMNOpqrsTUVwxyz`

### Schritt 2: Holen Sie Sich Ihre Chat-ID

**Methode 1: Verwendung eines Bots (Einfacher)**

1. Suchen Sie nach `@ChatIDHelperBot` in Telegram
2. Starten Sie die Unterhaltung mit `/start`
3. Er antwortet mit Ihrer **Chat-ID** (eine Nummer wie `123456789`)
4. **Speichern Sie diese Nummer** - Sie benötigen sie in der Konfiguration

**Methode 2: Verwendung eines Webbrowsers**

1. Senden Sie eine beliebige Nachricht an Ihren neu erstellten Bot
2. Öffnen Sie den Browser und besuchen Sie:
   ```
   https://api.telegram.org/bot<IHR_BOT_TOKEN>/getUpdates
   ```
   (Ersetzen Sie `<IHR_BOT_TOKEN>` mit Ihrem tatsächlichen Token)
3. Suchen Sie nach `"chat":{"id":123456789` in der JSON-Antwort
4. Diese Nummer ist Ihre **Chat-ID**

**✅ Sie haben jetzt:**
- Bot-Token: `1234567890:ABCdefGHIjklMNOpqrsTUVwxyz`
- Chat-ID: `123456789`

Bewahren Sie sie sicher auf! Sie werden sie bald brauchen.

---

## Hardware-Konfiguration

### Die Zwei USB-C Ports Verstehen

Ihr ESP32-S3 N16R8 hat **zwei USB-C Ports**:

```
┌─────────────────────┐
│  ┌─┐         ESP32  │
│  │•│  ← Power-LED    │
│  └─┘                 │
│  [USB-C]  ← RECHTER PORT (Bootloader/Upload)
│                      │
│                      │
│  [USB-C]  ← LINKER PORT (OTG/Normalbetrieb)
│                      │
└─────────────────────┘
```

**RECHTER PORT (nahe der Power-LED):**
- Wird für **initialen Firmware-Upload** verwendet
- Wird für **Bootloader-Modus** verwendet
- Verwenden Sie diesen, wenn Arduino IDE "Verbindung..." anzeigt

**LINKER PORT (OTG):**
- Wird für **Normalbetrieb** nach dem ersten Upload verwendet
- Wird für **zweiten Upload** (Partitionskorrektur) verwendet
- Verwenden Sie diesen für Serial Monitor im Normalbetrieb

---

## Arduino IDE Konfiguration

### Schritt 1: Versteckte Dateien Anzeigen (Windows)

1. Öffnen Sie den **Datei-Explorer**
2. Klicken Sie auf **Ansicht** → **Anzeigen** → Aktivieren:
   - ✅ Dateinamenerweiterungen
   - ✅ Ausgeblendete Elemente
3. In der **Filter**-Registerkarte: **Alle Dateitypen**

### Schritt 2: boards.txt Modifizieren

1. Navigieren Sie zu:
   ```
   C:\Users\<IHR_BENUTZERNAME>\AppData\Local\Arduino15\packages\esp32\hardware\esp32\3.3.4\
   ```
   (Ersetzen Sie `3.3.4` mit Ihrer ESP32-Core-Version, falls abweichend)

2. Finden und öffnen Sie `boards.txt` (verwenden Sie Notepad++ oder einen beliebigen Texteditor)

3. Drücken Sie `Strg+F` und suchen Sie nach:
   ```
   gen4esp32_4MBapp_4MBota_7MBspiffs
   ```

4. **Unmittelbar unterhalb dieser Zeile** fügen Sie diese drei Zeilen ein:
   ```
   gen4-ESP32-S3R8n16.menu.PartitionScheme.gen4esp32scheme2=Custom (4MB APP/12MB LtlFS)
   gen4-ESP32-S3R8n16.menu.PartitionScheme.gen4esp32scheme2.build.custom_partitions=partitions
   gen4-ESP32-S3R8n16.menu.PartitionScheme.gen4esp32scheme2.upload.maximum_size=4718592
   ```

5. **Speichern** und schließen Sie `boards.txt`

6. Falls Arduino IDE geöffnet war, **schließen und neu starten**

### Schritt 3: Arduino IDE Konfigurieren

1. **Öffnen** Sie Ihren KissTelegram-Sketch-Ordner (mit `.ino`, `.h`, `.cpp`, und `partitions.csv`)

2. In Arduino IDE gehen Sie zu **Werkzeuge** → **Board** → **4D Systems gen4-ESP32-S3R8n16**

3. **Konfigurieren Sie alle Werkzeuge-Menüoptionen:**

   | Einstellung | Wert |
   |---------|-------|
   | **Board** | 4D Systems gen4-ESP32-S3R8n16 |
   | **USB CDC On Boot** | Aktiviert |
   | **Flash-Größe** | 16MB (128Mb) |
   | **Partitionsschema** | **Custom (4MB APP/12MB LtlFS)** ⚠️ |
   | **PSRAM** | OPI PSRAM |
   | **Upload-Geschwindigkeit** | 921600 |
   | **Gesamten Flash Vor Sketch-Upload Löschen** | **Aktiviert** ⚠️ |

   ⚠️ **Kritische Einstellungen** - doppelt prüfen!

4. **Werkzeuge** → **Serieller Monitor** → Geschwindigkeit auf **115200** setzen

---

## Erster Upload (Häufige Probleme)

### Warum Zwei Uploads Benötigt Werden

**Das Problem:**
- Erster Upload: Arduino verwendet die **alte Partitionstabelle** zum Schreiben der Firmware
- ESP32 startet: Findet die **neue Partitionstabelle** (aus `partitions.csv`)
- **Inkongruenz** zwischen wo Firmware geschrieben wurde vs wo ESP32 danach sucht
- Ergebnis: Boot-Fehler, Partitionsfehler, Abstürze

**Die Lösung:**
Zwei Uploads stellen sicher, dass Firmware an der **korrekten Stelle** geschrieben wird, die durch die neue Partitionstabelle definiert ist.

---

### Upload #1: Initialer Flash

1. **Verbinden Sie RECHTEN USB-C Port** (nahe der Power-LED) mit Ihrem PC

2. **Port auswählen**: Werkzeuge → Port → Wählen Sie den angezeigten COM-Port

3. **Einstellungen überprüfen**:
   - ✅ Gesamten Flash Vor Sketch-Upload Löschen: **Aktiviert**
   - ✅ Partitionsschema: **Custom (4MB APP/12MB LtlFS)**
   

4. **Werkzeuge, Laden** oder (`Strg+U`) (Klicken Sie auf die gewünschte Option)
   - ✅ Die Firmware wird hochgeladen.
   - Dauert 53.6 Sekunden oder viel weniger, wenn Sie eine externe Stromversorgung für den ESP32s3 verwenden 

Fahren Sie mit Upload #2 fort.

---

### Upload #2: Sketch-Upload

1. **Trennen Sie RECHTEN USB-C Port**

2. **Verbinden Sie LINKEN USB-C Port** (OTG-Port) mit Ihrem PC

3. **Neuen Port auswählen**: Werkzeuge → Port → Wählen Sie den neuen COM-Port
   - **Wichtig**: Portnummer wird sich ändern! Suchen Sie nach Daten im Seriellen Monitor, um den korrekten Port zu bestätigen, drücken Sie z.B. ESP32s3-Reset, bis Sie eine Datenantwort sehen

4. **Einstellungen erneut überprüfen**:
   - ✅ Gesamten Flash Vor Sketch-Upload Löschen: **Aktiviert**
   - ✅ Partitionsschema: **Custom (4MB APP/12MB LtlFS)**

5. **Drücken Sie erneut Upload** (`Strg+U`)

6. **Warten Sie ~2-3 Minuten** (Löschen + Hochladen, hängt davon ab, ob Sie externe Stromversorgung verwenden)

7. **Öffnen Sie Seriellen Monitor** - Sie sollten jetzt sehen (wenn Sie die Anmeldedaten 
in system_setup.h korrekt gesetzt haben (die umbenannte system_setup_template)):
   ```
   ✅ KissTelegram v0.9.x
   ✅ WiFi verbunden
   ✅ Telegram-Bot aktiviert
   ✅ System bereit
   ```

8. **Telegram überprüfen** - Sie erhalten eine Willkommensnachricht:
   ```
   📦 Hallo! KissTelegram ist bereit.
   🔌 Build: 2025-12-12 10:30:45 (0xABCD1234)
   📡 WiFi-Signal: -59 dBm (Gut)
   ✅ 0 Nachrichten in Warteschlange
   ```

**Erfolg!** Ihr ESP32-S3 führt jetzt KissTelegram mit korrekten Partitionen aus.

---

### Zukünftige Uploads

**Gute Nachrichten:** Nach den beiden initialen Uploads funktionieren alle zukünftigen Uploads normal:

- Verwenden Sie **LINKEN USB-C Port** (OTG)
- **Benötigen nicht** "Gesamten Flash Löschen" mehr (es sei denn, Sie haben Änderungen an NVRAM-Daten vorgenommen)
- Upload einmal und es funktioniert sofort

---

## Konfigurationsdateien

### system_setup.h (Vor dem Ersten Upload Erforderlich!)

**Vor dem Kompilieren:**

1. Navigieren Sie zu Ihrem KissTelegram-Ordner
2. Finden Sie `system_setup_template.h`
3. **Benennen Sie es um** zu `system_setup.h`
4. **Öffnen** Sie `system_setup.h` und füllen Sie aus:

```cpp
// Ihr Telegram-Bot (von BotFather)
#define KISS_FALLBACK_BOT_TOKEN "1234567890:ABCdefGHIjklMNOpqrsTUVwxyz"

// Ihre Chat-ID (von @userinfobot)
#define KISS_FALLBACK_CHAT_ID "123456789"

// Ihre WiFi-Anmeldedaten
#define KISS_FALLBACK_WIFI_SSID "IhrWiFiName"
#define KISS_FALLBACK_WIFI_PASSWORD "IhrWiFiPasswort"

// OTA-Sicherheit (Standard-PIN/PUK ändern!)
#define KISS_FALLBACK_OTA_PIN "0000"        // 4 Ziffern
#define KISS_FALLBACK_OTA_PUK "00000000"    // 8 Ziffern
```

5. **Speichern** Sie die Datei

**⚠️ Sicherheitswarnung:** Ändern Sie Standard-PIN (`0000`) und PUK (`00000000`) zu Ihren eigenen Geheimnissen!

---

### lang.h (Optional: Wählen Sie Ihre Sprache)

KissTelegram unterstützt 7 Sprachen für Systemnachrichten:

```cpp
// In lang.h, kommentieren Sie EINE Sprache ein:

// #define LANG_CN  // 中文 (Chinesisch)
// #define LANG_DE  // Deutsch (Deutsch)
// #define LANG_EN  // English (Englisch)
// #define LANG_FR  // Français (Französisch)
// #define LANG_IT  // Italiano (Italienisch)
// #define LANG_PT  // Português (Portugiesisch)
// #define LANG_ES  // Español (Spanisch) - STANDARD wenn alle auskommentiert
```

Wählen Sie Ihre Sprache (einkommentieren) **vor dem Kompilieren** für lokalisierte Nachrichten.

---

## Erfolg! Was Kommt Als Nächstes?

### Überprüfen Sie, Dass Alles Funktioniert

1. **Senden Sie `/status` an Ihren Bot** in Telegram - Sie erhalten einen detaillierten Statusbericht:
   ```
   📦 KissTelegram v1.x.x
   🎯 SYSTEMZUVERLÄSSIGKEIT
   ✅ System: ZUVERLÄSSIG
   ✅ Nachrichten gesendet: 2
   💾 Ausstehende Nachrichten: 0
   📡 WiFi-Signal: -59 dBm (Gut)
   🔋 Betriebszeit: 123 Sekunden
   💾 Freier Speicher: 223 KB
   ```

2. **Seriellen Monitor überprüfen** - sollte keine Fehler anzeigen

3. **Befehle testen**:
   - `/start` - Willkommensnachricht
   - `/help` - Verfügbare Befehle
   - `/status` - Systemstatus (Gesundheitscheck)

---

### OTA-Updates Verstehen

Sobald KissTelegram läuft, können Sie Firmware **über Telegram** aktualisieren (kein USB-Kabel!):

1. Senden Sie `/ota` an Ihren Bot
2. PIN eingeben: `/otapin 0000` (oder Ihre benutzerdefinierte PIN)
3. **Senden Sie Ihre Firmware-`.bin`-Datei** (Drag & Drop in Telegram)
4. Bot überprüft Prüfsumme automatisch
5. Bestätigen: `/otaconfirm`
6. ESP32 startet mit neuer Firmware neu
7. **Innerhalb von 60 Sekunden** senden Sie `/otaok` zur Bestätigung, dass es funktioniert
8. Wenn Sie nicht bestätigen, **rollt ESP32 automatisch zurück** zur vorherigen Firmware!

📖 **Mehr lesen:** Siehe `README_KissOTA_DE.md` für vollständige OTA-Dokumentation.

---

### Beispielcode Erkunden

Das Beispiel `suite_kiss.ino` demonstriert:

- ✅ WiFi-Management mit Qualitätsüberwachung
- ✅ Nachrichtenwarteschlange mit Prioritäten
- ✅ Energieverwaltungsmodi
- ✅ Befehlsverarbeitung (`/start`, `/help`, `/status`, etc.)
- ✅ OTA-Updates über Telegram
- ✅ Absturzwiederherstellung und Persistenz
- ✅ Sichere SSL/TLS-Verbindungen

**Profi-Tipp:** Verwenden Sie den `/status`-Befehl als Ihr **Gesundheitsüberwachungs-Tool** - es ist Ihr Fenster zu KissTelegrams Interna!

---

### Häufige Fehlerbehebung

**Problem: "Port nicht gefunden" oder "Zugriff verweigert"**
- Windows hat den Port gesperrt. USB trennen, 5s warten, wieder verbinden.
- Anderes USB-Kabel probieren (einige sind nur zum Laden, nicht für Daten)

**Problem: "Zeitüberschreitung beim Warten auf Gerät" während des Uploads**
- Falscher USB-Port! Merken Sie sich: RECHTER Port für ersten Upload, LINKER Port für zweiten
- BOOT-Taste am ESP32 gedrückt halten während Sie Upload klicken, nach "Verbindung..." loslassen

**Problem: Serieller Monitor zeigt Zeichensalat**
- Falsche Baudrate. Auf **115200** in Serial Monitor Dropdown setzen

**Problem: Bot antwortet nicht in Telegram**
- Überprüfen Sie, dass `system_setup.h` korrektes Bot-Token und Chat-ID hat
- Überprüfen Sie, dass WiFi-Anmeldedaten korrekt sind
- Seriellen Monitor öffnen und nach WiFi-Verbindungsnachrichten suchen

**Problem: Kompilierfehler "Partitionstabelle passt nicht"**
- Benutzerdefinierte Partition nicht korrekt zu `boards.txt` hinzugefügt
- Oder "Custom (4MB APP/12MB LtlFS)" in Werkzeuge → Partitionsschema nicht ausgewählt

---

### Weitere Hilfe Erhalten

- 📧 **E-Mail**: victek@gmail.com
- 📖 **Dokumentation**: Siehe alle `README_*.md` Dateien in Ihrem KissTelegram-Ordner
- 🐛 **Fehlerberichte**: GitHub-Issues (Link in Haupt-README.md)
- 💡 **Feature-Anfragen**: Auch willkommen per E-Mail oder GitHub!

---

## Zusammenfassung: Der Vollständige Prozess

```
1. Bot-Token + Chat-ID von Telegram erhalten ✅
2. boards.txt modifizieren (benutzerdefinierte Partition hinzufügen) ✅
3. Arduino IDE konfigurieren (Benutzerdefinierte Partition, Löschen aktiviert) ✅
4. system_setup.h bearbeiten (Anmeldedaten) ✅
5. RECHTEN USB-Port verbinden ✅
6. Upload #1 (neue Partitionen)✅
7. RECHTEN trennen, LINKEN USB-Port verbinden ✅
8. Upload #2 (KissTelegram-Sketch hochladen) ✅
9. Willkommensnachricht in Telegram erhalten ✅
10. /status senden um zu überprüfen, dass alles funktioniert ✅
```

**Sie sind bereit, erstaunliche Projekte mit KissTelegram zu erstellen!** 🎉
