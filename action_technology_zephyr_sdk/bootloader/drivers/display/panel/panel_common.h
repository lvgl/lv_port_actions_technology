/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef DRIVER_PANEL_COMMON_H__
#define DRIVER_PANEL_COMMON_H__

/*********************
 *      INCLUDES
 *********************/
#include <soc.h>
#include <board.h>
#include <drivers/display/display_controller.h>
#include <drivers/display/display_engine.h>
#include <drivers/gpio.h>
#include <drivers/pwm.h>
#include <assert.h>

#ifndef CONFIG_MERGE_WORK_Q
/* lcd used ldc work queue default */
#  define CONFIG_LCD_WORK_QUEUE (1)
#  define CONFIG_LCD_WORK_Q_STACK_SIZE (1024)
#endif

#if defined(CONFIG_PANEL_BACKLIGHT_PWM) || defined(CONFIG_PANEL_BACKLIGHT_GPIO)
#  define CONFIG_PANEL_BACKLIGHT_CTRL 1
#endif

/* Horizontal fixed (hardware) offset from the active area by write() */
#ifndef CONFIG_PANEL_FIX_OFFSET_X
#  define CONFIG_PANEL_FIX_OFFSET_X (0)
#endif

/* Vertical fixed (hardware) offset from the active area by write() */
#ifndef CONFIG_PANEL_FIX_OFFSET_Y
#  define CONFIG_PANEL_FIX_OFFSET_Y (0)
#endif

/* Horizontal-Resolution reported by get_capabilities() */
#ifndef CONFIG_PANEL_HOR_RES
#  define CONFIG_PANEL_HOR_RES CONFIG_PANEL_TIMING_HACTIVE
#elif CONFIG_PANEL_HOR_RES > CONFIG_PANEL_TIMING_HACTIVE
#  error "CONFIG_PANEL_HOR_RES must not exceed CONFIG_PANEL_TIMING_HACTIVE"
#endif

/* Vertical-Resolution reported by get_capabilities() */
#ifndef CONFIG_PANEL_VER_RES
#  define CONFIG_PANEL_VER_RES CONFIG_PANEL_TIMING_VACTIVE
#elif CONFIG_PANEL_VER_RES > CONFIG_PANEL_TIMING_VACTIVE
#  error "CONFIG_PANEL_VER_RES must not exceed CONFIG_PANEL_TIMING_VACTIVE"
#endif

/* Horizontal logical offset from the active area by write() */
#ifndef CONFIG_PANEL_OFFSET_X
#  define CONFIG_PANEL_OFFSET_X (0)
#elif CONFIG_PANEL_OFFSET_X + CONFIG_PANEL_HOR_RES > CONFIG_PANEL_TIMING_HACTIVE
#  error "CONFIG_PANEL_OFFSET_X + CONFIG_PANEL_HOR_RES must not exceed CONFIG_PANEL_TIMING_HACTIVE"
#endif

/* Vertical logical offset from the active area by write() */
#ifndef CONFIG_PANEL_OFFSET_Y
#  define CONFIG_PANEL_OFFSET_Y (0)
#elif CONFIG_PANEL_OFFSET_Y + CONFIG_PANEL_VER_RES > CONFIG_PANEL_TIMING_VACTIVE
#  error "CONFIG_PANEL_OFFSET_Y + CONFIG_PANEL_VER_RES must not exceed CONFIG_PANEL_TIMING_VACTIVE"
#endif

/* Horizontal total offset from the active area by write() */
#define CONFIG_PANEL_MEM_OFFSET_X (CONFIG_PANEL_FIX_OFFSET_X + CONFIG_PANEL_OFFSET_X)
/* Vertical total offset from the active area by write() */
#define CONFIG_PANEL_MEM_OFFSET_Y (CONFIG_PANEL_FIX_OFFSET_Y + CONFIG_PANEL_OFFSET_Y)

/* Screen shape: round or square ? */
#ifndef CONFIG_PANEL_ROUND_SHAPE
#  define CONFIG_PANEL_ROUND_SHAPE (0)
#endif

/* Orientatioin: rotation angle [0|90|180|270] */
#ifndef CONFIG_PANEL_ORIENTATION
#  define CONFIG_PANEL_ORIENTATION (0)
#endif

