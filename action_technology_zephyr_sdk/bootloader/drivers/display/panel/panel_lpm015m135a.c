/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sys/byteorder.h>
#include "panel_device.h"

#include <logging/log.h>
LOG_MODULE_DECLARE(lcd_panel, CONFIG_DISPLAY_LOG_LEVEL);

/*********************
 *      DEFINES
 *********************/

/**
 * #define CONFIG_PANEL_PORT_TYPE		((2 << 8) | (0))
 * #define CONFIG_PANEL_PORT_TR_LOW_BIT 6
 * #define CONFIG_PANEL_PORT_TR_HCK_TAIL 0
 * #define CONFIG_PANEL_PORT_TR_FRP 1
 * #define CONFIG_PANEL_PORT_TR_HCK_ON_IDLE 1
 *
 * #define CONFIG_PANEL_PORT_TR_TW_XRST 2
 * #define CONFIG_PANEL_PORT_TR_TW_VCOM 100
 * #define CONFIG_PANEL_PORT_TR_TD_VST 16
 * #define CONFIG_PANEL_PORT_TR_TW_VST 64
 * #define CONFIG_PANEL_PORT_TR_TD_HST 0
 * #define CONFIG_PANEL_PORT_TR_TW_HST 4
 * #define CONFIG_PANEL_PORT_TR_TD_VCK 40
 * #define CONFIG_PANEL_PORT_TR_TW_VCK (200/4 + 2)
 * #define CONFIG_PANEL_PORT_TR_TP_HCK 2
 * #define CONFIG_PANEL_PORT_TR_TD_HCK 4
 * #define CONFIG_PANEL_PORT_TR_TS_ENB 2
 * #define CONFIG_PANEL_PORT_TR_TH_ENB 3
 * #define CONFIG_PANEL_PORT_TR_TD_DATA 3
 * #define CONFIG_PANEL_PORT_TR_TD_ENB (200/4 + 12)
 * #define CONFIG_PANEL_PORT_TR_TW_ENB ((200/4 + 12) + 52)
 *
 * #define CONFIG_PANEL_TIMING_HACTIVE	(200)
 * #define CONFIG_PANEL_TIMING_VACTIVE	(228)
 * #define CONFIG_PANEL_TIMING_PIXEL_CLK_KHZ (2000)
 * #define CONFIG_PANEL_TIMING_REFRESH_RATE_HZ (20)
 */

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *  FUNCTIONS
 **********************/

static int _panel_init(const struct device *dev)
{
	return 0;
}

static const struct lcd_panel_ops lcd_panel_ops = {
	.init = _panel_init,
};

const struct lcd_panel_config lcd_panel_lpm015m135a_config = {
	.videoport = PANEL_VIDEO_PORT_INITIALIZER,
	.videomode = PANEL_VIDEO_MODE_INITIALIZER,
	.ops = &lcd_panel_ops,
	.tw_reset = 10,
	.ts_reset = 120,
};
