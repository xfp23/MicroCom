/**
 * @file MicroCom_Lin.h
 * @author https://xfp23.github.io/
 * @brief Public interface of the MicroCom LIN scheduling module.
 * @version \ref MICROCOM_VERSION
 * @date 2026-09-09
 *
 * @details
 * Dependency direction:
 * The low-level LIN driver depends on this module, rather than this module
 * depending on the low-level driver.
 *
 * MicroCom_Lin_Transmit() and MicroCom_Lin_Receive() are declared in this
 * file and provide weak default implementations in MicroCom_Lin.c.
 * A device-specific driver (LPUART, FlexIO, or another vendor peripheral)
 * can provide strong implementations with the same names to integrate the
 * hardware layer.
 *
 * When changing the MCU or LIN peripheral, only these two functions need to
 * be reimplemented. MicroCom_Lin.c itself does not need to be modified.
 *
 * MicroCom_Lin_TxDone(), MicroCom_Lin_RxIndication(), and
 * MicroCom_Lin_RxError() are implemented by MicroCom_Lin.c.
 * The low-level driver must call these functions when a transmission or
 * reception completes or fails, allowing the scheduler to update its state.
 *
 * @copyright Copyright (c) 2026
 */

#ifndef MICROCOM_LIN_H
#define MICROCOM_LIN_H

#include "MicroCom_Lin_types.h"

