/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <tracing/tracing.h>
#include "panel_device.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(lcd_panel, CONFIG_DISPLAY_LOG_LEVEL);

/*********************
 *      DEFINES
 *********************/

/* time to wait between pixel transfer and next command */
#ifdef CONFIG_PANEL_FULL_SCREEN_OPT_AREA
#  define CONFIG_PANEL_PIXEL_WR_DELAY_US  (0)
#else
#  define CONFIG_PANEL_PIXEL_WR_DELAY_US  (0)
#endif

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

static int _lcd_panel_set_orientation(const struct device *dev,
				const enum display_orientation orientation);

static int _panel_blanking_on(const struct device *dev);
static int _panel_blanking_off(const struct device *dev);

static int _panel_te_set_enable(const struct device *dev, bool enabled);
static void _panel_submit_esd_check_work(struct lcd_panel_data *data);
static void _panel_cancel_esd_check_work(struct lcd_panel_data *data);

static int _panel_pm_early_suspend(const struct device *dev, bool in_turnoff);
static int _panel_pm_late_resume(const struct device *dev);
static void _panel_pm_resume_handler(struct k_work *work);

/**********************
 *  STATIC VARIABLES
 **********************/
#ifdef CONFIG_LCD_WORK_QUEUE
K_THREAD_STACK_DEFINE(pm_workq_stack, CONFIG_LCD_WORK_Q_STACK_SIZE);
#endif

static const struct lcd_panel_config *const lcd_panel_configs[] = {
#ifdef CONFIG_PANEL_ER76288A
	&lcd_panel_er76288a_config,
#endif
#ifdef CONFIG_PANEL_FT2308
	&lcd_panel_ft2308_config,
#endif
#ifdef CONFIG_PANEL_GC9C01
	&lcd_panel_gc9c01_config,
#endif
#ifdef CONFIG_PANEL_ICNA3310B
	&lcd_panel_icna3310b_config,
#endif
#ifdef CONFIG_PANEL_ICNA3311
	&lcd_panel_icna3311_config,
#endif
#ifdef CONFIG_PANEL_RM690B0
	&lcd_panel_rm690b0_config,
#endif
#ifdef CONFIG_PANEL_RM69090
	&lcd_panel_rm69090_config,
#endif
#ifdef CONFIG_PANEL_SH8601Z0
	&lcd_panel_sh8601z0_config,
#endif
#ifdef CONFIG_PANEL_ST77916
	&lcd_panel_st77916_config,
#endif
#ifdef CONFIG_PANEL_ST7802
	&lcd_panel_st7802_config,
#endif
#ifdef CONFIG_PANEL_JD9853
	&lcd_panel_jd9853_config,
#endif

	/* TR-panels */
#ifdef CONFIG_PANEL_LPM015M135A
	&lcd_panel_lpm015m135a_config,
#endif
};

/**********************
 *      MACROS
 **********************/

/**********************
 * STATIC FUNCTIONS
 **********************/

#if IS_TR_PANEL
static inline void _panel_reset_pin_set(const struct device *dev, bool active)
{
#ifdef CONFIG_PANEL_RESET_GPIO
	struct lcd_panel_data *data = dev->data;
	const struct lcd_panel_pincfg *pincfg = dev->config;
	gpio_pin_set(data->reset_gpio, pincfg->reset_cfg.gpion, active ? 1 : 0);
#endif
}
#endif /* IS_TR_PANEL */

static void _panel_reset(const struct device *dev)
{
	struct lcd_panel_data *data = dev->data;
	const struct lcd_panel_config *config = data->config;
	const struct lcd_panel_pincfg *pincfg = dev->config;

	display_controller_enable(data->lcdc_dev, &config->videoport);
	display_controller_set_mode(data->lcdc_dev, &config->videomode);

	LOG_DBG("Resetting display");

#ifdef CONFIG_PANEL_RESET_GPIO
	gpio_pin_set(data->reset_gpio, pincfg->reset_cfg.gpion, 1);
#endif

#ifdef CONFIG_PANEL_POWER_GPIO
	gpio_pin_set(data->power_gpio, pincfg->power_cfg.gpion, 1);
#endif

#ifdef CONFIG_PANEL_POWER1_GPIO
	gpio_pin_set(data->power1_gpio, pincfg->power1_cfg.gpion, 1);
#endif

#if IS_TR_PANEL == 0
#ifdef CONFIG_PANEL_RESET_GPIO
	k_msleep(config->tw_reset);
	gpio_pin_set(data->reset_gpio, pincfg->reset_cfg.gpion, 0);
	k_msleep(config->ts_reset);
#endif
#endif /* IS_TR_PANEL == 0 */
}

