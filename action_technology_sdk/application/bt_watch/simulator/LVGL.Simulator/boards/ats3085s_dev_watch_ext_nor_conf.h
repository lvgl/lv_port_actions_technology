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
#define CONFIG_PANEL_TIMING_REFRESH_RATE_HZ 50
#define CONFIG_PANEL_ROUND_SHAPE 1
/* lvgl config */
#define CONFIG_LV_VDB_NUM 2
#define CONFIG_LV_VDB_SIZE 17252
/* ui mem config */
#define CONFIG_UI_MEM_BLOCK_SIZE 434312
#define CONFIG_UI_MEM_NUMBER_BLOCKS 10
#define CONFIG_UI_GUI_MEM_POOL_SIZE 96000
#define CONFIG_UI_RES_MEM_POOL_SIZE 2200000
/* font cache config */
#define CONFIG_RES_MANAGER_BLOCK_SIZE 434432
#define CONFIG_RES_MANAGER_BLOCK_NUM 2
#define CONFIG_BITMAP_PER_FONT_CACHE_SIZE 98304
#define CONFIG_BITMAP_FONT_CACHE_POOL_SIZE 300000
#define CONFIG_FREETYPE_PER_FONT_CACHE_SIZE 24576
/* view cache config */
#define CONFIG_VIEW_CACHE_LEVEL 2
#define CONFIG_VIEW_SCROLL_MEM_DEFAULT 1
/* resource path config */
#define CONFIG_APP_UI_DISK "../../res/"
#define CONFIG_APP_FONT_DISK "../../fonts/"
#define CONFIG_WELCOME_UI_DISK "../../fs_sdfs/sdfs_k/"

/* psram size */
#define CONFIG_PSRAM_SIZE 8192

#endif /* ATS3085_DVB_WATCH_CONF_H_ */
