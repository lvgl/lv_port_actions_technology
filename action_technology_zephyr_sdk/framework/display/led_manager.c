/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file led manager interface
 */
#include <os_common_api.h>
#include <mem_manager.h>
#include <led_manager.h>
#include <kernel.h>
#include <led_hal.h>
#include <string.h>
#include <sys_monitor.h>
#if defined(CONFIG_SYS_LOG)
#define SYS_LOG_NO_NEWLINE
#ifdef SYS_LOG_DOMAIN
#undef SYS_LOG_DOMAIN
#endif
#define SYS_LOG_DOMAIN "led_manager"
#endif


#ifdef BOARD_LED_MAP
const led_map led_maps[] = {
	BOARD_LED_MAP
};
#else
#define BOARD_LED_MAP
const led_map led_maps[8] = {
};
#endif

#define MAX_LED_NUM ARRAY_SIZE(led_maps)

OS_MUTEX_DEFINE(led_manager_mutex);

struct led_state_t {
	uint8_t mode;
	uint16_t timeout;
	union{
		struct {
			uint16_t blink_period;
			uint16_t blink_pulse;
			uint8_t start_state;
		};
		struct {
			pwm_breath_ctrl_t ctrl;
		};
	};
	led_display_callback cb;
};

struct led_image {
	sys_snode_t snode;
	struct led_state_t led_state[MAX_LED_NUM];
};

struct led_manager_ctx_t {
	struct device *dev;
	struct led_image image;
	uint16_t timeout;
	uint8_t update_direct:1;
	uint8_t timeout_event_lock:1;
	led_timeout_callback timeout_cb;
	sys_slist_t	image_list;
};

static struct led_manager_ctx_t global_led_manager_ctx;

static struct led_manager_ctx_t *_led_manager_get_ctx(void)
{
	return &global_led_manager_ctx;
}
#ifdef BOARD_LED_MAP
static int _led_manager_update_state(void)
{
	struct led_manager_ctx_t *ctx = _led_manager_get_ctx();

	for (int led_index = 0 ; led_index < MAX_LED_NUM; led_index++) {
		struct led_state_t *led_state = &ctx->image.led_state[led_index];

		switch (led_state->mode) {
		case LED_BLINK:
			led_blink(led_index, led_state->blink_period, led_state->blink_pulse, led_state->start_state);
			break;
		case LED_BREATH:
			if (led_state->ctrl.rise_time_ms || led_state->ctrl.down_time_ms
				|| led_state->ctrl.high_time_ms || led_state->ctrl.low_time_ms) {
				led_breath(led_index, &led_state->ctrl);
			} else {
				led_breath(led_index, NULL);
			}
			break;
		case LED_ON:
			led_on(led_index);
			break;
		case LED_OFF:
			led_off(led_index);
			break;
		}
	}

	return 0;
}
#endif

int led_manager_set_timeout_event_start(void)
{
	struct led_manager_ctx_t *ctx = _led_manager_get_ctx();

	os_mutex_lock(&led_manager_mutex, OS_FOREVER);
	ctx->update_direct = 1;
	ctx->timeout_event_lock = 1;
	os_mutex_unlock(&led_manager_mutex);
	return 0;
}

int led_manager_set_timeout_event(int timeout, led_timeout_callback cb)
{
	struct led_manager_ctx_t *ctx = _led_manager_get_ctx();

	os_mutex_lock(&led_manager_mutex, OS_FOREVER);
	ctx->timeout_cb = cb;
	ctx->timeout = timeout / CONFIG_MONITOR_PERIOD;
	ctx->update_direct = 0;

	os_mutex_unlock(&led_manager_mutex);
	return 0;
}

#ifdef BOARD_LED_MAP
static int _led_manager_work_handle(void)
{
	struct led_manager_ctx_t *ctx = _led_manager_get_ctx();

	os_mutex_lock(&led_manager_mutex, OS_FOREVER);

	for (int led_index = 0; led_index < MAX_LED_NUM; led_index++) {
		struct led_state_t *led_state = &ctx->image.led_state[led_index];

		if (led_state->timeout != DISPLAY_FOREVER) {
			led_state->timeout--;
			if (!led_state->timeout) {
				led_on(led_index);
				led_state->timeout = DISPLAY_FOREVER;
				if (led_state->cb) {
					os_mutex_unlock(&led_manager_mutex);
					led_state->cb();
					os_mutex_lock(&led_manager_mutex, OS_FOREVER);
				}
			}
		}
	}

	if (ctx->timeout != DISPLAY_FOREVER) {
		if (ctx->timeout-- == 0) {
			ctx->timeout = DISPLAY_FOREVER;
			ctx->update_direct = 0;
			ctx->timeout_event_lock = 0;
			_led_manager_update_state();
			if (ctx->timeout_cb) {
				os_mutex_unlock(&led_manager_mutex);
				ctx->timeout_cb();
				os_mutex_lock(&led_manager_mutex, OS_FOREVER);
				ctx->timeout_cb = NULL;
			}
		}
	}
	os_mutex_unlock(&led_manager_mutex);

	return 0;
}
#endif
int led_manager_set_display(uint16_t led_index, uint8_t onoff, uint32_t timeout, led_display_callback cb)
{
#ifdef BOARD_LED_MAP
	struct led_manager_ctx_t *ctx = _led_manager_get_ctx();
	struct led_state_t *led_state = &ctx->image.led_state[led_index];

	os_mutex_lock(&led_manager_mutex, OS_FOREVER);
	if (!ctx->update_direct) {
		led_state->mode = onoff;
		led_state->cb = cb;

		if (timeout != OS_FOREVER) {
			led_state->timeout = timeout / CONFIG_MONITOR_PERIOD;
		} else {
			led_state->timeout = DISPLAY_FOREVER;
		}
	}

	if (!ctx->timeout_event_lock || ctx->update_direct) {
		if (onoff == LED_ON) {
			led_on(led_index);
		} else {
			led_off(led_index);
		}
	}
	os_mutex_unlock(&led_manager_mutex);
#endif
	return 0;
}

