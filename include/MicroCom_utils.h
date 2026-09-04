#ifndef MICROCOM_UTILS_H
#define MICROCOM_UTILS_H

#ifdef __cplusplus
extern "C"
{
#endif

#define MICROCOM_CHECK_PRT(x)              \
    do                                     \
    {                                      \
        if (x == NULL)                     \
        {                                  \
            return MICROCOM_PARAM_INVALID; \
        }                                  \
    } while (0)

#define MICROCOM_SIZEOF_CANMSG_TABLE(x) (sizeof(x) / sizeof(x[0]))

#define MICROCOM_CHECK_CAN_SIZE(x)          \
    do                                      \
    {                                       \
        if (x >= MICROCOM_CAN_MESSAGE_SIZE) \
        {                                   \
            return MICROCOM_PARAM_INVALID;  \
        }                                   \
    } while (0)

#ifdef __cplusplus
}
#endif

#endif
