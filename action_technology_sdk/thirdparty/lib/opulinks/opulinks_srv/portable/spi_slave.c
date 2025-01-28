/******************************************************************************
*  Copyright 2017 - 2021, Opulinks Technology Ltd.
*  ----------------------------------------------------------------------------
*  Statement:
*  ----------
*  This software is protected by Copyright and the information contained
*  herein is confidential. The software may not be copied and the information
*  contained herein may not be used or disclosed except with the written
*  permission of Opulinks Technology Ltd. (C) 2021
******************************************************************************/

/******************************************************************************
*  Filename:
*  ---------
*  example.c
*
*  Project:
*  --------
*  OPL1000 Project - the example implement file
*
*  Description:
*  ------------
*  This implement file is include the main patch function and api.
*
*  Author:
*  -------
*  TW FW
*
******************************************************************************/
/***********************
Head Block of The File
***********************/
// Sec 0: Comment block of the file


// Sec 1: Include File
#include <string.h>
#include <stdlib.h>
#include <kernel.h>
#include <drivers/spi.h>
#include "wifi.h"
#include "spi_slave.h"
#include "gpio.h"
#include "serial.h"
// Sec 2: Constant Definitions, Imported Symbols, miscellaneous

#define SPI_IDX                     SPI_IDX_2
#define SPI_SLAVE                   SPI_SLAVE_0
#define SPI_TX_DMA_CHANNEL          DMA_Channel_0
#define SPI_RX_DMA_CHANNEL          DMA_Channel_1

#define MIO                         22 //25
#define SIO                         23 //24

#define DMA_TIMEOUT_TICKS           (1 * SystemCoreClockGet())  // 1 sec

#define SEM_LOCK(id, time)          (k_sem_take(id, time))
#define SEM_UNLOCK(id)              (k_sem_give(id))

// #define MAX(a,b)                    (((a)>(b))?(a):(b))

#define TOGGLE_IDX(idx)             ((idx == 0) ? 1 : 0)

#ifdef DEBUG
#define DEBUG_LOG
#endif

#ifdef DEBUG_LOG
#define DBG_LOG(fmt, arg...)        printk(fmt, ##arg)
#else
#define DBG_LOG(x, ...)
#endif // DEBUG_LOG

/********************************************
Declaration of data structure
********************************************/
// Sec 3: structure, union, enum, linked list

/********************************************
Declaration of Global Variables & Functions
********************************************/
// Sec 4: declaration of global variable
struct k_work spi_slave_work;
struct k_work_q spi_slave_q;
struct k_sem g_tSemaphoreIdSpiDone;

#define CONFIG_SPI_WORK_Q_STACK_SIZE 1280
#define CONFIG_SPI_WORK_Q_PRIORITY  2

K_THREAD_STACK_DEFINE(spi_slave_q_stack, CONFIG_SPI_WORK_Q_STACK_SIZE);

const struct device *spi_dev = NULL;

struct spi_config spi_cfg = {
	.frequency = 13000000,
	.operation = SPI_OP_MODE_SLAVE | SPI_WORD_SET(8) | SPI_LINES_SINGLE,
	.slave = 0,
	.cs = NULL,
};

//===================================Buffer Interface===================================//
static T_Buffer g_tTxBuffer;
static T_Buffer g_tRxBuffer;
static BI_RecvDataCallbackPtr g_tRxCallback = NULL;
// Sec 5: declaration of global function prototype

/***************************************************
Declaration of static Global Variables & Functions
***************************************************/
// Sec 6: declaration of static global variable

// static T_SpiSlaveChStatus g_tSpiSlaveChStatus;

// Sec 7: declaration of static function prototype

//===================================Buffer Interface===================================//
T_RetCode BI_SetLen(T_Buffer *ptBuffer, uint16_t u16PayloadLen);
T_RetCode BI_SetData(T_Buffer *ptBuffer, uint8_t *pu8Payload, uint16_t u16PayloadLen);
T_RetCode BI_ClearData(T_Buffer *ptBuffer);
uint8_t *BI_GetWrDataPtr(T_Buffer *ptBuffer);
uint8_t *BI_GetRdDataPtr(T_Buffer *ptBuffer);
void BI_UpdWrDataIdx(T_Buffer *ptBuffer);
void BI_UpdRdDataIdx(T_Buffer *ptBuffer);
void BI_GetLen(T_Buffer *ptBuffer, uint16_t *pu16CurrLen, uint16_t *pu16NextLen);
bool BI_IsFull(T_Buffer *ptBuffer);
bool BI_IsEmpty(T_Buffer *ptBuffer);

