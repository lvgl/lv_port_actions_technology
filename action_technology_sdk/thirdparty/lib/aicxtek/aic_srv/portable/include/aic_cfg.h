/******************************************************************************/
/*                                                                            */
/*    Copyright 2021 by AICXTEK TECHNOLOGIES CO.,LTD. All rights reserved.    */
/*                                                                            */
/******************************************************************************/

/**
 *  DESCRIPTION
 *
 *    This file defines AIC config header file.
 */

#ifndef __AIC_CFG_H__
#define __AIC_CFG_H__

/* Ensure that standard C is used to process the API information. */
#ifdef __cplusplus
extern "C" {
#endif

#include <aic_type.h>

#define INVALID_GPIO    0xFFFF

typedef struct
{
    unsigned short gpio_num;
    unsigned short is_active_low;
} gpio_cfg_t;

typedef struct
{
    unsigned short dbg;
} aic_ctrl_cfg_t;

typedef struct
{
    unsigned short  tx_pin;
    unsigned short  rx_pin;
    unsigned int    baud;
} aic_uart_cfg_t;

typedef struct
{
    gpio_cfg_t      mcu_wakeup_modem;
    gpio_cfg_t      modem_wakeup_mcu;
} aic_gpio_wakeup_cfg_t;


/**
 * aic_ctrl_is_dbg
 */
bool aic_ctrl_is_dbg(void);


/*
 * Description: aicxtek get the uart config.
 * Arguments  : None.
 * Return     : return the pointer of the aic uart config.
 */
aic_uart_cfg_t *aic_get_uart_cfg(void);

/*
 * Description: aicxtek get the gpio wakeup config.
 * Arguments  : None.
 * Return     : return the pointer of the aic gpio wakeup config.
 */
aic_gpio_wakeup_cfg_t *aic_gpio_wakeup_cfg(void);

/* Ensure that standard C is used to process the API information. */
#ifdef __cplusplus
}
#endif

#endif/* __AIC_CFG_H__ */

