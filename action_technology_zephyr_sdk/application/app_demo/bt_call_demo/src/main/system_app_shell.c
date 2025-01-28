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
#include "btcall.h"


static int shell_btcall(const struct shell *shell, size_t argc, char *argv[])
{
	if (!strcmp(argv[1], "accept")) {
		btcall_accept_call();
	} else if (!strcmp(argv[1], "reject")) {
		btcall_reject_call();
	} else if (!strcmp(argv[1], "hangup")) {
		btcall_handup_call();
	} else if (!strcmp(argv[1], "siri")) {
		btcall_siri_control();
	}

	return 0;
}



SHELL_STATIC_SUBCMD_SET_CREATE(acts_app_cmds,
	SHELL_CMD(btcall, NULL, "btcall command", shell_btcall),
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

SHELL_CMD_REGISTER(app, &acts_app_cmds, "Application shell commands", cmd_acts_app);