int led_manager_set_breath(uint16_t led_index, pwm_breath_ctrl_t *ctrl, uint32_t timeout, led_display_callback cb)
{
#ifdef BOARD_LED_MAP
	struct led_manager_ctx_t *ctx = _led_manager_get_ctx();
	struct led_state_t *led_state = &ctx->image.led_state[led_index];

	os_mutex_lock(&led_manager_mutex, OS_FOREVER);
	if (!ctx->update_direct) {
		led_state->mode = LED_BREATH;
		if (ctrl) {
			memcpy(&led_state->ctrl, ctrl, sizeof(pwm_breath_ctrl_t));
		} else {
			memset(&led_state->ctrl, 0, sizeof(pwm_breath_ctrl_t));
		}

		if (timeout != OS_FOREVER) {
			led_state->timeout = timeout / CONFIG_MONITOR_PERIOD;
		} else {
			led_state->timeout = DISPLAY_FOREVER;
		}
		led_state->cb = cb;
	}

	if (!ctx->timeout_event_lock || ctx->update_direct) {
		led_breath(led_index, ctrl);
	}
	os_mutex_unlock(&led_manager_mutex);
#endif
	return 0;
}

int led_manager_set_blink(uint8_t led_index, uint16_t blink_period, uint16_t blink_pulse, uint32_t timeout, uint8_t start_state, led_display_callback cb)
{
#ifdef BOARD_LED_MAP
	struct led_manager_ctx_t *ctx = _led_manager_get_ctx();
	struct led_state_t *led_state = &ctx->image.led_state[led_index];

	os_mutex_lock(&led_manager_mutex, OS_FOREVER);

	if (!ctx->update_direct) {
		led_state->mode = LED_BLINK;
		if (timeout != OS_FOREVER) {
			led_state->timeout = timeout / CONFIG_MONITOR_PERIOD;
		} else {
			led_state->timeout = DISPLAY_FOREVER;
		}
		led_state->blink_period = blink_period;
		led_state->blink_pulse = blink_pulse;
		led_state->start_state = start_state;
		led_state->cb = cb;
	}
	if (!ctx->timeout_event_lock || ctx->update_direct) {
		led_blink(led_index, blink_period, blink_pulse, start_state);
	}

	os_mutex_unlock(&led_manager_mutex);
#endif
	return 0;
}

int led_manager_set_all(uint8_t onoff)
{
	for (int led_index = 0 ; led_index < MAX_LED_NUM; led_index++) {
		led_manager_set_display(led_index, onoff, DISPLAY_FOREVER, NULL);
	}

	return 0;
}

int led_manager_sleep(void)
{
#ifdef BOARD_LED_MAP
	led_manager_store();
	led_manager_set_all(LED_OFF);
#endif
	return 0;
}

int led_manager_wake_up(void)
{
#ifdef BOARD_LED_MAP
	led_manager_restore();
#endif
	return 0;
}

int led_manager_store(void)
{
	struct led_manager_ctx_t *ctx = _led_manager_get_ctx();
	struct led_image *image_history = mem_malloc(sizeof(struct led_image));

	if (!image_history)
		return -ENOMEM;

	os_mutex_lock(&led_manager_mutex, OS_FOREVER);

	memcpy(image_history, &ctx->image, sizeof(struct led_image));

	sys_slist_append(&ctx->image_list, (sys_snode_t *)image_history);

	os_mutex_unlock(&led_manager_mutex);
	return 0;
}


int led_manager_restore(void)
{
#ifdef BOARD_LED_MAP
	struct led_manager_ctx_t *ctx = _led_manager_get_ctx();
	struct led_image *image_history = NULL;

	os_mutex_lock(&led_manager_mutex, OS_FOREVER);

	image_history = (struct led_image *)sys_slist_peek_tail(&ctx->image_list);

	if (image_history) {
		memcpy(&ctx->image, image_history, sizeof(struct led_image));
		sys_slist_find_and_remove(&ctx->image_list, (sys_snode_t *)image_history);
		mem_free(image_history);
	} else {
		SYS_LOG_ERR("no history");
		return -ENODEV;
	}

	_led_manager_update_state();

	os_mutex_unlock(&led_manager_mutex);
#endif
	return 0;
}

int led_manager_init(void)
{
#ifdef BOARD_LED_MAP
	struct led_manager_ctx_t *led_manager_ctx = _led_manager_get_ctx();

	memset(led_manager_ctx, 0, sizeof(struct led_manager_ctx_t));

	sys_slist_init(&led_manager_ctx->image_list);

	if (sys_monitor_add_work(_led_manager_work_handle)) {
		SYS_LOG_ERR("add work failed\n");
		return -EFAULT;
	}
	led_manager_ctx->timeout = DISPLAY_FOREVER;
#endif

	return 0;
}

int led_manager_deinit(void)
{
	return 0;
}
