# MicroCom — CAN / LIN 消息调度

[English](./readme.md)

MicroCom 是一个面向嵌入式 MCU 的轻量级静态消息调度组件，主要针对基于 Cortex-M 的系统。

它为 CAN 和 LIN 通信提供统一的消息调度能力，包括周期发送、周期接收监控、事件驱动发送、接收指示、超时处理以及诊断通信挂起。

MicroCom 不实现 CAN 或 LIN 硬件驱动，而是通过少量驱动接口与硬件层连接，使同一套调度层可以复用于不同的 MCU 和外设。

---

## 特性

* CAN 周期 TX/RX 调度
* CAN 事件驱动 TX/RX 处理
* LIN 周期主节点 TX/RX 调度
* LIN 标识符及受保护标识符处理
* RX 超时监控
* TX 完成通知
* RX 接收指示及 RX 错误通知
* 诊断通信挂起与恢复
* 静态配置，无动态内存分配
* 可配置的通道及消息表容量
* 与硬件无关的调度层
* 通过 Weak 驱动接口适配具体 MCU
* 统一的消息事件回调上下文
* 可选的快速回调模式
* 兼容 C / C++ 公共接口

---

## 架构

MicroCom 在逻辑上分为三个层次：

```text
+------------------------------------------------------+
|                      应用层                          |
|                                                      |
|       消息回调 / 诊断 / 应用程序逻辑                 |
+---------------------------+--------------------------+
                            |
                            v
+------------------------------------------------------+
|                    MicroCom                          |
|                                                      |
|  +----------------+       +-----------------------+  |
|  | MicroCom_Can   |       | MicroCom_Lin          |  |
|  |                |       |                       |  |
|  | Cycle TX / RX  |       | 帧调度                |  |
|  | Event TX / RX  |       | TX / RX               |  |
|  | Timeout        |       | Timeout               |  |
|  | Diagnostics    |       | TX Done / RX Error     |  |
|  +----------------+       +-----------------------+  |
+---------------------------+--------------------------+
                            |
                            v
+------------------------------------------------------+
|                    硬件驱动层                        |
|                                                      |
|            CAN 外设 / LIN 外设                      |
|            MCU 相关硬件实现                          |
+------------------------------------------------------+
```

MicroCom 调度器不会直接访问 MCU 寄存器。

硬件驱动负责实际的总线操作，而 MicroCom 负责消息调度以及状态管理。

---

## 初始化与生命周期

CAN 和 LIN 遵循相同的基本生命周期：

```text
Init
  |
  v
注册配置
  |
  v
Start
  |
  +----> TickHandler
  |
  +----> TimerHandler
  |
  v
Stop
```

配置注册必须在 `Start()` 之前完成。

不支持运行时注册。

### CAN

```c
MicroCom_Can_Init();

MicroCom_Can_Register_CycleTxMsg(...);
MicroCom_Can_Register_CycleRxMsg(...);

#if MICROCOM_CAN_EVENTMSG_ENABLE
MicroCom_Can_Register_EventTxMsg(...);
MicroCom_Can_Register_EventRxMsg(...);
#endif

MicroCom_Can_Start();
```

### LIN

```c
MicroCom_Lin_Init();

MicroCom_Lin_RegisterTable(...);

MicroCom_Lin_Start();
```

---

# CAN

## CAN 特性

MicroCom CAN 提供四种相互独立的消息类型：

```text
Cycle TX
Cycle RX
Event TX
Event RX
```

每种消息类型都有独立的配置表和调度状态。

### 周期 TX

周期 TX 消息根据配置的周期时间进行周期性发送。

调度器根据配置的周期时间递增下一次发送时间，而不是根据当前时间重新计算。

这样可以避免长期运行产生的时间漂移。

```text
next_time += cycle
```

### 周期 RX

周期 RX 消息定义了预期的接收周期或超时时间。

如果在配置的超时时间内没有接收到匹配的报文，该消息将被标记为离线，并生成对应的回调事件。

### 事件 TX

