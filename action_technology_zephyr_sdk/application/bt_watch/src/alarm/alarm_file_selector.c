/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief alarm alarm file selector.
 */


#include "alarm.h"
#ifdef CONFIG_ITERATOR
#define ITERATOR_DIR_LEVEL 3

static iterator_t *alarm_iter = NULL;
static int _iterator_filter_dir(const char *path)
{
	return false;
}
static int _iterator_match_fn(const char *path, int is_dir)
{
	if (!is_dir) {
	#ifndef CONFIG_FORMAT_CHECK
		return true;
	#else
		return get_music_file_type(path) != UNSUPPORT_TYPE;
	#endif
	}

	if (_iterator_filter_dir(path))
		return false;
	else
		return true;
}

int alarm_init_iterator(struct alarm_app_t *alarm)
{
	file_iterator_cursor_t cursor;
	file_iterator_param_t param;

	if (alarm_iter)
		return -1;

	cursor.path = alarm->url;

	memset(&param, 0, sizeof(param));
	if (alarm->url[0] == 0) {
		SYS_LOG_INF("url null\n");
		cursor.path = NULL;
	}
	param.topdir = alarm->dir;
	param.cursor = &cursor;
	param.max_level = ITERATOR_DIR_LEVEL;
	param.match_fn = _iterator_match_fn;

	alarm_iter = file_iterator_create(&param);
	if (!alarm_iter)
		return -1;
	return 0;
}
void alarm_exit_iterator(void)
{
	if (alarm_iter)
		iterator_destroy(alarm_iter);
	alarm_iter = NULL;
}

const char *alarm_ring_play_next_url(struct alarm_app_t *alarm)
{
	if (!alarm_iter) {
		SYS_LOG_WRN("iter null\n");
		alarm->mplayer_state = ALARM_MPLAYER_STATE_ERROR;
		return NULL;
	}

	const char *path = iterator_next(alarm_iter, 0, NULL);

	if (!path) {
		return NULL;
	}

	strncpy(alarm->url, path, sizeof(alarm->url));
	alarm->url[sizeof(alarm->url) - 1] = 0;

	return alarm->url;
}
#endif

