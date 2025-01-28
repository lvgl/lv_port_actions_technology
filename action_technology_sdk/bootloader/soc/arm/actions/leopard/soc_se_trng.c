/*
 * Copyright (c) 2020 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file security engine trng for Actions SoC
 */

#include <kernel.h>
#include <device.h>
#include <soc.h>
#include <sys/printk.h>

static void se_init(void)
{
	/* SE CLK = HOSC/1 */
	sys_write32((0 << 8) | (0 << 0), CMU_SECCLK);

	acts_clock_peripheral_enable(CLOCK_ID_SE);

	acts_reset_peripheral(RESET_ID_SE);

	sys_write32((1 << 3), TRNG_CTRL);
}

static void se_deinit(void)
{
	sys_write32(sys_read32(TRNG_CTRL) & ~(1 << 3), TRNG_CTRL);

	acts_clock_peripheral_disable(CLOCK_ID_SE);
}

/**
 * \brief  IC inner true random number generator module init
 */
int se_trng_init(void)
{
	se_init();
	return 0;
}

/**
 * \brief  IC inner true random number generator module deinit
 */
int se_trng_deinit(void)
{
	se_deinit();
	return 0;
}

/**
 * \brief               true random number generator use IC inner crc module
 *
 * \param trng_low      true random number low 32bit
 * \param trng_high     true random number high 32bit
 *
 * \return              0
 */
uint32_t se_trng_process(uint32_t *trng_low, uint32_t *trng_high)
{
	uint32_t trng_ctrl_value;

	trng_ctrl_value = sys_read32(TRNG_CTRL);
	trng_ctrl_value = (trng_ctrl_value & ~(0x03<<1)) | (0x0<<1); //TRNG Mode : OSC+LFSR
	trng_ctrl_value |= (1<<8)|(1<<9)|(1<<10); //OSC XOR mode, SAMPLE CLK MODE
	trng_ctrl_value = (trng_ctrl_value & ~(0x03<<6)) | (0x02<<6); //OSC Sample Low
	sys_write32(trng_ctrl_value, TRNG_CTRL);

	trng_ctrl_value |= (1<<0); //TRNG enable
	sys_write32(trng_ctrl_value, TRNG_CTRL);

	while ((sys_read32(TRNG_CTRL) & (1<<31)) == 0); //wait trng ready

	*trng_low  = sys_read32(TRNG_LR);
	*trng_high = sys_read32(TRNG_MR);

	trng_ctrl_value = sys_read32(TRNG_CTRL);
	trng_ctrl_value &= ~(1<<0); //clear trng ready and finish trng
	sys_write32(trng_ctrl_value, TRNG_CTRL);

	return 0;
}

/* sample code, trng 64bit, 150us general one ramdom

	uint32_t trng_low, trng_high;
	int i;

	se_trng_init();
	for (i = 0; i < 8; i++) {
		se_trng_process(&trng_low, &trng_high);
		printk("ramdom %d : [0x%08x, 0x%08x] @ %d us\n", i + 1, trng_low, trng_high, BROM_TIMESTAMP_GET_US());
	}
	se_trng_deinit();

*/

