/**
 * @file MicroCom_Lin.c
 * @author https://xfp23.github.io/
 * @brief MicroCom LIN 调度模块实现
 * @version \ref MICROCOM_VERSION
 * @date 2026-09-09
 *
 * @copyright Copyright (c) 2026
 */
#include "MicroCom_Lin.h"
#include <string.h>

#if MICROCOM_LIN_ENABLE

/* ========================================================================= */
/* 运行时对象                                                                */
/* ========================================================================= */

static MicroCom_Lin_Obj_t lin_obj = {0};

/* 尽量用指针，为以后 MicroMem 打基础 */
static MicroCom_Lin_Obj_t *lin_ptr = &lin_obj;

/* ========================================================================= */
/* 内部小工具                                                                */
/* ========================================================================= */

static inline void MicroCom_Lin_Invoke(MicroCom_Lin_Table_t *t, MicroCom_Lin_Event_t event)
{
    if (t->callback != NULL)
    {
        MicroCom_Lin_Ctx_t ctx;
        ctx.event    = event;
        ctx.channel  = t->channel;
        ctx.id       = t->id;
        ctx.data     = t->data;
        ctx.len      = t->length;
        ctx.userData = t->userdata;
        t->callback(&ctx);
    }
}

/* ========================================================================= */
/* 向下：弱默认实现                                                          */
/*                                                                          */
/* 没有挂真正的驱动时统一返回 false——绝不能默认返回 true，否则调度层         */
/* 会以为提交成功、把通道置 busy，然后永远等不到 TxDone/RxIndication，        */
/* 这个通道就死锁了。                                                       */
/* ========================================================================= */

MicroCom_Status_t __attribute__((weak)) MicroCom_Lin_Transmit(uint8_t channel, uint8_t id, const uint8_t *data, uint8_t len)
{
    (void)channel;
    (void)id;
    (void)data;
    (void)len;
    return MICROCOM_STATUS_OK;
}

MicroCom_Status_t __attribute__((weak)) MicroCom_Lin_Receive(uint8_t channel, uint8_t id, uint8_t len)
{
    (void)channel;
    (void)id;
    (void)len;
    return MICROCOM_STATUS_OK;
}

/* ========================================================================= */
/* 生命周期                                                                   */
/* ========================================================================= */

MicroCom_Status_t MicroCom_Lin_Init(void)
{
    memset(lin_ptr, 0, sizeof(*lin_ptr));

    for (uint8_t ch = 0u; ch < MICROCOM_LIN_CHANNEL_NUM; ch++)
    {
        lin_ptr->active_idx[ch] = MICROCOM_LIN_IDX_INVALID;
    }

    return MICROCOM_STATUS_OK;
}

void MicroCom_Lin_Start(void)
{
    uint32_t now = lin_ptr->tick;

    for (uint8_t ch = 0u; ch < MICROCOM_LIN_CHANNEL_NUM; ch++)
    {
        lin_ptr->busy[ch]        = false;
        lin_ptr->active_idx[ch]  = MICROCOM_LIN_IDX_INVALID;
        lin_ptr->scan_cursor[ch] = 0u;

        for (uint32_t i = 0u; i < MICROCOM_LIN_MSG_SIZE; i++)
        {
            MicroCom_Lin_Table_t *t = &lin_ptr->table[ch][i];

            if (!t->is_valid)
            {
                continue;
            }

            t->next_time = now;

            if (t->dir == MICROCOM_DIR_RX)
            {
                t->last_rx_time = now;
            }
        }
    }

    lin_ptr->enable = true;
}

void MicroCom_Lin_Stop(void)
{
    lin_ptr->enable = false;
}

/* ========================================================================= */
/* 注册                                                                       */
/* ========================================================================= */

