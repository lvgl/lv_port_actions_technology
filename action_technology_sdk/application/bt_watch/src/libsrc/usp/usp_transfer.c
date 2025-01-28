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
 * \file     usp_transfer.c
 * \brief    USP data transfer
 * \author   zhouxl
 * \par      GENERAL DESCRIPTION:
 *               function related to usp protocol
 * \par      EXTERNALIZED FUNCTIONS:
 *
 * \version 1.0
 * \date  2017-4-7
*******************************************************************************/

#include "usp_protocol_inner.h"


/**
  \brief       send packet head and payload to peer USP device.
  \return      Sending payload bytes num.
*/
extern unsigned char crc8_maxim(unsigned char seed, unsigned char *p, unsigned int counter);

void SendingPacket(usp_handle_t *usp_handle, uint8_t *payload, uint32_t size, USP_PACKET_TYPE_E type, uint8_t command)
{
    uint16_t crc16_val = 0;
    uint32_t payload_bytes;
	uint32_t irq_flag;
    usp_packet_head_t head;

    memset(&head, 0, USP_PACKET_HEAD_SIZE);
    if (NULL == payload)
    {
        payload_bytes = 0;
    }
    else
    {
        payload_bytes = size;
    }

    head.type               = type;
    head.predefine_command  = command;

    if (USP_PACKET_TYPE_RSP != type)
    {
        head.protocol_type  = usp_handle->usp_protocol_global_data.protocol_type;
        head.sequence_number= usp_handle->usp_protocol_global_data.out_seq_number;
        head.payload_len    = payload_bytes;
    }

    if (command)
    {
        head.protocol_type  = USP_PROTOCOL_TYPE_FUNDAMENTAL;
    }

    head.crc8_val           = crc8_maxim(0, (uint8_t*)&head, USP_PACKET_HEAD_SIZE - 1);

    //如无payload,无需再发送CRC16码
    if (0 != payload_bytes)
    {
        //packet head与payload间增加计算时间,留够时间给PC处理
        //计算1KByte CRC16耗时1.03ms(CPU=26M)
        crc16_val = crc16_ccitt(0, payload, payload_bytes);
    }

    // usp_handle->api.ioctl(USP_IOCTL_ENTER_WRITE_CRITICAL, (void*)true, NULL);
    irq_flag = irq_lock();

    //sending packet head
    usp_handle->api.write((uint8_t*)&head, USP_PACKET_HEAD_SIZE, 0);

    //如无payload,无需再发送CRC16码
    if (0 != payload_bytes)
    {
        //sending CRC16
        usp_handle->api.write((uint8_t*)&crc16_val, 2, 0);
        //sending payload data
        usp_handle->api.write(payload, payload_bytes, 0);
    }

    // usp_handle->api.ioctl(USP_IOCTL_ENTER_WRITE_CRITICAL, (void*)false, NULL);
    irq_unlock(irq_flag);

    //wait write finish
    // while(usp_handle->api.ioctl(USP_IOCTL_CHECK_WRITE_FINISH, NULL, NULL));

    usp_handle->usp_protocol_global_data.out_seq_number++;
}



/**
  \brief       sending ACK packet to peer USP device.
*/
void SendingResponse(usp_handle_t *usp_handle, USP_PROTOCOL_STATUS status)
{
    SendingPacket(usp_handle, NULL, 0, USP_PACKET_TYPE_RSP, status);
}



/**
  \brief       receiving response from peer USP device.
  \return      response code from peer device.
                 - == 0:    ACK received
                 - others:  error occurred, \ref USP_PROTOCOL_STATUS
*/
USP_PROTOCOL_STATUS ReceivingResponse(usp_handle_t *usp_handle)
{
    USP_PROTOCOL_STATUS ret;
    usp_packet_head_t head;

    ret = ReceivingPacketHead(usp_handle, &head);

    if ((USP_PROTOCOL_OK           == ret)
        && (USP_PACKET_TYPE_RSP    != head.type))
    {
        USP_DEBUG_PRINT("%d, err: %d\n", __LINE__, head.type);
        ret = USP_PROTOCOL_ERR;
    }

    if (ret)
    {
        USP_DEBUG_PRINT("%d, err: %d\n", __LINE__, ret);
    }

    return ret;
}



