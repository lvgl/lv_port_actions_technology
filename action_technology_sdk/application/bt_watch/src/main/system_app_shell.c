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
#include "system_app.h"
#ifdef CONFIG_PROPERTY
#include <property_manager.h>
#endif
#ifdef CONFIG_PLAYTTS
#include "tts_manager.h"
#endif
#ifdef CONFIG_BT_MANAGER
#include "bt_manager.h"
#endif

#ifdef CONFIG_MONKEY_TEST
#include<system_monkey_test.h>
#endif

#ifdef CONFIG_SHELL
static int shell_input_key_event(const struct shell *shell, size_t argc, char *argv[])
{
	if (argv[1] != NULL) {
		uint32_t key_event;
		key_event = strtoul(argv[1], (char **) NULL, 0);
		sys_event_report_input(key_event);
	}
	return 0;
}
#ifdef CONFIG_MONKEY_TEST
static int shell_monkey_event(const struct shell *shell, size_t argc, char *argv[])
{
	int len = strlen(argv[1]);

	if (!strncmp(argv[1], "start", len)) {
		if (argv[2] != NULL) {
			system_monkey_test_start(strtoul(argv[2], (char **) NULL, 0));
		} else {
			system_monkey_test_start(100);
		}
	} else if(!strncmp(argv[1], "stop", len)){
		system_monkey_test_stop();
	}

	return 0;
}
#endif
static int shell_set_config(const struct shell *shell, size_t argc, char *argv[])
{
#ifdef CONFIG_PROPERTY
	int ret = 0;

	if (argc < 2) {
		ret = property_set(argv[1], argv[1], 0);
	} else {
		ret = property_set(argv[1], argv[2], strlen(argv[2]));
	}

	if (ret < 0) {
		ret = -1;
	} else {
		property_flush(NULL);
	}
#endif

	SYS_LOG_INF("set config %s : %s ok\n", argv[1], argv[2]);
	return 0;
}

static int shell_dump_bt_info(const struct shell *shell, size_t argc, char *argv[])
{
#ifdef CONFIG_BT_MANAGER
	bt_manager_dump_info();
#endif
	return 0;
}

#ifdef CONFIG_RECORD_APP
extern void recorder_sample_rate_set(uint8_t sample_rate_kh);
static int shell_set_record_sample_rate(const struct shell *shell, size_t argc, char *argv[])
{
	if (argv[1] != NULL) {
		recorder_sample_rate_set(atoi(argv[1]));
	}
	return 0;
}
#endif

SHELL_STATIC_SUBCMD_SET_CREATE(acts_app_cmds,
	SHELL_CMD(set_config, NULL, "set system config", shell_set_config),
	SHELL_CMD(input, NULL, "input key event", shell_input_key_event),
#ifdef CONFIG_MONKEY_TEST
	SHELL_CMD(monkey, NULL, "monkey test ", shell_monkey_event),
#endif
	SHELL_CMD(btinfo, NULL, "dump bt info", shell_dump_bt_info),
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
#endif
