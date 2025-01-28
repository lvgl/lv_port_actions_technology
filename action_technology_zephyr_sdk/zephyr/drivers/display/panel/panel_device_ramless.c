/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <spicache.h>
#include <tracing/tracing.h>
#include "panel_device.h"
#include "../dma2d_lite/dma2d_lite.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(lcd_panel, CONFIG_DISPLAY_LOG_LEVEL);

/*********************
 *      DEFINES
 *********************/

#if CONFIG_PANEL_PORT_TYPE != DISPLAY_PORT_QSPI_SYNC
#  error "only support QSPI sync mode"
#endif

#if CONFIG_PANEL_HOR_RES != CONFIG_PANEL_TIMING_HACTIVE
#  error "CONFIG_PANEL_HOR_RES must equal to CONFIG_PANEL_TIMING_HACTIVE"
#endif

#if CONFIG_PANEL_VER_RES != CONFIG_PANEL_TIMING_VACTIVE
#  error "CONFIG_PANEL_VER_RES must equal to CONFIG_PANEL_TIMING_VACTIVE"
#endif

#if defined(CONFIG_PANEL_TE_GPIO)
#  error "CONFIG_PANEL_TE_GPIO must not exist"
#endif

#define ESD_CHECK_COUNT \
	(CONFIG_PANEL_ESD_CHECK_PERIOD * CONFIG_PANEL_TIMING_REFRESH_RATE_HZ / 1000u)

#define SDMA       ((SDMA_Type *)SDMA_REG_BASE)
#define SDMA_CHAN  (&(SDMA->CHAN_CTL[4]))
#define SDMA_LINE  (&(SDMA->LINE_CTL[4]))

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

static int _panel_pm_early_suspend(const struct device *dev, bool in_turnoff);
#if ESD_CHECK_COUNT > 0
static void _panel_pm_suspend_handler(struct k_work *work);
#endif

static int _panel_pm_late_resume(const struct device *dev);
static void _panel_pm_resume_handler(struct k_work *work);

/**********************
 *  STATIC VARIABLES
 **********************/
#ifdef CONFIG_LCD_WORK_QUEUE
K_THREAD_STACK_DEFINE(pm_workq_stack, CONFIG_LCD_WORK_Q_STACK_SIZE);
#endif

static const struct lcd_panel_config *const lcd_panel_configs[] = {
#ifdef CONFIG_PANEL_AXS15231B
	&lcd_panel_axs15231b_config,
#endif
#ifdef CONFIG_PANEL_ST77903
	&lcd_panel_st77903_config,
#endif
};

/**********************
 *      MACROS
 **********************/

/**********************
 * STATIC FUNCTIONS
 **********************/

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

#ifdef CONFIG_PANEL_RESET_GPIO
	k_msleep(config->tw_reset);
	gpio_pin_set(data->reset_gpio, pincfg->reset_cfg.gpion, 0);
	k_msleep(config->ts_reset);
#endif
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

	ready = (data->brightness_delay == 0);
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

static int _panel_blanking_on(const struct device *dev)
{
	struct lcd_panel_data *data = dev->data;
	const struct lcd_panel_config *config = data->config;

	if (data->disp_on == 0)
		return 0;

	LOG_INF("display blanking on");
	data->disp_on = 0;
	data->brightness_delay = CONFIG_PANEL_BRIGHTNESS_DELAY_PERIODS;

	k_timer_stop(&data->te_timer);
	_panel_apply_brightness(dev, 0);

	if (config->fb_mem == NULL) { /* wait 100 ms timeout */
		display_engine_control(data->de_dev, DISPLAY_ENGINE_CTRL_DISPLAY_SYNC_STOP, (void *)(uintptr_t)100, NULL);
	}

	display_controller_disable(data->lcdc_dev);

	if (config->ops->blanking_on) {
		display_controller_enable(data->lcdc_dev, &config->videoport);
		config->ops->blanking_on(dev);
		display_controller_disable(data->lcdc_dev);
	}

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

	if (config->fb_mem) {
		/* initialize the fb memory to black color */
		memset(cache_to_uncache(config->fb_mem), 0, data->refr_desc.buf_size);
		spi1_cache_ops(SPI_WRITEBUF_FLUSH, cache_to_uncache(config->fb_mem), data->refr_desc.buf_size);

		/* start lcd */
		display_controller_set_source(data->lcdc_dev, DISPLAY_CONTROLLER_SOURCE_DMA, NULL);
		int res = display_controller_write_pixels(data->lcdc_dev, config->cmd_ramwr,
						      config->cmd_hsync, &data->refr_desc, config->fb_mem);
		if (res) {
			LOG_ERR("write pixels failed");
		}
	} else {
		/* source from DE */
		display_controller_set_source(data->lcdc_dev, DISPLAY_CONTROLLER_SOURCE_ENGINE, NULL);
		/* refresh not started yet, simply use k_timer as te signal */
		k_timer_start(&data->te_timer, K_MSEC(0), K_MSEC(CONFIG_PANEL_VSYNC_PERIOD_MS));
	}

	return 0;
}

