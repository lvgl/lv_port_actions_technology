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
#include <string.h>
#include <sys_monitor.h>

#define SYS_LOG_NO_NEWLINE
#ifdef SYS_LOG_DOMAIN
#undef SYS_LOG_DOMAIN
#endif
#define SYS_LOG_DOMAIN "seg_led_manager"

#include <seg_led_manager.h>

struct seg_led_image {
	sys_snode_t snode;
	uint8_t num[4];
	uint8_t icon;
	uint8_t flash_state:1;
	uint8_t num_state:4;
	uint8_t flash_cnt;
	uint8_t flash_period;
	uint8_t current_period;
	uint16_t flash_map;
	seg_led_flash_callback cb;
};

struct seg_led_manager_ctx_t {
	struct device *dev;
	struct seg_led_image image;
	uint8_t timeout;
	uint8_t update_direct:1;
	uint8_t timeout_event_lock:1;
	seg_led_timeout_callback timeout_cb;
	sys_slist_t	image_list;
};

static struct seg_led_manager_ctx_t global_seg_led_manager_ctx;

OS_MUTEX_DEFINE(seg_led_manager_mutex);

static struct seg_led_manager_ctx_t *_seg_led_get_ctx(void)
{
	return &global_seg_led_manager_ctx;
}

static int _seg_led_display_update(void)
{
	int i = 0;
	struct seg_led_manager_ctx_t *ctx = _seg_led_get_ctx();

	if (ctx->timeout_event_lock || ctx->update_direct) {
		return 0;
	}

	for (i = 0 ; i < 8; i++) {
		bool display = (((1 << i) & ctx->image.icon) != 0);

		if ((ctx->image.flash_map & (1 << i))) {
			display = ctx->image.flash_state;
		}

		segled_display_icon(ctx->dev, i, display);
	}

	for (i = 0 ; i < 4; i++) {
		bool display = (((1 << i) & ctx->image.num_state) != 0);

		if ((ctx->image.flash_map & (1 << (i + 8)))) {
			display = ctx->image.flash_state;
		}

		segled_display_number(ctx->dev, i, ctx->image.num[i], display);
	}

	return 0;
}

int seg_led_display_number(uint8_t num_addr, uint8_t c, bool display)
{
	int ret = 0;

	struct seg_led_manager_ctx_t *ctx = _seg_led_get_ctx();

	os_mutex_lock(&seg_led_manager_mutex, OS_FOREVER);

	num_addr -= SLED_NUMBER1;

	if (!ctx->update_direct) {

		ctx->image.num[num_addr] = c;

		if (display) {
			ctx->image.num_state |= (1 << num_addr);
		} else {
			ctx->image.num_state &= (~(1 << num_addr));
		}
	}

	if (!ctx->timeout_event_lock || ctx->update_direct) {
		ret = segled_display_number(ctx->dev, num_addr, c, display);
	}

	os_mutex_unlock(&seg_led_manager_mutex);

	return ret;
}

int seg_led_display_string(uint8_t start_pos, const uint8_t *str, bool display)
{
	int ret = 0;
	struct seg_led_manager_ctx_t *ctx = _seg_led_get_ctx();
	int len = strlen(str);

	os_mutex_lock(&seg_led_manager_mutex, OS_FOREVER);

	start_pos -= SLED_NUMBER1;

	if (len  + start_pos > sizeof(ctx->image.num)) {
		len = sizeof(ctx->image.num) - start_pos;
	}

	if (!ctx->update_direct) {
		for (int i = start_pos; i < (len + start_pos); i++) {
			if (display) {
				ctx->image.num_state |= (1 << i);
			} else {
				ctx->image.num_state &= (~(1 << i));
			}
		}

		memcpy(&ctx->image.num[start_pos], str, len);
	}

	if (!ctx->timeout_event_lock || ctx->update_direct) {
		ret = segled_display_number_string(ctx->dev, start_pos, str, display);
	}

	os_mutex_unlock(&seg_led_manager_mutex);

	return ret;
}

