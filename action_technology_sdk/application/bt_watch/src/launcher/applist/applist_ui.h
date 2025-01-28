/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SRC_LAUNCHER_APPLIST_UI_H_
#define SRC_LAUNCHER_APPLIST_UI_H_

/*********************
 *      INCLUDES
 *********************/
#include <app_ui.h>

/**********************
 *      TYPEDEFS
 **********************/
enum {
	APPLIST_MODE_GRID = 0,
	APPLIST_MODE_LIST,
	APPLIST_MODE_WATERWHEEL,
	APPLIST_MODE_CELLULAR,
	APPLIST_MODE_WONHOT,
	APPLIST_MODE_TURNTABLE,
	APPLIST_MODE_WATERFALL,
	APPLIST_MODE_CUBE_TURNTABLE,
	APPLIST_MODE_CIRCLE,
#if defined(CONFIG_VG_LITE)
	APPLIST_MODE_3D_WHEEL,
#endif
	NUM_APPLIST_MODES,
};

typedef struct applist_view_presenter {
	uint8_t (*get_view_mode)(void);
	void (*set_view_mode)(uint8_t mode);

	void (*load_scroll_value)(int16_t *scrl_x, int16_t *scrl_y);
	void (*save_scroll_value)(int16_t scrl_x, int16_t scrl_y);

	bool (*phone_is_on)(void);
	void (*toggle_phone)(void);

	bool (*vibrator_is_on)(void);
	void (*toggle_vibrator)(void);

	bool (*aod_mode_is_on)(void);
	void (*toggle_aod_mode)(void);

	void (*open_stopwatch)(void);
	void (*open_alarmclock)(void);
	void (*open_compass)(void);
	void (*open_longview)(void);
	void (*open_alipay)(void);
	void (*open_wxpay)(void);
	void (*open_gps)(void);
	void (*open_three_dimensional)(void);
	void (*open_svgmap)(void);
	void (*open_setting)(void);
	void (*open_vib)(void);
#ifdef CONFIG_THIRD_PARTY_APP
	void (*open_third_party_app)(void);
#endif
} applist_view_presenter_t;
extern const applist_view_presenter_t applist_view_presenter;

/**********************
 * GLOBAL PROTOTYPES
 **********************/
/**
 * Enter APPLIST_UI
*/
int applist_ui_enter(void);

#endif /* SRC_LAUNCHER_APPLIST_UI_H_ */