/***********
C Functions
***********/
// Sec 8: C Functions

//===================================SPI Host Protocol===================================//
//========================================BEGIN==========================================//
void SPI_SlaveDriverSIOSyncPulseTrigger(void)
{
    Hal_Gpio_Output(SIO, 1);
}

uint8_t SPI_SlaveDriverSpiInit(void)
{
	spi_dev = device_get_binding("SPI_2");
    
	if (!spi_dev) {
		DBG_LOG("spi2 slave device not found!\n");
		return 0;
	} else {
		DBG_LOG("spi2 device has been found!\n");
	}

    return 1;
}

extern bool spi_slave_transceive(uint8_t *pu8TxBuf, uint16_t u16TxLen, uint8_t *pu8RxBuf, uint16_t u16RxLen, void *callback);

static void SPI_SlaveDriverDmaCallback(uint8_t u8State)
{
	if (u8State == 1){
		 // dma ready
		 Hal_Gpio_Output(SIO, 0);
	} else if(u8State == 2) {
		 // dma done
    	SEM_UNLOCK(&g_tSemaphoreIdSpiDone);
	}
}

bool SPI_SlaveDriverTrxConfigure(uint8_t *pu8TxBuf, uint16_t u16TxLen, uint8_t *pu8RxBuf, uint16_t u16RxLen)
{
    return spi_slave_transceive(pu8TxBuf, u16TxLen, pu8RxBuf, u16RxLen, &SPI_SlaveDriverDmaCallback);
}

void SPI_SlaveDataRequestStart(void)
{
    DBG_LOG("START\n");
}

static void SPI_SlaveThread(struct k_work *work)
{
    // initiate SPI DMA driver
    if(SPI_SlaveDriverSpiInit() == 0)
    {
        return;
    }

    uint8_t *pu8TxDataPtr = BI_SendDataGetPtr();
    uint8_t *pu8RxDataPtr = BI_RecvDataGetPtr();

    uint16_t u16DataLen = sizeof(T_Payload);

    for(;;)
    {
        if(!SPI_SlaveDriverTrxConfigure(pu8TxDataPtr, u16DataLen, pu8RxDataPtr, u16DataLen))
        {
            DBG_LOG("DMA config fail\n");
            return;
        }

        SPI_SlaveDriverSIOSyncPulseTrigger();

        // Wait for tx and rx done
        if(SEM_LOCK(&g_tSemaphoreIdSpiDone, K_MSEC(5)) != 0) {
			SYS_LOG_INF("SPI DMA time out\n");
			SEM_UNLOCK(&g_tSemaphoreIdSpiDone);
		}

        T_Header *ptHeader = (T_Header *)pu8RxDataPtr;
        
        BI_RecvDataLenSet(ptHeader->u16CurrLen);

        BI_RecvDataCallbackHandle();

    }
}

void SPI_SlaveInit(void)
{

    k_sem_init(&g_tSemaphoreIdSpiDone, 0, 1);

	k_work_queue_start(&spi_slave_q, spi_slave_q_stack, 
			           K_THREAD_STACK_SIZEOF(spi_slave_q_stack),
			           CONFIG_SPI_WORK_Q_PRIORITY, NULL);

	k_work_init(&spi_slave_work, SPI_SlaveThread);
	k_work_submit_to_queue(&spi_slave_q, &spi_slave_work);
}

/*********** local functions ***********/

T_RetCode BI_SetLen(T_Buffer *ptBuffer, uint16_t u16PayloadLen)
{
    uint16_t u16Index = ptBuffer->u16WriteIdx;

    if(0 == u16PayloadLen || BI_MAX_PAYLOAD_LEN < u16PayloadLen)
    {
        return BI_RET_FAIL_LEN_INVALID;
    }

    ptBuffer->tPackage[u16Index].u16PayloadLen = u16PayloadLen;

    return BI_RET_OK;
}

T_RetCode BI_SetData(T_Buffer *ptBuffer, uint8_t *pu8Payload, uint16_t u16PayloadLen)
{
    T_RetCode tRet = BI_RET_OK;
    uint16_t u16Index = ptBuffer->u16WriteIdx;

    if(NULL == pu8Payload)
    {
        tRet = BI_RET_FAIL_PARAM_INVALID;
        return tRet;
    }

    tRet = BI_SetLen(ptBuffer, u16PayloadLen);

    if(BI_RET_OK != tRet)
    {
        return tRet;
    }

    memcpy(ptBuffer->tPackage[u16Index].tPayload.u8aPayload, pu8Payload, u16PayloadLen);

    return BI_RET_OK;
}