static int _panel_detect(const struct device *dev)
{
	struct lcd_panel_data *data = dev->data;

	if (0 == ARRAY_SIZE(lcd_panel_configs))
		return -ENODEV;

	if (1 == ARRAY_SIZE(lcd_panel_configs)) {
		data->config = lcd_panel_configs[0];
		return 0;
	}

	for (int i = 0; i < ARRAY_SIZE(lcd_panel_configs); i++) {
		data->config = lcd_panel_configs[i];
		if (data->config->ops->detect == NULL)
			return 0;

		_panel_reset(dev);
		if (!data->config->ops->detect(dev))
			return 0;
	}

	/* FIXME: fallback to the frist panel */
	data->config = lcd_panel_configs[0];

	return 0;
}

static void _panel_power_on(const struct device *dev)
{
	struct lcd_panel_data *data = dev->data;
	const struct lcd_panel_config *config = data->config;

	_panel_reset(dev);

	if (config->ops->init)
		config->ops->init(dev);

	/* Display On */
	_panel_blanking_off(dev);
}

static void _panel_power_off(const struct device *dev, bool in_turnoff)
{
	struct lcd_panel_data *data = dev->data;
	const struct lcd_panel_config *config = data->config;
	const struct lcd_panel_pincfg *pincfg = dev->config;

	_panel_blanking_on(dev);

	/* FIXME: place it in config->ops->blanking_on() ? */
	if (config->td_slpin > 0) {
		if (in_turnoff) {
			k_busy_wait(config->td_slpin * 1000u);
		} else {
			k_msleep(config->td_slpin);
		}
	}

#ifdef CONFIG_PANEL_RESET_GPIO
	gpio_pin_set(data->reset_gpio, pincfg->reset_cfg.gpion, 1);
#endif
#ifdef CONFIG_PANEL_POWER_GPIO
	gpio_pin_set(data->power_gpio, pincfg->power_cfg.gpion, 0);
#endif
#ifdef CONFIG_PANEL_POWER1_GPIO
	gpio_pin_set(data->power1_gpio, pincfg->power1_cfg.gpion, 0);
#endif
}

static void _panel_apply_brightness(const struct device *dev, uint8_t brightness)
{
	struct lcd_panel_data *data = dev->data;
	const struct lcd_panel_config *config = data->config;
	bool ready;

	if (data->brightness == brightness)
		return;

	ready = (data->brightness_delay == 0) && (data->first_frame_cplt > 0);
	if (!ready) {
		data->brightness_delay -= data->first_frame_cplt;
	}

	if (ready || brightness == 0) {
		if (config->ops->set_brightness) {
			config->ops->set_brightness(dev, brightness);
		} else {
#ifdef CONFIG_PANEL_BACKLIGHT_CTRL
			const struct lcd_panel_pincfg *pincfg = dev->config;

#ifdef CONFIG_PANEL_BACKLIGHT_PWM
			pwm_pin_set_cycles(data->backlight_dev, pincfg->backlight_cfg.chan, pincfg->backlight_cfg.period,
						(uint32_t)brightness * pincfg->backlight_cfg.period / 255, pincfg->backlight_cfg.flag);
#else
			gpio_pin_set(data->backlight_dev, pincfg->backlight_cfg.gpion, brightness ? 1 : 0);
#endif
#endif /* CONFIG_PANEL_BACKLIGHT_CTRL */
		}

		data->brightness = brightness;
	}
}

static void _panel_apply_post_change(const struct device *dev)
{
	struct lcd_panel_data *data = dev->data;

	_panel_apply_brightness(dev, data->pending_brightness);
}

/**********************
 *  DEVICE DECLARATION
 **********************/
DEVICE_DECLARE(lcd_panel);

/**********************
 *  ESD CHECK
 **********************/
#if CONFIG_PANEL_ESD_CHECK_PERIOD > 0
static void _panel_esd_check_custom(const struct device *dev)
{
	struct lcd_panel_data *data = dev->data;
	const struct lcd_panel_config *config = data->config;

	if (config->ops->check_esd) {
		data->esd_check_pended = data->transfering;
		if (!data->transfering) {
			int res = config->ops->check_esd(dev);
			data->esd_check_failed = res ? 1 : 0;
		}
	}
}

static void _panel_esd_check_handler(struct k_work *work)
{
	const struct device *dev = DEVICE_GET(lcd_panel);
	struct lcd_panel_data *data = dev->data;
	const struct lcd_panel_config *config = data->config;

	if (config->ops->check_esd) {
		unsigned int key = irq_lock();
		_panel_esd_check_custom(dev);
		irq_unlock(key);
	}

	if (data->esd_check_failed) {
		LOG_ERR("ESD check fail, reboot panel");
		goto fail_exit;
	}

	if (!data->te_active) {
		LOG_ERR("TE no signal, reboot panel");
		goto fail_exit;
	}

	data->te_active = 0;
	_panel_submit_esd_check_work(data);
	return;
fail_exit:
	lcd_panel_wake_lock();
	_panel_pm_early_suspend(dev, false);
	_panel_pm_late_resume(dev);
	lcd_panel_wake_unlock();
}
#endif /* CONFIG_TPKEY_ESD_CHECK_PERIOD > 0 */

