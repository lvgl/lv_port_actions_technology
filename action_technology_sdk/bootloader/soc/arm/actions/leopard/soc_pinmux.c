/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file pinmux interface for Actions SoC
 */

#include <errno.h>
#include <kernel.h>
#include "soc.h"

int acts_pinmux_set(unsigned int pin, unsigned int mode)
{
	unsigned int key, ctl_reg, val;

	if (pin >= GPIO_MAX_PIN_NUM)
		return -EINVAL;

	ctl_reg = GPIO_REG_CTL(GPIO_REG_BASE, pin);

	key = irq_lock();

	val = (sys_read32(ctl_reg) & ~PINMUX_MODE_MASK) | mode;
	sys_write32(val, ctl_reg);

	irq_unlock(key);

	return 0;
}

int acts_pinmux_get(unsigned int pin, unsigned int *mode)
{
	unsigned int ctl_reg = GPIO_REG_CTL(GPIO_REG_BASE, pin);

	*mode = sys_read32(ctl_reg) & PINMUX_MODE_MASK;

	return 0;
}

void acts_pinmux_setup_pins(const struct acts_pin_config *pinconf, int pins)
{
	int i;

	for (i = 0; i < pins; i++) {
		acts_pinmux_set(pinconf[i].pin_num, pinconf[i].mode);
		//printk("pin=%d, ctl_reg=0x%x\n", pinconf[i].pin_num,
		//	pinconf[i].mode);
	}
}

static int acts_pinctl_reg_set(unsigned int pin, unsigned int val)
{
	unsigned int key, ctl_reg;
	if (pin >= GPIO_MAX_PIN_NUM)
		return -EINVAL;
	ctl_reg = GPIO_REG_CTL(GPIO_REG_BASE, pin);
	key = irq_lock();
	sys_write32(val, ctl_reg);
	irq_unlock(key);
	return 0;
}
void acts_pinctl_reg_setup_pins(const struct acts_pin_config *pinconf, int pins)
{
	int i;
	for (i = 0; i < pins; i++) {
		acts_pinctl_reg_set(pinconf[i].pin_num, pinconf[i].mode);
	}
}