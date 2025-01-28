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
 * \file     usp_command.c
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



USP_PROTOCOL_STATUS ParseUSPCommand(usp_handle_t *usp_handle, uint8_t usp_cmd, uint32_t para_length)
{
    uint8_t para[32];
    int ret = USP_PROTOCOL_RX_ERR;
    uint32_t size;

    if (para_length > ARRAY_SIZE(para))
    {
        size = ARRAY_SIZE(para);
    }
    else
    {
        size = para_length;
    }

    if (USP_PROTOCOL_OK == ReceivingPayload(usp_handle, para, size))
    {
        ret = USP_PROTOCOL_OK;
        switch (usp_cmd)
        {
            case USP_DISCONNECT:
                SendingResponse(usp_handle, USP_PROTOCOL_OK);
                ret = USP_PROTOCOL_DISCONNECT;
                break;

            case USP_CONNECT:
                //对端会收到应答后会再回复一个ACK
                SendingResponse(usp_handle, USP_PROTOCOL_OK);
                ReceivingResponse(usp_handle);
                SetUSPConnectionFlag(usp_handle);

                if (0 == usp_handle->usp_protocol_global_data.master)
                {
                    // when in slave, should send its protocol info to master
                    ReportUSPProtocolInfo(usp_handle);
                }
                break;

            case USP_REPORT_PROTOCOL_INFO:
                SendingResponse(usp_handle, USP_PROTOCOL_OK);
                break;

            case USP_OPEN_PROTOCOL:
                ret = USP_PROTOCOL_NOT_SUPPORT_PROTOCOL;
                if (para[0] == usp_handle->usp_protocol_global_data.protocol_type)
                {
                    usp_handle->usp_protocol_global_data.opened = 1;
                    usp_handle->usp_protocol_global_data.transparent =
                        usp_handle->usp_protocol_global_data.transparent_bak;
                    ret = USP_PROTOCOL_OK;
                }
                SendingResponse(usp_handle, ret);
                break;

            case USP_SET_BAUDRATE:
                SendingResponse(usp_handle, USP_PROTOCOL_OK);
                //应答之后再去更新波特率
                usp_handle->api.ioctl(USP_IOCTL_SET_BAUDRATE, (void*)*((uint32_t*)para), NULL);
                break;

            case USP_INQUIRY_STATUS:
                ret = usp_handle->usp_protocol_global_data.state;
                SendingResponse(usp_handle, ret);
                break;

            case USP_SET_PAYLOAD_SIZE:
                usp_handle->usp_protocol_global_data.tx_max_payload_size = *((uint16_t*)para);
                usp_handle->usp_protocol_global_data.rx_max_payload_size = *((uint16_t*)para + 1);
                SendingResponse(usp_handle, USP_PROTOCOL_OK);
                break;

            case USP_REBOOT:
                SendingResponse(usp_handle, USP_PROTOCOL_OK);
                k_busy_wait((*(uint32_t*)para) * 1000 + 1);
                sys_pm_reboot(REBOOT_TYPE_NORMAL);
                break;

            default:
                //not support usp protocol command
                DiscardReceivedData(usp_handle);
                SendingResponse(usp_handle, USP_PROTOCOL_NOT_SUPPORT_CMD);
                ret = USP_PROTOCOL_NOT_SUPPORT_CMD;
                break;
        }
    }

    return ret;
}



void SetUSPProtocolState(usp_handle_t *usp_handle, USP_PROTOCOL_STATUS state)
{
    usp_handle->usp_protocol_global_data.state = state;
}



