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
#include <bt_player.h>

static int shell_btmusic(const struct shell *shell, size_t argc, char *argv[])
{
	int vol;

	if (argc == 1) {
		SYS_LOG_INF("<bt music cmd> <ex:bt music play>");
		SYS_LOG_INF("play: play bt music");
		SYS_LOG_INF("stop: stop bt music");
		SYS_LOG_INF("next: play next song");
		SYS_LOG_INF("pre: play pre song");
	}

	if (!strcmp(argv[1], "play")) {
		btmusic_start();
	} else if (!strcmp(argv[1], "stop")) {
		btmusic_stop(false);
	} else if (!strcmp(argv[1], "pre")) {
		btmusic_play_prev();
	} else if (!strcmp(argv[1], "next")) {
		btmusic_play_next();
	} else if (!strcmp(argv[1], "setvol")) {
		if (argv[2]) {
			vol = atoi(argv[2]);
			btmusic_vol_sync(vol);
		}
	}

	return 0;
}



SHELL_STATIC_SUBCMD_SET_CREATE(test_app_cmds,
	SHELL_CMD(btmusic, NULL, "btmusic cmd", shell_btmusic),
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

