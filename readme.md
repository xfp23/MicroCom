# MicroCom — CAN / LIN Message Scheduling

[中文](./readme.zh.md)

MicroCom is a lightweight static message scheduling component for embedded
MCUs, primarily targeting Cortex-M based systems.

It provides unified scheduling for CAN and LIN communication, including
cyclic transmission, cyclic reception monitoring, event-driven transmission,
reception indication, timeout handling, and diagnostic communication
suspension.

MicroCom does not implement a CAN or LIN hardware driver. Instead, the
hardware layer is connected through a small set of driver hooks, allowing the
same scheduling layer to be reused across different MCUs and peripherals.

---

## Features

- CAN cyclic TX/RX scheduling
- CAN event-driven TX/RX handling
- LIN cyclic master TX/RX scheduling
- LIN identifier and protected identifier handling
- RX timeout monitoring
- TX completion notification
- RX indication and RX error notification
- Diagnostic communication suspension and recovery
- Static configuration without dynamic memory allocation
- Configurable channel and message-table capacity
- Hardware-independent scheduling layer
- Weak driver hooks for MCU-specific integration
- Unified callback context for message events
- Optional fast callback mode
- C / C++ compatible public interfaces

---

## Architecture

MicroCom is divided into three logical layers:

```text
+------------------------------------------------------+
|                    Application                       |
|                                                      |
|  Message callbacks / Diagnostic / Application logic  |
+---------------------------+--------------------------+
                            |
                            v
+------------------------------------------------------+
|                    MicroCom                          |
|                                                      |
|  +----------------+       +-----------------------+  |
|  | MicroCom_Can   |       | MicroCom_Lin          |  |
|  |                |       |                       |  |
|  | Cycle TX / RX  |       | Frame Scheduling      |  |
|  | Event TX / RX  |       | TX / RX               |  |
|  | Timeout        |       | Timeout               |  |
|  | Diagnostics    |       | TX Done / RX Error     |  |
|  +----------------+       +-----------------------+  |
+---------------------------+--------------------------+
                            |
                            v
+------------------------------------------------------+
|                 Hardware Driver                     |
|                                                      |
|        CAN peripheral / LIN peripheral               |
|        MCU-specific implementation                   |
+------------------------------------------------------+
````

The MicroCom scheduler does not directly access MCU registers.

The hardware driver is responsible for the actual bus operation, while
MicroCom is responsible for message scheduling and state management.

---

## Initialization and Lifecycle

Both CAN and LIN follow the same basic lifecycle:

```text
Init
  |
  v
Register Configuration
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

Configuration registration must be completed before `Start()`.

Runtime registration is not supported.

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

## CAN Features

MicroCom CAN provides four independent message categories:

```text
Cycle TX
Cycle RX
Event TX
Event RX
```

Each category has its own configuration table and scheduler state.

### Cyclic TX

A cyclic TX message is transmitted periodically according to its configured
cycle time.

The scheduler advances the next transmission time using the configured
cycle period rather than recalculating it from the current time. This avoids
long-term timing drift.

```text
next_time += cycle
```

### Cyclic RX

A cyclic RX message defines an expected reception period or timeout.

If no matching frame is received within the configured timeout, the message
is marked offline and the corresponding callback event is generated.

### Event TX

An event TX message is transmitted on demand.

The application can trigger an event message through:

```c
MicroCom_Can_Trigger_EventTxMsg(...)
```

The scheduler consumes the pending trigger during normal scheduling.

### Event RX

Event RX messages are monitored for reception and offline state.

---

## CAN Message Registration

CAN configuration is divided into four independent registration operations:

```c
MicroCom_Can_Register_CycleTxMsg(...)
MicroCom_Can_Register_CycleRxMsg(...)

MicroCom_Can_Register_EventTxMsg(...)
MicroCom_Can_Register_EventRxMsg(...)
```

The registration process validates the complete configuration before modifying
the internal scheduler state.

A failed registration does not partially modify the registered configuration.

All registration must be completed before:

```c
MicroCom_Can_Start();
```

---

## CAN Scheduler

The scheduler consists of two handlers.

### TickHandler

```c
MicroCom_Can_TickHandler();
```

`TickHandler()` advances the internal scheduler time base.

It is typically called from a fixed-frequency timer interrupt.

For example:

```text
1 ms timer interrupt
        |
        v
MicroCom_Can_TickHandler()
```

### TimerHandler

```c
MicroCom_Can_TimerHandler();
```

`TimerHandler()` performs the actual scheduling work.

It is responsible for:

* Starting due cyclic TX transmissions
* Processing cyclic RX timeout conditions
* Processing event TX triggers
* Updating message scheduling state

`TickHandler()` and `TimerHandler()` do not have to execute in the same
context or at the same frequency.

For example:

```text
SysTick 1 ms
    |
    +--> TickHandler()

Main loop
    |
    +--> TimerHandler()
```

---

## CAN Receive Path

The CAN hardware driver calls:

```c
MicroCom_Can_RxIndication(
    channel,
    can_id,
    is_extend,
    data,
    len
);
```

when a CAN frame has been received.

