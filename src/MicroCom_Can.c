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
#include "string.h"

static MicroCOM_CAN_Obj_t can_obj = {0};

static void MicroCom_Can_Invoke(MicroCom_Func_t func, void *userData, uint8_t channel, uint32_t id, MicroCom_Event_t event, uint16_t mboxId, bool is_extend)
{
    if (func != NULL)
    {
        MicroCom_Ctx_t ctx;
        ctx.event = event;
        ctx.channel = channel;
        ctx.id = id;
        ctx.userData = userData;
        ctx.mboxId = mboxId;
        ctx.is_extend = is_extend;
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
        for (uint32_t i = 0; i < MICROCOM_CAN_CYCLEMSG_TX_SIZE; i++)
        {
            if (can_obj.CycleTx[ch][i].is_valid)
            {
                can_obj.CycleTx[ch][i].next_time = now;
            }
        }

        for (uint32_t i = 0; i < MICROCOM_CAN_CYCLEMSG_RX_SIZE; i++)
        {
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

MicroCom_Status_t MicroCom_Can_Register_CycleTxMsg(const MicroCom_CanCycleTxMsg_t *table, size_t size)
{
    MICROCOM_CHECK_PTR(table);
    MICROCOM_CHECK_CYCLE_CAN_TX_SIZE(size);

    uint32_t count[MICROCOM_CAN_CHANNEL_NUM] = {0};

    for (size_t i = 0; i < size; i++)
    {
        MICROCOM_CHECK_CAN_CHANNEL(table[i].channel);
        MICROCOM_CHECK_DLC(table[i].dlc);

        if (count[table[i].channel] >= MICROCOM_CAN_CYCLEMSG_TX_SIZE)
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
        slot->is_txing = false;
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
    MICROCOM_CHECK_CYCLE_CAN_RX_SIZE(size);

    uint32_t count[MICROCOM_CAN_CHANNEL_NUM] = {0};

    for (size_t i = 0; i < size; i++)
    {
        MICROCOM_CHECK_CAN_CHANNEL(table[i].channel);
        MICROCOM_CHECK_DLC(table[i].dlc);

        if (count[table[i].channel] >= MICROCOM_CAN_CYCLEMSG_RX_SIZE)
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
        slot->is_trigger = false;
    }

    for (uint8_t ch = 0; ch < MICROCOM_CAN_CHANNEL_NUM; ch++)
    {
        can_obj.c_rx_num[ch] = index[ch];
    }

    return MICROCOM_STATUS_OK;
}
#if MICROCOM_CAN_EVENTMSG_ENABLE
MicroCom_Status_t MicroCom_Can_Register_EventTxMsg(const MicroCom_CanEventTxMsg_t *table, size_t size)
{
    MICROCOM_CHECK_PTR(table);
    MICROCOM_CHECK_EVENT_CAN_TX_SIZE(size);

    uint32_t count[MICROCOM_CAN_CHANNEL_NUM] = {0};

    for (size_t i = 0; i < size; i++)
    {
        MICROCOM_CHECK_CAN_CHANNEL(table[i].channel);
        // MICROCOM_CHECK_DLC(table[i].dlc);

        if (count[table[i].channel] >= MICROCOM_CAN_EVENTMSG_TX_SIZE)
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
        slot->mbox_id = table[i].mbox_id;
        slot->channel = table[i].channel;
        slot->is_diag = table[i].is_diag;
        memset(slot->data,0,MICROCOM_CAN_MAX_DLC);
        slot->len = 0;
        slot->userData = table[i].userData;
        slot->func = table[i].func;

        slot->is_run = true;
        slot->trigger = false;
        slot->is_valid = true;
        slot->is_txing = false;
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
    MICROCOM_CHECK_EVENT_CAN_RX_SIZE(size);

    uint32_t count[MICROCOM_CAN_CHANNEL_NUM] = {0};

    for (size_t i = 0; i < size; i++)
    {
        MICROCOM_CHECK_CAN_CHANNEL(table[i].channel);
        MICROCOM_CHECK_DLC(table[i].dlc);

        if (count[table[i].channel] >= MICROCOM_CAN_EVENTMSG_RX_SIZE)
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
        slot->is_trigger = false;
    }

    for (uint8_t ch = 0; ch < MICROCOM_CAN_CHANNEL_NUM; ch++)
    {
        can_obj.e_rx_num[ch] = index[ch];
    }

    return MICROCOM_STATUS_OK;
}
#endif

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

                if (MicroCom_Can_Transmit(ch, tx->id, tx->mbox_id, tx->dlc, tx->data, tx->is_Extend) == MICROCOM_STATUS_OK)
                {
                    tx->is_txing = true;
                    MicroCom_Can_Invoke(tx->func, tx->userData, ch, tx->id, MICROCOM_EVENT_CYCLE_TX, tx->mbox_id, tx->is_Extend);
                }
                else
                {
                    MicroCom_Can_Invoke(tx->func, tx->userData, ch, tx->id, MICROCOM_EVENT_CYCLE_TX_ERROR, tx->mbox_id, tx->is_Extend);
                }
            }
        }

        for (uint32_t i = 0; i < can_obj.c_rx_num[ch]; i++)
        {
            MicroCom_CanCycleRxMsg_t *rx = &can_obj.CycleRx[ch][i];
            MICROCOM_SKIP_INVALID(rx->is_valid);

            if (rx->is_run && !rx->is_offline && (now - rx->last_rx_time >= rx->timeout))
            {
                rx->is_offline = true;

                MicroCom_Can_Invoke(rx->func, rx->userData, ch, rx->id, MICROCOM_EVENT_CYCLE_RX_ERROR, rx->mbox_id, rx->is_Extend);
            }

            if (rx->is_run && rx->is_trigger)
            {
                rx->is_trigger = false;
                MicroCom_Can_Invoke(rx->func, rx->userData, ch, rx->id, MICROCOM_EVENT_CYCLE_RX, rx->mbox_id, rx->is_Extend);
            }
        }
#if MICROCOM_CAN_EVENTMSG_ENABLE
        for (uint32_t i = 0; i < can_obj.e_tx_num[ch]; i++)
        {
            MicroCom_CanEventTxMsg_t *tx = &can_obj.EventTx[ch][i];
            MICROCOM_SKIP_INVALID(tx->is_valid);

            if (tx->is_run && tx->trigger)
            {
                tx->trigger = false;

                if (MicroCom_Can_Transmit(ch, tx->id, tx->mbox_id, tx->len, tx->data, tx->is_Extend) == MICROCOM_STATUS_OK)
                {
                    tx->is_txing = true;
                    MicroCom_Can_Invoke(tx->func, tx->userData, ch, tx->id, MICROCOM_EVENT_EVENT_TX, tx->mbox_id, tx->is_Extend);
                    memset(tx->data,0,MICROCOM_CAN_MAX_DLC);
                    tx->len = 0;
                }
                else
                {
                    MicroCom_Can_Invoke(tx->func, tx->userData, ch, tx->id, MICROCOM_EVENT_EVENT_TX_ERROR, tx->mbox_id, tx->is_Extend);
                }
            }
        }

        for (uint32_t i = 0; i < can_obj.e_rx_num[ch]; i++)
        {
            MicroCom_CanEventRxMsg_t *rx = &can_obj.EventRx[ch][i];

            if (rx->is_run && rx->is_trigger)
            {
            	rx->is_trigger = false;
                MicroCom_Can_Invoke(rx->func, rx->userData, ch, rx->id, MICROCOM_EVENT_EVENT_RX, rx->mbox_id, rx->is_Extend);
            }
        }
#endif
    }
}

MicroCom_Status_t MicroCom_Can_RxIndication(uint8_t channel, uint32_t can_id, bool is_Extend, const uint8_t *data, uint8_t len)
{
    MICROCOM_CHECK_CAN_CHANNEL(channel);
    MICROCOM_CHECK_PTR(data);

    if (!can_obj.enable)
    {
        return MICROCOM_STATUS_BUSY;
    }

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

        if (len > rx->dlc)
        {
            return MICROCOM_STATUS_ERR;
        }

        memset(rx->data, 0, rx->dlc);
        memcpy(rx->data, data, len);

        rx->last_rx_time = can_obj.tick;
        rx->is_offline = false;
        rx->is_trigger = true;

        return MICROCOM_STATUS_OK;
    }
#if MICROCOM_CAN_EVENTMSG_ENABLE
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
        rx->is_trigger = true;

        return MICROCOM_STATUS_OK;
    }