事件 TX 消息按需发送。

应用程序可以通过以下接口触发事件消息：

```c
MicroCom_Can_Trigger_EventTxMsg(...)
```

调度器会在正常调度过程中处理待发送的事件请求。

### 事件 RX

事件 RX 消息用于监控事件报文的接收以及离线状态。

---

## CAN 消息注册

CAN 配置分为四个相互独立的注册操作：

```c
MicroCom_Can_Register_CycleTxMsg(...)
MicroCom_Can_Register_CycleRxMsg(...)

MicroCom_Can_Register_EventTxMsg(...)
MicroCom_Can_Register_EventRxMsg(...)
```

注册过程会在修改内部调度器状态之前，对完整配置进行验证。

如果注册失败，已经注册的配置不会被部分修改。

所有注册操作都必须在：

```c
MicroCom_Can_Start();
```

之前完成。

---

## CAN 调度器

调度器由两个 Handler 组成。

### TickHandler

```c
MicroCom_Can_TickHandler();
```

`TickHandler()` 用于推进内部调度器时间基准。

通常由固定频率的定时器中断调用。

例如：

```text
1 ms 定时器中断
        |
        v
MicroCom_Can_TickHandler()
```

### TimerHandler

```c
MicroCom_Can_TimerHandler();
```

`TimerHandler()` 执行实际的调度工作。

它负责：

* 启动到期的周期 TX 发送
* 处理周期 RX 超时状态
* 处理事件 TX 触发请求
* 更新消息调度状态

`TickHandler()` 和 `TimerHandler()` 不要求在相同的上下文或相同的频率下执行。

例如：

```text
SysTick 1 ms
    |
    +--> TickHandler()

主循环
    |
    +--> TimerHandler()
```

---

## CAN 接收路径

CAN 硬件驱动在接收到 CAN 帧后调用：

```c
MicroCom_Can_RxIndication(
    channel,
    can_id,
    is_extend,
    data,
    len
);
```

调度器随后搜索对应的已配置消息，并更新其接收状态。

之后，应用程序回调将根据配置的回调策略被调用。

```text
CAN 硬件
     |
     v
CAN 驱动
     |
     v
MicroCom_Can_RxIndication()
     |
     v
消息匹配
     |
     v
消息状态更新
     |
     v
应用程序回调
```

---

## CAN 发送路径

MicroCom 不会直接访问 CAN 硬件。

当某条消息需要发送时，调度器调用：

```c
MicroCom_Can_Transmit(...)
```

该函数作为 Weak 硬件接口提供。

具体设备的 CAN 驱动负责提供实际实现。

```text
MicroCom 调度器
       |
       v
MicroCom_Can_Transmit()
       |
       v
CAN 驱动
       |
       v
CAN 外设
```

硬件发送完成后，驱动调用：

```c
MicroCom_Can_HwTxDone(...)
```

这样 MicroCom 可以明确区分：

```text
发送请求已提交
        !=
发送操作已完成
```

---

## CAN 事件回调

CAN 回调使用统一的上下文：

```c
typedef struct
{
    MicroCom_Can_Event_t event;
    uint8_t              channel;
    uint16_t             mboxId;
    bool                 is_extend;
    uint32_t             id;
    uint8_t              *data;
    uint8_t              len;
    void                 *userData;
} MicroCom_Can_Ctx_t;
```

因此，同一个回调函数可以被多个消息共享。

回调可以通过以下成员识别消息来源：

```c
ctx->channel
ctx->id
ctx->mboxId
ctx->is_extend
```

`event` 字段用于标识触发回调的原因。

```c
typedef enum
{
    MICROCOM_CAN_EVENT_CYCLE_TX,
    MICROCOM_CAN_EVENT_EVENT_TX,

    MICROCOM_CAN_EVENT_CYCLE_RX,
    MICROCOM_CAN_EVENT_EVENT_RX,

    MICROCOM_CAN_EVENT_CYCLE_TX_DONE,
    MICROCOM_CAN_EVENT_EVENT_TX_DONE,

    MICROCOM_CAN_EVENT_CYCLE_TX_ERROR,
    MICROCOM_CAN_EVENT_EVENT_TX_ERROR,

    MICROCOM_CAN_EVENT_CYCLE_RX_ERROR,
    MICROCOM_CAN_EVENT_EVENT_RX_ERROR,

} MicroCom_Can_Event_t;
```