/* Brightness in normal mode, range [0, 255] */
#ifndef CONFIG_PANEL_BRIGHTNESS
#  define CONFIG_PANEL_BRIGHTNESS (255)
#endif

/* Brightness in AOD mode, range [0, 255] */
#ifndef CONFIG_PANEL_AOD_BRIGHTNESS
#  define CONFIG_PANEL_AOD_BRIGHTNESS CONFIG_PANEL_BRIGHTNESS
#endif

/* Delay some TE/vsync periods to apply the brightness after blanking off, range [0, 255] */
#ifndef CONFIG_PANEL_BRIGHTNESS_DELAY_PERIODS
#  define CONFIG_PANEL_BRIGHTNESS_DELAY_PERIODS (0)
#endif

/* ESD check perion in milliseconds */
#ifndef CONFIG_PANEL_ESD_CHECK_PERIOD
#  define CONFIG_PANEL_ESD_CHECK_PERIOD (0)
#endif

/********************************************
 * Configuration of display videoport
 ********************************************/

/* define variable of struct display_videoport */
#ifndef CONFIG_PANEL_PORT_CS
#  define CONFIG_PANEL_PORT_CS (0)
#endif

#ifndef CONFIG_PANEL_PORT_LSB_FIRST
#  define CONFIG_PANEL_PORT_LSB_FIRST (0)
#endif

#ifndef CONFIG_PANEL_PORT_SPI_CPOL
#  define CONFIG_PANEL_PORT_SPI_CPOL (1)
#endif

#ifndef CONFIG_PANEL_PORT_SPI_CPHA
#  define CONFIG_PANEL_PORT_SPI_CPHA (1)
#endif

#ifndef CONFIG_PANEL_PORT_SPI_DCP_MODE
#  define CONFIG_PANEL_PORT_SPI_DCP_MODE (0)
#endif

#ifndef CONFIG_PANEL_PORT_SPI_DUAL_LANE
#  define CONFIG_PANEL_PORT_SPI_DUAL_LANE (1)
#endif

/* read data lane select for QSPI (if exist) */
#ifndef CONFIG_PANEL_PORT_SPI_RD_DATA_LANE
#  define CONFIG_PANEL_PORT_SPI_RD_DATA_LANE (0)
#endif

/* dummy cycles between read command and the following data (if exist) */
#ifndef CONFIG_PANEL_PORT_SPI_RD_DUMMY_CYCLES
#  define CONFIG_PANEL_PORT_SPI_RD_DUMMY_CYCLES (0)
#endif

#ifndef CONFIG_PANEL_PORT_SPI_RD_DELAY_NS
#  define CONFIG_PANEL_PORT_SPI_RD_DELAY_NS (0)
#endif

#ifndef CONFIG_PANEL_PORT_SPI_CSX_DELAY_NS
#  define CONFIG_PANEL_PORT_SPI_CSX_DELAY_NS (0)
#endif

#ifndef CONFIG_PANEL_PORT_SPI_SCL_DELAY_NS
#  define CONFIG_PANEL_PORT_SPI_SCL_DELAY_NS (0)
#endif

#ifndef CONFIG_PANEL_PORT_SPI_WR_D0_DELAY_NS
#  define CONFIG_PANEL_PORT_SPI_WR_D0_DELAY_NS (0)
#endif

#ifndef CONFIG_PANEL_PORT_SPI_WR_D1_DELAY_NS
#  define CONFIG_PANEL_PORT_SPI_WR_D1_DELAY_NS (0)
#endif

#ifndef CONFIG_PANEL_PORT_SPI_WR_D2_DELAY_NS
#  define CONFIG_PANEL_PORT_SPI_WR_D2_DELAY_NS (0)
#endif

#ifndef CONFIG_PANEL_PORT_SPI_WR_D3_DELAY_NS
#  define CONFIG_PANEL_PORT_SPI_WR_D3_DELAY_NS (0)
#endif

/* Possible values: 1, 2, 4, 8 */
#ifndef CONFIG_PANEL_PORT_SPI_AHB_CLK_DIVISION
#  define CONFIG_PANEL_PORT_SPI_AHB_CLK_DIVISION (1)
#endif

