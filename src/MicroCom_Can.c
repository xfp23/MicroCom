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

static inline void MicroCom_Can_Invoke(MicroCom_Func_t func, void *userData, uint8_t channel, uint32_t id, MicroCom_Event_t event)
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

MicroCom_Status_t MicroCom_Can_Init(void)
{
    memset(&can_obj, 0, sizeof(MicroCOM_CAN_Obj_t));
    return MICROCOM_STATUS_OK;
}

void MicroCom_Can_Start(void)
{
    uint32_t now = 0;

    now = can_obj.tick;

    for (uint8_t ch = 0; ch < MICROCOM_CAN_CHANNEL_NUM; ch++)
    {
        for (uint32_t i = 0; i < MICROCOM_CAN_CYCLEMSG_SIZE; i++)
        {
            if (can_obj.CycleTx[ch][i].is_valid)
            {
                can_obj.CycleTx[ch][i].next_time = now;
            }

            if (can_obj.CycleRx[ch][i].is_valid)
            {
                can_obj.CycleRx[ch][i].last_rx_time = now;
            }
        }
    }
    can_obj.enable = true;
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

    uint32_t count[MICROCOM_CAN_CHANNEL_NUM] = {0};

    for (size_t i = 0; i < size; i++)
    {
        MICROCOM_CHECK_CAN_CHANNEL(table[i].channel);
        MICROCOM_CHECK_DLC(table[i].dlc);

        if (count[table[i].channel] >= MICROCOM_CAN_CYCLEMSG_SIZE)
        {
            return MICROCOM_STATUS_ERR;
        }
        count[table[i].channel]++;
    }

    uint32_t index[MICROCOM_CAN_CHANNEL_NUM] = {0};

    for (size_t i = 0; i < size; i++)
    {
        uint8_t ch = table[i].channel;
        MicroCom_CanCycleTxMsg_t *slot = &can_obj.CycleTx[ch][index[ch]];
        index[ch]++;

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
        slot->is_valid = true;
    }

    for (uint8_t ch = 0; ch < MICROCOM_CAN_CHANNEL_NUM; ch++)
    {
        can_obj.c_tx_num[ch] = index[ch];
    }
    return MICROCOM_STATUS_OK;
}

MicroCom_Status_t MicroCom_Can_Register_CycleRxMsg(const MicroCom_CanCycleRxMsg_t *table, size_t size)
{
    MICROCOM_CHECK_PTR(table);
    MICROCOM_CHECK_CYCLE_CAN_SIZE(size);

    uint32_t count[MICROCOM_CAN_CHANNEL_NUM] = {0};

    for (size_t i = 0; i < size; i++)
    {
        MICROCOM_CHECK_CAN_CHANNEL(table[i].channel);
        MICROCOM_CHECK_DLC(table[i].dlc);

        if (count[table[i].channel] >= MICROCOM_CAN_CYCLEMSG_SIZE)
        {
            return MICROCOM_STATUS_ERR;
        }
        count[table[i].channel]++;
    }

    uint32_t index[MICROCOM_CAN_CHANNEL_NUM] = {0};

    for (size_t i = 0; i < size; i++)
    {
        uint8_t ch = table[i].channel;
        MicroCom_CanCycleRxMsg_t *slot = &can_obj.CycleRx[ch][index[ch]];
        index[ch]++;

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
        slot->is_offline = false;
        slot->last_rx_time = can_obj.tick;
        slot->is_valid = true;
    }

    for (uint8_t ch = 0; ch < MICROCOM_CAN_CHANNEL_NUM; ch++)
    {
        can_obj.c_rx_num[ch] = index[ch];
    }

    return MICROCOM_STATUS_OK;
}

