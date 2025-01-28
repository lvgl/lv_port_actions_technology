/*
 * Copyright (c) 2017 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief SPI controller helper for GL5120
 */


#ifndef __SPI_H__
#define __SPI_H__

#include <device.h>
#include <soc.h>
#include <arch/common/sys_io.h>
#include "../spi_flash.h"

#ifdef CONFIG_NOR_CODE_IN_RAM
 	#define _nor_fun	__ramfunc
#else
	#define _nor_fun
#endif


#define SPI_DELAY_LOOPS			5

/* spinor controller */
#define SSPI_CTL					0x00
#define SSPI_STATUS					0x04
#define SSPI_TXDAT					0x08
#define SSPI_RXDAT					0x0c
#define SSPI_BC						0x10
#define SSPI_SEED					0x14
#define SCACHE_ERR_ADDR				0x18
#define SNO_CACHE_ADDR				0x1C

#define SSPI_CTL_CLK_SEL_MASK		((unsigned int)1 << 31)
#define SSPI_CTL_CLK_SEL_CPU		((unsigned int)0 << 31)
#define SSPI_CTL_CLK_SEL_DMA		((unsigned int)1 << 31)

#define SSPI_CTL_MODE_MASK			(1 << 28)
#define SSPI_CTL_MODE_MODE3			(0 << 28)
#define SSPI_CTL_MODE_MODE0			(1 << 28)

#define SSPI_CTL_CRC_EN				(1 << 20)

#define SSPI_CTL_DELAYCHAIN_MASK	(0xf << 16)
#define SSPI_CTL_DELAYCHAIN_SHIFT	(16)

#define SSPI_CTL_RAND_MASK			(0xf << 12)
#define SSPI_CTL_RAND_PAUSE			(1 << 15)
#define SSPI_CTL_RAND_SEL			(1 << 14)
#define SSPI_CTL_RAND_TXEN			(1 << 13)
#define SSPI_CTL_RAND_RXEN			(1 << 12)

#define SSPI_CTL_IO_MODE_MASK		(0x3 << 10)
#define SSPI_CTL_IO_MODE_SHIFT		(10)
#define SSPI_CTL_IO_MODE_1X			(0x0 << 10)
#define SSPI_CTL_IO_MODE_2X			(0x2 << 10)
#define SSPI_CTL_IO_MODE_4X			(0x3 << 10)
#define SSPI_CTL_SPI_3WIRE			(1 << 9)
#define SSPI_CTL_AHB_REQ			(1 << 8)
#define SSPI_CTL_TX_DRQ_EN			(1 << 7)
#define SSPI_CTL_RX_DRQ_EN			(1 << 6)
#define SSPI_CTL_TX_FIFO_EN			(1 << 5)
#define SSPI_CTL_RX_FIFO_EN			(1 << 4)
#define SSPI_CTL_SS					(1 << 3)
#define SSPI_CTL_WR_MODE_MASK		(0x3 << 0)
#define SSPI_CTL_WR_MODE_DISABLE	(0x0 << 0)
#define SSPI_CTL_WR_MODE_READ		(0x1 << 0)
#define SSPI_CTL_WR_MODE_WRITE		(0x2 << 0)
#define SSPI_CTL_WR_MODE_READ_WRITE	(0x3 << 0)

#define SSPI_STATUS_BUSY			(1 << 6)
#define SSPI_STATUS_TX_FULL			(1 << 5)
#define SSPI_STATUS_TX_EMPTY		(1 << 4)
#define SSPI_STATUS_RX_FULL			(1 << 3)
#define SSPI_STATUS_RX_EMPTY		(1 << 2)


#define SSPI0_REGISTER				(SPI0_BASE)
#define SSPI1_REGISTER				(SPI1_BASE)
#define SSPI2_REGISTER				(SPI2_BASE)

/* spi1 registers bits */
#define SSPI1_CTL_MODE_MASK			(3 << 28)
#define SSPI1_CTL_MODE_MODE3		(3 << 28)
#define SSPI1_CTL_MODE_MODE0		(0 << 28)
#define SSPI1_CTL_AHB_REQ			(1 << 15)

#define SSPI1_STATUS_BUSY			(1 << 0)
#define SSPI1_STATUS_TX_FULL		(1 << 5)
#define SSPI1_STATUS_TX_EMPTY		(1 << 4)
#define SSPI1_STATUS_RX_FULL		(1 << 7)
#define SSPI1_STATUS_RX_EMPTY		(1 << 6)

#define MRCR0_SPI3RESET             (7)
#define MRCR0_SPI2RESET             (6)
#define MRCR0_SPI1RESET             (5)
#define MRCR0_SPI0RESET             (4)

#define CMU_DEVCLKEN0_SPI3CLKEN     (7)
#define CMU_DEVCLKEN0_SPI2CLKEN     (6)
#define CMU_DEVCLKEN0_SPI1CLKEN     (5)
#define CMU_DEVCLKEN0_SPI0CLKEN     (4)

extern void spi_delay(void);

static inline unsigned int spi_read(struct spi_info *si, unsigned int reg)
{
	return sys_read32(si->base + reg);
}

static inline void spi_write(struct spi_info *si, unsigned int reg,
					unsigned int value)
{
	sys_write32(value, si->base + reg);
}


#endif	/* __SPI_H__ */