static void _panel_submit_esd_check_work(struct lcd_panel_data *data)
{
#if CONFIG_PANEL_ESD_CHECK_PERIOD > 0
#ifdef CONFIG_LCD_WORK_QUEUE
	k_delayed_work_submit_to_queue(&data->pm_workq, &data->esd_check_work,
				       K_MSEC(CONFIG_PANEL_ESD_CHECK_PERIOD));
#else
	k_delayed_work_submit(&data->esd_check_work, K_MSEC(CONFIG_PANEL_ESD_CHECK_PERIOD));
#endif
#endif /* CONFIG_PANEL_ESD_CHECK_PERIOD > 0 */
}

static void _panel_cancel_esd_check_work(struct lcd_panel_data *data)
{
#if CONFIG_PANEL_ESD_CHECK_PERIOD > 0
	data->esd_check_pended = 0;
	data->esd_check_failed = 0;
	k_delayed_work_cancel(&data->esd_check_work);
#endif /* CONFIG_PANEL_ESD_CHECK_PERIOD > 0 */
}

/**********************
 *  TE CALLBACK
 **********************/
static void _panel_te_handler(void)
{
	const struct device *dev = DEVICE_GET(lcd_panel);
	struct lcd_panel_data *data = dev->data;
	uint32_t timestamp = k_cycle_get_32();

	sys_trace_void(SYS_TRACE_ID_VSYNC);

	data->te_active = 1;

	if (data->transfering) {
		sys_trace_void(SYS_TRACE_ID_LCD_POST_OVERTIME);
	} else {
		_panel_apply_post_change(dev);

#if CONFIG_PANEL_ESD_CHECK_PERIOD > 0
		if (data->esd_check_pended)
			_panel_esd_check_custom(dev);
#endif
	}

	if (data->callback && data->callback->vsync) {
		data->callback->vsync(data->callback, timestamp);
	}
}

#ifdef CONFIG_PANEL_TE_GPIO
static void _panel_te_gpio_handler(const struct device *port,
		struct gpio_callback *cb, gpio_port_pins_t pins)
{
	_panel_te_handler();
}

static int _panel_te_set_enable(const struct device *dev, bool enabled)
{
	struct lcd_panel_data *data = dev->data;
	const struct lcd_panel_pincfg *pincfg = dev->config;

	if (gpio_pin_interrupt_configure(data->te_gpio, pincfg->te_cfg.gpion,
				enabled ? GPIO_INT_EDGE_TO_ACTIVE : GPIO_INT_DISABLE)) {
		LOG_ERR("Couldn't config te interrupt (en=%d)", enabled);
		return -ENODEV;
	}

	return 0;
}

#else  /* CONFIG_PANEL_TE_GPIO */

static void _panel_te_timer_handler(struct k_timer *timer)
{
	_panel_te_handler();
}

static int _panel_te_set_enable(const struct device *dev, bool enabled)
{
	struct lcd_panel_data *data = dev->data;

	if (enabled) {
		k_timer_start(&data->te_timer, K_MSEC(0), K_MSEC(CONFIG_PANEL_VSYNC_PERIOD_MS));
	} else {
		k_timer_stop(&data->te_timer);
	}

	return 0;
}

#endif /* CONFIG_PANEL_TE_GPIO */

/**********************
 *  DE/LCDC CALLBACK
 **********************/
static void _panel_prepare_de_transfer(void *arg, const display_rect_t *area)
{
	const struct device *dev = arg;
	struct lcd_panel_data *data = dev->data;
	const struct lcd_panel_config *config = data->config;
	uint8_t bytes_per_pixel;

	assert(data->disp_on == 1 && data->transfering == 0);

	sys_trace_u32x4(SYS_TRACE_ID_LCD_POST,
			area->x, area->y, area->x + area->w - 1, area->y + area->h - 1);

	if (data->pm_state == PM_DEVICE_STATE_LOW_POWER)
		board_lcd_resume(true, false);

	if (config->ops->write_prepare) {
		int res = config->ops->write_prepare(dev, area->x, area->y, area->w, area->h);
		if (res) {
			LOG_ERR("write area (%u, %u, %u, %u) failed", area->x, area->y, area->w, area->h);
		}
	}

	if (config->videomode.pixel_format == PIXEL_FORMAT_BGR_565) {
		bytes_per_pixel = 2;
	} else if (config->videomode.pixel_format == PIXEL_FORMAT_RGB_888) {
		bytes_per_pixel = 3;
	} else {
		bytes_per_pixel = 4;
	}

	data->refr_desc.pixel_format = config->videomode.pixel_format;
	data->refr_desc.width = area->w;
	data->refr_desc.height = area->h;
	data->refr_desc.pitch = area->w * bytes_per_pixel;

	data->transfering = 1;
	data->transfering_last = (area->y + area->h >= CONFIG_PANEL_VER_RES);

	/* source from DE */
	display_controller_set_source(data->lcdc_dev, DISPLAY_CONTROLLER_SOURCE_ENGINE, NULL);
}

