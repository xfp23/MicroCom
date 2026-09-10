/**
 * @file MicroCom_Can.h
 * @author https://xfp23.github.io/
 * @brief Public interface for the MicroCom CAN message scheduling module.
 * @version 0.1
 * @date 2026-09-09
 *
 * @copyright Copyright (c) 2026
 *
 */

#ifndef MICROCOM_CAN_H
#define MICROCOM_CAN_H

#include "MicroCom_types.h"

#ifdef __cplusplus
extern "C"
{
#endif

/* ------------------------------------------------------------------------ */
/* Module Lifecycle                                                         */
/* ------------------------------------------------------------------------ */

/**

* @brief Initializes the CAN message scheduling module.
*
* This function must be called once before calling any other API
* provided by this module.
*
* @return MicroCom_Status_t
    */
MicroCom_Status_t MicroCom_Can_Init(void);

/**

* @brief Starts the CAN message scheduler.
    */
void MicroCom_Can_Start(void);

/**

* @brief Stops the CAN message scheduler.
    */
void MicroCom_Can_Stop(void);

/* ------------------------------------------------------------------------ */
/* Message Registration                                                     */
/* ------------------------------------------------------------------------ */

/**

* @brief Registers cyclic CAN transmit messages.
*
* @param table Pointer to the cyclic transmit message table.
* @param size  Number of entries in the message table.
*
* @return MicroCom_Status_t
    */
MicroCom_Status_t MicroCom_Can_Register_CycleTxMsg(const MicroCom_CanCycleTxMsg_t *table, size_t size);

/**

* @brief Registers cyclic CAN receive messages.
*
* @param table Pointer to the cyclic receive message table.
* @param size  Number of entries in the message table.
*
* @return MicroCom_Status_t
    */
MicroCom_Status_t MicroCom_Can_Register_CycleRxMsg(const MicroCom_CanCycleRxMsg_t *table, size_t size);

/**

* @brief Registers event-driven CAN transmit messages.
*
* @param table Pointer to the event transmit message table.
* @param size  Number of entries in the message table.
*
* @return MicroCom_Status_t
    */
MicroCom_Status_t MicroCom_Can_Register_EventTxMsg(const MicroCom_CanEventTxMsg_t *table, size_t size);

/**

* @brief Registers event-driven CAN receive messages.
*
* @param table Pointer to the event receive message table.
* @param size  Number of entries in the message table.
*
* @return MicroCom_Status_t
    */
MicroCom_Status_t MicroCom_Can_Register_EventRxMsg(const MicroCom_CanEventRxMsg_t *table, size_t size);

/* ------------------------------------------------------------------------ */
/* Scheduler Handlers                                                       */
/* ------------------------------------------------------------------------ */

/**

* @brief Handles the 1 ms scheduler tick.
*
* This function should typically be called from a 1 ms timer interrupt
* or a periodic system tick.
    */
void MicroCom_Can_TickHandler(void);

/**

* @brief Handles CAN message scheduling.
*
* This function should be called periodically from the main loop
* or a dedicated scheduler task.
    */
void MicroCom_Can_TimerHandler(void);

/* ------------------------------------------------------------------------ */
/* CAN Receive Interface                                                    */
/* ------------------------------------------------------------------------ */

/**

* @brief Notifies the MicroCom CAN module of a received CAN frame.
*
* This function should be called by the underlying CAN driver when
* a CAN frame is received.
*
* @param channel CAN channel on which the frame was received.
* @param can_id  CAN identifier of the received frame.
* @param data    Pointer to the received CAN data.
* @param len     Length of the received CAN data.
*
* @return MicroCom_Status_t
    */
MicroCom_Status_t MicroCom_Can_RxIndication(uint8_t channel, uint32_t can_id, bool is_Extend, const uint8_t *data, uint8_t len);

/* ------------------------------------------------------------------------ */
/* Event Message Management                                                 */
/* ------------------------------------------------------------------------ */

/**

* @brief Triggers an event-driven CAN transmit message.
*
* The trigger counter of the specified message is incremented.
* The message will be transmitted during the next scheduler cycle.
*
* @param id      CAN identifier of the message.
* @param channel CAN channel.
*
* @return MicroCom_Status_t
    */
MicroCom_Status_t MicroCom_Can_Trigger_EventMsg(uint32_t id, uint8_t channel);

/**

* @brief Marks an event message as bus-off.
*
* @param id      CAN identifier of the message.
* @param channel CAN channel.
*
* @return MicroCom_Status_t
    */
MicroCom_Status_t MicroCom_Can_SetEventOffline(uint32_t id, uint8_t channel);

/**

* @brief Clears the bus-off state of an event message.
*
* @param id      CAN identifier of the message.
* @param channel CAN channel.
*
* @return MicroCom_Status_t
    */
MicroCom_Status_t MicroCom_Can_ClearEventOffline(uint32_t id, uint8_t channel);

/* ------------------------------------------------------------------------ */
/* Diagnostic Session Control                                               */
/* ------------------------------------------------------------------------ */

/**

* @brief Disables all non-diagnostic CAN messages on the specified channel.
*
* This applies to both TX and RX messages, including cyclic and
* event-driven messages.
*
* Messages with is_diag == true are not affected.
*
* @param channel CAN channel.
*
* @return MicroCom_Status_t
    */
MicroCom_Status_t MicroCom_Can_DisableNonDiagnosticCom(uint8_t channel);

/**

* @brief Enables all non-diagnostic CAN messages on the specified channel.
*
* This applies to both TX and RX messages, including cyclic and
* event-driven messages.
*
* @param channel CAN channel.
*
* @return MicroCom_Status_t
    */
MicroCom_Status_t MicroCom_Can_EnableNonDiagnosticCom(uint8_t channel);

/**
 * @brief get the Frame offline Status
 *
 * @param channel CAN channel
 * @param id    CAN ID
 * @param is_extend CAN identifier type
 * @return true offline
 * @return false no offline
 */
extern bool MicroCom_Can_IsCycleRxBusOffline(uint8_t channel, uint32_t id, bool is_extend);

/* ------------------------------------------------------------------------ */
/* CAN Driver Hook                                                          */
/* ------------------------------------------------------------------------ */

/**

* @brief Transmits a CAN frame through the underlying CAN driver.
*
* This function is provided as a weak default implementation.
* Users may override this function to integrate their hardware-specific
* CAN driver.
*
* @param channel   CAN channel.
* @param can_id    CAN identifier.
* @param mbox      CAN mailbox index.
* @param dlc       CAN data length code.
* @param data      Pointer to CAN frame data.
* @param is_extend CAN identifier type.
* ```
                true  - Extended CAN identifier.
    ```
* ```
                false - Standard CAN identifier.
    ```
*
* @return MicroCom_Status_t
    */
MicroCom_Status_t MicroCom_Can_Transmit(uint8_t channel, uint32_t can_id, uint16_t mbox, uint8_t dlc, const uint8_t *data, bool is_extend);

#ifdef __cplusplus
}
#endif

#endif /* MICROCOM_CAN_H */
