/**
 * @file MicroCom_Can.h
 * @author https://xfp23.github.io/
 * @brief Public interface for the MicroCom Configure
 * @version 0.1
 * @date 2026-09-09
 * 
 * @copyright Copyright (c) 2026
 * 
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
#define MICROCOM_VERSION    "0.0.3"

/* ------------------------------------------------------------------------ */
/* Capacity Configuration                                                   */
/* ------------------------------------------------------------------------ */

/**

* @brief Number of supported CAN channels.
*
* CAN channels are indexed from 0 to (MICROCOM_CAN_CHANNEL_NUM - 1).
*/
#define MICROCOM_CAN_CHANNEL_NUM      (2u)

/**

* @brief Number of cyclic message slots per CAN channel.
*
* The TX and RX message tables each have this number of independent slots.
*/
#define MICROCOM_CAN_CYCLEMSG_SIZE    (8u)

/**

* @brief Number of event-driven message slots per CAN channel.
*
* The TX and RX message tables each have this number of independent slots.
*/
#define MICROCOM_CAN_EVENTMSG_SIZE    (8u)

/**

* @brief Maximum CAN frame data length.
*
* Use 8 for Classical CAN.
* Change this value to 64 to support CAN FD.
*/
#define MICROCOM_CAN_MAX_DLC          (8u)

#ifdef __cplusplus
}
#endif

#endif /* MICROCOM_CONF_H */
