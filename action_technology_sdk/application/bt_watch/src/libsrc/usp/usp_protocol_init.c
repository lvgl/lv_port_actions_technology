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
 * \file     usp_protocol_init.c
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


const usp_protocol_info_t usp_protocol_info =
{
    .protocol_version   = 0x130,
    .idVendor           = 0xACCA,
    .reserved0          = 0,
    .retry_count        = USP_PROTOCOL_MAX_RETRY_COUNT,
    .reserved1          = 0,
    .timeout            = USP_PROTOCOL_RX_TIMEOUT,
    .tx_max_payload     = USP_PAYLOAD_MAX_SIZE,
    .rx_max_payload     = USP_PAYLOAD_MAX_SIZE,
};




void InitUSPProtocol(usp_handle_t *usp_handle)
{
    if (!usp_handle->usp_protocol_global_data.protocol_init_flag)
    {
        // usp_handle->usp_protocol_global_data.connected              = 0;
        usp_handle->usp_protocol_global_data.protocol_init_flag     = 1;
        // usp_handle->usp_protocol_global_data.out_seq_number         = 0;
        //未连接阶段,retry次数减小,防止陷入时间太长
        usp_handle->usp_protocol_global_data.max_retry_count        = 1;
        usp_handle->usp_protocol_global_data.rx_timeout             = USP_PROTOCOL_RX_TIMEOUT;

        usp_handle->usp_protocol_global_data.tx_max_payload_size    = USP_PAYLOAD_MAX_SIZE;
        // reduce size to save rx buffer
        usp_handle->usp_protocol_global_data.rx_max_payload_size    = 128;

        // USP_Tx_Init(USP_CPU_TRANSFER | USP_DATA_INQUIRY, NULL, NULL);
        //通过CPU轮询方式,处理USP RX端数据
        // USP_Rx_Init(USP_CPU_TRANSFER | USP_DATA_INQUIRY, NULL, NULL);

        /* 配置 USP stub 通信波特率 */
        // USP_Configure(USP_PROTOCOL_DEFAULT_BAUDRATE);
    }
}




void ExitUSPProtocol(usp_handle_t *usp_handle)
{
    if (usp_handle->usp_protocol_global_data.protocol_init_flag)
    {
        usp_handle->usp_protocol_global_data.protocol_init_flag     = 0;
        // USP_Rx_Exit();
        // USP_Tx_Exit();
    }
}


void SetUSPProtocolMaxPayloadSize(usp_handle_t *usp_handle, uint16_t tx_max, uint16_t rx_max)
{
    usp_handle->usp_protocol_global_data.tx_max_payload_size = tx_max;
    usp_handle->usp_protocol_global_data.rx_max_payload_size = rx_max;
}
