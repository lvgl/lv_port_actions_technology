/**
 *  ***************************************************************************
 *  Copyright (c) 2003-2017 Actions Semiconductor. All rights reserved.
 *
 *  \file       stub_inner.h
 *  \brief      stub interface
 *  \author     zhouxl
 *  \version    1.00
 *  \date       2017-12-13
 *  ***************************************************************************
 *
 *  \History:
 *  Version 1.00
 *       Initial release
 */



#ifndef __STUB_INNER_H__
#define __STUB_INNER_H__


#include <zephyr/types.h>
#include <soc.h>
#include <string.h>
#include <os_common_api.h>
#include "stub.h"


extern usp_handle_t *stub_usp_op;
extern uint8_t stub_open_flag;


/**
 *  \brief Send stub command to peer device.
 *
 *  \param [in] opcode          stub opcode, defined in stub_command.h.
 *  \param [in] para            stub para.
 *  \param [in] payload_length  Next data packet payload length.
 *  \return If stub command send success or not. \ref in STUB_PROTOCOL_STATUS
 *
 *  \details
 */
STUB_PROTOCOL_STATUS send_stub_command(uint16_t opcode, uint32_t para, uint32_t payload_length);


#endif


