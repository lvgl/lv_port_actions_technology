/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public API for display controller drivers
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_DISPLAY_DISPLAY_CONTROLLER_H_
#define ZEPHYR_INCLUDE_DRIVERS_DISPLAY_DISPLAY_CONTROLLER_H_

#include <kernel.h>
#include <device.h>
#include <zephyr/types.h>
#include <drivers/cfg_drv/dev_config.h>
#include "display_graphics.h"

/**
 * @brief Display Controller Interface
 * @defgroup display_controller_interface Display Controller Interface
 * @ingroup display_interfaces
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/* invalid command ID */
#define DC_INVALID_CMD (UINT32_MAX)

#define DISPLAY_PORT_TYPE(major, minor) (((major) << 8) | (minor))
#define DISPLAY_PORT_TYPE_MAJOR(type) (((type) >> 8) & 0xFF)
#define DISPLAY_PORT_TYPE_MINOR(type) ((type) & 0xFF)

/**
 * @brief Enumeration with possible display major port type
 *
 */
#define DISPLAY_PORT_Unknown  (0)
#define DISPLAY_PORT_MCU      (1)
#define DISPLAY_PORT_TR       (2)
#define DISPLAY_PORT_SPI      (4)

/**
 * @brief Enumeration with possible display mcu port type
 *
 */
#define DISPLAY_MCU_8080  (0) /* Intel 8080 */
#define DISPLAY_MCU_6800  (1) /* Moto 6800 */

#define DISPLAY_PORT_MCU_8080  DISPLAY_PORT_TYPE(DISPLAY_PORT_MCU, DISPLAY_MCU_8080)
#define DISPLAY_PORT_MCU_6800  DISPLAY_PORT_TYPE(DISPLAY_PORT_MCU, DISPLAY_MCU_6800)

/**
 * @brief Enumeration with possible display spi port type
 *
 */
#define DISPLAY_SPI_3LINE_1  (0)
#define DISPLAY_SPI_3LINE_2  (1)
#define DISPLAY_SPI_4LINE_1  (2)
#define DISPLAY_SPI_4LINE_2  (3)
#define DISPLAY_QSPI         (4)
#define DISPLAY_QSPI_SYNC    (5)
#define DISPLAY_QSPI_DDR_0   (6)
#define DISPLAY_QSPI_DDR_1   (7)
#define DISPLAY_QSPI_DDR_2   (8)

#define DISPLAY_PORT_SPI_3LINE_1  DISPLAY_PORT_TYPE(DISPLAY_PORT_SPI, DISPLAY_SPI_3LINE_1)
#define DISPLAY_PORT_SPI_3LINE_2  DISPLAY_PORT_TYPE(DISPLAY_PORT_SPI, DISPLAY_SPI_3LINE_2)
#define DISPLAY_PORT_SPI_4LINE_1  DISPLAY_PORT_TYPE(DISPLAY_PORT_SPI, DISPLAY_SPI_4LINE_1)
#define DISPLAY_PORT_SPI_4LINE_2  DISPLAY_PORT_TYPE(DISPLAY_PORT_SPI, DISPLAY_SPI_4LINE_2)
#define DISPLAY_PORT_QSPI         DISPLAY_PORT_TYPE(DISPLAY_PORT_SPI, DISPLAY_QSPI)
#define DISPLAY_PORT_QSPI_SYNC    DISPLAY_PORT_TYPE(DISPLAY_PORT_SPI, DISPLAY_QSPI_SYNC)
#define DISPLAY_PORT_QSPI_DDR_0   DISPLAY_PORT_TYPE(DISPLAY_PORT_SPI, DISPLAY_QSPI_DDR_0)
#define DISPLAY_PORT_QSPI_DDR_1   DISPLAY_PORT_TYPE(DISPLAY_PORT_SPI, DISPLAY_QSPI_DDR_1)
#define DISPLAY_PORT_QSPI_DDR_2   DISPLAY_PORT_TYPE(DISPLAY_PORT_SPI, DISPLAY_QSPI_DDR_2)

/**
 * @brief Enumeration with possible display TR port type
 *
 */
