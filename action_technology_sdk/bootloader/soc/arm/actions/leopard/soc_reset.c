/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file peripheral reset interface for Actions SoC
 */

#include <kernel.h>
#include <soc.h>

static void acts_reset_peripheral_control(int reset_id, int assert)
{
	unsigned int key;

	if (reset_id > RESET_ID_MAX_ID)
		return;

	key = irq_lock();

	if (assert) {
		if (reset_id < 32) {
			sys_clear_bit(RMU_MRCR0, reset_id);
		} else {
			sys_clear_bit(RMU_MRCR1, reset_id - 32);
		}
	} else {
		if (reset_id < 32) {
			sys_set_bit(RMU_MRCR0, reset_id);
		} else {
			sys_set_bit(RMU_MRCR1, reset_id - 32);
		}
	}
	irq_unlock(key);
}

void acts_reset_peripheral_assert(int reset_id)
{
	acts_reset_peripheral_control(reset_id, 1);
}

void acts_reset_peripheral_deassert(int reset_id)
{
	acts_reset_peripheral_control(reset_id, 0);
}

void acts_reset_peripheral(int reset_id)
{
	acts_reset_peripheral_assert(reset_id);
	acts_reset_peripheral_deassert(reset_id);
}
