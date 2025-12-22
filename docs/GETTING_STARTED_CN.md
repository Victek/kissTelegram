# ESP32-S3上的KissTelegram入门指南

**从零开始配置ESP32-S3到发送第一条Telegram消息的完整指南**

> ⚠️ **重要提示**: 上传任何固件之前请完整阅读本指南。ESP32-S3 N16R8由于自定义分区需要**两步上传过程**。跳过步骤将导致错误!

---

## 目录

1. [开始之前](#开始之前)
2. [创建Telegram机器人](#创建telegram机器人)
3. [硬件配置](#硬件配置)
4. [Arduino IDE配置](#arduino-ide配置)
5. [首次上传(使用Arduino IDE创建分区)](#首次上传)
6. [配置文件](#配置文件)
7. [成功!接下来做什么?](#成功接下来做什么)

---

## 开始之前

### 您需要的物品

- **ESP32-S3 N16R8** (16MB Flash / 8MB PSRAM)
- **两根USB-C线** (用于在引导程序和OTG端口之间切换)
- **Arduino IDE 2.x** 或更高版本
- **Windows PC** (本指南专注于Windows,Linux/Mac请调整路径)
- **Telegram账号** 在您的手机上

### 为什么不同

您的新ESP32-S3 N16R8出厂时带有内置RGB LED演示应用。KissTelegram **完全替换分区表**以最大化您的16MB闪存:

| 分区 | Espressif默认 | KissTelegram自定义 |
|-----------|-------------------|---------------------|
| 应用空间 | 1.5 MB | 4.5 MB (3倍大!) |
| 文件系统 | 5 MB | 13 MB (2.6倍大!) |
| 总使用 | 6.5 MB | 17.5 MB |

这就是为什么需要两步上传过程:**分区表在上传之间改变**。

---

## 创建Telegram机器人

### 步骤1: 与BotFather对话

1. 在手机上打开Telegram
2. 搜索 `@BotFather` (官方机器人,有蓝色验证标记)
3. 用 `/start` 开始对话
4. 用 `/newbot` 创建您的机器人
5. 选择名称 (例如: "我的家庭助手")
6. 选择用户名 (必须以 `bot` 结尾,例如: "myhome_assistant_bot")
7. **保存您的机器人令牌** - 看起来像: `1234567890:ABCdefGHIjklMNOpqrsTUVwxyz`

### 步骤2: 获取您的聊天ID

**方法1: 使用机器人 (更简单)**

1. 在Telegram中搜索 `@ChatIDHelperBot`
2. 用 `/start` 开始对话
3. 它会回复您的 **聊天ID** (像 `123456789` 这样的数字)
4. **保存这个数字** - 配置时需要它

**方法2: 使用网页浏览器**

1. 向您新创建的机器人发送任何消息
2. 打开浏览器并访问:
   ```
   https://api.telegram.org/bot<您的机器人令牌>/getUpdates
   ```
   (将 `<您的机器人令牌>` 替换为您的实际令牌)
3. 在JSON响应中查找 `"chat":{"id":123456789`
4. 那个数字就是您的 **聊天ID**

**✅ 现在您有了:**
- 机器人令牌: `1234567890:ABCdefGHIjklMNOpqrsTUVwxyz`
- 聊天ID: `123456789`

妥善保管!您很快就会需要它们。

---

## 硬件配置

### 理解两个USB-C端口

您的ESP32-S3 N16R8有 **两个USB-C端口**:

```
┌─────────────────────┐
│  ┌─┐         ESP32  │
│  │•│  ← 电源LED     │
│  └─┘                 │
│  [USB-C]  ← 右侧端口 (引导程序/上传)
│                      │
│                      │
│  [USB-C]  ← 左侧端口 (OTG/正常操作)
│                      │
└─────────────────────┘
```

**右侧端口 (靠近电源LED):**
- 用于 **初始固件上传**
- 用于 **引导程序模式**
- 当Arduino IDE显示"连接中..."时使用此端口

**左侧端口 (OTG):**
- 用于首次上传后的 **正常操作**
- 用于 **第二次上传** (分区修正)
- 在正常操作时用于串口监视器

---

## Arduino IDE配置

### 步骤1: 显示隐藏文件 (Windows)

1. 打开 **文件资源管理器**
2. 点击 **查看** 选项卡 → **显示** → 勾选:
   - ✅ 文件扩展名
   - ✅ 隐藏的项目
3. 在 **筛选** 选项卡: **所有文件类型**

### 步骤2: 修改boards.txt

1. 导航到:
   ```
   C:\Users\<您的用户名>\AppData\Local\Arduino15\packages\esp32\hardware\esp32\3.3.4\
   ```
   (如果版本不同,将 `3.3.4` 替换为您的ESP32核心版本)

2. 找到并打开 `boards.txt` (使用Notepad++或任何文本编辑器)

3. 按 `Ctrl+F` 并搜索:
   ```
   gen4esp32_4MBapp_4MBota_7MBspiffs
   ```

4. **在该行的正下方**,粘贴这三行:
   ```
   gen4-ESP32-S3R8n16.menu.PartitionScheme.gen4esp32scheme2=Custom (4MB APP/12MB LtlFS)
   gen4-ESP32-S3R8n16.menu.PartitionScheme.gen4esp32scheme2.build.custom_partitions=partitions
   gen4-ESP32-S3R8n16.menu.PartitionScheme.gen4esp32scheme2.upload.maximum_size=4718592
   ```

5. **保存**并关闭 `boards.txt`

6. 如果Arduino IDE已打开,**关闭并重启它**

### 步骤3: 配置Arduino IDE

1. **打开**您的KissTelegram草图文件夹 (包含 `.ino`, `.h`, `.cpp`, 和 `partitions.csv`)

2. 在Arduino IDE中,转到 **工具** → **开发板** → **4D Systems gen4-ESP32-S3R8n16**

3. **配置所有工具菜单选项:**

   | 设置 | 值 |
   |---------|-------|
   | **开发板** | 4D Systems gen4-ESP32-S3R8n16 |
   | **USB CDC On Boot** | 启用 |
   | **Flash大小** | 16MB (128Mb) |
   | **分区方案** | **Custom (4MB APP/12MB LtlFS)** ⚠️ |
   | **PSRAM** | OPI PSRAM |
   | **上传速度** | 921600 |
   | **上传草图前擦除所有Flash** | **启用** ⚠️ |

   ⚠️ **关键设置** - 请仔细检查!

4. **工具** → **串口监视器** → 将速度设置为 **115200**

---

## 首次上传 (常见问题)

### 为什么需要两次上传

**问题:**
- 首次上传: Arduino使用 **旧分区表**写入固件
- ESP32启动: 找到 **新分区表** (来自 `partitions.csv`)
- 固件写入位置与ESP32查找位置之间的 **不匹配**
- 结果: 启动错误,分区错误,崩溃

**解决方案:**
两次上传确保固件写入到由新分区表定义的 **正确位置**。

---

### 上传#1: 初始刷写

1. **连接右侧USB-C端口** (靠近电源LED) 到您的PC

2. **选择端口**: 工具 → 端口 → 选择出现的COM端口

3. **验证设置**:
   - ✅ 上传草图前擦除所有Flash: **启用**
   - ✅ 分区方案: **Custom (4MB APP/12MB LtlFS)**
   

4. **工具,加载** 或 (`Ctrl+U`) (点击您喜欢的选项)
   - ✅ 固件已上传。
   - 需要53.6秒,如果为ESP32s3使用外部电源会快得多 

继续上传#2。

---

### 上传#2: 草图上传

1. **断开右侧USB-C端口**

2. **连接左侧USB-C端口** (OTG端口) 到您的PC

3. **选择新端口**: 工具 → 端口 → 选择新的COM端口
   - **重要**: 端口号会改变!在串口监视器中查找数据以确认正确端口,例如,按ESP32s3复位直到看到数据响应

4. **再次验证设置**:
   - ✅ 上传草图前擦除所有Flash: **启用**
   - ✅ 分区方案: **Custom (4MB APP/12MB LtlFS)**

5. **再次按上传** (`Ctrl+U`)

6. **等待~2-3分钟** (擦除+上传,取决于是否使用外部电源)

7. **打开串口监视器** - 您现在应该看到(如果已在system_setup.h中正确设置凭据
(重命名的system_setup_template)):
   ```
   ✅ KissTelegram v0.9.x
   ✅ WiFi已连接
   ✅ Telegram机器人已启用
   ✅ 系统就绪
   ```

8. **检查Telegram** - 您将收到欢迎消息:
   ```
   📦 您好! KissTelegram已就绪。
   🔌 构建: 2025-12-12 10:30:45 (0xABCD1234)
   📡 WiFi信号: -59 dBm (良好)
   ✅ 0条消息排队
   ```

**成功!** 您的ESP32-S3现在运行具有正确分区的KissTelegram。

---

### 未来的上传

**好消息:** 两次初始上传后,所有未来的上传都正常工作:

- 使用 **左侧USB-C端口** (OTG)
- **不再需要** "擦除所有Flash" (除非您对NVRAM数据进行了更改)
- 上传一次即可立即工作

---

## 配置文件

### system_setup.h (首次上传前必需!)

**编译前:**

1. 导航到您的KissTelegram文件夹
2. 找到 `system_setup_template.h`
3. **重命名**为 `system_setup.h`
4. **打开** `system_setup.h` 并填写:

```cpp
// 您的Telegram机器人 (来自BotFather)
#define KISS_FALLBACK_BOT_TOKEN "1234567890:ABCdefGHIjklMNOpqrsTUVwxyz"

// 您的聊天ID (来自@userinfobot)
#define KISS_FALLBACK_CHAT_ID "123456789"

// 您的WiFi凭据
#define KISS_FALLBACK_WIFI_SSID "您的WiFi名称"
#define KISS_FALLBACK_WIFI_PASSWORD "您的WiFi密码"

// OTA安全性 (更改默认PIN/PUK!)
#define KISS_FALLBACK_OTA_PIN "0000"        // 4位数字
#define KISS_FALLBACK_OTA_PUK "00000000"    // 8位数字
```

5. **保存**文件

**⚠️ 安全警告:** 将默认PIN (`0000`) 和PUK (`00000000`) 更改为您自己的密码!

---

### lang.h (可选: 选择您的语言)

KissTelegram支持7种系统消息语言:

```cpp
// 在lang.h中,取消注释一种语言:

// #define LANG_CN  // 中文 (中文)
// #define LANG_DE  // Deutsch (德语)
// #define LANG_EN  // English (英语)
// #define LANG_FR  // Français (法语)
// #define LANG_IT  // Italiano (意大利语)
// #define LANG_PT  // Português (葡萄牙语)
// #define LANG_ES  // Español (西班牙语) - 如果全部注释则为默认
```

在编译前选择您的语言(取消注释)以获得本地化消息。

---

## 成功!接下来做什么?

### 验证一切正常

1. **在Telegram中向您的机器人发送 `/status`** - 您将获得详细的状态报告:
   ```
   📦 KissTelegram v1.x.x
   🎯 系统可靠性
   ✅ 系统: 可靠
   ✅ 已发送消息: 2
   💾 待处理消息: 0
   📡 WiFi信号: -59 dBm (良好)
   🔋 运行时间: 123秒
   💾 可用内存: 223 KB
   ```

2. **检查串口监视器** - 不应显示错误

3. **测试命令**:
   - `/start` - 欢迎消息
   - `/help` - 可用命令
   - `/status` - 系统状态(健康检查)

---

### 理解OTA更新

一旦KissTelegram运行,您可以**通过Telegram更新固件**(无需USB线!):

1. 向您的机器人发送 `/ota`
2. 输入PIN: `/otapin 0000` (或您的自定义PIN)
3. **发送您的固件 `.bin` 文件** (在Telegram中拖放)
4. 机器人自动验证校验和
5. 确认: `/otaconfirm`
6. ESP32使用新固件重启
7. **在60秒内**,发送 `/otaok` 确认它工作
8. 如果不确认,ESP32 **自动回滚**到之前的固件!

📖 **了解更多:** 参见 `README_KissOTA_CN.md` 获取完整OTA文档。

---

### 探索示例代码

`suite_kiss.ino` 示例演示:

- ✅ WiFi管理与质量监控
- ✅ 带优先级的消息队列
- ✅ 电源管理模式
- ✅ 命令处理 (`/start`, `/help`, `/status`, 等)
- ✅ 通过Telegram进行OTA更新
- ✅ 崩溃恢复和持久性
- ✅ 安全SSL/TLS连接

**专业提示:** 使用 `/status` 命令作为您的 **健康监控工具** - 这是您了解KissTelegram内部情况的窗口!

---

### 常见故障排除

**问题: "未找到端口"或"拒绝访问"**
- Windows锁定了端口。断开USB,等待5秒,重新连接。
- 尝试不同的USB线(有些只能充电,不能传输数据)

**问题: 上传时"等待设备超时"**
- USB端口错误!记住: 首次上传用右侧端口,第二次用左侧端口
- 点击上传时按住ESP32上的BOOT按钮,出现"连接中..."后松开

**问题: 串口监视器显示乱码字符**
- 波特率错误。在串口监视器下拉菜单中设置为 **115200**

**问题: 机器人在Telegram中没有响应**
- 验证 `system_setup.h` 有正确的机器人令牌和聊天ID
- 验证WiFi凭据正确
- 打开串口监视器并查找WiFi连接消息

**问题: 编译错误"分区表不适合"**
- 未正确将自定义分区添加到 `boards.txt`
- 或未在 工具 → 分区方案 中选择 "Custom (4MB APP/12MB LtlFS)"

---

### 获取更多帮助

- 📧 **邮箱**: victek@gmail.com
- 📖 **文档**: 查看KissTelegram文件夹中的所有 `README_*.md` 文件
- 🐛 **错误报告**: GitHub问题(主README.md中的链接)
- 💡 **功能请求**: 也欢迎通过邮箱或GitHub!

---

## 总结: 完整过程

```
1. 从Telegram获取机器人令牌 + 聊天ID ✅
2. 修改boards.txt (添加自定义分区) ✅
3. 配置Arduino IDE (自定义分区,启用擦除) ✅
4. 编辑system_setup.h (凭据) ✅
5. 连接右侧USB端口 ✅
6. 上传#1 (新分区)✅
7. 断开右侧,连接左侧USB端口 ✅
8. 上传#2 (上传KissTelegram草图) ✅
9. 在Telegram中收到欢迎消息 ✅
10. 发送/status验证一切正常 ✅
```

**您已准备好使用KissTelegram构建令人惊叹的项目!** 🎉
