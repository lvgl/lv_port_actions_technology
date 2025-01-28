/**
 *  ***************************************************************************
 *  Copyright (c) 2003-2017 Actions Semiconductor. All rights reserved.
 *
 *  \file       stub_open.c
 *  \brief      open stub connection
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


int OpenStub(uint32_t timeout_ms)
{
    int ret = -1;
/*
    if (stub_open_flag)
    {
        return 0;
    }
*/
    if (WaitUSPConnect(stub_usp_op, timeout_ms))
    {
        // stub_open_flag = 1;
        ret = 0;
    }

    return ret;
}