#ifndef CONFIG_PANEL_PORT_MCU_CLK_HIGH_DURATION
#  define CONFIG_PANEL_PORT_MCU_CLK_HIGH_DURATION (1)
#endif

#ifndef CONFIG_PANEL_PORT_MCU_CLK_LOW_DURATION
#  define CONFIG_PANEL_PORT_MCU_CLK_LOW_DURATION (1)
#endif

#ifndef CONFIG_PANEL_PORT_BUS_WIDTH
#  define CONFIG_PANEL_PORT_BUS_WIDTH (8)
#endif

/**********************************
 * TR-LCD
 **********************************/
#ifndef CONFIG_PANEL_PORT_TR_LOW_BIT
#  define CONFIG_PANEL_PORT_TR_LOW_BIT (3)
#endif

#ifndef CONFIG_PANEL_PORT_TR_HCK_TAIL
#  define CONFIG_PANEL_PORT_TR_HCK_TAIL (1)
#endif

#ifndef CONFIG_PANEL_PORT_TR_VCK_ON_XRST_LOW
#  define CONFIG_PANEL_PORT_TR_VCK_ON_XRST_LOW (0)
#endif

#ifndef CONFIG_PANEL_PORT_TR_VCK_ON_IDLE
#  define CONFIG_PANEL_PORT_TR_VCK_ON_IDLE (0)
#endif

#ifndef CONFIG_PANEL_PORT_TR_HCK_ON_IDLE
#  define CONFIG_PANEL_PORT_TR_HCK_ON_IDLE (0)
#endif

#ifndef CONFIG_PANEL_PORT_TR_PARTIAL_UPDATE
#  define CONFIG_PANEL_PORT_TR_PARTIAL_UPDATE (0)
#endif

#ifndef CONFIG_PANEL_PORT_TR_FRP
#  define CONFIG_PANEL_PORT_TR_FRP (0)
#endif

#ifndef CONFIG_PANEL_PORT_TR_VCOM_INVERSE
#  define CONFIG_PANEL_PORT_TR_VCOM_INVERSE (0)
#endif

#ifndef CONFIG_PANEL_PORT_TR_FRP_INVERSE
#  define CONFIG_PANEL_PORT_TR_FRP_INVERSE (0)
#endif

#ifndef CONFIG_PANEL_PORT_TR_XFRP_INVERSE
#  define CONFIG_PANEL_PORT_TR_XFRP_INVERSE (0)
#endif

#ifndef CONFIG_PANEL_PORT_TR_XRST_INVERSE
#  define CONFIG_PANEL_PORT_TR_XRST_INVERSE (0)
#endif

#ifndef CONFIG_PANEL_PORT_TR_VST_INVERSE
#  define CONFIG_PANEL_PORT_TR_VST_INVERSE (0)
#endif

#ifndef CONFIG_PANEL_PORT_TR_HST_INVERSE
#  define CONFIG_PANEL_PORT_TR_HST_INVERSE (0)
#endif

#ifndef CONFIG_PANEL_PORT_TR_VCK_INVERSE
#  define CONFIG_PANEL_PORT_TR_VCK_INVERSE (0)
#endif

#ifndef CONFIG_PANEL_PORT_TR_HCK_INVERSE
#  define CONFIG_PANEL_PORT_TR_HCK_INVERSE (0)
#endif

#ifndef CONFIG_PANEL_PORT_TR_ENB_INVERSE
#  define CONFIG_PANEL_PORT_TR_ENB_INVERSE (0)
#endif

#ifndef CONFIG_PANEL_PORT_TR_TW_XRST
#  define CONFIG_PANEL_PORT_TR_TW_XRST (0)
#endif

#ifndef CONFIG_PANEL_PORT_TR_TW_VCOM
#  define CONFIG_PANEL_PORT_TR_TW_VCOM (0)
#endif

#ifndef CONFIG_PANEL_PORT_TR_TD_VST
#  define CONFIG_PANEL_PORT_TR_TD_VST (0)
#endif

#ifndef CONFIG_PANEL_PORT_TR_TW_VST
#  define CONFIG_PANEL_PORT_TR_TW_VST (0)
#endif