The scheduler then searches for the corresponding configured message and
updates its reception state.

The application callback is subsequently invoked according to the configured
callback policy.

```text
CAN Hardware
     |
     v
CAN Driver
     |
     v
MicroCom_Can_RxIndication()
     |
     v
Message Matching
     |
     v
Message State Update
     |
     v
Application Callback
```

---

## CAN Transmission Path

MicroCom does not directly access CAN hardware.

When a message needs to be transmitted, the scheduler calls:

```c
MicroCom_Can_Transmit(...)
```

This function is provided as a weak hardware hook.

A device-specific CAN driver provides the actual implementation.

```text
MicroCom Scheduler
       |
       v
MicroCom_Can_Transmit()
       |
       v
CAN Driver
       |
       v
CAN Peripheral
```

After the hardware transmission is completed, the driver calls:

```c
MicroCom_Can_HwTxDone(...)
```

This allows MicroCom to distinguish between:

```text
Transmission requested
        !=
Transmission completed
```

---

## CAN Event Callback

CAN callbacks receive a common context:

```c
typedef struct
{
    MicroCom_Can_Event_t event;
    uint8_t              channel;
    uint16_t             mboxId;
    bool                  is_extend;
    uint32_t              id;
    uint8_t              *data;
    uint8_t              len;
    void                  *userData;
} MicroCom_Can_Ctx_t;
```

A single callback can therefore be shared by multiple messages.

The callback can identify the source message through:

```c
ctx->channel
ctx->id
ctx->mboxId
ctx->is_extend
```

The event field identifies the reason for the callback.

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

## CAN Diagnostic Communication Control

MicroCom can suspend all non-diagnostic communication on a specific CAN
channel.

```c
MicroCom_Can_DisableNonDiagnosticCom(channel);
```

Messages configured as diagnostic messages are not affected.

Communication can later be restored with:

```c
MicroCom_Can_EnableNonDiagnosticCom(channel);
```

This mechanism is intended for diagnostic sessions where normal application
communication needs to be temporarily suspended.

---

## CAN Offline State

The offline state of a cyclic RX message can be queried using:

```c
MicroCom_Can_IsCycleRxBusOffline(...)
```

The state can be used by the application to determine whether a cyclic CAN
message is currently considered available.

---

# LIN

MicroCom LIN provides static scheduling for LIN master communication.

The LIN module uses the same general lifecycle as the CAN module:

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

Unlike CAN, LIN communication is master-driven.

The scheduler therefore decides whether the next frame is a master transmit
frame or a master receive frame.

---

## LIN Frame Model

MicroCom exposes the logical LIN identifier to the configuration layer.

The user configures:

```text
ID
Data Field
Length
```

The Protected Identifier (PID) is not configured directly.

The LIN driver or protocol layer derives the PID from the 6-bit identifier.

```text
6-bit ID
   |
   v
Parity calculation
   |
   v
8-bit PID
```

The valid LIN identifier range is:

```text
0x00 ~ 0x3F
```

The LIN data field supports up to:

```text
8 bytes
```

---

## LIN Message Configuration

LIN messages are registered through a single configuration table:

```c
MicroCom_Lin_RegisterTable(...)
```

The registration process performs complete validation before committing the
configuration.

If any configuration entry is invalid, the internal LIN scheduler state is
not modified.

All registration must be completed before:

```c
MicroCom_Lin_Start();
```

---

## LIN Scheduling

The LIN scheduler uses:

```c
MicroCom_Lin_TickHandler();
MicroCom_Lin_TimerHandler();
```

### TickHandler

`MicroCom_Lin_TickHandler()` advances the scheduler time base.

It should be called at the configured scheduler frequency.

### TimerHandler

`MicroCom_Lin_TimerHandler()` performs the actual scheduling.

For each LIN channel, the scheduler searches for the next frame whose
scheduled time has arrived.

Depending on the configured frame direction, it initiates either:

```text
Master TX
```

or:

```text
Master RX
```

The scheduler also monitors pending RX frames for timeout.

---

## LIN Hardware Interface

MicroCom LIN uses two hardware submission hooks:

```c
MicroCom_Lin_Transmit(...)
MicroCom_Lin_Receive(...)
```

Both functions are weak default implementations.

A device-specific LIN driver provides strong implementations.

### Master TX

```c
MicroCom_Lin_Transmit(...)
```

requests the hardware driver to:

```text
Transmit LIN Break
      |
      v
Transmit Sync
      |
      v
Transmit PID
      |
      v
Transmit configured response data
```

The function is asynchronous.

A successful return means only that the request has been accepted by the
driver.

It does not mean that transmission has completed.

### Master RX

```c
MicroCom_Lin_Receive(...)
```

requests the hardware driver to:

```text
Transmit LIN Break
      |
      v
Transmit Sync
      |
      v
Transmit PID
      |
      v
Wait for slave response
```

The received data is returned asynchronously through:

```c
MicroCom_Lin_RxIndication(...)
```

---

## LIN Driver Notification

After a transmission has physically completed, the hardware driver calls:

```c
MicroCom_Lin_TxDone(...)
```

After a valid response has been received:

```c
MicroCom_Lin_RxIndication(...)
```

