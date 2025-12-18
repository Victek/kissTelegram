# ESP32-S3 上的 KissTelegram 入门指南

**从零开始配置 ESP32-S3 直到发送第一条 Telegram 消息的完整指南**

> ⚠️ **关键提示**: 在上传任何固件之前请完整阅读本指南。ESP32-S3 N16R8 因为自定义分区需要**两步上传流程**。跳过步骤将导致错误!

---

## 目录

1. [开始之前](#开始之前)
2. [创建你的 Telegram 机器人](#创建你的-telegram-机器人)
3. [硬件配置](#硬件配置)
4. [Arduino IDE 配置](#arduino-ide-配置)
5. [第一次上传(使用 esptool 创建分区)](#第一次上传)
6. [配置文件](#配置文件)
7. [成功! 接下来做什么?](#成功-接下来做什么)

---

## 开始之前

### 你需要准备

- **ESP32-S3 N16R8** (16MB Flash / 8MB PSRAM)
- **两根 USB-C 线** (用于在 bootloader 和 OTG 端口之间切换)
- **Arduino IDE 2.x** 或更高版本
- **Windows PC** (本指南针对 Windows,Linux/Mac 需要调整路径)
- 手机上的 **Telegram 账号**

### 与众不同之处

你的新 ESP32-S3 N16R8 自带内置的 RGB LED 演示应用。KissTelegram **完全替换分区表**以最大化利用 16MB 闪存:

| 分区 | 默认 Espressif | KissTelegram 自定义 |
|------|---------------|---------------------|
| 应用空间 | 1.5 MB | 4.5 MB (3倍大!) |
| 文件系统 | 5 MB | 13 MB (2.6倍大!) |
| 总计使用 | 6.5 MB | 17.5 MB |

这就是为什么需要两步上传流程: **分区表在两次上传之间发生变化**.

---

## 创建你的 Telegram 机器人

### 步骤 1: 与 BotFather 对话

1. 在手机上打开 Telegram
2. 搜索 `@BotFather` (官方机器人,有蓝色认证标记)
3. 使用 `/start` 开始对话
4. 使用 `/newbot` 创建你的机器人
5. 选择一个名称 (例如: "我的家庭助手")
6. 选择一个用户名 (必须以 `bot` 结尾,例如: "myhome_assistant_bot")
7. **保存你的 Bot Token** - 看起来像: `1234567890:ABCdefGHIjklMNOpqrsTUVwxyz`

### 步骤 2: 获取你的 Chat ID

**方法 1: 使用机器人(更简单)**

1. 在 Telegram 中搜索 `@ChatIDHelperBot`
2. 使用 `/start` 开始对话
3. 它会回复你的 **Chat ID** (一个数字如 `123456789`)
4. **保存这个数字** - 配置时需要用到

**方法 2: 使用网页浏览器**

1. 向你新创建的机器人发送任意消息
2. 打开浏览器访问:
   ```
   https://api.telegram.org/bot<你的_BOT_TOKEN>/getUpdates
   ```
   (用你的实际 token 替换 `<你的_BOT_TOKEN>`)
3. 在 JSON 响应中查找 `"chat":{"id":123456789`
4. 那个数字就是你的 **Chat ID**

**✅ 现在你有了:**
- Bot Token: `1234567890:ABCdefGHIjklMNOpqrsTUVwxyz`
- Chat ID: `123456789`

妥善保管! 很快就会用到.

---

## 硬件配置

### 理解两个 USB-C 端口

你的 ESP32-S3 N16R8 有**两个 USB-C 端口**:

```
┌─────────────────────┐
│  ┌─┐         ESP32  │
│  │•│  ← 电源 LED     │
│  └─┘                 │
│  [USB-C]  ← 右侧端口 (Bootloader/上传)
│                      │
│                      │
│  [USB-C]  ← 左侧端口 (OTG/正常运行)
│                      │
└─────────────────────┘
```

**右侧端口 (靠近电源 LED):**
- 用于**初始固件上传**
- 用于 **bootloader 模式**
- 当 Arduino IDE 显示 "Connecting..." 时使用此端口

**左侧端口 (OTG):**
- 第一次上传后用于**正常运行**
- 用于**第二次上传**(分区修复)
- 正常运行时用于串口监视器

---

## Arduino IDE 配置

### 步骤 1: 显示隐藏文件 (Windows)

1. 打开**文件资源管理器**
2. 点击**查看**选项卡 → **显示** → 勾选:
   - ✅ 文件扩展名
   - ✅ 隐藏的项目
3. 在**筛选器**选项卡: **所有文件类型**

### 步骤 2: 修改 boards.txt

1. 导航到:
   ```
   C:\Users\<你的用户名>\AppData\Local\Arduino15\packages\esp32\hardware\esp32\3.3.4\
   ```
   (如果版本不同,用你的 ESP32 核心版本替换 `3.3.4`)

2. 找到并打开 `boards.txt` (使用 Notepad++ 或任何文本编辑器)

3. 按 `Ctrl+F` 搜索:
   ```
   gen4esp32_4MBapp_4MBota_7MBspiffs
   ```

4. **在该行正下方**,粘贴这三行:
   ```
   gen4-ESP32-S3R8n16.menu.PartitionScheme.gen4esp32scheme2=Custom (4MB APP/12MB LtlFS)
   gen4-ESP32-S3R8n16.menu.PartitionScheme.gen4esp32scheme2.build.custom_partitions=partitions
   gen4-ESP32-S3R8n16.menu.PartitionScheme.gen4esp32scheme2.upload.maximum_size=4718592
   ```

5. **保存**并关闭 `boards.txt`

6. 如果 Arduino IDE 已打开,**关闭并重启**

### 步骤 3: 配置 Arduino IDE

1. **打开**你的 KissTelegram sketch 文件夹 (包含 `.ino`, `.h`, `.cpp`, 和 `partitions.csv`)

2. 在 Arduino IDE 中,前往**工具** → **开发板** → **4D Systems gen4-ESP32-S3R8n16**

3. **工具** → **重新加载开发板数据** (底部会显示确认信息)

4. **配置所有工具菜单选项:**

   | 设置 | 值 |
   |------|-----|
   | **开发板** | 4D Systems gen4-ESP32-S3R8n16 |
   | **USB CDC On Boot** | Enabled |
   | **Flash Size** | 16MB (128Mb) |
   | **Partition Scheme** | **Custom (4MB APP/12MB LtlFS)** ⚠️ |
   | **PSRAM** | OPI PSRAM |
   | **Upload Speed** | 921600 |
   | **Erase All Flash Before Sketch Upload** | **Enabled** ⚠️ |

   ⚠️ **关键设置** - 请仔细检查!

5. **工具** → **串口监视器** → 设置波特率为 **115200**

---

## 第一次上传 (常见问题)

### 为什么需要两次上传

**问题:**
- 第一次上传: Arduino 使用**旧分区表**写入固件
- ESP32 启动: 发现**新分区表**(来自 `partitions.csv`)
- 固件写入位置与 ESP32 查找位置**不匹配**
- 结果: 启动错误、分区错误、崩溃

**解决方案:**
两次上传确保固件写入到新分区表定义的**正确位置**.

---

### 上传 #1: 初始闪存 (烧录 Bootloader)

1. **连接右侧 USB-C 端口**(靠近电源 LED)到你的 PC

2. **选择端口**: 工具 → 端口 → 选择出现的 COM 端口

3. **验证设置**:
   - ✅ Erase All Flash Before Sketch Upload: **Enabled**
   - ✅ Partition Scheme: **Custom (4MB APP/12MB LtlFS)**
   

4. **工具,烧录 Bootloader** (点击此选项)
   - ✅ 工具 ➡️, 在下拉菜单末尾找到'烧录 Bootloader'
   - ✅ 点击这里将写入新分区,使用 esptool
   - 耗时 53.6 秒,KissTelegram 的新分区就绪 

继续上传 #2.

---

### 上传 #2: Sketch 上传

1. **断开右侧 USB-C 端口**

2. **连接左侧 USB-C 端口**(OTG 端口)到你的 PC

3. **选择新端口**: 工具 → 端口 → 选择新的 COM 端口
   - **重要**: 端口号会改变! 在串口监视器中查找数据以确认正确端口,例如,按 ESP32s3 的复位键直到看到数据响应

4. **再次验证设置**:
   - ✅ Erase All Flash Before Sketch Upload: **Enabled**
   - ✅ Partition Scheme: **Custom (4MB APP/12MB LtlFS)**

5. **再次按上传** (`Ctrl+U`)

6. **等待 ~2-3 分钟** (擦除 + 上传)

7. **打开串口监视器** - 现在应该看到 (如果你在 system_setup.h (重命名的 system_setup_template) 中正确设置了凭据):
   ```
   ✅ KissTelegram v0.9.x
   ✅ WiFi 已连接
   ✅ Telegram 机器人已启用
   ✅ 系统就绪
   ```

8. **检查 Telegram** - 你会收到欢迎消息:
   ```
   📦 你好! KissTelegram 已就绪。
   🔌 构建: 2025-12-12 10:30:45 (0xABCD1234)
   📡 WiFi 信号: -59 dBm (良好)
   ✅ 0 条消息在队列中
   ```

**成功!** 你的 ESP32-S3 现在运行 KissTelegram 且分区正确.

---

### 未来的上传

**好消息:** 两次初始上传后,所有未来的上传正常工作:

- 使用**左侧 USB-C 端口**(OTG)
- **不再需要** "Erase All Flash" (除非你修改了 NVRAM 数据)
- 上传一次即可立即工作

---

## 配置文件

### system_setup.h (第一次上传前必需!)

**编译前:**

1. 导航到你的 KissTelegram 文件夹
2. 找到 `system_setup_template.h`
3. **重命名**为 `system_setup.h`
4. **打开** `system_setup.h` 并填写:

```cpp
// 你的 Telegram 机器人 (来自 BotFather)
#define KISS_FALLBACK_BOT_TOKEN "1234567890:ABCdefGHIjklMNOpqrsTUVwxyz"

// 你的 Chat ID (来自 @userinfobot)
#define KISS_FALLBACK_CHAT_ID "123456789"

// 你的 WiFi 凭据
#define KISS_FALLBACK_WIFI_SSID "你的WiFi名称"
#define KISS_FALLBACK_WIFI_PASSWORD "你的WiFi密码"

// OTA 安全 (更改默认 PIN/PUK!)
#define KISS_FALLBACK_OTA_PIN "0000"        // 4位数字
#define KISS_FALLBACK_OTA_PUK "00000000"    // 8位数字
```

5. **保存**文件

**⚠️ 安全警告:** 将默认 PIN (`0000`) 和 PUK (`00000000`) 更改为你自己的密码!

---

### lang.h (可选: 选择你的语言)

KissTelegram 支持 7 种系统消息语言:

```cpp
// 在 lang.h 中,取消注释一种语言:

// #define LANG_CN  // 中文 (简体中文)
// #define LANG_DE  // Deutsch (德语)
// #define LANG_EN  // English (英语)
// #define LANG_FR  // Français (法语)
// #define LANG_IT  // Italiano (意大利语)
// #define LANG_PT  // Português (葡萄牙语)
// #define LANG_ES  // Español (西班牙语) - 默认(如果全部注释)
```

**编译前**选择你的语言以获得本地化消息.

---

## 成功! 接下来做什么?

### 验证一切正常

1. 在 Telegram 中**向你的机器人发送 `/status`** - 你会得到详细状态报告:
   ```
   📦 KissTelegram v1.x.x
   🎯 系统可靠性
   ✅ 系统: 可靠
   ✅ 已发送消息: 2
   💾 待发送消息: 0
   📡 WiFi 信号: -59 dBm (良好)
   🔋 运行时间: 123 秒
   💾 可用内存: 223 KB
   ```

2. **检查串口监视器** - 应该没有错误显示

3. **测试命令**:
   - `/start` - 欢迎消息
   - `/help` - 可用命令
   - `/status` - 系统状态(健康检查)

---

### 理解 OTA 更新

一旦 KissTelegram 运行,你可以**通过 Telegram** 更新固件(无需 USB 线!):

1. 向你的机器人发送 `/ota`
2. 输入 PIN: `/otapin 0000` (或你的自定义 PIN)
3. **发送你的固件 `.bin` 文件**(在 Telegram 中拖放)
4. 机器人自动验证校验和
5. 确认: `/otaconfirm`
6. ESP32 用新固件重启
7. **60秒内**,发送 `/otaok` 确认正常工作
8. 如果不确认,ESP32 会**自动回滚**到之前的固件!

📖 **了解更多:** 查看 `README_KissOTA_CN.md` 获取完整 OTA 文档.

---

### 探索示例代码

示例 `suite_kiss.ino` 演示:

- ✅ 带质量监控的 WiFi 管理
- ✅ 带优先级的消息队列
- ✅ 电源管理模式
- ✅ 命令处理 (`/start`, `/help`, `/status` 等)
- ✅ 通过 Telegram 的 OTA 更新
- ✅ 崩溃恢复和持久化
- ✅ 安全的 SSL/TLS 连接

**专业提示:** 使用 `/status` 命令作为你的**健康监控工具** - 它是你了解 KissTelegram 内部的窗口!

---

### 常见故障排除

**问题: "未找到端口" 或 "拒绝访问"**
- Windows 锁定了端口。断开 USB,等待 5 秒,重新连接。
- 尝试不同的 USB 线(有些仅用于充电,不传输数据)

**问题: 上传时 "等待设备超时"**
- USB 端口错误! 记住: 第一次上传用右侧端口,第二次用左侧端口
- 点击上传时按住 ESP32 的 BOOT 按钮,"Connecting..." 出现后松开

**问题: 串口监视器显示乱码**
- 波特率错误。在串口监视器下拉菜单中设置为 **115200**

**问题: 机器人在 Telegram 中无响应**
- 验证 `system_setup.h` 有正确的 Bot Token 和 Chat ID
- 验证 WiFi 凭据正确
- 打开串口监视器查找 WiFi 连接消息

**问题: 编译错误 "分区表不适合"**
- 没有正确添加自定义分区到 `boards.txt`
- 或没有在工具 → Partition Scheme 中选择 "Custom (4MB APP/12MB LtlFS)"

---

### 获取更多帮助

- 📧 **电子邮件**: victek@gmail.com
- 📖 **文档**: 查看 KissTelegram 文件夹中所有 `README_*.md` 文件
- 🐛 **Bug 报告**: GitHub issues (主 README.md 中的链接)
- 💡 **功能请求**: 也欢迎通过电子邮件或 GitHub!

---

## 总结: 完整流程

```
1. 从 Telegram 获取 Bot Token + Chat ID ✅
2. 修改 boards.txt (添加自定义分区) ✅
3. 配置 Arduino IDE (自定义分区,启用擦除) ✅
4. 编辑 system_setup.h (凭据) ✅
5. 连接右侧 USB 端口 ✅
6. 上传 #1 (烧录 Bootloader) ✅
7. 断开右侧,连接左侧 USB 端口 ✅
8. 上传 #2 (上传 KissTelegram Sketch) ✅
9. 在 Telegram 中收到欢迎消息 ✅
10. 发送 /status 验证一切正常 ✅
```

**你已准备好使用 KissTelegram 构建精彩项目!** 🎉
