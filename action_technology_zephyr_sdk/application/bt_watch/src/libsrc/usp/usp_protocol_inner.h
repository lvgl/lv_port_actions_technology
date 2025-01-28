/**
 *  ***************************************************************************
 *  Copyright (c) 2003-2018 Actions Semiconductor. All rights reserved.
 *
 *  \file       usp_protocol_inner.h
 *  \brief      usp(univesal serial protocol) head file
 *  \author     zhouxl
 *  \version    1.00
 *  \date       2018-10-19
 *  ***************************************************************************
 *
 *  \History:
 *  Version 1.00
 *       Initial release
 */

#ifndef __USP_PROTOCOL_INNER__
#define __USP_PROTOCOL_INNER__

#include <zephyr/types.h>
#include <sys/crc.h>
#include <soc.h>
#include <string.h>
#include <stdio.h>
#include <os_common_api.h>
#include "usp_protocol.h"


//工具发起连接时,先发送32byte连续的 0x63,设备收到4B连续0x63后,发起 usp connect 请求
#define USP_PC_CONNECT_MAGIC        (0x63636363)


// #define USP_DEBUG

#ifdef USP_DEBUG
void dump_mem(unsigned char *data, unsigned int size);
#define USP_DEBUG_PRINT            printf
#define USP_DUMP                   dump_mem
#else
#define USP_DEBUG_PRINT(...)       do {} while(0)
#define USP_DUMP(...)              do {} while(0)
#endif


#ifndef _ASSEMBLER_
typedef struct
{
    uint16_t protocol_version;                 ///< 0x0100: 1.00
    uint16_t idVendor;                         ///< 0xACCA: ACTIONS
    uint16_t reserved0;
    uint8_t  retry_count;                      ///< data transfer err(CRC check fail), max retry count
    uint8_t  reserved1;
    uint16_t timeout;                          ///< acknowledge timeout: ms unit
    uint16_t tx_max_payload;                   ///< current device tx packet max payload size
    uint16_t rx_max_payload;                   ///< current device rx packet max payload size
}usp_protocol_info_t;

extern const usp_protocol_info_t usp_protocol_info;


typedef enum __USP_COMMAND
{
    USP_CONNECT                =  ('c'),    ///< usp connect
    USP_DISCONNECT             =  ('d'),    ///< usp disconnect
    USP_REBOOT                 =  ('R'),    ///< reboot device
    USP_OPEN_PROTOCOL          =  ('o'),    ///< usp open protocol connection
    USP_SET_BAUDRATE           =  ('b'),    ///< set usp communication baudrate
    USP_SET_PAYLOAD_SIZE       =  ('p'),    ///< set usp packet max payload size
    USP_INQUIRY_STATUS         =  ('i'),    ///< inquiry usp communication status
    USP_REPORT_PROTOCOL_INFO   =  ('r'),    ///< report usp protocol infomation
}USP_PREDEFINED_COMMAND;



/**
 *  \brief Set USP connection flag.
 *
 *  \param [in] usp_handle  USP protocol handler.
 *
 *  \details
 */
void SetUSPConnectionFlag(usp_handle_t *usp_handle);


/**
 *  \brief Parse usp predefine_command.
 *
 *  \param [in] usp_handle  USP protocol handler.
 *  \param [in] usp_cmd     received usp command.
 *  \param [in] para_length paramenter length, byte unit.
 *  \return     execution status \ref USP_PROTOCOL_STATUS
 *
 *  \details
 */
USP_PROTOCOL_STATUS ParseUSPCommand(usp_handle_t *usp_handle, uint8_t usp_cmd, uint32_t para_length);


/**
 *  \brief send 1 data packet(witch ACK) to peer USP device, maybe send predefined command(no payload).
 *
 *  \param [in] usp_handle  USP protocol handler.
 *  \param [in] payload     To be send payload buffer pointer.
 *  \param [in] size        To be send payload number, byte unit, payload size MUST be small than USP_PAYLOAD_MAX_SIZE.
 *  \param [in] command     USP fundamental command
 *  \return     if successfully send payload to peer device.
 *                  - value >= 0: has been successful send payload bytes
 *                  - value < 0:  error occurred
 *
 *  \details
 */
