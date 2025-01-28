/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief lcmusic app view
 */

#include "lcmusic.h"
#include <input_manager.h>
#ifdef CONFIG_UI_MANAGER
#include <ui_manager.h>
#endif


static void _lcmusic_display_icon_disk(void);


void lcmusic_view_init(struct lcmusic_app_t *lcmusic)
{
	u8_t init_play_state = lcmusic_get_play_state(lcmusic->cur_dir);

#ifdef CONFIG_UI_MANAGER
	ui_view_create(LCMUSIC_VIEW, NULL, 0);
#endif
#ifdef CONFIG_SEG_LED_MANAGER
	seg_led_manager_clear_screen(LED_CLEAR_ALL);
#endif
	_lcmusic_display_icon_disk();

	if (init_play_state == LCMUSIC_STATUS_STOP)
		lcmusic_display_icon_pause();
	else
		lcmusic_display_icon_play();

	/* don't show 0 although bp_info resume fail */
	if (lcmusic->music_bp_info.track_no == 0)
		lcmusic->music_bp_info.track_no = 1;
	lcmusic_display_track_no(lcmusic->music_bp_info.track_no, 3000);
	lcmusic->filt_track_no = 1;

	SYS_LOG_INF(" ok\n");
}

void lcmusic_view_deinit(void)
{
#ifdef CONFIG_UI_MANAGER
	ui_view_delete(LCMUSIC_VIEW);
#endif
	SYS_LOG_INF("ok\n");
}

static void _lcmusic_display_icon_disk(void)
{
	struct lcmusic_app_t *lcmusic = lcmusic_get_app();

	if (!lcmusic)
		return;

#ifdef CONFIG_SEG_LED_MANAGER
	if (memcmp(lcmusic->cur_dir, "/SD:", strlen("/SD:")) == 0) {
		seg_led_display_icon(SLED_SD, true);
	} else if (memcmp(lcmusic->cur_dir, "/USB:", strlen("/USB:")) == 0) {
		seg_led_display_icon(SLED_USB, true);
	}
#endif
}

void lcmusic_display_icon_play(void)
{
#ifdef CONFIG_SEG_LED_MANAGER
	seg_led_display_icon(SLED_PLAY, true);
	seg_led_display_icon(SLED_PAUSE, false);
#endif
#ifdef CONFIG_LED_MANAGER
	led_manager_set_breath(0, NULL, OS_FOREVER, NULL);
	led_manager_set_breath(1, NULL, OS_FOREVER, NULL);
#endif
}

void lcmusic_display_icon_pause(void)
{
#ifdef CONFIG_SEG_LED_MANAGER
	seg_led_display_icon(SLED_PLAY, false);
	seg_led_display_icon(SLED_PAUSE, true);
#endif
#ifdef CONFIG_LED_MANAGER
	led_manager_set_display(0, LED_ON, OS_FOREVER, NULL);
	led_manager_set_display(1, LED_ON, OS_FOREVER, NULL);
#endif
}

void lcmusic_display_play_time(struct lcmusic_app_t *lcmusic, int seek_time)
{
#ifdef CONFIG_SEG_LED_MANAGER
	char diplay_str[5] = {0};

	/* remove hours */
	int temp_time = (lcmusic->music_bp_info.bp_info.time_offset + seek_time) / 1000 % 3600;

	if (temp_time < 0)
		temp_time = 0;
	snprintf(diplay_str, sizeof(diplay_str), "%02u%02u", temp_time / 60, temp_time % 60);
	seg_led_display_string(SLED_NUMBER1, diplay_str, true);
	seg_led_display_icon(SLED_COL, true);
#endif
}

void lcmusic_display_track_no(u16_t track_no, int display_times)
{
#ifdef CONFIG_SEG_LED_MANAGER
	char diplay_str[5] = {0};

	snprintf(diplay_str, sizeof(diplay_str), "%04u", track_no % 10000);
	if (track_no > 9999)
		diplay_str[0] = 'A';
	seg_led_manager_clear_screen(LED_CLEAR_NUM);
	seg_led_display_icon(SLED_COL, false);
	seg_led_manager_set_timeout_event_start();
	seg_led_display_string(SLED_NUMBER1, diplay_str, true);
	seg_led_manager_set_timeout_event(display_times, NULL);
#endif
}

