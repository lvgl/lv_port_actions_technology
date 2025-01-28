/**
 *  ***************************************************************************
 *  Copyright (c) 2003-2017 Actions Semiconductor. All rights reserved.
 *
 *  \file       stub_write.c
 *  \brief      stub write operation
 *  \author     zhouxl
 *  \version    1.00
 *  \date       2017-12-13
 *  ***************************************************************************
 *
 *  \History:
 *  Version 1.00
 *       Initial release
 */


#include "stub_inner.h"


STUB_PROTOCOL_STATUS send_stub_command(uint16_t opcode, uint32_t para, uint32_t payload_length)
{
    stub_cmd_packet_t stub_cmd_packet;

    memset(&stub_cmd_packet, 0, STUB_CMD_PACKET_LEN);
    stub_cmd_packet.magic           = STUB_HEAD_CMD_MAGIC;
    stub_cmd_packet.opcode          = (uint8_t)opcode;
    stub_cmd_packet.stub_id         = (uint8_t)(opcode >> 8);
    stub_cmd_packet.para            = para;
    stub_cmd_packet.payload_length  = payload_length;

    if (STUB_CMD_PACKET_LEN == WriteUSPData(stub_usp_op, (uint8_t*)&stub_cmd_packet, STUB_CMD_PACKET_LEN))
    {
        return STUB_PROTOCOL_OK;
    }
    else
    {
        return STUB_PROTOCOL_SEND_ERROR;
    }
}




int stub_send_data(uint8_t *data_buffer, uint32_t data_len)
{
    if (data_len == WriteUSPData(stub_usp_op, data_buffer, data_len))
    {
        return STUB_PROTOCOL_OK;
    }
    else
    {
        return STUB_PROTOCOL_SEND_ERROR;
    }
}


int stub_send_iso_data(uint8_t *data_buffer, uint32_t data_len)
{
    if (data_len == WriteUSPISOData(stub_usp_op, data_buffer, data_len))
    {
        return STUB_PROTOCOL_OK;
    }
    else
    {
        return STUB_PROTOCOL_SEND_ERROR;
    }
}



int stub_send_response(uint32_t status)
{
    stub_response_packet_t stub_response;

    memset(&stub_response, 0, sizeof(stub_response));
    stub_response.magic     = STUB_HEAD_RSP_MAGIC;
    stub_response.status    = (uint8_t)status;

    if (sizeof(stub_response) == WriteUSPData(stub_usp_op, (uint8_t*)&stub_response, sizeof(stub_response)))
    {
        return STUB_PROTOCOL_OK;
    }
    else
    {
        return STUB_PROTOCOL_SEND_ERROR;
    }
}




int stub_write_packet(uint16_t opcode, uint32_t op_para, uint8_t *data_buffer, uint32_t data_len)
{
    int ret = STUB_PROTOCOL_SEND_ERROR;

    if (STUB_PROTOCOL_OK != send_stub_command(opcode, op_para, data_len))
    {
        return ret;
    }

    if ((data_len) && (NULL != data_buffer))
    {
        ret = stub_send_data(data_buffer, data_len);
        if (STUB_PROTOCOL_OK != ret)
        {
            return ret;
        }
    }

    return stub_receive_response();
}



