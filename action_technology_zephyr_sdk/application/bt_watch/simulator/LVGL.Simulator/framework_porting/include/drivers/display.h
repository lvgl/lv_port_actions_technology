/*
 * Copyright (c) 2017 Jan Van Winkel <jan.van_winkel@dxplore.eu>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public API for display drivers and applications
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_DISPLAY_H_
#define ZEPHYR_INCLUDE_DRIVERS_DISPLAY_H_

/**
 * @brief Display Interface
 * @defgroup display_interface Display Interface
 * @ingroup display_interfaces
 * @{
 */
#include <stddef.h>
#include <sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Display pixel formats
 *
 * Display pixel format enumeration.
 *
 * In case a pixel format consists out of multiple bytes the byte order is
 * big endian.
 */
enum display_pixel_format {
	PIXEL_FORMAT_RGB_888		= BIT(0),
	PIXEL_FORMAT_MONO01		= BIT(1), /* 0=Black 1=White */
	PIXEL_FORMAT_MONO10		= BIT(2), /* 1=Black 0=White */
	PIXEL_FORMAT_ARGB_8888		= BIT(3),
	PIXEL_FORMAT_RGB_565		= BIT(4),
	PIXEL_FORMAT_BGR_565		= BIT(5),

	PIXEL_FORMAT_BGR_888		= BIT(6),
	PIXEL_FORMAT_XRGB_8888		= BIT(7),
	PIXEL_FORMAT_BGRA_6666		= BIT(8),
	PIXEL_FORMAT_RGBA_6666		= BIT(9),
	PIXEL_FORMAT_BGRA_5658		= BIT(10),
	PIXEL_FORMAT_BGRA_5551		= BIT(11),

	PIXEL_FORMAT_I8		= BIT(12),
	PIXEL_FORMAT_I4		= BIT(13),
	PIXEL_FORMAT_I2		= BIT(14),
	PIXEL_FORMAT_I1		= BIT(15),

	PIXEL_FORMAT_A8		= BIT(16),
	PIXEL_FORMAT_A4		= BIT(17),
	PIXEL_FORMAT_A2		= BIT(18),
	PIXEL_FORMAT_A1		= BIT(19),
	PIXEL_FORMAT_A4_LE	= BIT(20), /* little endian */
	PIXEL_FORMAT_A1_LE	= BIT(21), /* little endian */

	PIXEL_FORMAT_ABGR_8888		= BIT(22),
	PIXEL_FORMAT_XBGR_8888		= BIT(23),
	PIXEL_FORMAT_RGBA_5658		= BIT(24),
	PIXEL_FORMAT_BGR_565_SWAP		= BIT(25),
};

enum display_screen_info {
	/**
	 * If selected, one octet represents 8 pixels ordered vertically,
	 * otherwise ordered horizontally.
	 */
	SCREEN_INFO_MONO_VTILED		= BIT(0),
	/**
	 * If selected, the MSB represents the first pixel,
	 * otherwise MSB represents the last pixel.
	 */
	SCREEN_INFO_MONO_MSB_FIRST	= BIT(1),
	/**
	 * Electrophoretic Display.
	 */
	SCREEN_INFO_EPD			= BIT(2),
	/**
	 * Screen has two alternating ram buffers
	 */
	SCREEN_INFO_DOUBLE_BUFFER	= BIT(3),
	/**
	 * Screen has x alignment constrained to width.
	 */
	SCREEN_INFO_X_ALIGNMENT_WIDTH	= BIT(4),
	/**
	 * Screen has y alignment constrained to height.
	 */
	SCREEN_INFO_Y_ALIGNMENT_HEIGHT	= BIT(5),
	/**
	 * Screen has no ram buffers
	 */
	SCREEN_INFO_ZERO_BUFFER	= BIT(6),
	/**
	 * Screen shape is round
	 */
	SCREEN_INFO_ROUND_SHAPE	= BIT(7),
};

/**
 * @enum display_orientation
 * @brief Enumeration with possible display orientation
 *
 */
enum display_orientation {
	DISPLAY_ORIENTATION_NORMAL,
	DISPLAY_ORIENTATION_ROTATED_90,
	DISPLAY_ORIENTATION_ROTATED_180,
	DISPLAY_ORIENTATION_ROTATED_270,
};

/**
 * @struct display_capabilities
 * @brief Structure holding display capabilities
 *
 * @var uint16_t display_capabilities::x_resolution
 * Display resolution in the X direction
 *
 * @var uint16_t display_capabilities::y_resolution
 * Display resolution in the Y direction
 *
 * @var uint8_t display_capabilities::refresh_rate
 * Display refresh rate in Hz
 *
 * @var uint32_t display_capabilities::supported_pixel_formats
 * Bitwise or of pixel formats supported by the display
 *
 * @var uint32_t display_capabilities::screen_info
 * Information about display panel
 *
 * @var enum display_pixel_format display_capabilities::current_pixel_format
 * Currently active pixel format for the display
 *
 * @var enum display_orientation display_capabilities::current_orientation
 * Current display orientation
 */
/** @brief Structure holding display capabilities. */
struct display_capabilities {
	/** Display resolution in the X direction */
	uint16_t x_resolution;
	/** Display resolution in the Y direction */
	uint16_t y_resolution;
	/** Bitwise or of pixel formats supported by the display */
	uint32_t supported_pixel_formats;
	/** Information about display panel */
	uint32_t screen_info;
	/** Currently active pixel format for the display */
	enum display_pixel_format current_pixel_format;
	/** Current display orientation */
	enum display_orientation current_orientation;
	/** Display refresh rate in Hz */
	uint8_t refresh_rate;
};

/**
 * @struct display_buffer_descriptor
 * @brief Structure to describe display data buffer layout
 *
 * @var uint32_t display_buffer_descriptor::buf_size
 * Data buffer size in bytes
 *
 * @var uint32_t display_buffer_descriptor::pixel_format
 * Data buffer pixel format. if 0, fallback to the current pixel format for the display
 *
 * @var uint32_t display_buffer_descriptor::width
 * Data buffer row width in pixels
 *
 * @var uint16_t display_buffer_descriptor::height
 * Data buffer column height in pixels
 *
 * @var uint32_t display_buffer_descriptor::pitch
 * Number of bytes between consecutive rows in the data buffer
 *
 */
/** @brief Structure to describe display data buffer layout */
struct display_buffer_descriptor {
	/** Data buffer size in bytes */
	uint32_t buf_size;
	/** Data buffer pixel format */
	uint32_t pixel_format;
	/** Number of bytes between consecutive rows in the data buffer */
	uint32_t pitch;
	/** Data buffer row width in pixels */
	uint32_t width;
	/** Data buffer column height in pixels */
	uint16_t height;
};

/**
 * @struct display_callback
 * @brief Structure to register specific event callback
 *
 */
struct display_callback {
	/*
	 * (*vsync)() is called when a vsync event is received (called in isr),
	 * timestamp is measured in cycles.
	 */
	void (*vsync)(const struct display_callback * callback, uint32_t timestamp);
	/* (*complete)() is called when refresh complete (called in isr) */
	void (*complete)(const struct display_callback * callback);
	/* (*pm_notify) is called when pm state changed */
	void (*pm_notify)(const struct display_callback * callback, uint32_t pm_action);
};


#ifdef __cplusplus
}
#endif
/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_DISPLAY_H_ */
