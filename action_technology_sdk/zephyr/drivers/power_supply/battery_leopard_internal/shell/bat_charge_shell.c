/*
 * Copyright (c) 2020 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Bat_charge Driver Shell implementation
 */
#include <shell/shell.h>
#include <board_cfg.h>

static int cmd_enter_shipmode(const struct shell *shell,
			      size_t argc, char **argv)
{
    sys_pm_factory_poweroff();
    return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_bat,
	SHELL_CMD(shipmode, NULL, "Enter ship mode.", cmd_enter_shipmode),

	SHELL_SUBCMD_SET_END /* Array terminated. */
);

SHELL_CMD_REGISTER(bat, &sub_bat, "Actions bat commands", NULL);