---

## CAN 诊断通信控制

MicroCom 可以暂停指定 CAN 通道上的所有非诊断通信。

```c
MicroCom_Can_DisableNonDiagnosticCom(channel);
```

配置为诊断消息的通信不受影响。

之后可以通过以下接口恢复通信：

```c
MicroCom_Can_EnableNonDiagnosticCom(channel);
```

该机制用于诊断会话，在诊断过程中需要暂时停止普通应用通信时使用。

---

## CAN 离线状态

可以通过以下接口查询周期 RX 消息的离线状态：

```c
MicroCom_Can_IsCycleRxBusOffline(...)
```

应用程序可以通过该状态判断某条周期 CAN 消息当前是否被认为处于可用状态。

---

# LIN

MicroCom LIN 为 LIN 主节点通信提供静态消息调度。

LIN 模块使用与 CAN 模块相同的基本生命周期：

```text
Init
  |
  v
RegisterTable
  |
  v
Start
  |
  +----> TickHandler
  |
  +----> TimerHandler
```

与 CAN 不同，LIN 通信由主节点驱动。

因此，调度器负责决定下一帧是主节点发送帧还是主节点接收帧。

---

## LIN 帧模型

MicroCom 在配置层暴露逻辑 LIN 标识符。

用户配置：

```text
ID
Data Field
Length
```

Protected Identifier（PID）不需要直接配置。

LIN 驱动或协议层根据 6 位 ID 计算 PID。

```text
6-bit ID
   |
   v
奇偶校验计算
   |
   v
8-bit PID
```

有效 LIN 标识符范围：

```text
0x00 ~ 0x3F
```

LIN 数据域最大支持：

```text
8 bytes
```

---

## LIN 消息配置

LIN 消息通过单个配置表进行注册：

```c
MicroCom_Lin_RegisterTable(...)
```

注册过程会在提交配置之前完成完整的配置检查。

如果任何配置项无效，内部 LIN 调度器状态都不会被修改。

所有注册操作必须在：

```c
MicroCom_Lin_Start();
```

之前完成。

---

## LIN 调度

LIN 调度器使用：

```c
MicroCom_Lin_TickHandler();
MicroCom_Lin_TimerHandler();
```

### TickHandler

`MicroCom_Lin_TickHandler()` 用于推进调度器时间基准。

它应当按照配置的调度频率调用。

### TimerHandler

`MicroCom_Lin_TimerHandler()` 执行实际的调度工作。

对于每个 LIN 通道，调度器会查找下一条已经到达调度时间的消息。

根据配置的帧方向，调度器启动：

```text
Master TX
```

或者：

```text
Master RX
```

调度器同时会监控等待中的 RX 消息是否发生超时。

---

## LIN 硬件接口

MicroCom LIN 使用两个硬件提交接口：

```c
MicroCom_Lin_Transmit(...)
MicroCom_Lin_Receive(...)
```

两个函数均提供 Weak 默认实现。

具体设备的 LIN 驱动提供 Strong 实现。

### Master TX

```c
MicroCom_Lin_Transmit(...)
```

请求硬件驱动执行：

```text
发送 LIN Break
      |
      v
发送 Sync
      |
      v
发送 PID
      |
      v
发送配置的数据域
```

该函数是异步的。

返回成功仅表示发送请求已经被驱动接受。

这并不表示发送操作已经完成。

### Master RX

```c
MicroCom_Lin_Receive(...)
```

请求硬件驱动执行：

```text
发送 LIN Break
      |
      v
发送 Sync
      |
      v
发送 PID
      |
      v
等待从节点响应
```

接收到的数据通过以下接口异步返回：

