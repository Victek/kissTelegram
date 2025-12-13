# KissOTA - ESP32 OTA更新系统

## 目录
- [概述](#概述)
- [主要特性](#主要特性)
- [与其他OTA系统的比较](#与其他ota系统的比较)
- [系统架构](#系统架构)
- [相对于Espressif标准OTA的优势](#相对于espressif标准ota的优势)
- [硬件要求](#硬件要求)
- [分区配置](#分区配置)
- [OTA流程](#ota流程)
- [可用命令](#可用命令)
- [安全特性](#安全特性)
- [错误管理和恢复](#错误管理和恢复)
- [基本使用](#基本使用)
- [常见问题 (FAQ)](#常见问题-faq)

---

## 概述

KissOTA是一个专为带有PSRAM的ESP32-S3设备设计的高级空中更新(OTA)系统。与Espressif的标准OTA系统不同，KissOTA提供多层安全、验证和自动恢复功能，所有这些都与Telegram集成以实现安全的远程更新。

**作者：** Vicente Soriano (victek@gmail.com)
**当前版本：** 0.9.0
**许可证：** 包含在KissTelegram Suite中

---

## 主要特性

### 🔒 强大的安全性
- **PIN/PUK认证**：通过PIN和PUK代码进行访问控制
- **自动锁定**：PIN错误3次后自动锁定
- **CRC32验证**：刷写前验证固件完整性
- **刷写后验证**：需要用户手动确认

### 🛡️ 故障保护
- **自动备份**：更新前备份当前固件
- **自动回滚**：如果新固件失败则恢复
- **启动循环检测**：如果在5分钟内失败3次，自动回滚
- **factory分区**：如果所有其他措施失败的最终安全网
- **全局超时**：7分钟完成整个过程

### 💾 资源高效使用
- **PSRAM缓冲区**：使用7-8MB PSRAM下载而不触及闪存
- **节省空间**：只需要factory + app分区（无需OTA_0/OTA_1）
- **自动清理**：删除临时文件和旧备份

### 📱 Telegram集成
- **远程更新**：直接通过Telegram发送.bin文件
- **实时反馈**：进度和状态消息
- **多语言**：支持7种语言（ES、EN、FR、IT、DE、PT、CN）

---

## 与其他OTA系统的比较

### KissOTA与其他流行解决方案

| 特性 | KissOTA | AsyncElegantOTA | ArduinoOTA | ESP-IDF OTA | ElegantOTA |
|----------------|---------|-----------------|------------|-------------|------------|
| **传输方式** | 📱 Telegram | 🌐 HTTP Web | 🌐 HTTP Web | 🌐 HTTP | 🌐 HTTP Web |
| **远程访问** | ✅ 无需配置全球访问 | ❌ 仅限局域网* | ❌ 仅限局域网* | ❌ 仅限局域网* | ❌ 仅限局域网* |
| **需要已知IP** | ❌ 否 | ✅ 是 | ✅ 是 | ✅ 是 | ✅ 是 |
| **端口转发** | ❌ 不需要 | ⚠️ 远程访问需要 | ⚠️ 远程访问需要 | ⚠️ 远程访问需要 | ⚠️ 远程访问需要 |
| **Web服务器** | ❌ 否 | ✅ AsyncWebServer | ✅ WebServer | ✅ 可配置 | ✅ WebServer |
| **认证** | 🔒 PIN/PUK（强大） | ⚠️ 基本用户名/密码 | ⚠️ 可选密码 | ⚠️ 基本 | ⚠️ 基本用户名/密码 |
| **固件备份** | ✅ 在LittleFS中 | ❌ 否 | ❌ 否 | ❌ 否 | ❌ 否 |
| **自动回滚** | ✅✅✅ 3个级别 | ⚠️ 有限（2个分区） | ❌ 否 | ⚠️ 有限（2个分区） | ⚠️ 有限（2个分区） |
| **手动验证** | ✅ 60秒内用/otaok | ❌ 否 | ❌ 否 | ⚠️ 可选 | ❌ 否 |
| **启动循环检测** | ✅ 自动 | ❌ 否 | ❌ 否 | ❌ 否 | ❌ 否 |
| **下载缓冲区** | 💾 PSRAM (8MB) | 🔥 Flash | 🔥 Flash | 🔥 Flash | 🔥 Flash |
| **实时进度** | ✅ Telegram消息 | ✅ Web UI | ⚠️ 仅串口 | ⚠️ 可配置 | ✅ Web UI |
| **用户界面** | 📱 Telegram聊天 | 🖥️ 网页浏览器 | 🖥️ 网页浏览器 | ⚡ 编程式 | 🖥️ 网页浏览器 |
| **依赖项** | KissTelegram | ESPAsyncWebServer | ESP mDNS | 无（原生） | WebServer |
| **所需闪存** | ~3.5 MB (app) | ~7 MB (OTA_0+1) | ~7 MB (OTA_0+1) | ~7 MB (OTA_0+1) | ~7 MB (OTA_0+1) |
| **互联网安全** | ✅ 高（Telegram API） | ⚠️ 暴露时易受攻击 | ⚠️ 暴露时易受攻击 | ⚠️ 暴露时易受攻击 | ⚠️ 暴露时易受攻击 |
| **易用性** | ✅✅ 只需发送.bin | ✅ 直观的Web UI | ⚠️ 需要配置 | ⚠️ 复杂度高 | ✅ 直观的Web UI |
| **多语言** | ✅ 7种语言 | ❌ 仅英语 | ❌ 仅英语 | ❌ 仅英语 | ❌ 仅英语 |
| **Factory恢复** | ✅ 是 | ❌ 否 | ❌ 否 | ⚠️ 手动 | ❌ 否 |

\* *使用端口转发/VPN可以实现远程访问，但需要高级网络配置*

### KissOTA的独特优势

#### 🌍 **真正的全球访问**
其他OTA系统需要：
- 知道设备的IP地址
- 在同一局域网中，或
- 配置端口转发（有风险），或
- 配置VPN（复杂）

**KissOTA：** 只需要Telegram。从世界任何地方更新而无需网络配置。因为它集成在KissTelegram中，使用SSL连接。

#### 🔒 **不妥协的安全性**
将HTTP Web服务器暴露到互联网是危险的：
- 容易受到暴力破解攻击
- 可能的Web漏洞利用向量
- 需要HTTPS才能安全（证书等）

**KissOTA：** 使用Telegram的安全基础设施。您的ESP32从不向外部暴露端口。

#### 🛡️ **多级恢复**
其他具有回滚功能的系统（ESP-IDF、AsyncElegantOTA）只有2个分区：
- 如果两个分区都失败 → 设备"变砖"
- 没有功能固件的备份

**KissOTA：** 3个安全级别：
1. **级别1：** 从LittleFS中的备份回滚
2. **级别2：** 从原始factory分区启动
3. **级别3：** 自动启动循环检测

#### 💾 **节省闪存空间**
传统系统（双bank OTA）：
```
Factory:  3.5 MB  ┐
OTA_0:    3.5 MB  ├─ 最少需要7 MB
OTA_1:    3.5 MB  ┘
总计: 10.5 MB
```

**KissOTA（单bank + 备份）：**
```
Factory:  3.5 MB  ┐
App:      3.5 MB  ├─ 总计7 MB
Backup:   ~1.1 MB │  (在LittleFS中，可压缩)
总计: ~8.1 MB   ┘
```
**节省：** 约2.4 MB的闪存空间用于您的应用程序或数据。

#### 🚀 **PSRAM作为缓冲区**
其他系统直接下载到闪存：
- **闪存磨损：** 闪存的写入周期有限（约100K）
- **风险：** 如果下载失败，闪存已被部分写入

**KissOTA：**
- 首先完整下载到PSRAM
- 在PSRAM中验证CRC32
- 只有在一切正常时才写入闪存
- **PSRAM没有磨损：** 无限写入周期

---

## 系统架构

### 分区结构

```
┌─────────────────────────────────────┐
│  Factory Partition (3.5 MB)        │ ← 原始出厂固件
│  - 最终安全网                       │
│  - 生产中只读                       │
└─────────────────────────────────────┘
┌─────────────────────────────────────┐
│  App Partition (3.5 MB)             │ ← 运行中的固件
│  - 当前活动固件                     │
│  - OTA期间更新                      │
└─────────────────────────────────────┘
┌─────────────────────────────────────┐
│  LittleFS (剩余约8-9 MB)            │
│  - /ota_backup.bin (备份)           │ ← 先前固件的备份
│  - 配置文件                         │
│  - 持久化数据                       │
└─────────────────────────────────────┘
┌─────────────────────────────────────┐
│  NVS (非易失性存储)                 │
│  - Boot flags (验证)                │ ← OTA验证状态
│  - PIN/PUK凭证                      │
│  - 启动计数器                       │
└─────────────────────────────────────┘
┌─────────────────────────────────────┐
│  PSRAM (7-8 MB)                     │
│  - 临时下载缓冲区                   │ ← 刷写前下载的固件
│  - 非持久化（会清除）               │
└─────────────────────────────────────┘
```

### OTA期间的数据流

```
Telegram API
    │
    ▼
[下载到PSRAM] ← 7-8 MB临时缓冲区
    │
    ▼
[CRC32验证]
    │
    ▼
[备份当前固件 → LittleFS] ← /ota_backup.bin
    │
    ▼
[刷写新固件 → App Partition]
    │
    ▼
[ESP32重启]
    │
    ▼
[60秒验证] → /otaok或自动回滚
    │
    ▼
[删除旧固件] -> 恢复旧固件的空间

```
---

## 相对于Espressif标准OTA的优势

| 特性 | Espressif OTA | KissOTA |
|----------------|---------------|---------|
| **所需闪存空间** | ~7 MB (OTA_0 + OTA_1) | ~3.5 MB (仅app) |
| **固件备份** | ❌ 否 | ✅ 是，在LittleFS中 |
| **自动回滚** | ⚠️ 有限 | ✅ 完整 + factory |
| **手动验证** | ❌ 否 | ✅ 是，60秒 |
| **认证** | ❌ 否 | ✅ PIN/PUK |
| **启动循环检测** | ❌ 否 | ✅ 是，自动 |
| **Telegram集成** | ❌ 否 | ✅ 原生 |
| **下载缓冲区** | Flash | PSRAM（不磨损闪存） |
| **紧急恢复** | ⚠️ 仅2个分区 | ✅ 3个级别（app/backup/factory） |

---

## 硬件要求

### 最低要求
- **MCU**：ESP32-S3（其他ESP32变体可能需要适配才能工作）
- **PSRAM**：最少取决于要替换的固件大小2-8MB PSRAM（用于下载缓冲区）
- **Flash**：最少取决于要替换的固件大小8-16-32MB
- **连接性**：功能正常的WiFi

### 推荐配置
- **ESP32-S3-WROOM-1-N16R8**：16MB Flash + 8MB PSRAM（理想）
- **ESP32-S3-DevKitC-1**：带N16R8或更高模块

---

## 分区配置

推荐的`partitions.csv`文件（N16R8）：

```csv

# Name,   Type, SubType, Offset,  Size
nvs,      data, nvs,     0x9000,  0x4000,
otadata,  data, ota,     0xd000,  0x2000,
app0,     app,  factory, 0x10000, 0x180000,
app1,     app,  ota_0,   0x190000,0x180000,
spiffs,   data, spiffs,  0x310000,0xCF0000,
```

**Arduino IDE配置：**
- Tools → Partition Scheme → Custom
- 指向`partitions.csv`文件

---

## OTA流程

### 状态图

```
┌─────────────┐
│  OTA_IDLE   │ ← 初始状态
└──────┬──────┘
       │ /ota
       ▼
┌─────────────┐
│ WAIT_PIN    │ ← 请求PIN（3次尝试）
└──────┬──────┘
       │ PIN正确
       ▼
┌──────────────┐
│ AUTHENTICATED│ ← PIN正确，准备接收.bin
└──────┬───────┘
       │ 用户发送.bin
       ▼
┌──────────────┐
│ DOWNLOADING  │ ← 下载到PSRAM（实时进度）
└──────┬───────┘
       │
       ▼
┌────────────────┐
│ VERIFY_CHECKSUM│ ← 计算CRC32
└──────┬─────────┘
       │
       ▼
┌──────────────┐
│ WAIT_CONFIRM │ ← 等待用户的/otaconfirm
└──────┬───────┘
       │ /otaconfirm
       ▼
┌──────────────┐
│ BACKUP_CURRENT│ ← 将当前固件保存到LittleFS
└──────┬───────┘
       │
       ▼
┌──────────────┐
│  FLASHING    │ ← 从PSRAM写入新固件
└──────┬───────┘
       │
       ▼
┌──────────────┐
│   REBOOT     │ ← 重启ESP32
└──────┬───────┘
       │
       ▼
┌──────────────┐
│  VALIDATING  │ ← 60秒内发送/otaok
└──────┬───────┘
       │ /otaok
       ▼
┌──────────────┐
│  COMPLETE    │ ← 固件已验证 ✅
└──────────────┘
       │ 超时或/otacancel
       ▼
┌──────────────┐
│  ROLLBACK    │ ← 自动恢复备份
└──────────────┘
```

### 重要的超时

- **PIN认证**：无限制（最多3次尝试），如果失败，PIN被锁定，需要PUK来恢复
- **.bin接收**：等待最多7分钟，如果超时取消/ota
- **确认**：等待最多7分钟，如果超时取消/ota（等待/otaconfirm）
- **完整过程**：从/otaconfirm开始最多7分钟
- **刷写后验证**：60秒内发送/otaok

---

## 可用命令

### `/ota`
启动OTA过程。

**用法：**
```
/ota
```

**响应：**
- 如果PIN未锁定：请求PIN
- 如果PIN已锁定：请求PUK

---

### `/otapin <代码>`
发送PIN代码（4-8位数字）。

**用法：**
```
/otapin 0000 (默认)
```

**响应：**
- ✅ PIN正确：状态AUTHENTICATED，准备接收.bin
- ❌ PIN错误：减少剩余尝试次数
- 🔒 3次失败后：锁定并请求PUK

---

### `/otapuk <代码>`
使用PUK代码解锁系统。

**用法：**
```
/otapuk 12345678
```

**响应：**
- ✅ PUK正确：解锁PIN并请求新PIN
- ❌ PUK错误：保持锁定

---

### `/otaconfirm`
确认您要刷写下载的固件。

**用法：**
```
/otaconfirm
```

**前提条件：**
- 固件已下载并验证
- CRC32校验和正常

**操作：**
- 创建当前固件的备份
- 刷写新固件
- 重启ESP32

---

### `/otaok`
验证新固件正常工作。

**用法：**
```
/otaok
```

**前提条件：**
- 必须在重启后60秒内发送
- 仅在OTA刷写后可用

**操作：**
- 将固件标记为有效
- 删除之前的备份
- 系统恢复正常操作

⚠️ **重要**：如果60秒内未发送`/otaok`，将执行自动回滚。

---

### `/otacancel`
取消OTA过程或强制回滚。

**用法：**
```
/otacancel
```

**行为：**
- 下载/验证期间：取消并清理临时文件
- 刷写后验证期间：立即执行回滚
- 如果没有活动的OTA：通知没有活动的过程

---

## 安全特性

### 1. PIN/PUK认证

#### PIN和PUK（可以使用/changepin <旧> <新>或/changepuk <旧> <新>远程更改）

#### PIN（个人识别码）
- **长度**：4-8位数字
- **尝试次数**：锁定前3次尝试
- **配置**：在`system_setup.h`凭证回退中定义
- **持久化**：安全保存在NVS中

#### PUK（PIN解锁密钥）
- **长度**：8位数字
- **功能**：在3次PIN失败后解锁
- **安全性**：只有管理员应该知道

**锁定流程示例：**
```
尝试1：PIN错误 → "❌ PIN错误。剩余2次尝试"
尝试2：PIN错误 → "❌ PIN错误。剩余1次尝试"
尝试3：PIN错误 → "🔒 PIN已锁定。使用/otapuk [代码]"
```

### 2. 完整性验证

#### CRC32校验和
- **算法**：CRC32 IEEE 802.3
- **计算**：对整个下载的.bin文件
- **验证**：在允许/otaconfirm之前
- **拒绝**：如果CRC32不匹配，删除下载

**输出示例：**
```
🔍 验证校验和...
🔍 CRC32: 0xF8CAACF6 (已验证1.07 MB)
✅ 校验和正常
```

### 3. 刷写后验证

#### 验证窗口（60秒）
刷写新固件后：
1. ESP32重启
2. Boot flags标记`otaInProgress = true`
3. 用户有60秒发送`/otaok`
4. 如果不响应 → 自动回滚

**时间图：**
```
刷写 → 重启 → [60秒内发送/otaok] → 超时则回滚
                         ↓
                      /otaok → ✅ 固件已验证
```

### 4. 防止未经授权的修改

- **维护模式**：OTA期间，其他命令受限
- **唯一的Chat ID**：只有配置的chat_id可以执行OTA
- **预先认证**：任何操作前必须输入PIN/PUK

---

## 错误管理和恢复

### 恢复级别

#### 级别1：自动重试
**场景：**
- 从Telegram下载失败（最多3次重试）
- 网络超时
- 下载中断

**操作：**
- 清除PSRAM
- 自动重试下载
- 通知用户尝试

---

#### 级别2：从备份回滚
**场景：**
- 用户在60秒内未发送`/otaok`
- 用户在验证期间发送`/otacancel`
- 检测到启动循环（5分钟内3次以上启动）

**回滚过程：**
1. 检测到`bootFlags.otaInProgress == true`
2. 读取`bootFlags.backupPath` → `/ota_backup.bin`
3. 从LittleFS恢复备份 → App Partition
4. 重启ESP32
5. 清除boot flags
6. 通过Telegram通知用户

**示例代码：**
```cpp
bool KissOTA::restoreFromBackup() {
  if (strlen(bootFlags.backupPath) == 0) {
    return false; // 没有备份
  }

  // 从LittleFS读取/ota_backup.bin
  // 写入App Partition
  // 重启
}
```

---

#### 级别3：回退到Factory
**场景：**
- 从备份回滚失败
- `/ota_backup.bin`文件损坏
- 闪存中的严重错误

**过程：**
1. `esp_ota_set_boot_partition(factory_partition)`
2. 重启ESP32
3. 从原始出厂固件启动
4. 通知用户严重错误

⚠️ **重要**：这是最后的手段。factory固件必须稳定，在生产中永远不应修改。

---

### 启动循环检测

**算法：**
```cpp
bool KissOTA::checkBootLoop() {
  if (bootFlags.bootCount > 3) {
    unsigned long timeSinceLastBoot = millis() - bootFlags.lastBootTime;
    if (timeSinceLastBoot < 300000) {  // 5分钟
      KISS_CRITICAL("🔥 启动循环：5分钟内3次以上启动");
      return true; // 执行回滚
    }
  }
  return false;
}
```

**保护：**
- 每次启动时递增`bootFlags.bootCount`
- 如果在< 5分钟内> 3次启动 → 自动回滚
- 使用`/otaok`验证时，重置计数器

---

### 错误状态

| 错误 | 代码 | 自动操作 |
|-------|--------|-------------------|
| **下载失败** | `DOWNLOAD_FAILED` | 重试最多3次 |
| **校验和错误** | `CHECKSUM_MISMATCH` | 删除下载，取消OTA |
| **备份失败** | `BACKUP_FAILED` | 取消OTA，不冒险 |
| **刷写失败** | `FLASH_FAILED` | 取消OTA，保持当前固件 |
| **验证超时** | `VALIDATION_TIMEOUT` | 自动回滚 |
| **启动循环** | `BOOT_LOOP_DETECTED` | 自动回滚 |
| **回滚失败** | `ROLLBACK_FAILED` | 回退到factory |
| **无备份** | `NO_BACKUP` | 保持当前固件，警告 |

---

## 基本使用

### 完整的逐步更新

#### 1. 准备固件
```bash
# 在Arduino IDE中编译项目
# .bin生成在：build/esp32.esp32.xxx/suite_kiss.ino.bin
```

#### 2. 启动OTA
从Telegram：
```
/ota
```

机器人响应：
```
🔐 OTA认证

输入4-8位数字的PIN代码：
/otapin [代码]

剩余尝试次数：3
```

#### 3. 使用PIN认证
```
/otapin 0000
```

响应：
```
✅ PIN正确

系统准备好OTA。
发送新固件的.bin文件。
```

#### 4. 发送固件
- 将`.bin`文件作为文档附加（不是照片）
- Telegram上传它，机器人自动下载

下载期间的响应：
```
📥 正在下载固件到PSRAM...
⏳ 进度：45%
```

#### 5. 自动验证
```
✅ 下载完成：PSRAM中1.07 MB
🔍 验证校验和...
✅ CRC32: 0xF8CAACF6

📋 固件已验证

文件：suite_kiss.ino.bin
大小：1.07 MB
CRC32: 0xF8CAACF6

⚠️ 需要确认
刷写固件：
/otaconfirm

取消：
/otacancel
```

#### 6. 确认刷写
```
/otaconfirm
```

响应：
```
💾 开始备份...
✅ 备份完成：1123456字节

⚡ 正在刷写固件...
✅ 刷写完成

固件已正确写入。
设备现在将重启。

重启后您将有60秒使用/otaok验证
如果不验证将执行自动回滚。
```

#### 7. 重启后验证
重启后（在60秒内）：
```
/otaok
```

响应：
```
✅ 固件已验证

新固件已确认。
系统恢复正常操作。

版本：0.9.0
```

---

### 手动回滚示例

如果新固件不能正常工作：

```
/otacancel
```

响应：
```
⚠️ 执行回滚

从备份恢复先前的固件...
✅ 先前的固件已恢复
🔄 重启中...

[重启后]
✅ 回滚完成

系统已恢复到先前的固件。
```

---

## 高级配置

### 自定义超时

在`KissOTA.h`中：

```cpp
// 验证超时（默认60秒）
static const int BOOT_VALIDATION_TIMEOUT = 60000;

// OTA过程的全局超时（默认7分钟）
static const unsigned long OTA_GLOBAL_TIMEOUT = 420000;
```

### OTA期间启用/禁用WDT

在`system_setup.h`中：

```cpp
// 在关键操作期间禁用WDT
#ifdef KISS_USE_RTOS
  KISS_PAUSE_WDT();  // 暂停看门狗
  // ... OTA操作 ...
  KISS_INIT_WDT();   // 重新激活看门狗
#endif
```

### 更改PSRAM缓冲区大小

在`KissOTA.cpp`中：

```cpp
bool KissOTA::initPSRAMBuffer() {
  psramBufferSize = 8 * 1024 * 1024;  // 默认8 MB
  // 根据ESP32上可用的PSRAM调整
}
```

---
### 更改默认PIN/PUK

在`system_setup.h`中：

```cpp
#define KISS_FALLBACK_OTA_PIN "0000"
#define KISS_FALLBACK_OTA_PUK "00000000"
```
---

## 故障排除

### 错误："❌ 没有可用的PSRAM"
**原因：** ESP32没有PSRAM或PSRAM未启用

**解决方案：**
1. 验证ESP32-S3具有物理PSRAM
2. 在Arduino IDE中：Tools → PSRAM → "OPI PSRAM"
3. 重新编译项目

---

### 错误："❌ 创建备份时出错"
**原因：** LittleFS没有空间或未挂载

**解决方案：**
1. 验证`partitions.csv`中的`spiffs`分区
2. 如有必要格式化LittleFS
3. 增加spiffs分区大小

---

### 错误："🔥 启动循环：5分钟内3次以上启动"
**原因：** 新固件持续失败

**解决方案：**
- 自动：回滚自动执行
- 手动：等待自动回滚
- 预防：在OTA之前在另一个ESP32上测试固件

---

### /otaok后固件未验证
**原因：** 60秒超时已过期

**解决方案：**
- 重启后留出更多时间发送`/otaok`以允许稳定连接
- 验证重启后的WiFi连接
- 如有必要增加`BOOT_VALIDATION_TIMEOUT`

---

## 集成示例代码

### 在`suite_kiss.ino`中初始化

```cpp
#include "KissOTA.h"

KissTelegram* bot;
KissCredentials* credentials;
KissOTA* ota;

void setup() {
  // 初始化凭证
  credentials = new KissCredentials();
  credentials->begin();

  // 初始化Telegram机器人
  bot = new KissTelegram(BOT_TOKEN);

  // 初始化OTA
  ota = new KissOTA(bot, credentials);

  // 检查我们是否来自中断的OTA
  if (ota->isFirstBootAfterOTA()) {
    ota->validateBootAfterOTA();
  }
}

void loop() {
  // 处理Telegram命令
  if (bot->getUpdates()) {
    for (int i = 0; i < bot->message_count; i++) {
      String command = bot->messages[i].text;

      if (command.startsWith("/ota")) {
        ota->handleOTACommand(command.c_str(), "");
      }
    }
  }

  // OTA循环（管理超时和状态）
  ota->loop();
}
```

---

## 其他技术信息

### Boot Flags格式（NVS）

```cpp
struct BootFlags {
  uint32_t magic;              // 0xCAFEBABE (验证)
  uint32_t bootCount;          // 启动计数器
  uint32_t lastBootTime;       // 上次启动时间戳
  bool otaInProgress;          // 如果等待验证为true
  bool firmwareValid;          // 如果固件已验证为true
  char backupPath[64];         // 备份路径 (/ota_backup.bin)
};
```

### 典型大小

| 元素 | 典型大小 |
|----------|---------------|
| 编译的固件 | 1.0 - 1.5 MB |
| LittleFS中的备份 | ~1.1 MB |
| PSRAM缓冲区 | 7-8 MB |
| Factory分区 | 3.5 MB |
| App分区 | 3.5 MB |

---

## 贡献和支持

**作者：** Vicente Soriano
**邮箱：** victek@gmail.com
**项目：** KissTelegram Suite

要报告错误或请求功能，请联系作者。

---

## 常见问题 (FAQ)

### 为什么不使用AsyncElegantOTA或ArduinoOTA？

**简短回答：** KissOTA不需要网络配置，从一开始就可以在全球范围内工作。

**完整回答：**

AsyncElegantOTA和ArduinoOTA非常适合本地开发，但在生产中有限制：

1. **远程访问复杂：**
   - 您需要知道设备的IP地址
   - 如果在路由器/NAT后面，需要端口转发
   - 端口转发将您的ESP32暴露到互联网（安全风险）
   - 替代方案：VPN（对最终用户来说配置复杂）

2. **安全性有限：**
   - 基本用户名/密码（容易受到暴力破解）
   - 没有加密的HTTP（除非使用证书配置HTTPS）
   - 暴露的Web服务器 = 广泛的攻击面

3. **没有真正的回滚：**
   - 只有2个分区（OTA_0和OTA_1）
   - 如果两者都失败，设备"变砖"
   - 没有已知功能固件的备份

**KissOTA解决了这个问题：**
- ✅ 无需配置任何内容的全球更新（Telegram API）
- ✅ 强大的安全性（PIN/PUK + Telegram基础设施）
- ✅ 多级回滚（backup + factory + 启动循环检测）
- ✅ 不向外部暴露端口

**何时使用每个：**
- **AsyncElegantOTA：** 本地开发、快速原型制作、私有局域网
- **KissOTA：** 生产、全球远程设备、关键安全性

---

### 没有PSRAM可以工作吗？

**回答：** 当前版本的KissOTA **需要PSRAM** 用于下载缓冲区和文件有效性验证。

**技术原因：**
- 典型固件：1-1.5 MB
- PSRAM缓冲区：7-8 MB可用
- ESP32-S3的普通RAM：只有约70-105 KB空闲

**如果您没有PSRAM的替代方案：**
1. **首先下载到LittleFS：**
   - 更慢（闪存比PSRAM慢）
   - 不必要地磨损闪存
   - 需要LittleFS中的可用空间

2. **使用AsyncElegantOTA：**
   - 不需要PSRAM
   - 直接下载到OTA分区

3. **贡献PR：**
   - 如果您为没有PSRAM的ESP32实现支持，欢迎PR

**推荐硬件：**
- ESP32-S3-WROOM-1-N16R8（16MB Flash + 8MB PSRAM） - 约3-6欧元
- ESP32-S3-DevKitC-1带N16R8模块

---

### 我可以不使用Telegram使用KissOTA吗？

**回答：** 技术上可以，但需要适配工作。

KissOTA的架构有两层：

1. **核心OTA（后端）：**
   - 状态管理
   - 备份/回滚
   - 从PSRAM刷写
   - CRC32验证
   - Boot flags
   - **这部分独立于传输**

2. **Telegram前端：**
   - PIN/PUK认证
   - 从Telegram API下载文件
   - 向用户发送进度消息
   - **这部分依赖于KissTelegram**

**使用其他传输（HTTP、MQTT、Serial等）：**

```cpp
// 您需要实现自己的前端
class KissOTACustom : public KissOTACore {
public:
  // 实现抽象方法：
  virtual bool downloadFirmware(const char* source) override;
  virtual void sendProgress(int percentage) override;
  virtual void sendMessage(const char* text) override;
};
```

**值得努力吗？**
- 如果您已经有KissTelegram：**不，按原样使用KissOTA**
- 如果您不想要Telegram：**可能更好使用AsyncElegantOTA**
- 如果您有特定用例（例如：工业MQTT）：**KissTelegram有企业版，请询问**

---

### 如果在下载期间失去WiFi连接会怎样？

**回答：** 系统安全地处理断开连接。

**场景：**

**1. PSRAM下载中断：**
```
状态：DOWNLOADING → 网络超时
自动操作：
  1. 检测超时（30秒无数据后）
  2. 清除PSRAM缓冲区
  3. 重试下载（最多3次尝试）
  4. 如果3次失败：取消OTA，返回IDLE
  5. 当前固件未触及（安全）
```

**2. 刷写期间断开连接：**
```
状态：FLASHING → WiFi断开
结果：
  - 刷写继续（不依赖WiFi）
  - 数据已在PSRAM中
  - 刷写正常完成
  - 重启时，将等待/otaok（60秒）
  - 如果没有WiFi发送/otaok：自动回滚
```

**3. 验证期间断开连接：**
```
状态：VALIDATING → 没有WiFi
结果：
  - 60秒计时器运行
  - 如果WiFi在60秒前返回：您可以发送/otaok
  - 如果60秒过去没有/otaok：自动回滚
  - 系统返回到先前的固件（安全）
```

**全局超时：** 从/otaconfirm到完成所有操作7分钟。如果超过，OTA自动取消。

---

### 刷写比当前固件小的固件安全吗？

**回答：** 是的，完全安全。

**技术解释：**

刷写过程总是：
1. **在写入之前完全擦除目标分区**
2. **写入新固件**（无论大小如何）
3. **在分区元数据中标记实际大小**

**示例：**
```
当前固件：1.5 MB
新固件：0.9 MB

过程：
1. 1.5 MB备份 → LittleFS
2. 擦除app分区（完整3.5 MB）
3. 写入新的0.9 MB
4. 元数据：size = 0.9 MB
5. 分区中的剩余空间：空/已擦除
```

**没有"残留垃圾"问题**因为：
- ESP32只执行到标记的固件末尾
- 分区的其余部分已擦除
- CRC32仅在实际大小上计算

**常见用例：**
- 禁用功能以减小大小
- 最小紧急固件（约500 KB）
- 使用不同标志的优化编译

---

### 如果一切都失败了，我该如何重置系统？

**回答：** 您有3个恢复选项。

#### 选项1：自动回滚（最常见）
如果新固件失败，只需**什么都不做**：
```
1. 新固件启动但失败
2. ESP32重启（崩溃/看门狗）
3. NVS中的计数器bootCount++
4. 5分钟内3次重启后 → 自动回滚
5. 系统从/ota_backup.bin恢复先前的固件
```

#### 选项2：手动回滚
如果您可以访问Telegram：
```
/otacancel
```
立即从备份强制回滚。

#### 选项3：在Factory中启动（紧急）
如果所有其他措施都失败：

**通过串口（需要物理访问）：**
```cpp
// 在setup()中，检测紧急情况
if (digitalRead(EMERGENCY_PIN) == LOW) {  // 引脚接地
  esp_ota_set_boot_partition(esp_partition_find_first(
    ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_FACTORY, NULL
  ));
  ESP.restart();
}
```

**通过esptool（最后手段）：**
```bash
# 使用原始固件刷写factory分区
esptool.py --port COM13 write_flash 0x10000 firmware_factory.bin
```

#### 选项4：完全刷写（全新开始）
```bash
# 擦除所有闪存
esptool.py --port COM13 erase_flash

# 刷写全新固件 + 分区
esptool.py --port COM13 write_flash 0x0 bootloader.bin 0x8000 partitions.bin 0x10000 firmware.bin
```

**预防：**
- ✅ 始终保持稳定的factory固件
- ✅ 首先在开发设备上测试新固件
- ✅ 在生产中不要修改factory分区

---

### 完整的OTA更新需要多长时间？

**回答：** 如果WiFi完美，约1 MB固件大约需要1-3分钟。

**典型时间分解：**

| 阶段 | 典型持续时间 | 因素 |
|------|----------------|----------|
| **认证** | 10-30秒 | 用户输入PIN的时间 |
| **上传到Telegram** | 5-15秒 | 用户的互联网速度 |
| **下载到PSRAM** | 10-20秒 | ESP32的WiFi速度 + Telegram API |
| **CRC32验证** | 2-5秒 | 固件大小 |
| **用户确认** | 5-30秒 | 用户反应时间 |
| **备份当前固件** | 10-30秒 | LittleFS写入速度 |
| **刷写新固件** | 5-10秒 | 闪存写入速度 |
| **重启** | 5-10秒 | ESP32启动时间 |
| **验证/otaok** | 5-60秒 | 用户反应时间 |
| **总计** | **约2-4分钟** | 根据条件变化 |

**影响速度的因素：**

**更快：**
- ✅ 强WiFi（靠近路由器）
- ✅ 小固件（<1 MB）
- ✅ 经验丰富的用户（快速响应）
- ✅ LittleFS未碎片化。KissTelegram每3-5分钟整理一次FS

**更慢：**
- ⚠️ WiFi弱或拥塞
- ⚠️ 大固件（>1.5 MB）
- ⚠️ 用户犹豫确认
- ⚠️ LittleFS很满

**安全超时：** 从/otaconfirm开始最多7分钟。如果超过，OTA取消。

---

### 我可以同时更新多个ESP32吗？

**回答：** 可以，但每个ESP32需要自己的Telegram机器人（不同的BOT_TOKEN）。

**Telegram限制：**
- 一个Telegram机器人只能可靠地处理1个同时对话
- Telegram API有速率限制（每个机器人约30条消息/秒）

**选项1：每个设备一个机器人（生产推荐）**
```cpp
// ESP32 #1
#define BOT_TOKEN_1 "123456:ABC-DEF..."
KissTelegram bot1(BOT_TOKEN_1);

// ESP32 #2
#define BOT_TOKEN_2 "789012:GHI-JKL..."
KissTelegram bot2(BOT_TOKEN_2);
```

**优点：**
- ✅ 完全独立的OTA
- ✅ 没有消息冲突
- ✅ 每个设备都有自己的聊天

**缺点：**
- ⚠️ 更多机器人要管理
- ⚠️ 更多令牌要配置

---

**选项2：一个机器人，多个设备顺序执行**
```cpp
// 所有使用相同的BOT_TOKEN
// 但手动一次更新一个
```

**过程：**
1. 更新ESP32 #1 → 等待完成
2. 更新ESP32 #2 → 等待完成
3. 等等

**优点：**
- ✅ 只有一个机器人要管理
- ✅ 对少数设备更简单

**缺点：**
- ⚠️ 一次一个（对许多设备来说很慢）
- ⚠️ 容易弄错设备

---

**选项3：集中管理（企业版KissTelegram）**
```
具有自己API的中央系统
    ↓
向多个ESP32分发固件
    ↓
每个ESP32向中央系统报告进度
    ↓
中央系统通过Json API或通过Telegram通知用户
```

**类似功能（但在企业版Kisstelegram中可用）：**
- Web/云后端
- 设备数据库
- OTA工作队列
- 监控仪表板

**欢迎贡献** KissOTA有很多可能性，请询问或提交PR。

---

## 更新日志

### v0.9.0（当前）
- ✅ 简化的版本系统
- ✅ 删除降级验证
- ✅ 清理过时代码
- ✅ 改进日志消息

### v0.1.0
- ✅ 实现自动备份/回滚
- ✅ 启动循环检测
- ✅ 完整的Telegram集成

### v0.0.2
- ✅ 完全重构为PSRAM
- ✅ 删除OTA_0/OTA_1分区
- ✅ 60秒验证系统

### v0.0.1
- ✅ 基本OTA的初始版本

---

**文档更新：** 12/12/2025
**文档版本：** 0.9.0
**作者** Vicente Soriano, victek@gmail.com**
**MIT许可证**
