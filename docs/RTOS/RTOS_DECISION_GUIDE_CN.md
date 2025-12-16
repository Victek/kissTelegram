# KissTelegram - RTOS vs 简单循环

## 决策指南: 何时使用哪种方法?

### 简单循环方法 (默认推荐)

**典型示例:**
```cpp
void loop() {
  bot.checkMessages(handler);
  bot.processQueue();
  delay(bot.getRecommendedDelay());
}
```

**✅ 使用条件:**
- 单一用途应用(仅机器人+简单逻辑)
- RAM有限 (< 100KB空闲)
- 优先级: 简单性和可维护性
- 团队无RTOS经验
- 顺序逻辑无长时间阻塞操作

**用例示例:**
- 简单温度监控器
- 远程开/关控制
- 基本数据记录器
- 事件通知器

---

### RTOS方法 (高级)

**架构示例:**
```cpp
Core 0: telegramTask()     // 网络 + Telegram
Core 1: applicationTask()  // 应用逻辑
        sensorTask()       // 传感器读取
        displayTask()      // 本地UI
```

**✅ 使用条件:**
- 多个并发子系统
- 应用中的阻塞操作
- 需要优先处理关键任务
- RAM充足 (> 200KB空闲)
- 具有多种功能的可扩展系统

**用例示例:**
- 具有VPD + AEMET +多个传感器的灌溉系统
- 具有多种协议的IoT网关
- 具有本地UI + Telegram + 云的系统
- 具有关键时序的应用

---

## 技术比较

| 方面 | 简单循环 | RTOS |
|---------|-------------|------|
| **代码复杂度** | 低 | 中-高 |
| **RAM开销** | ~0KB | ~12KB (2个任务) |
| **CPU利用率** | 顺序 | 真正并行 |
| **调试** | 简单 | 复杂 |
| **可扩展性** | 有限 | 优秀 |
| **响应延迟** | 可变 | 可预测 |
| **维护** | 简单 | 需要专业知识 |

---

## 混合架构

### 选项1: 具有非阻塞回调的循环
```cpp
void loop() {
  bot.checkMessages(handler);
  bot.processQueue();
  
  // 非阻塞逻辑
  if (millis() - lastSensorRead > INTERVAL) {
    readSensors();
  }
  
  delay(10); // Yield
}
```

**优点:**
- 循环的简单性
- 比纯循环更好的并发性
- 无RTOS开销

**限制:**
- 需要非阻塞纪律
- 无真正的优先级

---

### 选项2: 极简RTOS (1个额外任务)
```cpp
// Telegram在正常loop()中
void loop() {
  bot.checkMessages(handler);
  bot.processQueue();
  delay(bot.getRecommendedDelay());
}

// 仅重逻辑在RTOS任务中
void heavyTask(void *param) {
  while(1) {
    processComplexAlgorithm();
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}
```

**优点:**
- 比完整RTOS复杂度更低
- 隔离阻塞操作
- Telegram继续响应

---

## 迁移: 循环 → RTOS

**步骤1: 识别阻塞操作**
```cpp
// 之前 (阻塞)
void loop() {
  bot.checkMessages(handler);
  
  int result = longCalculation(); // 阻塞5秒
  
  bot.processQueue();
}

// 之后 (非阻塞)
void loop() {
  bot.checkMessages(handler);
  bot.processQueue();
  delay(10);
}

void calculationTask(void *param) {
  while(1) {
    int result = longCalculation();
    vTaskDelay(pdMS_TO_TICKS(5000));
  }
}
```

**步骤2: 添加RTOS原语**
- 机器人的互斥锁
- 通信队列
- 创建任务

**步骤3: 逐步测试**
- 监控堆栈使用
- 验证无死锁
- 测量消耗的RAM

---

## 真实用例

### 1. 简单监控器 (循环)
```cpp
void loop() {
  bot.checkMessages(handler);
  
  if (millis() - last > 60000) {
    float temp = bme.readTemperature();
    bot.queueMessage(CHAT_ID, String(temp));
    last = millis();
  }
  
  bot.processQueue();
  delay(bot.getRecommendedDelay());
}
```

### 2. 智能灌溉系统 (RTOS)
```cpp
Core 0: telegramTask()
  - 接收用户命令
  - 发送关键警报
  
Core 1: irrigationTask()
  - 计算VPD
  - 读取多个传感器
  - 查询AEMET API
  - 决定自适应灌溉
  
Core 1: sensorTask()
  - 持续读取BME680
  - 检测VOC
  - ML推理
```

### 3. IoT网关 (RTOS)
```cpp
Core 0: telegramTask()
  - Telegram网络
  
Core 0: wifiTask()
  - WiFi/LTE故障转移管理
  
Core 1: espNowTask()
  - 从ESP-NOW节点接收
  - 路由消息
  
Core 1: dataProcessingTask()
  - 聚合传感器数据
  - 生成报告
```

---

## 性能基准

### Telegram响应延迟

**简单循环:**
- 最佳情况: 50ms
- 最坏情况: 5000ms (如果应用阻塞)
- 平均: 100-500ms

**RTOS (Telegram优先级2):**
- 最佳情况: 20ms
- 最坏情况: 100ms
- 平均: 30-50ms

### 消息吞吐量

**简单循环:**
- ~0.5-1 msg/s (取决于应用)

**RTOS:**
- ~1-2 msg/s (独立于应用)
- 涡轮模式: ~0.9 msg/s持续

---

## 最终建议

### 对于80%的项目: **简单循环**
- 更容易开发
- 更容易维护
- 足够的性能
- 更少的潜在错误

### 对于20%的项目: **RTOS**
- 复杂的多子系统系统
- 关键时序要求
- 重要的未来可扩展性
- 具有RTOS专业知识的团队

### 黄金法则:
> "从简单开始。仅当您**测量**到简单循环不满足您的要求时才迁移到RTOS。"

---

## 其他资源

- **FreeRTOS ESP32 docs:** https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/freertos.html
- **KissTelegram示例:** 查看`examples/`文件夹
- **ESP32 RTOS教程:** https://www.freertos.org/

---

## 联系方式

对为您的项目选择哪种方法有疑问?

Vicente Soriano - victek@gmail.com
GitHub: https://github.com/victek/KissTelegram
