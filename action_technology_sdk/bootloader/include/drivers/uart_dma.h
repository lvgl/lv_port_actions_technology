/*
 * Copyright (c) 2018-2019 Nordic Semiconductor ASA
 * Copyright (c) 2015 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public APIs for UART dma drivers
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_UART_DMA_H_
#define ZEPHYR_INCLUDE_DRIVERS_UART_DMA_H_

/**
 * @brief UART Interface
 * @defgroup uart_interface UART Interface
 * @ingroup io_interfaces
 * @{
 */

#include <errno.h>
#include <stddef.h>
#include <device.h>
#include <drivers/dma.h>

#ifdef __cplusplus
extern "C" {
#endif


#ifdef CONFIG_UART_DMA_DRIVEN 

#define UART_FIFO_TYPE_CPU         0
#define UART_FIFO_TYPE_DMA         1


int uart_acts_dma_send_init(struct device *dev,  dma_callback_t callback, void *arg);
int uart_acts_dma_send(struct device *dev, char *s, int len);
int uart_acts_dma_send_complete(struct device *dev);
int uart_acts_dma_send_stop(struct device *dev);
int uart_acts_fifo_switch(struct device *dev, uint32_t is_tx, uint32_t fifo_type);

int uart_acts_dma_receive_init(struct device *dev, dma_callback_t callback, void *arg);
int uart_acts_dma_receive(struct device *dev, char *d, int len);
int uart_acts_dma_receive_complete(struct device *dev);
int uart_acts_dma_receive_stop(struct device *dev);
int uart_acts_dma_receive_drq_switch(struct device *dev, bool drq_enable);



static inline int uart_dma_send_init(struct device *dev,  dma_callback_t stream_handler, void *stream_data)
{
	return uart_acts_dma_send_init(dev, stream_handler, stream_data);
}

static inline int uart_dma_send(struct device *dev, char *s, int len)
{
	return uart_acts_dma_send(dev, s, len);
}

static inline int uart_dma_send_complete(struct device *dev)
{
	return uart_acts_dma_send_complete(dev);
}

static inline int uart_dma_send_stop(struct device *dev)
{
	return uart_acts_dma_send_stop(dev);
}

static inline int uart_fifo_switch(struct device *dev, uint32_t is_tx, uint32_t fifo_type)
{
	return uart_acts_fifo_switch(dev, is_tx, fifo_type);
}

static inline int uart_dma_receive_init(struct device *dev,  dma_callback_t stream_handler, void *stream_data)
{
	return uart_acts_dma_receive_init(dev, stream_handler, stream_data);
}

static inline int uart_dma_receive(struct device *dev, char *d, int len)
{
	return uart_acts_dma_receive(dev, d, len);
}

static inline int uart_dma_receive_complete(struct device *dev)
{
	return uart_acts_dma_receive_complete(dev);
}

static inline int uart_dma_receive_stop(struct device *dev)
{
	return uart_acts_dma_receive_stop(dev);
}

static inline int uart_dma_receive_drq_switch(struct device *dev, bool drq_enable)
{
	return uart_acts_dma_receive_drq_switch(dev, drq_enable);
}

static inline int uart_rx_dma_switch(struct device *dev, bool use_dma, dma_callback_t callback, void *arg)
{
	if(use_dma)
    {
		uart_fifo_switch(dev, 0, UART_FIFO_TYPE_DMA);
	}
    else
    {
		uart_fifo_switch(dev, 0, UART_FIFO_TYPE_CPU);
	}

	return 0;
}

static inline int uart_tx_dma_switch(struct device *dev, bool use_dma, dma_callback_t callback, void *arg)
{
	if(use_dma)
	{
		uart_fifo_switch(dev, 1, UART_FIFO_TYPE_DMA);
	}
	else
	{
		uart_fifo_switch(dev, 1, UART_FIFO_TYPE_CPU);
	}
	
	return 0;
}

#endif


#ifdef __cplusplus
}
#endif


/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_UART_DMA_H_ */
