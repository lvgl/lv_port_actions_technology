/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public API for display engine drivers
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_DISPLAY_DISPLAY_ENGINE_H_
#define ZEPHYR_INCLUDE_DRIVERS_DISPLAY_DISPLAY_ENGINE_H_

#include <kernel.h>
#include <device.h>
#include <zephyr/types.h>
#include <drivers/cfg_drv/dev_config.h>
#include "display_graphics.h"

/**
 * @brief Display Engine Interface
 * @defgroup display_engine_interface Display Engine Interface
 * @ingroup display_interfaces
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/* open flags of display engine */
#define DISPLAY_ENGINE_FLAG_HIGH_PRIO  BIT(0)
#define DISPLAY_ENGINE_FLAG_POST       BIT(1) /* For display post */

/**
 * @enum display_engine_mode_type
 * @brief Enumeration with possible display engine modes
 *
 */
enum display_engine_mode_type {
	DISPLAY_ENGINE_MODE_DEFAULT = 0,
	DISPLAY_ENGINE_MODE_DISPLAY_ONLY,
};

/**
 * @enum display_engine_ctrl_type
 * @brief Enumeration with possible display engine ctrl command
 *
 */
enum display_engine_ctrl_type {
	DISPLAY_ENGINE_CTRL_DISPLAY_PORT = 0, /* configure display video port */
	DISPLAY_ENGINE_CTRL_DISPLAY_MODE,     /* configure display video mode */
	/* prepare display refreshing.
	 * arg1 is the callback function, arg2 must point to the display device structure
	 */
	DISPLAY_ENGINE_CTRL_DISPLAY_PREPARE_CB,
	/* start display refreshing
	 * arg1 is the callback function, arg2 must point to the display device structure
	 */
	DISPLAY_ENGINE_CTRL_DISPLAY_START_CB,
	/* stop display refreshing during sync mode
	 * arg1 is the timeout in milliseconds
	 */
	DISPLAY_ENGINE_CTRL_DISPLAY_SYNC_STOP,
	/* work mode
	 */
	DISPLAY_ENGINE_CTRL_WORK_MODE,
};

/**
 * @struct display_engine_capabilities
 * @brief Structure holding display engine capabilities
 *
 * @var uint8_t display_engine_capabilities::num_overlays
 * Maximum number of overlays supported
 *
 * @var uint16_t display_engine_capabilities::max_width
 * X Resolution at maximum
 *
 * @var uint16_t display_engine_capabilities::max_height
 * Y Resolution at maximum
 *
 * @var uint8_t display_engine_capabilities::support_blend_fg
 * Blending constant fg color supported
 *
 * @var uint8_t display_engine_capabilities::support_blend_b
 * Blending constant bg color supported
 *
 * @var uint32_t display_engine_capabilities::supported_input_pixel_formats
 * Bitwise or of input pixel formats supported by the display engine
 *
 * @var uint32_t display_engine_capabilities::supported_output_pixel_formats
 * Bitwise or of output pixel formats supported by the display engine
 *
 * @var uint32_t display_engine_capabilities::supported_rotate_pixel_formats
 * Bitwise or of rotation pixel formats supported by the display engine
 *
 */
struct display_engine_capabilities {
	uint16_t max_width;
	uint16_t max_height;
	uint16_t max_pitch;
	uint16_t num_overlays : 3;
	uint16_t support_fill : 1;
	uint16_t support_blend : 1;
	uint16_t support_blend_fg : 1;
	uint16_t support_blend_bg : 1;
	uint32_t supported_input_pixel_formats;
	uint32_t supported_output_pixel_formats;
	uint32_t supported_rotate_pixel_formats;
};

/**
 * @struct display_engine_rotate_circle
 * @brief Structure holding display engine rotation configuration
 *
 */
