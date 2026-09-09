/**
 * @file example.c
 * @brief MicroCom CAN 模块使用示例
 *
 * 覆盖内容：
 *   - 周期发送报文（CycleTx）
 *   - 周期接收报文（CycleRx，超时后 is_offline 锁存，恢复收报后自动清除）
 *   - 事件发送报文（EventTx，按需 Trigger）
 *   - 事件接收报文（EventRx，is_offline 需要用户手动 Set/Clear）
 *   - 诊断会话期间挂起/恢复非诊断报文
 *   - 与底层驱动的对接方式（Transmit、Tick/Timer/RxIndication 挂载点）
 *
 * @copyright Copyright (c) 2026
 */
#include "MicroCom_Can.h"
#include <stdio.h>

/* ------------------------------------------------------------------------ */
/* 通道 / ID 定义                                                            */
/* ------------------------------------------------------------------------ */

#define CH_POWERTRAIN   0u   /* 通道 0：动力总线 */
#define CH_BODY         1u   /* 通道 1：车身总线 */

#define ID_ENGINE_STATUS   0x200u  /* 周期发送：本机发动机状态 */
#define ID_VEHICLE_SPEED   0x300u  /* 周期接收：整车车速 */
#define ID_DOOR_REQUEST    0x400u  /* 事件发送：车门请求，按需触发 */
#define ID_REMOTE_KEY      0x500u  /* 事件接收：遥控钥匙信号 */

/* ------------------------------------------------------------------------ */
/* 用户数据缓冲区（组件不分配内存，长度必须 >= 对应报文的 dlc）              */
/* ------------------------------------------------------------------------ */

static uint8_t s_engine_status_tx[8] = {0};
static uint8_t s_vehicle_speed_rx[8] = {0};
static uint8_t s_door_request_tx[8]  = {0};
static uint8_t s_remote_key_rx[8]    = {0};

/* ------------------------------------------------------------------------ */
/* 回调函数                                                                  */
/*                                                                          */
/* 回调运行在 TimerHandler / RxIndication 的调用上下文里（通常是中断），     */
/* 保持轻量，不要做阻塞操作；本示例里用 printf 只是为了演示。                */
/* ------------------------------------------------------------------------ */

static void OnEngineStatusSent(MicroCom_Ctx_t *ctx)
{
    /* 每次发送前更新一次报文内容，这里演示成简单的计数器 */
    s_engine_status_tx[0]++;
    (void)ctx;
}

static void OnVehicleSpeed(MicroCom_Ctx_t *ctx)
{
    switch (ctx->event)
    {
    case MICROCOM_EVENT_RX:
        printf("[CAN%u] 0x%03X vehicle speed updated: %u km/h\n",
               ctx->channel, (unsigned)ctx->id, s_vehicle_speed_rx[0]);
        break;

    case MICROCOM_EVENT_ERROR:
        /* 超过 timeout 未收到车速报文，is_offline 已被组件自动置位并锁存，
         * 直到下一帧正确的车速报文到来才会被 RxIndication 自动清除 */
        printf("[CAN%u] 0x%03X vehicle speed RX timeout, offline!\n",
               ctx->channel, (unsigned)ctx->id);
        break;

    default:
        break;
    }
}

static void OnDoorRequestSent(MicroCom_Ctx_t *ctx)
{
    printf("[CAN%u] 0x%03X door request sent\n", ctx->channel, (unsigned)ctx->id);
}

static void OnRemoteKey(MicroCom_Ctx_t *ctx)
{
    if (ctx->event == MICROCOM_EVENT_ERROR)
    {
        /* 我们自己通过 SetEventOffline 触发的回调，见下方 */
        printf("[CAN%u] 0x%03X remote key marked offline by app\n",
               ctx->channel, (unsigned)ctx->id);
        return;
    }

    printf("[CAN%u] 0x%03X remote key frame received, cmd=0x%02X\n",
           ctx->channel, (unsigned)ctx->id, s_remote_key_rx[0]);

    /* 事件报文没有自动超时机制，如果这里检测到异常数据，
     * 可以手动置位 is_offline，通知其它模块该信号暂不可信： */
    if (s_remote_key_rx[0] == 0xFF)
    {
        MicroCom_Can_SetEventOffline(ID_REMOTE_KEY, CH_BODY);
    }
}