#endif

    return MICROCOM_NOT_FIND;
}

#if MICROCOM_CAN_EVENTMSG_ENABLE
MicroCom_Status_t MicroCom_Can_Trigger_EventTxMsg(uint32_t id, uint8_t channel,const uint8_t *data,uint16_t len)
{
    MICROCOM_CHECK_CAN_CHANNEL(channel);
    MICROCOM_CHECK_DLC(len);
    MICROCOM_CHECK_PTR(data);

    if (!can_obj.enable)
    {
        return MICROCOM_STATUS_BUSY;
    }

    for (uint32_t i = 0; i < can_obj.e_tx_num[channel]; i++)
    {
        MicroCom_CanEventTxMsg_t *tx = &can_obj.EventTx[channel][i];

        MICROCOM_SKIP_INVALID(tx->is_valid);

        if (tx->is_run && tx->id == id)
        {

            tx->trigger = true;
            memcpy(tx->data,data,len);

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
            MicroCom_Can_Invoke(rx->func, rx->userData, channel, id, MICROCOM_EVENT_EVENT_RX_ERROR, rx->mbox_id, rx->is_Extend);
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
#endif

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

    for (uint32_t i = 0; i < can_obj.c_rx_num[channel]; i++)
    {
        if (can_obj.CycleRx[channel][i].is_valid && !can_obj.CycleRx[channel][i].is_diag)
        {
            can_obj.CycleRx[channel][i].is_run = false;
        }
    }
#if MICROCOM_CAN_EVENTMSG_ENABLE
    for (uint32_t i = 0; i < can_obj.e_tx_num[channel]; i++)
    {
        if (can_obj.EventTx[channel][i].is_valid && !can_obj.EventTx[channel][i].is_diag)
        {
            can_obj.EventTx[channel][i].is_run = false;
        }
    }

    for (uint32_t i = 0; i < can_obj.e_rx_num[channel]; i++)
    {
        if (can_obj.EventRx[channel][i].is_valid && !can_obj.EventRx[channel][i].is_diag)
        {
            can_obj.EventRx[channel][i].is_run = false;
        }
    }
#endif
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

    for (uint32_t i = 0; i < can_obj.c_rx_num[channel]; i++)
    {
        if (can_obj.CycleRx[channel][i].is_valid && !can_obj.CycleRx[channel][i].is_diag)
        {
            can_obj.CycleRx[channel][i].is_run = true;
        }
    }
#if MICROCOM_CAN_EVENTMSG_ENABLE
    for (uint32_t i = 0; i < can_obj.e_tx_num[channel]; i++)
    {
        if (can_obj.EventTx[channel][i].is_valid && !can_obj.EventTx[channel][i].is_diag)
        {
            can_obj.EventTx[channel][i].is_run = true;
        }
    }

    for (uint32_t i = 0; i < can_obj.e_rx_num[channel]; i++)
    {
        if (can_obj.EventRx[channel][i].is_valid && !can_obj.EventRx[channel][i].is_diag)
        {
            can_obj.EventRx[channel][i].is_run = true;
        }
    }
#endif
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
    if (channel >= MICROCOM_CAN_CHANNEL_NUM)
    {
        return false;
    }

    for (uint32_t i = 0; i < can_obj.c_rx_num[channel]; i++)
    {
        MicroCom_CanCycleRxMsg_t *msg = &can_obj.CycleRx[channel][i];

        if (msg->id == id && msg->is_Extend == is_extend)
        {
            return msg->is_offline;
        }
    }

    return false;
}

MicroCom_Status_t MicroCom_Can_HwTxDone(uint8_t channel, uint16_t mboxId)
{
    if (!can_obj.enable)
    {
        return MICROCOM_STATUS_BUSY;
    }

    for (uint32_t i = 0; i < can_obj.c_tx_num[channel]; i++)
    {
        MicroCom_CanCycleTxMsg_t *tx = &can_obj.CycleTx[channel][i];
        if (tx->mbox_id == mboxId && tx->is_txing)
        {
            tx->is_txing = false;
            MicroCom_Can_Invoke(tx->func, tx->userData, tx->channel, tx->id, MICROCOM_EVENT_CYCLE_TX_DONE, tx->mbox_id, tx->is_Extend);
            return MICROCOM_STATUS_OK;
        }
    }

#if MICROCOM_CAN_EVENTMSG_ENABLE
    for (uint32_t i = 0; i < can_obj.e_tx_num[channel]; i++)
    {
        MicroCom_CanEventTxMsg_t *tx = &can_obj.EventTx[channel][i];

        if (tx->mbox_id == mboxId && tx->is_txing)
        {
            tx->is_txing = false;
            MicroCom_Can_Invoke(tx->func, tx->userData, tx->channel, tx->id, MICROCOM_EVENT_EVENT_TX_DONE, tx->mbox_id, tx->is_Extend);
            return MICROCOM_STATUS_OK;
        }
    }
#endif
    return MICROCOM_NOT_FIND;
}
