/**
 *  ***************************************************************************
 *  Copyright (c) 2003-2018 Actions Semiconductor. All rights reserved.
 *  
 *  \file       usp_inquiry.c
 *  \brief      Inquiry USP protocol status
 *  \author     zhouxl
 *  \version    1.00
 *  \date       2018-10-24
 *  ***************************************************************************
 *  
 *  \History:
 *  Version 1.00
 *       Initial release
 */


#include "usp_protocol_inner.h"




/**
  \brief       Inquiry usp protocol status.
  \return      response code from peer device.
                 - == 0:    usp communication is OK
                 - others:  error occurred, \ref USP_PROTOCOL_STATUS
*/
USP_PROTOCOL_STATUS InquiryUSP(usp_handle_t *usp_handle)
{
    // 要避免循环调用
    if (usp_handle->usp_protocol_global_data.transparent)
    {
        return usp_handle->api.ioctl(USP_IOCTL_INQUIRY_STATE, NULL, NULL);
    }
    else
    {
        return SendUSPDataPacket(usp_handle, NULL, 0, USP_INQUIRY_STATUS);
    }
}





