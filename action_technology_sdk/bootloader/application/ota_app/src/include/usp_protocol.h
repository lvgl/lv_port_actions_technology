/**
 *  ***************************************************************************
 *  Copyright (c) 2003-2018 Actions Semiconductor. All rights reserved.
 *
 *  \file       usp_protocol.h
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

#ifndef __USP_PROTOCOL_H__
#define __USP_PROTOCOL_H__

#include <zephyr/types.h>
#include <stdbool.h>


// RX timeout, ms unit
#define USP_PROTOCOL_RX_TIMEOUT         (200)

// USP protocol re-transfer count
#define USP_PROTOCOL_MAX_RETRY_COUNT    (5)

#define USP_PAYLOAD_MAX_SIZE            (1024)


#ifndef _ASSEMBLER_
/**
\brief  usp protocol status of executed operation
*/
typedef enum __USP_PROTOCOL_STATUS
{
    USP_PROTOCOL_OK =  0,               ///< Operation succeeded
    USP_PROTOCOL_BUSY,

    USP_PROTOCOL_ERR = 10,              ///< Unspecified error
    USP_PROTOCOL_TX_ERR,                ///< usp protocol send data error happend
    USP_PROTOCOL_RX_ERR,                ///< usp protocol receive data error happend
    USP_PROTOCOL_DATA_CHECK_FAIL,
    USP_PROTOCOL_PAYLOAD_TOOLARGE,      ///< packet payload length too large to transfer

    USP_PROTOCOL_TIMEOUT = 20,          ///< usp protocol transaction timeout
    USP_PROTOCOL_DISCONNECT,            ///< disconnect USP

    USP_PROTOCOL_NOT_EXPECT_PROTOCOL = 30,  ///< NOT expected usp protocol
    USP_PROTOCOL_NOT_SUPPORT_PROTOCOL,  ///< NOT support usp protocol
    USP_PROTOCOL_NOT_SUPPORT_BAUDRATE,  ///< NOT support usp baudrate
    USP_PROTOCOL_NOT_SUPPORT_CMD,       ///< Not support usp command
}USP_PROTOCOL_STATUS;


/**
\brief  usp protocol packet type
*/
typedef enum
{
    USP_PACKET_TYPE_DATA            = 0x01,
    USP_PACKET_TYPE_ISO             = 0x16,
    USP_PACKET_TYPE_RSP             = 0x05,
}USP_PACKET_TYPE_E;


/**
\brief  usp protocol type
*/
typedef enum
{
    USP_PROTOCOL_TYPE_FUNDAMENTAL   = 0,
    USP_PROTOCOL_TYPE_STUB          = 1,
    USP_PROTOCOL_TYPE_ASET          = 2,
    USP_PROTOCOL_TYPE_ADFU          = 8,
}USP_PROTOCOL_TYPE_E;


/**
\brief  usp protocol data transfer direction
*/
typedef enum
{
    USP_PROTOCOL_DIRECTION_TX       = 0,
    USP_PROTOCOL_DIRECTION_RX,
}USP_PROTOCOL_TRANSFER_DIRECTION_E;


/**
\brief  usp protocol connect mode
*/
typedef enum
{
    USP_INITIAL_CONNECTION = 0,     ///< initial usp connect
    USP_CHECK_INITIAL_CONNECTION,   ///< if received matched data, initial usp connect
    USP_WAIT_CONNECTION,            ///< wait usp connect
} USP_CONNECT_MODE_E;


typedef struct
{
    u8_t   type;
    u8_t   sequence_number:4;
    u8_t   protocol_type:4;
    u16_t  payload_len;            ///< data length for payload only, not include packet head & crc16
    u8_t   predefine_command;
    u8_t   crc8_val;
}usp_packet_head_t;


typedef struct
{
    usp_packet_head_t head;
    u16_t crc16_val;
    u8_t payload[0];
}usp_data_pakcet_t;


typedef struct
{
    u8_t   type;
    u8_t   reserved[3];
    u8_t   status;                 ///< respond status, \ref USP_PROTOCOL_STATUS
    u8_t   crc8_val;
}usp_ack_packet_t;


