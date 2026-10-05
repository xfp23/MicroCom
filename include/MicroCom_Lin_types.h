/**
 * @file MicroCom_Lin_types.h
 * @author https://xfp23.github.io/
 * @brief MicroCom LIN 调度模块类型定义
 * @version \ref MICROCOM_VERSION
 * @date 2026-09-09
 *
 * @copyright Copyright (c) 2026
 */
#ifndef MICROCOM_LIN_TYPES_H
#define MICROCOM_LIN_TYPES_H

#include "MicroCom_Can_types.h" /* 复用平台共享的 MicroCom_Status_t */
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define MICROCOM_LIN_IDX_INVALID   (0xFFu)

/* ------------------------------------------------------------------------ */
/* 方向                                                                      */
/* ------------------------------------------------------------------------ */

typedef enum
{
    MICROCOM_DIR_TX,
    MICROCOM_DIR_RX,
} MicroCom_DIR_t;

/* ------------------------------------------------------------------------ */
/* 事件                                                                      */
/*                                                                          */
/* LIN 只有周期报文，这里不保留 CAN 那边 EVENT_* 系列事件。                  */
/* ------------------------------------------------------------------------ */

typedef enum
{
    MICROCOM_LIN_EVENT_TX,        /**< 主发帧已成功提交给底层驱动 */
    MICROCOM_LIN_EVENT_TX_DONE,   /**< 底层通知这次发送已结束（不区分成功失败） */
    MICROCOM_LIN_EVENT_TX_ERROR,  /**< 提交失败（MicroCom_Lin_Transmit 直接返回 false） */

    MICROCOM_LIN_EVENT_RX,        /**< 收到一帧有效的从节点响应 */
    MICROCOM_LIN_EVENT_RX_ERROR,  /**< 距上次成功接收已超过用户配置的 timeout */
} MicroCom_Lin_Event_t;

/* ------------------------------------------------------------------------ */
/* 回调上下文 / 回调函数类型                                                 */
/* ------------------------------------------------------------------------ */

typedef struct
{
    MicroCom_Lin_Event_t event;
    uint8_t  channel;
    uint8_t  id;
    uint8_t *data;
    uint8_t  len;
    void    *userData;
} MicroCom_Lin_Ctx_t;

typedef void (*MicroCom_Lin_Func_t)(MicroCom_Lin_Ctx_t *ctx);

/* ------------------------------------------------------------------------ */
/* 注册表（用户填写的配置项）                                                */
/* ------------------------------------------------------------------------ */

typedef struct
{
    uint8_t  channel;
    uint8_t  id;        /**< 6 位逻辑 ID（0x00~0x3F） */
    uint8_t  length;    /**< 数据长度，<= 8 */
    uint8_t *data;       /**< 用户管理的数据缓冲区，长度需 >= length；
                              TX 方向：发送内容由用户自行维护在这块内存里；
                              RX 方向：收到的数据会被写进这块内存 */
    uint32_t period;      /**< 调度周期（单位：tick）。TX：多久发一次；
                              RX：多久轮询（请求）一次从节点 */
    uint32_t timeout;     /**< 仅 RX 方向有意义：距上一次成功收到数据
                              多少 tick 后判定为超时 */
    MicroCom_DIR_t       dir;
    MicroCom_Lin_Func_t  callback;
    void                *userdata;
} MicroCom_Lin_ConfigTable_t;

/* ------------------------------------------------------------------------ */
/* 运行时表项                                                                */
/* ------------------------------------------------------------------------ */

typedef struct
{
    uint8_t  channel;
    uint8_t  id;
    uint8_t  length;
    uint8_t *data;

    uint32_t cycle;          /* 调度周期（tick），TX/RX 通用：TX 是发送周期，
                                 RX 是轮询周期 */
    uint32_t next_time;      /* 下一次应该被调度的 tick，next_time += cycle
                                 推进，避免长期漂移 */

    uint32_t timeout;        /* 仅 RX 有意义：距上一次成功收到数据多少 tick
                                 后判定超时 */
    uint32_t last_rx_time;   /* 仅 RX 有意义：最近一次成功收到数据的 tick */

    MicroCom_DIR_t dir;
    MicroCom_Lin_Func_t callback;
    void *userdata;

    volatile bool is_valid;     /* 该槽位是否已注册 */
    volatile bool is_timeout;   /* 仅 RX：是否已处于超时锁存状态 */

#if MICROCOM_FASTMODE_ENABLE == 0
    volatile bool is_trigger;   /* 仅 RX：是否有一帧已到达、等 TimerHandler 消费 */
#endif
} MicroCom_Lin_Table_t;

/* ------------------------------------------------------------------------ */
/* 模块对象                                                                  */
/* ------------------------------------------------------------------------ */

typedef struct
{
    MicroCom_Lin_Table_t table[MICROCOM_LIN_CHANNEL_NUM][MICROCOM_LIN_MSG_SIZE];
    uint32_t msg_num[MICROCOM_LIN_CHANNEL_NUM];

    /* 每通道同一时刻只能有一路传输在跑，这是和 MicroCom_Can 最大的
     * 调度差异：CAN 每条报文各自独立判断、互不影响；LIN 这里用
     * busy[ch] 把整个通道的调度串成一条队列。 */
    volatile bool    busy[MICROCOM_LIN_CHANNEL_NUM];
    volatile uint8_t active_idx[MICROCOM_LIN_CHANNEL_NUM];   /* 当前在跑的是哪个槽位 */
    uint8_t           scan_cursor[MICROCOM_LIN_CHANNEL_NUM]; /* 轮询起点，防止低下标饿死高下标 */

    volatile uint32_t tick;
    volatile bool     enable;
} MicroCom_Lin_Obj_t;

#ifdef __cplusplus
}
#endif

#endif /* MICROCOM_LIN_TYPES_H */