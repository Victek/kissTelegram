# Erste Schritte mit KissTelegram auf ESP32-S3

**Vollständige Anleitung zur Einrichtung Ihres ESP32-S3 von Grund auf bis zur ersten Telegram-Nachricht**

> ⚠️ **KRITISCH**: Lesen Sie diese Anleitung vollständig, bevor Sie Firmware hochladen. Der ESP32-S3 N16R8 erfordert einen **zweistufigen Upload-Prozess** aufgrund benutzerdefinierter Partitionen. Das Überspringen von Schritten verursacht Fehler!

---

## Inhaltsverzeichnis

1. [Bevor Sie Beginnen](#bevor-sie-beginnen)
2. [Erstellen Sie Ihren Telegram-Bot](#erstellen-sie-ihren-telegram-bot)
3. [Hardware-Konfiguration](#hardware-konfiguration)
4. [Arduino IDE-Konfiguration](#arduino-ide-konfiguration)
5. [Erster Upload (Partitionen mit esptool erstellen)](#erster-upload)
6. [Konfigurationsdateien](#konfigurationsdateien)
7. [Erfolg! Was Kommt Als Nächstes?](#erfolg-was-kommt-als-nächstes)

---

## Bevor Sie Beginnen

### Was Sie Benötigen

- **ESP32-S3 N16R8** (16MB Flash / 8MB PSRAM)
- **Zwei USB-C-Kabel** (zum Wechseln zwischen Bootloader- und OTG-Ports)
- **Arduino IDE 2.x** oder höher
- **Windows-PC** (diese Anleitung ist Windows-fokussiert, passen Sie Pfade für Linux/Mac an)
- **Telegram-Konto** auf Ihrem Telefon

### Was Dies Anders Macht

Ihr neuer ESP32-S3 N16R8 wird mit einer integrierten RGB-LED-Demo-App geliefert. KissTelegram **ersetzt die Partitionstabelle vollständig**, um Ihre 16MB Flash zu maximieren:

| Partition | Standard Espressif | KissTelegram Benutzerdefiniert |
|-----------|-------------------|--------------------------------|
| App-Bereich | 1.5 MB | 4.5 MB (3x größer!) |
| Dateisystem | 5 MB | 13 MB (2.6x größer!) |
| Gesamt Verwendet | 6.5 MB | 17.5 MB |

Deshalb ist der zweistufige Upload-Prozess erforderlich: **die Partitionstabelle ändert sich zwischen den Uploads**.

---

## Erstellen Sie Ihren Telegram-Bot

### Schritt 1: Mit BotFather Sprechen

1. Öffnen Sie Telegram auf Ihrem Telefon
2. Suchen Sie nach `@BotFather` (offizieller Bot, hat blaues Häkchen)
3. Starten Sie Gespräch mit `/start`
4. Erstellen Sie Ihren Bot mit `/newbot`
5. Wählen Sie einen Namen (Beispiel: "Mein Heim-Assistent")
6. Wählen Sie einen Benutzernamen (muss mit `bot` enden, Beispiel: "meinheim_assistent_bot")
7. **Speichern Sie Ihr Bot-Token** - sieht so aus: `1234567890:ABCdefGHIjklMNOpqrsTUVwxyz`

### Schritt 2: Holen Sie Ihre Chat-ID

**Methode 1: Verwenden eines Bots (Einfacher)**

1. Suchen Sie nach `@ChatIDHelperBot` in Telegram
2. Starten Sie Gespräch mit `/start`
3. Er antwortet mit Ihrer **Chat-ID** (eine Nummer wie `123456789`)
4. **Speichern Sie diese Nummer** - Sie werden sie in der Konfiguration benötigen

**Methode 2: Verwenden eines Webbrowsers**

1. Senden Sie eine beliebige Nachricht an Ihren neu erstellten Bot
2. Öffnen Sie Browser und besuchen Sie:
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

### Die Zwei USB-C-Ports Verstehen

Ihr ESP32-S3 N16R8 hat **zwei USB-C-Ports**:

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
- Wird für den **initialen Firmware-Upload** verwendet
- Wird für den **Bootloader-Modus** verwendet
- Verwenden Sie diesen, wenn Arduino IDE "Connecting..." sagt

**LINKER PORT (OTG):**
- Wird für den **Normalbetrieb** nach dem ersten Upload verwendet
- Wird für den **zweiten Upload** (Partitionskorrektur) verwendet
- Verwenden Sie diesen für Serial Monitor im Normalbetrieb

---

## Arduino IDE-Konfiguration

### Schritt 1: Versteckte Dateien Anzeigen (Windows)

1. Öffnen Sie den **Datei-Explorer**
2. Klicken Sie auf Registerkarte **Ansicht** → **Anzeigen** → Aktivieren Sie:
   - ✅ Dateinamenerweiterungen
   - ✅ Ausgeblendete Elemente
3. In Registerkarte **Filter**: **Alle Dateitypen**

### Schritt 2: boards.txt Modifizieren

1. Navigieren Sie zu:
   ```
   C:\Users\<IHR_BENUTZERNAME>\AppData\Local\Arduino15\packages\esp32\hardware\esp32\3.3.4\
   ```
   (Ersetzen Sie `3.3.4` mit Ihrer ESP32-Core-Version, falls abweichend)

2. Finden und öffnen Sie `boards.txt` (verwenden Sie Notepad++ oder einen beliebigen Texteditor)

3. Drücken Sie `Ctrl+F` und suchen Sie nach:
   ```
   gen4esp32_4MBapp_4MBota_7MBspiffs
   ```

4. **Unmittelbar unter dieser Zeile**, fügen Sie diese drei Zeilen ein:
   ```
   gen4-ESP32-S3R8n16.menu.PartitionScheme.gen4esp32scheme2=Custom (4MB APP/12MB LtlFS)
   gen4-ESP32-S3R8n16.menu.PartitionScheme.gen4esp32scheme2.build.custom_partitions=partitions
   gen4-ESP32-S3R8n16.menu.PartitionScheme.gen4esp32scheme2.upload.maximum_size=4718592
   ```

5. **Speichern** und schließen Sie `boards.txt`

6. Falls Arduino IDE geöffnet war, **schließen und neu starten**

### Schritt 3: Arduino IDE Konfigurieren

1. **Öffnen** Sie Ihren KissTelegram-Sketch-Ordner (mit `.ino`, `.h`, `.cpp`, und `partitions.csv`)

2. In Arduino IDE, gehen Sie zu **Werkzeuge** → **Board** → **4D Systems gen4-ESP32-S3R8n16**

3. **Werkzeuge** → **Board-Daten Neu Laden** (Sie sehen eine Bestätigung unten)

4. **Konfigurieren Sie alle Werkzeug-Menü-Optionen:**

   | Einstellung | Wert |
   |-------------|------|
   | **Board** | 4D Systems gen4-ESP32-S3R8n16 |
   | **USB CDC On Boot** | Enabled |
   | **Flash Size** | 16MB (128Mb) |
   | **Partition Scheme** | **Custom (4MB APP/12MB LtlFS)** ⚠️ |
   | **PSRAM** | OPI PSRAM |
   | **Upload Speed** | 921600 |
   | **Erase All Flash Before Sketch Upload** | **Enabled** ⚠️ |

   ⚠️ **Kritische Einstellungen** - doppelt überprüfen!

5. **Werkzeuge** → **Serieller Monitor** → Setzen Sie Geschwindigkeit auf **115200**

---

## Erster Upload (Häufige Probleme)

### Warum Zwei Uploads Benötigt Werden

**Das Problem:**
- Erster Upload: Arduino verwendet die **alte Partitionstabelle** zum Schreiben der Firmware
- ESP32 startet: Findet die **neue Partitionstabelle** (von `partitions.csv`)
- **Diskrepanz** zwischen wo Firmware geschrieben wurde vs wo ESP32 danach sucht
- Ergebnis: Boot-Fehler, Partitionsfehler, Abstürze

**Die Lösung:**
Zwei Uploads stellen sicher, dass Firmware an der **korrekten Stelle** geschrieben wird, die von der neuen Partitionstabelle definiert ist.

---

### Upload #1: Initialer Flash (Bootloader Brennen)

1. **Verbinden Sie RECHTEN USB-C-Port** (nahe der Power-LED) mit Ihrem PC

2. **Wählen Sie Port**: Werkzeuge → Port → Wählen Sie den erscheinenden COM-Port

3. **Überprüfen Sie Einstellungen**:
   - ✅ Erase All Flash Before Sketch Upload: **Enabled**
   - ✅ Partition Scheme: **Custom (4MB APP/12MB LtlFS)**
   

4. **Werkzeuge, Bootloader Brennen** (Klicken Sie auf diese Option)
   - ✅ Werkzeuge ➡️, am Ende des Dropdown-Menüs finden Sie 'Bootloader Brennen'
   - ✅ Klicken Sie hier und es wird die neue Partition schreiben, mit esptool
   - Dauert 53.6 Sekunden und Sie haben die neue Partition für KissTelegram 

Fahren Sie fort mit Upload #2.

---

### Upload #2: Sketch-Upload

1. **Trennen Sie RECHTEN USB-C-Port**

2. **Verbinden Sie LINKEN USB-C-Port** (OTG-Port) mit Ihrem PC

3. **Wählen Sie neuen Port**: Werkzeuge → Port → Wählen Sie den neuen COM-Port
   - **Wichtig**: Port-Nummer wird sich ändern! Suchen Sie nach Daten im Seriellen Monitor zur Bestätigung des korrekten Ports, zum Beispiel, drücken Sie Reset des ESP32s3 bis Sie Daten als Antwort sehen

4. **Überprüfen Sie Einstellungen erneut**:
   - ✅ Erase All Flash Before Sketch Upload: **Enabled**
   - ✅ Partition Scheme: **Custom (4MB APP/12MB LtlFS)**

5. **Drücken Sie Upload erneut** (`Ctrl+U`)

6. **Warten Sie ~2-3 Minuten** (Löschen + Hochladen)

7. **Öffnen Sie Seriellen Monitor** - Sie sollten jetzt sehen (wenn Sie die Anmeldedaten korrekt gesetzt haben 
in system_setup.h (dem umbenannten system_setup_template)):
   ```
   ✅ KissTelegram v0.9.x
   ✅ WiFi verbunden
   ✅ Telegram-Bot aktiviert
   ✅ System bereit
   ```

8. **Überprüfen Sie Telegram** - Sie erhalten die Willkommensnachricht:
   ```
   📦 Hallo! KissTelegram ist bereit.
   🔌 Build: 2025-12-12 10:30:45 (0xABCD1234)
   📡 WiFi-Signal: -59 dBm (Gut)
   ✅ 0 Nachrichten in Warteschlange
   ```

**Erfolg!** Ihr ESP32-S3 führt jetzt KissTelegram mit den korrekten Partitionen aus.

---

### Zukünftige Uploads

**Gute Nachrichten:** Nach den zwei initialen Uploads funktionieren alle zukünftigen Uploads normal:

- Verwenden Sie **LINKEN USB-C-Port** (OTG)
- **Benötigen nicht** "Erase All Flash" mehr (es sei denn, Sie haben Änderungen an NVRAM-Daten vorgenommen)
- Laden Sie einmal hoch und es funktioniert sofort

---

## Konfigurationsdateien

### system_setup.h (Vor Dem Ersten Upload Erforderlich!)

**Vor dem Kompilieren:**

1. Navigieren Sie zu Ihrem KissTelegram-Ordner
2. Finden Sie `system_setup_template.h`
3. **Benennen Sie es um** in `system_setup.h`
4. **Öffnen** Sie `system_setup.h` und füllen Sie aus:

```cpp
// Ihr Telegram-Bot (von BotFather)
#define KISS_FALLBACK_BOT_TOKEN "1234567890:ABCdefGHIjklMNOpqrsTUVwxyz"

// Ihre Chat-ID (von @userinfobot)
#define KISS_FALLBACK_CHAT_ID "123456789"

// Ihre WiFi-Anmeldedaten
#define KISS_FALLBACK_WIFI_SSID "IhrWiFiName"
#define KISS_FALLBACK_WIFI_PASSWORD "IhrWiFiPasswort"

// OTA-Sicherheit (ändern Sie Standard-PIN/PUK!)
#define KISS_FALLBACK_OTA_PIN "0000"        // 4 Ziffern
#define KISS_FALLBACK_OTA_PUK "00000000"    // 8 Ziffern
```

5. **Speichern** Sie die Datei

**⚠️ Sicherheitswarnung:** Ändern Sie Standard-PIN (`0000`) und PUK (`00000000`) zu Ihren eigenen Geheimnissen!

---

### lang.h (Optional: Wählen Sie Ihre Sprache)

KissTelegram unterstützt 7 Sprachen für Systemnachrichten:

```cpp
// In lang.h, kommentieren Sie EINE Sprache aus:

// #define LANG_CN  // 中文 (Chinesisch)
// #define LANG_DE  // Deutsch (Deutsch)
// #define LANG_EN  // English (Englisch)
// #define LANG_FR  // Français (Französisch)
// #define LANG_IT  // Italiano (Italienisch)
// #define LANG_PT  // Português (Portugiesisch)
// #define LANG_ES  // Español (Spanisch) - STANDARD wenn alle auskommentiert
```

Wählen Sie Ihre Sprache **vor dem Kompilieren** für lokalisierte Nachrichten.

---

## Erfolg! Was Kommt Als Nächstes?

### Überprüfen, Dass Alles Funktioniert

1. **Senden Sie `/status` an Ihren Bot** in Telegram - Sie erhalten einen detaillierten Statusbericht:
   ```
   📦 KissTelegram v1.x.x
   🎯 SYSTEMZUVERLÄSSIGKEIT
   ✅ System: ZUVERLÄSSIG
   ✅ Gesendete Nachrichten: 2
   💾 Ausstehende Nachrichten: 0
   📡 WiFi-Signal: -59 dBm (Gut)
   🔋 Betriebszeit: 123 Sekunden
   💾 Freier Speicher: 223 KB
   ```

2. **Überprüfen Sie Seriellen Monitor** - sollte keine Fehler zeigen

3. **Testen Sie Befehle**:
   - `/start` - Willkommensnachricht
   - `/help` - Verfügbare Befehle
   - `/status` - Systemstatus (Gesundheitsprüfung)

---

### OTA-Updates Verstehen

Sobald KissTelegram läuft, können Sie Firmware **über Telegram** aktualisieren (kein USB-Kabel!):

1. Senden Sie `/ota` an Ihren Bot
2. Geben Sie PIN ein: `/otapin 0000` (oder Ihre benutzerdefinierte PIN)
3. **Senden Sie Ihre Firmware-Datei `.bin`** (ziehen und ablegen in Telegram)
4. Bot überprüft Prüfsumme automatisch
5. Bestätigen Sie: `/otaconfirm`
6. ESP32 startet mit neuer Firmware neu
7. **Innerhalb von 60 Sekunden**, senden Sie `/otaok` zur Bestätigung, dass es funktioniert
8. Wenn Sie nicht bestätigen, **rollt ESP32 automatisch zurück** zur vorherigen Firmware!

📖 **Mehr lesen:** Siehe `README_KissOTA_DE.md` für vollständige OTA-Dokumentation.

---

### Beispielcode Erkunden

Das Beispiel `suite_kiss.ino` demonstriert:

- ✅ WiFi-Verwaltung mit Qualitätsüberwachung
- ✅ Nachrichtenwarteschlange mit Prioritäten
- ✅ Energieverwaltungsmodi
- ✅ Befehlsverarbeitung (`/start`, `/help`, `/status`, etc.)
- ✅ OTA-Updates über Telegram
- ✅ Absturz-Wiederherstellung und Persistenz
- ✅ Sichere SSL/TLS-Verbindungen

**Profi-Tipp:** Verwenden Sie den `/status`-Befehl als Ihr **Gesundheitsüberwachungswerkzeug** - es ist Ihr Fenster in die KissTelegram-Interna!

---

### Häufige Fehlersuche

**Problem: "Port nicht gefunden" oder "Zugriff verweigert"**
- Windows hat den Port gesperrt. USB trennen, 5s warten, wieder verbinden.
- Versuchen Sie ein anderes USB-Kabel (einige sind nur zum Laden, nicht für Daten)

**Problem: "Timeout beim Warten auf Gerät" während des Uploads**
- Falscher USB-Port! Denken Sie daran: RECHTER Port für ersten Upload, LINKER Port für zweiten
- Halten Sie BOOT-Taste am ESP32 während Sie auf Upload klicken, loslassen nachdem "Connecting..." erscheint

**Problem: Serieller Monitor zeigt Müll-Zeichen**
- Falsche Baudrate. Setzen Sie auf **115200** im Dropdown des Seriellen Monitors

**Problem: Bot antwortet nicht in Telegram**
- Überprüfen Sie, dass `system_setup.h` korrektes Bot-Token und Chat-ID hat
- Überprüfen Sie, dass WiFi-Anmeldedaten korrekt sind
- Öffnen Sie Seriellen Monitor und suchen Sie nach WiFi-Verbindungsnachrichten

**Problem: Kompilierungsfehler "Partitionstabelle passt nicht"**
- Haben benutzerdefinierte Partition nicht korrekt zu `boards.txt` hinzugefügt
- Oder haben "Custom (4MB APP/12MB LtlFS)" nicht in Werkzeuge → Partition Scheme gewählt

---

### Mehr Hilfe Erhalten

- 📧 **E-Mail**: victek@gmail.com
- 📖 **Dokumentation**: Siehe alle `README_*.md`-Dateien in Ihrem KissTelegram-Ordner
- 🐛 **Fehlerberichte**: GitHub issues (Link in Haupt-README.md)
- 💡 **Feature-Anfragen**: Auch willkommen per E-Mail oder GitHub!

---

## Zusammenfassung: Der Vollständige Prozess

```
1. Bot-Token + Chat-ID von Telegram erhalten ✅
2. boards.txt modifizieren (benutzerdefinierte Partition hinzufügen) ✅
3. Arduino IDE konfigurieren (Custom-Partition, Erase aktiviert) ✅
4. system_setup.h bearbeiten (Anmeldedaten) ✅
5. RECHTEN USB-Port verbinden ✅
6. Upload #1 (Bootloader Brennen) ✅
7. RECHTEN trennen, LINKEN USB-Port verbinden ✅
8. Upload #2 (KissTelegram-Sketch Hochladen) ✅
9. Willkommensnachricht in Telegram empfangen ✅
10. /status senden um zu überprüfen, dass alles funktioniert ✅
```

**Sie sind bereit, erstaunliche Projekte mit KissTelegram zu bauen!** 🎉
