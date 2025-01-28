/**
 *  ***************************************************************************
 *  Copyright (c) 2003-2021 Actions Semiconductor. All rights reserved.
 *
 *  \file       stub_baudrate.c
 *  \brief      stub baudrate config
 *  \author     zhouxl
 *  \version    1.00
 *  \date       2021-3-26
 *  ***************************************************************************
 *
 *  \History:
 *  Version 1.00
 *       Initial release
 */


#include "stub_inner.h"



bool stub_speed_set(uint32_t baudrate)
{
    return SetUSPBaudrate(stub_usp_op, baudrate);
}

