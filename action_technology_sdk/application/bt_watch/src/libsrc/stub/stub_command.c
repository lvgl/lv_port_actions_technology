/*******************************************************************************
 *                                      US212A
 *                            Module: Nand Flash Driver
 *                 Copyright(c) 2003-2012 Actions Semiconductor,
 *                            All Rights Reserved.
 *
 * History:
 *      <author>    <time>           <version >             <desc>
 *       zhouxl    2017-4-7  11:39     1.0             build this file
*******************************************************************************/
/*!
 * \file     stub_command.c
 * \brief    stub protocol
 * \author   zhouxl
 * \par      GENERAL DESCRIPTION:
 *               function related to STUB
 * \par      EXTERNALIZED FUNCTIONS:
 *
 * \version 1.0
 * \date  2017-4-7
*******************************************************************************/

#include "stub_inner.h"



STUB_PROTOCOL_STATUS StubMessageLoop(void)
{
    int ret;
    stub_cmd_packet_t stub_cmd_packet;

    ret = USP_Protocol_Inquiry(stub_usp_op, (uint8_t*)&stub_cmd_packet, STUB_CMD_PACKET_LEN, NULL);
    if (STUB_CMD_PACKET_LEN == ret)
    {
        ret = STUB_PROTOCOL_COMMAND_NOT_SUPPORT;
    }

    return ret;
}