If reception fails:

```c
MicroCom_Lin_RxError(...)
```

The dependency direction is therefore:

```text
+------------------------+
|   MicroCom LIN         |
|                        |
| Scheduler / State      |
+-----------+------------+
            |
            | calls
            v
+------------------------+
| MicroCom_Lin_Transmit  |
| MicroCom_Lin_Receive   |
+-----------+------------+
            |
            v
+------------------------+
| Device LIN Driver      |
+-----------+------------+
            |
            | calls back
            v
+------------------------+
| MicroCom_Lin_TxDone    |
| MicroCom_Lin_RxIndication|
| MicroCom_Lin_RxError   |
+------------------------+
```

This allows the LIN scheduler to remain independent of the actual MCU
peripheral.

---

## LIN Callback Context

LIN callbacks receive:

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

The event type is:

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

The callback can use:

```c
ctx->event
ctx->channel
ctx->id
ctx->data
ctx->len
ctx->userData
```

to determine the source and result of the LIN operation.

---

# Configuration

The main compile-time configuration is located in:

```text
MicroCom_conf.h
```

Typical configuration includes:

| Macro                           | Description                        |
| ------------------------------- | ---------------------------------- |
| `MICROCOM_VERSION`              | MicroCom version                   |
| `MICROCOM_FREQ_HZ`              | Scheduler frequency                |
| `MICROCOM_FASTMODE_ENABLE`      | Enable fast callback mode          |
| `MICROCOM_CAN_EVENTMSG_ENABLE`  | Enable CAN event messages          |
| `MICROCOM_CAN_CHANNEL_NUM`      | Number of CAN channels             |
| `MICROCOM_CAN_CYCLEMSG_TX_SIZE` | Cyclic CAN TX capacity per channel |
| `MICROCOM_CAN_CYCLEMSG_RX_SIZE` | Cyclic CAN RX capacity per channel |
| `MICROCOM_CAN_EVENTMSG_TX_SIZE` | Event CAN TX capacity per channel  |
| `MICROCOM_CAN_EVENTMSG_RX_SIZE` | Event CAN RX capacity per channel  |
| `MICROCOM_CAN_MAX_DLC`          | Maximum CAN data length            |
| `MICROCOM_LIN_CHANNEL_NUM`      | Number of LIN channels             |
| `MICROCOM_LIN_MSG_SIZE`         | LIN message capacity per channel   |

MicroCom does not allocate message storage dynamically at runtime.

The configured capacities determine the static memory footprint of the
component.

---

# Status Codes

MicroCom APIs use:

```c
MicroCom_Status_t
```

The currently defined status values are:

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

These status values are shared by the CAN and LIN modules.

---

# Thread Safety and Interrupt Context

MicroCom is designed for small embedded systems and does not require dynamic
memory allocation.

The application should distinguish between:

```text
Configuration context
Runtime scheduler context
Hardware interrupt context
```

### Configuration

Registration APIs should only be called before `Start()`.

They are not intended to be called concurrently with the scheduler.

### Runtime

`TickHandler()` and `TimerHandler()` operate on scheduler state.

If they are executed from different contexts, the application must ensure
that the selected execution model provides the required synchronization.

### Hardware callbacks

Functions such as:

```c
MicroCom_Can_RxIndication()
MicroCom_Can_HwTxDone()

MicroCom_Lin_TxDone()
MicroCom_Lin_RxIndication()
MicroCom_Lin_RxError()
```

may be called from interrupt context depending on the hardware driver.

Callbacks should therefore remain short and non-blocking.

---

# Memory Model

MicroCom uses static memory.

The component does not use:

```c
malloc()
calloc()
realloc()
free()
```

Message capacity is determined at compile time through `MicroCom_conf.h`.

This provides:

* Deterministic memory usage
* No heap fragmentation
* Predictable runtime behavior
* Easier RAM estimation
* Easier embedded configuration

The actual RAM consumption depends on:

* Number of CAN channels
* Number of CAN message slots
* Number of LIN channels
* Number of LIN message slots
* Configuration structure sizes
* Callback and user-data storage

---

# Example

A minimal CAN initialization:

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

A typical scheduler integration:

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

The actual execution frequency of `TimerHandler()` may be different from the
tick frequency, depending on the application's scheduling requirements.

---

# Directory Structure

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

The exact directory organization may vary depending on the integration
project.

---

# Design Principles

MicroCom follows several core design principles:

### Static over dynamic

All message capacities are known at compile time.

### Scheduler over hardware

MicroCom decides **when** a message should be transmitted or processed.

The hardware driver decides **how** the frame is physically transmitted.

### Explicit asynchronous completion

Submitting a transmission request does not mean that transmission has
completed.

Hardware completion is explicitly reported through a callback.

### Hardware independence

The scheduler does not depend on a specific CAN or LIN peripheral.

The same MicroCom implementation can therefore be reused across different
MCUs.

### Configuration before runtime

Message tables are registered during initialization and remain fixed during
normal runtime.

This keeps the runtime scheduler simple and deterministic.

---

# License

Copyright (c) 2026.

[LICENSE](./LICENSE)