```c
MicroCom_Lin_RxIndication(...)
```

---

## LIN 驱动通知

发送操作完成后，硬件驱动调用：

```c
MicroCom_Lin_TxDone(...)
```

接收到有效响应后调用：

```c
MicroCom_Lin_RxIndication(...)
```

如果接收失败：

```c
MicroCom_Lin_RxError(...)
```

因此 LIN 的依赖方向为：

```text
+------------------------+
|   MicroCom LIN         |
|                        |
| 调度器 / 状态管理      |
+-----------+------------+
            |
            | 调用
            v
+------------------------+
| MicroCom_Lin_Transmit  |
| MicroCom_Lin_Receive   |
+-----------+------------+
            |
            v
+------------------------+
| 具体设备 LIN 驱动      |
+-----------+------------+
            |
            | 回调
            v
+------------------------+
| MicroCom_Lin_TxDone    |
| MicroCom_Lin_RxIndication|
| MicroCom_Lin_RxError   |
+------------------------+
```

这样可以使 LIN 调度器与具体 MCU 外设保持独立。

---

## LIN 回调上下文

LIN 回调接收：

```c
typedef struct
{
    MicroCom_Lin_Event_t event;
    uint8_t              channel;
    uint8_t              id;
    uint8_t              *data;
    uint8_t              len;
    void                 *userData;
} MicroCom_Lin_Ctx_t;
```

事件类型：

```c
typedef enum
{
    MICROCOM_LIN_EVENT_TX,
    MICROCOM_LIN_EVENT_TX_DONE,
    MICROCOM_LIN_EVENT_TX_ERROR,
    MICROCOM_LIN_EVENT_RX,
    MICROCOM_LIN_EVENT_RX_ERROR,

} MicroCom_Lin_Event_t;
```

回调可以使用：

```c
ctx->event
ctx->channel
ctx->id
ctx->data
ctx->len
ctx->userData
```

来确定 LIN 操作的来源和结果。

---

# 配置

主要编译期配置位于：

```text
MicroCom_conf.h
```

典型配置包括：

| 宏                               | 描述              |
| ------------------------------- | --------------- |
| `MICROCOM_VERSION`              | MicroCom 版本     |
| `MICROCOM_FREQ_HZ`              | 调度器频率           |
| `MICROCOM_FASTMODE_ENABLE`      | 启用快速回调模式        |
| `MICROCOM_CAN_EVENTMSG_ENABLE`  | 启用 CAN 事件消息     |
| `MICROCOM_CAN_CHANNEL_NUM`      | CAN 通道数量        |
| `MICROCOM_CAN_CYCLEMSG_TX_SIZE` | 每通道周期 CAN TX 容量 |
| `MICROCOM_CAN_CYCLEMSG_RX_SIZE` | 每通道周期 CAN RX 容量 |
| `MICROCOM_CAN_EVENTMSG_TX_SIZE` | 每通道事件 CAN TX 容量 |
| `MICROCOM_CAN_EVENTMSG_RX_SIZE` | 每通道事件 CAN RX 容量 |
| `MICROCOM_CAN_MAX_DLC`          | CAN 最大数据长度      |
| `MICROCOM_LIN_CHANNEL_NUM`      | LIN 通道数量        |
| `MICROCOM_LIN_MSG_SIZE`         | 每通道 LIN 消息容量    |

MicroCom 不会在运行时动态分配消息存储空间。

配置的容量决定了组件的静态内存占用。

---

# 状态码

MicroCom API 使用：

```c
MicroCom_Status_t
```

当前定义的状态值：

```c
typedef enum
{
    MICROCOM_STATUS_OK,
    MICROCOM_STATUS_ERR,
    MICROCOM_STATUS_BUSY,
    MICROCOM_PARAM_INVALID,
    MICROCOM_CHANNEL_ERR,
    MICROCOM_NOT_FIND,
    MICROCOM_CHANNEL_OVERFLOW,
    MICROCOM_DATA_OVERFLOW,

} MicroCom_Status_t;
```

