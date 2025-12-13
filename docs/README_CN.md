# KissTelegram

![License](https://img.shields.io/badge/license-MIT-blue.svg)
![Platform](https://img.shields.io/badge/platform-ESP32--S3-orange.svg)
![Version](https://img.shields.io/badge/version-1.x-green.svg)
![Language](https://img.shields.io/badge/languages-7-brightgreen.svg)

**中文** | [文档](GETTING_STARTED_CN.md) | [基准测试](BENCHMARK.md)

---

> 🚨 **第一次使用 ESP32-S3 与 KissTelegram？**
> **先读这个：** [**GETTING_STARTED_CN.md**](GETTING_STARTED_CN.md) ⚠️
> ESP32-S3 由于自定义分区需要**两步上传流程**。跳过此指南会导致启动错误！

---

## 用于 ESP32-S3 的强大企业级 Telegram 机器人库

KissTelegram 是**唯一专为任务关键型应用从零开始构建的 ESP32 Telegram 库**。与其他依赖 Arduino `String` 类的库不同（会导致内存碎片化和泄漏），KissTelegram 使用纯 `char[]` 数组以实现非常稳定的性能。

### 为什么选择 KissTelegram？

- 厌倦了弱库导致的项目丢失、内存泄漏、临时解决方案、缺乏支持、虚幻承诺、重启....

- 我的愿景，现在变成了事实：

- **零消息丢失**：持久化 LittleFS 队列可在崩溃、重启和 WiFi 故障中存活
- **无内存泄漏**：纯 `char[]` 实现，无 String 碎片化问题
- **SSL/TLS 安全**：与 Telegram API 的安全连接，支持证书验证
- **智能电源管理**：6 种功率模式（BOOT、LOW、IDLE、ACTIVE、TURBO、MAINTENANCE）
- **消息优先级**：CRITICAL、HIGH、NORMAL、LOW，支持智能队列管理
- **Turbo 模式**：大消息队列的批处理（0.9 msg/s）
- **多语言 i18n**：编译时语言选择（7 种语言，无运行时开销）
- **企业级 OTA**：双启动固件更新，自动回滚和安全网关
- **100% Flash 利用**：自定义分区方案最大化 ESP32-S3 16MB Flash
- **比 Espressif OTA 更安全**：PIN/PUK 认证、校验和验证、60 秒验证窗口
- **独立于外部库**：一切从零开始编写，包括自己的 Json 解析器

---

## 硬件要求

- **ESP32-S3**，配备 **16MB Flash** / **8MB PSRAM**
- WiFi 连接
- Arduino IDE 或 PlatformIO

---

## 安装

### Arduino IDE

1. 下载此存储库为 ZIP 文件
2. 打开 Arduino IDE → Sketch → Include Library → Add .ZIP Library
3. 选择下载的文件

### PlatformIO

添加到你的 `platformio.ini`：

```ini
lib_deps =
    https://github.com/victek/KissTelegram.git
```

---

## 自定义分区方案

KissTelegram 包含优化的 `partitions.csv` 以最大化 Flash 使用：

```csv
# Name,   Type, SubType, Offset,  Size
nvs,      data, nvs,     0x9000,  0x4000,
otadata,  data, ota,     0xd000,  0x2000,
app0,     app,  factory, 0x10000, 0x180000,   # 1.5MB
app1,     app,  ota_0,   0x190000,0x180000,   # 1.5MB
spiffs,   data, spiffs,  0x310000,0xCF0000,   # 13MB!
```

**13MB SPIFFS 存储** - 比 Espressif 默认方案多 8MB！

使用此分区方案：
1. 复制 `partitions.csv` 到你的项目目录
2. 在 Arduino IDE 中：Tools → Partition Scheme → Custom
3. 在 PlatformIO 中：`board_build.partitions = partitions.csv`

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

### OTA 更新示例

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

**OTA 流程：**
1. 向机器人发送 `/ota`
2. 用 `/otapin YOUR_PIN` 输入 PIN
3. 上传 `.bin` 固件文件
4. 机器人自动验证校验和
5. 用 `/otaconfirm` 确认
6. 重启后，在 60 秒内用 `/otaok` 验证
7. 验证失败时自动回滚！

- 阅读你首选语言的 Readme_KissOTA.md 了解更多关于此解决方案的信息

---

## 关键特性详解

### 1. 持久化消息队列

消息存储在 LittleFS 中，支持自动批量删除：

```cpp
bot.queueMessage(chat_id, "Important message", KissTelegram::PRIORITY_HIGH);
```

- 在崩溃、WiFi 断开和重启时存活
- 自动重试失败的发送
- 智能批删除（每 10 条消息 + 队列为空时）
- 零消息丢失保证

### 2. 电源管理

6 种智能功率模式适应应用需求：

```cpp
bot.setPowerMode(KissTelegram::POWER_ACTIVE);
bot.setPowerConfig(30, 60, 10); // idle, decay, boot stable times
```

- **POWER_BOOT**：初始启动阶段（10 秒）
- **POWER_LOW**：最小活动，慢速轮询
- **POWER_IDLE**：无最近活动，减少检查
- **POWER_ACTIVE**：正常操作
- **POWER_TURBO**：高速批处理（50ms 间隔）
- **POWER_MAINTENANCE**：手动覆盖以进行更新
- **衰减时序以实现平滑切换**

### 3. 消息优先级

四个优先级确保关键消息优先发送：

```cpp
bot.sendMessage(chat_id, "Normal message", KissTelegram::PRIORITY_NORMAL);
bot.sendMessage(chat_id, "Alert!", KissTelegram::PRIORITY_CRITICAL);
```

队列处理：**CRITICAL → HIGH → NORMAL → LOW**
内部流程：**OTAMODE → MAINTENANCEMODE**

### 4. SSL/TLS 安全

具有证书验证的安全连接：

```cpp
bool secure = bot.isSSLSecure();
String sslInfo = bot.getSSLInfo();
bot.testSSLConnection();
```

- 自动安全/不安全回退
- 定期 ping 检查以保持连接
- 可重用连接代码保存连接头以获得最高吞吐量

### 5. Turbo 模式

当队列 > 100 条消息时自动激活：

```cpp
bot.enableTurboMode();  // Manual activation
```

- 每个周期处理 10 条消息
- 批次间隔 50ms
- 实现 15-20 msg/s 吞吐量
- 队列发送完毕时自动停用

### 6. 操作模式

针对不同场景的预配置配置文件：

```cpp
bot.setOperationMode(KissTelegram::MODE_RELIABILITY);
```

- **MODE_BALANCED**：默认（轮询：10s，重试：3）
- **MODE_PERFORMANCE**：快速（轮询：5s，重试：2）
- **MODE_POWERSAVE**：慢速（轮询：30s，重试：2）
- **MODE_RELIABILITY**：健壮（轮询：15s，重试：5）

### 7. 诊断

全面的监控和调试：

```cpp
bot.printDiagnostics();
bot.printStorageStatus();
bot.printPowerStatistics();
bot.printSystemStatus();
```

显示：
- 可用内存（heap/PSRAM）
- 消息队列统计
- 连接质量
- 功率模式历史
- 存储使用情况
- 运行时间

---

## 8. WiFi 管理
- 内置 WiFi 管理器在 WiFi 稳定前触发其他任务开始
- 防止竞态条件
- 声称正在进行的消息进入 FS 存储直到连接重新建立，可保存多达 3500 条消息（默认但易于扩展）
- 连接质量监控（EXCELLENT、GOOD、FAIR、POOR、DEAD）和 RSSI 输出级别
- 你只需要关心你的程序，KissTelegram 处理其余部分

---

## 9. 杀手锏特性：`/estado` 命令

**你将使用过最强大的调试工具：**

向你的机器人发送 `/estado`，在数秒内获得**完整的健康报告**：

```
📦 KissTelegram v1.x.x
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

**为什么 `/estado` 是必需的：**
- ✅ 即时系统健康检查
- ✅ WiFi 质量监控（诊断连接问题）
- ✅ 内存泄漏检测（监控可用堆）
- ✅ 消息队列状态（查看待处理/失败消息）
- ✅ 运行时间跟踪（稳定性监控）
- ✅ 支持 7 种语言
- ✅ 调试问题时的首选工具

**专业提示：**在每次固件更新后将 `/estado` 设为你的第一条消息以验证一切正常！

---

## 10. NTP
- 自己的代码用于 SSL 和调度器的同步/重新同步（企业版）
---

## 11. 文档（7 种语言）

- **[GETTING_STARTED_CN.md](GETTING_STARTED_CN.md)** - **从这里开始！** 从打开 ESP32-S3 到发送第一条 Telegram 消息的完整指南
- **[README_CN.md](README_CN.md)** （此文件）- 功能概览、快速开始、API 参考
- **[BENCHMARK.md](BENCHMARK.md)** - 与 6 个其他 Telegram 库的技术对比（仅英文）
- **[README_KissOTA_XX.md](README_KissOTA_EN.md)** - OTA 更新系统文档（7 种语言：EN、ES、FR、IT、DE、PT、CN）

**选择你的语言：**KissTelegram 的所有系统消息通过编译时选择支持 7 种语言。


## OTA 安全优势

KissTelegram OTA **比 Espressif 的实现更安全**：

| 特性 | KissTelegram OTA | Espressif OTA |
|---------|------------------|---------------|
| 认证 | PIN + PUK | 无 |
| 校验和验证 | CRC32 自动 | 手动 |
| 备份和回滚 | 自动 | 手动 |
| 验证窗口 | 60 秒 with `/otaok` | 无 |
| 启动循环检测 | 是 | 否 |
| Telegram 集成 | 原生 | 需要自定义代码 |
| Flash 优化 | 13MB SPIFFS | 5MB SPIFFS |

---

## API 参考

### 初始化

```cpp
KissTelegram(const char* token);
void enable();
void disable();
```

### 消息

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

查看库中包含的 .ino 以探索一些场景和特性以及我的 KissTelegram 代码风格。更好的是，在 [lang.h] 中取消注释你的语言以从主构造函数 (.cpp) 接收你本地语言的消息。如果所有语言都被注释，你会得到西班牙语的消息，这是默认语言：

代码约定为英文，但思想和注释为我的母语，使用你的在线翻译器，代码很简单，但代码背后更复杂...

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
- 在你的 KissTelegram 文件夹中将 system_setup_template.h 重命名为 system_setup.h 以开始编译
- 用你的设置替换以下行

````cpp
#define KISS_FALLBACK_BOT_TOKEN "YOUR_TELEGRAM_BOT_TOKEN"
#define KISS_FALLBACK_CHAT_ID "YOUR_CHAT_ID number"
#define KISS_FALLBACK_WIFI_SSID "YOUR_WIFI_SSID"
#define KISS_FALLBACK_WIFI_PASSWORD "YOUR_WIFI_PASSWORD"
#define KISS_FALLBACK_OTA_PIN "0000"
#define KISS_FALLBACK_OTA_PUK "00000000"
````

## 许可证

本项目根据 MIT 许可证授权 - 有关详细信息，请参阅 [LICENSE](LICENSE) 文件

---

## 架构、愿景、概念、解决方案和设计（以及任何故障的责任人）

**Vicente Soriano**
电邮：victek@gmail.com
GitHub：[victek](https://github.com/victek)

**贡献者**
- 许多 AI 助手在翻译、代码、故障排除和玩笑方面的帮助

---


## 贡献

欢迎贡献！请随时提交拉取请求

---

## 支持

如果你发现此库有用，请考虑：
- 给这个存储库加星
- 通过 GitHub Issues 报告 bug
- 分享使用 KissTelegram 的项目

---
