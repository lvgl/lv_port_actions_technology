/*
 * Copyright (c) 2016 Open-RnD Sp. z o.o.
 * Copyright (c) 2016 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file SoC configuration macros for the LARK family processors.
 *
 */

#ifndef _LEOPARD_SOC_H_
#define _LEOPARD_SOC_H_

#include <sys/util.h>

#ifndef _ASMLANGUAGE

#include <soc_regs.h>

/* Add include for DTS generated information */
#include <devicetree.h>

#include "soc_clock.h"
#include "soc_reset.h"
#include "soc_irq.h"
#include "soc_gpio.h"
#include "soc_pinmux.h"
#include "soc_pmu.h"
#include "soc_pm.h"
#include "soc_sleep.h"
#include "soc_timer.h"
#include "soc_boot.h"
#include "soc_se.h"
#include "soc_memctrl.h"
#include "soc_ppi.h"
#include "soc_pstore.h"


void jtag_set(void);
int check_adfu_connect(unsigned int gpio_in, unsigned int gpio_out);
int check_adfu_gpiokey(unsigned int gpio);
int soc_dvfs_opt(void);

void soc_watchdog_clear(void);


#define __FPU_PRESENT			1
#define __MPU_PRESENT           1
#define CHIP_LEOPARD            1

#endif /* !_ASMLANGUAGE */

#endif /* _LARK_SOC_H_ */
