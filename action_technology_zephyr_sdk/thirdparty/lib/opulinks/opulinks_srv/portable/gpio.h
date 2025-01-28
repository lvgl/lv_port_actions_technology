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

#ifndef _GPIO_PORTING_H_
#define _GPIO_PORTING_H_

#include <stdint.h>
#include <stdbool.h>
#include <drivers/gpio.h>

#ifdef __cplusplus
extern "C" {
#endif

int Hal_Gpio_Output(uint32_t gpio_num, uint32_t gpio_level);
int Hal_Gpio_Level_Get(uint32_t gpio_num);
int Hal_Gpio_IntInit(uint32_t gpio_num, uint32_t trig_flag, gpio_callback_handler_t callback);
int Hal_Gpio_IntDeInit(uint32_t gpio_num);

#ifdef __cplusplus
}
#endif
#endif  /* _SPI_SLAVE_H_ */