#define USP_PACKET_HEAD_SIZE       (sizeof(usp_packet_head_t))


typedef struct
{
    u8_t   type;
    u8_t   reserved:4;
    u8_t   protocol_type:4;
    int (*packet_handle_hook)(usp_packet_head_t *head);
}usp_protocol_fsm_item_t;


typedef struct
{
    u8_t    protocol_type;

    // 0: slave; 1: master
    u8_t    master;

    u8_t    protocol_init_flag:1;
    u8_t    connected:1;
    u8_t    opened:1;
    // on some channel(such as USB, SPP), use transparent transfer
    u8_t    transparent:1;
    u8_t    reserved:4;

    u8_t    out_seq_number:4;
    u8_t    in_seq_number:4;

    u8_t    state;
    u8_t    max_retry_count;
    u16_t   rx_timeout;

    u16_t   tx_max_payload_size;
    u16_t   rx_max_payload_size;
} usp_protocol_global_data_t;


enum usp_phy_ioctl_cmd_e
{
    USP_IOCTL_SET_BAUDRATE = 1,
    USP_IOCTL_INQUIRY_STATE,
    USP_IOCTL_ENTER_WRITE_CRITICAL,
    USP_IOCTL_CHECK_WRITE_FINISH,
};

typedef struct
{
    int (*read)(u8_t *data, u32_t length, u32_t timeout_ms);
    int (*write)(u8_t *data, u32_t length, u32_t timeout_ms);
    int (*ioctl)(u32_t mode, void *para1, void *para2);
}usp_phy_interface_t;


typedef struct
{
    usp_protocol_global_data_t usp_protocol_global_data;
    usp_phy_interface_t api;
    int (*handle_hook)(void *para1, void *para2, void *para3);
}usp_handle_t;


/**
 *  \brief Initial usp protocol.
 *
 *  \param [in] usp_handle  USP protocol handler.
 *
 *  \details
 */
void InitUSPProtocol(usp_handle_t *usp_handle);


/**
 *  \brief exit usp protocol..
 *
 *  \param [in] usp_handle  USP protocol handler.
 *
 *  \details
 */
void ExitUSPProtocol(usp_handle_t *usp_handle);


/**
 *  \brief Initial usp connection protocol.
 *
 *  \param [in] usp_handle  USP protocol handler.
 *  \return     Whether usp protocol connection is established.
 *                  TRUE:   success
 *                  FALSE:  fail
 *
 *  \details
 */
bool ConnectUSP(usp_handle_t *usp_handle);


 /**
 *  \brief Check connection flag and initiate usp connection.
 *
 *  \param [in] usp_handle  USP protocol handler.
 *  \param [in] timeout_ms  Maximun wait time, millisecond unit.
 *  \return     Whether usp protocol connection is established.
 *                  TRUE:   success
 *                  FALSE:  fail
 *
 *  \details More details
 */
bool Check_ConnectUSP(usp_handle_t *usp_handle, u32_t timeout_ms);


 /**
 *  \brief Wait peer device to initiate usp connection.
 *
 *  \param [in] usp_handle  USP protocol handler.
 *  \param [in] timeout_ms  Maximun wait time, millisecond unit.
 *  \return     Whether usp protocol connection is established.
 *                  TRUE:   success
 *                  FALSE:  fail
 *
 *  \details More details
 */
bool WaitUSPConnect(usp_handle_t *usp_handle, u32_t timeout_ms);


/**
 *  \brief Disconnect usp protocol.
 *
 *  \param [in] usp_handle      USP protocol handler.
 *  \return     Whether usp protocol disconnect successful.
 *                  TRUE:   success
 *                  FALSE:  fail
 *
 *  \details
 */
bool DisconnectUSP(usp_handle_t *usp_handle);


/**
 *  \brief Open usp application protocol.
 *
 *  \param [in] usp_handle      USP protocol handler.
 *  \param [in] protocol_type   to be communicate protocol type, \ref in USP_PROTOCOL_TYPE_E.
 *  \return     execution status \ref USP_PROTOCOL_STATUS
 *
 *  \details
 */
USP_PROTOCOL_STATUS OpenUSP(usp_handle_t *usp_handle);