static void _panel_start_de_transfer(void *arg)
{
	const struct device *dev = arg;
	struct lcd_panel_data *data = dev->data;
	const struct lcd_panel_config *config = data->config;
	int res;

#if IS_TR_PANEL
	_panel_reset_pin_set(dev, false);
#endif

	res = display_controller_write_pixels(data->lcdc_dev, config->cmd_ramwr, DC_INVALID_CMD,
					      &data->refr_desc, NULL);
	if (res) {
		LOG_ERR("write pixels failed");
		data->transfering = 0;
	}
}

static void _panel_complete_transfer(void *arg)
{
	const struct device *dev = arg;
	struct lcd_panel_data *data = dev->data;

#if IS_TR_PANEL
	_panel_reset_pin_set(dev, true);
#endif

	if (data->pm_state == PM_DEVICE_STATE_LOW_POWER)
		board_lcd_suspend(true, false);

	sys_trace_end_call(SYS_TRACE_ID_LCD_POST);

	data->transfering = 0;
	if (data->transfering_last) {
		data->first_frame_cplt = 1;
	}

#if CONFIG_PANEL_PIXEL_WR_DELAY_US > 0
	if (data->transfering_last == 0) {
		k_busy_wait(CONFIG_PANEL_PIXEL_WR_DELAY_US);
	}
#endif

	if (data->callback && data->callback->complete) {
		data->callback->complete(data->callback);
	}
}

static int _panel_blanking_on(const struct device *dev)
{
	struct lcd_panel_data *data = dev->data;
	const struct lcd_panel_config *config = data->config;

	if (data->disp_on == 0)
		return 0;

	LOG_INF("display blanking on");
	data->disp_on = 0;
	data->in_sleep = 1;
	data->first_frame_cplt = 0;
	data->brightness_delay = CONFIG_PANEL_BRIGHTNESS_DELAY_PERIODS;

	_panel_te_set_enable(dev, false);
	_panel_apply_brightness(dev, 0);

	if (config->ops->blanking_on)
		config->ops->blanking_on(dev);

	display_controller_disable(data->lcdc_dev);
	return 0;
}

static int _panel_blanking_off(const struct device *dev)
{
	struct lcd_panel_data *data = dev->data;
	const struct lcd_panel_config *config = data->config;

	if (data->disp_on == 1)
		return 0;

	LOG_INF("display blanking off");
	display_controller_enable(data->lcdc_dev, &config->videoport);
	display_controller_set_mode(data->lcdc_dev, &config->videomode);

	data->disp_on = 1;

	if (config->ops->blanking_off)
		config->ops->blanking_off(dev);

	_panel_te_set_enable(dev, true);

	return 0;
}

/**********************
 *  DEVICE API
 **********************/
static int _lcd_panel_blanking_on(const struct device *dev)
{
	struct lcd_panel_data *data = dev->data;
	int timeout;

	for (timeout = 2000; timeout > 0; timeout--) {
		if (data->pm_changing == 0)
			return _panel_pm_early_suspend(dev, false);

		k_msleep(1);
	}

	return -ETIMEDOUT;
}

static int _lcd_panel_blanking_off(const struct device *dev)
{
	struct lcd_panel_data *data = dev->data;
	int timeout;

	for (timeout = 2000; data->pm_changing && timeout > 0; timeout--) {
		k_msleep(1);
	}

	if (data->pm_changing)
		return -ETIMEDOUT;

	if (_panel_pm_late_resume(dev))
		return -EIO;

	for (timeout = 2000; data->pm_changing && timeout > 0; timeout--) {
		k_msleep(1);
	}

	return data->pm_changing ? -ETIMEDOUT : 0;
}

static int _lcd_panel_read(const struct device *dev,
			const uint16_t x,
			const uint16_t y,
			const struct display_buffer_descriptor *desc,
			void *buf)
{
	return -ENOTSUP;
}