T_RetCode BI_ClearData(T_Buffer *ptBuffer)
{
    uint16_t u16Index = ptBuffer->u16ReadIdx;

    ptBuffer->tPackage[u16Index].u16PayloadLen = 0;

    return BI_RET_OK;
}

uint8_t *BI_GetWrDataPtr(T_Buffer *ptBuffer)
{
    uint16_t u16Index = ptBuffer->u16WriteIdx;

    return (uint8_t *)&ptBuffer->tPackage[u16Index].tPayload;
}

uint8_t *BI_GetRdDataPtr(T_Buffer *ptBuffer)
{
    uint16_t u16Index = ptBuffer->u16ReadIdx;

    return (uint8_t *)&ptBuffer->tPackage[u16Index].tPayload;
}

void BI_UpdWrDataIdx(T_Buffer *ptBuffer)
{
    uint16_t u16Index = ptBuffer->u16WriteIdx;

    ptBuffer->u16WriteIdx = TOGGLE_IDX(u16Index);
}

void BI_UpdRdDataIdx(T_Buffer *ptBuffer)
{
    uint16_t u16Index = ptBuffer->u16ReadIdx;

    ptBuffer->u16ReadIdx = TOGGLE_IDX(u16Index);
}

void BI_GetLen(T_Buffer *ptBuffer, uint16_t *pu16CurrLen, uint16_t *pu16NextLen)
{
    uint16_t u16Index = ptBuffer->u16ReadIdx;

    if(NULL != pu16CurrLen)
    {
        *pu16CurrLen = ptBuffer->tPackage[u16Index].u16PayloadLen;
    }

    if(NULL != pu16NextLen)
    {
        u16Index = TOGGLE_IDX(u16Index);
        *pu16NextLen = ptBuffer->tPackage[u16Index].u16PayloadLen;
    }
}

bool BI_IsFull(T_Buffer *ptBuffer)
{
    if(ptBuffer->u16WriteIdx == ptBuffer->u16ReadIdx)
    {
#if 1
        if(ptBuffer->tPackage[0].u16PayloadLen != 0)
        {
            return true;
        }
#else
        if((ptBuffer->tPackage[0].u16PayloadLen != 0) &&
           (ptBuffer->tPackage[1].u16PayloadLen != 0))
        {
            return true;
        }
#endif
    }

    return false;
}

bool BI_IsEmpty(T_Buffer *ptBuffer)
{
    if(ptBuffer->u16WriteIdx == ptBuffer->u16ReadIdx)
    {
#if 1
        if(ptBuffer->tPackage[0].u16PayloadLen == 0)
        {
            return true;
        }
#else
        if((ptBuffer->tPackage[0].u16PayloadLen == 0) &&
           (ptBuffer->tPackage[1].u16PayloadLen == 0))
        {
            return true;
        }
#endif
    }

    return false;
}

/*********** global functions ***********/

void BI_Init(void)
{
    memset(&g_tTxBuffer, 0, sizeof(g_tTxBuffer));
    memset(&g_tRxBuffer, 0, sizeof(g_tRxBuffer));
}

T_RetCode BI_SendData(uint8_t *pu8Payload, uint16_t u16PayloadLen)
{
    T_RetCode tRet = BI_RET_OK;

    if(true == BI_IsFull(&g_tTxBuffer))
    {
        tRet = BI_RET_FAIL_BUF_FULL;
        return tRet;
    }

    tRet = BI_SetData(&g_tTxBuffer, pu8Payload, u16PayloadLen);

    if(tRet != BI_RET_OK)
    {
        return tRet;
    }

#ifdef BI_DEBUG
    BI_UpdWrDataIdx(&g_tTxBuffer);
#endif

    // trigger SPI protocol
#if (APP_IS_MASTER == 1) // MASTER
    SPI_MasterDataRequestStart();
#else
    SPI_SlaveDataRequestStart();
#endif

    return tRet;
}

uint8_t *BI_SendDataGetPtr(void)
{
    return BI_GetRdDataPtr(&g_tTxBuffer);
}

