/**
 * @file simulator_config.h
 * Configuration file for bt watch simulator
 *
 */

#ifndef SIMULATOR_CONF_H_
#define SIMULATOR_CONF_H_

#define CONFIG_SIMULATOR 1
#define CONFIG_BOARD_ATS3085S_DEV_WATCH_EXT_NOR 1

/***********************************************
 * general config
 ***********************************************/
/* LVGL */
#define CONFIG_LVGL 1
#define CONFIG_LV_COLOR_DEPTH 16
#define CONFIG_LV_USE_GPU_ACTS 1
#define CONFIG_LV_MEM_CUSTOM 1
#define CONFIG_LVGL_USE_BITMAP_FONT 1
#define CONFIG_LVGL_USE_FREETYPE_FONT 1
#define CONFIG_LVGL_USE_RES_MANAGER 1
#define CONFIG_LVGL_RES_PRELOAD_STACKSIZE 1536
#define CONFIG_LVGL_RES_PRELOAD_PRIORITY  5
//#define CONFIG_LVGL_USE_IMG_DECODER_ACTS 1
#define CONFIG_LVGL_DECODE_CACHE_BUF_SIZE 1

/* VG-Lite */
//#define CONFIG_VG_LITE 1
//#define CONFIG_LV_USE_GPU_ACTS_VG_LITE 1
//#define CONFIG_LV_GPU_ACTS_VG_LITE_SIZE_LIMIT 64
//#define CONFIG_LV_GPU_ACTS_VG_LITE_COMMAND_BUFFER_SIZE 32

/* Image decoder tile cache */
#define CONFIG_TILE_CACHE_NUM 1
#define CONFIG_TILE_BYTES_PER_PIXELS 4
#define CONFIG_TILE_MAX_H 32
#define CONFIG_TILE_MAX_W 32

/* Resource & font manager */
#define CONFIG_RES_MANAGER 1
//#define CONFIG_RES_MANAGER_USE_STYLE_MMAP 1
#define CONFIG_BITMAP_FONT 1
#define CONFIG_BITMAP_FONT_SUPPORT_EMOJI 1
#define CONFIG_EMOJI_FONT_USE_MMAP 1
#define CONFIG_FREETYPE_FONT 1
#define CONFIG_FREETYPE_FONT_BITMAP_BPP 2
#define CONFIG_FREETYPE_FONT_ENABLE_MEMORY_FACE 1
#define CONFIG_FREETYPE_FONT_MAX_FACES 2
#define CONFIG_FREETYPE_FONT_MAX_SIZES 3
//#define CONFIG_FREETYPE_FONT_ENABLE_SVG_PATH 1

/* UI manager */
#define CONFIG_UI_MANAGER 1
#define CONFIG_UI_SERVICE 1
#define CONFIG_UISRV_STACKSIZE 8192
#define CONFIG_UISRV_PRIORITY 1
#define CONFIG_UI_EFFECT_TRANSFORM_BUFFER_COUNT 2
#define CONFIG_UI_VIEW_OVERLAY_OPA 128
#define CONFIG_UI_MEMORY_MANAGER 1
#define CONFIG_SURFACE_DOUBLE_BUFFER 1
#define CONFIG_VIEW_STACK_LEVEL 5

/* Input manager */
#define CONFIG_INPUT_MANAGER 1
#define CONFIG_INPUT_POINTER 1
#define CONFIG_INPUT_KEYPAD 1

/* System */
#define CONFIG_APP_USED_MEM_POOL 1
#define CONFIG_RAM_POOL_PAGE_NUM 14
#define CONFIG_SYS_HEAP_ALLOC_LOOPS 3
#define CONFIG_NUM_MBOX_ASYNC_MSGS 50
#define CONFIG_NUM_COOP_PRIORITIES 15

/* Application */
#define CONFIG_LAUNCHER_APP 1
#define CONFIG_APP_PRIORITY 10
#define CONFIG_APP_STACKSIZE 2048
#define CONFIG_MAIN_STACK_SIZE 2048
#define CONFIG_APP_FAT_DISK "../../udisk/"

#define CONFIG_JPEG_HAL 1
#define CONFIG_JPEG_HW 1
/***********************************************
 * board specific config
 ***********************************************/
#if defined(CONFIG_BOARD_ATS3085S_DEV_WATCH_EXT_NOR)
#include "boards/ats3085s_dev_watch_ext_nor_conf.h"
#else /* default board config */
/* panel config */
#define CONFIG_PANEL_HOR_RES 454
#define CONFIG_PANEL_VER_RES 454
#define CONFIG_PANEL_TIMING_REFRESH_RATE_HZ 60
#define CONFIG_PANEL_ROUND_SHAPE 1
/* psram config */
#define CONFIG_PSRAM_SIZE 8192
/* lvgl config */
#define CONFIG_LV_VDB_NUM 2
#define CONFIG_LV_VDB_SIZE 103058 /* 50% framebuffer */
/* ui mem config */
#define CONFIG_UI_MEM_BLOCK_SIZE 412232
#define CONFIG_UI_MEM_NUMBER_BLOCKS 10
#define CONFIG_UI_GUI_MEM_POOL_SIZE 96
#define CONFIG_UI_RES_MEM_POOL_SIZE 2200000
/* ui decoder */
#define CONFIG_TILE_CACHE_NUM 1
#define CONFIG_TILE_MAX_H 32
#define CONFIG_TILE_MAX_W 32
#define CONFIG_LVGL_DECODE_CACHE_BUF_SIZE 64800
/* font cache config */
#define CONFIG_BITMAP_PER_FONT_CACHE_SIZE  98304
#define CONFIG_BITMAP_FONT_CACHE_POOL_SIZE 300000
#define CONFIG_FREETYPE_PER_FONT_CACHE_SIZE 24576
/* view cache config */
#define CONFIG_VIEW_CACHE_LEVEL 2
#define CONFIG_VIEW_SCROLL_MEM_DEFAULT 1
/* resource path config */
#define CONFIG_APP_UI_DISK "../../boards/ats3085_dvb_watch/res/"
#define CONFIG_APP_FONT_DISK "../../boards/ats3085_dvb_watch/fonts/"
#define CONFIG_WELCOME_UI_DISK "../../boards/ats3085_dvb_watch/sdfs_k/"
#endif /* defined(CONFIG_BOARD_ATS3085_DVB_WATCH) */

#endif /*SIMULATOR_CONF_H_*/