MicroCom_Status_t MicroCom_Can_Register_EventTxMsg(const MicroCom_CanEventTxMsg_t *table, size_t size)
{
    MICROCOM_CHECK_PTR(table);
    MICROCOM_CHECK_EVENT_CAN_SIZE(size);

    uint32_t count[MICROCOM_CAN_CHANNEL_NUM] = {0};

    for (size_t i = 0; i < size; i++)
    {
        MICROCOM_CHECK_CAN_CHANNEL(table[i].channel);
        MICROCOM_CHECK_DLC(table[i].dlc);

        if (count[table[i].channel] >= MICROCOM_CAN_EVENTMSG_SIZE)
        {
            return MICROCOM_STATUS_ERR;
        }
        count[table[i].channel]++;
    }

    uint32_t index[MICROCOM_CAN_CHANNEL_NUM] = {0};

    for (size_t i = 0; i < size; i++)
    {
        uint8_t ch = table[i].channel;
        MicroCom_CanEventTxMsg_t *slot = &can_obj.EventTx[ch][index[ch]];
        index[ch]++;

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
        slot->is_valid = true;
    }

    for (uint8_t ch = 0; ch < MICROCOM_CAN_CHANNEL_NUM; ch++)
    {
        can_obj.e_tx_num[ch] = index[ch];
    }

    return MICROCOM_STATUS_OK;
}

MicroCom_Status_t MicroCom_Can_Register_EventRxMsg(const MicroCom_CanEventRxMsg_t *table, size_t size)
{
    MICROCOM_CHECK_PTR(table);
    MICROCOM_CHECK_EVENT_CAN_SIZE(size);

    uint32_t count[MICROCOM_CAN_CHANNEL_NUM] = {0};

    for (size_t i = 0; i < size; i++)
    {
        MICROCOM_CHECK_CAN_CHANNEL(table[i].channel);
        MICROCOM_CHECK_DLC(table[i].dlc);

        if (count[table[i].channel] >= MICROCOM_CAN_EVENTMSG_SIZE)
        {
            return MICROCOM_STATUS_ERR;
        }
        count[table[i].channel]++;
    }

    uint32_t index[MICROCOM_CAN_CHANNEL_NUM] = {0};

    for (size_t i = 0; i < size; i++)
    {
        uint8_t ch = table[i].channel;
        MicroCom_CanEventRxMsg_t *slot = &can_obj.EventRx[ch][index[ch]];
        index[ch]++;

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
        slot->is_offline = false;
        slot->is_valid = true;
    }

    for (uint8_t ch = 0; ch < MICROCOM_CAN_CHANNEL_NUM; ch++)
    {
        can_obj.e_rx_num[ch] = index[ch];
    }

    return MICROCOM_STATUS_OK;
}

/* ========================================================================= */
/* Tick / Timer                                                              */
/* ========================================================================= */

void MicroCom_Can_TickHandler(void)
{
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
        for (uint32_t i = 0; i < can_obj.c_tx_num[ch]; i++)
        {
            MicroCom_CanCycleTxMsg_t *tx = &can_obj.CycleTx[ch][i];

            MICROCOM_SKIP_INVALID(tx->is_valid);

            if (tx->is_run && (now - tx->next_time >= tx->cycle))
            {
                tx->next_time += tx->cycle;

                MicroCom_Can_Invoke(tx->func, tx->userData, ch, tx->id, MICROCOM_EVENT_TX);
                MicroCom_Can_Transmit(ch, tx->id, tx->mbox_id, tx->dlc, tx->data, tx->is_Extend);
            }
        }

        for (uint32_t i = 0; i < can_obj.c_rx_num[ch]; i++)
        {
            MicroCom_CanCycleRxMsg_t *rx = &can_obj.CycleRx[ch][i];
            MICROCOM_SKIP_INVALID(rx->is_valid);

            if (rx->is_run && !rx->is_offline && (now - rx->last_rx_time >= rx->timeout))
            {
                // rx->last_rx_time += rx->timeout;
                rx->is_offline = true;

                MicroCom_Can_Invoke(rx->func, rx->userData, ch, rx->id, MICROCOM_EVENT_ERROR);
            }
        }

        for (uint32_t i = 0; i < can_obj.e_tx_num[ch]; i++)
        {
            MicroCom_CanEventTxMsg_t *tx = &can_obj.EventTx[ch][i];
            MICROCOM_SKIP_INVALID(tx->is_valid);

            if (tx->is_run && tx->trigger > 0)
            {
                tx->trigger--;
                MicroCom_Can_Invoke(tx->func, tx->userData, ch, tx->id, MICROCOM_EVENT_TX);
                MicroCom_Can_Transmit(ch, tx->id, tx->mbox_id, tx->dlc, tx->data, tx->is_Extend);
            }
        }
    }
}