/* ------------------------------------------------------------------------ */
/* 注册表                                                                    */
/*                                                                          */
/* 只需要填业务字段（id/dlc/channel/data/func/...），is_run、is_valid、      */
/* next_time/last_rx_time、trigger、is_offline 这些运行时字段由 Register_*  */
/* 内部强制初始化，这里填了也会被覆盖，不用管。                             */
/* ------------------------------------------------------------------------ */

static const MicroCom_CanCycleTxMsg_t s_cycle_tx_table[] = {
    {
        .id = ID_ENGINE_STATUS,
        .is_Extend = false,
        .dlc = 8,
        .mbox_id = 0,
        .channel = CH_POWERTRAIN,
        .is_diag = false,     /* 非诊断报文，诊断会话期间会被自动挂起 */
        .cycle = 100,         /* 100 个调度 tick 发送一次 */
        .data = s_engine_status_tx,
        .userData = NULL,
        .func = OnEngineStatusSent,
    },
};

static const MicroCom_CanCycleRxMsg_t s_cycle_rx_table[] = {
    {
        .id = ID_VEHICLE_SPEED,
        .is_Extend = false,
        .dlc = 8,
        .mbox_id = 0,
        .channel = CH_POWERTRAIN,
        .is_diag = false,
        .timeout = 500,       /* 500 tick 内没收到就判超时并置 is_offline */
        .data = s_vehicle_speed_rx,
        .userData = NULL,
        .func = OnVehicleSpeed,
    },
};

static const MicroCom_CanEventTxMsg_t s_event_tx_table[] = {
    {
        .id = ID_DOOR_REQUEST,
        .is_Extend = false,
        .dlc = 1,
        .mbox_id = 1,
        .channel = CH_BODY,
        .is_diag = false,
        .data = s_door_request_tx,
        .userData = NULL,
        .func = OnDoorRequestSent,
    },
};

static const MicroCom_CanEventRxMsg_t s_event_rx_table[] = {
    {
        .id = ID_REMOTE_KEY,
        .is_Extend = false,
        .dlc = 2,
        .mbox_id = 1,
        .channel = CH_BODY,
        .is_diag = false,
        .data = s_remote_key_rx,
        .userData = NULL,
        .func = OnRemoteKey,
    },
};

/* ------------------------------------------------------------------------ */
/* 底层发送钩子：对接真实 CAN 驱动                                          */
/*                                                                          */
/* 这里只是打印代替真实发送，实际工程里替换成 HAL_CAN_AddTxMessage() /      */
/* 具体外设寄存器操作 / RTOS 消息队列投递等。                               */
/* ------------------------------------------------------------------------ */

MicroCom_Status_t MicroCom_Can_Transmit(uint8_t channel, uint32_t can_id, uint16_t mbox,
                                         uint8_t dlc, const uint8_t *data, bool is_extend)
{
    printf("[CAN%u] TX id=0x%03X mbox=%u dlc=%u extend=%d data[0]=0x%02X\n",
           channel, (unsigned)can_id, mbox, dlc, is_extend, dlc ? data[0] : 0);

    return MICROCOM_STATUS_OK;
}

/* ------------------------------------------------------------------------ */
/* 初始化                                                                    */
/* ------------------------------------------------------------------------ */

