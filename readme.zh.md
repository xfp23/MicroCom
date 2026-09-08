# 通信调度管理组件 (CAN / LIN)

[English](./README.md)

MicroCom 是一个面向嵌入式 MCU（以 Cortex-M4 为主要目标）的总线报文调度组件，
负责周期报文的定时发送、接收超时监控、事件报文的按需触发，以及诊断会话期间
非诊断报文的统一挂起/恢复。

---

## 目录结构

```
MicroCom_conf.h    # 编译期配置：通道数、表容量、临界区宏
MicroCom_types.h   # 数据类型定义
MicroCom_utils.h    # 参数校验宏
MicroCom_Can.h      # CAN 模块对外接口
MicroCom_Can.c      # CAN 模块实现
MicroCom_Lin.h      # (预留) LIN 模块对外接口
MicroCom_Lin.c      # (预留) LIN 模块实现
```

> LIN 相关文件与本文档章节均为占位，尚未实现。

---

## CAN

### 功能特性

- 按方向拆分为四张独立表：周期发送（CycleTx）、周期接收（CycleRx）、
  事件发送（EventTx）、事件接收（EventRx），每张表按通道独立管理。
- 周期发送报文到点自动发送，内部使用 `time += cycle` 的方式推进基准时间，
  避免长期运行产生周期漂移。
- 周期接收报文带超时监控，超时自动置位 `busoff` 并回调用户。
- 事件报文按需触发（`trigger` 计数），可跨中断/任务上下文安全调用。
- 诊断会话期间可一键挂起/恢复某通道上所有非诊断报文（`is_diag == false`）。
- 统一的回调上下文 `MicroCom_Ctx_t`（携带 `event / channel / id / userData`），
  一个回调函数即可服务多条报文。

### 架构设计

#### 数据结构

| 结构体                        | 方向         | 关键字段                                        |
| ------------------------------ | ------------ | ------------------------------------------------- |
| `MicroCom_CanCycleTxMsg_t`    | TX（周期）   | `cycle`, `next_time`, `is_run`                    |
| `MicroCom_CanCycleRxMsg_t`    | RX（周期）   | `timeout`, `last_rx_time`, `busoff`, `is_run`     |
| `MicroCom_CanEventTxMsg_t`    | TX（事件）   | `trigger`, `is_run`                               |
| `MicroCom_CanEventRxMsg_t`    | RX（事件）   | `busoff`, `is_run`                                |

四张表均以 `[channel][index]` 二维数组组织，`index` 为注册表中的全局序号
（同一通道内不会冲突，但不同通道各自占满一份 `MICROCOM_CAN_CYCLEMSG_SIZE` /
`MICROCOM_CAN_EVENTMSG_SIZE` 容量，属于以空间换简单性的设计取舍）。

#### 调度模型

```
MicroCom_Can_TickHandler()   -> 每个调度 tick 调用一次，推进 can_obj.tick

MicroCom_Can_TimerHandler()  -> 周期性调用，驱动 TX 发送 / RX 超时检测 / 事件 TX 消耗

MicroCom_Can_RxIndication()  -> 由 CAN 接收中断/任务调用，喂入收到的一帧报文
```

`TickHandler` 与 `TimerHandler` 可以挂在同一个中断里按顺序调用，也可以分开、
以不同频率调用（例如 `TickHandler` 挂 1ms SysTick，`TimerHandler` 在主循环里
以更低频率调用）。

#### 线程安全与中断

| 共享状态              | 访问上下文                                              | 保护方式                                                          |
| ---------------------- | ---------------------------------------------------------- | -------------------------------------------------------------------- |
| `tick`                 | 单写者（TickHandler），多读者                              | 32bit 对齐读写天然原子，无需额外保护                                  |
| `is_run` / `busoff`    | 多写者，单字段布尔写                                        | 单字节写天然原子，无需额外保护                                        |
| `trigger`              | `Trigger_EventMsg` 中 `++`，`TimerHandler` 中 `--`          | **读改写，需要临界区**：`MICROCOM_ENTER/EXIT_CRITICAL()`             |

> 注册函数（`Register_*`）不是中断/线程安全的，约定必须在
> `MicroCom_Can_Start()` 之前完成全部注册，运行期间不要再注册。

`MICROCOM_ENTER_CRITICAL()` / `MICROCOM_EXIT_CRITICAL()` 默认使用
`__disable_irq()` / `__enable_irq()`；若工程使用 RTOS，请在
`MicroCom_conf.h` 中改写为对应的临界区 API（如
`taskENTER_CRITICAL()` / `taskENTER_CRITICAL_FROM_ISR()`），避免全局关中断
影响系统实时性。

### 配置

`MicroCom_conf.h` 中的宏：

