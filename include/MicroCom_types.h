/**
 * @file MicroCom_types.h
 * @author https://xfp23.github.io/
 * @brief
 * @version 0.1
 * @date 2026-09-04
 *
 * @copyright Copyright (c) 2026
 *
 */
#ifndef MICROCOM_TYPES_H
#define MICROCOM_TYPES_H

#include "MicroCom_conf.h"
#include "MicroCom_utils.h"
#include "stdbool.h"
#include "string.h"
#include "stdlib.h"
#include "stdint.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef enum
{
    MICROCOM_STATUS_OK,
    MICROCOM_STATUS_ERR,
    MICROCOM_STATUS_BUSY,
    MICROCOM_PARAM_INVALID, // Invalid parameter
} MicroCom_Status_t;

typedef enum
{
    MICROCOM_MSGTYPE_CYCLE,                           // 周期性报文
    MICROCOM_MSGTYPE_EVENT,                           // 事件触发性报文
    MICROCOM_MSGTYPE_NORMAL = MICROCOM_MSGTYPE_CYCLE, // 默认是周期性报文
} MicroCom_MsgType_t;                                 // 报文类型

typedef enum
{
    MICROCOM_DIR_TRANSMIT,
    MICROCOM_DIR_RECEIVE,
} MicroCOM_DIR_t; // direction

typedef struct
{
    uint32_t id;                 // id
    bool is_Exted;               // 是否扩展帧
    uint32_t cycle;              // 报文周期
    MicroCom_MsgType_t msg_type; // 报文类型
    MicroCOM_DIR_t dir;          // 报文方向
    uint8_t *data;               // 不存储数据，让用户自己管理数据
    uint8_t dlc;                 // 报文长度
    volatile bool busoff;        // 总线错误
    uint16_t mbox_id;            // 报文邮箱号

    uint8_t chanel; // 通道

} MicroCom_CanMessage_t;

typedef struct
{
    MicroCom_CanMessage_t msg[MICROCOM_CAN_MESSAGE_SIZE];

} MicroCOM_CAN_Obj_t;

typedef struct
{
    uint8_t a;
} MicroCOM_CANFD_Obj_t;

typedef struct
{
    uint8_t b;
} MicroCOM_LIN_Obj_t;

#ifdef __cplusplus
}
#endif

#endif
