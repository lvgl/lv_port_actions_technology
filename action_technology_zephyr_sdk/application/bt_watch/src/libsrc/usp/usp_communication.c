/*******************************************************************************
 *                                      US283C
 *                            Module: usp driver
 *                 Copyright(c) 2003-2017 Actions Semiconductor,
 *                            All Rights Reserved.
 *
 * History:
 *      <author>    <time>           <version >             <desc>
 *       zhouxl    2017-4-7  11:39     1.0             build this file
*******************************************************************************/
/*!
 * \file     usp_communication.c
 * \brief    usp protocol packet R/W
 * \author   zhouxl
 * \par      GENERAL DESCRIPTION:
 *               function related to usp
 * \par      EXTERNALIZED FUNCTIONS:
 *
 * \version 1.0
 * \date  2017-4-7
*******************************************************************************/

#include "usp_protocol_inner.h"




/**
  \brief       send 1 iso packet(witchout ACK) to peer USP device.
*/
void SendUSPISOPacket(usp_handle_t *usp_handle, uint8_t *payload, uint32_t size)
{
    SendingPacket(usp_handle, payload, size, USP_PACKET_TYPE_ISO, 0);
}



/**
  \brief       send user payload to peer USP device, no need ACK from peer device.
  \return      payload bytes of successfully send to peer device.
*/
int WriteUSPISOData(usp_handle_t *usp_handle, uint8_t *user_payload, uint32_t size)
{
    int total_size;
    uint32_t send_bytes;

    total_size = size;

    //payload可以为空
    do
    {
        if (total_size  > usp_handle->usp_protocol_global_data.tx_max_payload_size)
        {
            send_bytes  = usp_handle->usp_protocol_global_data.tx_max_payload_size;
        }
        else
        {
            send_bytes  = total_size;
        }

        SendUSPISOPacket(usp_handle, user_payload, send_bytes);

        if (NULL != user_payload)
        {
            user_payload    = user_payload + send_bytes;
            total_size      = total_size - send_bytes;
        }
    } while (total_size > 0);

    return (size - total_size);
}



/**
  \brief       send 1 data packet(witch ACK) to peer USP device, maybe send predefined command(no payload).
  \return      if successfully send data to peer device.
                 - == 0:    ACK received
                 - others:  error occurred, \ref USP_PROTOCOL_STATUS
*/
USP_PROTOCOL_STATUS SendUSPDataPacket(usp_handle_t *usp_handle, uint8_t *payload, uint32_t size, uint8_t command)
{
    int status = USP_PROTOCOL_DATA_CHECK_FAIL, retry_count = 0;

    DiscardReceivedData(usp_handle);

    do
    {
        //接收方明确告知数据校验fail才重传
        if (USP_PROTOCOL_DATA_CHECK_FAIL == status)
        {
            SendingPacket(usp_handle, payload, size, USP_PACKET_TYPE_DATA, command);
        }

        status = ReceivingResponse(usp_handle);
        if (USP_PROTOCOL_OK == status)
        {
            break;
        }

        //only payload check fail need to retry
        if (USP_PROTOCOL_DATA_CHECK_FAIL != status)
        {
            USP_DEBUG_PRINT("out err：%d.\n", status);
            break;
        }

        retry_count++;
    } while (retry_count < usp_handle->usp_protocol_global_data.max_retry_count);

    return status;
}



/**
  \brief       send user payload to peer USP device, maybe send predefined command(no payload).
  \return      payload bytes of successfully send to peer device.
*/
int WriteUSPData(usp_handle_t *usp_handle, uint8_t *user_payload, uint32_t size)
{
    int total_size, ret;
    uint32_t send_bytes;

    total_size = size;

    //payload可以为空
    do
    {
        if (total_size  > usp_handle->usp_protocol_global_data.tx_max_payload_size)
        {
            send_bytes  = usp_handle->usp_protocol_global_data.tx_max_payload_size;
        }
        else
        {
            send_bytes  = total_size;
        }

        if (usp_handle->usp_protocol_global_data.transparent)
        {
            ret = usp_handle->api.write(user_payload, send_bytes, 0);
            if (ret != send_bytes)
            {
                break;
            }
        }
        else
        {
            if (USP_PROTOCOL_OK != SendUSPDataPacket(usp_handle, user_payload, send_bytes, 0))
            {
                break;
            }
        }

        if (NULL != user_payload)
        {
            user_payload    = user_payload + send_bytes;
            total_size      = total_size - send_bytes;
        }
    } while (total_size > 0);

    return (size - total_size);
}