typedef struct display_engine_rotate_param {
	uint32_t is_circle : 1;
	uint32_t blend_en : 1;

	/*
	 * the ARGB-8888 color value combined fill color with global alpha, where
	 * 1) RGB channels plus fixed Alpha 0x00 as the fill color
	 * 2) A channel as the global alpha applied to both src image and fill color area
	 */
	display_color_t color;
	/* transform mapping parameters */
	display_matrix_t matrix;

	union {
		struct {
			/* line range [line_start, line_start + dest_height) to do rotation */
			uint16_t line_start;

			/*
			 * The ring area is decided by outer_radius_sq and inner_radius_sq, and the pixles
			 * outside the ring will be filled with fill_color if enable.
			 *
			 * The outer_diameter must be equal to the width - 1 of the source image
			 */
			uint32_t outer_radius_sq; /* outer circle radius square in .2 fixedpoint format */
			uint32_t inner_radius_sq; /* inner circle radius square in .2 fixedpoint format */
		} circle;
	};
} display_engine_transform_param_t;

/**
 * @typedef display_engine_prepare_display_t
 * @brief Callback API executed when display engine prepare refreshing the display
 *
 */
typedef void (*display_engine_prepare_display_t)(void *arg, const display_rect_t *area);

/**
 * @typedef display_engine_start_display_t
 * @brief Callback API executed when display engine start refreshing the display
 *
 */
typedef void (*display_engine_start_display_t)(void *arg);

/**
 * @typedef display_engine_instance_callback_t
 * @brief Callback API executed when any display engine instance transfer complete or error
 *
 */
typedef void (*display_engine_instance_callback_t)(int err_code, uint16_t cmd_seq, void *user_data);

/**
 * @typedef display_engine_control_api
 * @brief Callback API to control display engine device
 * See display_engine_control() for argument description
 */
typedef int (*display_engine_control_api)(const struct device *dev,
		int cmd, void *arg1, void *arg2);

/**
 * @typedef display_engine_open_api
 * @brief Callback API to open display engine
 * See display_engine_open() for argument description
 */
typedef int (*display_engine_open_api)(const struct device *dev, uint32_t flags);

/**
 * @typedef display_engine_close_api
 * @brief Callback API to close display engine
 * See display_engine_close() for argument description
 */
typedef int (*display_engine_close_api)(const struct device *dev, int inst);

/**
 * @typedef display_engine_get_capabilities_api
 * @brief Callback API to get display engine capabilities
 * See display_engine_get_capabilities() for argument description
 */
typedef void (*display_engine_get_capabilities_api)(const struct device *dev,
		struct display_engine_capabilities *capabilities);

/**
 * @typedef display_engine_register_callback_api
 * @brief Callback API to register instance callback
 * See display_engine_register_callback() for argument description
 */
typedef int (*display_engine_register_callback_api)(const struct device *dev,
		int inst, display_engine_instance_callback_t callback, void *user_data);

/**
 * @typedef display_engine_fill_api
 * @brief Callback API to fill color using display engine
 * See display_engine_fill() for argument description
 */
typedef int (*display_engine_fill_api)(const struct device *dev,
		int inst, const display_buffer_t *dest, display_color_t color);

/**
 * @typedef display_engine_blit_api
 * @brief Callback API to blit using display engine
 * See display_engine_blit() for argument description
 */
typedef int (*display_engine_blit_api)(const struct device *dev,
		int inst, const display_buffer_t *dest, const display_buffer_t *src);

/**
 * @typedef display_engine_blend_api
 * @brief Callback API to blend using display engine
 * See display_engine_blend() for argument description
 */
typedef int (*display_engine_blend_api)(const struct device *dev,
		int inst, const display_buffer_t *dest,
		const display_buffer_t *fg, display_color_t fg_color,
		const display_buffer_t *bg, display_color_t bg_color);

/**
 * @typedef display_engine_transform_api
 * @brief Callback API to transform using display engine
 * See display_engine_transform() for argument description
 */
typedef int (*display_engine_transform_api)(const struct device *dev,
		int inst, const display_buffer_t *dest, const display_buffer_t *src,
		const display_engine_transform_param_t *param);

/**
 * @typedef display_engine_compose_api
 * @brief Callback API to compose using display engine
 * See display_engine_compose() for argument description
 */
typedef int (*display_engine_compose_api)(const struct device *dev,
		int inst, const display_buffer_t *target,
		const display_layer_t *layers, int num_ovls);

/**
 * @typedef display_engine_set_clut_api
 * @brief Callback API to set CLUT
 * See display_engine_set_clut() for argument description
 */
typedef int (*display_engine_set_clut_api)(const struct device *dev,
		int inst, uint16_t layer_idx, uint16_t size, const uint32_t *clut);

