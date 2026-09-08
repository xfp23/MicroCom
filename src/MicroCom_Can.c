/**
 * @file MicroCom_Can.c
 * @author https://xfp23.github.io/
 * @brief Implementation of MicroCom's CAN module
 * @version 0.1
 * @date 2026-09-08
 * 
 * @copyright Copyright (c) 2026
 * 
 */
#include "MicroCom_Can.h"
#include "MicroCom_utils.h"
#include <string.h>

static MicroCOM_CAN_Obj_t can_obj = {0};

/* ========================================================================= */
/* 内部小工具                                                                 */
/* ========================================================================= */

/** 构造回调上下文并调用（若 func 非空） */
static inline void MicroCom_Can_Invoke(MicroCom_Func_t func, void *userData,uint8_t channel, uint32_t id, MicroCom_Event_t event)
{
    if (func != NULL)
    {
        MicroCom_Ctx_t ctx;
        ctx.event = event;
        ctx.channel = channel;
        ctx.id = id;
        ctx.userData = userData;
        func(&ctx);
    }
}

/* ========================================================================= */
/* 生命周期                                                                   */
/* ========================================================================= */

MicroCom_Status_t MicroCom_Can_Init(void)
{
    memset(&can_obj, 0, sizeof(MicroCOM_CAN_Obj_t));
    return MICROCOM_STATUS_OK;
}

void MicroCom_Can_Start(void)
{
    uint32_t now;

    // MICROCOM_ENTER_CRITICAL();
    now = can_obj.tick;

    for (uint8_t ch = 0; ch < MICROCOM_CAN_CHANNEL_NUM; ch++)
    {
        for (uint32_t i = 0; i < MICROCOM_CAN_CYCLEMSG_SIZE; i++)
        {
            can_obj.CycleTx[ch][i].next_time = now;
            can_obj.CycleRx[ch][i].last_rx_time = now;
        }
    }

    can_obj.enable = true;
    // MICROCOM_EXIT_CRITICAL();
}

void MicroCom_Can_Stop(void)
{
    can_obj.enable = false;
}

/* ========================================================================= */
/* 注册                                                                       */
/* ========================================================================= */

MicroCom_Status_t MicroCom_Can_Register_CycleTxMsg(const MicroCom_CanCycleTxMsg_t *table, size_t size)
{
    MICROCOM_CHECK_PTR(table);
    MICROCOM_CHECK_CYCLE_CAN_SIZE(size);

    for (size_t i = 0; i < size; i++)
    {
        MICROCOM_CHECK_CAN_CHANNEL(table[i].channel);
        MICROCOM_CHECK_DLC(table[i].dlc);

        MicroCom_CanCycleTxMsg_t *slot = &can_obj.CycleTx[table[i].channel][i];

        slot->id = table[i].id;
        slot->is_Extend = table[i].is_Extend;
        slot->dlc = table[i].dlc;
        slot->mbox_id = table[i].mbox_id;
        slot->channel = table[i].channel;
        slot->is_diag = table[i].is_diag;
        slot->cycle = table[i].cycle;
        slot->data = table[i].data;
        slot->userData = table[i].userData;
        slot->func = table[i].func;

        slot->is_run = true;
        slot->next_time = can_obj.tick;
    }

    return MICROCOM_STATUS_OK;
}

MicroCom_Status_t MicroCom_Can_Register_CycleRxMsg(const MicroCom_CanCycleRxMsg_t *table, size_t size)
{
    MICROCOM_CHECK_PTR(table);
    MICROCOM_CHECK_CYCLE_CAN_SIZE(size);

    for (size_t i = 0; i < size; i++)
    {
        MICROCOM_CHECK_CAN_CHANNEL(table[i].channel);
        MICROCOM_CHECK_DLC(table[i].dlc);

        MicroCom_CanCycleRxMsg_t *slot = &can_obj.CycleRx[table[i].channel][i];

        slot->id = table[i].id;
        slot->is_Extend = table[i].is_Extend;
        slot->dlc = table[i].dlc;
        slot->mbox_id = table[i].mbox_id;
        slot->channel = table[i].channel;
        slot->is_diag = table[i].is_diag;
        slot->timeout = table[i].timeout;
        slot->data = table[i].data;
        slot->userData = table[i].userData;
        slot->func = table[i].func;

        slot->is_run = true;
        slot->busoff = false;
        slot->last_rx_time = can_obj.tick;
    }

    return MICROCOM_STATUS_OK;
}