/**
  \brief       receive 1 iso packet(witchout ACK) from peer USP device.
  \return      if successfully received payload bytes from peer device.
                 - == 0: has been successfully received all payload data
                 - others:  error occurred, \ref USP_PROTOCOL_STATUS
*/
int ReceiveUSPIsoPacket(usp_handle_t *usp_handle, uint8_t *data, uint32_t size)
{
    return 0;
}



/**
  \brief       receive 1 data packet(witch ACK) from peer USP device, maybe send predefined command(no payload).
  \return      if successfully received payload bytes from peer device.
                 - == 0: has been successfully received all payload data
                 - others:  error occurred, \ref USP_PROTOCOL_STATUS
*/
int ReceiveUSPDataPacket(usp_handle_t *usp_handle, usp_packet_head_t *recv_head, uint8_t *payload, uint32_t size)
{
    uint32_t retry_count, payload_len = 0;
    int status;
    usp_packet_head_t head;
    usp_packet_head_t *recv_head_bak = recv_head;

    retry_count = 0;
    do
    {
        if (recv_head)
        {
            status = USP_PROTOCOL_OK;
            memcpy(&head, recv_head, USP_PACKET_HEAD_SIZE);
            recv_head = NULL;
        }
        else
        {
            status = ReceivingPacketHead(usp_handle, &head);
            if(recv_head_bak)
                *recv_head_bak = head;
        }

        if (USP_PROTOCOL_OK == status)
        {
            payload_len = head.payload_len;
            if (usp_handle->usp_protocol_global_data.protocol_type != head.protocol_type)
            {
                status  = USP_PROTOCOL_NOT_EXPECT_PROTOCOL;
            }

            //防止内存访问越界
            if (payload_len > size)
            {
                status  = USP_PROTOCOL_PAYLOAD_TOOLARGE;
            }
        }

        if (USP_PROTOCOL_OK == status)
        {
            status  = ReceivingPayload(usp_handle, payload, head.payload_len);
        }

        if (USP_PROTOCOL_OK != status)
        {
            //invalid usp packet
            DiscardReceivedData(usp_handle);
        }

        //timeour or none specific error type, will NOT send response
        if ((USP_PROTOCOL_TIMEOUT != status) && (USP_PROTOCOL_ERR != status))
        {
            SendingResponse(usp_handle, (uint8_t)status);
        }

        //only payload check fail need to retry
        if (USP_PROTOCOL_DATA_CHECK_FAIL != status)
        {
            break;
        }

        //数据没成功接收到,继续retry
        retry_count++;
    } while (retry_count < usp_handle->usp_protocol_global_data.max_retry_count);

    if (USP_PROTOCOL_OK == status)
    {
        return payload_len;
    }
    else
    {
        USP_DEBUG_PRINT("in err: %d\n", status);
        return -status;
    }
}




/**
  \brief       receive user payload from peer USP device, maybe is predefined command(no payload).
  \return      successfully received payload bytes from peer device.
*/
int ReadUSPData(usp_handle_t *usp_handle, uint8_t *user_payload, uint32_t size)
{
    uint32_t total_size, receive_bytes;
    int ret;

    if (NULL == user_payload)
    {
        return -1;
    }

    total_size = size;

    //payload可以为空
    do
    {
        if (total_size > usp_handle->usp_protocol_global_data.rx_max_payload_size)
        {
            receive_bytes = usp_handle->usp_protocol_global_data.rx_max_payload_size;
        }
        else
        {
            receive_bytes = total_size;
        }

        if (usp_handle->usp_protocol_global_data.transparent)
        {
            ret = usp_handle->api.read(user_payload, receive_bytes,
                usp_handle->usp_protocol_global_data.rx_timeout);
            if (ret != receive_bytes)
            {
                break;
            }
        }
        else
        {
            ret = ReceiveUSPDataPacket(usp_handle, NULL, user_payload, receive_bytes);
            if (ret < 0)
            {
                break;
            }
        }

        receive_bytes   = (uint32_t)ret;
        user_payload    = user_payload + receive_bytes;
        total_size      = total_size - receive_bytes;
    } while (total_size > 0);

    return (size - total_size);
}







