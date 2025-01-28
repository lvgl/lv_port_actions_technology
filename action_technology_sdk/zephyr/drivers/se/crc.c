/*
 * Copyright (c) 2017 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <init.h>
#include <device.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <drivers/se/se.h>
#include <soc.h>
#include <logging/log.h>
LOG_MODULE_REGISTER(crc, CONFIG_LOG_DEFAULT_LEVEL);

#define CRC_FIFO_DEPTH            16
#define CRC_TYPE_16               0
#define CRC_TYPE_32               1
#define POLY_04C11DB7             0
#define POLY_1021                 1
#define POLY_8005                 2

/* CRC mode and paramter */
struct crc_param_data {
	uint8_t crc_type;
	uint8_t poly;
	bool in_invert;
	bool out_invert;
	uint32_t out_xor;
	uint32_t inital_val;
};

/****************************************************************************
 * Name: crc_hw_calc
 *
 * Description:
 *   hardware calculates crc result.
 *
 ****************************************************************************/

static uint32_t crc_hw_calc(const void *in, size_t size)
{
	size_t size_remain;
	size_t crc_len;
	uint32_t crc_ctrl_val;
	uint32_t crc_infifoctl_val;
	uint8_t *crc_in = (uint8_t *)in;
	uint32_t crc_out_val;

	/* set SE FIFO to CRC */

	sys_write32(SE_FIFOCTRL_CRC, SE_FIFOCTRL);

	/* disable DRQ */

	crc_infifoctl_val  = sys_read32(CRC_INFIFOCTL);
	crc_infifoctl_val &= ~CRC_INFIFOCTL_DE_MASK;
	sys_write32(crc_infifoctl_val, CRC_INFIFOCTL);

	size_remain = size;

	while (size_remain > 0) {
		if (size_remain >= CRC_FIFO_DEPTH) {
			crc_len      = CRC_FIFO_DEPTH;
			size_remain -= CRC_FIFO_DEPTH;
		} else {
			crc_len     = size_remain;
			size_remain = 0;
		}

		sys_write32(crc_len, CRC_LEN);

		se_memcpy((void *)CRC_INFIFO,
                   crc_in, crc_len, CPY_MEMU8_TO_FIFO);

		/* enable crc */

		crc_ctrl_val = sys_read32(CRC_CTRL);
		sys_write32(crc_ctrl_val | CRC_CTRL_EN, CRC_CTRL);

		/* wait crc calculation completed */
		while ((sys_read32(CRC_CTRL) & CRC_CTRL_END) == 0);

		/* Clear completed flag for next CRC */

		crc_ctrl_val = sys_read32(CRC_CTRL);
		crc_ctrl_val |= AES_CTRL_END;
		sys_write32(crc_ctrl_val, AES_CTRL);

		crc_in += crc_len;
	}

	crc_out_val = sys_read32(CRC_DATAOUT);

	/* enable CRC reset */

	crc_ctrl_val = sys_read32(CRC_CTRL);
	crc_ctrl_val |= CRC_CTRL_RESET;
	sys_write32(crc_ctrl_val, CRC_CTRL);

	/* Wait finished */

	while (sys_read32(CRC_CTRL) & CRC_CTRL_RESET);

	return crc_out_val;
}

/****************************************************************************
 * Name: crc_setup_mr
 *
 * Description:
 *   set crc mode and parameter.
 *
 ****************************************************************************/

static int crc_setup_mr(struct crc_param_data *param)
{
	uint32_t regval = 0;

	sys_write32(regval, CRC_MODE);

	if (param->crc_type == CRC_TYPE_32)
		regval |= CRC_MODE_TYPE_CRC32;

	if (param->poly == POLY_8005)
		regval |= CRC_MODE_CRC16_POLY_8005;

	if (param->in_invert)
		regval |= CRC_MODE_IN_INV_TRUE;

	if (param->out_invert)
		regval |= CRC_MODE_OUT_INV_TRUE;

	sys_write32(regval | CRC_MODE_INITDATA_UPD, CRC_MODE);
	sys_write32(param->out_xor, CRC_DATAOUTXOR);
	sys_write32(param->inital_val, CRC_DATAINIT);

	return 0;
}

