/**
 * @file MicroCom_example.c
 * @author https://xfp23.github.io/
 * @brief MicroCom LIN usage example.
 * @version \ref MICROCOM_VERSION
 * @date 2026-10-06
 *
 * @details
 * This example demonstrates the basic integration flow of the MicroCom LIN
 * scheduling module:
 *
 *   1. Define LIN message configuration.
 *   2. Register the configuration table.
 *   3. Initialize and start the LIN scheduler.
 *   4. Call TickHandler() from a periodic timer interrupt.
 *   5. Call TimerHandler() from the main loop or scheduler task.
 *   6. Handle LIN events through a common callback.
 *
 * The low-level LIN driver is not implemented in this example.
 * A device-specific driver must provide strong implementations of:
 *
 *   MicroCom_Lin_Transmit()
 *   MicroCom_Lin_Receive()
 *
 * The driver reports hardware events through:
 *
 *   MicroCom_Lin_TxDone()
 *   MicroCom_Lin_RxIndication()
 *   MicroCom_Lin_RxError()
 *
 * @copyright Copyright (c) 2026
 */

#include "MicroCom_Lin.h"

uint8_t LIN_Example_Data[2][8];

/* -------------------------------------------------------------------------- */
/* LIN message configuration                                                  */
/* -------------------------------------------------------------------------- */

static void LinMessageCallback(MicroCom_Lin_Ctx_t *ctx);

static const MicroCom_Lin_ConfigTable_t g_linTable[] =
{
    {
        .channel = 0u,
        .id = 0x10u,
        .data = &LIN_Example_Data[0],
        .length = 4u,
        .period = MICROCOM_MS_TICK(100),
        .timeout = 0u, // tx不关心
        .callback = LinMessageCallback,
        .userdata = NULL,
        .dir = MICROCOM_DIR_TX,
    },

    {
        .channel = 0u,
        .id = 0x17u,
        .data = &LIN_Example_Data[0],
        .length = 4u,
        .period = MICROCOM_MS_TICK(100),
        .timeout = 3000u,
        .callback = LinMessageCallback,
        .userdata = NULL,
        .dir = MICROCOM_DIR_RX,
    },
};


/* -------------------------------------------------------------------------- */
/* LIN callback                                                               */
/* -------------------------------------------------------------------------- */

static void LinMessageCallback(MicroCom_Lin_Ctx_t *ctx)
{
    if (ctx == NULL)
    {
        return;
    }

    switch (ctx->event)
    {
        case MICROCOM_LIN_EVENT_TX:
            /*
             * The master TX request has been accepted by the LIN driver.
             */
            break;

        case MICROCOM_LIN_EVENT_TX_DONE:
            /*
             * The LIN driver reports that the transmission operation
             * has completed.
             */
            break;

        case MICROCOM_LIN_EVENT_TX_ERROR:
            /*
             * The LIN TX request was rejected by the low-level driver.
             */
            break;

        case MICROCOM_LIN_EVENT_RX:
            /*
             * A valid LIN slave response has been received.
             */
            if (ctx->data != NULL)
            {
                /*
                 * Process received data here.
                 */
            }
            break;

        case MICROCOM_LIN_EVENT_RX_ERROR:
            /*
             * The expected LIN response was not received within
             * the configured timeout.
             */
            break;

        default:
            break;
    }
}


/* -------------------------------------------------------------------------- */
/* LIN initialization                                                         */
/* -------------------------------------------------------------------------- */

void MicroCom_Example_LinInit(void)
{
    size_t size;

    size = sizeof(g_linTable) / sizeof(g_linTable[0]);

    if (MicroCom_Lin_Init() != MICROCOM_STATUS_OK)
    {
        return;
    }

    if (MicroCom_Lin_RegisterTable(
            g_linTable,
            &size) != MICROCOM_STATUS_OK)
    {
        return;
    }

    MicroCom_Lin_Start();
}


/* -------------------------------------------------------------------------- */
/* Periodic scheduler                                                         */
/* -------------------------------------------------------------------------- */

/*
 * Call this function from the configured MicroCom scheduler tick.
 *
 * Example:
 *
 *     void SysTick_Handler(void)
 *     {
 *         MicroCom_Lin_TickHandler();
 *     }
 */
void MicroCom_Example_LinTick(void)
{
    MicroCom_Lin_TickHandler();
}


/*
 * Call this function from the main loop or another scheduler task.
 *
 * Example:
 *
 *     while (1)
 *     {
 *         MicroCom_Lin_TimerHandler();
 *     }
 */
void MicroCom_Example_LinTask(void)
{
    MicroCom_Lin_TimerHandler();
}


/* -------------------------------------------------------------------------- */
/* Example application                                                        */
/* -------------------------------------------------------------------------- */

void MicroCom_Example_Init(void)
{
    MicroCom_Example_LinInit();
}