/**
 *  \brief Set usp protocol state.
 *
 *  \param [in] usp_handle  USP protocol handler.
 *  \param [in] state       USP protocol state, \ref USP_PROTOCOL_STATUS.
 *  \return None
 *
 *  \details
 */
void SetUSPProtocolState(usp_handle_t *usp_handle, USP_PROTOCOL_STATUS state);


/**
 *  \brief USP protocol data packet inquiry.
 *
 *  \param [in] usp_handle  USP protocol handler.
 *  \param [in] payload     USP protocol payload buffer pointer.
 *  \param [in] payload_len USP protocol payload length, byte unit.
 *  \param [in] recv_head   pre received packet head pointer, only type received.
 *  \return successfully received data bytes from peer device.
 *              0   : USP fundamental command receive and handle corectly
 *              < 0 : Error Hanppend, \ref in USP_PROTOCOL_STATUS
 *              > 0 : Successfully received USP protocol payload size.
 *
 *  \details when recv_head is NOT NULL, means received 1byte in the recv_head.
 */
int USP_Protocol_Inquiry(usp_handle_t *usp_handle, u8_t *payload, u32_t payload_len, usp_packet_head_t *recv_head);


/**
 *  \brief send user payload to peer USP device, maybe is predefined command(no payload).
 *
 *  \param [in] usp_handle  USP protocol handler.
 *  \param [in] user_data   To be send data buffer pointer.
 *  \param [in] size        To be send data number, byte unit.
 *  \return data bytes of successfully send to peer device.
 *
 *  \details
 */
int WriteUSPData(usp_handle_t *usp_handle, u8_t *user_data, u32_t size);


/**
 *  \brief send user data to peer USP device, no ACK from peer device.
 *
 *  \param [in] usp_handle  USP protocol handler.
 *  \param [out]user_data   To be send data buffer pointer.
 *  \param [in] size        To be send data number, byte unit.
 *  \return data bytes of successfully send to peer device.
 *
 *  \details
 */
int WriteUSPISOData(usp_handle_t *usp_handle, u8_t *user_data, u32_t size);


/**
 *  \brief receive 1 payload packet from peer USP device, maybe is predefined command(no payload).
 *
 *  \param [in] usp_handle  USP protocol handler.
 *  \param [in] recv_head   pre received packet head pointer.
 *  \param [in] payload     To be received payload buffer pointer.
 *  \param [in] size        To be received payload number, byte unit, payload size MUST be small than USP_PAYLOAD_MAX_SIZE.
 *  \return if successfully received payload bytes from peer device.
 *              - value >= 0: has been successful received payload bytes
 *              - value < 0: error occurred, -value is \ref USP_PROTOCOL_STATUS
 *
 *  \details
 */
int ReceiveUSPDataPacket(usp_handle_t *usp_handle, usp_packet_head_t *recv_head, u8_t *payload, u32_t size);


/**
 *  \brief receive user data from peer USP device, maybe is predefined command(no payload).
 *
 *  \param [in] usp_handle  USP protocol handler.
 *  \param [in] user_data   To be received data buffer pointer.
 *  \param [in] size        To be received data number, byte unit.
 *  \return successfully received data bytes from peer device.
 *
 *  \details
 */
int ReadUSPData(usp_handle_t *usp_handle, u8_t *user_data, u32_t size);


/**
 *  \brief Send inquiry comamnd to peer device, check if it is still online.
 *
 *  \param [in] usp_handle  USP protocol handler.
 *  \return Peer device status, \ref USP_PROTOCOL_STATUS.
 *
 *  \details
 */
USP_PROTOCOL_STATUS InquiryUSP(usp_handle_t *usp_handle);


/**
 *  \brief Change communication baudrate.
 *
 *  \param [in] usp_handle  USP protocol handler.
 *  \param [in] baudrate    New baudrate for communication.
 *  \return     Whether baudrate config successful.
 *                  TRUE:   success
 *                  FALSE:  fail
 *  \details
 */
bool SetUSPBaudrate(usp_handle_t *usp_handle, u32_t baudrate);
#endif /*_ASSEMBLER_*/
#endif  /* __USP_PROTOCOL_H__ */

