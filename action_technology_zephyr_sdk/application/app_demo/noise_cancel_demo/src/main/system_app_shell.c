/*
 * Copyright (c) 2019 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief system app shell.
 */
#include <os_common_api.h>
#include <mem_manager.h>
#include <msg_manager.h>
#include <stdio.h>
#include <string.h>
#include <shell/shell.h>
#include <stdlib.h>
#include <userapp.h>


static int shell_nc(const struct shell *shell, size_t argc, char *argv[])
{
	if (!strcmp(argv[1], "start")) {
		if(argv[2]) {
			nc_start(argv[2]);
			printk("noise cancel start\n");
		}
	} else if (!strcmp(argv[1], "stop")) {
		nc_stop();
		printk("noise cancel stop\n");
	} else if (!strcmp(argv[1], "mp3start")) {
		if(argv[2]) {
			nc_mp3_start(argv[2]);
			printk("noise cancel start\n");
		}	
	} else if (!strcmp(argv[1], "getvad")) {
		nc_get_vad();
		
	}

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(test_app_cmds,
	SHELL_CMD(nc, NULL, "noise cancel cmd", shell_nc),
	SHELL_SUBCMD_SET_END
);

static int cmd_acts_app(const struct shell *shell, size_t argc, char **argv)
{
	if (argc == 1) {
		shell_help(shell);
		return SHELL_CMD_HELP_PRINTED;
	}

	shell_error(shell, "%s unknown parameter: %s", argv[0], argv[1]);

	return -EINVAL;
}

SHELL_CMD_REGISTER(app, &test_app_cmds, "Application shell commands", cmd_acts_app);

