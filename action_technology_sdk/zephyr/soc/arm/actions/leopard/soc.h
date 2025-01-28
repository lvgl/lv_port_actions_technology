/*
 * Copyright (c) 2016 Open-RnD Sp. z o.o.
 * Copyright (c) 2016 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file SoC configuration macros for the LEOPARD family processors.
 *
 */

#ifndef _LEOPARD_SOC_H_
#define _LEOPARD_SOC_H_

#ifdef __cplusplus
extern "C" {
#endif


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
#include "soc_psram.h"
#include "soc_powergate.h"


void jtag_set(void);
int soc_dvfs_opt(void);
void soc_watchdog_clear(void);
void wd_clear_wdreset_cnt(void);
uint8_t ipmsg_btc_get_ic_pkt(void);

//#define CONFIG_SLEEP_DBG 
#ifdef CONFIG_SLEEP_DBG
#define sl_dbg      printk
#else
#define sl_dbg(...)
#endif

#define __FPU_PRESENT			1
#define __MPU_PRESENT           1
#define CHIP_LEOPARD            1

#endif /* !_ASMLANGUAGE */

#ifdef __cplusplus
}
#endif

#endif /* _LEOPARD_SOC_H_ */
