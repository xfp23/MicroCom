/**
 * @file MicroCom_utils.h
 * @brief 参数校验宏。全部要求调用处函数返回类型为 MicroCom_Status_t。
 *
 * @copyright Copyright (c) 2026
 */
#ifndef MICROCOM_UTILS_H
#define MICROCOM_UTILS_H

#include "MicroCom_conf.h"
#include "MicroCom_types.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define MICROCOM_CHECK_PTR(ptr) \
    do { if ((ptr) == NULL) { return MICROCOM_PARAM_INVALID; } } while (0)

#define MICROCOM_CHECK_CAN_CHANNEL(ch) \
    do { if ((ch) >= MICROCOM_CAN_CHANNEL_NUM) { return MICROCOM_CHANNEL_OVERFLOW; } } while (0)

#define MICROCOM_CHECK_CYCLE_CAN_SIZE(size) \
    do { if ((size) == 0u || (size) > (MICROCOM_CAN_CHANNEL_NUM * MICROCOM_CAN_CYCLEMSG_SIZE)) { return MICROCOM_PARAM_INVALID; } } while (0)

#define MICROCOM_CHECK_EVENT_CAN_SIZE(size) \
    do { if ((size) == 0u || (size) > (MICROCOM_CAN_CHANNEL_NUM * MICROCOM_CAN_EVENTMSG_SIZE)) { return MICROCOM_PARAM_INVALID; } } while (0)

#define MICROCOM_CHECK_DLC(dlc) \
    do { if ((dlc) > MICROCOM_CAN_MAX_DLC) { return MICROCOM_PARAM_INVALID; } } while (0)

#define MICROCOM_SKIP_INVALID(x) if(!x) { continue; }

#define MICROCOM_MS_TICK(ms) (((uint32_t)(ms) * MICROCOM_FREQ_HZ) / 1000U)

#ifdef __cplusplus
}
#endif

#endif /* MICROCOM_UTILS_H */
