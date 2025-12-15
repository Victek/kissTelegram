# KissTelegram

![License](https://img.shields.io/badge/license-MIT-blue.svg)
![Platform](https://img.shields.io/badge/platform-ESP32--S3-orange.svg)
![Version](https://img.shields.io/badge/version-1.x-green.svg)
![Language](https://img.shields.io/badge/languages-7-brightgreen.svg)

---

## 🌍 README Available in 7 Languages

**[English](docs/README.md)** | **[Español](docs/README_ES.md)** | **[Français](docs/README_FR.md)** | **[Deutsch](docs/README_DE.md)** | **[Italiano](docs/README_IT.md)** | **[Português](docs/README_PT.md)** | **[中文](docs/README_CN.md)**

📚 **Detailed guides and documentation**: See the [docs/](docs/) folder for GETTING_STARTED guides, benchmarks, and OTA documentation in all languages.

---

**English** | [Documentation](docs/GETTING_STARTED.md) | [Benchmarks](docs/BENCHMARK.md)

---

> 🚨 **FIRST TIME USING ESP32-S3 WITH KISSTELEGRAM?**
> **READ THIS FIRST:** [**GETTING_STARTED.md**](docs/GETTING_STARTED.md) ⚠️ Available in 7 languages !!.
> ESP32-S3 requires a **two-step upload process** due to custom partitions. Skipping this guide will cause boot errors!

---

## A Robust, Enterprise-Grade Telegram Bot Library for ESP32-S3

KissTelegram is the **only ESP32 Telegram library** built from the ground up for mission-critical applications. Unlike other libraries that rely on Arduino `String` class (causing memory fragmentation and leaks), KissTelegram uses pure `char[]` arrays for rock-solid stability.

### Why KissTelegram?

- Desperate of lost projects by weak libraries, memmory leaks, last minute solutions, lack of support, buzzwords, reboots....

- My vision, now the facts:

- **Zero Message Loss**: Persistent LittleFS queue survives crashes, reboots, and WiFi failures
- **No Memory Leaks**: Pure `char[]` implementation, no String fragmentation
- **SSL/TLS Security**: Secure connections to Telegram API with certificate validation
- **Smart Power Management**: 6 power modes (BOOT, LOW, IDLE, ACTIVE, TURBO, MAINTENANCE)
- **Message Priorities**: CRITICAL, HIGH, NORMAL, LOW with intelligent queue management
- **Turbo Mode**: Batch processing for large message queues (0,9 msg/s)
- **Multilingual i18n**: Compile-time language selection (7 languages, no runtime overhead)
- **Enterprise OTA**: Dual-boot firmware updates with automatic rollback and security gateway
- **100% Flash Utilization**: Custom partition scheme maximizes ESP32-S3 16MB flash
- **More Secure than Espressif OTA**: PIN/PUK authentication, checksum verification, 60s validation window
- **Independent from external libraries**: Everything made from scratch, own Json parser.

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

**13MB of SPIFFS storage** - that's 8MB more than Espressif's default schemes!

To use this partition scheme:
1. Copy `partitions.csv` to your project directory
2. In Arduino IDE: Tools → Partition Scheme → Custom
3. In PlatformIO: `board_build.partitions = partitions.csv`
4. In Platformio: `platformio.ini` in /src folder

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

### OTA Updates Example

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
3. Upload `.bin` firmware file
4. Bot verifies checksum automatically
5. Confirm with `/otaconfirm`
6. After reboot, validate with `/otaok` within 60 seconds
7. Automatic rollback if validation fails!

- Read Readme_KissOTA.md in your preferred language to know more about the solution.

---

## Key Features Explained

### 1. Persistent Message Queue

Messages are stored in LittleFS with automatic batch deletion:

```cpp
bot.queueMessage(chat_id, "Important message", KissTelegram::PRIORITY_HIGH);
```

- Survives crashes, WiFi disconnections, reboots
- Automatically retries failed sends
- Smart batch deletion (every 10 messages + on queue empty)
- Zero message loss guarantee

### 2. Power Management

6 intelligent power modes adapt to your application's needs:

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
- **Decay Timing for smooth switch**

### 3. Message Priorities

Four priority levels ensure critical messages are sent first:

```cpp
bot.sendMessage(chat_id, "Normal message", KissTelegram::PRIORITY_NORMAL);
bot.sendMessage(chat_id, "Alert!", KissTelegram::PRIORITY_CRITICAL);
```

Queue processes: **CRITICAL → HIGH → NORMAL → LOW**
Internal processes: **OTAMODE → MAINTENANCEMODE**

### 4. SSL/TLS Security

Secure connections with certificate validation:

```cpp
bool secure = bot.isSSLSecure();
String sslInfo = bot.getSSLInfo();
bot.testSSLConnection();
```

- Automatic secure/insecure fallback
- Periodic ping checks to maintain connection
- Reusable connection code saves connection header for highest throughput

### 5. Turbo Mode

Automatically activates when queue > 100 messages:

```cpp
bot.enableTurboMode();  // Manual activation
```

- Processes 10 messages per cycle
- 50ms intervals between batches
- Achieves 15-20 msg/s throughput
- Auto-deactivates when queue is sent

### 6. Operation Modes

Pre-configured profiles for different scenarios:

```cpp
bot.setOperationMode(KissTelegram::MODE_RELIABILITY);
```

- **MODE_BALANCED**: Default (polling: 10s, retry: 3)
- **MODE_PERFORMANCE**: Fast (polling: 5s, retry: 2)
- **MODE_POWERSAVE**: Slow (polling: 30s, retry: 2)
- **MODE_RELIABILITY**: Robust (polling: 15s, retry: 5)

### 7. Diagnostics

Comprehensive monitoring and debugging:

```cpp
bot.printDiagnostics();
bot.printStorageStatus();
bot.printPowerStatistics();
bot.printSystemStatus();
```

Displays:
- Free memory (heap/PSRAM)
- Message queue stats
- Connection quality
- Power mode history
- Storage usage
- Uptime

---

## 8. WiFi Management
- WiFi Manager built in triggers other task begin until WiFi is stable
- Prevents Race conditions
- Claims ongoing messages to go to FS storage until connection restablished, can Keep up to 3500 msg (default but easily expanded)
- Connection quality monitoring (EXCELLENT, GOOD, FAIR, POOR, DEAD) and RSSI output level
- You have only to take care about your sketch, use KissTelegram for the rest

---

## 9. Killer Feature: `/estado` Command

**The most powerful debugging tool you'll ever use:**

Send `/estado` to your bot and get a **complete health report** in seconds:

```
📦 KissTelegram v0.9.x
🎯 SYSTEM RELIABILITY
✅ System: RELIABLE
✅ Messages sent: 5,234
💾 Messages pending: 0
📡 WiFi Signal: -59 dBm (Good)
🔌 WiFi reconnections: 2
⏱️ Uptime: 86,400 seconds (24h)
💾 Free memory: 223 KB
📊 Queue statistics: All systems operational
```

**Why `/estado` is essential:**
- ✅ Instant system health check
- ✅ WiFi quality monitoring (diagnose connectivity issues)
- ✅ Memory leak detection (watch free heap)
- ✅ Message queue status (see pending/failed messages)
- ✅ Uptime tracking (stability monitoring)
- ✅ Available in 7 languages
- ✅ Your first tool when debugging problems

**Pro tip:** Make `/estado` your first message after every firmware update to verify everything works!

---

## 10. NTP
- Own code to sync/rsync for SSL and Scheduler (Enterprise Edition)
---

## 11. Documentation (7 Languages)

- **[GETTING_STARTED.md](GETTING_STARTED.md)** - **START HERE!** Complete guide from unboxing ESP32-S3 to first Telegram message
- **[README.md](README.md)** (this file) - Feature overview, quick start, API reference
- **[BENCHMARK.md](BENCHMARK.md)** - Technical comparison vs 6 other Telegram libraries (English only)
- **[README_KissOTA_XX.md](README_KissOTA_EN.md)** - OTA update system documentation (7 languages: EN, ES, FR, IT, DE, PT, CN)

**Choose your language:** All KissTelegram system messages support 7 languages via compile-time selection.


## OTA Security Advantages

KissTelegram OTA is **more secure than Espressif's implementation**:

| Feature | KissTelegram OTA | Espressif OTA |
|---------|------------------|---------------|
| Authentication | PIN + PUK | None |
| Checksum Verification | CRC32 automatic | Manual |
| Backup & Rollback | Automatic | Manual |
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

See the included .ino en the library to explore some scenarios and features and my code style of KissTelegram. Better, uncomment your language in [lang.h] to receive messages from the main constructors (.cpp) in your local language, if all languages are commented you get messages in spanish, the default language:

Code conventions are in English, but thoughs and comments in my native language, use your online translator, code is easy, behind the code is much more complicated ...

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
## Basic configuration settings
- Rename system_setup_template.h to system_setup.h in your KissTelegram folder to start compilation.
- Replace the following lines by your settings.
- If you changed any of these lines and flash your ESP32 remember to aply a complete Flash erase partition, since all these credentials are keept in NVRAM.

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

## Architecture, Vision, Concept, Solutions && Design (and the responsible of any malfunction)

**Vicente Soriano**
Email: victek@gmail.com
GitHub: [victek](https://github.com/victek)

**Contributors**
- Many IA assistants in Translations, Code, Troubleshooting and jokes.

---


## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

---

## Support

If you find this library useful, please consider:
- Starring this repository
- Reporting bugs via GitHub Issues
- Sharing your projects using KissTelegram

---