#ifndef CONFIG_PANEL_PORT_TR_TD_HST
#  define CONFIG_PANEL_PORT_TR_TD_HST (0)
#endif

#ifndef CONFIG_PANEL_PORT_TR_TW_HST
#  define CONFIG_PANEL_PORT_TR_TW_HST (0)
#endif

#ifndef CONFIG_PANEL_PORT_TR_TD_VCK
#  define CONFIG_PANEL_PORT_TR_TD_VCK (0)
#endif

#ifndef CONFIG_PANEL_PORT_TR_TW_VCK
#  define CONFIG_PANEL_PORT_TR_TW_VCK (0)
#endif

#ifndef CONFIG_PANEL_PORT_TR_TP_HCK
#  define CONFIG_PANEL_PORT_TR_TP_HCK (2)
#endif

#ifndef CONFIG_PANEL_PORT_TR_TD_HCK
#  define CONFIG_PANEL_PORT_TR_TD_HCK (4)
#endif

#ifndef CONFIG_PANEL_PORT_TR_TS_ENB
#  define CONFIG_PANEL_PORT_TR_TS_ENB (0)
#endif

#ifndef CONFIG_PANEL_PORT_TR_TH_ENB
#  define CONFIG_PANEL_PORT_TR_TH_ENB (0)
#endif

#ifndef CONFIG_PANEL_PORT_TR_TD_DATA
#  define CONFIG_PANEL_PORT_TR_TD_DATA (0)
#endif

#ifndef CONFIG_PANEL_PORT_TR_TD_ENB
#  define CONFIG_PANEL_PORT_TR_TD_ENB (0)
#endif

#ifndef CONFIG_PANEL_PORT_TR_TW_ENB
#  define CONFIG_PANEL_PORT_TR_TW_ENB (0)
#endif

#ifndef CONFIG_PANEL_PORT_TR_TSM_ENB
#  define CONFIG_PANEL_PORT_TR_TSM_ENB (3)
#endif

#ifndef CONFIG_PANEL_PORT_TR_THM_ENB
#  define CONFIG_PANEL_PORT_TR_THM_ENB (0)
#endif

#ifndef CONFIG_PANEL_PORT_TR_TWM_VCK
#  define CONFIG_PANEL_PORT_TR_TWM_VCK (1)
#endif

/********************************************
 * structure display_videoport initializer
 ********************************************/

#define IS_MCU_PANEL (DISPLAY_PORT_TYPE_MAJOR(CONFIG_PANEL_PORT_TYPE) == DISPLAY_PORT_MCU)
#define IS_SPI_PANEL (DISPLAY_PORT_TYPE_MAJOR(CONFIG_PANEL_PORT_TYPE) == DISPLAY_PORT_SPI)
#define IS_QSPI_SYNC_PANEL (CONFIG_PANEL_PORT_TYPE == DISPLAY_PORT_QSPI_SYNC)
#define IS_TR_PANEL  (DISPLAY_PORT_TYPE_MAJOR(CONFIG_PANEL_PORT_TYPE) == DISPLAY_PORT_TR)

#if IS_MCU_PANEL
#define PANEL_VIDEO_PORT_INITIALIZER \
	{ \
		.type = CONFIG_PANEL_PORT_TYPE, \
		.mcu_mode = { \
			.cs = CONFIG_PANEL_PORT_CS, \
			.lsb_first = CONFIG_PANEL_PORT_LSB_FIRST, \
			.bus_width = CONFIG_PANEL_PORT_BUS_WIDTH, \
			.clk_high_duration = CONFIG_PANEL_PORT_MCU_CLK_HIGH_DURATION, \
			.clk_low_duration = CONFIG_PANEL_PORT_MCU_CLK_LOW_DURATION, \
		}, \
	}

