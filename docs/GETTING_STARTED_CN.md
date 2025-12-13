# ESP32-S3 上的 KissTelegram 入门指南

**从零开始到发送第一条 Telegram 消息的完整指南**

> ⚠️ **关键提示**：上传任何固件前，请完整阅读本指南。ESP32-S3 N16R8 由于自定义分区需要**两步上传流程**。跳过步骤会导致错误！

---

## 目录

1. [开始前的准备](#开始前的准备)
2. [创建 Telegram 机器人](#创建-telegram-机器人)
3. [硬件设置](#硬件设置)
4. [Arduino IDE 配置](#arduino-ide-配置)
5. [首次上传（常见问题）](#首次上传常见问题)
6. [配置文件](#配置文件)
7. [成功！接下来呢？](#成功接下来呢)

---

## 开始前的准备

### 你需要的工具

- **ESP32-S3 N16R8**（16MB Flash / 8MB PSRAM）
- **两条 USB-C 数据线**（用于在 bootloader 和 OTG 端口之间切换）
- **Arduino IDE 2.x** 或更新版本
- **Windows PC**（本指南针对 Windows，Linux/Mac 请调整路径）
- **Telegram 账号**（需在手机上）

### 这有什么特别的

你新购买的 ESP32-S3 N16R8 预装了演示 RGB LED 程序。KissTelegram 将**完全替换分区表**以最大化你的 16MB Flash：

| 分区 | Espressif 默认 | KissTelegram 自定义 |
|-----------|-------------------|---------------------|
| 应用空间 | 1.5 MB | 4.5 MB（大 3 倍！） |
| 文件系统 | 5 MB | 13 MB（大 2.6 倍！） |
| 总占用 | 6.5 MB | 17.5 MB |

这就是为什么需要两步上传过程的原因：**上传之间分区表会改变**。

---

## 创建 Telegram 机器人

### 第 1 步：与 BotFather 沟通

1. 在手机上打开 Telegram
2. 搜索 `@BotFather`（官方机器人，有蓝色验证标记）
3. 用 `/start` 开始对话
4. 用 `/newbot` 创建机器人
5. 选择一个名字（例如："My Home Assistant"）
6. 选择一个用户名（必须以 `bot` 结尾，例如："myhome_assistant_bot"）
7. **保存你的机器人令牌** - 格式如：`1234567890:ABCdefGHIjklMNOpqrsTUVwxyz`

### 第 2 步：获取你的 Chat ID

**方法 1：使用机器人（最简单）**

1. 在 Telegram 中搜索 `@userinfobot`
2. 用 `/start` 开始对话
3. 它会回复你的 **Chat ID**（一个数字如 `123456789`）
4. **保存这个数字** - 你配置时会用到

**方法 2：使用网络浏览器**

1. 向你新建的机器人发送任何消息
2. 打开浏览器访问：
   ```
   https://api.telegram.org/bot<YOUR_BOT_TOKEN>/getUpdates
   ```
   （将 `<YOUR_BOT_TOKEN>` 替换为你的实际令牌）
3. 在 JSON 响应中查找 `"chat":{"id":123456789`
4. 那个数字就是你的 **Chat ID**

**✅ 现在你有了：**
- 机器人令牌：`1234567890:ABCdefGHIjklMNOpqrsTUVwxyz`
- Chat ID：`123456789`

妥善保管！你很快会需要它们。

---

## 硬件设置

### 理解两个 USB-C 端口

你的 ESP32-S3 N16R8 有**两个 USB-C 端口**：

```
┌─────────────────────┐
│  ┌─┐         ESP32  │
│  │•│  ← 电源 LED     │
│  └─┘                 │
│  [USB-C]  ← 右端口（Bootloader/上传）│
│                      │
│                      │
│  [USB-C]  ← 左端口（OTG/正常操作）│
│                      │
└─────────────────────┘
```

**右端口（靠近电源 LED）：**
- 用于**初始固件上传**
- 用于 **bootloader 模式**
- Arduino IDE 显示"Connecting..."时使用

**左端口（OTG）：**
- 用于首次上传后的**正常操作**
- 用于**第二次上传**（分区修复）
- 正常操作时用于串口监视器

---

## Arduino IDE 配置

### 第 1 步：显示隐藏文件（Windows）

1. 打开**文件管理器**
2. 点击**查看**选项卡 → **显示** → 勾选：
   - ✅ 文件扩展名
   - ✅ 隐藏的项目
3. 在**筛选器**选项卡中：**所有文件类型**

### 第 2 步：修改 boards.txt

1. 导航到：
   ```
   C:\Users\<YOUR_USERNAME>\AppData\Local\Arduino15\packages\esp32\hardware\esp32\3.3.4\
   ```
   （如果 ESP32 核心版本不同，请将 `3.3.4` 替换为你的版本）

2. 找到并打开 `boards.txt`（使用 Notepad++ 或任何文本编辑器）

3. 按 `Ctrl+F` 搜索：
   ```
   gen4esp32_4MBapp_4MBota_7MBspiffs
   ```

4. **在那行下面立即**粘贴这三行：
   ```
   gen4-ESP32-S3R8n16.menu.PartitionScheme.gen4esp32scheme2=Custom (4MB APP/12MB LtlFS)
   gen4-ESP32-S3R8n16.menu.PartitionScheme.gen4esp32scheme2.build.custom_partitions=partitions
   gen4-ESP32-S3R8n16.menu.PartitionScheme.gen4esp32scheme2.upload.maximum_size=4718592
   ```

5. **保存**并关闭 `boards.txt`

6. 如果 Arduino IDE 打开着，**关闭并重启它**

### 第 3 步：配置 Arduino IDE

1. **打开**你的 KissTelegram 草图文件夹（包含 `.ino`、`.h`、`.cpp` 和 `partitions.csv`）

2. 在 Arduino IDE 中，转到 **Tools** → **Board** → **4D Systems gen4-ESP32-S3R8n16**

3. **Tools** → **Reload Board Data**（你会在屏幕底部看到确认）

4. **配置所有 Tools 菜单选项：**

   | 设置 | 值 |
   |---------|-------|
   | **Board** | 4D Systems gen4-ESP32-S3R8n16 |
   | **USB CDC On Boot** | Enabled |
   | **Flash Size** | 16MB (128Mb) |
   | **Partition Scheme** | **Custom (4MB APP/12MB LtlFS)** ⚠️ |
   | **PSRAM** | OPI PSRAM |
   | **Upload Speed** | 921600 |
   | **Erase All Flash Before Sketch Upload** | **Enabled** ⚠️ |

   ⚠️ **关键设置** - 请双检查！

5. **Tools** → **Serial Monitor** → 将波特率设为 **115200**

---

## 首次上传（常见问题）

### 为什么需要两次上传

**问题：**
- 首次上传：Arduino 使用**旧分区表**写入固件
- ESP32 启动：找到**新分区表**（来自 `partitions.csv`）
- **不匹配** - 固件写入位置 vs ESP32 查找位置
- 结果：启动错误、分区错误、崩溃

**解决方案：**
两次上传确保固件被写入新分区表定义的**正确位置**。

---

### 上传 #1：初始刷写（预期会出错！）

1. **连接右 USB-C 端口**（靠近电源 LED）到电脑

2. **选择端口**：Tools → Port → 选择显示的 COM 端口

3. **验证设置**：
   - ✅ Erase All Flash Before Sketch Upload：**Enabled**
   - ✅ Partition Scheme：**Custom (4MB APP/12MB LtlFS)**

4. **按上传**（`Ctrl+U` 或 ➡️ 按钮）

5. **等待约 2-3 分钟**（擦除 + 上传）

6. **预期结果**：
   ```
   ✅ Upload successful
   ```

7. **打开串口监视器** - 你会看到如下错误：
   ```
   ❌ E (123) boot: No factory partition found
   ❌ E (456) esp_image: Image length 12345 doesn't fit in partition length 67890
   ❌ E (789) boot: Failed to verify app partition
   Guru Meditation Error: Core 0 panic'ed (LoadProhibited)
   ```

8. **如果你正确配置了 `system_setup.h`**：你可能会收到**第一条 Telegram 消息**（但串口仍会显示错误）

**不用惊慌！** 这些错误是**预期的**且**正常的**。继续上传 #2。

---

### 上传 #2：分区修复（错误消失）

1. **断开右 USB-C 端口**

2. **连接左 USB-C 端口**（OTG 端口）到电脑

3. **选择新端口**：Tools → Port → 选择新的 COM 端口
   - **重要**：端口号会改变！在串口监视器中查找数据以确认正确端口。

4. **再次验证设置**：
   - ✅ Erase All Flash Before Sketch Upload：**Enabled**
   - ✅ Partition Scheme：**Custom (4MB APP/12MB LtlFS)**

5. **再次按上传**（`Ctrl+U`）

6. **等待约 2-3 分钟**（擦除 + 上传）

7. **打开串口监视器** - 你现在应该看到：
   ```
   ✅ KissTelegram v1.x.x
   ✅ WiFi connected
   ✅ Telegram bot enabled
   ✅ System ready
   ```

8. **检查 Telegram** - 你会收到欢迎消息：
   ```
   📦 Hello! KissTelegram is ready.
   🔌 Build: 2025-12-12 10:30:45 (0xABCD1234)
   📡 WiFi Signal: -59 dBm (Good)
   ✅ 0 messages queued
   ```

**成功！** 你的 ESP32-S3 现在运行 KissTelegram 且分区正确。

---

### 后续上传

**好消息：** 初始两次上传之后，所有后续上传都正常工作：

- 使用**左 USB-C 端口**（OTG）
- **无需**"Erase All Flash"（除非你想清空）
- 上传一次即可立即工作

---

## 配置文件

### system_setup.h（首次上传前必需！）

**编译前：**

1. 导航到你的 KissTelegram 文件夹
2. 找到 `system_setup_template.h`
3. **重命名**为 `system_setup.h`
4. **打开** `system_setup.h` 并填入：

```cpp
// 你的 Telegram 机器人（来自 BotFather）
#define KISS_FALLBACK_BOT_TOKEN "1234567890:ABCdefGHIjklMNOpqrsTUVwxyz"

// 你的 Chat ID（来自 @userinfobot）
#define KISS_FALLBACK_CHAT_ID "123456789"

// 你的 WiFi 凭证
#define KISS_FALLBACK_WIFI_SSID "YourWiFiName"
#define KISS_FALLBACK_WIFI_PASSWORD "YourWiFiPassword"

// OTA 安全（改变默认 PIN/PUK！）
#define KISS_FALLBACK_OTA_PIN "0000"        // 4 位数
#define KISS_FALLBACK_OTA_PUK "00000000"    // 8 位数
```

5. **保存**文件

**⚠️ 安全警告：** 改变默认 PIN（`0000`）和 PUK（`00000000`）为你自己的密钥！

---

### lang.h（可选：选择你的语言）

KissTelegram 支持 7 种系统消息语言：

```cpp
// 在 lang.h 中，取消注释一种语言：

// #define LANG_CN  // 中文 (Chinese)
// #define LANG_DE  // Deutsch (German)
// #define LANG_EN  // English
// #define LANG_FR  // Français (French)
// #define LANG_IT  // Italiano (Italian)
// #define LANG_PT  // Português (Portuguese)
// #define LANG_ES  // Español (Spanish) - 如果全部注释则为默认
```

编译**前**选择你的语言以获得本地化消息。

---

## 成功！接下来呢？

### 验证一切工作正常

1. **在 Telegram 中向机器人发送 `/estado`** - 你会收到详细的状态报告：
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

2. **检查串口监视器** - 应该没有错误

3. **测试命令**：
   - `/start` - 欢迎消息
   - `/help` - 可用命令
   - `/estado` - 系统状态（健康检查）

---

### 理解 OTA 更新

KissTelegram 运行后，你可以**通过 Telegram 更新固件**（无需 USB 线！）：

1. 向机器人发送 `/ota`
2. 输入 PIN：`/otapin 0000`（或你的自定义 PIN）
3. **发送你的 `.bin` 固件文件**（在 Telegram 中拖放）
4. 机器人自动验证校验和
5. 确认：`/otaconfirm`
6. ESP32 用新固件重启
7. **在 60 秒内**，发送 `/otaok` 确认它工作
8. 如果你不确认，ESP32 **自动回滚**到之前的固件！

📖 **了解更多：** 见 `README_KissOTA_EN.md`（或你的语言）获取完整 OTA 文档。

---

### 探索示例代码

`suite_kiss.ino` 示例演示了：

- ✅ WiFi 管理与质量监控
- ✅ 优先级消息队列
- ✅ 电源管理模式
- ✅ 命令处理（`/start`、`/help`、`/estado` 等）
- ✅ 通过 Telegram 的 OTA 更新
- ✅ 崩溃恢复和持久化
- ✅ SSL/TLS 安全连接

**专业提示：** 使用 `/estado` 命令作为你的**健康监控工具** - 它是你了解 KissTelegram 内部的窗口！

---

### 常见故障排除

**问题："Port not found"或"Access denied"**
- Windows 阻止了端口。拔掉 USB，等待 5 秒，再插回。
- 尝试另一条 USB 线（有些只能充电，不能传数据）

**问题：上传时"Timeout waiting for device"**
- 错误的 USB 端口！记住：首次上传用右端口，第二次用左端口
- 点击上传时按住 BOOT 按钮，"Connecting..."出现后释放

**问题：串口监视器显示乱码**
- 错误的波特率。在串口监视器下拉菜单中设为 **115200**

**问题：机器人在 Telegram 中无响应**
- 检查 `system_setup.h` 有正确的机器人令牌和 Chat ID
- 检查 WiFi 凭证是否正确
- 打开串口监视器查看 WiFi 连接消息

**问题："Partition table does not fit"编译错误**
- 你没有正确在 `boards.txt` 中添加自定义分区
- 或者没有在 Tools → Partition Scheme 中选择"Custom (4MB APP/12MB LtlFS)"

---

### 获取更多帮助

- 📧 **邮箱**：victek@gmail.com
- 📖 **文档**：见你 KissTelegram 文件夹中的所有 `README_*.md` 文件
- 🐛 **Bug 报告**：GitHub issues（链接在主 README.md 中）
- 💡 **功能请求**：欢迎通过邮箱或 GitHub！

---

## 总结：完整过程

```
1. 从 Telegram 获取机器人令牌 + Chat ID ✅
2. 修改 boards.txt（添加自定义分区） ✅
3. 配置 Arduino IDE（自定义分区、启用擦除） ✅
4. 编辑 system_setup.h（凭证） ✅
5. 连接右 USB 端口 ✅
6. 上传 #1（预期会出错） ✅
7. 断开右 USB，连接左 USB 端口 ✅
8. 上传 #2（错误消失！） ✅
9. 在 Telegram 中收到欢迎消息 ✅
10. 发送 /estado 验证一切工作 ✅
```

**你已准备好用 KissTelegram 构建令人惊艳的项目！** 🎉

---

**祝你编码愉快！**

*Vicente Soriano - victek@gmail.com*
