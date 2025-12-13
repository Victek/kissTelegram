# Getting Started with KissTelegram on ESP32-S3

**A comprehensive guide to set up your ESP32-S3 from zero to first Telegram message**

> ⚠️ **CRITICAL**: Read this guide completely before uploading any firmware. The ESP32-S3 N16R8 requires a **two-step upload process** due to custom partitions. Skipping steps will cause errors!

---

## Table of Contents

1. [Before You Begin](#before-you-begin)
2. [Creating Your Telegram Bot](#creating-your-telegram-bot)
3. [Hardware Setup](#hardware-setup)
4. [Arduino IDE Configuration](#arduino-ide-configuration)
5. [First Upload (Common Issues)](#first-upload-common-issues)
6. [Configuration Files](#configuration-files)
7. [Success! What's Next?](#success-whats-next)

---

## Before You Begin

### What You Need

- **ESP32-S3 N16R8** (16MB Flash / 8MB PSRAM)
- **Two USB-C cables** (for alternating between bootloader and OTG ports)
- **Arduino IDE 2.x** or newer
- **Windows PC** (this guide is Windows-focused, adapt paths for Linux/Mac)
- **Telegram account** on your phone

### What Makes This Different

Your new ESP32-S3 N16R8 arrives with a demo RGB LED program. KissTelegram will **completely replace the partition table** to maximize your 16MB flash:

| Partition | Espressif Default | KissTelegram Custom |
|-----------|-------------------|---------------------|
| App Space | 1.5 MB | 4.5 MB (3x larger!) |
| File System | 5 MB | 13 MB (2.6x larger!) |
| Total Used | 6.5 MB | 17.5 MB |

This is why the two-upload process is required: **the partition table changes between uploads**.

---

## Creating Your Telegram Bot

### Step 1: Talk to BotFather

1. Open Telegram on your phone
2. Search for `@BotFather` (official bot, has blue verification checkmark)
3. Start a conversation with `/start`
4. Create your bot with `/newbot`
5. Choose a name (example: "My Home Assistant")
6. Choose a username (must end in `bot`, example: "myhome_assistant_bot")
7. **Save your Bot Token** - looks like: `1234567890:ABCdefGHIjklMNOpqrsTUVwxyz`

### Step 2: Get Your Chat ID

**Method 1: Using a Bot (Easiest)**

1. Search for `@userinfobot` in Telegram
2. Start conversation with `/start`
3. It will reply with your **Chat ID** (a number like `123456789`)
4. **Save this number** - you'll need it in configuration

**Method 2: Using Web Browser**

1. Send any message to your newly created bot
2. Open browser and visit:
   ```
   https://api.telegram.org/bot<YOUR_BOT_TOKEN>/getUpdates
   ```
   (Replace `<YOUR_BOT_TOKEN>` with your actual token)
3. Look for `"chat":{"id":123456789` in the JSON response
4. That number is your **Chat ID**

**✅ Now you have:**
- Bot Token: `1234567890:ABCdefGHIjklMNOpqrsTUVwxyz`
- Chat ID: `123456789`

Keep these safe! You'll need them soon.

---

## Hardware Setup

### Understanding the Two USB-C Ports

Your ESP32-S3 N16R8 has **two USB-C ports**:

```
┌─────────────────────┐
│  ┌─┐         ESP32  │
│  │•│  ← Power LED    │
│  └─┘                 │
│  [USB-C]  ← RIGHT PORT (Bootloader/Upload)
│                      │
│                      │
│  [USB-C]  ← LEFT PORT (OTG/Normal Operation)
│                      │
└─────────────────────┘
```

**RIGHT PORT (near Power LED):**
- Used for **initial firmware upload**
- Used for **bootloader mode**
- Use this when Arduino IDE says "Connecting..."

**LEFT PORT (OTG):**
- Used for **normal operation** after first upload
- Used for **second upload** (partition fix)
- Use this for Serial Monitor in normal operation

---

## Arduino IDE Configuration

### Step 1: Show Hidden Files (Windows)

1. Open **File Explorer**
2. Click **View** tab → **Show** → Check:
   - ✅ File name extensions
   - ✅ Hidden items
3. In **Filter** tab: **All file types**

### Step 2: Modify boards.txt

1. Navigate to:
   ```
   C:\Users\<YOUR_USERNAME>\AppData\Local\Arduino15\packages\esp32\hardware\esp32\3.3.4\
   ```
   (Replace `3.3.4` with your ESP32 core version if different)

2. Find and open `boards.txt` (use Notepad++ or any text editor)

3. Press `Ctrl+F` and search for:
   ```
   gen4esp32_4MBapp_4MBota_7MBspiffs
   ```

4. **Immediately below that line**, paste these three lines:
   ```
   gen4-ESP32-S3R8n16.menu.PartitionScheme.gen4esp32scheme2=Custom (4MB APP/12MB LtlFS)
   gen4-ESP32-S3R8n16.menu.PartitionScheme.gen4esp32scheme2.build.custom_partitions=partitions
   gen4-ESP32-S3R8n16.menu.PartitionScheme.gen4esp32scheme2.upload.maximum_size=4718592
   ```

5. **Save** and close `boards.txt`

6. If Arduino IDE was open, **close and restart it**

### Step 3: Configure Arduino IDE

1. **Open** your KissTelegram sketch folder (with `.ino`, `.h`, `.cpp`, and `partitions.csv`)

2. In Arduino IDE, go to **Tools** → **Board** → **4D Systems gen4-ESP32-S3R8n16**

3. **Tools** → **Reload Board Data** (you'll see confirmation at bottom of screen)

4. **Configure all Tools menu options:**

   | Setting | Value |
   |---------|-------|
   | **Board** | 4D Systems gen4-ESP32-S3R8n16 |
   | **USB CDC On Boot** | Enabled |
   | **Flash Size** | 16MB (128Mb) |
   | **Partition Scheme** | **Custom (4MB APP/12MB LtlFS)** ⚠️ |
   | **PSRAM** | OPI PSRAM |
   | **Upload Speed** | 921600 |
   | **Erase All Flash Before Sketch Upload** | **Enabled** ⚠️ |

   ⚠️ **Critical settings** - double-check these!

5. **Tools** → **Serial Monitor** → Set baud rate to **115200**

---

## First Upload (Common Issues)

### Why Two Uploads Are Needed

**The Problem:**
- First upload: Arduino uses **old partition table** to write firmware
- ESP32 boots: Finds **new partition table** (from `partitions.csv`)
- **Mismatch** between where firmware was written vs where ESP32 looks for it
- Result: Boot errors, partition errors, crashes

**The Solution:**
Two uploads ensure firmware is written to the **correct location** defined by the new partition table.

---

### Upload #1: Initial Flash (Expect Errors!)

1. **Connect RIGHT USB-C port** (near Power LED) to your PC

2. **Select the port**: Tools → Port → Select the COM port that appears

3. **Verify settings**:
   - ✅ Erase All Flash Before Sketch Upload: **Enabled**
   - ✅ Partition Scheme: **Custom (4MB APP/12MB LtlFS)**

4. **Press Upload** (`Ctrl+U` or ➡️ button)

5. **Wait ~2-3 minutes** (erasing + uploading)

6. **Expected result**:
   ```
   ✅ Upload successful
   ```

7. **Open Serial Monitor** - you'll see errors like:
   ```
   ❌ E (123) boot: No factory partition found
   ❌ E (456) esp_image: Image length 12345 doesn't fit in partition length 67890
   ❌ E (789) boot: Failed to verify app partition
   Guru Meditation Error: Core 0 panic'ed (LoadProhibited)
   ```

8. **If you configured `system_setup.h` correctly**: You may receive the **first Telegram message** (but Serial will show errors)

**Don't panic!** These errors are **expected** and **normal**. Continue to Upload #2.

---

### Upload #2: Partition Fix (Errors Disappear)

1. **Disconnect RIGHT USB-C port**

2. **Connect LEFT USB-C port** (OTG port) to your PC

3. **Select the new port**: Tools → Port → Select the new COM port
   - **Important**: The port number will change! Look in Serial Monitor for data to confirm correct port.

4. **Verify settings again**:
   - ✅ Erase All Flash Before Sketch Upload: **Enabled**
   - ✅ Partition Scheme: **Custom (4MB APP/12MB LtlFS)**

5. **Press Upload again** (`Ctrl+U`)

6. **Wait ~2-3 minutes** (erasing + uploading)

7. **Open Serial Monitor** - you should now see:
   ```
   ✅ KissTelegram v1.x.x
   ✅ WiFi connected
   ✅ Telegram bot enabled
   ✅ System ready
   ```

8. **Check Telegram** - you'll receive the welcome message:
   ```
   📦 Hello! KissTelegram is ready.
   🔌 Build: 2025-12-12 10:30:45 (0xABCD1234)
   📡 WiFi Signal: -59 dBm (Good)
   ✅ 0 messages queued
   ```

**Success!** Your ESP32-S3 is now running KissTelegram with correct partitions.

---

### Future Uploads

**Good news:** After the two initial uploads, all future uploads work normally:

- Use **LEFT USB-C port** (OTG)
- **No need** to "Erase All Flash" anymore (unless you want a clean slate)
- Upload once and it works immediately

---

## Configuration Files

### system_setup.h (Required Before First Upload!)

**Before compiling:**

1. Navigate to your KissTelegram folder
2. Find `system_setup_template.h`
3. **Rename** it to `system_setup.h`
4. **Open** `system_setup.h` and fill in:

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

5. **Save** the file

**⚠️ Security Warning:** Change the default PIN (`0000`) and PUK (`00000000`) to your own secrets!

---

### lang.h (Optional: Choose Your Language)

KissTelegram supports 7 languages for system messages:

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

Choose your language **before compiling** for localized messages.

---

## Success! What's Next?

### Verify Everything Works

1. **Send `/estado` to your bot** in Telegram - you'll get a detailed status report:
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

2. **Check Serial Monitor** - should show no errors

3. **Test commands**:
   - `/start` - Welcome message
   - `/help` - Available commands
   - `/estado` - System status (health check)

---

### Understanding OTA Updates

Once KissTelegram is running, you can update firmware **via Telegram** (no USB cable!):

1. Send `/ota` to your bot
2. Enter PIN: `/otapin 0000` (or your custom PIN)
3. **Send your `.bin` firmware file** (drag & drop in Telegram)
4. Bot verifies checksum automatically
5. Confirm: `/otaconfirm`
6. ESP32 reboots with new firmware
7. **Within 60 seconds**, send `/otaok` to confirm it works
8. If you don't confirm, ESP32 **automatically rolls back** to previous firmware!

📖 **Read more:** See `README_KissOTA_EN.md` (or your language) for complete OTA documentation.

---

### Explore the Example Code

The `suite_kiss.ino` example demonstrates:

- ✅ WiFi management with quality monitoring
- ✅ Message queue with priorities
- ✅ Power management modes
- ✅ Command handling (`/start`, `/help`, `/estado`, etc.)
- ✅ OTA updates via Telegram
- ✅ Crash recovery and persistence
- ✅ SSL/TLS secure connections

**Pro tip:** Use `/estado` command as your **health monitoring tool** - it's your window into KissTelegram's internals!

---

### Common Troubleshooting

**Problem: "Port not found" or "Access denied"**
- Windows blocked the port. Unplug USB, wait 5s, plug back in.
- Try a different USB cable (some are charge-only, not data)

**Problem: "Timeout waiting for device" during upload**
- Wrong USB port! Remember: RIGHT port for first upload, LEFT port for second
- Hold BOOT button on ESP32 while clicking Upload, release after "Connecting..." appears

**Problem: Serial Monitor shows garbage characters**
- Wrong baud rate. Set to **115200** in Serial Monitor dropdown

**Problem: Bot doesn't respond in Telegram**
- Check `system_setup.h` has correct Bot Token and Chat ID
- Check WiFi credentials are correct
- Open Serial Monitor and look for WiFi connection messages

**Problem: "Partition table does not fit" compile error**
- You didn't add the custom partition to `boards.txt` correctly
- Or didn't select "Custom (4MB APP/12MB LtlFS)" in Tools → Partition Scheme

---

### Get More Help

- 📧 **Email**: victek@gmail.com
- 📖 **Documentation**: See all `README_*.md` files in your KissTelegram folder
- 🐛 **Bug Reports**: GitHub issues (link in main README.md)
- 💡 **Feature Requests**: Also welcome via email or GitHub!

---

## Summary: The Complete Process

```
1. Get Bot Token + Chat ID from Telegram ✅
2. Modify boards.txt (add custom partition) ✅
3. Configure Arduino IDE (Custom partition, Erase enabled) ✅
4. Edit system_setup.h (credentials) ✅
5. Connect RIGHT USB port ✅
6. Upload #1 (expect errors) ✅
7. Disconnect RIGHT, connect LEFT USB port ✅
8. Upload #2 (errors gone!) ✅
9. Receive welcome message in Telegram ✅
10. Send /estado to verify everything works ✅
```

**You're ready to build amazing projects with KissTelegram!** 🎉

---

**Happy coding!**

*Vicente Soriano - victek@gmail.com*
