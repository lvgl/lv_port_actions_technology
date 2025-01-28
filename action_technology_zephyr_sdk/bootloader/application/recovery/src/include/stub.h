/**
 *  ***************************************************************************
 *  Copyright (c) 2003-2017 Actions Semiconductor. All rights reserved.
 *
 *  \file       stub.h
 *  \brief      stub structure definition
 *  \author     zhouxl
 *  \version    1.00
 *  \date       2017-11-23
 *  ***************************************************************************
 *
 *  \History:
 *  Version 1.00
 *       Initial release
 */

#ifndef __STUB_H__
#define __STUB_H__

#include <zephyr/types.h>
#include "stub_command.h"
#include "usp_protocol.h"


//pc工具的类型
typedef enum
{
    STUB_PC_TOOL_ASQT_MODE      = 1,
    STUB_PC_TOOL_ASET_MODE      = 2,
    STUB_PC_TOOL_ASET_EQ_MODE   = 3,
    STUB_PC_TOOL_ATT_MODE       = 4,
    STUB_PC_TOOL_TK_PMODE       = 5,
    STUB_PC_TOOL_PRINT_MODE     = 6,
    STUB_PC_TOOL_WAVES_ASET_MODE= 7,
    STUB_PC_TOOL_ADFU_MODE      = 8,
    STUB_PC_TOOL_BTT_MODE       = 0x42,
} STUB_CONNECTION_MODE;


/**
\brief  STUB Status of executed operation
*/
typedef enum __STUB_PROTOCOL_STATUS
{
    STUB_PROTOCOL_OK =  0,                  ///< Operation succeeded
    STUB_PROTOCOL_BUSY,                     ///< STUB comunication is BUSY
    STUB_PROTOCOL_ERROR,                    ///< Unspecified error
    STUB_PROTOCOL_SEND_ERROR,               ///< STUB upload data error happend
    STUB_PROTOCOL_RECEIVE_ERROR,            ///< STUB download data error happend
    STUB_PROTOCOL_TIMEOUT,                  ///< STUB transaction timeout
    STUB_PROTOCOL_COMMAND_NOT_SUPPORT,      ///< Not support STUB command
    STUB_PROTOCOL_PARA_NOT_SUPPORT,         ///< Not support STUB parameter in command
    STUB_PROTOCOL_DISCONNECT,               ///< STUB disconnect
    STUB_PROTOCOL_END,                      ///< STUB transaction finish
}STUB_PROTOCOL_STATUS;


typedef enum
{
    STUB_HEAD_CMD_MAGIC     = 0xAE,
    STUB_HEAD_RSP_MAGIC     = 0xAC,
}STUB_PACKET_HEAD_MAGIC_E;


typedef struct
{
    u8_t  magic;               ///< stub magic: 0xAE
    u8_t  stub_id;             ///< stub connection identification: \ref STUB_CONNECTION_MODE.
    u8_t  opcode;              ///< operation code
    u8_t  reserved;
    u32_t para;
    u32_t payload_length;      ///< STUB data packet length, byte unit
}stub_cmd_packet_t;

#define STUB_CMD_PACKET_LEN     (sizeof(stub_cmd_packet_t))


typedef struct
{
    u8_t  magic;               ///< stub magic: 0xAC
    u8_t  reserved[2];
    u8_t  status;              ///< stub response status, \ref in STUB_PROTOCOL_STATUS
}stub_response_packet_t;


/**
 *  \brief Initial stub protocol.
 *
 *  \param [in] stub_op     stub operation handler.
 *
 *  \details
 */
void InitStub(usp_handle_t *stub_op);


/**
 *  \brief Exit stub protocol.
 *
 *  \param [in] stub_op     stub operation handler.
 *
 *  \details
 */
void ExitStub(usp_handle_t *stub_op);


/**
 *  \brief Open the stub connection, initialize stub driver, check if the stub protocol connection is ready.
 *
 *  \param [in] timeout_ms      Maximun wait time, millisecond unit.
 *  \return If the stub connection is OK.
 *              - 0         Be prepared for stub protocol transaction.
 *              - -1        the stub connection is NOT ready.
 *              - othres    the stub connection is ready, but NOT stub tools.
 *
 *  \details
 */