static void App_CanInit(void)
{
    MicroCom_Can_Init();

    /* 全部注册操作必须在 Start() 之前完成；注册失败（返回非 OK）时
     * can_obj 状态保持不变，可以直接打印状态排查配置问题。 */
    MicroCom_Status_t st;

    st = MicroCom_Can_Register_CycleTxMsg(s_cycle_tx_table, 1);
    if (st != MICROCOM_STATUS_OK) { printf("Register CycleTx failed: %d\n", st); }

    st = MicroCom_Can_Register_CycleRxMsg(s_cycle_rx_table, 1);
    if (st != MICROCOM_STATUS_OK) { printf("Register CycleRx failed: %d\n", st); }

    st = MicroCom_Can_Register_EventTxMsg(s_event_tx_table, 1);
    if (st != MICROCOM_STATUS_OK) { printf("Register EventTx failed: %d\n", st); }

    st = MicroCom_Can_Register_EventRxMsg(s_event_rx_table, 1);
    if (st != MICROCOM_STATUS_OK) { printf("Register EventRx failed: %d\n", st); }

    MicroCom_Can_Start();
}

/* ------------------------------------------------------------------------ */
/* 驱动挂载点：分别挂到定时中断和 CAN 接收中断/任务里                       */
/* ------------------------------------------------------------------------ */

/** 挂到 1ms 定时中断（例如 SysTick） */
static void App_1msTick(void)
{
    MicroCom_Can_TickHandler();
    MicroCom_Can_TimerHandler();
}

/** 挂到 CAN 接收中断/任务，把驱动收到的一帧喂给调度器 */
static void App_CanRxCallback(uint8_t channel, uint32_t id, const uint8_t *data, uint8_t len)
{
    MicroCom_Status_t st = MicroCom_Can_RxIndication(channel, id, true,data, len);

    if (st == MICROCOM_NOT_FIND)
    {
        /* 收到了一帧本组件未注册过的 ID，按需忽略或记录日志 */
    }
}

/* ------------------------------------------------------------------------ */
/* 业务动作：诊断会话控制、事件触发                                          */
/* ------------------------------------------------------------------------ */

/** 进入诊断会话时调用：挂起本通道上所有非诊断报文 */
static void App_EnterDiagSession(uint8_t channel)
{
    MicroCom_Can_DisableNonDiagnosticCom(channel);
}

/** 退出诊断会话时调用：恢复正常通信 */
static void App_ExitDiagSession(uint8_t channel)
{
    MicroCom_Can_EnableNonDiagnosticCom(channel);
}

/** 用户按下车门按键时调用：触发一次事件发送 */
static void App_OnDoorButtonPressed(void)
{
    s_door_request_tx[0] = 0x01; /* 请求开锁 */
    MicroCom_Can_Trigger_EventMsg(ID_DOOR_REQUEST, CH_BODY);
}

/* ------------------------------------------------------------------------ */
/* 简单的时序演示（仅用于本地验证逻辑，不代表真实 MCU 主循环写法）           */
/* ------------------------------------------------------------------------ */

int main(void)
{
    App_CanInit();

    for (int t = 0; t < 700; t++)
    {
        App_1msTick();

        /* 第 50 tick 时模拟收到一帧车速报文 */
        if (t == 50)
        {
            uint8_t speed_frame[8] = {60, 0, 0, 0, 0, 0, 0, 0};
            App_CanRxCallback(CH_POWERTRAIN, ID_VEHICLE_SPEED, speed_frame, sizeof(speed_frame));
        }

        /* 第 80 tick 时模拟用户按下车门按键 */
        if (t == 80)
        {
            App_OnDoorButtonPressed();
        }

        /* 第 100 tick 进入诊断会话，第 130 tick 退出：
         * 期间 CycleTx / CycleRx 上 is_diag == false 的条目会被挂起，
         * 恢复后重新参与调度。 */
        if (t == 100) { App_EnterDiagSession(CH_POWERTRAIN); }
        if (t == 130) { App_ExitDiagSession(CH_POWERTRAIN); }

        /* 第 130 tick 之后车速报文不再更新，last_rx_time 停留在 t=50，
         * timeout=500，预计在 t ≈ 550 触发一次 MICROCOM_EVENT_ERROR 回调 */
    }

    return 0;
}