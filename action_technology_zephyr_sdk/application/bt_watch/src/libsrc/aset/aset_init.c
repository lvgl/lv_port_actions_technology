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


void InitASET(usp_handle_t *aset_op)
{
    aset_op->usp_protocol_global_data.protocol_type = USP_PROTOCOL_TYPE_ASET;
    InitUSPProtocol(aset_op);
}

void ExitASET(usp_handle_t *aset_op)
{
    ExitUSPProtocol(aset_op);
}



