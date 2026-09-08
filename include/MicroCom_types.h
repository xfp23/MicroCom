/**
 * @file MicroCom_types.h
 * @author https://xfp23.github.io/
 * @brief
 * @version 0.2 (重构：TX/RX 分表，去除 dir 维度)
 * @date 2026-09-08
 *
 * @copyright Copyright (c) 2026
 */
#ifndef MICROCOM_TYPES_H
#define MICROCOM_TYPES_H

#include "MicroCom_conf.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

/* ------------------------------------------------------------------------ */
/* 通用类型                                                                  */
/* ------------------------------------------------------------------------ */

typedef enum
{
    MICROCOM_STATUS_OK,
    MICROCOM_STATUS_ERR,
    MICROCOM_STATUS_BUSY,
    MICROCOM_PARAM_INVALID,  /* 参数非法 */
    MICROCOM_CHANNEL_ERR,
    MICROCOM_NOT_FIND,
} MicroCom_Status_t;

typedef enum
{
    MICROCOM_EVENT_TX,
    MICROCOM_EVENT_RX,
    MICROCOM_EVENT_ERROR,
} MicroCom_Event_t;

/**
 * @brief 报文回调上下文
 *
 * 同一个回调函数可能被挂在多条报文上，channel/id 用于在回调内部区分
 * 是哪一条报文触发的，避免必须为每条报文单独写一个回调。
 */
typedef struct
{
    MicroCom_Event_t event;
    uint8_t          channel;
    uint32_t         id;
    void            *userData;
} MicroCom_Ctx_t;

typedef void (*MicroCom_Func_t)(MicroCom_Ctx_t *ctx);

/* ------------------------------------------------------------------------ */
/* 周期性发送报文（TX）                                                      */
/* ------------------------------------------------------------------------ */
typedef struct
{
    /* --- 配置字段：由 Register 接口写入 --- */
    uint32_t id;
    bool     is_Extend;      /* 是否扩展帧 */
    uint8_t  dlc;            /* 报文长度，<= MICROCOM_CAN_MAX_DLC */
    uint16_t mbox_id;        /* 报文邮箱号 */
    uint8_t  channel;        /* 通道 */
    bool     is_diag;        /* 是否诊断通信 */
    uint32_t cycle;          /* 发送周期（单位：tick） */

    uint8_t *data;           /* 用户管理的数据缓冲区，长度需 >= dlc */
    void    *userData;       /* 回调用户数据 */
    MicroCom_Func_t func;    /* 每次发送后的回调，可为 NULL */

    /* --- 运行时字段：内部维护，注册时会被强制初始化，用户无需填写 --- */
    volatile bool     is_run;    /* 是否参与调度 */
    volatile uint32_t next_time; /* 下一次应发送的 tick（内部用，避免周期漂移） */
} MicroCom_CanCycleTxMsg_t;

/* ------------------------------------------------------------------------ */
/* 周期性接收报文（RX，带超时监控）                                          */
/* ------------------------------------------------------------------------ */
typedef struct
{
    uint32_t id;
    bool     is_Extend;
    uint8_t  dlc;
    uint16_t mbox_id;
    uint8_t  channel;
    bool     is_diag;
    uint32_t timeout;        /* 接收超时（单位：tick） */

    uint8_t *data;
    void    *userData;
    MicroCom_Func_t func;    /* 收到报文 / 超时 均会回调，通过 ctx.event 区分 */

    volatile bool     is_run;
    volatile bool     busoff;      /* true 表示超时 / 总线错误 */
    volatile uint32_t last_rx_time;/* 最近一次收到报文（或重置）时的 tick */
} MicroCom_CanCycleRxMsg_t;

/* ------------------------------------------------------------------------ */
/* 事件触发发送报文（TX）                                                    */
/* ------------------------------------------------------------------------ */
typedef struct
{
    uint32_t id;
    bool     is_Extend;
    uint8_t  dlc;
    uint16_t mbox_id;
    uint8_t  channel;
    bool     is_diag;

    uint8_t *data;
    void    *userData;
    MicroCom_Func_t func;

    volatile bool     is_run;
    volatile uint16_t trigger; /* 待发送次数；Trigger 接口 ++，TimerHandler --。
                                   跨上下文读改写，访问时须加临界区保护 */
} MicroCom_CanEventTxMsg_t;

/* ------------------------------------------------------------------------ */
/* 事件接收报文（RX）                                                        */
/* ------------------------------------------------------------------------ */
typedef struct
{
    uint32_t id;
    bool     is_Extend;
    uint8_t  dlc;
    uint16_t mbox_id;
    uint8_t  channel;
    bool     is_diag;

    uint8_t *data;
    void    *userData;
    MicroCom_Func_t func;

    volatile bool is_run;
    volatile bool busoff;   /* 事件报文无自动超时机制，由 Set/ClearEventBusOff 管理 */
} MicroCom_CanEventRxMsg_t;

/* ------------------------------------------------------------------------ */
/* 模块对象                                                                  */
/* ------------------------------------------------------------------------ */
typedef struct
{
    MicroCom_CanCycleTxMsg_t CycleTx[MICROCOM_CAN_CHANNEL_NUM][MICROCOM_CAN_CYCLEMSG_SIZE];
    MicroCom_CanCycleRxMsg_t CycleRx[MICROCOM_CAN_CHANNEL_NUM][MICROCOM_CAN_CYCLEMSG_SIZE];
    MicroCom_CanEventTxMsg_t EventTx[MICROCOM_CAN_CHANNEL_NUM][MICROCOM_CAN_EVENTMSG_SIZE];
    MicroCom_CanEventRxMsg_t EventRx[MICROCOM_CAN_CHANNEL_NUM][MICROCOM_CAN_EVENTMSG_SIZE];

    volatile uint32_t tick;
    volatile bool     enable;
} MicroCOM_CAN_Obj_t;

#ifdef __cplusplus
}
#endif

#endif /* MICROCOM_TYPES_H */