#define DISPLAY_PORT_TR_LCD  DISPLAY_PORT_TYPE(DISPLAY_PORT_TR, 0)

/**
 * @struct display_videoport
 * @brief Structure holding display port configuration
 *
 */
struct display_videoport {
	/* port type */
	union {
		uint16_t type;
		struct {
			uint8_t minor_type;
			uint8_t major_type;
		};
	};

	union {
		/* mcu port */
		struct {
			uint8_t cs : 1; /* chip select */
			uint8_t lsb_first : 1;
			uint8_t bus_width; /* output interface buf width */
			uint8_t clk_low_duration;  /* mem clock low level duration */
			uint8_t clk_high_duration; /* mem clock high level duration */
		} mcu_mode;

		/* spi port */
		struct {
			uint8_t cs : 1; /* chip select */
			uint8_t lsb_first : 1; /* if set, transfer LSB first */
			/**
			 * Clock Polarity: if set, clock idle state will be 1 and active
			 * state will be 0. If untouched, the inverse will be true
			 */
			uint8_t cpol : 1;
			/**
			 * Clock Phase: this dictates when is the data captured, and depends
			 * clock's polarity. When cpol is set and this bit as well,
			 * capture will occur on low to high transition and high to low if
			 * this bit is not set (default). This is fully reversed if CPOL is
			 * not set.
			 */
			uint8_t cpha : 1;
			uint8_t dual_lane : 1; /* dual data lane enable */
			uint8_t dcp_mode : 1; /* data compat mode enable */
			uint8_t rd_lane : 2; /* the data lane where to read data from panel (only for QSPI) */
			uint8_t rd_dummy_cycles; /* read dummy cycles between command and the following data */
			uint8_t rd_delay_ns; /* read data delay in ns */

			uint8_t wr_delay_d0_ns; /* write signal d0 delay in ns */
			uint8_t wr_delay_d1_ns; /* write signal d1 delay in ns */
			uint8_t wr_delay_d2_ns; /* write signal d2 delay in ns */
			uint8_t wr_delay_d3_ns; /* write signal d3 delay in ns */
			uint8_t delay_csx_ns; /* signal csx delay in ns */
			uint8_t delay_scl_ns; /* signal scl delay in ns */

			uint8_t ahb_clk_div; /* AHB clock divisor of AHB duration */
		} spi_mode;

		struct {
			uint8_t low_bit : 3; /* lowest pixel bit */
			uint8_t hck_tail_on : 1; /* HCK tail enable */
			uint8_t vck_on_xrstl : 1; /* VCK continue when XRST low */
			uint8_t vck_on_idle : 1; /* VCK continue when idle */
			uint8_t hck_on_idle : 1; /* HCK continue when idle */
			uint8_t ptl_on : 1; /* patial update enabled */
			uint8_t frp_on : 1; /* Frame refresh pulse enabled */
			uint8_t vcom_inv : 1; /* VCOM inverse */
			uint8_t frp_inv : 1; /* FRP inverse */
			uint8_t xfrp_inv : 1; /* XFRP inverse */
			uint8_t xrst_inv : 1; /* XRST inverse */
			uint8_t vst_inv : 1; /* VST inverse */
			uint8_t hst_inv : 1; /* HST inverse */
			uint8_t vck_inv : 1; /* VCK inverse */
			uint8_t hck_inv : 1; /* HCK inverse */
			uint8_t enb_inv : 1; /* ENB inverse */

			uint32_t tw_xrst : 8;  /* XRST Low width in us */
			uint32_t tw_vcom : 23; /* VCOM Low and high width in us */
			uint32_t td_vst : 8; /* VST delay time in us */
			uint32_t tw_vst : 8; /* VST high width in us */
			uint32_t td_hst : 8; /* HST delay time */
			uint32_t tw_hst : 8; /* HST high width in us */
			uint32_t td_vck : 8; /* VCK delay time */
			uint32_t tw_vck : 11; /* VCK Low and high width */
			uint32_t tp_hck : 2; /* HCK phase */
			uint32_t td_hck : 8; /* HCK divisor by LCD_CLK, multiple of 4 */
			uint32_t ts_enb : 11; /* ENB set-up time */
			uint32_t th_enb : 11; /* ENB hold time */
			uint32_t td_data : 8; /* DATA delay time */
			uint32_t td_enb : 13; /* ENB delay time */
			uint32_t tw_enb : 13; /* ENB high width */
			uint32_t tsm_enb : 11; /* Minimum VCK set-up time in VCK/4 */
			uint32_t thm_enb : 11; /* Minimum VCK hold time in VCK/4 */
			uint32_t twm_vck : 10; /* Minimum VCK width in HCK */
		} tr_mode;
	};
};