MicroCom_Status_t MicroCom_Can_Register_EventTxMsg(const MicroCom_CanEventTxMsg_t *table, size_t size)
{
    MICROCOM_CHECK_PTR(table);
    MICROCOM_CHECK_EVENT_CAN_SIZE(size);

    for (size_t i = 0; i < size; i++)
    {
        MICROCOM_CHECK_CAN_CHANNEL(table[i].channel);
        MICROCOM_CHECK_DLC(table[i].dlc);

        MicroCom_CanEventTxMsg_t *slot = &can_obj.EventTx[table[i].channel][i];

        slot->id = table[i].id;
        slot->is_Extend = table[i].is_Extend;
        slot->dlc = table[i].dlc;
        slot->mbox_id = table[i].mbox_id;
        slot->channel = table[i].channel;
        slot->is_diag = table[i].is_diag;
        slot->data = table[i].data;
        slot->userData = table[i].userData;
        slot->func = table[i].func;

        slot->is_run = true;
        slot->trigger = 0;
    }

    return MICROCOM_STATUS_OK;
}

MicroCom_Status_t MicroCom_Can_Register_EventRxMsg(const MicroCom_CanEventRxMsg_t *table, size_t size)
{
    MICROCOM_CHECK_PTR(table);
    MICROCOM_CHECK_EVENT_CAN_SIZE(size);

    for (size_t i = 0; i < size; i++)
    {
        MICROCOM_CHECK_CAN_CHANNEL(table[i].channel);
        MICROCOM_CHECK_DLC(table[i].dlc);

        MicroCom_CanEventRxMsg_t *slot = &can_obj.EventRx[table[i].channel][i];

        slot->id = table[i].id;
        slot->is_Extend = table[i].is_Extend;
        slot->dlc = table[i].dlc;
        slot->mbox_id = table[i].mbox_id;
        slot->channel = table[i].channel;
        slot->is_diag = table[i].is_diag;
        slot->data = table[i].data;
        slot->userData = table[i].userData;
        slot->func = table[i].func;

        slot->is_run = true;
        slot->busoff = false;
    }

    return MICROCOM_STATUS_OK;
}

/* ========================================================================= */
/* Tick / Timer                                                              */
/* ========================================================================= */

void MicroCom_Can_TickHandler(void)
{
    /* can_obj.tick 是自然对齐的 32bit volatile 变量，在 Cortex-M4 上单条
     * load/store 就是一次总线事务，读写本身是原子的，这里只是简单自增，
     * 且只有本函数一个写者，不需要临界区。 */
    if (can_obj.enable)
    {
        can_obj.tick++;
    }
}

void MicroCom_Can_TimerHandler(void)
{
    if (!can_obj.enable)
    {
        return;
    }

    const uint32_t now = can_obj.tick;

    for (uint8_t ch = 0; ch < MICROCOM_CAN_CHANNEL_NUM; ch++)
    {
        /* ---- 周期发送：到点发送，time += cycle 避免长期漂移 ---- */
        for (uint32_t i = 0; i < MICROCOM_CAN_CYCLEMSG_SIZE; i++)
        {
            MicroCom_CanCycleTxMsg_t *tx = &can_obj.CycleTx[ch][i];

            if (tx->is_run && (now - tx->next_time >= tx->cycle))
            {
                tx->next_time += tx->cycle;

                MicroCom_Can_Invoke(tx->func, tx->userData, ch, tx->id, MICROCOM_EVENT_TX);
                MicroCom_Can_Transmit(ch, tx->id, tx->mbox_id, tx->dlc, tx->data, tx->is_Extend);
            }
        }

        /* ---- 周期接收：超时检测 ---- */
        for (uint32_t i = 0; i < MICROCOM_CAN_CYCLEMSG_SIZE; i++)
        {
            MicroCom_CanCycleRxMsg_t *rx = &can_obj.CycleRx[ch][i];

            if (rx->is_run && (now - rx->last_rx_time >= rx->timeout))
            {
                rx->last_rx_time += rx->timeout;
                rx->busoff = true;

                MicroCom_Can_Invoke(rx->func, rx->userData, ch, rx->id, MICROCOM_EVENT_ERROR);
            }
        }

        /* ---- 事件发送：trigger 计数消耗 ---- */
        for (uint32_t i = 0; i < MICROCOM_CAN_EVENTMSG_SIZE; i++)
        {
            MicroCom_CanEventTxMsg_t *tx = &can_obj.EventTx[ch][i];

            if (tx->is_run && tx->trigger > 0)
            {
                /* trigger-- 是读改写，Trigger_EventMsg() 可能在另一个上下文
                 * 里同时对它 ++，必须加临界区。临界区只包一条自减指令，
                 * 时间极短，不影响实时性。 */
                // MICROCOM_ENTER_CRITICAL();
                tx->trigger--;
                // MICROCOM_EXIT_CRITICAL();

                MicroCom_Can_Invoke(tx->func, tx->userData, ch, tx->id, MICROCOM_EVENT_TX);
                MicroCom_Can_Transmit(ch, tx->id, tx->mbox_id, tx->dlc, tx->data, tx->is_Extend);
            }
        }
    }
}