/****************************************************************************
 * Name: crc_init
 *
 * Description:
 *   before using crc, we should init crc clock and related things.
 *
 ****************************************************************************/

static int crc_init(void)
{
	uint32_t clken0_val;

	clken0_val = sys_read32(CMU_DEVCLKEN0);

	/* check whether the CLOCK_ID_SE has been enabled or not */
	if (!(clken0_val & (0x1 << CLOCK_ID_SE))) {

		/* if not initialized, we should init it */
		/* SE CLK = HOSC/1 */
		sys_write32((0 << 8) | (0 << 0), CMU_SECCLK);

		/* enable SE controller clock */
		acts_clock_peripheral_enable(CLOCK_ID_SE);

		/* reset SE controller */
		acts_reset_peripheral(RESET_ID_SE);
	}

	/* enable CRC controller clock */
	sys_write32(CRC_CTRL_CLK_EN, CRC_CTRL);

	return 0;
}

/****************************************************************************
 * Name: crc_deinit
 *
 * Description:
 *   Disable CRC for energy saving purpose.
 *
 ****************************************************************************/

static void crc_deinit(void)
{
	/* disable crc */
	sys_write32(0, CRC_CTRL);
}

/****************************************************************************
 * Name: crc
 *
 * Description:
 *   get the CRC result.
 *
 ****************************************************************************/

static uint32_t crc(struct crc_param_data *param, const unsigned char *ptr, unsigned int len)
{
	uint32_t crc_val;

	k_mutex_lock(&se_lock, K_FOREVER);

	crc_init();

	crc_setup_mr(param);

	crc_val = crc_hw_calc(ptr, len);

	crc_deinit();

	k_mutex_unlock(&se_lock);

	return crc_val;
}

/****************************************************************************
 * Name: crc16_ibm
 *
 * Description:
 *   get the CRC16-IBM result.
 *
 ****************************************************************************/

uint16_t crc16_ibm(uint16_t inital_val, const unsigned char* ptr, unsigned int len)
{
	struct crc_param_data param;

	param.crc_type = CRC_TYPE_16;
	param.poly = POLY_8005;
	param.in_invert = true;
	param.out_invert = true;
	param.out_xor = 0x0000;
	param.inital_val = inital_val;
	return crc(&param, ptr, len);
}

/****************************************************************************
 * Name: crc16_maxim
 *
 * Description:
 *   get the CRC16-MAXIM result.
 *
 ****************************************************************************/

uint16_t crc16_maxim(uint16_t inital_val, const unsigned char* ptr, unsigned int len)
{
	struct crc_param_data param;

	param.crc_type = CRC_TYPE_16;
	param.poly = POLY_8005;
	param.in_invert = true;
	param.out_invert = true;
	param.out_xor = 0xFFFF;
	param.inital_val = inital_val;
	return crc(&param, ptr, len);
}

/****************************************************************************
 * Name: crc16_usb
 *
 * Description:
 *   get the CRC16-USB result.
 *
 ****************************************************************************/

uint16_t crc16_usb(uint16_t inital_val, const unsigned char* ptr, unsigned int len)
{
	struct crc_param_data param;

	param.crc_type = CRC_TYPE_16;
	param.poly = POLY_8005;
	param.in_invert = true;
	param.out_invert = true;
	param.out_xor = 0xFFFF;
	param.inital_val = inital_val;
	return crc(&param, ptr, len);
}

/****************************************************************************
 * Name: crc16_modbus
 *
 * Description:
 *   get the CRC16-MODBUS result.
 *
 ****************************************************************************/

