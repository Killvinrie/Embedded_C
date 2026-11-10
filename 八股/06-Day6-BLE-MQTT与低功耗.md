---
tags: [嵌入式, BLE, MQTT, 低功耗, Day6]
---

# Day 6：BLE、MQTT 与低功耗

[[05-Day5-WiFi与TCPUDP|上一天]] · [[00-开始这里|总计划]] · [[07-Day7-OTA安全与看门狗|下一天]]

## 今日任务（240 分钟）

- [ ] 100 分钟：阅读五个主题，区分无线角色、应用协议与电源状态。
- [ ] 50 分钟：画 BLE 配对/GATT 与 MQTT QoS 交换过程。
- [ ] 50 分钟：计算一年续航预算，推演重连功耗。
- [ ] 40 分钟：复习 Day 5，完成自测。

## Q33 BLE 广播、连接与配对

**广播**是不先建立连接就发出可被扫描到的信息，例如设备名、服务 UUID 或简短状态。广播可以不可连接，也可以允许连接；“看得到广播”不等于“设备允许建立连接”。

传统 BLE 主广播信道为 37、38、39，传统广播数据常见上限 31 字节，扫描响应还有自己的空间；扩展广播的规则和容量不同，不能把 31 字节套用到所有 BLE 模式。

常见流程：外设 Peripheral 广播 → 中央 Central 扫描 → Central 发起连接 → 按协商的连接参数定期交换数据。连接间隔、Peripheral latency、supervision timeout 会影响功耗、延迟和失联判断，参数需符合协议及对端限制。

过去常称“主/从”，现在更常用 Central/Peripheral。**连接、配对、绑定是三件事：**

| 概念 | 目的 |
|---|---|
| 连接 Connection | 建立链路，让两个设备按连接参数通信 |
| 配对 Pairing | 协商安全能力，建立所需密钥与安全关系 |
| 绑定 Bonding | 保存密钥等信息，供未来重连复用 |

设备可以连接而不配对；访问受保护特征时才触发安全要求。配对方式由双方 I/O 能力和安全要求决定，可能是 Just Works、Passkey Entry、Numeric Comparison、OOB 等。Just Works 不提供可靠的中间人认证保障；LE Secure Connections 等机制也要结合实际配对方法判断安全性质。

