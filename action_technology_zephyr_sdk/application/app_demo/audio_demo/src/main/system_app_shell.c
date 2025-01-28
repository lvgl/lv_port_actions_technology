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
#include "audio_app.h"

void test_function(void);

static int shell_audio_test(const struct shell *shell, size_t argc, char *argv[])
{
	if (!strcmp(argv[1], "record")) {
		if (!strcmp(argv[2], "start") && argc > 3) {
			record_start(argv[3]);
			printk("pcm_recorder_start %s\n", argv[3]);
		} else if (!strcmp(argv[2], "stop")) {
			record_stop();
			printk("pcm_recorder_stop\n");
		}
	} else if (!strcmp(argv[1], "play")) {
		if (!strcmp(argv[2], "start") && argc > 3) {
			play_start(argv[3]);
			printk("pcm_player_play %s\n", argv[3]);
		} else if (!strcmp(argv[2], "stop")) {
			play_stop();
			printk("pcm_player_stop\n");
		}
	} else if (!strcmp(argv[1], "micbypass")) {
		if (!strcmp(argv[2], "start")) {
			mic_bypass_start();
			printk("mic_bypass_start\n");
		} else if (!strcmp(argv[2], "stop")) {
			mic_bypass_stop();
			printk("mic_bypass_stop\n");
		}
	}

	return 0;
}



SHELL_STATIC_SUBCMD_SET_CREATE(acts_app_cmds,
	SHELL_CMD(audio_app, NULL, "audio app cmd", shell_audio_test),
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

