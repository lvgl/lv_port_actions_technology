/**************************************************************************/
/*                                                                        */
/* Copyright 2023 by AICXTEK TECHNOLOGIES CO.,LTD. All rights reserved.   */
/*                                                                        */
/**************************************************************************/

/**
 *  DESCRIPTION
 *
 *    This file aic ctrl module adpter source file.
 */
#include <aic_ctrl_drv.h>
#include <aic_gpio.h>

#define PIN_VALUE_LOW           0
#define PIN_VALUE_HIGH          1

#define PWR_PIN_NUM             60

void aic_ctrl_active_power_pin(bool active)
{
    if (active)
        aic_gpio_set_outvalue(PWR_PIN_NUM, PIN_VALUE_LOW);
    else
        aic_gpio_set_outvalue(PWR_PIN_NUM, PIN_VALUE_HIGH);
}

