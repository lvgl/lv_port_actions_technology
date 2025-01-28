/**
 *  ***************************************************************************
 *  Copyright (c) 2003-2018 Actions Semiconductor. All rights reserved.
 *
 *  \file       aset_init.c
 *  \brief      Init ASET protocol
 *  \author     zhouxl
 *  \version    1.00
 *  \date       2018-10-25
 *  ***************************************************************************
 *
 *  \History:
 *  Version 1.00
 *       Initial release
 */

#include "aset_inner.h"


int ReadASETPacket(usp_handle_t *aset_op, uint8_t* payload, uint32_t size)
{
    if (size != ReadUSPData(aset_op, payload, size))
    {
        return -ASET_RECEIVE_ERROR;
    }

    return size;
}