/**********************
 *  DEVICE DECLARATION
 **********************/
DEVICE_DECLARE(lcd_panel);

/**********************
 *  TE CALLBACK
 **********************/
static void _panel_te_handler(void)
{
	const struct device *dev = DEVICE_GET(lcd_panel);
	struct lcd_panel_data *data = dev->data;
	uint32_t timestamp = k_cycle_get_32();

	sys_trace_void(SYS_TRACE_ID_VSYNC);

	if (data->callback && data->callback->vsync) {
		data->callback->vsync(data->callback, timestamp);
	}
}

static void _panel_te_timer_handler(struct k_timer *timer)
{
	_panel_te_handler();
}

/**********************
 *  DE/LCDC CALLBACK
 **********************/
static void _panel_start_de_transfer(void *arg)
{
	const struct device *dev = arg;
	struct lcd_panel_data *data = dev->data;
	const struct lcd_panel_config *config = data->config;
	int res;

	if (config->fb_mem) {
		k_panic(); /* should not goto this path */
	}

#if CONFIG_PANEL_TE_SCANLINE == 0
	k_timer_stop(&data->te_timer);
#endif

	res = display_controller_write_pixels(data->lcdc_dev, config->cmd_ramwr, config->cmd_hsync,
					&data->refr_desc, NULL);
	if (res) {
		LOG_ERR("write pixels failed");
	}
}

static void _panel_complete_transfer(void *arg)
{
	const struct device *dev = arg;
	struct lcd_panel_data *data = dev->data;
	const struct lcd_panel_config *config __unused = data->config;

	_panel_apply_brightness(dev, data->pending_brightness);

#if CONFIG_PANEL_TE_SCANLINE == 0
	_panel_te_handler();
#else
	k_timer_start(&data->te_timer, K_MSEC(CONFIG_PANEL_TE_SCANLINE), K_MSEC(0));
#endif

#if ESD_CHECK_COUNT > 0
	if (++data->esd_check_cnt >= ESD_CHECK_COUNT) {
		data->esd_check_cnt = 0;

		if (config->ops->check_esd && config->ops->check_esd(dev)) {
			LOG_ERR("ESD check failed");
			data->pm_changing = 1;
#ifdef CONFIG_LCD_WORK_QUEUE
			k_work_submit_to_queue(&data->pm_workq, &data->suspend_work);
			k_work_submit_to_queue(&data->pm_workq, &data->resume_work);
#else
			k_work_submit(&data->suspend_work);
			k_work_submit(&data->resume_work);
#endif
		}
	}
#endif /* ESD_CHECK_COUNT > 0 */
}

static void _panel_sdma4_irq_handler(const void *arg)
{
	const struct device *dev = arg;
	struct lcd_panel_data *data = dev->data;

	SDMA->IP = BIT(4);

	sys_trace_end_call(SYS_TRACE_ID_LCD_POST);

	data->transfering = 0;
	if (data->callback && data->callback->complete) {
		data->callback->complete(data->callback);
	}
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

	if (config->fb_mem == NULL)
		return -EPERM;

	if (desc->pixel_format != data->refr_desc.pixel_format ||
		x + desc->width > data->refr_desc.width ||
		y + desc->height > data->refr_desc.height)
		return -EINVAL;

	if (data->transfering)
		return -EBUSY;

	data->transfering = 1;

	sys_trace_u32x4(SYS_TRACE_ID_LCD_POST,
			x, y, x + desc->width - 1, y + desc->height - 1);

	uint32_t bytes_per_line = desc->width * (CONFIG_PANEL_COLOR_DEPTH / 8);

	SDMA_LINE->COUNT = desc->height;
	SDMA_LINE->LENGTH = bytes_per_line;
	SDMA_CHAN->BC = desc->height * bytes_per_line;
	SDMA_LINE->SSTRIDE = desc->pitch;
	SDMA_LINE->DSTRIDE = data->refr_desc.pitch;
	SDMA_CHAN->CTL = BIT(24); /* stride mode */
	SDMA_CHAN->SADDR = (uint32_t)buf;
	SDMA_CHAN->DADDR = (uint32_t)config->fb_mem +
			y * data->refr_desc.pitch + x * (CONFIG_PANEL_COLOR_DEPTH / 8);
	SDMA_CHAN->START = 1;

	return 0;
}

