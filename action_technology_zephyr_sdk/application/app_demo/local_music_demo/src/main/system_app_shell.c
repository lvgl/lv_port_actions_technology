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
#include <lcmusic.h>

static int shell_lcmusic(const struct shell *shell, size_t argc, char *argv[])
{
	if (!strcmp(argv[1], "play")) {
		lcmusic_start_player();
	} else if (!strcmp(argv[1], "stop")) {
		lcmusic_stop();
	} else if (!strcmp(argv[1], "next")) {
		lcmusic_play_next();
	} else if (!strcmp(argv[1], "pre")) {
		lcmusic_play_prev();
	}

	return 0;

}



SHELL_STATIC_SUBCMD_SET_CREATE(acts_app_cmds,
	SHELL_CMD(lcmusic, NULL, "lcmusic command", shell_lcmusic),
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

