/*
 * Copyright (c) 2019 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief fcc test headfile.
 */

#include "soc_regs.h"
#include "soc_reset.h"
#include "soc_clock.h"

#include "types.h"
#include "stdarg.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"

#include "sys_io.h"
#include "ft_debug.h"
#include "ft_sys_io.h"

#define PASSED		    0
#define FAILED		    1

#define CONFIG_HOSC_CLK_MHZ 	32
#define TIMER_CLK_FRE_MHZ		CONFIG_HOSC_CLK_MHZ
#define TIMER_CLOCK				(TIMER_CLK_FRE_MHZ * 1000 * 1000)
#define COUNT_TO_USEC(x)		((x) / TIMER_CLK_FRE_MHZ)
#define USEC_TO_COUNT(x)		((x) * TIMER_CLK_FRE_MHZ)
#define TIMER_MAX_VALUE			0xfffffffful

#define  GPIOX_CTL(n)			(GPIO_REG_BASE+n*4)

/* UART registers */
#define UART_CTL				(0x0000)
#define UART_RXDAT				(0x0004)
#define UART_TXDAT				(0x0008)
#define UART_STAT				(0x000C)

#define UART_STAT_UTBB			(0x1 << 21)
#define UART_STAT_TFES			(0x1 << 10)
#define UART_STAT_TFFU			(0x1 << 6)
#define UART_STAT_RFEM			(0x1 << 5)

#define UART_REG_CONFIG_NUM		(3)

struct uart_reg_config {
	unsigned int reg;
	unsigned int mask;
	unsigned int val;
};

struct owl_uart_dev {
	unsigned char chan;
	unsigned char mfp;
	unsigned char clk_id;
	unsigned char rst_id;
	unsigned long base;
	unsigned long uart_clk;
	struct uart_reg_config reg_cfg[UART_REG_CONFIG_NUM];
};

enum {
	FCC_UART_MODE = 0,
	FCC_BT_TX_MODE = 1,
	FCC_BT_RX_MODE = 2,
	FCC_BT_ATT_MODE = 3,
	FCC_BT_EXIT = 4,
};

struct ft_env_var {
	void (*ft_printf)(const char *fmt, ...);
	void (*ft_udelay)(unsigned int us);
	void (*ft_mdelay)(unsigned int ms);
	uint32 (*ft_get_time_ms)(void);
	int (*ft_efuse_write_32bits)(uint32 bits, uint32 num);
	uint32 (*ft_efuse_read_32bits)(uint32 num, uint32* efuse_value);
	void (*ft_load_fcc_bin)(void);
};

#define ft_udelay(us)   self->ft_udelay(us)
#define ft_mdelay(ms)   self->ft_mdelay(ms)
#define ft_get_time_ms()   self->ft_get_time_ms()
#define ft_efuse_write_32bits(bits, num)   self->ft_efuse_write_32bits(bits, num)
#define ft_efuse_read_32bits(num, efuse_value)   self->ft_efuse_read_32bits(num, efuse_value)
#define ft_load_fcc_bin()   self->ft_load_fcc_bin()