/* display timing flags */
enum display_flags {
	/* tearing effect sync flag */
	DISPLAY_FLAGS_TE_LOW      = BIT(0),
	DISPLAY_FLAGS_TE_HIGH     = BIT(1),
	/* horizontal sync flag */
	DISPLAY_FLAGS_HSYNC_LOW   = BIT(2),
	DISPLAY_FLAGS_HSYNC_HIGH  = BIT(3),
	/* vertical sync flag */
	DISPLAY_FLAGS_VSYNC_LOW   = BIT(4),
	DISPLAY_FLAGS_VSYNC_HIGH  = BIT(5),

	/* data enable flag */
	DISPLAY_FLAGS_DE_LOW   = BIT(6),
	DISPLAY_FLAGS_DE_HIGH  = BIT(7),
	/* drive data on pos. edge (rising edge) */
	DISPLAY_FLAGS_PIXDATA_POSEDGE = BIT(8),
	/* drive data on neg. edge (falling edge) */
	DISPLAY_FLAGS_PIXDATA_NEGEDGE = BIT(9),
	/* drive sync on pos. edge (rising edge) */
	DISPLAY_FLAGS_SYNC_POSEDGE    = BIT(10),
	/* drive sync on neg. edge (falling edge) */
	DISPLAY_FLAGS_SYNC_NEGEDGE    = BIT(11),
};

/**
 * @struct display_videomode
 * @brief Structure holding display mode configuration
 *
 */
struct display_videomode {
	uint32_t pixel_format;  /* see enum display_pixel_format */

	/* timing */
	uint32_t pixel_clk;     /* pixel clock in KHz */
	uint16_t refresh_rate;  /* refresh rate in Hz */

	uint16_t hactive;       /* hor. active video in pixels */
	uint16_t hfront_porch;  /* hor. front porch in clock cycles */
	uint16_t hback_porch;   /* hor. back porch in clock cycles */
	uint16_t hsync_len;     /* hor. sync pulse width in clock cycles */

	uint16_t vactive;       /* ver. active video in lines */
	uint16_t vfront_porch;  /* ver. front porch in lines */
	uint16_t vback_porch;   /* ver. back porch in lines */
	uint16_t vsync_len;     /* ver. sync pulse width in lines */

	/* timing flags */
	uint16_t flags; /* display flags */
};

/**
 * @enum display_controller_ctrl_type
 * @brief Enumeration with possible display controller ctrl commands
 *
 */
enum display_controller_ctrl_type {
	DISPLAY_CONTROLLER_CTRL_COMPLETE_CB = 0,
	DISPLAY_CONTROLLER_CTRL_VSYNC_CB, /* vsync or te callback */
	DISPLAY_CONTROLLER_CTRL_HOTPLUG_CB,
};

/**
 * @typedef display_controller_complete_t
 * @brief Callback API executed when display controller complete one frame
 *
 */
typedef void (*display_controller_complete_t)(void *arg);

/**
 * @typedef display_controller_vsync_t
 * @brief Callback API executed when display controller receive vsync or te signal
 *
 */
typedef void (*display_controller_vsync_t)(void *arg, uint32_t timestamp);

/**
 * @typedef display_controller_vsync_t
 * @brief Callback API executed when display controller receive vsync or te signal
 *
 */
typedef void (*display_controller_hotplug_t)(void *arg, int connected);

/**
 * @enum display_controller_source_type
 * @brief Enumeration with possible display controller intput source
 *
 */
enum display_controller_source_type {
	DISPLAY_CONTROLLER_SOURCE_MCU = 0,    /* MCU write */
	DISPLAY_CONTROLLER_SOURCE_ENGINE, /* display engine transfer */
	DISPLAY_CONTROLLER_SOURCE_DMA,    /* DMA transfer */

