# KissOTA - OTA Update System for ESP32

## Table of Contents
- [Overview](#overview)
- [Key Features](#key-features)
- [Comparison with Other OTA Systems](#comparison-with-other-ota-systems)
- [System Architecture](#system-architecture)
- [Advantages over Espressif's Standard OTA](#advantages-over-espressifs-standard-ota)
- [Hardware Requirements](#hardware-requirements)
- [Partition Configuration](#partition-configuration)
- [OTA Process Flow](#ota-process-flow)
- [Available Commands](#available-commands)
- [Security Features](#security-features)
- [Error Handling and Recovery](#error-handling-and-recovery)
- [Basic Usage](#basic-usage)
- [Frequently Asked Questions (FAQ)](#frequently-asked-questions-faq)

---

## Overview

KissOTA is an advanced Over-The-Air (OTA) update system designed specifically for ESP32-S3 devices with PSRAM. Unlike Espressif's standard OTA system, KissOTA provides multiple layers of security, validation, and automatic recovery, all integrated with Telegram for secure remote updates.

**Author:** Vicente Soriano (victek@gmail.com)
**Current version:** 0.9.0
**License:** Included in KissTelegram Suite

---

## Key Features

### 🔒 Robust Security
- **PIN/PUK Authentication**: Access control via PIN and PUK codes
- **Automatic lockout**: After 3 failed PIN attempts
- **CRC32 Verification**: Firmware integrity validation before flashing
- **Post-flash validation**: Requires manual user confirmation

### 🛡️ Failure Protection
- **Automatic backup**: Copy of current firmware before updating
- **Automatic rollback**: Restoration if new firmware fails
- **Boot loop detection**: If fails 3 times in 5 minutes, automatic rollback
- **Factory partition**: Final safety net if everything else fails
- **Global timeout**: 7 minutes to complete entire process

### 💾 Efficient Resource Usage
- **PSRAM Buffer**: Uses 7-8MB of PSRAM to download without touching flash
- **Space saving**: Only needs factory + app partitions (no OTA_0/OTA_1)
- **Automatic cleanup**: Removes temporary files and old backups

### 📱 Telegram Integration
- **Remote update**: Send .bin directly via Telegram
- **Real-time feedback**: Progress and status messages
- **Multi-language**: Support for 7 languages (ES, EN, FR, IT, DE, PT, CN)

---

## Comparison with Other OTA Systems

### KissOTA vs Other Popular Solutions

| Feature | KissOTA | AsyncElegantOTA | ArduinoOTA | ESP-IDF OTA | ElegantOTA |
|---------|---------|-----------------|------------|-------------|------------|
| **Transport** | 📱 Telegram | 🌐 HTTP Web | 🌐 HTTP Web | 🌐 HTTP | 🌐 HTTP Web |
| **Remote access** | ✅ Global without config | ❌ LAN only* | ❌ LAN only* | ❌ LAN only* | ❌ LAN only* |
| **Requires known IP** | ❌ No | ✅ Yes | ✅ Yes | ✅ Yes | ✅ Yes |
| **Port forwarding** | ❌ Not needed | ⚠️ For remote access | ⚠️ For remote access | ⚠️ For remote access | ⚠️ For remote access |
| **Web server** | ❌ No | ✅ AsyncWebServer | ✅ WebServer | ✅ Configurable | ✅ WebServer |
| **Authentication** | 🔒 PIN/PUK (robust) | ⚠️ Basic user/pass | ⚠️ Optional password | ⚠️ Basic | ⚠️ Basic user/pass |
| **Firmware backup** | ✅ In LittleFS | ❌ No | ❌ No | ❌ No | ❌ No |
| **Automatic rollback** | ✅✅✅ 3 levels | ⚠️ Limited (2 part.) | ❌ No | ⚠️ Limited (2 part.) | ⚠️ Limited (2 part.) |
| **Manual validation** | ✅ 60s with /otaok | ❌ No | ❌ No | ⚠️ Optional | ❌ No |
| **Boot loop detection** | ✅ Automatic | ❌ No | ❌ No | ❌ No | ❌ No |
| **Download buffer** | 💾 PSRAM (8MB) | 🔥 Flash | 🔥 Flash | 🔥 Flash | 🔥 Flash |
| **Real-time progress** | ✅ Telegram messages | ✅ Web UI | ⚠️ Serial only | ⚠️ Configurable | ✅ Web UI |
| **User interface** | 📱 Telegram chat | 🖥️ Web browser | 🖥️ Web browser | ⚡ Programmatic | 🖥️ Web browser |
| **Dependencies** | KissTelegram | ESPAsyncWebServer | ESP mDNS | None (native) | WebServer |
| **Flash required** | ~3.5 MB (app) | ~7 MB (OTA_0+1) | ~7 MB (OTA_0+1) | ~7 MB (OTA_0+1) | ~7 MB (OTA_0+1) |
| **Internet security** | ✅ High (Telegram API) | ⚠️ Vulnerable if exposed | ⚠️ Vulnerable if exposed | ⚠️ Vulnerable if exposed | ⚠️ Vulnerable if exposed |
| **Easy to use** | ✅✅ Just send .bin | ✅ Intuitive web UI | ⚠️ Requires setup | ⚠️ High complexity | ✅ Intuitive web UI |
| **Multi-language** | ✅ 7 languages | ❌ English only | ❌ English only | ❌ English only | ❌ English only |
| **Factory recovery** | ✅ Yes | ❌ No | ❌ No | ⚠️ Manual | ❌ No |

\* *With port forwarding/VPN remote access is possible, but requires advanced network configuration*

### Unique Advantages of KissOTA

#### 🌍 **Truly Global Access**
Other OTA systems require:
- Knowing the device's IP
- Being on the same LAN network, or
- Configuring port forwarding (risky), or
- Configuring VPN (complex)

**KissOTA:** Only need Telegram. Update from anywhere in the world without network configuration. As it's integrated in KissTelegram uses SSL connection.

#### 🔒 **Security without Compromises**
Exposing an HTTP web server to the internet is dangerous:
- Vulnerable to brute force attacks
- Possible web exploit vector
- Requires HTTPS for security (certificates, etc.)

**KissOTA:** Uses Telegram's secure infrastructure. Your ESP32 never exposes ports to the outside.

#### 🛡️ **Multi-level Recovery**
Other systems with rollback (ESP-IDF, AsyncElegantOTA) only have 2 partitions:
- If both partitions fail → "bricked" device
- No backup of working firmware

**KissOTA:** 3 security levels:
1. **Level 1:** Rollback from backup in LittleFS
2. **Level 2:** Boot from original factory partition
3. **Level 3:** Automatic boot loop detection

#### 💾 **Flash Space Savings**
Traditional systems (dual-bank OTA):
```
Factory:  3.5 MB  ┐
OTA_0:    3.5 MB  ├─ 7 MB minimum required
OTA_1:    3.5 MB  ┘
Total: 10.5 MB
```

**KissOTA (single-bank + backup):**
```
Factory:  3.5 MB  ┐
App:      3.5 MB  ├─ 7 MB total
Backup:   ~1.1 MB │  (in LittleFS, compressible)
Total: ~8.1 MB   ┘
```
**Gain:** ~2.4 MB of free flash for your application or data.

#### 🚀 **PSRAM as Buffer**
Other systems download directly to flash:
- **Flash wear:** Flash has limited write cycles (~100K)
- **Risk:** If download fails, flash already partially written

**KissOTA:**
- Complete download to PSRAM first
- Verifies CRC32 in PSRAM
- Only writes to flash if everything is OK
- **PSRAM has no wear:** Infinite write cycles

---

## System Architecture

### Partition Structure

```
┌─────────────────────────────────────┐
│  Factory Partition (3.5 MB)        │ ← Original factory firmware
│  - Final safety net                │
│  - Read-only in production         │
└─────────────────────────────────────┘
┌─────────────────────────────────────┐
│  App Partition (3.5 MB)             │ ← Running firmware
│  - Current active firmware          │
│  - Updated during OTA               │
└─────────────────────────────────────┘
┌─────────────────────────────────────┐
│  LittleFS (remaining ~8-9 MB)       │
│  - /ota_backup.bin (backup)         │ ← Backup of previous firmware
│  - Configuration files              │
│  - Persistent data                  │
└─────────────────────────────────────┘
┌─────────────────────────────────────┐
│  NVS (Non-Volatile Storage)         │
│  - Boot flags (validation)          │ ← OTA validation state
│  - PIN/PUK credentials              │
│  - Boot counter                     │
└─────────────────────────────────────┘
┌─────────────────────────────────────┐
│  PSRAM (7-8 MB)                     │
│  - Temporary download buffer        │ ← Firmware downloaded before flash
│  - Non-persistent (cleared)         │
└─────────────────────────────────────┘
```

### Data Flow During OTA

```
Telegram API
    │
    ▼
[Download to PSRAM] ← 7-8 MB temporary buffer
    │
    ▼
[CRC32 Verification]
    │
    ▼
[Backup current firmware → LittleFS] ← /ota_backup.bin
    │
    ▼
[Flash new firmware → App Partition]
    │
    ▼
[ESP32 Reboot]
    │
    ▼
[60-second Validation] → /otaok or automatic rollback
    │
    ▼
[Delete previous firmware] -> Recover space from previous firmware

```
---

## Advantages over Espressif's Standard OTA

| Feature | Espressif OTA | KissOTA |
|---------|---------------|---------|
| **Flash space required** | ~7 MB (OTA_0 + OTA_1) | ~3.5 MB (app only) |
| **Firmware backup** | ❌ No | ✅ Yes, in LittleFS |
| **Automatic rollback** | ⚠️ Limited | ✅ Complete + factory |
| **Manual validation** | ❌ No | ✅ Yes, 60 seconds |
| **Authentication** | ❌ No | ✅ PIN/PUK |
| **Boot loop detection** | ❌ No | ✅ Yes, automatic |
| **Telegram integration** | ❌ No | ✅ Native |
| **Download buffer** | Flash | PSRAM (no flash wear) |
| **Emergency recovery** | ⚠️ Only 2 partitions | ✅ 3 levels (app/backup/factory) |

---

## Hardware Requirements

### Minimum Required
- **MCU**: ESP32-S3 (other ESP32 variants may work with adaptations)
- **PSRAM**: Minimum depends on firmware size to replace 2-8MB PSRAM (for download buffer)
- **Flash**: Minimum depends on firmware size to replace 8-16-32MB
- **Connectivity**: Working WiFi

### Recommended Configuration
- **ESP32-S3-WROOM-1-N16R8**: 16MB Flash + 8MB PSRAM (ideal)
- **ESP32-S3-DevKitC-1**: With N16R8 module or higher

---

## Partition Configuration

Recommended `partitions.csv` file (N16R8):

```csv

# Name,   Type, SubType, Offset,  Size
nvs,      data, nvs,     0x9000,  0x4000,
otadata,  data, ota,     0xd000,  0x2000,
app0,     app,  factory, 0x10000, 0x180000,
app1,     app,  ota_0,   0x190000,0x180000,
spiffs,   data, spiffs,  0x310000,0xCF0000,
```

**Arduino IDE configuration:**
- Tools → Partition Scheme → Custom
- Point to the `partitions.csv` file

---

## OTA Process Flow

### State Diagram

```
┌─────────────┐
│  OTA_IDLE   │ ← Initial state
└──────┬──────┘
       │ /ota
       ▼
┌─────────────┐
│ WAIT_PIN    │ ← Request PIN (3 attempts)
└──────┬──────┘
       │ Correct PIN
       ▼
┌──────────────┐
│ AUTHENTICATED│ ← PIN OK, ready to receive .bin
└──────┬───────┘
       │ User sends .bin
       ▼
┌──────────────┐
│ DOWNLOADING  │ ← Download to PSRAM (real-time progress)
└──────┬───────┘
       │
       ▼
┌────────────────┐
│ VERIFY_CHECKSUM│ ← Calculate CRC32
└──────┬─────────┘
       │
       ▼
┌──────────────┐
│ WAIT_CONFIRM │ ← Wait for user's /otaconfirm
└──────┬───────┘
       │ /otaconfirm
       ▼
┌──────────────┐
│ BACKUP_CURRENT│ ← Save current firmware to LittleFS
└──────┬───────┘
       │
       ▼
┌──────────────┐
│  FLASHING    │ ← Write new firmware from PSRAM
└──────┬───────┘
       │
       ▼
┌──────────────┐
│   REBOOT     │ ← Restart ESP32
└──────┬───────┘
       │
       ▼
┌──────────────┐
│  VALIDATING  │ ← 60 seconds for /otaok
└──────┬───────┘
       │ /otaok
       ▼
┌──────────────┐
│  COMPLETE    │ ← Firmware validated ✅
└──────────────┘
       │ Timeout or /otacancel
       ▼
┌──────────────┐
│  ROLLBACK    │ ← Restore backup automatically
└──────────────┘
```

### Important Timeouts

- **PIN authentication**: Unlimited (up to 3 attempts), if fails, PIN locked and requires PUK to restore
- **.bin reception**: Wait up to 7 minutes, if exceeded cancels /ota
- **Confirmation**: Wait up to 7 minutes, if exceeded cancels /ota (waits for /otaconfirm)
- **Complete process**: 7 minutes maximum from /otaconfirm
- **Post-flash validation**: 60 seconds for /otaok

---

## Available Commands

### `/ota`
Start the OTA process.

**Usage:**
```
/ota
```

**Response:**
- If PIN not locked: Request PIN
- If PIN locked: Request PUK

---

### `/otapin <code>`
Send the PIN code (4-8 digits).

**Usage:**
```
/otapin 0000 (default)
```

**Response:**
- ✅ Correct PIN: AUTHENTICATED state, ready to receive .bin
- ❌ Incorrect PIN: Reduces remaining attempts
- 🔒 After 3 failures: Locks and requests PUK

---

### `/otapuk <code>`
Unlock the system with PUK code.

**Usage:**
```
/otapuk 12345678
```

**Response:**
- ✅ Correct PUK: Unlocks PIN and requests new PIN
- ❌ Incorrect PUK: Remains locked

---

### `/otaconfirm`
Confirm that you want to flash the downloaded firmware.

**Usage:**
```
/otaconfirm
```

**Preconditions:**
- Firmware downloaded and verified
- CRC32 checksum OK

**Action:**
- Creates backup of current firmware
- Flashes new firmware
- Restarts ESP32

---

### `/otaok`
Validate that new firmware works correctly.

**Usage:**
```
/otaok
```

**Preconditions:**
- Must be sent within 60 seconds after restart
- Only available after an OTA flash

**Action:**
- Marks firmware as valid
- Deletes previous backup
- System returns to normal operation

⚠️ **IMPORTANT**: If `/otaok` is not sent within 60 seconds, automatic rollback will be executed.

---

### `/otacancel`
Cancel the OTA process or force rollback.

**Usage:**
```
/otacancel
```

**Behavior:**
- During download/validation: Cancels and cleans temporary files
- During post-flash validation: Executes immediate rollback
- If no active OTA: Informs that there's no active process

---

## Security Features

### 1. PIN/PUK Authentication

#### PIN and PUK (Can be changed remotely with /changepin <old> <new> or /changepuk <old> <new>)

#### PIN (Personal Identification Number)
- **Length**: 4-8 digits
- **Attempts**: 3 attempts before lockout
- **Configuration**: Defined in `system_setup.h` Fallback Credentials
- **Persistence**: Securely saved in NVS

#### PUK (PIN Unlock Key)
- **Length**: 8 digits
- **Function**: Unlock after 3 PIN failures
- **Security**: Only administrator should know it

**Lockout flow example:**
```
Attempt 1: Incorrect PIN → "❌ Incorrect PIN. 2 attempts remaining"
Attempt 2: Incorrect PIN → "❌ Incorrect PIN. 1 attempt remaining"
Attempt 3: Incorrect PIN → "🔒 PIN locked. Use /otapuk [code]"
```

### 2. Integrity Verification

#### CRC32 Checksum
- **Algorithm**: CRC32 IEEE 802.3
- **Calculation**: Over entire downloaded .bin file
- **Validation**: Before allowing /otaconfirm
- **Rejection**: If CRC32 doesn't match, download is deleted

**Output example:**
```
🔍 Verifying checksum...
🔍 CRC32: 0xF8CAACF6 (1.07 MB verified)
✅ Checksum OK
```

### 3. Post-Flash Validation

#### Validation Window (60 seconds)
After flashing new firmware:
1. ESP32 restarts
2. Boot flags mark `otaInProgress = true`
3. User has 60 seconds to send `/otaok`
4. If no response → automatic rollback

**Time diagram:**
```
FLASH → RESTART → [60s for /otaok] → ROLLBACK if timeout
                         ↓
                      /otaok → ✅ Firmware validated
```

### 4. Protection against Unauthorized Modification

- **Maintenance mode**: During OTA, other commands are limited
- **Unique chat ID**: Only configured chat_id can execute OTA
- **Prior authentication**: PIN/PUK mandatory before any action

---

## Error Handling and Recovery

### Recovery Levels

#### Level 1: Automatic Retry
**Scenarios:**
- Download failure from Telegram (max 3 retries)
- Network timeout
- Interrupted download

**Action:**
- Clears PSRAM
- Automatically retries download
- Notifies user of attempt

---

#### Level 2: Rollback from Backup
**Scenarios:**
- User doesn't send `/otaok` within 60 seconds
- User sends `/otacancel` during validation
- Boot loop detected (3+ boots in 5 minutes)

**Rollback Process:**
1. Detects `bootFlags.otaInProgress == true`
2. Reads `bootFlags.backupPath` → `/ota_backup.bin`
3. Restores backup from LittleFS → App Partition
4. Restarts ESP32
5. Clears boot flags
6. Notifies user via Telegram

**Example code:**
```cpp
bool KissOTA::restoreFromBackup() {
  if (strlen(bootFlags.backupPath) == 0) {
    return false; // No backup
  }

  // Read /ota_backup.bin from LittleFS
  // Write to App Partition
  // Restart
}
```

---

#### Level 3: Factory Fallback
**Scenarios:**
- Rollback from backup fails
- `/ota_backup.bin` file corrupted
- Critical flash error

**Process:**
1. `esp_ota_set_boot_partition(factory_partition)`
2. Restarts ESP32
3. Boots from original factory firmware
4. Notifies user of critical error

⚠️ **IMPORTANT**: This is last resort. Factory firmware must be stable and never modified in production.

---

### Boot Loop Detection

**Algorithm:**
```cpp
bool KissOTA::checkBootLoop() {
  if (bootFlags.bootCount > 3) {
    unsigned long timeSinceLastBoot = millis() - bootFlags.lastBootTime;
    if (timeSinceLastBoot < 300000) {  // 5 minutes
      KISS_CRITICAL("🔥 BOOT LOOP: 3+ boots in 5 minutes");
      return true; // Execute rollback
    }
  }
  return false;
}
```

**Protection:**
- Increments `bootFlags.bootCount` on each boot
- If > 3 boots in < 5 minutes → Automatic rollback
- When validating with `/otaok`, resets counter

---

### Error States

| Error | Code | Automatic Action |
|-------|------|------------------|
| **Download failed** | `DOWNLOAD_FAILED` | Retry up to 3 times |
| **Incorrect checksum** | `CHECKSUM_MISMATCH` | Delete download, cancel OTA |
| **Backup failure** | `BACKUP_FAILED` | Cancel OTA, don't risk |
| **Flash failure** | `FLASH_FAILED` | Cancel OTA, keep current firmware |
| **Validation timeout** | `VALIDATION_TIMEOUT` | Automatic rollback |
| **Boot loop** | `BOOT_LOOP_DETECTED` | Automatic rollback |
| **Rollback failed** | `ROLLBACK_FAILED` | Fallback to factory |
| **No backup** | `NO_BACKUP` | Keep current firmware, warn |

---

## Basic Usage

### Complete Update Step by Step

#### 1. Prepare the Firmware
```bash
# Compile project in Arduino IDE
# .bin is generated in: build/esp32.esp32.xxx/suite_kiss.ino.bin
```

#### 2. Start OTA
From Telegram:
```
/ota
```

Bot response:
```
🔐 OTA AUTHENTICATION

Enter 4-8 digit PIN code:
/otapin [code]

Attempts remaining: 3
```

#### 3. Authenticate with PIN
```
/otapin 0000
```

Response:
```
✅ Correct PIN

System ready for OTA.
Send the new firmware .bin file.
```

#### 4. Send the Firmware
- Attach the `.bin` file as document (not as photo)
- Telegram uploads it and bot downloads automatically

Response during download:
```
📥 Downloading firmware to PSRAM...
⏳ Progress: 45%
```

#### 5. Automatic Verification
```
✅ Download complete: 1.07 MB in PSRAM
🔍 Verifying checksum...
✅ CRC32: 0xF8CAACF6

📋 FIRMWARE VERIFIED

File: suite_kiss.ino.bin
Size: 1.07 MB
CRC32: 0xF8CAACF6

⚠️ CONFIRMATION REQUIRED
To flash firmware:
/otaconfirm

To cancel:
/otacancel
```

#### 6. Confirm Flash
```
/otaconfirm
```

Response:
```
💾 Starting backup...
✅ Backup complete: 1123456 bytes

⚡ Flashing firmware...
✅ FLASH COMPLETED

Firmware written successfully.
Device will now restart.

After restart you have 60 seconds to validate with /otaok
If you don't validate automatic rollback will execute.
```

#### 7. Post-Restart Validation
After restart (within 60 seconds):
```
/otaok
```

Response:
```
✅ FIRMWARE VALIDATED

New firmware has been confirmed.
System returns to normal operation.

Version: 0.9.0
```

---

### Manual Rollback Example

If new firmware doesn't work well:

```
/otacancel
```

Response:
```
⚠️ EXECUTING ROLLBACK

Restoring previous firmware from backup...
✅ Previous firmware restored
🔄 Restarting...

[After restart]
✅ ROLLBACK COMPLETED

System restored to previous firmware.
```

---

## Advanced Configuration

### Customize Timeouts

In `KissOTA.h`:

```cpp
// Validation timeout (default 60 seconds)
static const int BOOT_VALIDATION_TIMEOUT = 60000;

// Global OTA process timeout (default 7 minutes)
static const unsigned long OTA_GLOBAL_TIMEOUT = 420000;
```

### Enable/Disable WDT During OTA

In `system_setup.h`:

```cpp
// Disable WDT during critical operations
#ifdef KISS_USE_RTOS
  KISS_PAUSE_WDT();  // Pause watchdog
  // ... OTA operation ...
  KISS_INIT_WDT();   // Reactivate watchdog
#endif
```

### Change PSRAM Buffer Size

In `KissOTA.cpp`:

```cpp
bool KissOTA::initPSRAMBuffer() {
  psramBufferSize = 8 * 1024 * 1024;  // 8 MB default
  // Adjust according to available PSRAM on your ESP32
}
```

---
### Change default PIN/PUK

In `system_setup.h`:

```cpp
#define KISS_FALLBACK_OTA_PIN "0000"
#define KISS_FALLBACK_OTA_PUK "00000000"
```
---

## Troubleshooting

### Error: "❌ No PSRAM available"
**Cause:** ESP32 without PSRAM or PSRAM not enabled

**Solution:**
1. Verify ESP32-S3 has physical PSRAM
2. In Arduino IDE: Tools → PSRAM → "OPI PSRAM"
3. Recompile project

---

### Error: "❌ Error creating backup"
**Cause:** LittleFS no space or not mounted

**Solution:**
1. Verify `spiffs` partition in `partitions.csv`
2. Format LittleFS if necessary
3. Increase spiffs partition size

---

### Error: "🔥 BOOT LOOP: 3+ boots in 5 minutes"
**Cause:** New firmware consistently fails

**Solution:**
- Automatic: Rollback executes itself
- Manual: Wait for automatic rollback
- Preventive: Test firmware on another ESP32 before OTA

---

### Firmware doesn't validate after /otaok
**Cause:** 60-second timeout expired

**Solution:**
- Send `/otaok` allowing more time after restart for stable connection
- Verify WiFi connectivity after restart
- Increase `BOOT_VALIDATION_TIMEOUT` if necessary

---

## Integration Code Example

### Initialization in `suite_kiss.ino`

```cpp
#include "KissOTA.h"

KissTelegram* bot;
KissCredentials* credentials;
KissOTA* ota;

void setup() {
  // Initialize credentials
  credentials = new KissCredentials();
  credentials->begin();

  // Initialize Telegram bot
  bot = new KissTelegram(BOT_TOKEN);

  // Initialize OTA
  ota = new KissOTA(bot, credentials);

  // Check if coming from interrupted OTA
  if (ota->isFirstBootAfterOTA()) {
    ota->validateBootAfterOTA();
  }
}

void loop() {
  // Process Telegram commands
  if (bot->getUpdates()) {
    for (int i = 0; i < bot->message_count; i++) {
      String command = bot->messages[i].text;

      if (command.startsWith("/ota")) {
        ota->handleOTACommand(command.c_str(), "");
      }
    }
  }

  // OTA loop (manages timeouts and states)
  ota->loop();
}
```

---

## Additional Technical Information

### Boot Flags Format (NVS)

```cpp
struct BootFlags {
  uint32_t magic;              // 0xCAFEBABE (validation)
  uint32_t bootCount;          // Boot counter
  uint32_t lastBootTime;       // Last boot timestamp
  bool otaInProgress;          // true if waiting validation
  bool firmwareValid;          // true if firmware validated
  char backupPath[64];         // Backup path (/ota_backup.bin)
};
```

### Typical Sizes

| Element | Typical Size |
|---------|--------------|
| Compiled firmware | 1.0 - 1.5 MB |
| LittleFS backup | ~1.1 MB |
| PSRAM buffer | 7-8 MB |
| Factory partition | 3.5 MB |
| App partition | 3.5 MB |

---

## Contributions and Support

**Author:** Vicente Soriano
**Email:** victek@gmail.com
**Project:** KissTelegram Suite

To report bugs or request features, contact the author.

---

## Frequently Asked Questions (FAQ)

### Why not use AsyncElegantOTA or ArduinoOTA?

**Short Answer:** KissOTA requires no network configuration and works globally from the start.

**Complete Answer:**

AsyncElegantOTA and ArduinoOTA are excellent for local development, but have limitations in production:

1. **Complicated Remote Access:**
   - Need to know device IP
   - If behind router/NAT, need port forwarding
   - Port forwarding exposes your ESP32 to internet (security risk)
   - Alternative: VPN (complex to configure for end users)

2. **Limited Security:**
   - Basic user/password (vulnerable to brute force)
   - HTTP without encryption (unless you configure HTTPS with certificates)
   - Exposed web server = large attack surface

3. **No Real Rollback:**
   - Only have 2 partitions (OTA_0 and OTA_1)
   - If both fail, "bricked" device
   - No backup of known working firmware

**KissOTA solves this:**
- ✅ Global update without configuring anything (Telegram API)
- ✅ Robust security (PIN/PUK + Telegram infrastructure)
- ✅ Multi-level rollback (backup + factory + boot loop detection)
- ✅ Doesn't expose ports to outside

**When to use each?**
- **AsyncElegantOTA:** Local development, rapid prototyping, private LAN
- **KissOTA:** Production, global remote devices, critical security

---

### Does it work without PSRAM?

**Answer:** Current version of KissOTA **requires PSRAM** for download buffer and file validity verification.

**Technical reasons:**
- Typical firmware: 1-1.5 MB
- PSRAM buffer: 7-8 MB available
- Normal ESP32-S3 RAM: Only ~70-105 KB free

**Alternatives if you don't have PSRAM:**
1. **Download to LittleFS first:**
   - Slower (flash is slower than PSRAM)
   - Unnecessarily wears flash
   - Requires free space in LittleFS

2. **Use AsyncElegantOTA:**
   - Doesn't require PSRAM
   - Downloads directly to OTA partition

3. **Contribute a PR:**
   - If you implement support for ESP32 without PSRAM, PRs welcome

**Recommended hardware:**
- ESP32-S3-WROOM-1-N16R8 (16MB Flash + 8MB PSRAM) - ~3-6 euros
- ESP32-S3-DevKitC-1 with N16R8 module

---

### Can I use KissOTA without Telegram?

**Answer:** Technically yes, but requires adaptation work.

KissOTA architecture has two layers:

1. **Core OTA (backend):**
   - State management
   - Backup/rollback
   - Flash from PSRAM
   - CRC32 validation
   - Boot flags
   - **This part is transport-independent**

2. **Telegram Frontend:**
   - PIN/PUK authentication
   - File download from Telegram API
   - Progress messages to user
   - **This part depends on KissTelegram**

**To use another transport (HTTP, MQTT, Serial, etc.):**

```cpp
// You would need to implement your own frontend
class KissOTACustom : public KissOTACore {
public:
  // Implement abstract methods:
  virtual bool downloadFirmware(const char* source) override;
  virtual void sendProgress(int percentage) override;
  virtual void sendMessage(const char* text) override;
};
```

**Is the effort worth it?**
- If you already have KissTelegram: **No, use KissOTA as is**
- If you don't want Telegram: **Probably better to use AsyncElegantOTA**
- If you have a specific use case (eg: industrial MQTT): **KissTelegram has an Enterprise version, ask**

---

### What happens if I lose WiFi connection during download?

**Answer:** System handles disconnections safely.

**Scenarios:**

**1. Interrupted PSRAM download:**
```
State: DOWNLOADING → Network timeout
Automatic action:
  1. Detects timeout (after 30 seconds without data)
  2. Clears PSRAM buffer
  3. Retries download (maximum 3 attempts)
  4. If 3 failures: Cancels OTA, returns to IDLE
  5. Current firmware NOT touched (safe)
```

**2. Disconnection during flash:**
```
State: FLASHING → WiFi drops
Result:
  - Flash continues (doesn't depend on WiFi)
  - Data already in PSRAM
  - Flash completes normally
  - After restart, will wait for /otaok (60s)
  - If no WiFi to send /otaok: Automatic rollback
```

**3. Disconnection during validation:**
```
State: VALIDATING → No WiFi
Result:
  - 60-second timer running
  - If WiFi returns before 60s: Can send /otaok
  - If 60s pass without /otaok: Automatic rollback
  - System returns to previous firmware (safe)
```

**Global timeout:** 7 minutes from /otaconfirm to complete everything. If exceeded, OTA cancels automatically.

---

### Is it safe to flash firmware smaller than current?

**Answer:** Yes, completely safe.

**Technical explanation:**

Flash process always:
1. **Completely erases destination partition** before writing
2. **Writes new firmware** (whatever size)
3. **Marks actual size** in partition metadata

**Example:**
```
Current firmware:  1.5 MB
New firmware:      0.9 MB

Process:
1. Backup 1.5 MB → LittleFS
2. Erase app partition (full 3.5 MB)
3. Write new 0.9 MB
4. Metadata: size = 0.9 MB
5. Remaining space in partition: Empty/erased
```

**No "residual garbage" problem** because:
- ESP32 only executes up to marked firmware end
- Rest of partition is erased
- CRC32 calculated only over actual size

**Common use cases:**
- Deactivate features to reduce size
- Minimalist emergency firmware (~500 KB)
- Optimized compilation with different flags

---

### How do I reset the system if everything fails?

**Answer:** You have 3 recovery options.

#### Option 1: Automatic Rollback (Most Common)
If new firmware fails, simply **do nothing**:
```
1. New firmware boots but fails
2. ESP32 restarts (crash/watchdog)
3. bootCount++ counter in NVS
4. After 3 restarts in 5 minutes → Automatic rollback
5. System restores previous firmware from /ota_backup.bin
```

#### Option 2: Manual Rollback
If you have Telegram access:
```
/otacancel
```
Forces immediate rollback from backup.

#### Option 3: Factory Boot (Emergency)
If everything else fails:

**Via Serial (requires physical access):**
```cpp
// In setup(), detect emergency condition
if (digitalRead(EMERGENCY_PIN) == LOW) {  // Pin to GND
  esp_ota_set_boot_partition(esp_partition_find_first(
    ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_FACTORY, NULL
  ));
  ESP.restart();
}
```

**Via esptool (last resort):**
```bash
# Flash factory partition with original firmware
esptool.py --port COM13 write_flash 0x10000 firmware_factory.bin
```

#### Option 4: Complete Flash (Clean Slate)
```bash
# Erase entire flash
esptool.py --port COM13 erase_flash

# Flash fresh firmware + partitions
esptool.py --port COM13 write_flash 0x0 bootloader.bin 0x8000 partitions.bin 0x10000 firmware.bin
```

**Prevention:**
- ✅ Always keep stable factory firmware
- ✅ Test new firmware on development device first
- ✅ Don't modify factory partition in production

---

### How long does a complete OTA update take?

**Answer:** Approximately 1-3 minutes total for ~1 MB firmware if WiFi is perfect.

**Typical time breakdown:**

| Phase | Typical Duration | Factors |
|-------|-----------------|---------|
| **Authentication** | 10-30 seconds | User time typing PIN |
| **Upload to Telegram** | 5-15 seconds | User's internet speed |
| **Download to PSRAM** | 10-20 seconds | ESP32 WiFi speed + Telegram API |
| **CRC32 Verification** | 2-5 seconds | Firmware size |
| **User confirmation** | 5-30 seconds | User reaction time |
| **Backup current firmware** | 10-30 seconds | LittleFS write speed |
| **Flash new firmware** | 5-10 seconds | Flash write speed |
| **Restart** | 5-10 seconds | ESP32 boot time |
| **Validation /otaok** | 5-60 seconds | User reaction time |
| **TOTAL** | **~2-4 minutes** | Varies by conditions |

**Factors affecting speed:**

**Faster:**
- ✅ Strong WiFi (near router)
- ✅ Small firmware (<1 MB)
- ✅ Experienced user (quick response)
- ✅ Non-fragmented LittleFS. KissTelegram defrags FS every 3-5 minutes

**Slower:**
- ⚠️ Weak or congested WiFi
- ⚠️ Large firmware (>1.5 MB)
- ⚠️ User hesitating to confirm
- ⚠️ Very full LittleFS

**Safety timeout:** 7 minutes maximum from /otaconfirm. If exceeded, OTA cancels.

---

### Can I update multiple ESP32 simultaneously?

**Answer:** Yes, but each ESP32 needs its own Telegram bot (different BOT_TOKEN).

**Telegram limitation:**
- One Telegram bot can only reliably process 1 simultaneous conversation
- Telegram API has rate limits (~30 messages/second per bot)

**Option 1: One Bot per Device (Recommended for Production)**
```cpp
// ESP32 #1
#define BOT_TOKEN_1 "123456:ABC-DEF..."
KissTelegram bot1(BOT_TOKEN_1);

// ESP32 #2
#define BOT_TOKEN_2 "789012:GHI-JKL..."
KissTelegram bot2(BOT_TOKEN_2);
```

**Advantages:**
- ✅ Completely independent OTAs
- ✅ No message conflicts
- ✅ Each device has its own chat

**Disadvantages:**
- ⚠️ More bots to manage
- ⚠️ More tokens to configure

---

**Option 2: One Bot, Multiple Devices Sequentially**
```cpp
// All use same BOT_TOKEN
// But update ONE AT A TIME manually
```

**Process:**
1. Update ESP32 #1 → Wait to complete
2. Update ESP32 #2 → Wait to complete
3. etc.

**Advantages:**
- ✅ Only one bot to manage
- ✅ Simpler for few devices

**Disadvantages:**
- ⚠️ One at a time (slow for many devices)
- ⚠️ Easy to confuse devices

---

**Option 3: Centralized Management (KissTelegram for Enterprise)**
```
Central system with own API
    ↓
Distributes firmware to multiple ESP32
    ↓
Each ESP32 reports progress to central system
    ↓
Central system notifies via Json API or user via Telegram
```

**Features liked (But available in Kisstelegram for enterprise):**
- Web/cloud backend
- Device database
- OTA job queue
- Monitoring dashboard

**Contributions welcome** KissOTA has many possibilities, ask or make a PR.

---

## Changelog

### v0.9.0 (Current)
- ✅ Simplified version system
- ✅ Removed downgrade verification
- ✅ Cleanup of obsolete code
- ✅ Improvements in log messages

### v0.1.0
- ✅ Implementation of automatic backup/rollback
- ✅ Boot loop detection
- ✅ Complete Telegram integration

### v0.0.2
- ✅ Complete refactoring to PSRAM
- ✅ Removal of OTA_0/OTA_1 partitions
- ✅ 60-second validation system

### v0.0.1
- ✅ Initial version with basic OTA

---

**Document updated:** 12/12/2025
**Document version:** 0.9.0
**Author:** Vicente Soriano, victek@gmail.com
**MIT License**