T_RetCode BI_SendDataComplete(void)
{
    T_RetCode tRet = BI_RET_OK;

    if(true == BI_IsEmpty(&g_tTxBuffer))
    {
        tRet = BI_RET_FAIL_BUF_EMPTY;
        return tRet;
    }

    BI_ClearData(&g_tTxBuffer);

#if 1
#else
    BI_UpdRdDataIdx(&g_tTxBuffer);
#endif

    return tRet;
}

void BI_SendDataLenGet(uint16_t *pu16CurrLen, uint16_t *pu16NextLen)
{
    BI_GetLen(&g_tTxBuffer, pu16CurrLen, pu16NextLen);
}

void BI_RecvDataCallbackReg(BI_RecvDataCallbackPtr ptCallback)
{
    g_tRxCallback = ptCallback;
}

void BI_RecvDataCallbackHandle(void)
{
    if(true == BI_IsEmpty(&g_tRxBuffer))
    {
        return;
    }

    T_Payload *ptPayload = NULL;
    uint16_t u16PayloadLen = 0;

    ptPayload = (T_Payload *)BI_GetRdDataPtr(&g_tRxBuffer);
    BI_GetLen(&g_tRxBuffer, &u16PayloadLen, NULL);

    if(NULL != g_tRxCallback)
    {
        g_tRxCallback((uint8_t *)ptPayload->u8aPayload, u16PayloadLen);
    }

    BI_ClearData(&g_tRxBuffer);

#if 1
#else
    BI_UpdRdDataIdx(&g_tRxBuffer);
#endif
}

uint8_t *BI_RecvDataGetPtr(void)
{
    if(true == BI_IsFull(&g_tRxBuffer))
    {
        // TODO: Unconfirmed sequence? need to discuss...
        // currently will overwrite the previous payload,
        // and directly make the Read Index move to the next position.
        BI_ClearData(&g_tRxBuffer);

        BI_UpdRdDataIdx(&g_tRxBuffer);
    }

    return BI_GetWrDataPtr(&g_tRxBuffer);
}

void BI_RecvDataLenSet(uint16_t u16PayloadLen)
{
    BI_SetLen(&g_tRxBuffer, u16PayloadLen);
}

T_RetCode BI_RecvDataComplete(void)
{
    T_RetCode tRet = BI_RET_OK;


#ifdef BI_DEBUG
    BI_UpdWrDataIdx(&g_tRxBuffer);
#endif

    return tRet;
}

#ifdef BI_DEBUG
void BI_RawDump(void)
{
    DBG_LOG("-----Buffer Interface-----\n");
    DBG_LOG("[TX] WrIdx %d, RdIdx %d\n", g_tTxBuffer.u16WriteIdx, g_tTxBuffer.u16ReadIdx);
    
    DBG_LOG("[TX] Pkg[0]: %d, ", g_tTxBuffer.tPackage[0].u16PayloadLen);
    for(uint8_t i = 0; i < 20; i++)
    {
        DBG_LOG("%x ", g_tTxBuffer.tPackage[0].tPayload.u8aPayload[i]);
    }
    DBG_LOG("\n");

    DBG_LOG("[TX] Pkg[1]: %d, ", g_tTxBuffer.tPackage[1].u16PayloadLen);
    for(uint8_t i = 0; i < 20; i++)
    {
        DBG_LOG("%x ", g_tTxBuffer.tPackage[1].tPayload.u8aPayload[i]);
    }
    DBG_LOG("\n");

    DBG_LOG("[RX] WrIdx %d, RdIdx %d\n", g_tRxBuffer.u16WriteIdx, g_tRxBuffer.u16ReadIdx);
    
    DBG_LOG("[RX] Pkg[0]: %d, ", g_tRxBuffer.tPackage[0].u16PayloadLen);
    for(uint8_t i = 0; i < 20; i++)
    {
        DBG_LOG("%x ", g_tRxBuffer.tPackage[0].tPayload.u8aPayload[i]);
    }
    DBG_LOG("\n");

    DBG_LOG("[RX] Pkg[1]: %d, ", g_tRxBuffer.tPackage[1].u16PayloadLen);
    for(uint8_t i = 0; i < 20; i++)
    {
        DBG_LOG("%x ", g_tRxBuffer.tPackage[1].tPayload.u8aPayload[i]);
    }
    DBG_LOG("\n");
    DBG_LOG("-----Buffer Interface-----\n\n");
}
#endif


int spi_slave_init(BI_RecvDataCallbackPtr data_indication_callback)
{
    printk("Slave SPI Init\n");
    BI_Init();
    BI_RecvDataCallbackReg(data_indication_callback);
	SPI_SlaveInit();
	return 0;
}
