/**
 *  ***************************************************************************
 *  Copyright (c) 2003-2017 Actions Semiconductor. All rights reserved.
 *
 *  \file       aset.h
 *  \brief      aset structure definition
 *  \author     zhouxl
 *  \version    1.00
 *  \date       2017-11-23
 *  ***************************************************************************
 *
 *  \History:
 *  Version 1.00
 *       Initial release
 */

#ifndef __ASET_H__
#define __ASET_H__

#include "usp_protocol.h"


/**
\brief  ASET Status of executed operation
*/
typedef enum __ASET_STATUS
{
    ASET_OK =  0,               ///< Operation succeeded
    ASET_BUSY,                  ///< ASET comunication is BUSY
    ASET_ERROR,                 ///< Unspecified error
    ASET_SEND_ERROR,            ///< ASET upload data error happend
    ASET_RECEIVE_ERROR,         ///< ASET download data error happend
    ASET_TIMEOUT,               ///< ASET transaction timeout
    ASET_COMMAND_NOT_SUPPORT,   ///< Not support ASET command
    ASET_PARA_NOT_SUPPORT,      ///< Not support ASET parameter in command
    ASET_DISCONNECT,            ///< ASET disconnect
    ASET_END,                   ///< ASET transaction finish
}ASET_STATUS;

typedef struct
{
    uint8_t  magic;                ///< ASET magic: 0xE0
    uint8_t  reserved0;
    uint8_t  opcode;               ///< ASET operation code
    uint8_t  reserved1;
    uint32_t reserved2;
    uint32_t para_length;          ///< ASET parameter length, byte unit
}aset_cmd_packet_t;

#define ASET_CMD_PACKET_LEN     (sizeof(aset_cmd_packet_t))

/**
 *  \brief Initial aset protocol.
 *
 *  \param [in] aset_op     aset operation handler.
 *
 *  \details
 */
void InitASET(usp_handle_t *aset_op);


/**
 *  \brief Exit aset protocol.
 *
 *  \param [in] aset_op     aset operation handler.
 *
 *  \details
 */
void ExitASET(usp_handle_t *aset_op);


/**
 *  \brief ASET protocol RX finite state machine
 *
 *  \param [in] aset_op     aset operation handler.
 *  \return FSM state, \ref in ASET_STATUS
 *
 *  \details
 */
ASET_STATUS ASET_Protocol_Rx_Fsm(usp_handle_t *aset_op);


/**
 *  \brief Read ASET 1 packet payload from peer device, head has been received already.
 *
 *  \param [in] aset_op     aset operation handler.
 *  \param [in] payload     To be received payload buffer pointer.
 *  \param [in] size        To be received payload number, byte unit, payload size MUST be small than USP_PAYLOAD_MAX_SIZE.
 *  \return if successfully received payload bytes from peer device.
 *              - value >= 0: has been successful received payload bytes
 *              - value < 0: error occurred, -value is \ref USP_PROTOCOL_STATUS
 *
 *  \details
 */
int ReadASETPacket(usp_handle_t *aset_op, uint8_t* payload, uint32_t size);
#endif


