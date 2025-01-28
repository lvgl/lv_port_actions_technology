/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <spicache.h>
#include <drivers/cfg_drv/dev_config.h>
#include <drivers/display.h>
#include <drivers/display/display_engine.h>
#include <drivers/display/display_controller.h>
#include <assert.h>
#include <string.h>

#include "lcdc_lark.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(lcdc, CONFIG_DISPLAY_LOG_LEVEL);

#define USE_LCDC_TE 0

#define SUPPORTED_PIXEL_FORMATS (PIXEL_FORMAT_BGR_565 | PIXEL_FORMAT_RGB_888 | \
		PIXEL_FORMAT_BGR_888 | PIXEL_FORMAT_ARGB_8888 | PIXEL_FORMAT_XRGB_8888)

#define LCDC     ((LCDC_Type *)LCDC_REG_BASE)
#define DMACHAN  ((LCDC_DMACHAN_CTL_Type *)(DMA_REG_BASE + 0x100 + CONFIG_LCDC_DMA_CHAN_ID * 0x100))
#define DMALINE  ((LCDC_DMALINE_CTL_Type *)(DMA_LINE0_REG_BASE + 0x20 * CONFIG_LCDC_DMA_LINE_ID))

#define SPI_AHB_CMD_MASK  \
		(LCD_SPI_SRC_MASK | LCD_SPI_AHB_CSX_MASK | LCD_SPI_CDX_MASK)
#define SPI_AHB_DATA_MASK \
		(SPI_AHB_CMD_MASK | LCD_SPI_AHB_F565_MASK | \
		 LCD_SPI_AHB_CFG_DATA | LCD_SPI_RWL_MASK)

#define CPU_AHB_CMD_MASK \
		(LCD_CPU_SRC_MASK | LCD_CPU_AHB_F565_MASK | \
		 LCD_CPU_RS_MASK | LCD_CPU_AHB_DATA_MASK | LCD_CPU_AHB_CSX_MASK)
#define CPU_AHB_DATA_MASK CPU_AHB_CMD_MASK

struct lcdc_data {
	/* pointer to current video port passed in interface enable() */
	const struct display_videoport *port;
	/* pointer to current video mode passed in interface set_mode() */
	const struct display_videomode *mode;
	/* enum display_controller_source_type */
	uint8_t source_type;

	display_controller_complete_t complete_fn;
	void *complete_fn_arg;

	bool busy;
};

static int lcdc_config_port(const struct device *dev, const struct display_videoport *port)
{
	uint32_t lcd_ctl = LCD_EN | LCD_CLK_EN;

	if (port->type != DISPLAY_PORT_QSPI_SYNC) {
		lcd_ctl |= LCD_HOLD_EN;
	}

	switch (port->major_type) {
	case DISPLAY_PORT_MCU:
		lcd_ctl |= (port->minor_type == DISPLAY_MCU_8080) ?
				LCD_IF_SEL_MCU_8080 : LCD_IF_SEL_MCU_6880;
		lcd_ctl |= LCD_IF_CE_SEL(port->mcu_mode.cs) | LCD_IF_MLS_SEL(port->mcu_mode.lsb_first);
		LCDC->CPU_CTL = LCD_CPU_FEND_IRQ_EN | LCD_CPU_AHB_F565_16BIT | LCD_CPU_AHB_CSX(1);
		LCDC->CPU_CLK = LCD_CPU_CLK(port->mcu_mode.clk_high_duration,
				port->mcu_mode.clk_low_duration, port->mcu_mode.clk_low_duration);
		break;

	case DISPLAY_PORT_SPI:
		lcd_ctl |= LCD_IF_SEL_SPI;
		lcd_ctl |= LCD_IF_CE_SEL(port->spi_mode.cs) | LCD_IF_MLS_SEL(port->spi_mode.lsb_first);
		LCDC->SPI_CTL = LCD_SPI_FTC_IRQ_EN | LCD_SPI_FEND_PD_EN |
				LCD_SPI_CDX(1) | LCD_SPI_AHB_CSX(1) | LCD_SPI_AHB_F565_16BIT |
				LCD_SPI_TYPE_SEL(port->minor_type == LCD_QSPI_SYNC ? LCD_QSPI : port->minor_type) |
				LCD_SPI_SCLK_POL((port->spi_mode.cpol == port->spi_mode.cpha) ? 0 : 1) |
				LCD_SPI_DELAY_CHAIN_SEL(port->spi_mode.rd_delay_ns) |
				LCD_SPI_RDLC_SEL(port->spi_mode.rd_dummy_cycles) |
				LCD_SPI_DCP_SEL(port->spi_mode.dcp_mode) |
				LCD_SPI_DUAL_LANE_SEL(port->spi_mode.dual_lane);
		break;

	default:
		return -ENOTSUP;
	}

	LCDC->CTL = lcd_ctl;

	return 0;
}