#ifdef __cplusplus
extern "C"
{
#endif

#if MICROCOM_LIN_ENABLE
/**
 * @brief Initialize the MicroCom LIN scheduling module.
 *
 * This function initializes all internal states required by the LIN
 * scheduler. No LIN frame scheduling is performed until
 * MicroCom_Lin_Start() is called.
 *
 * @return Module initialization status.
 */
extern MicroCom_Status_t MicroCom_Lin_Init(void);

/**
 * @brief Register a LIN frame configuration table.
 *
 * The registration process uses the same two-pass validation strategy as
 * MicroCom_Can:
 *
 * 1. Validate the complete configuration without modifying any internal
 *    module state.
 * 2. Commit the configuration only after all validation checks succeed.
 *
 * If validation fails, the module state remains unchanged.
 *
 * All LIN frame configurations must be registered before
 * MicroCom_Lin_Start() is called.
 *
 * @param table Pointer to the LIN configuration table.
 * @param size  Number of entries in the configuration table.
 *
 * @return Registration status.
 */
extern MicroCom_Status_t MicroCom_Lin_RegisterTable(const MicroCom_Lin_ConfigTable_t *table, size_t size);

/**
 * @brief Start LIN frame scheduling.
 *
 * This function starts the LIN scheduler and synchronizes the time base of
 * all registered frames.
 *
 * After this function returns, MicroCom_Lin_TickHandler() and
 * MicroCom_Lin_TimerHandler() can begin processing registered LIN frames.
 *
 * Frame registration must be completed before calling this function.
 */
extern void MicroCom_Lin_Start(void);

/**
 * @brief Stop LIN frame scheduling.
 *
 * This function stops the LIN scheduler. No new LIN transmission or
 * reception request will be initiated while the scheduler is stopped.
 *
 * Registered frame configurations are retained and can be used again after
 * MicroCom_Lin_Start() is called.
 */
extern void MicroCom_Lin_Stop(void);

/**
 * @brief Process one LIN scheduler tick.
 *
 * This function must be called once for every scheduler tick.
 *
 * The tick handler is responsible for maintaining the scheduler time base.
 * It does not directly initiate LIN frame transmission or reception.
 */
extern void MicroCom_Lin_TickHandler(void);

/**
 * @brief Execute the LIN scheduling logic.
 *
 * For each LIN channel, the scheduler uses a polling cursor to locate the
 * next frame whose transmission time has arrived and initiates the
 * corresponding transfer.
 *
 * This function also checks all configured RX frames for reception timeout.
 *
 * MicroCom_Lin_Transmit() and MicroCom_Lin_Receive() are initiated only from
 * this scheduling process. Their scheduling behavior is not affected by
 * MICROCOM_FASTMODE_ENABLE.
 *
 * Fast mode only determines when received data is delivered to the upper
 * layer; it does not change when the scheduler decides to initiate the next
 * transfer.
 */
extern void MicroCom_Lin_TimerHandler(void);

/**
 * @brief Submit a LIN master transmit request to the low-level driver.
 *
 * The low-level driver shall transmit the LIN header and provide the
 * specified data as the slave response.
 *
 * This function is asynchronous. A successful return only means that the
 * transfer request has been accepted by the low-level driver. It does not
 * indicate that the LIN frame has already been transmitted.
 *
 * The low-level driver must call MicroCom_Lin_TxDone() after the physical
 * transmission has completed.
 *
 * The default weak implementation returns a failure status when no actual
 * hardware driver is provided.
 *
 * The data buffer is referenced asynchronously by the low-level driver.
 * Therefore, the caller must keep the buffer valid and unchanged until
 * MicroCom_Lin_TxDone() is called.
 *
 * @param channel Logical LIN channel number.
 * @param id      6-bit LIN frame identifier (0x00 to 0x3F).
 * @param data    Pointer to the response data.
 * @param len     Number of response data bytes. Must not exceed 8.
 *
 * @return Submission status.
 */
extern MicroCom_Status_t MicroCom_Lin_Transmit(uint8_t channel, uint8_t id, const uint8_t *data, uint8_t len);

/**
 * @brief Submit a LIN master receive request to the low-level driver.
 *
 * The low-level driver shall transmit the LIN header and wait for the
 * slave to provide the response data.
 *
 * This function is asynchronous and does not require the caller to provide
 * a receive buffer. After the response has been received, the low-level
 * driver shall call MicroCom_Lin_RxIndication() with a pointer to the
 * received data and its actual length.
 *
 * @param channel Logical LIN channel number.
 * @param id      6-bit LIN frame identifier (0x00 to 0x3F).
 * @param len     Expected response data length. Must not exceed 8.
 *
 * @return Submission status.
 */
extern MicroCom_Status_t MicroCom_Lin_Receive(uint8_t channel, uint8_t id, uint8_t len);

/**
 * @brief Notify the LIN scheduler that a transmission has completed.
 *
 * This function is called by the low-level LIN driver after the requested
 * LIN frame has been physically transmitted successfully.
 *
 * The function updates the corresponding scheduler state and allows the
 * next scheduled transfer to proceed.
 *
 * @param channel Logical LIN channel number.
 * @param id      6-bit LIN frame identifier associated with the completed
 *                transmission.
 */
extern void MicroCom_Lin_TxDone(uint8_t channel, uint8_t id);

/**
 * @brief Notify the LIN scheduler that a reception has completed.
 *
 * This function is called by the low-level LIN driver after a LIN response
 * has been received successfully.
 *
 * The received data is passed to the scheduler together with its actual
 * length for subsequent processing and delivery to the upper layer.
 *
 * @param channel Logical LIN channel number.
 * @param id      6-bit LIN frame identifier associated with the received
 *                response.
 * @param data    Pointer to the received response data.
 * @param len     Number of valid bytes in the received response.
 */
extern void MicroCom_Lin_RxIndication(uint8_t channel, uint8_t id, const uint8_t *data, uint8_t len);

/**
 * @brief Notify the LIN scheduler that a reception has failed.
 *
 * This function is called by the low-level LIN driver when a LIN reception
 * cannot be completed successfully, for example because of a timeout,
 * checksum error, or other hardware-level reception error.
 *
 * The scheduler uses this notification to terminate the pending reception
 * and update the corresponding frame state.
 *
 * @param channel Logical LIN channel number.
 * @param id      6-bit LIN frame identifier associated with the failed
 *                reception.
 */
extern void MicroCom_Lin_RxError(uint8_t channel, uint8_t id);

/**
 * @brief Check the frame is offline
 * 
 * @param channel Logical LIN channel number.
 * @param id 6-bit LIN frame identifier associated with the received
 *                response.
 * @return true 
 * @return false 
 */
extern bool MicroCom_Lin_IsRxOffline(uint8_t channel, uint8_t id);

#endif

#ifdef __cplusplus
}
#endif

#endif /* MICROCOM_LIN_H */