/* ========================================================================= */
/* 接收处理                                                                   */
/* ========================================================================= */

MicroCom_Status_t MicroCom_Can_RxIndication(uint8_t channel, uint32_t can_id, bool is_Extend, const uint8_t *data, uint8_t len)
{
    MICROCOM_CHECK_CAN_CHANNEL(channel);
    MICROCOM_CHECK_PTR(data);

    if (!can_obj.enable)
    {
        return MICROCOM_STATUS_BUSY;
    }

    /* 先在周期接收表里找 */
    for (size_t i = 0; i < can_obj.c_rx_num[channel]; i++)
    {
        MicroCom_CanCycleRxMsg_t *rx = &can_obj.CycleRx[channel][i];

        MICROCOM_SKIP_INVALID(rx->is_valid);

        if (!rx->is_run || rx->id != can_id || rx->is_Extend != is_Extend)
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
        rx->is_offline = false;

        MicroCom_Can_Invoke(rx->func, rx->userData, channel, can_id, MICROCOM_EVENT_RX);

        return MICROCOM_STATUS_OK;
    }

    /* 再在事件接收表里找 */
    for (size_t i = 0; i < can_obj.e_rx_num[channel]; i++)
    {
        MicroCom_CanEventRxMsg_t *rx = &can_obj.EventRx[channel][i];

        MICROCOM_SKIP_INVALID(rx->is_valid);

        if (!rx->is_run || rx->id != can_id || rx->is_Extend != is_Extend)
        {
            continue;
        }

        if (rx->data == NULL)
        {
            return MICROCOM_STATUS_ERR;
        }

        if (len > rx->dlc)
        {
            return MICROCOM_STATUS_ERR;
        }

        memset(rx->data, 0, rx->dlc);
        memcpy(rx->data, data, len);

        MicroCom_Can_Invoke(rx->func, rx->userData, channel, can_id, MICROCOM_EVENT_RX);

        /* 事件报文无自动 is_offline 处理机制，由用户通过 Set/ClearEventis_offline 管理 */
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

    for (uint32_t i = 0; i < can_obj.e_tx_num[channel]; i++)
    {
        MicroCom_CanEventTxMsg_t *tx = &can_obj.EventTx[channel][i];

        MICROCOM_SKIP_INVALID(tx->is_valid);

        if (tx->is_run && tx->id == id && tx->trigger < UINT16_MAX)
        {

            tx->trigger++;

            return MICROCOM_STATUS_OK;
        }
    }

    return MICROCOM_NOT_FIND;
}

MicroCom_Status_t MicroCom_Can_SetEventOffline(uint32_t id, uint8_t channel)
{
    MICROCOM_CHECK_CAN_CHANNEL(channel);

    if (!can_obj.enable)
    {
        return MICROCOM_STATUS_BUSY;
    }

    for (uint32_t i = 0; i < can_obj.e_rx_num[channel]; i++)
    {
        MicroCom_CanEventRxMsg_t *rx = &can_obj.EventRx[channel][i];

        MICROCOM_SKIP_INVALID(rx->is_valid);

        if (rx->id == id)
        {
            rx->is_offline = true;
            MicroCom_Can_Invoke(rx->func, rx->userData, channel, id, MICROCOM_EVENT_ERROR);
            return MICROCOM_STATUS_OK;
        }
    }

    return MICROCOM_NOT_FIND;
}

MicroCom_Status_t MicroCom_Can_ClearEventOffline(uint32_t id, uint8_t channel)
{
    MICROCOM_CHECK_CAN_CHANNEL(channel);

    if (!can_obj.enable)
    {
        return MICROCOM_STATUS_BUSY;
    }

    for (uint32_t i = 0; i < can_obj.e_rx_num[channel]; i++)
    {
        MicroCom_CanEventRxMsg_t *rx = &can_obj.EventRx[channel][i];

        MICROCOM_SKIP_INVALID(rx->is_valid);

        if (rx->id == id)
        {
            rx->is_offline = false;
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

    for (uint32_t i = 0; i < can_obj.c_tx_num[channel]; i++)
    {
        if (can_obj.CycleTx[channel][i].is_valid && !can_obj.CycleTx[channel][i].is_diag)
        {
            can_obj.CycleTx[channel][i].is_run = false;
        }
    }

    for(uint32_t i = 0; i < can_obj.c_rx_num[channel]; i++)
    {
        if (can_obj.CycleRx[channel][i].is_valid && !can_obj.CycleRx[channel][i].is_diag)
        {
            can_obj.CycleRx[channel][i].is_run = false;
        }
    }

    for (uint32_t i = 0; i < can_obj.e_tx_num[channel]; i++)
    {
        if (can_obj.EventTx[channel][i].is_valid && !can_obj.EventTx[channel][i].is_diag)
        {
            can_obj.EventTx[channel][i].is_run = false;
        }
    }

    for(uint32_t i = 0; i < can_obj.e_rx_num[channel]; i++)
    {
        if (can_obj.EventRx[channel][i].is_valid && !can_obj.EventRx[channel][i].is_diag)
        {
            can_obj.EventRx[channel][i].is_run = false;
        }
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

    for (uint32_t i = 0; i < can_obj.c_tx_num[channel]; i++)
    {
        if (can_obj.CycleTx[channel][i].is_valid && !can_obj.CycleTx[channel][i].is_diag)
        {
            can_obj.CycleTx[channel][i].is_run = true;
        }
    }

    for(uint32_t i = 0; i < can_obj.c_rx_num[channel]; i++)
    {
        if (can_obj.CycleRx[channel][i].is_valid && !can_obj.CycleRx[channel][i].is_diag)
        {
            can_obj.CycleRx[channel][i].is_run = true;
        }
    }

    for (uint32_t i = 0; i < can_obj.e_tx_num[channel]; i++)
    {
        if (can_obj.EventTx[channel][i].is_valid && !can_obj.EventTx[channel][i].is_diag)
        {
            can_obj.EventTx[channel][i].is_run = true;
        }
    }

    for(uint32_t i = 0; i < can_obj.e_rx_num[channel]; i++)
    {
        if (can_obj.EventRx[channel][i].is_valid && !can_obj.EventRx[channel][i].is_diag)
        {
            can_obj.EventRx[channel][i].is_run = true;
        }
    }
    return MICROCOM_STATUS_OK;
}

MicroCom_Status_t __attribute__((weak)) MicroCom_Can_Transmit(uint8_t channel, uint32_t can_id, uint16_t mbox, uint8_t dlc, const uint8_t *data, bool is_extend)
{
    (void)channel;
    (void)can_id;
    (void)mbox;
    (void)dlc;
    (void)data;
    (void)is_extend;

    return MICROCOM_STATUS_OK;
}


bool MicroCom_Can_IsCycleRxBusOffline(uint8_t channel, uint32_t id, bool is_extend)
{
    if(channel >= MICROCOM_CAN_CHANNEL_NUM)
    {
        return false;
    }

    for(int i = 0; i < can_obj.c_rx_num; i++)
    {
        MicroCom_CanCycleRxMsg_t *msg = &can_obj.CycleRx[channel][i];

        if(msg->id == id && msg->is_Extend == is_extend)
        {
            return msg->is_offline;
        }
    }

    return false;
}