这些状态码由 CAN 和 LIN 模块共享。

---

# 线程安全与中断上下文

MicroCom 面向小型嵌入式系统设计，不需要动态内存分配。

应用程序应当区分：

```text
配置上下文
运行时调度上下文
硬件中断上下文
```

### 配置阶段

注册 API 应当只在 `Start()` 之前调用。

不应当在调度器运行期间并发调用注册 API。

### 运行阶段

`TickHandler()` 和 `TimerHandler()` 会操作调度器状态。

如果二者运行在不同上下文中，应用程序必须确保所采用的执行模型能够提供所需的同步机制。

### 硬件回调

以下函数可能根据具体硬件驱动的实现，在中断上下文中被调用：

```c
MicroCom_Can_RxIndication()
MicroCom_Can_HwTxDone()

MicroCom_Lin_TxDone()
MicroCom_Lin_RxIndication()
MicroCom_Lin_RxError()
```

因此，回调函数应当保持短小并且非阻塞。

---

# 内存模型

MicroCom 使用静态内存。

组件不使用：

```c
malloc()
calloc()
realloc()
free()
```

消息容量通过 `MicroCom_conf.h` 在编译期确定。

这样可以提供：

* 确定性的内存使用
* 无堆内存碎片
* 可预测的运行时行为
* 更容易进行 RAM 估算
* 更容易进行嵌入式配置

实际 RAM 消耗取决于：

* CAN 通道数量
* CAN 消息槽数量
* LIN 通道数量
* LIN 消息槽数量
* 配置结构体大小
* 回调及用户数据存储

---

# 示例

一个最小的 CAN 初始化示例：

```c
#include "MicroCom_Can.h"

void App_CanInit(void)
{
    MicroCom_Can_Init();

    MicroCom_Can_Register_CycleTxMsg(
        cycle_tx_table,
        cycle_tx_size
    );

    MicroCom_Can_Register_CycleRxMsg(
        cycle_rx_table,
        cycle_rx_size
    );

#if MICROCOM_CAN_EVENTMSG_ENABLE

    MicroCom_Can_Register_EventTxMsg(
        event_tx_table,
        event_tx_size
    );

    MicroCom_Can_Register_EventRxMsg(
        event_rx_table,
        event_rx_size
    );

#endif

    MicroCom_Can_Start();
}
```

一个典型的调度器集成方式：

```c
void SysTick_Handler(void)
{
    MicroCom_Can_TickHandler();
    MicroCom_Lin_TickHandler();
}

void MainLoop(void)
{
    MicroCom_Can_TimerHandler();
    MicroCom_Lin_TimerHandler();
}
```

`TimerHandler()` 的实际执行频率可以与 Tick 频率不同，具体取决于应用程序的调度需求。

---

# 目录结构

```text
MicroCom/
|
+-- MicroCom_conf.h
+-- MicroCom_utils.h
|   +-- MicroCom_Can.h
|   +-- MicroCom_Can.c
|   +-- MicroCom_Can_types.h
    +-- MicroCom_Lin.h
    +-- MicroCom_Lin.c
    +-- MicroCom_Lin_types.h
```

具体目录组织方式可以根据集成项目进行调整。

---

# 设计原则

MicroCom 遵循以下核心设计原则：

### 静态优于动态

所有消息容量都在编译期确定。

### 调度器优于硬件

MicroCom 决定消息**何时**应该发送或处理。

硬件驱动决定帧**如何**被实际发送。

### 明确的异步完成机制

提交发送请求并不意味着发送已经完成。

硬件发送完成通过明确的回调进行通知。

### 硬件无关

调度器不依赖特定的 CAN 或 LIN 外设。

因此，同一套 MicroCom 实现可以复用于不同 MCU。

### 运行前完成配置

消息表在初始化阶段注册，并在正常运行期间保持固定。

这样可以使运行时调度器保持简单且具有确定性。

---

# 许可证

Copyright (c) 2026.

[许可证](./LICENSE)