int seg_led_display_icon(uint8_t icon_idx, bool display)
{
	int ret = 0;
	struct seg_led_manager_ctx_t *ctx = _seg_led_get_ctx();

	os_mutex_lock(&seg_led_manager_mutex, OS_FOREVER);

	if (!ctx->update_direct) {
		if (display) {
			ctx->image.icon |= (1 << icon_idx);
		} else {
			ctx->image.icon &= (~(1 << icon_idx));
		}

		ctx->image.flash_map &=  ~(1 << icon_idx);
		if (!ctx->image.flash_map) {
			ctx->image.flash_state = 0;
		}
	}

	if (!ctx->timeout_event_lock || ctx->update_direct) {
		ret = segled_display_icon(ctx->dev, icon_idx, display);
	}

	os_mutex_unlock(&seg_led_manager_mutex);

	return ret;
}

int seg_led_display_set_flash(uint16_t flash_map, uint16_t flash_period, uint16_t flash_cnt, seg_led_flash_callback cb)
{
	struct seg_led_manager_ctx_t *ctx = _seg_led_get_ctx();

	os_mutex_lock(&seg_led_manager_mutex, OS_FOREVER);
	ctx->image.flash_map = flash_map;
	ctx->image.flash_period  = flash_period / CONFIG_MONITOR_PERIOD / 2;

	if (flash_cnt < FLASH_FOREVER / 2) {
		ctx->image.flash_cnt  = flash_cnt * 2;
	} else {
		ctx->image.flash_cnt = FLASH_FOREVER;
	}

	ctx->image.flash_state = 1;
	ctx->image.current_period = ctx->image.flash_period;
	ctx->image.cb = cb;
	_seg_led_display_update();

	os_mutex_unlock(&seg_led_manager_mutex);

	return 0;

}


int seg_led_manager_clear_screen(uint8_t clr_mode)
{
	int ret = 0;

	struct seg_led_manager_ctx_t *ctx = _seg_led_get_ctx();

	os_mutex_lock(&seg_led_manager_mutex, OS_FOREVER);

	if (!ctx->update_direct && clr_mode == LED_CLEAR_ALL) {
		memset(&ctx->image, 0, sizeof(struct seg_led_image));
	}

	if (!ctx->timeout_event_lock || ctx->update_direct) {
		ret = segled_clear_screen(ctx->dev, clr_mode);
	}

	os_mutex_unlock(&seg_led_manager_mutex);

	return ret;
}

int seg_led_manager_sleep(void)
{
	int ret = 0;
	struct seg_led_manager_ctx_t *ctx = _seg_led_get_ctx();

	seg_led_manager_store();
	os_mutex_lock(&seg_led_manager_mutex, OS_FOREVER);
	ret = segled_sleep(ctx->dev);
	os_mutex_unlock(&seg_led_manager_mutex);
	return ret;
}

int seg_led_manager_wake_up(void)
{
	int ret = 0;
	struct seg_led_manager_ctx_t *ctx = _seg_led_get_ctx();

	os_mutex_lock(&seg_led_manager_mutex, OS_FOREVER);
	ret = segled_wakeup(ctx->dev);
	os_mutex_unlock(&seg_led_manager_mutex);
	seg_led_manager_restore();
	return ret;
}

int seg_led_manager_store(void)
{
	struct seg_led_manager_ctx_t *ctx = _seg_led_get_ctx();
	struct seg_led_image *image_history = mem_malloc(sizeof(struct seg_led_image));

	if (!image_history)
		return -ENOMEM;

	os_mutex_lock(&seg_led_manager_mutex, OS_FOREVER);

	memcpy(image_history, &ctx->image, sizeof(struct seg_led_image));

	sys_slist_append(&ctx->image_list, (sys_snode_t *)image_history);

	os_mutex_unlock(&seg_led_manager_mutex);

	return 0;
}

int seg_led_manager_restore(void)
{
	struct seg_led_manager_ctx_t *ctx = _seg_led_get_ctx();
	struct seg_led_image *image_history = (struct seg_led_image *)sys_slist_peek_tail(&ctx->image_list);

	os_mutex_lock(&seg_led_manager_mutex, OS_FOREVER);

	if (image_history) {
		memcpy(&ctx->image, image_history, sizeof(struct seg_led_image));
		sys_slist_find_and_remove(&ctx->image_list, (sys_snode_t *)image_history);
		mem_free(image_history);
	} else {
		SYS_LOG_ERR("no history");
		return -ENODEV;
	}

	_seg_led_display_update();
	os_mutex_unlock(&seg_led_manager_mutex);
	return 0;
}