	DISPLAY_CONTROLLER_NUM_SOURCES,
};

/**
 * @typedef display_controller_control_api
 * @brief Callback API to control display controller device
 * See display_controller_control() for argument description
 */
typedef int (*display_controller_control_api)(
		const struct device *dev, int cmd, void *arg1, void *arg2);

/**
 * @typedef display_controller_enable_api
 * @brief Callback API to enable display controller
 * See display_controller_enable() for argument description
 */
typedef int (*display_controller_enable_api)(const struct device *dev,
		const struct display_videoport *port);

/**
 * @typedef display_controller_disable_api
 * @brief Callback API to disable display controller
 * See display_controller_disable() for argument description
 */
typedef int (*display_controller_disable_api)(const struct device *dev);

/**
 * @typedef display_controller_set_mode_api
 * @brief Callback API to set display mode
 * See display_controller_set_mode() for argument description
 */
typedef int (*display_controller_set_mode_api)(const struct device *dev,
		const struct display_videomode *mode);

/**
 * @typedef display_controller_read_config
 * @brief Callback API to read display configuration
 * See display_controller_read_config() for argument description
 */
typedef int (*display_controller_read_config_api)(const struct device *dev,
		uint32_t cmd, void *buf, uint32_t len);

/**
 * @typedef display_controller_write_config_api
 * @brief Callback API to write display configuration
 * See display_controller_write_config() for argument description
 */
typedef int (*display_controller_write_config_api)(const struct device *dev,
		uint32_t cmd, const void *buf, uint32_t len);

/**
 * @typedef display_controller_read_pixels_api
 * @brief Callback API to read display image when source is MCU write
 * See display_controller_read_pixels() for argument description
 */
typedef int (*display_controller_read_pixels_api)(const struct device *dev,
		uint32_t cmd, const struct display_buffer_descriptor *desc, void *buf);

/**
 * @typedef display_controller_write_pixels_api
 * @brief Callback API to write display image when source is MCU write
 * See display_controller_write_pixels() for argument description
 */
typedef int (*display_controller_write_pixels_api)(
		const struct device *dev, uint32_t cmd, uint32_t hsync_cmd,
		const struct display_buffer_descriptor *desc, const void *buf);

/**
 * @typedef display_controller_set_source_api
 * @brief Callback API to set display image input source device
 * See display_controller_set_source() for argument description
 */
typedef int (*display_controller_set_source_api)(const struct device *dev,
		enum display_controller_source_type source_type,
		const struct device *source_dev);

/**
 * @brief Display Controller driver API
 * API which a display controller driver should expose
 */
struct display_controller_driver_api {
	display_controller_control_api control;
	display_controller_enable_api enable;
	display_controller_disable_api disable;
	display_controller_set_mode_api set_mode;
	display_controller_set_source_api set_source;
	display_controller_read_config_api read_config;
	display_controller_write_config_api write_config;
	display_controller_read_pixels_api read_pixels;
	display_controller_write_pixels_api write_pixels;
};

/**
 * @brief Control display controller
 *
 * @param dev Pointer to device structure
 * @param cmd Control command
 * @param arg1 Control command argument 1
 * @param arg2 Control command argument 2
 *
 * @retval 0 on success else negative errno code.
 */
static inline int display_controller_control(const struct device *dev,
		int cmd, void *arg1, void *arg2)
{
	struct display_controller_driver_api *api =
		(struct display_controller_driver_api *)dev->api;

	return api->control(dev, cmd, arg1, arg2);
}

/**
 * @brief Turn display controller on
 *
 * @param dev Pointer to device structure
 * @param port Pointer to display_videoport structure, which must be static defined,
 *           since display controller will still refer it until next api call.
 *
 * @retval 0 on success else negative errno code.
 */
static inline int display_controller_enable(const struct device *dev,
		const struct display_videoport *port)
{
	struct display_controller_driver_api *api =
		(struct display_controller_driver_api *)dev->api;

	return api->enable(dev, port);
}

/**
 * @brief Turn display controller off
 *
 * @param dev Pointer to device structure
 *
 * @retval 0 on success else negative errno code.
 */
