/**
 * @file MicroCom_Conf.h
 * @author https://xfp23.github.io/
 * @brief Public configuration interface for the MicroCom module.
 * @version \ref MICROCOM_VERSION
 * @date 2026-09-09
 *
 * @copyright Copyright (c) 2026
 */

#ifndef MICROCOM_CONF_H
#define MICROCOM_CONF_H

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief MicroCom version string.
 */
#define MICROCOM_VERSION "2.0.0"

/**
 * @brief MicroCom scheduler frequency in Hertz.
 *
 * This value defines the frequency at which the MicroCom scheduler
 * time base is updated.
 */
#define MICROCOM_FREQ_HZ (1000u)

/**
 * @brief Enable or disable fast callback mode.
 *
 * 0: Disabled
 * 1: Enabled
 *
 * When enabled, received event notifications can be delivered directly
 * without waiting for the normal scheduler processing.
 */
#define MICROCOM_FASTMODE_ENABLE (1u)

/**
 * @brief Enable or disable CAN event-message support.
 *
 * 0: Disabled
 * 1: Enabled
 *
 * When disabled, event-message configuration and scheduling resources
 * are not included.
 */
#define MICROCOM_CAN_EVENTMSG_ENABLE (1u)

/**
 * @brief Enable or disable LIN module support.
 * 
 */
#define MICROCOM_LIN_ENABLE          (1u)

/**
 * @brief Number of supported CAN channels.
 *
 * CAN channels are indexed from 0 to
 * (MICROCOM_CAN_CHANNEL_NUM - 1).
 */
#define MICROCOM_CAN_CHANNEL_NUM (2u)

/**
 * @brief Number of cyclic TX message slots per CAN channel.
 *
 * Each CAN channel provides this number of independent cyclic
 * transmission message slots.
 */
#define MICROCOM_CAN_CYCLEMSG_TX_SIZE (6u)

/**
 * @brief Number of cyclic RX message slots per CAN channel.
 *
 * Each CAN channel provides this number of independent cyclic
 * reception message slots.
 */
#define MICROCOM_CAN_CYCLEMSG_RX_SIZE (6u)

#if MICROCOM_CAN_EVENTMSG_ENABLE

/**
 * @brief Number of event-driven TX message slots per CAN channel.
 *
 * Each CAN channel provides this number of independent event-driven
 * transmission message slots.
 */
#define MICROCOM_CAN_EVENTMSG_TX_SIZE (3u)

/**
 * @brief Number of event-driven RX message slots per CAN channel.
 *
 * Each CAN channel provides this number of independent event-driven
 * reception message slots.
 */
#define MICROCOM_CAN_EVENTMSG_RX_SIZE (3u)

#endif

/**
 * @brief Maximum CAN frame data length in bytes.
 *
 * Use 8 for Classical CAN.
 * Use 64 for CAN FD.
 */
#define MICROCOM_CAN_MAX_DLC (8u)

/**
 * @brief Number of supported LIN channels.
 *
 * LIN channels are indexed from 0 to
 * (MICROCOM_LIN_CHANNEL_NUM - 1).
 */
#define MICROCOM_LIN_CHANNEL_NUM (1u)

/**
 * @brief Number of LIN message slots supported by each LIN channel.
 *
 * Each LIN channel provides this number of configurable message slots.
 */
#define MICROCOM_LIN_MSG_SIZE (2u)

#ifdef __cplusplus
}
#endif

#endif /* MICROCOM_CONF_H */
