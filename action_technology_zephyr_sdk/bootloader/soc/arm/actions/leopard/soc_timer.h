/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file timer for Actions SoC
 */

#ifndef SOC_TIMER_H_
#define SOC_TIMER_H_

#define     T0_CTL_EN                                                         5
#define     T0_CTL_RELO                                                       2
#define     T0_CTL_ZIEN                                                       1
#define     T0_CTL_ZIPD                                                       0

#define TIMER_MAX_CYCLES_VALUE			0xfffffffful


typedef struct {
    volatile uint32_t ctl;
    volatile uint32_t val;
    volatile uint32_t cnt;
} timer_register_t;

static inline void timer_reg_wait(void)
{
	volatile int i;

	for (i = 0; i < 10; i++)
		;
}


#endif /* SOC_TIMER_H_ */