中央/外设是连接角色，GATT 客户端/服务器是属性访问角色，**不是一一绑定**。手机通常是 Central 和 GATT Client，传感器通常是 Peripheral 和 GATT Server，但这只是常见组合。[Bluetooth SIG BLE primer](https://www.bluetooth.com/bluetooth-le-primer/)

## Q34 GATT 数据模型

ATT 提供属性的访问机制，GATT 在属性之上组织服务和特征，规定发现与访问过程。

```text
设备的 GATT 数据库
└── Service：温度采集服务（一个功能集合）
    ├── Characteristic：温度
    │   ├── Value：实际温度编码
    │   ├── Properties：Read / Notify 等能力
    │   └── Descriptor：如 CCCD，控制订阅
    └── Characteristic：采样周期
        ├── Value：周期数值
        └── Properties：Read / Write
```

UUID 标识属性类型/用途；Handle 是此服务器属性数据库中的标识位置。标准服务常用 SIG 分配的 UUID，自定义服务常用 128 位 UUID。不能认为 UUID 相同就一定是同一台设备或同一条实例数据。

| 操作 | 主要特点 |
|---|---|
| Read | 客户端请求并读取值 |
| Write Request | 有 ATT 层响应 |
| Write Command / Without Response | 没有逐条 ATT 响应；仍需流控和业务设计 |
| Notification | 服务端主动发值，没有 ATT 层逐条确认 |
| Indication | 服务端主动发值，客户端需要 ATT Confirmation |

Notification 没有 ATT 确认，不等于底层“完全没有重传”；连接链路层仍有自己的可靠机制。反过来，Indication 的确认也不等于对方业务已经处理完。

订阅通常通过写 CCCD（客户端特征配置描述符）启用 notify/indicate。传统常见 ATT MTU=23 时，普通通知能装 `MTU-3=20` 字节的特征值；协商更大 MTU 后会变化，**不是 BLE 永远只能传 20 字节**。大对象还要自行设计分片、序号、长度和完成确认。[Bluetooth SIG 的 GATT/ATT 说明](https://www.bluetooth.com/bluetooth-le-primer/)

练习：手机写采样周期后，设备返回“已应用的周期”和请求 ID；不要把 ATT 写成功直接当成传感器配置已经生效。

## Q35 MQTT 与 QoS

MQTT 是发布/订阅协议：设备向 Broker 的 Topic 发布消息，订阅者按 Topic Filter 接收。常见 MQTT 运行在 TCP 上，TLS 提供传输安全。

```text
温度节点 → 发布 device/123/telemetry → Broker → 数据平台订阅
控制台   → 发布 device/123/command   → Broker → 温度节点订阅
```

### 三个 QoS 的含义与机制

| QoS | 语义 | 典型交换 | 要付出的代价 |
|---|---|---|---|
| 0 | 至多一次，可能丢失 | PUBLISH | 最少协议状态和报文 |
| 1 | 至少一次，可能重复 | PUBLISH → PUBACK | 维护未确认状态及标识，接收业务要考虑重复 |
| 2 | MQTT 协议规定范围内恰好一次交付 | PUBLISH → PUBREC → PUBREL → PUBCOMP | 更多交互和两端状态 |

QoS 1/2 使用 Packet Identifier 关联在途事务；QoS 2 依靠状态机识别重复 PUBLISH/PUBREL，避免重复交付，而不是“多发几次所以就恰好一次”。[MQTT 3.1.1 规范 §4.3](https://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.html#_Toc398718099)

```text
QoS 2 发送者                  接收者
PUBLISH (packet id=7)  ──────>
                     <────── PUBREC (7)
PUBREL (7)            ──────>
                     <────── PUBCOMP (7)
该事务完成，释放对应状态
```

### 面试必须讲清的边界

- QoS 的保障适用于 MQTT 协议交付及其会话状态；发布者→Broker 与 Broker→订阅者分别处理，订阅侧有效 QoS 受发布 QoS 和获准订阅 QoS 共同限制。
- “QoS 2”不能自动保证数据库、继电器或扣款业务恰好执行一次；应用崩溃、重新发布的新事务、外部副作用仍需 request_id、持久化和幂等设计。
- MQTT 3.1.1 持久会话通常涉及稳定 Client ID 和 `CleanSession=0`；重连时按规范恢复未确认交换，不能假设任意断电后 RAM 中的在途状态都还在。
- 不要笼统宣称“MQTT 1/2 在正常 TCP 连接上固定每隔 N 秒重发”。重传/恢复应遵循 MQTT 版本和客户端实现，尤其注意会话重连规则。[MQTT 3.1.1 消息重发与会话章节](https://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.html#_Toc398718102)

### 配套概念

Keep Alive 与 PINGREQ/PINGRESP 用于连接活性检测；遗嘱 Will 用于异常失联后的通知；Retain 通常保存 Topic 的最新保留消息，**不是消息历史数据库**。QoS、Retain、持久会话解决不同问题，不能互相代替。

建议按业务选：高频可丢遥测用 QoS 0；重要事件常用 QoS 1 加应用去重；确有协议交付要求且能接受开销时考虑 QoS 2。对电池节点还要计算重连、握手、确认往返的能耗。

## Q36 ESP32 的睡眠模式

以下以经典 ESP32、ESP-IDF v5.2 为参考。功耗数值与芯片型号、电源域、外设及开发板电路有关，不能背一个“ESP32 深睡电流”用于所有设备。

| 模式 | CPU/无线状态 | 内存与恢复 | 适合 |
|---|---|---|---|
| Modem-sleep | 无线按策略休眠，CPU 可继续运行 | 普通 RAM 保留，可维持连接相关策略 | 需要持续业务但允许无线间歇活动 |
| Light-sleep | CPU 暂停，许多时钟关闭/电源降低 | 相关状态与 RAM 保留，唤醒后继续运行 | 低延迟恢复、短空闲窗口 |
| Deep-sleep | 主 CPU 和大部分数字域关闭 | 普通 RAM 不保留；可保留特定 RTC 域，唤醒走启动流程 | 长间隔采样、允许重新联网 |
| 更激进的 RTC 域关闭组合 | 尽量只保留必要唤醒能力 | 可保留内容更少；有时称 hibernation 子模式 | 极低待机功耗，功能受限 |

手动 light/deep sleep 通常不会自动维持 Wi-Fi/BLE 连接；若希望维持连接并自动低功耗，应结合协议栈的 modem-sleep、自动 light-sleep 与电源管理配置。[ESP-IDF v5.2 睡眠说明](https://docs.espressif.com/projects/esp-idf/en/v5.2/esp32/api-reference/system/sleep_modes.html)

### 深睡醒来，RAM 数据还在吗

不能说“RAM 都在”，也不能说“一切数据全丢”。普通任务栈、堆和普通全局变量不会作为原任务运行状态继续保留。被配置保留的 RTC 内存可跨 deep sleep 保存少量数据；Flash/NVS 的持久数据不因深睡丢失。

```c
/* ESP-IDF 经典 ESP32 示例片段；与其他 app_main 分开使用。 */
#include <stdint.h>
#include "esp_attr.h"
#include "esp_sleep.h"
#include "esp_err.h"

RTC_DATA_ATTR static uint32_t wake_count;

void app_main(void)
{
    ++wake_count;
    /* 此处完成有界的采样/上传，再检查返回值和保存必要状态。 */
    ESP_ERROR_CHECK(esp_sleep_enable_timer_wakeup(60ULL * 1000000ULL));
    esp_deep_sleep_start(); /* 深睡唤醒后重新启动，不从下一行接着执行 */
}
```

RTC 保留变量不是永久存储；完全断电会丢失，其他复位类型的行为也要核对启动初始化规则。关键 RTC 状态可带 magic、版本和校验；长期配置放 NVS，并控制写频率与 Flash 磨损。

## Q37 一年续航怎么设计

先从电量预算倒推，而不是先选一个 sleep API。

设电池标称 2400mAh，保守可用比例 80%，目标一年 8760 小时：

```text
可用容量 = 2400 × 0.8 = 1920mAh
允许平均电流 = 1920 / 8760 = 0.219mA ≈ 219µA
```

这是简化估算，容量会受温度、自放电、脉冲负载和截止电压影响；跨不同供电电压时还要按能量及转换效率折算。

### 分阶段算平均电流

每 600 秒上报一次，假设所有电流均是**电池端测量值**：

| 阶段 | 电流 | 每周期时间 | 电荷（mA·s） |
|---|---:|---:|---:|
| 联网/上传 | 120mA | 1s | 120 |
| 采样与处理 | 10mA | 0.1s | 1 |
| 休眠 | 0.01mA | 598.9s | 5.989 |
| 合计 | — | 600s | 126.989 |

`I_avg = 126.989/600 ≈ 0.212mA = 212µA`。已经接近 219µA 的预算，几乎没有实际余量。一年耗电约 `0.212×8760≈1854mAh`。

如果每次弱网联网要 3 秒，其余休眠变为 596.9 秒：

`I_avg=(120×3+10×0.1+0.01×596.9)/600≈0.612mA`，一年约 5358mAh，明显超标。这个例子说明，**连接耗时可能比深睡电流从 10µA 优化到 5µA 更值得优先处理**。这些数值是教学假设，不是 ESP32 实测承诺。

### 从哪些方向省电

| 方向 | 动作 | 需要验证的代价 |
|---|---|---|
| 业务频率 | 批量上传、事件触发、减少无意义心跳 | 数据时效、告警时延 |
| 无线过程 | 缩短扫描和握手，合理缓存连接信息 | AP 变化后的恢复能力、安全会话正确性 |
| 弱网策略 | 有限重试、指数退避、离线聚合 | 可接受的数据延迟/丢弃 |
| CPU/RTOS | 事件驱动、阻塞等待、tickless、合适频率 | 唤醒开销与计算完成时间 |
| 外设 | 传感器断电、降低采样率、关闭不需要的外设 | 重新稳定/校准时间 |
| 硬件 | 低静态电流稳压器、去掉常亮 LED/高耗 USB 桥、降低漏电 | 峰值电流、供电纹波与启动可靠性 |
| GPIO | 防悬空，检查上拉分压与外设反向供电 | 唤醒脚电平保持与误唤醒 |

不能只测开发板“芯片标称深睡电流”；要在最终电池供电入口记录完整周期，积分得到电荷，并覆盖冷启动、TLS、重连、OTA、低电量等场景。峰值电流即便平均值合格，也可能造成压降复位。

> [!tip] 面试回答
> 我先把电池可用容量换成平均电流上限，再按采样、联网、传输、休眠分阶段测量并积分。优先优化占电荷最多的阶段，特别是弱网重连；同时检查电源静态电流和外设漏电。最终用最坏场景留余量验证一年目标。

## 自测与答案

1. BLE 连接成功是否代表已配对并绑定？
2. Notify 与 Indicate 的区别在哪一层？
3. QoS 2 能否保证继电器命令绝不执行两次？
4. 深睡后，malloc 返回的指针能继续用吗？
5. `static` 是否等于“深睡保留”？
6. 2000mAh 电池按 70% 可用，一年平均电流上限是多少？

> [!answer]- 答案
> 1. 不能，连接、配对、绑定不同。2. ATT 层是否有确认，不能与链路层重传或业务确认混淆。3. 不能，业务副作用需要幂等与持久状态。4. 不能，普通堆不按原执行上下文保留。5. 不等于，静态存储期不代表电源域保留。6. 1400/8760≈0.160mA，即约 160µA，还需核对转换效率等假设。

## 今日过关标准

- [ ] 能分开讲 Central/Peripheral、GATT Client/Server。
- [ ] 能画出 QoS 0/1/2 的消息交换，并说明保证范围。
- [ ] 能回答普通 RAM、RTC 内存、NVS 在深睡后的区别。
- [ ] 能用电量积分否定一个不现实的续航方案。
