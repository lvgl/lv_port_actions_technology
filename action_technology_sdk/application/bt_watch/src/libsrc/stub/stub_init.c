/**
 *  ***************************************************************************
 *  Copyright (c) 2003-2018 Actions Semiconductor. All rights reserved.
 *
 *  \file       stub_init.c
 *  \brief      Init stub protocol
 *  \author     zhouxl
 *  \version    1.00
 *  \date       2018-10-25
 *  ***************************************************************************
 *
 *  \History:
 *  Version 1.00
 *       Initial release
 */

#include "stub_inner.h"

usp_handle_t *stub_usp_op;
// uint8_t stub_open_flag;


void InitStub(usp_handle_t *stub_op)
{
    stub_usp_op = stub_op;

    stub_usp_op->usp_protocol_global_data.protocol_type = USP_PROTOCOL_TYPE_STUB;

    InitUSPProtocol(stub_usp_op);
}


void ExitStub(usp_handle_t *stub_op)
{
    ExitUSPProtocol(stub_op);
}


void stub_max_payload_modify(uint16_t tx_max, uint16_t rx_max)
{
    SetUSPProtocolMaxPayloadSize(stub_usp_op, tx_max, rx_max);
}

