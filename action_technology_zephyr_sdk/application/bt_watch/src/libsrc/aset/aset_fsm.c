/**
 *  ***************************************************************************
 *  Copyright (c) 2003-2018 Actions Semiconductor. All rights reserved.
 *
 *  \file       aset_fsm.c
 *  \brief      ASET finite state machine
 *  \author     zhouxl
 *  \version    1.00
 *  \date       2018-12-28
 *  ***************************************************************************
 *
 *  \History:
 *  Version 1.00
 *       Initial release
 */


#include "aset_inner.h"



ASET_STATUS ASET_Protocol_Rx_Fsm(usp_handle_t *aset_op)
{
    ASET_STATUS ret = ASET_TIMEOUT;
    aset_cmd_packet_t aset_cmd_packet;

    ret = USP_Protocol_Inquiry(aset_op, (uint8_t*)&aset_cmd_packet, ASET_CMD_PACKET_LEN, NULL);
    if (ASET_CMD_PACKET_LEN == ret)
    {
        ret = aset_op->handle_hook(&aset_cmd_packet, NULL, NULL);
    }

    return ret;
}

