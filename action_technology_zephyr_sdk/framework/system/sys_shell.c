/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file system shell
 */

#define SYS_LOG_NO_NEWLINE
#ifdef SYS_LOG_DOMAIN
#undef SYS_LOG_DOMAIN
#endif
#define SYS_LOG_DOMAIN "system_shell"

#include <os_common_api.h>
#include <mem_manager.h>
#include <stdio.h>
#include <string.h>
#include <shell/shell.h>
#include <stdlib.h>
#include <limits.h>
#include <property_manager.h>
#include <sys_wakelock.h>
#ifdef CONFIG_MEDIA_PLAYER
#include <media_player.h>
#endif

extern void mem_manager_dump_ext(int dump_detail, const char* match_value);

static int shell_set_config(const struct shell *shell,
					size_t argc, char **argv)
{
	int ret = 0;

	if (argc < 2) {
		SYS_LOG_INF("argc <\n");
		return 0;
	}

#ifdef CONFIG_PROPERTY
	if (argc < 3) {
		ret = property_set(argv[1], argv[1], 0);
	} else {
		ret = property_set(argv[1], argv[2], strlen(argv[2]));
	}
#endif
	if (ret < 0) {
		SYS_LOG_INF("%s failed %d\n", argv[1], ret);
		return -1;
	}
#ifdef CONFIG_PROPERTY
	property_flush(NULL);
#endif
	SYS_LOG_INF("%s : %s ok\n", argv[1], argv[2]);
	return 0;
}

#ifdef CONFIG_MEDIA_PLAYER
static int shell_set_effect_config(const struct shell *shell,
					size_t argc, char **argv)
{
	if (argc < 2) {
		printk("argc %d < 2\n",argc);
		return 0;
	}

    media_player_set_effect_bypass(atoi(argv[1]));

	SYS_LOG_INF("set_music_effect %s ok\n", argv[1]);
	return 0;
}

static int shell_set_voice_effect_config(const struct shell *shell,
					size_t argc, char **argv)
{
	if (argc < 2) {
		SYS_LOG_INF("argc %d < 2\n",argc);
		return 0;
	}

	media_player_set_voice_effect_bypass(atoi(argv[1]), atoi(argv[2]));

	SYS_LOG_INF("set_voice %s : %s ok\n", argv[1], argv[2]);
	return 0;
}
#endif /* CONFIG_MEDIA_PLAYER */

static int shell_dump_meminfo(const struct shell *shell,
					size_t argc, char **argv)
{
	if (argc == 1) {
		mem_manager_dump();
	} else if (argc == 2) {
		SYS_LOG_INF("argv[1] %s \n",argv[1]);
		mem_manager_dump_ext(atoi(argv[1]), NULL);
	} else if (argc == 3) {
		SYS_LOG_INF("argv[1] %s  argv[2] %s \n",argv[1], argv[2]);
		mem_manager_dump_ext(atoi(argv[1]), argv[2]);
	}

	return 0;
}

#ifdef CONFIG_SYS_WAKELOCK
static int shell_wake_lock(const struct shell *shell, size_t argc, char **argv)
{
	if (argc == 2) {
		if (!strcmp(argv[1], "lock")) {
			shell_print(shell, "wake dbg use lock\n");
			sys_wake_lock_ext(FULL_WAKE_LOCK,SYS_WAKE_LOCK_USER);
		} else if (!strcmp(argv[1], "unlock")) {
			shell_print(shell, "wake dbg use unlock\n");
			sys_wake_unlock_ext(FULL_WAKE_LOCK,SYS_WAKE_LOCK_USER);
		} else if (!strcmp(argv[1], "dump")) {
			sys_wakelocks_dump();
		} 
	}
	return 0;
}
#endif


SHELL_STATIC_SUBCMD_SET_CREATE(sub_system,
	SHELL_CMD(dumpmem, NULL, "dump mem info.", shell_dump_meminfo),
	SHELL_CMD(set_config, NULL, "set system config ", shell_set_config),
#ifdef CONFIG_MEDIA_PLAYER
	SHELL_CMD(set_voice_effect, NULL, "set voice effect bypass ", shell_set_voice_effect_config),
	SHELL_CMD(set_music_effect, NULL, "set music effect bypass ", shell_set_effect_config),
#endif
#ifdef CONFIG_SYS_WAKELOCK
	SHELL_CMD(wlock, NULL, "wlock lock[unlock] ", shell_wake_lock),
#endif
	SHELL_SUBCMD_SET_END /* Array terminated. */
);

SHELL_CMD_REGISTER(system, &sub_system, "system commands", NULL);