MicroCom_Status_t MicroCom_Lin_RegisterTable(const MicroCom_Lin_ConfigTable_t *table, size_t size)
{
    if (table == NULL)
    {
        return MICROCOM_PARAM_INVALID;
    }

    if ((size == 0u) || (size > ((size_t)MICROCOM_LIN_CHANNEL_NUM * MICROCOM_LIN_MSG_SIZE)))
    {
        return MICROCOM_PARAM_INVALID;
    }

    /* ---- 第一趟：只校验，不写入 lin_obj，保证失败时状态完全不变 ---- */
    uint32_t count[MICROCOM_LIN_CHANNEL_NUM] = {0};

    for (size_t i = 0u; i < size; i++)
    {
        if (table[i].channel >= MICROCOM_LIN_CHANNEL_NUM)
        {
            return MICROCOM_CHANNEL_OVERFLOW;
        }

        if (table[i].length > 8)
        {
            return MICROCOM_DATA_OVERFLOW;
        }

        if (table[i].id > 0x3Fu)
        {
            return MICROCOM_PARAM_INVALID;
        }

        if (table[i].data == NULL)
        {
            return MICROCOM_PARAM_INVALID;
        }

        if (count[table[i].channel] >= MICROCOM_LIN_MSG_SIZE)
        {
            return MICROCOM_STATUS_ERR;
        }
        count[table[i].channel]++;
    }

    /* ---- 第二趟：校验已全部通过，安心写入 ---- */
    uint32_t index[MICROCOM_LIN_CHANNEL_NUM] = {0};

    for (size_t i = 0u; i < size; i++)
    {
        uint8_t ch = table[i].channel;
        MicroCom_Lin_Table_t *t = &lin_ptr->table[ch][index[ch]];
        index[ch]++;

        t->channel  = table[i].channel;
        t->id       = table[i].id;
        t->length   = table[i].length;
        t->data     = table[i].data;
        t->cycle    = table[i].period;
        t->timeout  = table[i].timeout;
        t->dir      = table[i].dir;
        t->callback = table[i].callback;
        t->userdata = table[i].userdata;

        t->next_time    = lin_ptr->tick;
        t->last_rx_time = lin_ptr->tick;
        t->is_timeout   = false;
        t->is_valid     = true;
#if MICROCOM_FASTMODE_ENABLE == 0
        t->is_trigger = false;
#endif
    }

    for (uint8_t ch = 0u; ch < MICROCOM_LIN_CHANNEL_NUM; ch++)
    {
        lin_ptr->msg_num[ch] = index[ch];
    }

    return MICROCOM_STATUS_OK;
}

/* ========================================================================= */
/* Tick                                                                       */
/* ========================================================================= */

void MicroCom_Lin_TickHandler(void)
{
    if (lin_ptr->enable)
    {
        lin_ptr->tick++;
    }
}

/* ========================================================================= */
/* 调度：每通道找下一条到点的报文，发起一次传输                              */
/* ========================================================================= */

static void MicroCom_Lin_DispatchChannel(uint8_t ch, uint32_t now)
{
    uint32_t count = lin_ptr->msg_num[ch];

    if (count == 0u)
    {
        return;
    }

    /* 从轮询游标开始扫一整圈，找第一条到点的；用游标而不是每次都从 0
     * 开始，避免某条短周期报文把同通道里排在后面的报文长期饿死。 */
    for (uint32_t step = 0u; step < count; step++)
    {
        uint8_t idx = (uint8_t)((lin_ptr->scan_cursor[ch] + step) % count);
        MicroCom_Lin_Table_t *t = &lin_ptr->table[ch][idx];

        if (!t->is_valid)
        {
            continue;
        }

        if ((now - t->next_time) < t->cycle)
        {
            continue; /* 还没到点 */
        }

        /* 到点了：不管这次提交成功与否，都先推进 next_time，
         * 避免提交失败时下一轮又立刻重复判定"到点"，导致疯狂重试。 */
        t->next_time += t->cycle;
        lin_ptr->scan_cursor[ch] = (uint8_t)((idx + 1u) % count);

        if (t->dir == MICROCOM_DIR_TX)
        {
            bool ok = MicroCom_Lin_Transmit(ch, t->id, t->data, t->length);

            if (ok)
            {
                lin_ptr->busy[ch]       = true;
                lin_ptr->active_idx[ch] = idx;
                MicroCom_Lin_Invoke(t, MICROCOM_LIN_EVENT_TX);
            }
            else
            {
                MicroCom_Lin_Invoke(t, MICROCOM_LIN_EVENT_TX_ERROR);
            }
        }
        else /* MICROCOM_DIR_RX */
        {
            bool ok = MicroCom_Lin_Receive(ch, t->id, t->length);

            if (ok)
            {
                lin_ptr->busy[ch]       = true;
                lin_ptr->active_idx[ch] = idx;
            }
            /* 提交失败就安静地等下个周期重试，不单独通知用户——
             * 和下面 RX 超时锁存的设计是一回事：偶发的单次失败
             * 不值得打扰用户，只有持续失败才需要。 */
        }

        return; /* 这个通道这个 tick 只发起一次 */
    }
}

/* ========================================================================= */
/* RX 超时检查：和 MicroCom_Can 的 CycleRx 超时模型完全一致                 */
/* ========================================================================= */

static void MicroCom_Lin_CheckTimeout(uint8_t ch, uint32_t now)
{
    for (uint32_t i = 0u; i < lin_ptr->msg_num[ch]; i++)
    {
        MicroCom_Lin_Table_t *t = &lin_ptr->table[ch][i];

        if (!t->is_valid || (t->dir != MICROCOM_DIR_RX))
        {
            continue;
        }

        if (!t->is_timeout && ((now - t->last_rx_time) >= t->timeout))
        {
            t->is_timeout = true;
            memset(t->data, 0, t->length); /* 对应老代码里"节点丢失后把接收报文清零"的行为 */
            MicroCom_Lin_Invoke(t, MICROCOM_LIN_EVENT_RX_ERROR);
        }
    }
}