/* ========================================================================= */
/* 接收处理                                                                   */
/* ========================================================================= */

MicroCom_Status_t MicroCom_Can_RxIndication(uint8_t channel, uint32_t can_id, const uint8_t *data, uint8_t len)
{
    MICROCOM_CHECK_CAN_CHANNEL(channel);

    if (!can_obj.enable)
    {
        return MICROCOM_STATUS_BUSY;
    }

    /* 先在周期接收表里找 */
    for (size_t i = 0; i < MICROCOM_CAN_CYCLEMSG_SIZE; i++)
    {
        MicroCom_CanCycleRxMsg_t *rx = &can_obj.CycleRx[channel][i];

        if (!rx->is_run || rx->id != can_id)
        {
            continue;
        }

        if (rx->data == NULL)
        {
            return MICROCOM_STATUS_ERR;
        }

        if (len > rx->dlc) /* 防止用户 data 缓冲区（按 dlc 分配）被越界写 */
        {
            return MICROCOM_STATUS_ERR;
        }

        memset(rx->data, 0, rx->dlc);
        memcpy(rx->data, data, len);

        rx->last_rx_time = can_obj.tick; /* 收到报文，重置超时计时基准 */
        rx->busoff = false;

        MicroCom_Can_Invoke(rx->func, rx->userData, channel, can_id, MICROCOM_EVENT_RX);

        return MICROCOM_STATUS_OK;
    }

    /* 再在事件接收表里找 */
    for (size_t i = 0; i < MICROCOM_CAN_EVENTMSG_SIZE; i++)
    {
        MicroCom_CanEventRxMsg_t *rx = &can_obj.EventRx[channel][i];

        if (!rx->is_run || rx->id != can_id)
        {
            continue;
        }

        if (rx->data == NULL)
        {
            return MICROCOM_STATUS_ERR;
        }

        if (len > rx->dlc) /* 事件报文分支之前缺失的越界检查，现已补齐 */
        {
            return MICROCOM_STATUS_ERR;
        }

        memset(rx->data, 0, rx->dlc);
        memcpy(rx->data, data, len);

        MicroCom_Can_Invoke(rx->func, rx->userData, channel, can_id, MICROCOM_EVENT_RX);

        /* 事件报文无自动 busoff 处理机制，由用户通过 Set/ClearEventBusOff 管理 */
        return MICROCOM_STATUS_OK;
    }

    return MICROCOM_NOT_FIND;
}

MicroCom_Status_t MicroCom_Can_Trigger_EventMsg(uint32_t id, uint8_t channel)
{
    MICROCOM_CHECK_CAN_CHANNEL(channel);

    if (!can_obj.enable)
    {
        return MICROCOM_STATUS_BUSY;
    }

    for (uint32_t i = 0; i < MICROCOM_CAN_EVENTMSG_SIZE; i++)
    {
        MicroCom_CanEventTxMsg_t *tx = &can_obj.EventTx[channel][i];

        if (tx->is_run && tx->id == id)
        {
            /* ++ 是读改写，TimerHandler 会在另一个上下文里对它 --，
             * 必须加临界区保护，防止并发丢计数。 */
            // MICROCOM_ENTER_CRITICAL();
            tx->trigger++;
            // MICROCOM_EXIT_CRITICAL();

            return MICROCOM_STATUS_OK;
        }
    }

    return MICROCOM_NOT_FIND;
}