USP_PROTOCOL_STATUS SendUSPDataPacket(usp_handle_t *usp_handle, uint8_t *payload, uint32_t size, uint8_t command);


/**
 *  \brief Sending ISO packet to peer device.
 *
 *  \param [in] usp_handle  USP protocol handler.
 *  \param [in] payload     To be send payload buffer pointer.
 *  \param [in] size        To be send payload number, byte unit.
 *  \return None
 *
 *  \details
 */
void SendUSPISOPacket(usp_handle_t *usp_handle, uint8_t *payload, uint32_t size);


/**
 *  \brief send 1 data packet to peer USP device.
 *
 *  \param [in] usp_handle  USP protocol handler.
 *  \param [in] payload     To be send payload buffer pointer.
 *  \param [in] size        To be send payload number, byte unit, payload size MUST be small than USP_PAYLOAD_MAX_SIZE.
 *  \param [in] type        USP packet type, \ref USP_PACKET_TYPE_E
 *  \param [in] command     USP fundamental command
 *  \return None
 *
 *  \details
 */
void SendingPacket(usp_handle_t *usp_handle, uint8_t *payload, uint32_t size, USP_PACKET_TYPE_E type, uint8_t command);


/**
 *  \brief receiving packet head data, and verify it, first data has been received.
 *
 *  \param [in] usp_handle  USP protocol handler.
 *  \param [in] head        received packet head data buffer.
 *  \return Packet head data received and verify result, \ref USP_PROTOCOL_STATUS.
 *
 *  \details
 */
USP_PROTOCOL_STATUS ReceivingPacketHeadExcepttype(usp_handle_t *usp_handle, usp_packet_head_t *head);


/**
 *  \brief receiving packet head data, and verify it.
 *
 *  \param [in] usp_handle  USP protocol handler.
 *  \param [in] head        received packet head data buffer.
 *  \return Packet head data received and verify result, \ref USP_PROTOCOL_STATUS.
 *
 *  \details
 */
USP_PROTOCOL_STATUS ReceivingPacketHead(usp_handle_t *usp_handle, usp_packet_head_t *head);


/**
 *  \brief Receiving usp payload data, and check if CRC16 is correct.
 *
 *  \param [in] usp_handle  USP protocol handler.
 *  \param [in] payload     To be received payload buffer pointer.
 *  \param [in] payload_len To be received payload number, byte unit, payload size MUST be small than USP_PAYLOAD_MAX_SIZE.
 *  \return execution status \ref USP_PROTOCOL_STATUS
 *
 *  \details
 */
USP_PROTOCOL_STATUS ReceivingPayload(usp_handle_t *usp_handle, uint8_t *payload, uint32_t payload_len);


/**
 *  \brief sending respond packet to peer USP device.
 *
 *  \param [in] usp_handle  USP protocol handler.
 *  \param [in] status      respond status code, \ref USP_PROTOCOL_STATUS.
 *  \return None
 *
 *  \details
 */
void SendingResponse(usp_handle_t *usp_handle, USP_PROTOCOL_STATUS status);


/**
 *  \brief receiving respond data, and verify it.
 *
 *  \param [in] usp_handle USP protocol handler.
 *  \return Respond data received and verify result, \ref USP_PROTOCOL_STATUS.
 *
 *  \details
 */
USP_PROTOCOL_STATUS ReceivingResponse(usp_handle_t *usp_handle);


/**
 *  \brief discard all received USP data.
 *
 *  \param [in] usp_handle  USP protocol handler.
 *  \return     Discard received data bytes number.
 *
 *  \details
 */
int DiscardReceivedData(usp_handle_t *usp_handle);


/**
 *  \brief report usp protocol infomation to peer device, \ref usp_protocol_info_t.
 *
 *  \param [in] usp_handle  USP protocol handler.
 *
 *  \details
 */
void ReportUSPProtocolInfo(usp_handle_t *usp_handle);


#endif /*_ASSEMBLER_*/
#endif  /* __USP_PROTOCOL_INNER__ */