static inline int display_controller_disable(const struct device *dev)
{
	struct display_controller_driver_api *api =
		(struct display_controller_driver_api *)dev->api;

	return api->disable(dev);
}

/**
 * @brief Set display video mode
 *
 * @param dev Pointer to device structure
 * @param mode Pointer to display_mode_set structure, which must be static defined,
 *           since display controller will still refer it until next api call.
 *
 * @retval 0 on success else negative errno code.
 */
static inline int display_controller_set_mode(const struct device *dev,
		const struct display_videomode *mode)
{
	struct display_controller_driver_api *api =
		(struct display_controller_driver_api *)dev->api;

	return api->set_mode(dev, mode);
}

/**
 * @brief Set display controller image input source
 *
 * This routine configs the image input source device, which should be a display
 * accelerator 2D device.
 *
 * @param dev Pointer to device structure
 * @param source_type source type
 * @param source_dev Pointer to structure of source device
 *
 * @retval 0 on success else negative errno code.
 */
static inline int display_controller_set_source(const struct device *dev,
		enum display_controller_source_type source_type, const struct device *source_dev)
{
	struct display_controller_driver_api *api =
		(struct display_controller_driver_api *)dev->api;

	return api->set_source(dev, source_type, source_dev);
}

/**
 * @brief Read configuration data from display
 *
 * @param dev Pointer to device structure
 * @param cmd Read reg command, ignored for DC_INVAL_CMD
 * @param buf Pointer to data array
 * @param len Length of data to read
 *
 * @retval 0 on success else negative errno code.
 */
static inline int display_controller_read_config(const struct device *dev,
		uint32_t cmd, void *buf, uint32_t len)
{
	struct display_controller_driver_api *api =
		(struct display_controller_driver_api *)dev->api;

	return api->read_config(dev, cmd, buf, len);
}

/**
 * @brief Write configuration data to display
 *
 * @param dev Pointer to device structure
 * @param cmd Write reg command, ignored for DC_INVAL_CMD
 * @param buf Pointer to data array
 * @param len Length of data to write
 *
 * @retval 0 on success else negative errno code.
 */
static inline int display_controller_write_config(const struct device *dev,
		uint32_t cmd, const void *buf, uint32_t len)
{
	struct display_controller_driver_api *api =
		(struct display_controller_driver_api *)dev->api;

	return api->write_config(dev, cmd, buf, len);
}

/**
 * @brief Read image data from display
 *
 * @param dev Pointer to device structure
 * @param cmd Read pixel command, ignored for DC_INVAL_CMD
 * @param desc Pointer to a structure describing the buffer layout
 * @param buf Pointer to buffer array
 *
 * @retval 0 on success else negative errno code.
 */
static inline int display_controller_read_pixels(const struct device *dev,
		uint32_t cmd, const struct display_buffer_descriptor *desc, void *buf)
{
	struct display_controller_driver_api *api =
		(struct display_controller_driver_api *)dev->api;

	return api->read_pixels(dev, cmd, desc, buf);
}

/**
 * @brief Write image data to display
 *
 * This routine may return immediately without waiting complete, So the caller must make
 * sure the previous write_pixels() has completed by listening to complete() callback
 * registered by control() with cmd DISPLAY_CONTROLLER_CTRL_COMPLETE_CB.
 *
 * @param dev Pointer to device structure
 * @param cmd Write pixel command (also vsync command), ignored for DC_INVAL_CMD
 * @param hsync_cmd Additional hsync command for sync interface, ignored for DC_INVAL_CMD
 * @param desc Pointer to a structure describing the buffer layout
 * @param buf Pointer to buffer array
 *
 * @retval 0 on success else negative errno code.
 */
static inline int display_controller_write_pixels(
		const struct device *dev, uint32_t cmd, uint32_t hsync_cmd,
		const struct display_buffer_descriptor *desc, const void *buf)
{
	struct display_controller_driver_api *api =
		(struct display_controller_driver_api *)dev->api;

	return api->write_pixels(dev, cmd, hsync_cmd, desc, buf);
}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_DISPLAY_DISPLAY_CONTROLLER_H_ */
