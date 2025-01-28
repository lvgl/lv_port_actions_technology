/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef DRIVER_PANEL_DEVICE_H__
#define DRIVER_PANEL_DEVICE_H__

/*********************
 *      INCLUDES
 *********************/
#include "panel_common.h"
#include <device.h>
#ifdef CONFIG_SYS_WAKELOCK
#  include <sys_wakelock.h>
#endif

/*********************
 *      DEFINES
 *********************/
#define SUPPORTED_PANEL_PIXEL_FORMATS (PIXEL_FORMAT_BGR_565 | PIXEL_FORMAT_RGB_888 | \
		PIXEL_FORMAT_BGR_888 | PIXEL_FORMAT_ARGB_8888 | PIXEL_FORMAT_XRGB_8888)

/**********************
 *      TYPEDEFS
 **********************/
struct lcd_panel_ops {
	/* all ops return 0 on success else negative code by default */

	/* init sequence after power on */
	int (*init)(const struct device *dev);
	/* detect connected or not */
	int (*detect)(const struct device *dev);
	/* blanking on (display off) */
	int (*blanking_on)(const struct device *dev);
	/* blanking off (display on) */
	int (*blanking_off)(const struct device *dev);
	/* enter low-power state */
	int (*lowpower_enter)(const struct device *dev);
	/* exit low-power state */
	int (*lowpower_exit)(const struct device *dev);
	/* check ESD, executed in ISR context */
	int (*check_esd)(const struct device *dev);
	/* set brightness in Normal mode, range [0, 255] */
	int (*set_brightness)(const struct device *dev, uint8_t brightness);
	/**
	 * set brightness in Lowpower/AOD/Idle mode, range [0, 255];
	 * assinged it only if has AOD (always on display) specific brightness
	 */
	int (*set_aod_brightness)(const struct device *dev, uint8_t brightness);
	/* prepare writing mem with specific area */
	int (*write_prepare)(const struct device *dev, uint16_t x, uint16_t y, uint16_t w, uint16_t h);
};

struct lcd_panel_pincfg {
#ifdef CONFIG_PANEL_POWER_GPIO
	struct gpio_cfg power_cfg;
#endif

#ifdef CONFIG_PANEL_POWER1_GPIO
	struct gpio_cfg power1_cfg;
#endif

#ifdef CONFIG_PANEL_RESET_GPIO
	struct gpio_cfg reset_cfg;
#endif

#ifdef CONFIG_PANEL_TE_GPIO
	struct gpio_cfg te_cfg;
#endif

#ifdef CONFIG_PANEL_BACKLIGHT_PWM
	struct pwm_cfg backlight_cfg;
#elif defined(CONFIG_PANEL_BACKLIGHT_GPIO)
	struct gpio_cfg backlight_cfg;
#endif
};

struct lcd_panel_config {
	struct display_videoport videoport;
	struct display_videomode videomode;

	const struct lcd_panel_ops *ops;
	const void *custom_config;
	void *custom_data;

#if CONFIG_PANEL_PORT_TYPE == DISPLAY_PORT_QSPI_SYNC
	void * fb_mem;      /* framebuffer memory address */
	uint32_t cmd_hsync; /* command mem write's hsync signal */
#else
	uint32_t cmd_ramwc; /* command mem write continue for QSPI */
#endif

	uint32_t cmd_ramwr; /* command mem write start (also vsync signal) for QSPI */

	uint8_t tw_reset; /* reset pulse width in milliseconds during power on sequence */
	uint8_t ts_reset; /* secure reset completion time in milliseconds during power on sequence */
	uint8_t td_slpin; /* reset hold width after SLPIN in milliseconds during power off sequence */
};

