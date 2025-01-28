/*******************************************************************************
 *                                      US283C
 *                            Module: usp Driver
 *                 Copyright(c) 2003-2017 Actions Semiconductor,
 *                            All Rights Reserved.
 *
 * History:
 *      <author>    <time>           <version >             <desc>
 *       zhouxl    2017-4-7  11:39     1.0             build this file
*******************************************************************************/
/*!
 * \file     usp_disconnect.c
 * \brief    usp protocol
 * \author   zhouxl
 * \par      GENERAL DESCRIPTION:
 *               function related to usp
 * \par      EXTERNALIZED FUNCTIONS:
 *
 * \version 1.0
 * \date  2017-4-7
*******************************************************************************/

#include "usp_protocol_inner.h"



bool DisconnectUSP(usp_handle_t *usp_handle)
{
    int ret;

    if (usp_handle->usp_protocol_global_data.connected)
    {
        SendUSPDataPacket(usp_handle, (uint8_t*)&ret, 0, USP_DISCONNECT);
        usp_handle->usp_protocol_global_data.connected = 0;
    }

    ExitUSPProtocol(usp_handle);

    return true;
}