int seg_led_manager_set_timeout_restore(int timeout)
{
	struct seg_led_manager_ctx_t *ctx = _seg_led_get_ctx();

	os_mutex_lock(&seg_led_manager_mutex, OS_FOREVER);

	if (ctx->timeout == FLASH_FOREVER) {
		os_mutex_unlock(&seg_led_manager_mutex);
		seg_led_manager_store();
		os_mutex_lock(&seg_led_manager_mutex, OS_FOREVER);
	}
	ctx->timeout = timeout / CONFIG_MONITOR_PERIOD;
	ctx->timeout_cb = seg_led_manager_restore;
	os_mutex_unlock(&seg_led_manager_mutex);
	return 0;
}

int seg_led_manager_set_timeout_event_start(void)
{
	struct seg_led_manager_ctx_t *ctx = _seg_led_get_ctx();

	os_mutex_lock(&seg_led_manager_mutex, OS_FOREVER);
	ctx->update_direct = 1;
	ctx->timeout_event_lock = 1;
	os_mutex_unlock(&seg_led_manager_mutex);
	return 0;
}


int seg_led_manager_set_timeout_event(int timeout, seg_led_flash_callback cb)
{
	struct seg_led_manager_ctx_t *ctx = _seg_led_get_ctx();

	os_mutex_lock(&seg_led_manager_mutex, OS_FOREVER);
	ctx->timeout_cb = cb;
	ctx->timeout = timeout / CONFIG_MONITOR_PERIOD;
	ctx->update_direct = 0;

	os_mutex_unlock(&seg_led_manager_mutex);
	return 0;
}

static int _seg_led_manager_work_handle(void)
{
	struct seg_led_manager_ctx_t *ctx = _seg_led_get_ctx();

	os_mutex_lock(&seg_led_manager_mutex, OS_FOREVER);

	if (ctx->timeout != FLASH_FOREVER) {
		if (ctx->timeout-- == 0) {
			ctx->timeout = FLASH_FOREVER;
			ctx->update_direct = 0;
			ctx->timeout_event_lock = 0;
			_seg_led_display_update();
			os_mutex_unlock(&seg_led_manager_mutex);

			if (ctx->timeout_cb) {
				ctx->timeout_cb();
				ctx->timeout_cb = NULL;
			}
			return 0;
		}
	}

	ctx->image.current_period--;
	if (ctx->image.flash_map != 0
		&& !ctx->image.current_period) {

		ctx->image.current_period = ctx->image.flash_period;

		if (ctx->image.flash_state) {
			ctx->image.flash_state = 0;
		} else {
			ctx->image.flash_state = 1;
		}
		_seg_led_display_update();
		if (ctx->image.flash_cnt != FLASH_FOREVER) {
			ctx->image.flash_cnt--;
			if (ctx->image.flash_cnt == 0) {
				ctx->image.flash_map = 0;
				if (ctx->image.cb) {
					os_mutex_unlock(&seg_led_manager_mutex);
					ctx->image.cb();
					os_mutex_lock(&seg_led_manager_mutex, OS_FOREVER);
				}
			}
		}
	}

	os_mutex_unlock(&seg_led_manager_mutex);

	return 0;
}

int seg_led_manager_init(void)
{
	memset(&global_seg_led_manager_ctx, 0, sizeof(struct seg_led_manager_ctx_t));

	global_seg_led_manager_ctx.dev = device_get_binding(CONFIG_SEG_LED_DEVICE_NAME);

	if (!global_seg_led_manager_ctx.dev)
		return -ENODEV;

	if (sys_monitor_add_work(_seg_led_manager_work_handle)) {
		SYS_LOG_ERR("add work failed\n");
		return -EFAULT;
	}

	sys_slist_init(&global_seg_led_manager_ctx.image_list);

	global_seg_led_manager_ctx.timeout = FLASH_FOREVER;

	return 0;
}

int seg_led_manager_deinit(void)
{
	return 0;
}