static int _lcd_panel_write(const struct device *dev,
			 const uint16_t x,
			 const uint16_t y,
			 const struct display_buffer_descriptor *desc,
			 const void *buf)
{
	struct lcd_panel_data *data = dev->data;
	const struct lcd_panel_config *config = data->config;
	int res;

#if IS_TR_PANEL
	if (desc->width != CONFIG_PANEL_HOR_RES || desc->height != CONFIG_PANEL_VER_RES) {
		LOG_ERR("TR panel requires full-screen write");
		return -EINVAL;
	}
#endif

	if (data->disp_on == 0 || data->transfering != 0) {
		LOG_ERR("disp not ready");
		return -EBUSY;
	}

	sys_trace_u32x4(SYS_TRACE_ID_LCD_POST,
			x, y, x + desc->width - 1, y + desc->height - 1);

	if (data->pm_state == PM_DEVICE_STATE_LOW_POWER)
		board_lcd_resume(true, false);

	if (config->ops->write_prepare) {
		res = config->ops->write_prepare(dev, x, y, desc->width, desc->height);
		if (res) {
			LOG_ERR("write area (%u, %u, %u, %u) failed", x, y, desc->width, desc->height);
			return res;
		}
	}

	data->transfering = 1;
	data->transfering_last = (y + desc->height >= CONFIG_PANEL_VER_RES);

#if IS_TR_PANEL
	_panel_reset_pin_set(dev, false);
#endif

	/**
	 * CARE:
	 * For QSPI (Sync) series interface, RAMWR cmd will send automatically by LCDC.
	 * For other interfaces, RAMWR cmd must send in write_prepare() ops manually.
	 */
#if 0
	_panel_transmit(dev, DDIC_CMD_RAMWR, NULL, 0);

	display_controller_set_source(data->lcdc_dev, DISPLAY_CONTROLLER_SOURCE_MCU, NULL);
	res = display_controller_write_pixels(data->lcdc_dev,
			config->cmd_ramwc, DC_INVALID_CMD, desc, buf);
#else
	display_controller_set_source(data->lcdc_dev, DISPLAY_CONTROLLER_SOURCE_DMA, NULL);
	res = display_controller_write_pixels(data->lcdc_dev, config->cmd_ramwr,
					DC_INVALID_CMD, desc, buf);
#endif

	if (res) {
		LOG_ERR("write pixels failed");
		data->transfering = 0;
	}

	return res;
}

static void *_lcd_panel_get_framebuffer(const struct device *dev)
{
	return NULL;
}

static int _lcd_panel_set_brightness(const struct device *dev,
			   const uint8_t brightness)
{
	struct lcd_panel_data *data = dev->data;

	if (data->disp_on == 0)
		return -EPERM;

	if (brightness == data->pending_brightness)
		return 0;

	LOG_INF("display set_brightness %u", brightness);
	data->pending_brightness = brightness;

	/* delayed set in TE interrupt handler */

	return 0;
}

static int _lcd_panel_get_brightness(const struct device *dev)
{
   struct lcd_panel_data *data = dev->data;

   return data->brightness;
}

static int _lcd_panel_set_contrast(const struct device *dev, const uint8_t contrast)
{
	return -ENOTSUP;
}

static void _lcd_panel_get_capabilities(const struct device *dev,
					struct display_capabilities *capabilities)
{
	struct lcd_panel_data *data = dev->data;
	const struct lcd_panel_config *config = data->config;

	memset(capabilities, 0, sizeof(struct display_capabilities));
	capabilities->x_resolution = CONFIG_PANEL_HOR_RES;
	capabilities->y_resolution = CONFIG_PANEL_VER_RES;
	capabilities->refresh_rate = config->videomode.refresh_rate;
	capabilities->supported_pixel_formats = SUPPORTED_PANEL_PIXEL_FORMATS;
	capabilities->current_pixel_format = config->videomode.pixel_format;
	capabilities->current_orientation = (enum display_orientation)(CONFIG_PANEL_ROTATION / 90);
	capabilities->screen_info = (CONFIG_PANEL_ROUND_SHAPE ? SCREEN_INFO_ROUND_SHAPE : 0);
#if IS_TR_PANEL
	capabilities->screen_info |= (SCREEN_INFO_X_ALIGNMENT_WIDTH | SCREEN_INFO_Y_ALIGNMENT_HEIGHT);
#endif
}

static int _lcd_panel_set_pixel_format(const struct device *dev,
				       const enum display_pixel_format pixel_format)
{
	struct lcd_panel_data *data = dev->data;
	const struct lcd_panel_config *config = data->config;

	if (pixel_format == config->videomode.pixel_format) {
		return 0;
	}

	LOG_ERR("Pixel format change not implemented");
	return -ENOTSUP;
}

static int _lcd_panel_set_orientation(const struct device *dev,
				      const enum display_orientation orientation)
{
	enum display_orientation current_orientation =
					(enum display_orientation)(CONFIG_PANEL_ROTATION / 90);

	return (orientation == current_orientation) ? 0 : -ENOTSUP;
}

static int _lcd_panel_register_callback(const struct device *dev,
					const struct display_callback *callback)
{
	struct lcd_panel_data *data = dev->data;

	if (data->callback == NULL) {
		data->callback = callback;
		return 0;
	}

	return -EBUSY;
}

static int _lcd_panel_unregister_callback(const struct device *dev,
					  const struct display_callback *callback)
{
	struct lcd_panel_data *data = dev->data;

	if (data->callback == callback) {
		data->callback = NULL;
		return 0;
	}

	return -EINVAL;
}

