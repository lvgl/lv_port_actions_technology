/*
 * Copyright (c) 2020 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Audio Driver Shell implementation
 */
#include <shell/shell.h>
#include <string.h>
#include <stdlib.h>
#include <sys/byteorder.h>
#include <drivers/power_supply.h>
#include <board_cfg.h>
#include <sys/ring_buffer.h>
#include "../bat_charge_private.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(bshell, CONFIG_LOG_DEFAULT_LEVEL);

extern int dump_cw6305_all_reg(void);
extern bool _cw6305_write_bytes(uint8_t reg, uint8_t *data, uint16_t len);

static int cmd_read_register(const struct shell *shell,
			      size_t argc, char **argv)
{
	bat_charge_context_t *bat_charge = bat_charge_get_context();

	bat_charge->ext_charger_debug_open = 1;

	dump_cw6305_all_reg();

	bat_charge->ext_charger_debug_open = 0;

	return 0;
}

static int cmd_write_register(const struct shell *shell,
			      size_t argc, char **argv)
{
	unsigned long addr;
	uint8_t value;
	bool result;
	bat_charge_context_t *bat_charge = bat_charge_get_context();

	addr = strtoul(argv[1], NULL, 16);
	value = (uint8_t)strtoul(argv[2], NULL, 16);

	LOG_INF("0x%lx: 0x%x", addr, value);

	bat_charge->ext_charger_debug_open = 1;

	result = _cw6305_write_bytes(addr, &value, 1);

	bat_charge->ext_charger_debug_open = 0;
	
	if(!result) {
		LOG_ERR("set reg fail: 0x%lx", addr);
	}

	return 0;
}


SHELL_STATIC_SUBCMD_SET_CREATE(sub_bat,
	SHELL_CMD(mdb, NULL, "Read register.", cmd_read_register),
	SHELL_CMD(mwb, NULL, "Write register.", cmd_write_register),

	SHELL_SUBCMD_SET_END /* Array terminated. */
);

SHELL_CMD_REGISTER(bat, &sub_bat, "Actions bat commands", NULL);
