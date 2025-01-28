/******************************************************************************
*  Copyright 2024, Opulinks Technology Ltd.
*  ----------------------------------------------------------------------------
*  Statement:
*  ----------
*  This software is protected by Copyright and the information contained
*  herein is confidential. The software may not be copied and the information
*  contained herein may not be used or disclosed except with the written
*  permission of Opulinks Technology Ltd. (C) 2024
******************************************************************************/

/******************************************************************************
*  Filename:
*  ---------
*  spi_slave.h
*
*  Project:
*  --------
*  
*
*  Description:
*  ------------
*  
*
*  Author:
*  -------
*  AE Team
*
******************************************************************************/
/***********************
Head Block of The File
***********************/
// Sec 0: Comment block of the file

// Sec 1: Include File

// Sec 2: Constant Definitions, Imported Symbols, miscellaneous

#ifndef _SPI_SLAVE_H_
#define _SPI_SLAVE_H_

#include <stdint.h>
#include <stdbool.h>
#include <wifi.h>
#ifdef __cplusplus
extern "C" {
#endif

// SpiMaster Thread Configuration
#define SPI_SLAVE_THREAD_PRIORITY           (osPriorityNormal)
#define SPI_SLAVE_THREAD_STACK_SIZE         (512)
#define SPI_SLAVE_QUEUE_SIZE                (8)

/* SPI_BAUDRATE the bit rate in SPI_CLK pin
 *
 * Note: 
 *     OPL2500 SPI slave hardware SPI rate limitation
 *       RX-only mode: SPI pclk > 12 * SPI_BAUDRATE
 *       TRX mode: SPI pclk > 6 * SPI_BAUDRATE
 *
 *
 * Note:
 *     OPL2500 DMA is using system clock, please udpate system clock to higher than SPI data rate.
 *     But if the system clock is too high, maybe over 100MHz, it needs to set wait state for SRAM or it will get error when read or writing SRAM.
 *     Call Hal_Sys_SramDffBypass(0) to set wait state, i.e. disable SRAM DFF bypass.
 */
#define SPI_BAUDRATE            13000000    /* TRX mode, slave minimum spi bit rate = 160M/12 */
#define SPI_RX_SAMPLE_DELAY     2           /* Related to physical wire length */
#define SPI_MASTER_RX_ENABLE    1           /* Set to zero when slave is RX-only mode */
#define EN_DATA_CHECKING        (1 & SPI_MASTER_RX_ENABLE)
#define CALC_BLK_CNT            1000       /* To calculate throughput block counts */

#define SPI_PERFORMANCE_TEST    1

#define DEBUG

//===================================Buffer Interface===================================//
#define BI_MAX_PAYLOAD_LEN              (2048)
#define BI_MIN_PAYLOAD_LEN              (16)
#define BI_HEADER_TAG                   (0x000A)
#define BI_DEBUG

/********************************************
Declaration of data structure
********************************************/
// Sec 3: structure, uniou, enum, linked list

//===================================SPI Host Protocol===================================//
typedef enum E_SpiSlaveStatusType
{
    ST_CH_IDLE = 0,         // idle state
    ST_CH_SYNC,             // sync-packet state
    ST_CH_SYNC_WAIT_ACK,    // sync-packet wait ack state
    ST_CH_PROGRESS,         // progress state
    ST_CH_WAIT_ACK,         // wait ack state
} T_SpiSlaveStatusType;

typedef enum E_SpiSlaveActivateType
{
    ST_CH_DEACTIVE = 0,     // de-activate
    ST_CH_ACTIVE,           // activate
} T_SpiSlaveActivateType;

typedef struct S_SpiSlaveChStatus
{
    T_SpiSlaveStatusType u8EventSt;      // status of receive channel
    T_SpiSlaveStatusType u8RequestSt;    // status of transmit channel
    T_SpiSlaveActivateType u8EventAct;     // activate / de-activate of receive channel 
    T_SpiSlaveActivateType u8RequestAct;   // activate / de-activate of transmit channel 
} T_SpiSlaveChStatus;

//===================================Buffer Interface===================================//
typedef int (* BI_RecvDataCallbackPtr)(uint8_t *pu8Payload, uint16_t u16PayloadLen);

typedef enum E_RetCode
{
    BI_RET_OK = 0,
    BI_RET_FAIL_PARAM_INVALID,
    BI_RET_FAIL_LEN_INVALID,
    BI_RET_FAIL_BUF_FULL,
    BI_RET_FAIL_BUF_EMPTY,
} T_RetCode;

typedef struct S_Header
{
    uint16_t u16Header;
    uint16_t u16Crc;
    uint8_t  u8ReqAck:1;
    uint8_t  u8RspAck:1;
    uint8_t  u8AckRst:1;
    uint8_t  u8Rsvd  :5;
    uint8_t  u8Mode;
    uint8_t  u8AckSeqNum;
    uint8_t  u8SeqNum;
    uint16_t u16NextLen;
    uint16_t u16CurrLen;
} T_Header;

typedef struct S_Payload
{
    T_Header tHeader;
    uint8_t  u8aPayload[BI_MAX_PAYLOAD_LEN];
} T_Payload;

typedef struct S_Package
{
    T_Payload tPayload;
    uint16_t  u16PayloadLen;
} T_Package;

typedef struct S_Buffer
{
    uint16_t u16ReadIdx;
    uint16_t u16WriteIdx;
    T_Package tPackage[2];
} T_Buffer;

/********************************************
Declaration of Global Variables & Functions
********************************************/
// Sec 4: declaration of global variable

// Sec 5: declaration of global function prototype

//===================================SPI Host Protocol===================================//
void SPI_SlaveInit(void);
int spi_slave_init(BI_RecvDataCallbackPtr data_indication_callback);
//===================================Buffer Interface===================================//
void BI_Init(void);
T_RetCode BI_SendData(uint8_t *pu8Payload, uint16_t u16PayloadLen);
uint8_t *BI_SendDataGetPtr(void);
T_RetCode BI_SendDataComplete(void);
void BI_SendDataLenGet(uint16_t *pu16CurrLen, uint16_t *pu16NextLen);
void BI_RecvDataCallbackReg(BI_RecvDataCallbackPtr ptCallback);
void BI_RecvDataCallbackHandle(void);
uint8_t *BI_RecvDataGetPtr(void);
void BI_RecvDataLenSet(uint16_t u16PayloadLen);
T_RetCode BI_RecvDataComplete(void);

#ifdef BI_DEBUG
void BI_RawDump(void);
#endif

/***************************************************
Declaration of static Global Variables & Functions
***************************************************/
// Sec 6: declaration of static global variable

// Sec 7: declaration of static function prototype

/***********
C Functions
***********/
// Sec 8: C Functions

#ifdef __cplusplus
}
#endif
#endif  /* _SPI_SLAVE_H_ */