static int lcdc_config_src_pixel_format(const struct device *dev,
		uint32_t pixel_format, uint8_t *bytes_per_pixel)
{
	struct lcdc_data *data = dev->data;

	if (pixel_format == PIXEL_FORMAT_BGR_565) {
		*bytes_per_pixel = 2;

		switch (data->port->major_type) {
		case DISPLAY_PORT_SPI:
			LCDC->SPI_CTL = (LCDC->SPI_CTL & ~LCD_SPI_SDT_MASK) | LCD_SPI_SDT_RGB565;
			break;
		default:
		case DISPLAY_PORT_MCU:
			LCDC->CPU_CTL = (LCDC->CPU_CTL & ~LCD_SPI_SDT_MASK) | LCD_CPU_SDT_RGB565;
			break;
		}
	} else if (pixel_format == PIXEL_FORMAT_ARGB_8888 ||
			   pixel_format == PIXEL_FORMAT_XRGB_8888) {
		*bytes_per_pixel = 4;

		switch (data->port->major_type) {
		case DISPLAY_PORT_SPI:
			LCDC->SPI_CTL = (LCDC->SPI_CTL & ~LCD_SPI_SDT_MASK) | LCD_SPI_SDT_ARGB8888;
			break;
		case DISPLAY_PORT_MCU:
		default:
			LCDC->CPU_CTL = (LCDC->CPU_CTL & ~LCD_SPI_SDT_MASK) | LCD_CPU_SDT_ARGB8888;
			break;
		}
	} else if (pixel_format == PIXEL_FORMAT_RGB_888) {
		*bytes_per_pixel = 3;

		switch (data->port->major_type) {
		case DISPLAY_PORT_SPI:
			LCDC->SPI_CTL = (LCDC->SPI_CTL & ~LCD_SPI_SDT_MASK) | LCD_SPI_SDT_RGB888;
			break;
		case DISPLAY_PORT_MCU:
		default:
			LCDC->CPU_CTL = (LCDC->CPU_CTL & ~LCD_SPI_SDT_MASK) | LCD_CPU_SDT_RGB888;
			break;
		}
	} else if (pixel_format == PIXEL_FORMAT_BGR_888) {
		*bytes_per_pixel = 3;

		switch (data->port->major_type) {
		case DISPLAY_PORT_SPI:
			LCDC->SPI_CTL = (LCDC->SPI_CTL & ~LCD_SPI_SDT_MASK) | LCD_SPI_SDT_BGR888;
			break;
		case DISPLAY_PORT_MCU:
		default:
			LCDC->CPU_CTL = (LCDC->CPU_CTL & ~LCD_SPI_SDT_MASK) | LCD_CPU_SDT_BGR888;
			break;
	} else {
		return -EINVAL;
	}

	return 0;
}

static int lcdc_config_mode(const struct device *dev, const struct display_videomode *mode)
{
	struct lcdc_data *data = dev->data;

	clk_set_rate(CLOCK_ID_LCD, KHZ(mode->pixel_clk));

	LCDC->CTL &= ~(LCD_TE_EN | LCD_TE_MODE_MASK | LCD_OUT_FORMAT_MASK);

#if USE_LCDC_TE
	if (mode->flags & DISPLAY_FLAGS_TE_HIGH) {
		LCDC->CTL |= LCD_TE_EN | LCD_TE_MODE_RISING_EDGE;
	} else if (mode->flags & DISPLAY_FLAGS_TE_LOW) {
		LCDC->CTL |= LCD_TE_EN | LCD_TE_MODE_FALLING_EDGE;
	}
#endif

	switch (mode->pixel_format) {
	case PIXEL_FORMAT_BGR_565:
		LCDC->CTL |= LCD_OUT_FORMAT_RGB565;
		break;
	case PIXEL_FORMAT_RGB_888:
	case PIXEL_FORMAT_BGR_888:
	case PIXEL_FORMAT_ARGB_8888:
	case PIXEL_FORMAT_XRGB_8888:
	default:
		LCDC->CTL |= LCD_OUT_FORMAT_RGB888;
		break;
	}

	if (data->port->type == DISPLAY_PORT_QSPI_SYNC) {
		uint16_t data_cycles = mode->hactive *
			display_format_get_bits_per_pixel(mode->pixel_format) / 4;

		LCDC->QSPI_DTAS = LCD_QSPI_DTAS(0, data_cycles);
		LCDC->QSPI_SYNC_TIM = LCD_QSPI_SYNC_TIM(
				mode->hsync_len + mode->hback_porch + mode->hfront_porch,
				mode->vfront_porch, mode->vback_porch);
	}

	return 0;
}

static int lcdc_config_source(const struct device *dev,
		enum display_controller_source_type source_type)
{
	static const uint32_t mcu_source_set[DISPLAY_CONTROLLER_NUM_SOURCES] = {
		LCD_CPU_SRC_SEL_AHB,
		LCD_CPU_SRC_SEL_DE | LCD_CPU_RS_HIGH,
		LCD_CPU_SRC_SEL_DMA | LCD_CPU_RS_HIGH,
	};
	static const uint32_t spi_source_set[DISPLAY_CONTROLLER_NUM_SOURCES] = {
		LCD_SPI_SRC_SEL_AHB | LCD_SPI_CDX(1),
		LCD_SPI_SRC_SEL_DE | LCD_SPI_CDX(1),
		LCD_SPI_SRC_SEL_DMA | LCD_SPI_CDX(1),
	};
	struct lcdc_data *data = dev->data;

#if CONFIG_LCDC_DMA_CHAN_ID < 0
	if (source_type == DISPLAY_CONTROLLER_SOURCE_DMA)
		return -EINVAL;
#endif

	switch (data->port->major_type) {
	case DISPLAY_PORT_MCU:
		LCDC->CPU_CTL = (LCDC->CPU_CTL & ~(LCD_CPU_SRC_MASK | LCD_CPU_RS_MASK))
				| mcu_source_set[source_type];
		break;

	case DISPLAY_PORT_SPI:
		LCDC->SPI_CTL = (LCDC->SPI_CTL & ~(LCD_SPI_SRC_MASK | LCD_SPI_CDX_MASK))
				| spi_source_set[source_type];
		break;

	default:
		return -EINVAL;
	}

	return 0;
}

static int lcdc_enable(const struct device *dev, const struct display_videoport *port)
{
	struct lcdc_data *data = dev->data;

	if (data->port != NULL) {
		return -EALREADY;
	}

	if (port == NULL) {
		return -EINVAL;
	}

	/* initially use the CMU_LCDCLK default setting: HOSC 1/1 */
	//clk_set_rate(CLOCK_ID_LCD, MHZ(50));
	acts_reset_peripheral_assert(RESET_ID_LCD);
	acts_clock_peripheral_enable(CLOCK_ID_LCD);
	acts_reset_peripheral_deassert(RESET_ID_LCD);

	if (lcdc_config_port(dev, port)) {
		return -EINVAL;
	}

	/* FIXME: just save the pointer ? */
	data->port = port;
	data->mode = NULL;
	data->source_type = UINT8_MAX;

	return 0;
}

static int lcdc_disable(const struct device *dev)
{
	struct lcdc_data *data = dev->data;

	if (data->port != NULL) {
		data->port = NULL;
		data->busy = false;

#if CONFIG_LCDC_DMA_CHAN_ID >= 0
		DMACHAN->START = 0; /* force stop DMA */
#endif

		acts_clock_peripheral_disable(CLOCK_ID_LCD);
	}

	return 0;
}

static int lcdc_set_mode(const struct device *dev,
		const struct display_videomode *mode)
{
	struct lcdc_data *data = dev->data;

	if (mode == NULL) {
		return -EINVAL;
	}

	if (mode == data->mode) {
		return 0;
	}

	if (lcdc_config_mode(dev, mode)) {
		return -EINVAL;
	}

	/* FIXME: just save the pointer ? */
	data->mode = mode;

	return 0;
}

static int lcdc_set_source(const struct device *dev,
		enum display_controller_source_type source_type, const struct device *source_dev)
{
	struct lcdc_data *data = dev->data;

	if (data->port == NULL) {
		return -EINVAL;
	}

	if (source_type == data->source_type) {
		return 0;
	}

	if (lcdc_config_source(dev, source_type)) {
		return -EINVAL;
	}

	data->source_type = source_type;
	return 0;
}

static void _read_config_data32(uint8_t *buf8, const uint32_t *data, uint32_t len)
{
	int pos = (len >= 4) ? 24 : (len - 1) * 8;

	while (len-- > 0) {
		*buf8++ = (data[0] >> pos) & 0xFF;

		pos -= 8;
		if (pos < 0) {
			pos = 24;
			data++;
		}
	}
}

static int lcdc_read_config(const struct device *dev,
		uint32_t cmd, void *buf, uint32_t len)
{
	struct lcdc_data *lcdc_data = dev->data;

	if (lcdc_data->port == NULL) {
		return -EINVAL;
	}

	if (buf == NULL || len == 0) {
		return -EINVAL;
	}

	if (lcdc_data->port->major_type == DISPLAY_PORT_SPI) {
		uint32_t lcdc_spi_ctl = LCDC->SPI_CTL; /* save REG SPI_CTL */

		if (lcdc_data->port->minor_type < DISPLAY_QSPI) {
			LCDC->SPI_CTL &= ~SPI_AHB_DATA_MASK;
			LCDC->SPI_CTL |= LCD_SPI_SRC_SEL_AHB | LCD_SPI_CDX(0);
			LCDC->DATA = cmd;
			LCDC->SPI_CTL |= LCD_SPI_AHB_CSX(1);

			LCDC->SPI_CTL &= ~(SPI_AHB_DATA_MASK | LCD_SPI_DUAL_LANE_MASK);
			if (len <= 4) {
				LCDC->SPI_CTL |= LCD_SPI_SRC_SEL_AHB | LCD_SPI_AHB_CFG_DATA | LCD_SPI_RWL(len);

				uint32_t data = LCDC->DATA;
				_read_config_data32(buf, &data, len);
			} else {
				LCDC->SPI_CTL |= LCD_SPI_SRC_SEL_AHB | LCD_SPI_AHB_CFG_DATA | LCD_SPI_RWL(1);
				do {
					*(uint8_t *)buf = LCDC->DATA & 0xff;
					buf = (uint8_t *)buf + 1;
				} while (--len > 0);
			}

			LCDC->SPI_CTL |= LCD_SPI_AHB_CSX(1);
		} else {
			uint32_t tmp_data[8];
			int i;

			if (len > 32)
				return -EDOM;

			/* config spi type 'LCD_QSPI_SYNC' to 'LCD_QSPI' temporarily */
			LCDC->SPI_CTL &= ~(SPI_AHB_DATA_MASK | LCD_SPI_TYPE_MASK);
			LCDC->SPI_CTL |= LCD_QSPI | LCD_SPI_SRC_SEL_AHB | LCD_SPI_AHB_CFG_DATA |
					LCD_SPI_CDX(0) | LCD_SPI_RWL(len);
			LCDC->QSPI_CMD = cmd;

			tmp_data[0] = LCDC->DATA;
			for (i = (len - 1) / 4 - 1; i >= 0; i--) {
				tmp_data[i + 1] = LCDC->DATA_1[i];
			}

			_read_config_data32(buf, tmp_data, len);

			LCDC->SPI_CTL |= LCD_SPI_AHB_CSX(1);
		}

		/* restore REG SPI_CTL */
		LCDC->SPI_CTL = lcdc_spi_ctl;
	} else if (lcdc_data->port->major_type == DISPLAY_PORT_MCU) {
		uint32_t lcdc_cpu_ctl = LCDC->CPU_CTL; /* save REG CPU_CTL */

		LCDC->CPU_CTL &= ~CPU_AHB_CMD_MASK;
		LCDC->CPU_CTL |= LCD_CPU_AHB_DATA_SEL_CFG | LCD_CPU_RS_LOW;
		LCDC->DATA = cmd;
		LCDC->CPU_CTL |= LCD_CPU_AHB_CSX(1);

		LCDC->CPU_CTL &= ~CPU_AHB_CMD_MASK;
		LCDC->CPU_CTL |= LCD_CPU_AHB_DATA_SEL_CFG | LCD_CPU_RS_HIGH;

		do {
			*(uint8_t *)buf = LCDC->DATA & 0xff;
			buf = (uint8_t *)buf + 1;
		} while (--len > 0);

		LCDC->CPU_CTL |= LCD_CPU_AHB_CSX(1);

		/* restore REG CPU_CTL */
		LCDC->CPU_CTL = lcdc_cpu_ctl;
	}

	return 0;
}

static int _fill_spi_config_data32(uint32_t *data, const uint8_t *buf8, uint32_t len)
{
	int data_num = (len + 3) >> 2;

	for (; len >= 4; len -= 4) {
		*data++ = ((uint32_t)buf8[0] << 24) | ((uint32_t)buf8[1] << 16) |
				((uint32_t)buf8[2] << 8) | buf8[3];
		buf8 += 4;
	}

	if (len > 0) {
		*data = *buf8++;
		while (--len > 0) {
			*data = (*data << 8) | (*buf8++);
		}
	}

	return data_num;
}

static int lcdc_write_config(const struct device *dev,
		uint32_t cmd, const void *buf, uint32_t len)
{
	struct lcdc_data *lcdc_data = dev->data;
	const uint8_t *buf8 = buf;

	if (lcdc_data->port == NULL) {
		return -EINVAL;
	}

	if (lcdc_data->port->major_type == DISPLAY_PORT_SPI) {
		uint32_t lcdc_spi_ctl = LCDC->SPI_CTL;

		if (lcdc_data->port->minor_type < DISPLAY_QSPI) {
			if (cmd != DC_INVALID_CMD) {
				/* make sure source has selected AHB successfully */
				LCDC->SPI_CTL &= ~SPI_AHB_DATA_MASK;
				LCDC->SPI_CTL |= LCD_SPI_SRC_SEL_AHB | LCD_SPI_CDX(0);
				LCDC->DATA = cmd;
				LCDC->SPI_CTL |= LCD_SPI_AHB_CSX(1);
			}

			if (len > 0) {
				uint32_t data;

				LCDC->SPI_CTL &= ~(SPI_AHB_DATA_MASK | LCD_SPI_DUAL_LANE_MASK);
				if (len <= 4) {
					LCDC->SPI_CTL |= LCD_SPI_SRC_SEL_AHB | LCD_SPI_AHB_CFG_DATA |
							LCD_SPI_CDX(1) | LCD_SPI_RWL(len);

					_fill_spi_config_data32(&data, buf8, len);
					LCDC->DATA = data;
				} else {
					LCDC->SPI_CTL |= LCD_SPI_SRC_SEL_AHB | LCD_SPI_AHB_CFG_DATA |
							LCD_SPI_CDX(1) | LCD_SPI_RWL(1);
					do {
						LCDC->DATA = *buf8++;
					} while (--len > 0);
				}

				LCDC->SPI_CTL |= LCD_SPI_AHB_CSX(1);
			}
		} else {
			uint32_t tmp_data[8];
			int i;

			if (len > 32)
				return -EDOM;

			/* config spi type 'LCD_QSPI_SYNC' to 'LCD_QSPI' temporarily */
			LCDC->SPI_CTL &= ~(SPI_AHB_DATA_MASK | LCD_SPI_TYPE_MASK);
			LCDC->SPI_CTL |= LCD_QSPI | LCD_SPI_SRC_SEL_AHB | LCD_SPI_AHB_CFG_DATA |
					LCD_SPI_CDX(0) | LCD_SPI_RWL(len);

			/* Transfer sequence:
			 * 1) DATA, DATA_1, ..., DATA_7
			 * 2) In every DATA: always (effective) MSB first:
			 *   if 32 bit in DATA, then [31..24], [23..16], [15..8], [7..0]
			 *   if 24 bit in DATA, then [23..16], [15..8], [7..0]
			 *   if 16 bit in DATA, then [15..8], [7..0]
			 *   if 8 bit in DATA, then [7..0]
			 */
			i = _fill_spi_config_data32(tmp_data, buf8, len);

			LCDC->QSPI_CMD = cmd;
			for (i -= 1; i > 0; i--) {
				LCDC->DATA_1[i - 1] = tmp_data[i];
			}
			LCDC->DATA = tmp_data[0];

			LCDC->SPI_CTL |= LCD_SPI_AHB_CSX(1);
		}

		/* restore REG SPI_CTL */
		LCDC->SPI_CTL = lcdc_spi_ctl;
	} else if (lcdc_data->port->major_type == DISPLAY_PORT_MCU) {
		uint32_t lcdc_cpu_ctl = LCDC->CPU_CTL; /* save REG CPU_CTL */

		if (cmd != DC_INVALID_CMD) {
			LCDC->CPU_CTL &= ~CPU_AHB_CMD_MASK;
			LCDC->CPU_CTL |= LCD_CPU_AHB_DATA_SEL_CFG | LCD_CPU_RS_LOW;
			LCDC->DATA = cmd;
			LCDC->CPU_CTL |= LCD_CPU_AHB_CSX(1);
		}

		if (len > 0) {
			LCDC->CPU_CTL &= ~CPU_AHB_CMD_MASK;
			LCDC->CPU_CTL |= LCD_CPU_AHB_DATA_SEL_CFG | LCD_CPU_RS_HIGH;
			do {
				LCDC->DATA = *buf8++;
			} while (--len > 0);

			LCDC->CPU_CTL |= LCD_CPU_AHB_CSX(1);
		}

		/* restore REG CPU_CTL */
		LCDC->CPU_CTL = lcdc_cpu_ctl;
	}

	return 0;
}

static int lcdc_write_pixels_by_mcu(const struct device *dev, uint32_t cmd,
		const struct display_buffer_descriptor *desc, const void *buf)
{
	struct lcdc_data *lcdc_data = dev->data;
	uint32_t pixel_format = desc->pixel_format;
	uint16_t bytes_per_line;
	bool f565_24bit = false;
	int i, j;

	if (pixel_format == 0) {
		pixel_format = lcdc_data->mode->pixel_format;
	} else if (pixel_format != lcdc_data->mode->pixel_format) {
		if (pixel_format == PIXEL_FORMAT_ARGB_8888 && (
				lcdc_data->mode->pixel_format == PIXEL_FORMAT_BGR_565)) {
			f565_24bit = true;
		} else {
			return -EINVAL;
		}
	}

	bytes_per_line = (desc->pitch > 0) ? desc->pitch :
			(desc->width * display_format_get_bits_per_pixel(pixel_format) / 8);

	lcdc_data->busy = true;

	if (lcdc_data->port->major_type == DISPLAY_PORT_SPI) {
		uint32_t lcdc_spi_ctl = LCDC->SPI_CTL; /* save LCD_SPI_CTL */

		LCDC->SPI_CTL &= ~SPI_AHB_DATA_MASK;
		LCDC->SPI_CTL |= LCD_SPI_SRC_SEL_AHB | LCD_SPI_CDX(1) |
				(f565_24bit ? LCD_SPI_AHB_F565_24BIT : LCD_SPI_AHB_F565_16BIT);

		if (lcdc_data->port->minor_type < DISPLAY_QSPI) {
			if (pixel_format == PIXEL_FORMAT_BGR_565) { // rgb565
				for (j = desc->height; j > 0; j--) {
					for (i = desc->width; i > 0; i--) {
						LCDC->DATA = *(uint16_t *)buf;
						buf = (uint16_t *)buf + 1;
					}

					buf = (uint8_t *)buf + (bytes_per_line - desc->width * 2);
				}
			} else { /* argb_8888 */
				for (j = desc->height; j > 0; j--) {
					for (i = desc->width; i > 0; i--) {
						LCDC->DATA = *(uint32_t *)buf;
						buf = (uint32_t *)buf + 1;
					}

					buf = (uint8_t *)buf + (bytes_per_line - desc->width * 4);
				}
			}

			LCDC->SPI_CTL |= LCD_SPI_AHB_CSX(1);
		} else {
			if (pixel_format == PIXEL_FORMAT_BGR_565) { // rgb565
				for (j = desc->height; j > 0; j--) {
					for (i = desc->width; i > 0; i--) {
						LCDC->SPI_CTL &= ~LCD_SPI_AHB_CSX_MASK;
						LCDC->QSPI_CMD = cmd;
						LCDC->DATA = *(uint16_t *)buf;
						LCDC->SPI_CTL |= LCD_SPI_AHB_CSX(1);

						buf = (uint16_t *)buf + 1;
					}

					buf = (uint8_t *)buf + (bytes_per_line - desc->width * 2);
				}
			} else { /* argb_8888 */
				for (j = desc->height; j > 0; j--) {
					for (i = desc->width; i > 0; i--) {
						LCDC->SPI_CTL &= ~LCD_SPI_AHB_CSX_MASK;
						LCDC->QSPI_CMD = cmd;
						LCDC->DATA = *(uint32_t *)buf;
						LCDC->SPI_CTL |= LCD_SPI_AHB_CSX(1);

						buf = (uint32_t *)buf + 1;
					}

					buf = (uint8_t *)buf + (bytes_per_line - desc->width * 4);
				}
			}
		}

		/* restore LCD_SPI_CTL */
		LCDC->SPI_CTL = lcdc_spi_ctl;
	} else if (lcdc_data->port->major_type == DISPLAY_PORT_MCU) {
		uint32_t lcdc_cpu_ctl = LCDC->CPU_CTL;  /* save LCD_CPU_CTL */

		LCDC->CPU_CTL &= ~CPU_AHB_CMD_MASK;
		LCDC->CPU_CTL |= LCD_CPU_AHB_DATA_SEL_IMG | LCD_CPU_RS_HIGH |
				(f565_24bit ? LCD_CPU_AHB_F565_24BIT : LCD_CPU_AHB_F565_16BIT);

		if (pixel_format == PIXEL_FORMAT_BGR_565) { // rgb565
			for (j = desc->height; j > 0; j--) {
				for (i = desc->width; i > 0; i--) {
					LCDC->DATA = *(uint16_t *)buf;
					buf = (uint16_t *)buf + 1;
				}

				buf = (uint8_t *)buf + (bytes_per_line - desc->width * 2);
			}
		} else { /* argb_8888 */
			for (j = desc->height; j > 0; j--) {
				for (i = desc->width; i > 0; i--) {
					LCDC->DATA = *(uint32_t *)buf;
					buf = (uint32_t *)buf + 1;
				}

				buf = (uint8_t *)buf + (bytes_per_line - desc->width * 4);
			}
		}

		/* restore LCD_CPU_CTL */
		LCDC->CPU_CTL = lcdc_cpu_ctl;
	}

	lcdc_data->busy = false;

	/* notify transfer completed */
	if (lcdc_data->complete_fn) {
		lcdc_data->complete_fn(lcdc_data->complete_fn_arg);
	}

	return 0;
}

static int lcdc_write_pixels_by_de(const struct device *dev,
		uint32_t cmd, uint32_t hsync_cmd,
		const struct display_buffer_descriptor *desc)
{
	struct lcdc_data *data = dev->data;

	data->busy = true;

	/* only required for non-sync mode */
	LCDC->TPL = (uint32_t)desc->width * desc->height - 1;

	switch (data->port->major_type) {
	case DISPLAY_PORT_SPI:
		LOG_DBG("start spi-if\n");
		LCDC->QSPI_CMD = cmd;

		if (data->port->minor_type == DISPLAY_QSPI_SYNC) {
			LCDC->QSPI_CMD1 = hsync_cmd; /* only required for QUAD_SYNC */
			LCDC->QSPI_IMG_SIZE = LCD_QSPI_IMG_SIZE(desc->width, desc->height);

			LCDC->CTL &= ~LCD_EN;
			/* set to quad sync mode */
			LCDC->SPI_CTL = (LCDC->SPI_CTL & ~LCD_SPI_TYPE_MASK) | LCD_QSPI_SYNC;
			LCDC->CTL |= LCD_EN;
		}

		LCDC->SPI_CTL |= LCD_SPI_START;
		break;
	case DISPLAY_PORT_MCU:
	default:
		LOG_DBG("start cpu-if\n");
		LCDC->CPU_CTL |= LCD_CPU_START;
		break;
	}

	return 0;
}

#if CONFIG_LCDC_DMA_CHAN_ID >= 0

static int lcdc_write_pixels_by_dma(const struct device *dev,
		uint32_t cmd, uint32_t hsync_cmd,
		const struct display_buffer_descriptor *desc, const void *buf)
{
	struct lcdc_data *data = dev->data;
	uint32_t pixel_format = desc->pixel_format;
	uint16_t copy_per_line, bytes_per_line;
	uint8_t bytes_per_pixel;

	if ((uintptr_t)buf & 0x3) {
		LOG_ERR("unaligned addr %p", buf);
		return -EINVAL;
	}

	if (pixel_format == 0) {
		pixel_format = data->mode->pixel_format;
	}

	if (lcdc_config_src_pixel_format(dev, pixel_format, &bytes_per_pixel)) {
		LOG_ERR("invalid pixel format 0x%x", pixel_format);
		return -EINVAL;
	}

	copy_per_line = desc->width * bytes_per_pixel;
	bytes_per_line = (desc->pitch > 0) ? desc->pitch : copy_per_line;

	if (copy_per_line != bytes_per_line &&
			((copy_per_line & 0x3) || (bytes_per_line & 0x3))) {
		LOG_ERR("width %u or stride %u unaligned\n", desc->width, desc->pitch);
		return -EINVAL;
	}

	if (buf_is_psram(buf)) {
		buf = cache_to_uncache((void *)buf);
	}

	data->busy = true;

	/* configure DMA */
	assert(DMACHAN->START == 0);

	DMACHAN->BC = (uint32_t)copy_per_line * desc->height;
	DMACHAN->SADDR0 = (uint32_t)buf;
	DMACHAN->DADDR0 = (uint32_t)&LCDC->DATA;

	if (copy_per_line == bytes_per_line) {
		DMACHAN->CTL = BIT(15) | (0x16 << 8);
	} else {
		DMALINE->COUNT = desc->height;
		DMALINE->LENGTH = copy_per_line;
		DMALINE->STRIDE = bytes_per_line;
		DMACHAN->CTL = BIT(15) | (0x16 << 8) | BIT(24) | (CONFIG_LCDC_DMA_LINE_ID << 25);
	}

	/* start DMA */
	DMACHAN->START = 0x1;

	LCDC->DISP_SIZE = LCD_DISP_SIZE(desc->width, desc->height);

	if (data->port->major_type == DISPLAY_PORT_SPI) {
		LCDC->QSPI_CMD = cmd;

		if (data->port->minor_type == DISPLAY_QSPI_SYNC) {
			LCDC->QSPI_CMD1 = hsync_cmd;
			LCDC->QSPI_IMG_SIZE = LCD_QSPI_IMG_SIZE(desc->width, desc->height);

			LCDC->CTL &= ~LCD_EN;
			/* set to quad sync mode */
			LCDC->SPI_CTL = (LCDC->SPI_CTL & ~LCD_SPI_TYPE_MASK) | LCD_QSPI_SYNC;
			LCDC->CTL |= LCD_EN;
		}

		LCDC->SPI_CTL |= LCD_SPI_START;
	} else { /* DISPLAY_PORT_MCU */
		LCDC->CPU_CTL |= LCD_CPU_START;
	}

	return 0;
}
#endif /* CONFIG_LCDC_DMA_CHAN_ID >= 0 */

static int lcdc_write_pixels(const struct device *dev,
		uint32_t cmd, uint32_t hsync_cmd,
		const struct display_buffer_descriptor *desc, const void *buf)
{
	struct lcdc_data *data = dev->data;

	if (data->port == NULL || data->mode == NULL) {
		return -EINVAL;
	}

	switch (data->source_type) {
#if CONFIG_LCDC_DMA_CHAN_ID >= 0
	case DISPLAY_CONTROLLER_SOURCE_DMA:
		return lcdc_write_pixels_by_dma(dev, cmd, hsync_cmd, desc, buf);
#endif
	case DISPLAY_CONTROLLER_SOURCE_ENGINE:
		return lcdc_write_pixels_by_de(dev, cmd, hsync_cmd, desc);
	case DISPLAY_CONTROLLER_SOURCE_MCU:
		return lcdc_write_pixels_by_mcu(dev, cmd, desc, buf);
	default:
		return -EINVAL;
	}
}

static int lcdc_read_pixels(const struct device *dev, uint32_t cmd,
		const struct display_buffer_descriptor *desc, void *buf)
{
	return -ENOTSUP;
}

static int lcdc_control(const struct device *dev, int cmd, void *arg1, void *arg2)
{
	struct lcdc_data *data = dev->data;

	switch (cmd) {
	case DISPLAY_CONTROLLER_CTRL_COMPLETE_CB:
		data->complete_fn_arg = arg2;
		data->complete_fn = arg1;
		break;

	default:
		return -EINVAL;
	}

	return 0;
}

void lcdc_dump(void)
{
	int i;

	printk("lcdc regs:\n");
	printk("\t LCD_CTL            0x%08x\n", LCDC->CTL);
	printk("\t LCD_DISP_SIZE      0x%08x\n", LCDC->DISP_SIZE);
	printk("\t LCD_CPU_CTL        0x%08x\n", LCDC->CPU_CTL);
	printk("\t LCD_DATA           0x%08x\n", LCDC->DATA);
	printk("\t LCD_CPU_CLK        0x%08x\n", LCDC->CPU_CLK);
	printk("\t LCD_TPL            0x%08x\n", LCDC->TPL);
	printk("\t LCD_SPI_CTL        0x%08x\n", LCDC->SPI_CTL);
	printk("\t LCD_QSPI_CMD       0x%08x\n", LCDC->QSPI_CMD);
	printk("\t LCD_QSPI_CMD1      0x%08x\n", LCDC->QSPI_CMD1);
	printk("\t LCD_QSPI_SYNC_TIM  0x%08x\n", LCDC->QSPI_SYNC_TIM);
	printk("\t LCD_QSPI_IMG_SIZE  0x%08x\n", LCDC->QSPI_IMG_SIZE);
	printk("\t DE_INTERFACE_CTL   0x%08x\n", LCDC->DE_INTERFACE_CTL);
	printk("\t LCD_PENDING        0x%08x\n", LCDC->PENDING);
	printk("\t LCD_QSPI_DTAS      0x%08x\n", LCDC->QSPI_DTAS);

	for (i = 0; i < ARRAY_SIZE(LCDC->DATA_1); i++) {
		printk("\t LCD_DATA_%d         0x%08x\n", i + 1, LCDC->DATA_1[i]);
	}

#if CONFIG_LCDC_DMA_CHAN_ID >= 0
	printk("\t DMA_CTL            0x%08x\n", DMACHAN->CTL);
	printk("\t DMA_START          0x%08x\n", DMACHAN->START);
	printk("\t DMA_SADDR0         0x%08x\n", DMACHAN->SADDR0);
	printk("\t DMA_SADDR1         0x%08x\n", DMACHAN->SADDR1);
	printk("\t DMA_DADDR0         0x%08x\n", DMACHAN->DADDR0);
	printk("\t DMA_DADDR1         0x%08x\n", DMACHAN->DADDR1);
	printk("\t DMA_BC             0x%08x\n", DMACHAN->BC);
	printk("\t DMA_RC             0x%08x\n", DMACHAN->RC);
	printk("\t DMA_LINE_LENGTH    0x%08x\n", DMALINE->LENGTH);
	printk("\t DMA_LINE_COUNT     0x%08x\n", DMALINE->COUNT);
	printk("\t DMA_LINE_STRIDE    0x%08x\n", DMALINE->STRIDE);
	printk("\t DMA_LINE_REMAIN    0x%08x\n", DMALINE->REMAIN);
#endif
}

static void lcdc_isr(const void *arg)
{
	const struct device *dev = arg;
	struct lcdc_data *data = dev->data;
	uint32_t pending = LCDC->PENDING;

	LCDC->PENDING = pending;

	if (pending & LCD_STAT_FTC) {
		if (pending & LCD_STAT_QSPI_SYNC_FTC) {
#if CONFIG_LCDC_DMA_CHAN_ID >= 0
			if (data->source_type == DISPLAY_CONTROLLER_SOURCE_DMA) {
				if (DMACHAN->START > 0) {
					LOG_ERR("LCD remain %u", DMACHAN->RC);
					DMACHAN->START = 0;
				}

				/* restart DMA */
				DMACHAN->START = 0x1;
			}
#endif
		} else {
			data->busy = false;
		}

		if (data->complete_fn)
			data->complete_fn(data->complete_fn_arg);
	}

	LOG_DBG("LCD pending 0x%x", pending);
}

DEVICE_DECLARE(lcdc);

static int lcdc_init(const struct device *dev)
{
	IRQ_CONNECT(IRQ_ID_LCD, 0, lcdc_isr, DEVICE_GET(lcdc), 0);
	irq_enable(IRQ_ID_LCD);
	return 0;
}

#ifdef CONFIG_PM_DEVICE
static int lcdc_pm_control(const struct device *dev, enum pm_device_action action)
{
	struct lcdc_data *data = dev->data;
	int ret = 0;

	switch (action) {
	case PM_DEVICE_ACTION_SUSPEND:
	case PM_DEVICE_ACTION_FORCE_SUSPEND:
	case PM_DEVICE_ACTION_TURN_OFF:
		if (data->busy) {
			LOG_WRN("lcdc busy (action=%d)", action);
			ret = -EBUSY;
		}
		break;
	default:
		break;
	}

	return ret;
}
#endif /* CONFIG_PM_DEVICE */

static const struct display_controller_driver_api lcdc_api = {
	.control = lcdc_control,
	.enable = lcdc_enable,
	.disable = lcdc_disable,
	.set_mode = lcdc_set_mode,
	.set_source = lcdc_set_source,
	.read_config = lcdc_read_config,
	.write_config = lcdc_write_config,
	.read_pixels = lcdc_read_pixels,
	.write_pixels = lcdc_write_pixels,
};

static struct lcdc_data lcdc_drv_data;

DEVICE_DEFINE(lcdc, CONFIG_LCDC_DEV_NAME, &lcdc_init,
			lcdc_pm_control, &lcdc_drv_data, NULL, POST_KERNEL,
			CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &lcdc_api);
