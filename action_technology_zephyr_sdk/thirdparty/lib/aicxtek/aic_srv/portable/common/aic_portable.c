/******************************************************************************/
/*                                                                            */
/*    Copyright 2023 by AICXTEK TECHNOLOGIES CO.,LTD. All rights reserved.    */
/*                                                                            */
/******************************************************************************/

#include <aic_init.h>
#include <aic_portable.h>
#include <aic_type.h>
#include <aic_ctrl.h>
#include <init.h>
#include <dvfs.h>

extern void aic_gpio_init(void);

void aic_srv_init(void)
{
    dvfs_set_level(DVFS_LEVEL_HIGH_PERFORMANCE, "aic_srv");
    aic_gpio_init();
    aic_init();
    aic_ctrl_poweron();

    printk("aic init sucess!\n");
}