int OpenStub(u32_t timeout_ms);


/**
 *  \brief Close the stub connection.
 *
 *  \return If close stub success, \ref in STUB_PROTOCOL_STATUS.
 *
 *  \details None.
 */
int CloseStub(void);


/**
 *  \brief      Stub command packet handle.
 *
 *  \return     execution status \ref STUB_PROTOCOL_STATUS
 *
 *  \details    More details
 */
STUB_PROTOCOL_STATUS StubMessageLoop(void);


/**
 *  \brief  Send a command packet to peer stub device, and read data from it.
 *
 *  \param [in] opcode      stub opcode in the command packet.
 *  \param [in] op_para     stub operation parameter.
 *  \param [in] data_buffer data buffer address to save the read data.
 *  \param [in] data_len    read data length, byte unit.
 *  \return Data length has been successfully receive.
 *              - >=0   Successfully received data length.
 *              - <0    Error happend, \ref in STUB_PROTOCOL_STATUS.
 *
 *  \details
 */
int stub_read_packet(u16_t opcode, u32_t op_para, u8_t *data_buffer, u32_t data_len);


/**
 *  \brief Send a command packet and a data packet to peer stub device, then receive response from peer device.
 *
 *  \param [in] opcode      stub opcode.
 *  \param [in] op_para     stub operation parameter.
 *  \param [in] data_buffer data buffer address to be send.
 *  \param [in] data_len    send data length, byte unit.
 *  \return Data length has been successfully send.
 *              - 0         Successfully send data length.
 *              - others    Error happend, \ref in STUB_PROTOCOL_STATUS.
 *
 *  \details
 */
int stub_write_packet(u16_t opcode, u32_t op_para, u8_t *data_buffer, u32_t data_len);


/**
 *  \brief Send a data packet(with check and ACK) to peer stub device.
 *
 *  \param [in] data_buffer data buffer address to be send.
 *  \param [in] data_len    send data length, byte unit.
 *  \return If the data send is success.
 *              - 0     YES.
 *              - -1    NO, data write timeout, or stub device disconnect.
 *
 *  \details
 */
int stub_send_data(u8_t *data_buffer, u32_t data_len);


/**
 *  \brief Send a isochronous data packet(without check and NO ACK) to peer stub device.
 *
 *  \param [in] data_buffer data buffer address to be send.
 *  \param [in] data_len    send data length, byte unit.
 *  \return If the data send is success.
 *              - 0                             YES.
 *              - STUB_PROTOCOL_DISCONNECT      NO, device is disconnected.
 *
 *  \details
 */
int stub_send_iso_data(u8_t *data_buffer, u32_t data_len);


/**
 *  \brief Receive a data packet(with check and ACK) from peer stub device.
 *
 *  \param [in] data_buffer data buffer address to save the read data.
 *  \param [in] data_len    send data length, byte unit.
 *  \return If the data send is success.
 *              > 0                             Received data length, byte unit.
 *              - STUB_PROTOCOL_RECEIVE_ERROR   Error hanppend.
 *
 *  \details    Only data packet, and data is checked.
 */
int stub_receive_data(u8_t *data_buffer, u32_t data_len);


/**
 *  \brief Send a response packet to peer stub device.
 *
 *  \param [in] status      stub protocol status code, \ref in STUB_PROTOCOL_STATUS.
 *  \return If the data send is success.
 *              - 0     YES.
 *              - -1    NO, data write timeout, or stub device disconnect.
 *
 *  \details
 */
int stub_send_response(u32_t status);


/**
 *  \brief Receive a response packet to peer stub device.
 *
 *  \return Stub response status, \ref in STUB_PROTOCOL_STATUS.
 *
 *  \details
 */
int stub_receive_response(void);


/**
 *  \brief Inquiry stub protocol communication status.
 *
 *  \return Stub protocol state, \ref in STUB_PROTOCOL_STATUS
 *
 *  \details More details
 */
int stub_status_inquiry(void);


/**
 *  \brief Change stub communication baudrate.
 *
 *  \param [in] baudrate    New baudrate for communication.
 *  \return     Whether baudrate config successful.
 *                  TRUE:   success
 *                  FALSE:  fail
 *  \details
 */
bool stub_speed_set(u32_t baudrate);
#endif