/**
  \brief       receiving 1 packet payload(NOT include usp_packet_head) from peer USP device.
  \return      if successfully received payload from peer device.
                 - == 0:    payload data has been successful received
                 - others:  error occurred, \ref USP_PROTOCOL_STATUS
*/
USP_PROTOCOL_STATUS ReceivingPayload(usp_handle_t *usp_handle, uint8_t *payload, uint32_t payload_len)
{
    uint16_t crc16_val = 0, reading_crc16 = 0;
    uint32_t received_bytes;

    if (NULL == payload)
    {
        return USP_PROTOCOL_ERR;
    }

    if (0 != payload_len)
    {
        usp_handle->api.read((uint8_t*)&reading_crc16, 2, usp_handle->usp_protocol_global_data.rx_timeout);
        received_bytes = usp_handle->api.read(payload, payload_len, usp_handle->usp_protocol_global_data.rx_timeout);
        if (payload_len != received_bytes)
        {
            USP_DEBUG_PRINT("%d, err: %d(%d)\n", __LINE__, received_bytes, payload_len);
            // return USP_PROTOCOL_RX_ERR;
            // payload长度不足, 让对端设备重传
            return USP_PROTOCOL_DATA_CHECK_FAIL;
        }

        crc16_val = crc16_ccitt(0, payload, payload_len);
    }

    if (0 == (reading_crc16 - crc16_val))
    {
        return USP_PROTOCOL_OK;
    }
    else
    {
        USP_DUMP(payload, payload_len);
        USP_DEBUG_PRINT("%d, err: %x\n", __LINE__, crc16_val);
        return USP_PROTOCOL_DATA_CHECK_FAIL;
    }
}



/**
  \brief       receiving 1 packet head data from peer USP device, first byte(type) has been read.
  \return      if successfully received correct packet head from peer device.
                 - == 0:    rest of packet head data has been successful received
                 - others:  error occurred, \ref USP_PROTOCOL_STATUS
*/
USP_PROTOCOL_STATUS ReceivingPacketHeadExcepttype(usp_handle_t *usp_handle, usp_packet_head_t *head)
{
    int ret;
    uint32_t received_bytes;

    //data already been received in USP RX FIFO
    // rx timeout 不能小于底层驱动最小收数 interval
    received_bytes = usp_handle->api.read((uint8_t*)head + 1, USP_PACKET_HEAD_SIZE - 1, USP_PROTOCOL_RX_TIMEOUT) + 1;

    //CRC8校验+比较时间必须 < 13.3us = (8B * 10b/B / 6Mbps)
    if ((USP_PACKET_HEAD_SIZE   == received_bytes)
        && (head->crc8_val      == crc8_maxim(0, (uint8_t*)head, USP_PACKET_HEAD_SIZE - 1)))
    {
        ret = USP_PROTOCOL_OK;
    }
    else
    {
        USP_DEBUG_PRINT("%d, err: %d\n", __LINE__, received_bytes);
        USP_DUMP((uint8_t*)head, USP_PACKET_HEAD_SIZE);
        ret = USP_PROTOCOL_ERR;
    }

    return ret;
}





/**
  \brief       receiving response from peer USP device.
  \return      response code from peer device.
                 - == 0:    ACK received
                 - others:  error occurred, \ref USP_PROTOCOL_STATUS
*/
USP_PROTOCOL_STATUS ReceivingPacketHead(usp_handle_t *usp_handle, usp_packet_head_t *head)
{
    int status;

    //检查ACK,对方处理数据时间不能超过ack_timeout
    if (1 == usp_handle->api.read((uint8_t*)head, 1, usp_handle->usp_protocol_global_data.rx_timeout))
    {
        status  = ReceivingPacketHeadExcepttype(usp_handle, head);
    }
    else
    {
        USP_DEBUG_PRINT("%d, err: %d\n", __LINE__, USP_PROTOCOL_TIMEOUT);
        status  = USP_PROTOCOL_TIMEOUT;
    }

    if ((USP_PROTOCOL_OK           == status)
        && (USP_PACKET_TYPE_RSP    == head->type))
    {
        //status is in the same postion as predefine_command
        status  = head->predefine_command;
    }

    if (status)
    {
        USP_DEBUG_PRINT("%d, err: %d\n", __LINE__, status);
    }

    return status;
}



/**
  \brief       discard ALL data received in usp RX FIFO.
  \return      discard usp rx data bytes.
*/
int DiscardReceivedData(usp_handle_t *usp_handle)
{
    // k_busy_wait(1000);
    //读空 USP RX FIFO,防止对端在TX时发送数据,影响协议传输
    return usp_handle->api.read(NULL, -1, 1);
}