struct lcd_panel_data {
	const struct lcd_panel_config *config;
	const struct device *lcdc_dev;
	const struct display_callback *callback;
	struct display_buffer_descriptor refr_desc;

#if CONFIG_PANEL_PORT_TYPE == DISPLAY_PORT_QSPI_SYNC
	const struct device *de_dev;
#endif

#ifdef CONFIG_PANEL_RESET_GPIO
	const struct device *reset_gpio;
#endif

#ifdef CONFIG_PANEL_POWER_GPIO
	const struct device *power_gpio;
#endif

#ifdef CONFIG_PANEL_POWER1_GPIO
	const struct device *power1_gpio;
#endif

#ifdef CONFIG_PANEL_TE_GPIO
	const struct device *te_gpio;
	struct gpio_callback te_gpio_cb;
#else
	struct k_timer te_timer;
#endif

#ifdef CONFIG_PANEL_BACKLIGHT_CTRL
	const struct device *backlight_dev;
#endif

#if CONFIG_PANEL_ESD_CHECK_PERIOD > 0
#if CONFIG_PANEL_PORT_TYPE == DISPLAY_PORT_QSPI_SYNC
	struct k_work suspend_work;
	uint16_t esd_check_cnt;
#else
	struct k_delayed_work esd_check_work;
	uint8_t esd_check_pended : 1;
	uint8_t esd_check_failed : 1;
#endif
#endif /* CONFIG_PANEL_ESD_CHECK_PERIOD > 0 */

	struct k_work resume_work;
#ifdef CONFIG_LCD_WORK_QUEUE
	struct k_work_q pm_workq;
#endif

	uint32_t pm_state;
	uint8_t pm_changing : 1; /* power state changing */
	uint8_t te_active : 1;
	uint8_t __reserved : 6;

	uint8_t disp_on : 1;
	uint8_t in_sleep : 1;
	/* indicate transferring the last part of frame */
	uint8_t transfering_last : 1;
	/* indicate if the first frame has written to panel after blanking off */
	uint8_t first_frame_cplt : 1;

	/* for accessing atomic */
	uint8_t transfering;

	/* the left delay to apply brightness after blanking off */
	uint8_t brightness_delay;

	uint8_t brightness;
	uint8_t pending_brightness;
};

/**********************
 * GLOBAL VARIABLES
 **********************/
/* Normal panels */
extern const struct lcd_panel_config lcd_panel_er76288a_config;
extern const struct lcd_panel_config lcd_panel_ft2308_config;
extern const struct lcd_panel_config lcd_panel_gc9c01_config;
extern const struct lcd_panel_config lcd_panel_icna3310b_config;
extern const struct lcd_panel_config lcd_panel_icna3311_config;
extern const struct lcd_panel_config lcd_panel_rm690b0_config;
extern const struct lcd_panel_config lcd_panel_rm69090_config;
extern const struct lcd_panel_config lcd_panel_sh8601z0_config;
extern const struct lcd_panel_config lcd_panel_st77916_config;
extern const struct lcd_panel_config lcd_panel_st7802_config;
extern const struct lcd_panel_config lcd_panel_jd9853_config;

/* TR panels */
extern const struct lcd_panel_config lcd_panel_lpm015m135a_config;

/* Ram-less panels */
extern const struct lcd_panel_config lcd_panel_axs15231b_config;
extern const struct lcd_panel_config lcd_panel_st77903_config;

/**********************
 * GLOBAL PROTOTYPES
 **********************/
static inline const struct lcd_panel_config *lcd_panel_get_config(const struct device *dev)
{
	struct lcd_panel_data *data = dev->data;
	return data->config;
}

static inline const struct lcd_panel_pincfg *lcd_panel_get_pincfg(const struct device *dev)
{
	return dev->config;
}

static inline void lcd_panel_wake_lock(void)
{
#ifdef CONFIG_SYS_WAKELOCK
	sys_wake_lock_ext(FULL_WAKE_LOCK, DISPLAY_WAKE_LOCK_USER);
#endif
}

static inline void lcd_panel_wake_unlock(void)
{
#ifdef CONFIG_SYS_WAKELOCK
	sys_wake_unlock_ext(FULL_WAKE_LOCK, DISPLAY_WAKE_LOCK_USER);
#endif
}

#endif /* DRIVER_PANEL_DEVICE_H__ */
