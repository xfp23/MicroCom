# Communication Scheduling Component (CAN / LIN)

[中文文档](./readme.zh.md)

MicroCom is a bus-message scheduling component for embedded MCUs (primarily
targeting Cortex-M4). It handles periodic message transmission, receive
timeout monitoring, on-demand event-message triggering, and uniformly
suspending/resuming non-diagnostic traffic during a diagnostic session.

---

## Directory Structure

```
MicroCom_conf.h    # Compile-time config: channel count, table sizes, critical-section macros
MicroCom_types.h   # Data type definitions
MicroCom_utils.h    # Parameter-check macros
MicroCom_Can.h      # CAN module public API
MicroCom_Can.c      # CAN module implementation
MicroCom_Lin.h      # (Reserved) LIN module public API
MicroCom_Lin.c      # (Reserved) LIN module implementation
```

> LIN-related files and the corresponding documentation section below are
> placeholders and have not been implemented yet.

---

## CAN

### Features

- Split into four independent tables by direction: periodic TX (CycleTx),
  periodic RX (CycleRx), event TX (EventTx), event RX (EventRx); each
  table is managed per channel.
- Periodic TX messages are sent automatically when due; the internal
  baseline advances via `time += cycle` to avoid long-term period drift.
- Periodic RX messages carry timeout monitoring; on timeout, `busoff` is
  set automatically and the user callback fires.
- Event messages are triggered on demand via a `trigger` counter, safe to
  call across interrupt/task contexts.
- All non-diagnostic messages (`is_diag == false`) on a channel can be
  suspended/resumed in one call during a diagnostic session.
- A unified callback context `MicroCom_Ctx_t` (carrying
  `event / channel / id / userData`) lets a single callback serve multiple
  messages.

### Architecture

#### Data Structures

| Struct                        | Direction    | Key fields                                    |
| ------------------------------ | ------------ | ----------------------------------------------- |
| `MicroCom_CanCycleTxMsg_t`    | TX (cyclic)  | `cycle`, `next_time`, `is_run`                  |
| `MicroCom_CanCycleRxMsg_t`    | RX (cyclic)  | `timeout`, `last_rx_time`, `busoff`, `is_run`   |
| `MicroCom_CanEventTxMsg_t`    | TX (event)   | `trigger`, `is_run`                             |
| `MicroCom_CanEventRxMsg_t`    | RX (event)   | `busoff`, `is_run`                              |

All four tables are organized as `[channel][index]` 2D arrays, where
`index` is the entry's global position in the registration table (no
collision within a channel, but each channel reserves a full
`MICROCOM_CAN_CYCLEMSG_SIZE` / `MICROCOM_CAN_EVENTMSG_SIZE` worth of slots —
a deliberate space-for-simplicity trade-off).

#### Scheduling Model

```
MicroCom_Can_TickHandler()   -> Called once per scheduling tick, advances can_obj.tick

MicroCom_Can_TimerHandler()  -> Called periodically; drives TX transmission,
                                 RX timeout detection, and event-TX consumption

MicroCom_Can_RxIndication()  -> Called from the CAN RX interrupt/task with an
                                 incoming frame
```

`TickHandler` and `TimerHandler` can be called sequentially from the same
interrupt, or independently at different rates (e.g. `TickHandler` on a 1ms
SysTick, `TimerHandler` at a lower rate from the main loop).

#### Concurrency & Interrupt Safety

| Shared state           | Access contexts                                          | Protection                                                |
| ------------------------ | ------------------------------------------------------------ | -------------------------------------------------------------- |
| `tick`                   | Single writer (TickHandler), multiple readers                | Naturally atomic on 32-bit aligned access, no extra protection needed |
| `is_run` / `busoff`      | Multiple writers, single-bool write                          | Atomic single-byte write, no extra protection needed          |
| `trigger`                | `++` in `Trigger_EventMsg`, `--` in `TimerHandler`            | **Read-modify-write, needs a critical section**: `MICROCOM_ENTER/EXIT_CRITICAL()` |

> The `Register_*` functions are **not** interrupt/thread-safe. All
> registration must complete before calling `MicroCom_Can_Start()`; do not
> register while the scheduler is running.

`MICROCOM_ENTER_CRITICAL()` / `MICROCOM_EXIT_CRITICAL()` default to
`__disable_irq()` / `__enable_irq()`. If the project uses an RTOS, override
them in `MicroCom_conf.h` with the RTOS's own critical-section API (e.g.
`taskENTER_CRITICAL()` / `taskENTER_CRITICAL_FROM_ISR()`) to avoid impacting
real-time behavior with a global interrupt disable.

### Configuration

Macros in `MicroCom_conf.h`:

| Macro                        | Description                                                                     | Default |
| ------------------------------ | ----------------------------------------------------------------------------------- | --------- |
| `MICROCOM_CAN_CHANNEL_NUM`   | Number of CAN channels                                                             | `2`      |
| `MICROCOM_CAN_CYCLEMSG_SIZE` | Slots per channel for periodic messages (TX and RX each have their own)            | `16`     |
| `MICROCOM_CAN_EVENTMSG_SIZE` | Slots per channel for event messages (TX and RX each have their own)               | `16`     |
| `MICROCOM_CAN_MAX_DLC`       | Max frame length; 8 for classic CAN, up to 64 for CAN FD                           | `8`      |
| `MICROCOM_ENTER_CRITICAL()`  | Enter critical section                                                              | `__disable_irq()` |
| `MICROCOM_EXIT_CRITICAL()`   | Exit critical section                                                               | `__enable_irq()`  |

### API Reference

| Function                                            | Description                                                                     |
| ------------------------------------------------------ | ------------------------------------------------------------------------------------ |
| `MicroCom_Can_Init(void)`                               | Module init, call first, once                                                       |
| `MicroCom_Can_Start(void)`                              | Start scheduling, aligns all message baselines                                      |
| `MicroCom_Can_Stop(void)`                               | Stop scheduling                                                                     |
| `MicroCom_Can_Register_CycleTxMsg(table, size)`         | Register periodic TX messages                                                       |
| `MicroCom_Can_Register_CycleRxMsg(table, size)`         | Register periodic RX messages                                                       |
| `MicroCom_Can_Register_EventTxMsg(table, size)`         | Register event TX messages                                                          |
| `MicroCom_Can_Register_EventRxMsg(table, size)`         | Register event RX messages                                                          |
| `MicroCom_Can_TickHandler(void)`                        | Advance the scheduling tick                                                         |
| `MicroCom_Can_TimerHandler(void)`                       | Periodic scheduling (TX send / RX timeout / event consumption)                      |
| `MicroCom_Can_RxIndication(channel, id, data, len)`     | Feed in one received frame                                                          |
| `MicroCom_Can_Trigger_EventMsg(id, channel)`            | Trigger one event TX message                                                        |
| `MicroCom_Can_SetEventOffline(id, channel)`              | Set busoff on an event RX message                                                   |
| `MicroCom_Can_ClearEventBusOff(id, channel)`            | Clear busoff on an event RX message                                                 |
| `MicroCom_Can_DisableNonDiagnosticCom(channel)`         | Suspend all non-diagnostic messages on a channel                                    |
| `MicroCom_Can_EnableNonDiagnosticCom(channel)`          | Resume all non-diagnostic messages on a channel                                     |
| `MicroCom_Can_Transmit(channel, id, mbox, dlc, data, is_extend)` | Low-level TX hook (weak symbol, implemented by the CAN driver)             |

### Usage Example

```c
#include "MicroCom_Can.h"

static uint8_t rx_buf_100[8];

static void OnMsg100(MicroCom_Ctx_t *ctx)
{
    if (ctx->event == MICROCOM_EVENT_RX)
    {
        /* rx_buf_100 has been updated */
    }
    else if (ctx->event == MICROCOM_EVENT_ERROR)
    {
        /* receive timeout */
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
    /* ... register other tables ... */
    MicroCom_Can_Start();
}

/* 1ms timer interrupt */
void SysTick_Handler(void)
{
    MicroCom_Can_TickHandler();
    MicroCom_Can_TimerHandler();
}

/* CAN RX interrupt */
void CAN_RxCallback(uint8_t channel, uint32_t id, const uint8_t *data, uint8_t len)
{
    MicroCom_Can_RxIndication(channel, id, data, len);
}
```

### Notes & Constraints

- Complete all registration before calling `MicroCom_Can_Start()`; dynamic
  add/remove of messages at runtime is not synchronized.
- Callback functions (`func`) execute directly within the calling context of
  `TimerHandler` / `RxIndication`. Keep callbacks lightweight and non-blocking
  to avoid impacting scheduling real-time behavior.
- The `data` buffer is allocated and owned by the caller and must be
  `>= dlc` bytes; the component itself performs no dynamic allocation.
- Do not register two entries with the same ID, direction, and type on the
  same channel; lookups return the first match, so a duplicate will never be
  reachable.

---

## LIN (Reserved)

> Not yet implemented. The section structure below mirrors the CAN section
> and is reserved for future content.

### Features

_TODO_

### Architecture

#### Data Structures

_TODO_

#### Scheduling Model

_TODO_

#### Concurrency & Interrupt Safety

_TODO_

### Configuration

_TODO_

### API Reference

_TODO_

### Usage Example

_TODO_

### Notes & Constraints

_TODO_