static void *_lcd_panel_get_framebuffer(const struct device *dev)
{
	struct lcd_panel_data *data = dev->data;
	return data->config->fb_mem;
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
	capabilities->supported_pixel_formats = config->videomode.pixel_format;
	capabilities->current_pixel_format = config->videomode.pixel_format;
	capabilities->current_orientation = (enum display_orientation)(CONFIG_PANEL_ROTATION / 90);
	capabilities->screen_info = (CONFIG_PANEL_ROUND_SHAPE ? SCREEN_INFO_ROUND_SHAPE : 0);
	if (config->fb_mem == NULL) {
		capabilities->screen_info |= SCREEN_INFO_ZERO_BUFFER |
			SCREEN_INFO_X_ALIGNMENT_WIDTH | SCREEN_INFO_Y_ALIGNMENT_HEIGHT;
	}
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

	k_timer_init(&data->te_timer, _panel_te_timer_handler, NULL);

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

	data->de_dev = device_get_binding(CONFIG_DISPLAY_ENGINE_DEV_NAME);
	if (data->de_dev == NULL) {
		LOG_ERR("Could not get display engine device");
		return -ENODEV;
	}

	if (data->config->fb_mem == NULL) {
		display_engine_control(data->de_dev,
				DISPLAY_ENGINE_CTRL_DISPLAY_PORT, (void *)&data->config->videoport, NULL);
		display_engine_control(data->de_dev,
				DISPLAY_ENGINE_CTRL_DISPLAY_MODE, (void *)&data->config->videomode, NULL);
		display_engine_control(data->de_dev,
				DISPLAY_ENGINE_CTRL_DISPLAY_START_CB, _panel_start_de_transfer, (void *)dev);
	} else {
		if (IS_ENABLED(CONFIG_JPEG_HW)) {
			printk("Must disable JPEG_HW to release SDMA4\n");
			k_panic();
		}

		if (IS_ENABLED(CONFIG_DMA2D_LITE) && CONFIG_DMA2D_LITE_SDMA_CHAN == 4) {
			printk("Must not set CONFIG_DMA2D_LITE_SDMA_CHAN to 4 to release SDMA4\n");
			k_panic();
		}

		/* config SDMA interrupt */
		SDMA->IE |= BIT(4);
		IRQ_CONNECT(IRQ_ID_SDMA4, 0, _panel_sdma4_irq_handler, DEVICE_GET(lcd_panel), 0);
		irq_enable(IRQ_ID_SDMA4);
	}

	data->refr_desc.pixel_format = data->config->videomode.pixel_format;
	data->refr_desc.width = CONFIG_PANEL_HOR_RES;
	data->refr_desc.height = CONFIG_PANEL_VER_RES;
	data->refr_desc.pitch = CONFIG_PANEL_HOR_RES *
				display_format_get_bits_per_pixel(data->refr_desc.pixel_format) / 8;
	data->refr_desc.buf_size = data->refr_desc.height * data->refr_desc.pitch;

	data->disp_on = 0;
	data->pending_brightness = CONFIG_PANEL_BRIGHTNESS;
	data->brightness_delay = CONFIG_PANEL_BRIGHTNESS_DELAY_PERIODS;
	data->pm_state = PM_DEVICE_STATE_SUSPENDED;

#if ESD_CHECK_COUNT > 0
	k_work_init(&data->suspend_work, _panel_pm_suspend_handler);
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
		return -EPERM;
	}

	LOG_INF("panel early-suspend");

	_panel_pm_notify(data, PM_DEVICE_ACTION_EARLY_SUSPEND);
	_panel_power_off(dev, in_turnoff);

#if ESD_CHECK_COUNT > 0
	data->esd_check_cnt = 0;
#endif
	data->pm_state = PM_DEVICE_STATE_SUSPENDED;
	return 0;
}

#if ESD_CHECK_COUNT > 0
static void _panel_pm_suspend_handler(struct k_work *work)
{
	const struct device *dev = DEVICE_GET(lcd_panel);

	lcd_panel_wake_lock();
	_panel_pm_early_suspend(dev, false);
	lcd_panel_wake_unlock();
}
#endif

static int _panel_pm_late_resume(const struct device *dev)
{
	struct lcd_panel_data *data = dev->data;

	if (data->pm_state != PM_DEVICE_STATE_SUSPENDED) {
		return -EPERM;
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

	_panel_power_on(dev);
	_panel_pm_notify(data, PM_DEVICE_ACTION_LATE_RESUME);

	data->pm_changing = 0;

	lcd_panel_wake_unlock();

	LOG_INF("panel active");
}

#ifdef CONFIG_PM_DEVICE
static int _lcd_panel_pm_control(const struct device *dev, enum pm_device_action action)
{
	struct lcd_panel_data *data = dev->data;
	int ret = 0;

	if (data->pm_state == PM_DEVICE_STATE_OFF)
		return -EPERM;

	switch (action) {
	case PM_DEVICE_ACTION_EARLY_SUSPEND:
		ret = _panel_pm_early_suspend(dev, false);
		board_lcd_suspend(false, true);
		break;

	case PM_DEVICE_ACTION_LATE_RESUME:
		board_lcd_resume(false, true);
		ret = _panel_pm_late_resume(dev);
		break;

	case PM_DEVICE_ACTION_SUSPEND:
		if (data->pm_changing || data->pm_state == PM_DEVICE_STATE_ACTIVE) {
			ret = -EPERM;
		}
		break;

	case PM_DEVICE_ACTION_RESUME:
		if (data->config->fb_mem) {
			SDMA->IE |= BIT(4); /* Make sure interrupt enabled */
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