/**
 * @typedef display_engine_poll_api
 * @brief Callback API to poll complete of display engine
 * See display_engine_poll() for argument description
 */
typedef int (*display_engine_poll_api)(const struct device *dev,
		int inst, int timeout_ms);

/**
 * @brief Display Engine driver API
 * API which a display engine driver should expose
 */
struct display_engine_driver_api {
	display_engine_control_api control;
	display_engine_open_api open;
	display_engine_close_api close;
	display_engine_get_capabilities_api get_capabilities;
	display_engine_register_callback_api register_callback;
	display_engine_fill_api fill;
	display_engine_blit_api blit;
	display_engine_blend_api blend;
	display_engine_transform_api transform;
	display_engine_compose_api compose;
	display_engine_set_clut_api set_clut;
	display_engine_poll_api poll;
};

/**
 * @brief Control display engine
 *
 * @param dev Pointer to device structure
 * @param cmd Control command
 * @param arg1 Control command argument 1
 * @param arg2 Control command argument 2
 *
 * @retval 0 on success else negative errno code.
 */
static inline int display_engine_control(const struct device *dev, int cmd,
		void *arg1, void *arg2)
{
	struct display_engine_driver_api *api =
		(struct display_engine_driver_api *)dev->api;

	if (api->control) {
		return api->control(dev, cmd, arg1, arg2);
	}

	return -ENOTSUP;
}

/**
 * @brief Open display engine
 *
 * The instance is not thread safe, and must be referred in the same thread.
 *
 * @param dev Pointer to device structure
 * @param flags flags to display engine
 *
 * @retval instance id on success else negative errno code.
 */
static inline int display_engine_open(const struct device *dev, uint32_t flags)
{
	struct display_engine_driver_api *api =
		(struct display_engine_driver_api *)dev->api;

	return api->open(dev, flags);
}

/**
 * @brief Close display engine
 *
 * The caller must make sure all the commands belong to the inst have completed before close.
 *
 * @param dev Pointer to device structure
 * @param inst instance id return in open()
 *
 * @retval 0 on success else negative errno code.
 */
static inline int display_engine_close(const struct device *dev, int inst)
{
	struct display_engine_driver_api *api =
		(struct display_engine_driver_api *)dev->api;

	return api->close(dev, inst);
}

/**
 * @brief Get display engine capabilities
 *
 * @param dev Pointer to device structure
 * @param capabilities Pointer to capabilities structure to populate
 */
static inline void display_engine_get_capabilities(const struct device *dev,
		struct display_engine_capabilities *capabilities)
{
	struct display_engine_driver_api *api =
		(struct display_engine_driver_api *)dev->api;

	api->get_capabilities(dev, capabilities);
}

/**
 * @brief Register display engine instance callback
 *
 * This procedure will only succeed when display engine instance is not busy, and the
 * registered callback may be called in isr.
 *
 * @param dev Pointer to device structure
 * @param inst Instance id returned in open()
 * @param callback callback function
 * @param user_data callback parameter "user_data"
 *
 * @retval 0 on success else negative errno code.
 */
static inline int display_engine_register_callback(const struct device *dev,
		int inst, display_engine_instance_callback_t callback, void *user_data)
{
	struct display_engine_driver_api *api =
		(struct display_engine_driver_api *)dev->api;

	return api->register_callback(dev, inst, callback, user_data);
}

/**
 * @brief Fill color
 *
 * @param dev Pointer to device structure
 * @param inst Instance id returned in open()
 * @param dest Pointer to the filling display framebuffer
 * @param color The filling color
 *
 * @retval command sequence (uint16_t) on success else negative errno code.
 */
static inline int display_engine_fill(const struct device *dev,
		int inst, const display_buffer_t *dest, display_color_t color)
{
	struct display_engine_driver_api *api =
		(struct display_engine_driver_api *)dev->api;

	return api->fill(dev, inst, dest, color);
}

/**
 * @brief Blit source fb to destination fb
 *
 * @param dev Pointer to device structure
 * @param inst Instance id returned in open()
 * @param dest Pointer to the destination fb
 * @param src Pointer to the source fb
 *
 * @retval command sequence (uint16_t) on success else negative errno code.
 */