static int _lcd_panel_init(const struct device *dev)
{
	struct lcd_panel_data *data = dev->data;
	const struct lcd_panel_pincfg *pincfg = dev->config;
	const struct device *de_dev = NULL;

#ifdef CONFIG_PANEL_BACKLIGHT_PWM
	data->backlight_dev = device_get_binding(pincfg->backlight_cfg.dev_name);
	if (data->backlight_dev == NULL) {
		LOG_ERR("Couldn't find pwm device\n");
		return -ENODEV;
	}

	pwm_pin_set_cycles(data->backlight_dev, pincfg->backlight_cfg.chan,
			pincfg->backlight_cfg.period, 0, pincfg->backlight_cfg.flag);
#elif defined(CONFIG_PANEL_BACKLIGHT_GPIO)
	data->backlight_dev = device_get_binding(pincfg->backlight_cfg.gpio_dev_name);
	if (data->backlight_dev == NULL) {
		LOG_ERR("Couldn't find backlight pin");
		return -ENODEV;
	}

	if (gpio_pin_configure(data->backlight_dev, pincfg->backlight_cfg.gpion,
				GPIO_OUTPUT_INACTIVE | pincfg->backlight_cfg.flag)) {
		LOG_ERR("Couldn't configure backlight pin");
		return -ENODEV;
	}
#endif /* CONFIG_PANEL_BACKLIGHT_PWM */

#ifdef CONFIG_PANEL_RESET_GPIO
	data->reset_gpio = device_get_binding(pincfg->reset_cfg.gpio_dev_name);
	if (data->reset_gpio == NULL) {
		LOG_ERR("Couldn't find reset pin");
		return -ENODEV;
	}

	if (gpio_pin_configure(data->reset_gpio, pincfg->reset_cfg.gpion,
				GPIO_OUTPUT_ACTIVE | pincfg->reset_cfg.flag)) {
		LOG_ERR("Couldn't configure reset pin");
		return -ENODEV;
	}
#endif /* CONFIG_PANEL_RESET_GPIO */

#ifdef CONFIG_PANEL_POWER_GPIO
	data->power_gpio = device_get_binding(pincfg->power_cfg.gpio_dev_name);
	if (data->power_gpio == NULL) {
		LOG_ERR("Couldn't find power pin");
		return -ENODEV;
	}

	if (gpio_pin_configure(data->power_gpio, pincfg->power_cfg.gpion,
				GPIO_OUTPUT_INACTIVE | pincfg->power_cfg.flag)) {
		LOG_ERR("Couldn't configure power pin");
		return -ENODEV;
	}
#endif /* CONFIG_PANEL_POWER_GPIO */

#ifdef CONFIG_PANEL_POWER1_GPIO
	data->power1_gpio = device_get_binding(pincfg->power1_cfg.gpio_dev_name);
	if (data->power1_gpio == NULL) {
		LOG_ERR("Couldn't find power1 pin");
		return -ENODEV;
	}

	if (gpio_pin_configure(data->power1_gpio, pincfg->power1_cfg.gpion,
					GPIO_OUTPUT_INACTIVE | pincfg->power1_cfg.flag)) {
		LOG_ERR("Couldn't configure power1 pin");
		return -ENODEV;
	}
#endif /* CONFIG_PANEL_POWER1_GPIO */

#ifdef CONFIG_PANEL_TE_GPIO
	data->te_gpio = device_get_binding(pincfg->te_cfg.gpio_dev_name);
	if (data->te_gpio == NULL) {
		LOG_ERR("Couldn't find te pin");
		return -ENODEV;
	}

	if (gpio_pin_configure(data->te_gpio, pincfg->te_cfg.gpion,
				GPIO_INPUT | pincfg->te_cfg.flag)) {
		LOG_ERR("Couldn't configure te pin");
		return -ENODEV;
	}

	/* FIXME: make sure interrupt disabled */
	gpio_pin_interrupt_configure(data->te_gpio, pincfg->te_cfg.gpion, GPIO_INT_DISABLE);
	gpio_init_callback(&data->te_gpio_cb, _panel_te_gpio_handler, BIT(pincfg->te_cfg.gpion));
	gpio_add_callback(data->te_gpio, &data->te_gpio_cb);
#else
	k_timer_init(&data->te_timer, _panel_te_timer_handler, NULL);
#endif /* CONFIG_PANEL_TE_GPIO */

	data->lcdc_dev = device_get_binding(CONFIG_LCDC_DEV_NAME);
	if (data->lcdc_dev == NULL) {
		LOG_ERR("Could not get LCD controller device");
		return -EPERM;
	}

	display_controller_control(data->lcdc_dev,
			DISPLAY_CONTROLLER_CTRL_COMPLETE_CB, _panel_complete_transfer, (void *)dev);

	if (_panel_detect(dev)) {
		LOG_ERR("No panel connected");
		return -ENODEV;
	}

	de_dev = device_get_binding(CONFIG_DISPLAY_ENGINE_DEV_NAME);
	if (de_dev == NULL) {
		LOG_WRN("Could not get display engine device");
	} else {
		display_engine_control(de_dev,
				DISPLAY_ENGINE_CTRL_DISPLAY_PORT, (void *)&data->config->videoport, NULL);
		display_engine_control(de_dev,
				DISPLAY_ENGINE_CTRL_DISPLAY_MODE, (void *)&data->config->videomode, NULL);
		display_engine_control(de_dev,
				DISPLAY_ENGINE_CTRL_DISPLAY_PREPARE_CB, _panel_prepare_de_transfer, (void *)dev);
		display_engine_control(de_dev,
				DISPLAY_ENGINE_CTRL_DISPLAY_START_CB, _panel_start_de_transfer, (void *)dev);
	}

	data->disp_on = 0;
	data->in_sleep = 1;
	data->te_active = 0;
	data->pending_brightness = CONFIG_PANEL_BRIGHTNESS;
	data->brightness_delay = CONFIG_PANEL_BRIGHTNESS_DELAY_PERIODS;
	data->pm_state = PM_DEVICE_STATE_SUSPENDED;

#if CONFIG_PANEL_ESD_CHECK_PERIOD > 0
	k_delayed_work_init(&data->esd_check_work, _panel_esd_check_handler);
#endif

	k_work_init(&data->resume_work, _panel_pm_resume_handler);
#ifdef CONFIG_LCD_WORK_QUEUE
	k_work_queue_start(&data->pm_workq, pm_workq_stack, K_THREAD_STACK_SIZEOF(pm_workq_stack), 6, NULL);
#endif

	_panel_pm_late_resume(dev);
	return 0;
}

