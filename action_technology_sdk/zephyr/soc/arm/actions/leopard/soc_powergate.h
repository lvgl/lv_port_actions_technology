/*
 * Copyright (c) 2018 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file reboot configuration macros for Actions SoC
 */

#ifndef	_ACTIONS_SOC_POWERGATE_H_
#define	_ACTIONS_SOC_POWERGATE_H_

enum
{
	POWERGATE_DISPLAY_PG_DEV,
	POWERGATE_GPU_PG_DEV,
	POWERGATE_DSP_AU_PG_DEV,
	POWERGATE_BT_PG_DEV,
	PWRGATE_MAINCPU_PG_DEV,
	POWERGATE_MAX_DEV,
};

bool soc_powergate_is_poweron(uint8_t pg_dev);
int soc_powergate_set(uint8_t pg_dev,  bool power_on);
void soc_powergate_init(void);
void soc_powergate_dump(void);

#endif /* _ACTIONS_SOC_POWERGATE_H_	*/