static inline int display_engine_blit(const struct device *dev,
		int inst, const display_buffer_t *dest, const display_buffer_t *src)
{
	struct display_engine_driver_api *api =
		(struct display_engine_driver_api *)dev->api;

	return api->blit(dev, inst, dest, src);
}

/**
 * @brief Blending source fb over destination fb
 *
 * @param dev Pointer to device structure
 * @param inst Instance id returned in open()
 * @param dest Pointer to the destination fb
 * @param fg Pointer to the foreground fb, can be NULL
 * @param fg_color Foreground color
         1) used as fg plane color, when fg is NULL
         2) used as fg pixel rgb color component, when fg format is A8 (A4/1 not allowed)
         3) used as fg plane alpha, when fg format is not A8
 * @param bg Pointer to the background fb
 * @param bg Background color
         1) used as bg plane color, when fg is NULL
         2) used as bg pixel rgb color component, when fg format is A8 (A4/1 not allowed)
         3) used as bg plane alpha, when fg format is not A8
 * @param plane_alpha plane alpha applied to blending
 *
 * @retval command sequence (uint16_t) on success else negative errno code.
 */
static inline int display_engine_blend(const struct device *dev,
		int inst, const display_buffer_t *dest,
		const display_buffer_t *fg, display_color_t fg_color,
		const display_buffer_t *bg, display_color_t bg_color)
{
	struct display_engine_driver_api *api =
		(struct display_engine_driver_api *)dev->api;

	return api->blend(dev, inst, dest, fg, fg_color, bg, bg_color);
}

/**
 * @brief Transform an image to a framebuffer
 *
 * @param dev Pointer to device structure
 * @param inst Instance id returned in open()
 * @param dest Pointer to the destination fb
 * @param param Pointer to structure display_engine_rotate_param
 * @param src Pointer to the source fb whose address must be the origin image address before transformation
 * @retval command sequence (uint16_t) on success else negative errno code.
 */
static inline int display_engine_transform(const struct device *dev,
		int inst, const display_buffer_t *dest, const display_buffer_t *src,
		const display_engine_transform_param_t *param)
{
	struct display_engine_driver_api *api =
		(struct display_engine_driver_api *)dev->api;

	if (api->transform) {
		return api->transform(dev, inst, dest, src, param);
	}

	return -ENOTSUP;
}

/**
 * @brief Composing layers to diplay device or target framebuffer
 *
 * @param dev Pointer to device structure
 * @param inst Instance id returned in open()
 * @param target Target buffer, set NULL if output to display device
 * @param layers Array of layers
 * @param num_layers Number of layers
 *
 * @retval command sequence (uint16_t) on success else negative errno code.
 */
static inline int display_engine_compose(const struct device *dev,
		int inst, const display_buffer_t *target,
		const display_layer_t *layers, int num_layers)
{
	struct display_engine_driver_api *api =
		(struct display_engine_driver_api *)dev->api;

	return api->compose(dev, inst, target, layers, num_layers);
}

/**
 * @brief Composing layers to diplay device or target framebuffer
 *
 * @param dev Pointer to device structure
 * @param inst Instance id returned in open()
 * @param layer_idx layer index to upload clut
 * @param size Number of colors in the color look up table
 * @param clut Pointer to the color look up table
 *
 * @retval command sequence (uint16_t) on success else negative errno code.
 */
static inline int display_engine_set_clut(const struct device *dev,
		int inst, uint16_t layer_idx, uint16_t size, const uint32_t *clut)
{
	struct display_engine_driver_api *api =
		(struct display_engine_driver_api *)dev->api;

	if (api->set_clut) {
		return api->set_clut(dev, inst, layer_idx, size, clut);
	}

	return -ENOTSUP;
}

/**
 * @brief Polling display engine complete
 *
 * @param dev Pointer to device structure
 * @param inst Instance id returned in open()
 * @param timeout_ms Timeout duration in milliseconds
 *
 * @retval 0 on success else negative errno code.
 */
static inline int display_engine_poll(const struct device *dev,
		int inst, int timeout_ms)
{
	struct display_engine_driver_api *api =
		(struct display_engine_driver_api *)dev->api;

	return api->poll(dev, inst, timeout_ms);
}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_DISPLAY_DISPLAY_ENGINE_H_ */
