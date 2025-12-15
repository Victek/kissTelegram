# KissTelegram

![License](https://img.shields.io/badge/license-MIT-blue.svg)
![Platform](https://img.shields.io/badge/platform-ESP32--S3-orange.svg)
![Version](https://img.shields.io/badge/version-1.x-green.svg)
![Language](https://img.shields.io/badge/languages-7-brightgreen.svg)

**中文** | [文档](docs/GETTING_STARTED_CN.md) | [基准测试](docs/BENCHMARK.md)

---

> **首次使用ESP32-S3与KISSTELEGRAM?**
> **请先阅读:** [**GETTING_STARTED_CN.md**](docs/GETTING_STARTED_CN.md)
> ESP32-S3需要**两步上传过程**,因为有自定义分区。忽略本指南将导致启动错误和错误的分区!

---

## ESP32-S3上Telegram机器人的企业级强大库

KissTelegram是**唯一为ESP32从头构建的Telegram库**,专为关键任务应用而设计。与依赖Arduino的`String`类(导致内存碎片和泄漏)的其他库不同,KissTelegram使用纯`char[]`数组以获得坚不可摧的稳定性。

### 为什么选择KissTelegram?

- 厌倦了因弱库、内存泄漏、临时解决方案、缺乏支持、空话、不起作用的术语、重启而丢失的项目....

- 这是我对其他库的愿景和经验,这是KissTelegram的结果:

- **零消息丢失**: LittleFS上的持久队列,可在崩溃、重启和WiFi故障中幸存
- **无内存泄漏**: 纯`char[]`实现,无String碎片
- **SSL/TLS安全**: 通过证书验证安全连接到Telegram API(到2035年)
- **智能电源管理**: 6种电源模式(BOOT、LOW、IDLE、ACTIVE、TURBO、MAINTENANCE)
- **消息优先级**: CRITICAL、HIGH、NORMAL、LOW,具有智能队列管理
- **涡轮模式**: 大型消息队列的批处理(0.9 msg/s)
- **多语言i18n**: 编译时语言选择用于消息发送(7种语言,零运行时开销)
- **企业级OTA**: 双启动固件更新,具有自动回滚和安全管理
- **100%闪存利用**: 自定义分区方案最大化ESP32-S3的16MB闪存
- **比Espressif OTA更安全**: PIN/PUK认证,校验和验证,60秒验证窗口
- **独立于外部库**: 一切从头构建,KissTelegram库自带JSON解析器。

---

## 硬件要求

- **ESP32-S3** 配备 **16MB Flash** / **8MB PSRAM**
- WiFi连接
- Arduino IDE或PlatformIO

---

## 安装

### Arduino IDE

1. 下载此存储库为ZIP
2. 打开Arduino IDE → Sketch → Include Library → Add .ZIP Library
3. 选择下载的文件

### PlatformIO

添加到您的`platformio.ini`:

```ini
lib_deps =
    https://github.com/victek/KissTelegram.git
```

---

## 自定义分区方案

KissTelegram包含优化的`partitions.csv`,最大化闪存使用:

```csv
# Name,   Type, SubType, Offset,  Size
nvs,      data, nvs,     0x9000,  0x4000,
otadata,  data, ota,     0xd000,  0x2000,
app0,     app,  factory, 0x10000, 0x180000,   # 1.5MB
app1,     app,  ota_0,   0x190000,0x180000,   # 1.5MB
spiffs,   data, spiffs,  0x310000,0xCF0000,   # 13MB!
```

**13MB SPIFFS存储** - 比Espressif的默认方案多8MB!

使用此分区方案:
1. 将`partitions.csv`复制到您的项目目录
2. 在Arduino IDE中: Tools ->Partition Scheme ->Custom
3. 在PlatformIO中: `board_build.partitions = partitions.csv`

---

## 快速开始

### 基本示例

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

### OTA更新示例

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

**OTA流程:**
1. 向您的机器人发送`/ota`
2. 使用`/otapin YOUR_PIN`输入PIN
3. 上传`.bin`固件文件
4. 机器人自动验证校验和
5. 使用`/otaconfirm`确认
6. 重启后,在60秒内使用`/otaok`验证
7. 验证失败时自动回滚!

- 阅读您首选语言的Readme_KissOTA.md以了解更多解决方案。

---

## 主要功能说明

### 1. 持久消息队列

消息存储在LittleFS上,具有自动批量删除:

```cpp
bot.queueMessage(chat_id, "Important message", KissTelegram::PRIORITY_HIGH);
```

- 在崩溃、WiFi断开、重启中幸存
- 自动重试失败的发送
- 智能批量删除(每10条消息+队列为空时)
- 零消息丢失保证

### 2. 电源管理

6种智能电源模式适应您应用的需求:

```cpp
bot.setPowerMode(KissTelegram::POWER_ACTIVE);
bot.setPowerConfig(30, 60, 10); // idle, decay, boot stable times
```

- **POWER_BOOT**: 初始启动阶段(10s)
- **POWER_LOW**: 最小活动,慢速轮询
- **POWER_IDLE**: 无最近活动,减少检查
- **POWER_ACTIVE**: 正常操作
- **POWER_TURBO**: 高速批处理(50ms间隔)
- **POWER_MAINTENANCE**: 手动覆盖用于更新
- **平滑过渡的衰减时序**

### 3. 消息优先级

四个优先级确保关键消息首先发送,跳过较低优先级:

```cpp
bot.sendMessage(chat_id, "Normal message", KissTelegram::PRIORITY_NORMAL);
bot.sendMessage(chat_id, "Alert!", KissTelegram::PRIORITY_CRITICAL);
```

队列处理: **CRITICAL /HIGH /NORMAL /LOW**
内部进程: **OTAMODE /MAINTENANCEMODE**

### 4. SSL/TLS安全

通过证书验证的安全连接(到2035年):

```cpp
bool secure = bot.isSSLSecure();
String sslInfo = bot.getSSLInfo();
bot.testSSLConnection();
```

- 安全和不安全之间的自动回退
- 定期ping检查以维持连接
- 可重用的连接代码节省连接开销以获得最大性能

### 5. 涡轮模式

使用/llenar命令发送批次时自动激活:

```cpp
bot.enableTurboMode();  // Auto activation
```

- 每个周期处理10条消息
- 批次之间50ms间隔
- 达到0.9 msg/s吞吐量
- 队列发送后自动停用

### 6. 操作模式

不同场景的预配置配置文件:

```cpp
bot.setOperationMode(KissTelegram::MODE_RELIABILITY);
```

- **MODE_BALANCED**: 默认(轮询:10s,重试:3)
- **MODE_PERFORMANCE**: 快速(轮询:5s,重试:2)
- **MODE_POWERSAVE**: 慢速(轮询:30s,重试:2)
- **MODE_RELIABILITY**: 健壮(轮询:15s,重试:5)

### 7. 诊断

完整的监控和调试:

```cpp
bot.printDiagnostics();
bot.printStorageStatus();
bot.printPowerStatistics();
bot.printSystemStatus();
```

显示:
- 空闲内存(heap/PSRAM)
- 消息队列统计
- 连接质量
- 电源模式历史
- 存储使用
- 正常运行时间

---

## 8. WiFi管理
- 集成WiFi管理器仅在WiFi稳定时激活其他任务
- 防止竞争条件
- 将进行中的消息回收到FS存储,直到恢复连接,可容纳多达3500条消息(默认但易于扩展,取决于您想使用多少空间)
- 连接质量监控(EXCELLENT、GOOD、FAIR、POOR、DEAD)和RSSI输出级别
- 您只需要关心您的草图,添加您的代码,KissTelegram处理关键任务,WiFi、SSL、消息、OTA、电源管理、优先级..您节省的所有工作....

---

## 9. 关键功能: `/estado`命令

**您将在草图中包含的最强大的调试工具**

向您的机器人发送`/estado`,获取当时**草图的完整健康报告**(提供7种语言):

```
KissTelegram_test, [15/12/2025 13:11]
🔧 KissTelegram v0.9.0
📦 Build: Dec 15 2025 13:10:23 (0xD984C13E)

✅ 系统可靠性
✓ 系统: 可靠
✓ 已发送消息: 940
📨 待处理消息: 70
✓ 丢失消息: 0
🗑️ 丢弃(队列满): 0

⚠️ 外部逆境
⚠️ 总错误: 0
🔄 已恢复(回退): 0
📡 WiFi掉线: 0

📊 技术信息
⏱️ 正常运行时间: 0h 0m
🧠 空闲RAM: 223960 bytes
💾 空闲PSRAM: 1027820 bytes
💽 空闲FS: 13549568 bytes
📦 FS中最大: 3500条消息
⚡ 电源模式: 3 
📶 WiFi信号: -64 dBm (一般)
🔒 SSL: 安全
🚀 涡轮: 不活动
🤖 自动消息: 是

```

**为什么`/estado`必不可少:**
- 即时系统健康检查
- WiFi质量监控(诊断连接问题)
- 内存泄漏检测(观察空闲堆)
- 消息队列状态(查看待处理/失败的消息)
- 正常运行时间跟踪(稳定性监控)
- 您的第一个诊断工具

**专业提示:** 在每次固件更新后让`/estado`成为您的第一条消息,以验证一切正常!

---

## 10. NTP
- 用于SSL的同步/重新同步的自有代码。GNSS、LTE和调度程序(企业版)
---

## 11. 文档(7种语言)

- **[GETTING_STARTED_CN.md](docs/GETTING_STARTED_CN.md)** - **从这里开始!** 从收到ESP32-S3到发送第一条Telegram消息的完整指南
- **[README_CN.md](docs/README_CN.md)**(本文件) - 功能概述、快速入门、API参考
- **[BENCHMARK.md](docs/BENCHMARK.md)** - 与6个Telegram库的技术比较(仅英文,但不言自明)
- **[README_KissOTA_XX.md](docs/README_KissOTA_CN.md)** - 极具价值,因为它详细说明了OTA更新系统的步骤(7种语言:EN、ES、FR、IT、DE、PT、CN)

**选择您的语言:** 发送到Telegram的所有构造函数消息通过编译期间的语言选择以7种语言显示(lang.h)。


## OTA安全优势

KissTelegram OTA**比Espressif的架构更安全,并在您的ESP32S3上节省空间**:

| 功能 | KissTelegram OTA | Espressif OTA |
|---------|------------------|---------------|
| 认证 | PIN + PUK | 无 |
| 校验和确认 | 自动CRC32 | 手动 |
| 备份和回滚 | 自动 | 手动 |
| 验证窗口 | 使用`/otaok`60秒 | 无 |
| 启动循环检测 | 是 | 否 |
| Telegram集成 | 原生 | 需要自定义代码 |
| 闪存优化 | 13MB SPIFFS | 5MB SPIFFS |

---

## API参考

### 初始化

```cpp
KissTelegram(const char* token);
void enable();
void disable();
```

### 消息传递

```cpp
bool sendMessage(const char* chat_id, const char* text,
                 MessagePriority priority = PRIORITY_NORMAL);
bool sendMessageDirect(const char* chat_id, const char* text);
bool queueMessage(const char* chat_id, const char* text,
                  MessagePriority priority = PRIORITY_NORMAL);
void processQueue();
```

### 配置

```cpp
void setMinMessageInterval(int milliseconds);
void setMaxMessageSize(int size);
void setMaxRetryAttempts(int attempts);
void setPollingTimeout(int seconds);
void setOperationMode(OperationMode mode);
void setPowerMode(PowerMode mode);
```

### 监控

```cpp
int getQueueSize();
int getPendingMessages();
ConnectionQuality getConnectionQuality();
PowerMode getCurrentPowerMode();
unsigned long getUptime();
int getFreeMemory();
```

### 存储

```cpp
void enableStorage(bool enable = true);
bool saveNow();
bool restoreFromStorage();
void clearStorage();
```

---

## 示例

查看库中包含的.ino以探索一些场景和功能以及我在KissTelegram中的编码风格。更好的是,在[lang.h]中取消注释您的语言以接收主要构造函数(.cpp)在您本地语言中的消息,如果所有语言都被注释掉,消息为西班牙语,默认语言:

代码约定是英文的,但思想和注释是西班牙语,我的母语,使用您的在线翻译器,代码很简单,代码中是我的愿景和KissTelegram的概念...

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
## 基本配置设置
- 在您的KissTelegram文件夹中将system_setup_template.h重命名为system_setup.h以开始编译。
- 用您的凭据替换以下行。

````cpp
#define KISS_FALLBACK_BOT_TOKEN "YOUR_TELEGRAM_BOT_TOKEN"
#define KISS_FALLBACK_CHAT_ID "YOUR_CHAT_ID number"
#define KISS_FALLBACK_WIFI_SSID "YOUR_WIFI_SSID"
#define KISS_FALLBACK_WIFI_PASSWORD "YOUR_WIFI_PASSWORD"
#define KISS_FALLBACK_OTA_PIN "0000"
#define KISS_FALLBACK_OTA_PUK "00000000"
````

## 许可证

此项目根据MIT许可证授权 - 有关详细信息,请参阅[LICENSE](LICENSE)文件。

---

## 架构、愿景、概念、解决方案和设计(以及任何故障的责任人,是我...)

**Vicente Soriano**
电子邮件: victek@gmail.com
GitHub: [victek](https://github.com/victek/KissTelegram)

**贡献者**
- 许多AI助手在翻译、代码、故障排除和许多小时试图阻止它们重新发明轮子.....

---


## 贡献

欢迎贡献!请随时提交Pull Request或给我发电子邮件,但我更喜欢PR,以便其他人可以找到您的问题。

---

## 支持

如果您发现此库有用,请考虑:
- 为此存储库加星标
- 通过GitHub Issues报告错误
- 分享您使用KissTelegram的项目
- 向您认识的人评论此库的功能和解决方案
- 提出使用案例和使用KissTelegram的经验

---