/**********************
 *  POWER MANAGEMENT
 **********************/
static void _panel_pm_notify(struct lcd_panel_data *data, uint32_t pm_action)
{
	if (data->callback && data->callback->pm_notify) {
		data->callback->pm_notify(data->callback, pm_action);
	}
}

static int _panel_pm_early_suspend(const struct device *dev, bool in_turnoff)
{
	struct lcd_panel_data *data = dev->data;

	if (data->pm_state != PM_DEVICE_STATE_ACTIVE &&
		data->pm_state != PM_DEVICE_STATE_LOW_POWER) {
		return 0;
	}

	LOG_INF("panel early-suspend");

	_panel_cancel_esd_check_work(data);
	_panel_pm_notify(data, in_turnoff ? PM_DEVICE_ACTION_TURN_OFF : PM_DEVICE_ACTION_EARLY_SUSPEND);
	_panel_power_off(dev, in_turnoff);

	data->pm_state = PM_DEVICE_STATE_SUSPENDED;
	return 0;
}

static int _panel_pm_late_resume(const struct device *dev)
{
	struct lcd_panel_data *data = dev->data;

	if (data->pm_state != PM_DEVICE_STATE_SUSPENDED) {
		return 0;
	}

	LOG_INF("panel late-resume");
	data->pm_changing = 1;

#ifdef CONFIG_LCD_WORK_QUEUE
	k_work_submit_to_queue(&data->pm_workq, &data->resume_work);
#else
	k_work_submit(&data->resume_work);
#endif

	return 0;
}

static void _panel_pm_resume_handler(struct k_work *work)
{
	const struct device *dev = DEVICE_GET(lcd_panel);
	struct lcd_panel_data *data = dev->data;

	LOG_INF("panel resuming");

	lcd_panel_wake_lock();

	data->pm_state = PM_DEVICE_STATE_ACTIVE;
	data->transfering = 0; /* reset flag in case fail again */

	_panel_power_on(dev);
	_panel_pm_notify(data, PM_DEVICE_ACTION_LATE_RESUME);
	_panel_submit_esd_check_work(data);

	data->pm_changing = 0;
	lcd_panel_wake_unlock();

	LOG_INF("panel active");
}

#ifdef CONFIG_PM_DEVICE
static void _panel_pm_change_low_power(const struct device *dev, uint32_t pm_state)
{
	struct lcd_panel_data *data = dev->data;
	const struct lcd_panel_config *config = data->config;
	unsigned int key;

	while (1) {
		key = irq_lock();
		if (!data->transfering)
			break;

		irq_unlock(key);
		k_msleep(1); /* wait transfer finish */
	}

	data->pm_state = pm_state;

	if (data->pm_state == PM_DEVICE_STATE_LOW_POWER) {
		if (config->ops->lowpower_enter)
			config->ops->lowpower_enter(dev);

		if (config->ops->set_aod_brightness) {
			config->ops->set_aod_brightness(dev, CONFIG_PANEL_AOD_BRIGHTNESS);
		} else {
			_panel_apply_brightness(dev, CONFIG_PANEL_AOD_BRIGHTNESS);
		}

		_panel_te_set_enable(dev, false);
		board_lcd_suspend(true, true);
	} else {
		board_lcd_resume(true, true);

		if (config->ops->lowpower_exit)
			config->ops->lowpower_exit(dev);

		_panel_te_set_enable(dev, true);
	}

	irq_unlock(key);
}

