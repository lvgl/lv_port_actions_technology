/**
 *  ***************************************************************************
 *  Copyright (c) 2003-2017 Actions Semiconductor. All rights reserved.
 *
 *  \file       usp_open.c
 *  \brief      open usp protocol
 *  \author     zhouxl
 *  \version    1.00
 *  \date       2017-12-06
 *  ***************************************************************************
 *
 *  \History:
 *  Version 1.00
 *       Initial release
 */


#include "usp_protocol_inner.h"



USP_PROTOCOL_STATUS OpenUSP(usp_handle_t *usp_handle)
{
    USP_PROTOCOL_STATUS ret;
    uint32_t open_para = usp_handle->usp_protocol_global_data.protocol_type;

    ret = SendUSPDataPacket(usp_handle, (uint8_t*)&open_para, sizeof(open_para), USP_OPEN_PROTOCOL);

    if (USP_PROTOCOL_OK == ret)
    {
        usp_handle->usp_protocol_global_data.opened = 1;
        usp_handle->usp_protocol_global_data.transparent =
            usp_handle->usp_protocol_global_data.transparent_bak;
    }

    return ret;
}


