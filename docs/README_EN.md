# KissTelegram

![License](https://img.shields.io/badge/license-MIT-blue.svg)
![Platform](https://img.shields.io/badge/platform-ESP32--S3-orange.svg)
![Version](https://img.shields.io/badge/version-1.x-green.svg)
![Language](https://img.shields.io/badge/languages-7-brightgreen.svg)

**English** | [Documentation](docs/GETTING_STARTED_EN.md) | [Benchmarks](docs/BENCHMARK.md)

---

> **FIRST TIME USING ESP32-S3 WITH KISSTELEGRAM?**
> **READ THIS FIRST:** [**GETTING_STARTED_EN.md**](docs/GETTING_STARTED_EN.md)
> ESP32-S3 requires a **two-step upload process** due to custom partitions. Ignoring this guide will cause boot errors and wrong partitions!

---

## An Enterprise-Grade Robust Library for Telegram Bots on ESP32-S3

KissTelegram is the **only Telegram library for ESP32** built from scratch for mission-critical applications. Unlike other libraries that rely on Arduino's `String` class (causing memory fragmentation and leaks), KissTelegram uses pure `char[]` arrays for unwavering stability.

### Why KissTelegram?

- Tired of lost projects due to weak libraries, memory leaks, last-minute solutions, lack of support, empty words, terms that don't work, restarts....

- This was my vision and experience with other libraries and this is the result with KissTelegram:

- **Zero Message Loss**: Persistent queue on LittleFS that survives crashes, reboots, and WiFi failures
- **No Memory Leaks**: Pure `char[]` implementation, no String fragmentation
- **SSL/TLS Security**: Secure connections to Telegram API with certificate validation (until 2035)
- **Smart Power Management**: 6 power modes (BOOT, LOW, IDLE, ACTIVE, TURBO, MAINTENANCE)
- **Message Priorities**: CRITICAL, HIGH, NORMAL, LOW with intelligent queue management
- **Turbo Mode**: Batch processing for large message queues (0.9 msg/s)
- **Multilingual i18n**: Compile-time language selection for message sending (7 languages, zero runtime overhead)
- **Enterprise OTA**: Dual-boot firmware updates with automatic rollback and security management
- **100% Flash Utilization**: Custom partition scheme maximizing ESP32-S3's 16MB flash
- **Safer than Espressif OTA**: PIN/PUK authentication, checksum verification, 60s validation window
- **Independent from external libraries**: Everything built from scratch, own JSON parser for KissTelegram libraries.

---

## Hardware Requirements

- **ESP32-S3** with **16MB Flash** / **8MB PSRAM**
- WiFi connectivity
- Arduino IDE or PlatformIO

---

## Installation

### Arduino IDE

1. Download this repository as ZIP
2. Open Arduino IDE → Sketch → Include Library → Add .ZIP Library
3. Select the downloaded file

### PlatformIO

Add to your `platformio.ini`:

```ini
lib_deps =
    https://github.com/victek/KissTelegram.git
```

---

## Custom Partition Scheme

KissTelegram includes an optimized `partitions.csv` that maximizes flash usage:

```csv
# Name,   Type, SubType, Offset,  Size
nvs,      data, nvs,     0x9000,  0x4000,
otadata,  data, ota,     0xd000,  0x2000,
app0,     app,  factory, 0x10000, 0x180000,   # 1.5MB
app1,     app,  ota_0,   0x190000,0x180000,   # 1.5MB
spiffs,   data, spiffs,  0x310000,0xCF0000,   # 13MB!
```

**13MB of SPIFFS storage** - That's 8MB more than Espressif's default schemes!

To use this partition scheme:
1. Copy `partitions.csv` to your project directory
2. In Arduino IDE: Tools ->Partition Scheme ->Custom
3. In PlatformIO: `board_build.partitions = partitions.csv`

---

## Quick Start

### Basic Example

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

### OTA Update Example

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

**OTA Process:**
1. Send `/ota` to your bot
2. Enter PIN with `/otapin YOUR_PIN`
3. Upload the `.bin` firmware file
4. Bot verifies checksum automatically
5. Confirm with `/otaconfirm`
6. After reboot, validate with `/otaok` within 60 seconds
7. Automatic rollback if validation fails!

- Read Readme_KissOTA.md in your preferred language to learn more about the solution.

---

## Key Features Explained

### 1. Persistent Message Queue

Messages are stored on LittleFS with automatic batch deletion:

```cpp
bot.queueMessage(chat_id, "Important message", KissTelegram::PRIORITY_HIGH);
```

- Survives crashes, WiFi disconnections, reboots
- Automatic retry of failed sends
- Smart batch deletion (every 10 messages + when queue is empty)
- Zero message loss guarantee

### 2. Power Management

6 smart power modes adapt to your application's needs:

```cpp
bot.setPowerMode(KissTelegram::POWER_ACTIVE);
bot.setPowerConfig(30, 60, 10); // idle, decay, boot stable times
```

- **POWER_BOOT**: Initial startup phase (10s)
- **POWER_LOW**: Minimal activity, slow polling
- **POWER_IDLE**: No recent activity, reduced checks
- **POWER_ACTIVE**: Normal operation
- **POWER_TURBO**: High-speed batch processing (50ms intervals)
- **POWER_MAINTENANCE**: Manual override for updates
- **Decay Timing for smooth transitions**

### 3. Message Priorities

Four priority levels ensure critical messages are sent first, jumping over lower priority ones:

```cpp
bot.sendMessage(chat_id, "Normal message", KissTelegram::PRIORITY_NORMAL);
bot.sendMessage(chat_id, "Alert!", KissTelegram::PRIORITY_CRITICAL);
```

Queue processes: **CRITICAL /HIGH /NORMAL /LOW**
Internal processes: **OTAMODE /MAINTENANCEMODE**

### 4. SSL/TLS Security

Secure connections with certificate validation (until 2035):

```cpp
bool secure = bot.isSSLSecure();
String sslInfo = bot.getSSLInfo();
bot.testSSLConnection();
```

- Automatic fallback between secure and insecure
- Periodic ping checks to maintain connection
- Reusable connection code saves connection overhead for maximum performance

### 5. Turbo Mode

Automatically activated when sending batches using the /llenar command:

```cpp
bot.enableTurboMode();  // Auto activation
```

- Processes 10 messages per cycle
- 50ms intervals between batches
- Achieves 0.9 msg/s throughput
- Auto-deactivates when queue is sent

### 6. Operation Modes

Preconfigured profiles for different scenarios:

```cpp
bot.setOperationMode(KissTelegram::MODE_RELIABILITY);
```

- **MODE_BALANCED**: Default (polling: 10s, retry: 3)
- **MODE_PERFORMANCE**: Fast (polling: 5s, retry: 2)
- **MODE_POWERSAVE**: Slow (polling: 30s, retry: 2)
- **MODE_RELIABILITY**: Robust (polling: 15s, retry: 5)

### 7. Diagnostics

Complete monitoring and debugging:

```cpp
bot.printDiagnostics();
bot.printStorageStatus();
bot.printPowerStatistics();
bot.printSystemStatus();
```

Shows:
- Free memory (heap/PSRAM)
- Message queue statistics
- Connection quality
- Power mode history
- Storage usage
- Uptime

---

## 8. WiFi Management
- Integrated WiFi Manager activates other tasks only once WiFi is stable
- Prevents race conditions
- Reclaims in-progress messages to FS storage until connection is restored, can hold up to 3500 msg (default but easily expandable, depends on how much space you want to use)
- Connection quality monitoring (EXCELLENT, GOOD, FAIR, POOR, DEAD) and RSSI output level
- You only need to worry about your sketch, add your code and KissTelegram handles critical tasks, WiFi, SSL, Messages, OTA, Power Management, Priorities.. all the work you save....

---

## 9. Key Feature: `/estado` Command

**The most powerful debugging tool you'll ever include in your sketch**

Send `/estado` to your bot and get a **complete health report of your sketch** at that moment, (available in 7 languages):

```
KissTelegram_test, [15/12/2025 13:11]
🔧 KissTelegram v0.9.0
📦 Build: Dec 15 2025 13:10:23 (0xD984C13E)

✅ SYSTEM RELIABILITY
✓ System: RELIABLE
✓ Messages sent: 940
📨 Pending messages: 70
✓ Lost messages: 0
🗑️ Discards (full queue): 0

⚠️ EXTERNAL ADVERSITIES
⚠️ Total errors: 0
🔄 Recovered (fallback): 0
📡 WiFi drops: 0

📊 TECHNICAL INFORMATION
⏱️ Uptime: 0h 0m
🧠 Free RAM: 223960 bytes
💾 Free PSRAM: 1027820 bytes
💽 Free FS: 13549568 bytes
📦 Max. in FS: 3500 Messages
⚡ Power Mode: 3 
📶 WiFi Signal: -64 dBm (Fair)
🔒 SSL: SECURE
🚀 Turbo: INACTIVE
🤖 Auto-messages: YES

```

**Why `/estado` is essential:**
- Instant system health check
- WiFi quality monitoring (diagnose connectivity issues)
- Memory leak detection (watch free heap)
- Message queue status (see pending/failed messages)
- Uptime tracking (stability monitoring)
- Your first diagnostic tool

**Pro tip:** Make `/estado` your first message after each firmware update to verify everything works!

---

## 10. NTP
- Own code to synchronize/resynchronize for SSL. GNSS, LTE and Scheduler (Enterprise Edition)
---

## 11. Documentation (7 Languages)

- **[GETTING_STARTED_EN.md](docs/GETTING_STARTED_EN.md)** - **START HERE!** Complete guide from receiving the ESP32-S3 to the first message sent to Telegram
- **[README_EN.md](docs/README_EN.md)** (this file) - Feature overview, quick start, API reference
- **[BENCHMARK.md](docs/BENCHMARK.md)** - Technical comparison with 6 Telegram libraries (English only, but self-explanatory)
- **[README_KissOTA_XX.md](docs/README_KissOTA_EN.md)** - Great value as it details the steps of the OTA update system (7 languages: EN, ES, FR, IT, DE, PT, CN)

**Choose your language:** All constructor messages sent to Telegram are displayed in 7 languages via language selection during compilation (lang.h).


## OTA Security Advantages

KissTelegram OTA is **much safer than Espressif's architecture and saves space on your ESP32S3**:

| Feature | KissTelegram OTA | Espressif OTA |
|---------|------------------|---------------|
| Authentication | PIN + PUK | None |
| Checksum Confirmation | Automatic CRC32 | Manual |
| Backup and Rollback | Automatic | Manual |
| Validation Window | 60s with `/otaok` | None |
| Boot Loop Detection | Yes | No |
| Telegram Integration | Native | Requires custom code |
| Flash Optimization | 13MB SPIFFS | 5MB SPIFFS |

---

## API Reference

### Initialization

```cpp
KissTelegram(const char* token);
void enable();
void disable();
```

### Messaging

```cpp
bool sendMessage(const char* chat_id, const char* text,
                 MessagePriority priority = PRIORITY_NORMAL);
bool sendMessageDirect(const char* chat_id, const char* text);
bool queueMessage(const char* chat_id, const char* text,
                  MessagePriority priority = PRIORITY_NORMAL);
void processQueue();
```

### Configuration

```cpp
void setMinMessageInterval(int milliseconds);
void setMaxMessageSize(int size);
void setMaxRetryAttempts(int attempts);
void setPollingTimeout(int seconds);
void setOperationMode(OperationMode mode);
void setPowerMode(PowerMode mode);
```

### Monitoring

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

## Examples

Check the .ino included in the library to explore some scenarios and features and my coding style in KissTelegram. Even better, uncomment your language in [lang.h] to receive messages from the main constructors (.cpp) in your local language, if all languages are commented the messages are in Spanish, the default language:

Code conventions are in English, but thoughts and comments are in Spanish, my native language, use your online translator, the code is easy, within the code is my vision and the concept of KissTelegram...

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
## Basic Configuration Setup
- Rename system_setup_template.h to system_setup.h in your KissTelegram folder to start compilation.
- Replace the following lines with your credentials.

````cpp
#define KISS_FALLBACK_BOT_TOKEN "YOUR_TELEGRAM_BOT_TOKEN"
#define KISS_FALLBACK_CHAT_ID "YOUR_CHAT_ID number"
#define KISS_FALLBACK_WIFI_SSID "YOUR_WIFI_SSID"
#define KISS_FALLBACK_WIFI_PASSWORD "YOUR_WIFI_PASSWORD"
#define KISS_FALLBACK_OTA_PIN "0000"
#define KISS_FALLBACK_OTA_PUK "00000000"
````

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

---

## Architecture, Vision, Concept, Solutions and Design (and responsible for any malfunction, it's me...)

**Vicente Soriano**
Email: victek@gmail.com
GitHub: [victek](https://github.com/victek/KissTelegram)

**Contributors**
- Many AI assistants in Translations, Code, Troubleshooting and many hours trying to prevent them from reinventing the wheel.....

---


## Contributing

Contributions are welcome! Please feel free to submit a Pull Request or send me an email, but I prefer a PR so others can find your question.

---

## Support

If you find this library useful, please consider:
- Starring this repository
- Reporting bugs through GitHub Issues
- Sharing your projects using KissTelegram
- Telling your friends about the features and solutions of this library
- Proposing use cases and experiences with KissTelegram

---