static int _panel_pm_enter_low_power(const struct device *dev)
{
	struct lcd_panel_data *data = dev->data;

	if (data->pm_state != PM_DEVICE_STATE_ACTIVE) {
		return -EPERM;
	}

	_panel_cancel_esd_check_work(data);
	_panel_pm_notify(data, PM_DEVICE_ACTION_LOW_POWER);
	_panel_pm_change_low_power(dev, PM_DEVICE_STATE_LOW_POWER);

	LOG_INF("panel enter low-power");
	return 0;
}

static int _panel_pm_exit_low_power(const struct device *dev)
{
	struct lcd_panel_data *data = dev->data;

	if (data->pm_state != PM_DEVICE_STATE_LOW_POWER) {
		return -EPERM;
	}

	_panel_pm_notify(data, PM_DEVICE_ACTION_LATE_RESUME);
	_panel_pm_change_low_power(dev, PM_DEVICE_STATE_ACTIVE);
	_panel_submit_esd_check_work(data);

	LOG_INF("panel exit low-power");
	return 0;
}

static int _lcd_panel_pm_control(const struct device *dev, enum pm_device_action action)
{
	struct lcd_panel_data *data = dev->data;
	int ret = 0;

	if (data->pm_state == PM_DEVICE_STATE_OFF)
		return -EPERM;

	switch (action) {
	case PM_DEVICE_ACTION_EARLY_SUSPEND:
		if (soc_get_aod_mode()) {
			ret = _panel_pm_enter_low_power(dev);
		} else {
			ret = _panel_pm_early_suspend(dev, false);
			board_lcd_suspend(false, true);
		}
		break;

	case PM_DEVICE_ACTION_LATE_RESUME:
		if (data->pm_state == PM_DEVICE_STATE_LOW_POWER) {
			ret = _panel_pm_exit_low_power(dev);
		} else {
			board_lcd_resume(false, true);
			ret = _panel_pm_late_resume(dev);
		}
		break;

	case PM_DEVICE_ACTION_SUSPEND:
		if (data->pm_changing || data->pm_state == PM_DEVICE_STATE_ACTIVE) {
			ret = -EPERM;
		}
		break;

	case PM_DEVICE_ACTION_TURN_OFF:
		_panel_pm_early_suspend(dev, true);
		data->pm_state = PM_DEVICE_STATE_OFF;
		LOG_INF("panel turn-off");
		break;

	default:
		break;
	}

	return ret;
}
#endif /* CONFIG_PM_DEVICE */

/**********************
 *  DEVICE DEFINITION
 **********************/
static const struct display_driver_api lcd_panel_driver_api = {
	.blanking_on = _lcd_panel_blanking_on,
	.blanking_off = _lcd_panel_blanking_off,
	.write = _lcd_panel_write,
	.read = _lcd_panel_read,
	.get_framebuffer = _lcd_panel_get_framebuffer,
	.set_brightness = _lcd_panel_set_brightness,
	.get_brightness = _lcd_panel_get_brightness,
	.set_contrast = _lcd_panel_set_contrast,
	.get_capabilities = _lcd_panel_get_capabilities,
	.set_pixel_format = _lcd_panel_set_pixel_format,
	.set_orientation = _lcd_panel_set_orientation,
	.register_callback = _lcd_panel_register_callback,
	.unregister_callback = _lcd_panel_unregister_callback,
};

static const struct lcd_panel_pincfg lcd_panel_pin_config = {
#ifdef CONFIG_PANEL_POWER_GPIO
	.power_cfg = CONFIG_PANEL_POWER_GPIO,
#endif

#ifdef CONFIG_PANEL_POWER1_GPIO
	.power1_cfg = CONFIG_PANEL_POWER1_GPIO,
#endif

#ifdef CONFIG_PANEL_RESET_GPIO
	.reset_cfg = CONFIG_PANEL_RESET_GPIO,
#endif

#ifdef CONFIG_PANEL_TE_GPIO
	.te_cfg = CONFIG_PANEL_TE_GPIO,
#endif

#ifdef CONFIG_PANEL_BACKLIGHT_PWM
	.backlight_cfg = CONFIG_PANEL_BACKLIGHT_PWM,
#elif defined(CONFIG_PANEL_BACKLIGHT_GPIO)
	.backlight_cfg = CONFIG_PANEL_BACKLIGHT_GPIO,
#endif
};

static struct lcd_panel_data lcd_panel_data;

#if IS_ENABLED(CONFIG_PANEL)
DEVICE_DEFINE(lcd_panel, CONFIG_LCD_DISPLAY_DEV_NAME, _lcd_panel_init, _lcd_panel_pm_control,
		&lcd_panel_data, &lcd_panel_pin_config, POST_KERNEL, CONFIG_APPLICATION_INIT_PRIORITY,
		&lcd_panel_driver_api);
#endif
