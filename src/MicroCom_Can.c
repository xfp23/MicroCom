#include "MicroCom_Can.h"

#define CHECK_PTR MICROCOM_CHECK_PRT
#define CHECK_SIZE MICROCOM_CHECK_CAN_SIZE

static MicroCOM_CAN_Obj_t can_obj = {0};

MicroCom_Status_t MicroCom_Can_Init(void)
{
    memset(&can_obj,0,sizeof(MicroCOM_CAN_Obj_t));

    return MICROCOM_STATUS_OK;
}

MicroCom_Status_t MicroCom_Can_Register_Msg(MicroCom_CanMessage_t *table,size_t size)
{
    CHECK_PTR(table);
    CHECK_SIZE(size);

    for(int i = 0; i < MICROCOM_CAN_MESSAGE_SIZE; i++)
    {

    }
}