#elif IS_SPI_PANEL
#define PANEL_VIDEO_PORT_INITIALIZER \
	{ \
		.type = CONFIG_PANEL_PORT_TYPE, \
		.spi_mode = { \
			.cs = CONFIG_PANEL_PORT_CS, \
			.lsb_first = CONFIG_PANEL_PORT_LSB_FIRST, \
			.cpol = CONFIG_PANEL_PORT_SPI_CPOL, \
			.cpha = CONFIG_PANEL_PORT_SPI_CPHA, \
			.dual_lane = CONFIG_PANEL_PORT_SPI_DUAL_LANE, \
			.dcp_mode = CONFIG_PANEL_PORT_SPI_DCP_MODE, \
			.rd_lane = CONFIG_PANEL_PORT_SPI_RD_DATA_LANE, \
			.rd_dummy_cycles = CONFIG_PANEL_PORT_SPI_RD_DUMMY_CYCLES, \
			.rd_delay_ns = CONFIG_PANEL_PORT_SPI_RD_DELAY_NS, \
			.wr_delay_d0_ns = CONFIG_PANEL_PORT_SPI_WR_D0_DELAY_NS, \
			.wr_delay_d1_ns = CONFIG_PANEL_PORT_SPI_WR_D1_DELAY_NS, \
			.wr_delay_d2_ns = CONFIG_PANEL_PORT_SPI_WR_D2_DELAY_NS, \
			.wr_delay_d3_ns = CONFIG_PANEL_PORT_SPI_WR_D3_DELAY_NS, \
			.delay_csx_ns = CONFIG_PANEL_PORT_SPI_CSX_DELAY_NS, \
			.delay_scl_ns = CONFIG_PANEL_PORT_SPI_SCL_DELAY_NS, \
			.ahb_clk_div = CONFIG_PANEL_PORT_SPI_AHB_CLK_DIVISION, \
		}, \
	}

#elif IS_TR_PANEL
#define PANEL_VIDEO_PORT_INITIALIZER \
	{ \
		.type = CONFIG_PANEL_PORT_TYPE, \
		.tr_mode = { \
			.low_bit = CONFIG_PANEL_PORT_TR_LOW_BIT, \
			.hck_tail_on = CONFIG_PANEL_PORT_TR_HCK_TAIL, \
			.vck_on_xrstl = CONFIG_PANEL_PORT_TR_VCK_ON_XRST_LOW, \
			.vck_on_idle = CONFIG_PANEL_PORT_TR_VCK_ON_IDLE, \
			.hck_on_idle = CONFIG_PANEL_PORT_TR_HCK_ON_IDLE, \
			.ptl_on = CONFIG_PANEL_PORT_TR_PARTIAL_UPDATE, \
			.frp_on = CONFIG_PANEL_PORT_TR_FRP, \
			.vcom_inv = CONFIG_PANEL_PORT_TR_VCOM_INVERSE, \
			.frp_inv = CONFIG_PANEL_PORT_TR_FRP_INVERSE, \
			.xfrp_inv = CONFIG_PANEL_PORT_TR_XFRP_INVERSE, \
			.xrst_inv = CONFIG_PANEL_PORT_TR_XRST_INVERSE, \
			.vst_inv = CONFIG_PANEL_PORT_TR_VST_INVERSE, \
			.hst_inv = CONFIG_PANEL_PORT_TR_HST_INVERSE, \
			.vck_inv = CONFIG_PANEL_PORT_TR_VCK_INVERSE, \
			.hck_inv = CONFIG_PANEL_PORT_TR_HCK_INVERSE, \
			.enb_inv = CONFIG_PANEL_PORT_TR_ENB_INVERSE, \
			.tw_xrst = CONFIG_PANEL_PORT_TR_TW_XRST, \
			.tw_vcom = CONFIG_PANEL_PORT_TR_TW_VCOM, \
			.td_vst = CONFIG_PANEL_PORT_TR_TD_VST, \
			.tw_vst = CONFIG_PANEL_PORT_TR_TW_VST, \
			.td_hst = CONFIG_PANEL_PORT_TR_TD_HST, \
			.tw_hst = CONFIG_PANEL_PORT_TR_TW_HST, \
			.td_vck = CONFIG_PANEL_PORT_TR_TD_VCK, \
			.tw_vck = CONFIG_PANEL_PORT_TR_TW_VCK, \
			.tp_hck = CONFIG_PANEL_PORT_TR_TP_HCK, \
			.td_hck = CONFIG_PANEL_PORT_TR_TD_HCK, \
			.ts_enb = CONFIG_PANEL_PORT_TR_TS_ENB, \
			.th_enb = CONFIG_PANEL_PORT_TR_TH_ENB, \
			.td_data = CONFIG_PANEL_PORT_TR_TD_DATA, \
			.td_enb = CONFIG_PANEL_PORT_TR_TD_ENB, \
			.tw_enb = CONFIG_PANEL_PORT_TR_TW_ENB, \
			.tsm_enb = CONFIG_PANEL_PORT_TR_TSM_ENB, \
			.thm_enb = CONFIG_PANEL_PORT_TR_THM_ENB, \
			.twm_vck = CONFIG_PANEL_PORT_TR_TWM_VCK, \
		}, \
	}
