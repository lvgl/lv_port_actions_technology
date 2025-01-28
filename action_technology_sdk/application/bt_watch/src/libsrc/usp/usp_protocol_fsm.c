/**
 *  ***************************************************************************
 *  Copyright (c) 2003-2018 Actions Semiconductor. All rights reserved.
 *
 *  \file       usp_protocol_fsm.c
 *  \brief      usp protocol finite state machine
 *  \author     zhouxl
 *  \version    1.00
 *  \date       2018-12-26
 *  ***************************************************************************
 *
 *  \History:
 *  Version 1.00
 *       Initial release
 */

#include "usp_protocol_inner.h"



int USP_Protocol_Inquiry(usp_handle_t *usp_handle, uint8_t *payload, uint32_t length, usp_packet_head_t* recv_head)
{
    int ret = -1;
    usp_packet_head_t head;

    if (usp_handle->usp_protocol_global_data.transparent)
    {
        if ((NULL != payload) && (0 != length))
        {
            ret = ReadUSPData(usp_handle, payload, length);
        }
    }
    else
    {
        ret = ReceivingPacketHead(usp_handle, &head);
        if ((USP_PROTOCOL_OK == ret) && (NULL != recv_head))
        {
            memcpy(recv_head, &head, USP_PACKET_HEAD_SIZE);
        }

        if (USP_PROTOCOL_OK != ret)
        {
            //invalid USP protocol packet
            DiscardReceivedData(usp_handle);
            ret = -ret;
        }
        else if (USP_PACKET_TYPE_RSP == head.type)
        {
            //status is in the same postion as predefine_command
            ret = -head.predefine_command;
        }
        else if ((USP_PACKET_TYPE_DATA      == head.type)   &&
            (USP_PROTOCOL_TYPE_FUNDAMENTAL  == head.protocol_type))
        {
            ret = -ParseUSPCommand(usp_handle, head.predefine_command, head.payload_len);
        }
        else if ((NULL != payload) && (0 != length) &&
            (usp_handle->usp_protocol_global_data.protocol_type == head.protocol_type))
        {
            ret = ReceiveUSPDataPacket(usp_handle, &head, payload, length);
        }
    }

    return ret;
}




