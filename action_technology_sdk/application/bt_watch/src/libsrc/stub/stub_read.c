/**
 *  ***************************************************************************
 *  Copyright (c) 2003-2017 Actions Semiconductor. All rights reserved.
 *
 *  \file       stub_read.c
 *  \brief      stub read operation
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



int stub_receive_data(uint8_t *data_buffer, uint32_t data_len)
{
    if (data_len != ReadUSPData(stub_usp_op, data_buffer, data_len))
    {
        return -STUB_PROTOCOL_RECEIVE_ERROR;
    }

    return data_len;
}




int stub_receive_response(void)
{
    stub_response_packet_t stub_response;
    int ret = STUB_PROTOCOL_RECEIVE_ERROR;

    if (sizeof(stub_response) == ReadUSPData(stub_usp_op, (uint8_t*)&stub_response, sizeof(stub_response)))
    {
        if (STUB_HEAD_RSP_MAGIC == stub_response.magic)
        {
            ret = stub_response.status;
        }
    }

    return ret;
}



int stub_read_packet(uint16_t opcode, uint32_t op_para, uint8_t *data_buffer, uint32_t data_len)
{
    int ret;
    if (STUB_PROTOCOL_OK != send_stub_command(opcode, op_para, data_len))
    {
        return -STUB_PROTOCOL_SEND_ERROR;
    }

    ret = stub_receive_data(data_buffer, data_len);

    return ret;
}



int stub_status_inquiry(void)
{
    int ret;

    ret = InquiryUSP(stub_usp_op);
    if ((USP_PROTOCOL_OK != ret) && (USP_PROTOCOL_BUSY != ret))
    {
        ret = STUB_PROTOCOL_ERROR;
    }

    return ret;
}