#else
#error "invalid port type."
#endif

#define PANEL_VIDEO_PORT_DEFINE(name) \
	struct display_videoport name = PANEL_VIDEO_PORT_INITIALIZER

/********************************************
 * Configuration of display videomode
 ********************************************/

#ifndef CONFIG_PANEL_COLOR_DEPTH
#  define CONFIG_PANEL_COLOR_DEPTH 16
#endif

#if CONFIG_PANEL_COLOR_DEPTH == 32
#  define CONFIG_PANEL_PIXEL_FORMAT PIXEL_FORMAT_ARGB_8888
#elif CONFIG_PANEL_COLOR_DEPTH == 24
#  define CONFIG_PANEL_PIXEL_FORMAT PIXEL_FORMAT_RGB_888
#else
#  define CONFIG_PANEL_PIXEL_FORMAT PIXEL_FORMAT_BGR_565
#endif

#ifndef CONFIG_PANEL_TIMING_REFRESH_RATE_HZ
#  define CONFIG_PANEL_TIMING_REFRESH_RATE_HZ (60)
#endif

#define CONFIG_PANEL_VSYNC_PERIOD_US \
	((1000000 + CONFIG_PANEL_TIMING_REFRESH_RATE_HZ - 1) / CONFIG_PANEL_TIMING_REFRESH_RATE_HZ)

#define CONFIG_PANEL_VSYNC_PERIOD_MS \
	((1000 + CONFIG_PANEL_TIMING_REFRESH_RATE_HZ - 1) / CONFIG_PANEL_TIMING_REFRESH_RATE_HZ)

#ifndef CONFIG_PANEL_TIMING_PIXEL_CLK_KHZ
#  define CONFIG_PANEL_TIMING_PIXEL_CLK_KHZ (50000)
#endif

/* measured in pixels */
#ifndef CONFIG_PANEL_TIMING_HACTIVE
#  define CONFIG_PANEL_TIMING_HACTIVE (1)
#endif

/* measured in clock cycles */
#ifndef CONFIG_PANEL_TIMING_HFRONT_PORCH
#  define CONFIG_PANEL_TIMING_HFRONT_PORCH (0)
#endif

/* measured in clock cycles */
#ifndef CONFIG_PANEL_TIMING_HBACK_PORCH
#  define CONFIG_PANEL_TIMING_HBACK_PORCH (0)
#endif

/* measured in clock cycles */
#ifndef CONFIG_PANEL_TIMING_HSYNC_LEN
#  define CONFIG_PANEL_TIMING_HSYNC_LEN (0)
#endif

/* measured in lines */
#ifndef CONFIG_PANEL_TIMING_VACTIVE
#  define CONFIG_PANEL_TIMING_VACTIVE (1)
#endif

/* measured in lines */
#ifndef CONFIG_PANEL_TIMING_VFRONT_PORCH
#  define CONFIG_PANEL_TIMING_VFRONT_PORCH (0)
#endif

/* measured in lines */
#ifndef CONFIG_PANEL_TIMING_VBACK_PORCH
#  define CONFIG_PANEL_TIMING_VBACK_PORCH (0)
#endif

/* measured in lines */
#ifndef CONFIG_PANEL_TIMING_VSYNC_LEN
#  define CONFIG_PANEL_TIMING_VSYNC_LEN (0)
#endif

#ifdef CONFIG_PANEL_TIMING_TE_ACTIVE
#  define PANEL_VIDEO_MODE_TE_FLAGS ((CONFIG_PANEL_TIMING_TE_ACTIVE) ? DISPLAY_FLAGS_TE_HIGH : DISPLAY_FLAGS_TE_LOW)
#else
#  define PANEL_VIDEO_MODE_TE_FLAGS (0)
#endif

