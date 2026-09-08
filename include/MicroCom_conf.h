/**
 * @file MicroCom_conf.h
 * @brief MicroCom CAN 模块编译期配置
 *
 * @copyright Copyright (c) 2026
 */
#ifndef MICROCOM_CONF_H
#define MICROCOM_CONF_H

#ifdef __cplusplus
extern "C"
{
#endif

/* ------------------------------------------------------------------------ */
/* 容量配置                                                                  */
/* ------------------------------------------------------------------------ */

/** CAN 通道数量（0..N-1） */
#define MICROCOM_CAN_CHANNEL_NUM      (2u)

/** 每个通道的周期报文槽位数（TX 表与 RX 表各自独立拥有这么多槽位） */
#define MICROCOM_CAN_CYCLEMSG_SIZE    (16u)

/** 每个通道的事件报文槽位数（TX 表与 RX 表各自独立拥有这么多槽位） */
#define MICROCOM_CAN_EVENTMSG_SIZE    (16u)

/** 单帧最大数据长度：经典 CAN = 8，如需 CAN FD 请改为 64 */
#define MICROCOM_CAN_MAX_DLC          (8u)

/* ------------------------------------------------------------------------ */
/* 临界区保护                                                                */
/*                                                                          */
/* trigger 计数、Start() 时刻的批量时间重置等操作，是跨中断/任务上下文的     */
/* “读-改-写”，在 Cortex-M4 上单条 volatile 读或写虽然是原子的，但          */
/* “读-改-写”序列（例如 trigger++ / trigger--）不是，必须加临界区保护。     */
/*                                                                          */
/* 默认实现直接关/开全局中断，适用于裸机工程。若工程使用 RTOS               */
/* （FreeRTOS/RTX等），请在包含本文件之前 #define 覆盖以下两个宏，          */
/* 换成对应 RTOS 的临界区 API（如 taskENTER_CRITICAL/taskEXIT_CRITICAL，    */
/* 或中断安全版本 taskENTER_CRITICAL_FROM_ISR），避免全局关中断影响实时性。 */
/* ------------------------------------------------------------------------ */
// #ifndef MICROCOM_ENTER_CRITICAL
// #include "cmsis_compiler.h" /* 提供 __disable_irq / __enable_irq，若工程已包含
//                                CMSIS 头文件，可删除本行 */
// #define MICROCOM_ENTER_CRITICAL()  do { __disable_irq(); } while (0)
// #endif

// #ifndef MICROCOM_EXIT_CRITICAL
// #define MICROCOM_EXIT_CRITICAL()   do { __enable_irq(); } while (0)
// #endif

#ifdef __cplusplus
}
#endif

#endif /* MICROCOM_CONF_H */