MicroCom_Status_t MicroCom_Can_SetEventBusOff(uint32_t id, uint8_t channel)
{
    MICROCOM_CHECK_CAN_CHANNEL(channel);

    if (!can_obj.enable)
    {
        return MICROCOM_STATUS_BUSY;
    }

    for (uint32_t i = 0; i < MICROCOM_CAN_EVENTMSG_SIZE; i++)
    {
        MicroCom_CanEventRxMsg_t *rx = &can_obj.EventRx[channel][i];

        if (rx->id == id)
        {
            /* 单个 bool 字段的写入在 Cortex-M4 上是单字节原子操作，
             * 不需要临界区。 */
            rx->busoff = true;
            MicroCom_Can_Invoke(rx->func, rx->userData, channel, id, MICROCOM_EVENT_ERROR);
            return MICROCOM_STATUS_OK;
        }
    }

    return MICROCOM_NOT_FIND;
}

MicroCom_Status_t MicroCom_Can_ClearEventBusOff(uint32_t id, uint8_t channel)
{
    MICROCOM_CHECK_CAN_CHANNEL(channel);

    if (!can_obj.enable)
    {
        return MICROCOM_STATUS_BUSY;
    }

    for (uint32_t i = 0; i < MICROCOM_CAN_EVENTMSG_SIZE; i++)
    {
        MicroCom_CanEventRxMsg_t *rx = &can_obj.EventRx[channel][i];

        if (rx->id == id)
        {
            rx->busoff = false;
            return MICROCOM_STATUS_OK;
        }
    }

    return MICROCOM_NOT_FIND;
}

MicroCom_Status_t MicroCom_Can_DisableNonDiagnosticCom(uint8_t channel)
{
    MICROCOM_CHECK_CAN_CHANNEL(channel);

    if (!can_obj.enable)
    {
        return MICROCOM_STATUS_BUSY;
    }

    for (uint32_t i = 0; i < MICROCOM_CAN_CYCLEMSG_SIZE; i++)
    {
        if (!can_obj.CycleTx[channel][i].is_diag)
            can_obj.CycleTx[channel][i].is_run = false;
        if (!can_obj.CycleRx[channel][i].is_diag)
            can_obj.CycleRx[channel][i].is_run = false;
    }

    for (uint32_t i = 0; i < MICROCOM_CAN_EVENTMSG_SIZE; i++)
    {
        if (!can_obj.EventTx[channel][i].is_diag)
            can_obj.EventTx[channel][i].is_run = false;
        if (!can_obj.EventRx[channel][i].is_diag)
            can_obj.EventRx[channel][i].is_run = false;
    }

    return MICROCOM_STATUS_OK;
}

MicroCom_Status_t MicroCom_Can_EnableNonDiagnosticCom(uint8_t channel)
{
    MICROCOM_CHECK_CAN_CHANNEL(channel);

    if (!can_obj.enable)
    {
        return MICROCOM_STATUS_BUSY;
    }

    for (uint32_t i = 0; i < MICROCOM_CAN_CYCLEMSG_SIZE; i++)
    {
        if (!can_obj.CycleTx[channel][i].is_diag)
            can_obj.CycleTx[channel][i].is_run = true;
        if (!can_obj.CycleRx[channel][i].is_diag)
            can_obj.CycleRx[channel][i].is_run = true;
    }

    for (uint32_t i = 0; i < MICROCOM_CAN_EVENTMSG_SIZE; i++)
    {
        if (!can_obj.EventTx[channel][i].is_diag)
            can_obj.EventTx[channel][i].is_run = true;
        if (!can_obj.EventRx[channel][i].is_diag)
            can_obj.EventRx[channel][i].is_run = true;
    }

    return MICROCOM_STATUS_OK;
}

MicroCom_Status_t __attribute__((weak)) MicroCom_Can_Transmit(uint8_t channel, uint32_t can_id, uint16_t mbox,
                                                                  uint8_t dlc, const uint8_t *data, bool is_extend)
{
    (void)channel;
    (void)can_id;
    (void)mbox;
    (void)dlc;
    (void)data;
    (void)is_extend;

    return MICROCOM_STATUS_OK;
}