#ifdef CONFIG_PANEL_TIMING_HSYNC_ACTIVE
#  define PANEL_VIDEO_MODE_HSYNC_FLAGS ((CONFIG_PANEL_TIMING_HSYNC_ACTIVE) ? DISPLAY_FLAGS_HSYNC_HIGH : DISPLAY_FLAGS_HSYNC_LOW)
#else
#  define PANEL_VIDEO_MODE_HSYNC_FLAGS (0)
#endif

#ifdef CONFIG_PANEL_TIMING_VSYNC_ACTIVE
#  define PANEL_VIDEO_MODE_VSYNC_FLAGS ((CONFIG_PANEL_TIMING_VSYNC_ACTIVE) ? DISPLAY_FLAGS_VSYNC_HIGH : DISPLAY_FLAGS_VSYNC_LOW)
#else
#  define PANEL_VIDEO_MODE_VSYNC_FLAGS (0)
#endif

#ifdef CONFIG_PANEL_TIMING_DE_ACTIVE
#  define PANEL_VIDEO_MODE_DE_FLAGS ((CONFIG_PANEL_TIMING_DE_ACTIVE) ? DISPLAY_FLAGS_DE_HIGH : DISPLAY_FLAGS_DE_LOW)
#else
#  define PANEL_VIDEO_MODE_DE_FLAGS (0)
#endif

#ifdef CONFIG_PANEL_TIMING_PIXELCLK_ACTIVE
#  define PANEL_VIDEO_MODE_PIXELCLK_FLAGS ((CONFIG_PANEL_TIMING_PIXELCLK_ACTIVE) ? DISPLAY_FLAGS_PIXDATA_NEGEDGE : DISPLAY_FLAGS_PIXDATA_POSEDGE)
#else
#  define PANEL_VIDEO_MODE_PIXELCLK_FLAGS (0)
#endif

#ifdef CONFIG_PANEL_TIMING_SYNCCLK_ACTIVE
#  define PANEL_VIDEO_MODE_SYNCCLK_FLAGS ((CONFIG_PANEL_TIMING_SYNCCLK_ACTIVE) ? DISPLAY_FLAGS_SYNC_NEGEDGE : DISPLAY_FLAGS_SYNC_POSEDGE)
#else
#  define PANEL_VIDEO_MODE_SYNCCLK_FLAGS (0)
#endif

/********************************************
 * structure display_videomode initializer
 ********************************************/

#define PANEL_VIDEO_MODE_INITIALIZER \
	{ \
		.pixel_format = CONFIG_PANEL_PIXEL_FORMAT, \
		.pixel_clk = CONFIG_PANEL_TIMING_PIXEL_CLK_KHZ, \
		.refresh_rate = CONFIG_PANEL_TIMING_REFRESH_RATE_HZ, \
		.hactive = CONFIG_PANEL_TIMING_HACTIVE, \
		.hfront_porch = CONFIG_PANEL_TIMING_HFRONT_PORCH, \
		.hback_porch = CONFIG_PANEL_TIMING_HBACK_PORCH, \
		.hsync_len = CONFIG_PANEL_TIMING_HSYNC_LEN, \
		.vactive = CONFIG_PANEL_TIMING_VACTIVE, \
		.vfront_porch = CONFIG_PANEL_TIMING_VFRONT_PORCH, \
		.vback_porch = CONFIG_PANEL_TIMING_VBACK_PORCH, \
		.vsync_len = CONFIG_PANEL_TIMING_VSYNC_LEN, \
		.flags = PANEL_VIDEO_MODE_TE_FLAGS	 | \
				PANEL_VIDEO_MODE_HSYNC_FLAGS | \
				PANEL_VIDEO_MODE_VSYNC_FLAGS | \
				PANEL_VIDEO_MODE_DE_FLAGS	 | \
				PANEL_VIDEO_MODE_PIXELCLK_FLAGS | \
				PANEL_VIDEO_MODE_SYNCCLK_FLAGS, \
	}

#define PANEL_VIDEO_MODE_DEFINE(name) \
	struct display_videomode name = PANEL_VIDEO_MODE_INITIALIZER

#endif /* DRIVER_PANEL_COMMON_H__ */