void MicroCom_Lin_TimerHandler(void)
{
    if (!lin_ptr->enable)
    {
        return;
    }

    uint32_t now = lin_ptr->tick;

    for (uint8_t ch = 0u; ch < MICROCOM_LIN_CHANNEL_NUM; ch++)
    {
        if (!lin_ptr->busy[ch])
        {
            MicroCom_Lin_DispatchChannel(ch, now);
        }

        MicroCom_Lin_CheckTimeout(ch, now);

#if MICROCOM_FASTMODE_ENABLE == 0
        for (uint32_t i = 0u; i < lin_ptr->msg_num[ch]; i++)
        {
            MicroCom_Lin_Table_t *t = &lin_ptr->table[ch][i];

            if (t->is_valid && t->is_trigger)
            {
                t->is_trigger = false;
                MicroCom_Lin_Invoke(t, MICROCOM_LIN_EVENT_RX);
            }
        }
#endif
    }
}

/* ========================================================================= */
/* 向上：驱动调用，通知调度结果                                             */
/* ========================================================================= */

/** 根据当前 active_idx 找到槽位，并校验 id/方向是否匹配，不匹配返回 NULL */
static MicroCom_Lin_Table_t *MicroCom_Lin_GetActiveEntry(uint8_t channel, uint8_t id, MicroCom_DIR_t expectDir)
{
    if (channel >= MICROCOM_LIN_CHANNEL_NUM)
    {
        return NULL;
    }

    uint8_t idx = lin_ptr->active_idx[channel];

    if (idx == MICROCOM_LIN_IDX_INVALID)
    {
        return NULL; /* 没有在等待的传输，忽略（可能是驱动的迟到/重复事件） */
    }

    MicroCom_Lin_Table_t *t = &lin_ptr->table[channel][idx];

    if ((t->id != id) || (t->dir != expectDir))
    {
        return NULL; /* 和当前记录的不匹配，防御性丢弃 */
    }

    return t;
}

void MicroCom_Lin_TxDone(uint8_t channel, uint8_t id)
{
    if(channel >= MICROCOM_LIN_CHANNEL_NUM)
    {
        return;
    }

    MicroCom_Lin_Table_t *t = MicroCom_Lin_GetActiveEntry(channel, id, MICROCOM_DIR_TX);

    if (t == NULL)
    {
        return;
    }

    lin_ptr->busy[channel]       = false;
    lin_ptr->active_idx[channel] = MICROCOM_LIN_IDX_INVALID;

    MicroCom_Lin_Invoke(t, MICROCOM_LIN_EVENT_TX_DONE);
}

void MicroCom_Lin_RxIndication(uint8_t channel, uint8_t id, const uint8_t *data, uint8_t len)
{
    MicroCom_Lin_Table_t *t = MicroCom_Lin_GetActiveEntry(channel, id, MICROCOM_DIR_RX);

    if (t == NULL)
    {
        return;
    }

    uint8_t copyLen = (len > t->length) ? t->length : len; /* 防止越界写 */

    memset(t->data, 0, t->length);
    memcpy(t->data, data, copyLen);

    lin_ptr->busy[channel]       = false;
    lin_ptr->active_idx[channel] = MICROCOM_LIN_IDX_INVALID;

    t->last_rx_time = lin_ptr->tick;
    t->is_timeout   = false; /* 收到一次成功数据，解除超时锁存 */

#if MICROCOM_FASTMODE_ENABLE == 0
    t->is_trigger = true;
#else
    MicroCom_Lin_Invoke(t, MICROCOM_LIN_EVENT_RX);
#endif
}

void MicroCom_Lin_RxError(uint8_t channel, uint8_t id)
{
    MicroCom_Lin_Table_t *t = MicroCom_Lin_GetActiveEntry(channel, id, MICROCOM_DIR_RX);

    if (t == NULL)
    {
        return;
    }

    lin_ptr->busy[channel]       = false;
    lin_ptr->active_idx[channel] = MICROCOM_LIN_IDX_INVALID;

    /* 不触发任何用户事件：next_time 已经在 Dispatch 时推进过，下个周期
     * 会自然重试；是否判定为"超时"完全交给 MicroCom_Lin_CheckTimeout()
     * 按用户配置的 timeout 周期性判断，不在这里直接对用户的单次失败
     * 做反应——这正是老代码"连续多次收不到才算丢失"想做但没做对的事。 */
}
#endif