uint16_t crc16_modbus(uint16_t inital_val, const unsigned char* ptr, unsigned int len)
{
	struct crc_param_data param;

	param.crc_type = CRC_TYPE_16;
	param.poly = POLY_8005;
	param.in_invert = true;
	param.out_invert = true;
	param.out_xor = 0x0000;
	param.inital_val = inital_val;
	return crc(&param, ptr, len);
}

/****************************************************************************
 * Name: crc16_ccitt
 *
 * Description:
 *   get the CRC16-CCITT result.
 *
 ****************************************************************************/

uint16_t crc16_ccitt(uint16_t inital_val, const unsigned char* ptr, unsigned int len)
{
	struct crc_param_data param;

	param.crc_type = CRC_TYPE_16;
	param.poly = POLY_1021;
	param.in_invert = false;
	param.out_invert = false;
	param.out_xor = 0x0000;
	param.inital_val = inital_val;
	return crc(&param, ptr, len);
}

/****************************************************************************
 * Name: crc16_ccitt_false
 *
 * Description:
 *   get the CRC16-CCITT-FALSE result.
 *
 ****************************************************************************/

uint16_t crc16_ccitt_false(uint16_t inital_val, const unsigned char* ptr, unsigned int len)
{
	struct crc_param_data param;

	param.crc_type = CRC_TYPE_16;
	param.poly = POLY_1021;
	param.in_invert = false;
	param.out_invert = false;
	param.out_xor = 0x0000;
	param.inital_val = inital_val;
	return crc(&param, ptr, len);
}

/****************************************************************************
 * Name: crc16_x5
 *
 * Description:
 *   get the CRC16-X5 result.
 *
 ****************************************************************************/

uint16_t crc16_x5(uint16_t inital_val, const unsigned char* ptr, unsigned int len)
{
	struct crc_param_data param;

	param.crc_type = CRC_TYPE_16;
	param.poly = POLY_1021;
	param.in_invert = true;
	param.out_invert = true;
	param.out_xor = 0xFFFF;
	param.inital_val = inital_val;
	return crc(&param, ptr, len);
}

/****************************************************************************
 * Name: crc16_xmodem
 *
 * Description:
 *   get the CRC16-XMODEM result.
 *
 ****************************************************************************/

uint16_t crc16_xmodem(uint16_t inital_val, const unsigned char* ptr, unsigned int len)
{
	struct crc_param_data param;

	param.crc_type = CRC_TYPE_16;
	param.poly = POLY_1021;
	param.in_invert = false;
	param.out_invert = false;
	param.out_xor = 0x0000;
	param.inital_val = inital_val;
	return crc(&param, ptr, len);
}

/****************************************************************************
 * Name: crc32
 *
 * Description:
 *   get the CRC32 result.
 *
 ****************************************************************************/

uint32_t crc32(uint32_t inital_val, const unsigned char* ptr, unsigned int len)
{
	struct crc_param_data param;

	param.crc_type = CRC_TYPE_32;
	param.poly = POLY_04C11DB7;
	param.in_invert = true;
	param.out_invert = true;
	param.out_xor = 0xFFFFFFFF;
	param.inital_val = inital_val;
	return crc(&param, ptr, len);
}

/****************************************************************************
 * Name: crc32_mpeg2
 *
 * Description:
 *   get the CRC32-MPEG2 result.
 *
 ****************************************************************************/

uint32_t crc32_mpeg2(uint32_t inital_val, const unsigned char* ptr, unsigned int len)
{
	struct crc_param_data param;

	param.crc_type = CRC_TYPE_32;
	param.poly = POLY_04C11DB7;
	param.in_invert = false;
	param.out_invert = false;
	param.out_xor = 0x00000000;
	param.inital_val = inital_val;
	return crc(&param, ptr, len);
}

/****************************************************************************
 * Name: utils_crc32
 *
 * Description:
 *   get the utils_CRC32 result.
 *
 ****************************************************************************/

uint32_t utils_crc32(uint32_t crc, const uint8_t *ptr, int buf_len)
{
	return crc32(~crc, ptr, buf_len);
}

