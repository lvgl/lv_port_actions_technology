/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ATS3085_DVB_WATCH_CONF_H_
#define ATS3085_DVB_WATCH_CONF_H_

/* panel config */
#define CONFIG_PANEL_HOR_RES 466
#define CONFIG_PANEL_VER_RES 466
#define CONFIG_PANEL_TIMING_REFRESH_RATE_HZ 60
#define CONFIG_PANEL_ROUND_SHAPE 1
/* lvgl config */
#define CONFIG_LV_VDB_NUM 2
#define CONFIG_LV_VDB_SIZE 109044
/* ui mem config */
#define CONFIG_UI_MEM_BLOCK_SIZE 434312
#define CONFIG_UI_GUI_MEM_POOL_SIZE 1396000
#define CONFIG_UI_RES_MEM_POOL_SIZE 2500000
/* font cache config */
#define CONFIG_BITMAP_PER_FONT_CACHE_SIZE 98304
#define CONFIG_BITMAP_FONT_CACHE_POOL_SIZE 300000
#define CONFIG_FREETYPE_PER_FONT_CACHE_SIZE 8192
/* resource path config */
#define CONFIG_APP_UI_DISK "../../boards/ats3085s_dev_watch_ext_nor/res/"
#define CONFIG_APP_FONT_DISK "../../boards/ats3085s_dev_watch_ext_nor/fonts/"
#define CONFIG_WELCOME_UI_DISK "../../boards/ats3085s_dev_watch_ext_nor/sdfs_k/"

/* psram size */
#define CONFIG_PSRAM_SIZE 8192

#endif /* ATS3085_DVB_WATCH_CONF_H_ */
