/**
 *  ***************************************************************************
 *  Copyright (c) 2003-2021 Actions Semiconductor. All rights reserved.
 *
 *  \file       usp_baudrate.c
 *  \brief      usp speed config
 *  \author     zhouxl
 *  \version    1.00
 *  \date       2021-3-26
 *  ***************************************************************************
 *
 *  \History:
 *  Version 1.00
 *       Initial release
 */


#include "usp_protocol_inner.h"



bool SetUSPBaudrate(usp_handle_t *usp_handle, uint32_t baudrate)
{
    uint32_t cfg_baudrate = baudrate;

    if ((0 == usp_handle->usp_protocol_global_data.transparent) &&
        (USP_PROTOCOL_OK == SendUSPDataPacket(usp_handle, (uint8_t*)&cfg_baudrate, sizeof(uint32_t), USP_SET_BAUDRATE)))
    {
        // modify baudrate after send reponse
        usp_handle->api.ioctl(USP_IOCTL_SET_BAUDRATE, (void*)cfg_baudrate, NULL);
        return true;
    }

    return false;
}