| 宏                             | 说明                                                              | 默认值 |
| -------------------------------- | ---------------------------------------------------------------------- | -------- |
| `MICROCOM_CAN_CHANNEL_NUM`      | CAN 通道数量                                                          | `2`     |
| `MICROCOM_CAN_CYCLEMSG_SIZE`    | 每通道周期报文槽位数（TX/RX 各自独立）                                  | `16`    |
| `MICROCOM_CAN_EVENTMSG_SIZE`    | 每通道事件报文槽位数（TX/RX 各自独立）                                  | `16`    |
| `MICROCOM_CAN_MAX_DLC`          | 单帧最大数据长度，经典 CAN=8，CAN FD 可改为 64                          | `8`     |
| `MICROCOM_ENTER_CRITICAL()`     | 进入临界区                                                            | `__disable_irq()` |
| `MICROCOM_EXIT_CRITICAL()`      | 退出临界区                                                            | `__enable_irq()`  |

### API 参考

| 函数                                             | 说明                                                                       |
| -------------------------------------------------- | -------------------------------------------------------------------------------- |
| `MicroCom_Can_Init(void)`                          | 模块初始化，需最先调用一次                                                    |
| `MicroCom_Can_Start(void)`                         | 启动调度，对齐所有报文的时间基准                                              |
| `MicroCom_Can_Stop(void)`                          | 停止调度                                                                      |
| `MicroCom_Can_Register_CycleTxMsg(table, size)`    | 注册周期发送报文                                                              |
| `MicroCom_Can_Register_CycleRxMsg(table, size)`    | 注册周期接收报文                                                              |
| `MicroCom_Can_Register_EventTxMsg(table, size)`    | 注册事件发送报文                                                              |
| `MicroCom_Can_Register_EventRxMsg(table, size)`    | 注册事件接收报文                                                              |
| `MicroCom_Can_TickHandler(void)`                   | 推进调度 tick                                                                 |
| `MicroCom_Can_TimerHandler(void)`                  | 周期调度处理（TX 发送 / RX 超时 / 事件消耗）                                   |
| `MicroCom_Can_RxIndication(channel, id, data, len)` | 喂入一帧接收报文                                                             |
| `MicroCom_Can_Trigger_EventMsg(id, channel)`       | 触发一次事件发送报文                                                          |
| `MicroCom_Can_SetEventBusOff(id, channel)`         | 置位事件接收报文的 busoff                                                     |
| `MicroCom_Can_ClearEventBusOff(id, channel)`       | 清除事件接收报文的 busoff                                                     |
| `MicroCom_Can_DisableNonDiagnosticCom(channel)`    | 挂起通道上所有非诊断报文                                                      |
| `MicroCom_Can_EnableNonDiagnosticCom(channel)`     | 恢复通道上所有非诊断报文                                                      |
| `MicroCom_Can_Transmit(channel, id, mbox, dlc, data, is_extend)` | 底层发送钩子（弱符号，由 CAN 驱动实现）                          |

### 使用示例

```c
#include "MicroCom_Can.h"

static uint8_t rx_buf_100[8];

static void OnMsg100(MicroCom_Ctx_t *ctx)
{
    if (ctx->event == MICROCOM_EVENT_RX)
    {
        /* rx_buf_100 已更新 */
    }
    else if (ctx->event == MICROCOM_EVENT_ERROR)
    {
        /* 接收超时 */
    }
}

static const MicroCom_CanCycleRxMsg_t cycle_rx_table[] = {
    {
        .id = 0x100, .is_Extend = false, .dlc = 8, .mbox_id = 0,
        .channel = 0, .is_diag = false, .timeout = 100,
        .data = rx_buf_100, .userData = NULL, .func = OnMsg100,
    },
};

void App_CanInit(void)
{
    MicroCom_Can_Init();
    MicroCom_Can_Register_CycleRxMsg(cycle_rx_table, 1);
    /* ... 其余表的注册 ... */
    MicroCom_Can_Start();
}

/* 1ms 定时中断 */
void SysTick_Handler(void)
{
    MicroCom_Can_TickHandler();
    MicroCom_Can_TimerHandler();
}

/* CAN 接收中断 */
void CAN_RxCallback(uint8_t channel, uint32_t id, const uint8_t *data, uint8_t len)
{
    MicroCom_Can_RxIndication(channel, id, data, len);
}
```

### 注意事项

- 注册操作请在 `MicroCom_Can_Start()` 之前全部完成；运行期间动态增删报文
  未做同步保护。
- 回调函数（`func`）在 `TimerHandler` / `RxIndication` 的调用上下文中被直接
  执行，请保持回调轻量，避免阻塞操作，否则会影响调度实时性。
- `data` 缓冲区由调用者分配和管理，长度需 `>= dlc`；组件本身不做动态内存
  分配。
- 同一通道内不允许注册两条 ID 相同、方向和类型也相同的报文，查找按"第一个
  匹配"返回，重复注册会导致后一条永远无法被匹配到。

---

## LIN （预留）

> 尚未实现，以下章节结构与 CAN 部分保持一致，供后续填充。

### 功能特性

_TODO_

### 架构设计

#### 数据结构

_TODO_

#### 调度模型

_TODO_

#### 线程安全与中断

_TODO_

### 配置

_TODO_

### API 参考

_TODO_

### 使用示例

_TODO_

### 注意事项

_TODO_