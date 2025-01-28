/*
 * Copyright (c) 2017 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief UART Driver for Actions SoC
 */


#include <kernel.h>
#include <device.h>
#include <string.h>
#include <drivers/uart.h>
#include <soc.h>
#include <drivers/uart_dma.h>
#include <board_cfg.h>
#ifdef CONFIG_CFG_DRV
#include <config.h>
#include <drivers/cfg_drv/driver_config.h>
#endif


/* Device data config */
struct acts_uart_config {
	uint32_t base;
	const char *dma_dev_name;
	uint8_t txdma_id;
	uint8_t txdma_chan;
	uint8_t rxdma_id;
	uint8_t rxdma_chan;
	uint8_t clock_id;
	uint8_t reset_id;
	uint8_t flag_use_txdma:1;
	uint8_t flag_use_rxdma:1;
	uint32_t baud_rate;
	uart_irq_config_func_t	irq_config_func;

};

/* Device data structure */
struct acts_uart_data {
	/* Baud rate */
	struct uart_config cfg;

	uint32_t baud_rate;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	/* IRQ Callback function pointer */
	uart_irq_callback_user_data_t cb;
	void *user_data;
#endif
#ifdef CONFIG_UART_DMA_DRIVEN
    struct device *dma_dev;
	uint8_t txdma_chan;
	uint8_t rxdma_chan;
#endif
};

/* convenience defines */
#define DEV_CFG(dev) \
	((const struct acts_uart_config * const)(dev)->config)
#define DEV_DATA(dev) \
	((struct acts_uart_data * const)(dev)->data)
#define UART_STRUCT(dev) \
	((struct acts_uart_controller *)(DEV_CFG(dev))->base)

/* UART registers struct */
typedef struct acts_uart_controller {
    volatile uint32_t ctrl;

    volatile uint32_t rxdat;

    volatile uint32_t txdat;

    volatile uint32_t stat;

    volatile uint32_t br;
} uart_register_t;

/* bits */
#define UART_CTL_RX_EN              BIT(31)
#define UART_CTL_TX_EN              BIT(30)

#define UART_CTL_TX_FIFO_EN         BIT(29)
#define UART_CTL_RX_FIFO_EN         BIT(28)

#define UART_CTL_TX_DST_SHIFT       26
#define UART_FIFO_CPU         0
#define UART_FIFO_DMA         1
#define UART_FIFO_DSP         2
#define UART_CTL_TX_DST(x)          ((x) << UART_CTL_TX_DST_SHIFT)
#define UART_CTL_TX_DST_MASK        UART_CTL_TX_DST(0x3)

#define UART_CTL_RX_SRC_SHIFT       24
#define UART_CTL_RX_SRC(x)          ((x) << UART_CTL_RX_SRC_SHIFT)
#define UART_CTL_RX_SRC_MASK        UART_CTL_RX_SRC(0x3)


//#define UART_CTL_TX_DMA             BIT(21)
#define UART_CTL_LB_EN              BIT(20)
#define UART_CTL_TX_IE              BIT(19)
#define UART_CTL_RX_IE              BIT(18)
#define UART_CTL_TX_DE              BIT(17)
#define UART_CTL_RX_DE              BIT(16)
#define UART_CTL_EN             BIT(15)
//#define UART_CTL_RX_DMA             BIT(14)
#define UART_CTL_RTS_EN             BIT(13)
#define UART_CTL_AF_EN              BIT(12)
#define UART_CTL_RX_FIFO_THREHOLD_SHIFT     10
#define UART_CTL_RX_FIFO_THREHOLD(x)        ((x) << UART_CTL_RX_FIFO_THREHOLD_SHIFT)
#define UART_CTL_RX_FIFO_THREHOLD_MASK      UART_CTL_RX_FIFO_THREHOLD(0x3)
#define     UART_CTL_RX_FIFO_THREHOLD_1BYTE     UART_CTL_RX_FIFO_THREHOLD(0x0)
#define     UART_CTL_RX_FIFO_THREHOLD_4BYTES    UART_CTL_RX_FIFO_THREHOLD(0x1)
#define     UART_CTL_RX_FIFO_THREHOLD_8BYTES    UART_CTL_RX_FIFO_THREHOLD(0x2)
#define     UART_CTL_RX_FIFO_THREHOLD_12BYTES   UART_CTL_RX_FIFO_THREHOLD(0x3)
#define UART_CTL_TX_FIFO_THREHOLD_SHIFT     8
#define UART_CTL_TX_FIFO_THREHOLD(x)        ((x) << UART_CTL_TX_FIFO_THREHOLD_SHIFT)
#define UART_CTL_TX_FIFO_THREHOLD_MASK      UART_CTL_TX_FIFO_THREHOLD(0x3)
#define     UART_CTL_TX_FIFO_THREHOLD_1BYTE     UART_CTL_TX_FIFO_THREHOLD(0x0)
#define     UART_CTL_TX_FIFO_THREHOLD_4BYTES    UART_CTL_TX_FIFO_THREHOLD(0x1)
#define     UART_CTL_TX_FIFO_THREHOLD_8BYTES    UART_CTL_TX_FIFO_THREHOLD(0x2)
#define     UART_CTL_TX_FIFO_THREHOLD_12BYTES   UART_CTL_TX_FIFO_THREHOLD(0x3)
//#define UART_CTL_CTS_EN             BIT(7)
#define UART_CTL_PARITY_SHIFT           4
#define UART_CTL_PARITY(x)          ((x) << UART_CTL_PARITY_SHIFT)
#define UART_CTL_PARITY_MASK            UART_CTL_PARITY(0x7)
#define     UART_CTL_PARITY_NONE            UART_CTL_PARITY(0x0)
#define     UART_CTL_PARITY_ODD         UART_CTL_PARITY(0x4)
#define     UART_CTL_PARITY_LOGIC_1         UART_CTL_PARITY(0x5)
#define     UART_CTL_PARITY_EVEN            UART_CTL_PARITY(0x6)
#define     UART_CTL_PARITY_LOGIC_0         UART_CTL_PARITY(0x7)
#define UART_CTL_STOP_SHIFT         2
#define UART_CTL_STOP(x)            ((x) << UART_CTL_STOP_SHIFT)
#define UART_CTL_STOP_MASK          UART_CTL_STOP(0x1)
#define     UART_CTL_STOP_1BIT          UART_CTL_STOP(0x0)
#define     UART_CTL_STOP_2BIT          UART_CTL_STOP(0x1)
#define UART_CTL_DATA_WIDTH_SHIFT       0
#define UART_CTL_DATA_WIDTH(x)          ((x) << UART_CTL_DATA_WIDTH_SHIFT)
#define UART_CTL_DATA_WIDTH_MASK        UART_CTL_DATA_WIDTH(0x3)
#define     UART_CTL_DATA_WIDTH_5BIT        UART_CTL_DATA_WIDTH(0x0)
#define     UART_CTL_DATA_WIDTH_6BIT        UART_CTL_DATA_WIDTH(0x1)
#define     UART_CTL_DATA_WIDTH_7BIT        UART_CTL_DATA_WIDTH(0x2)
#define     UART_CTL_DATA_WIDTH_8BIT        UART_CTL_DATA_WIDTH(0x3)

#define UART_STA_PAER               BIT(23)
#define UART_STA_STER               BIT(22)
#define UART_STA_UTBB                   BIT(21)
#define UART_STA_TXFL_SHIFT         16
#define UART_STA_TXFL_MASK          (0x1f << UART_STA_TXFL_SHIFT)
#define UART_STA_RXFL_SHIFT         11
#define UART_STA_RXFL_MASK          (0x1f << UART_STA_RXFL_SHIFT)
#define UART_STA_TFES                   BIT(10)
#define UART_STA_RFFS               BIT(9)
#define UART_STA_RTSS               BIT(8)
#define UART_STA_CTSS               BIT(7)
#define UART_STA_TFFU               BIT(6)
#define UART_STA_RFEM               BIT(5)
#define UART_STA_RXST               BIT(4)
#define UART_STA_TFER               BIT(3)
#define UART_STA_RXER               BIT(2)
#define UART_STA_TIP                BIT(1)
#define UART_STA_RIP                BIT(0)

#define UART_STA_ERRS               (UART_STA_RXER | UART_STA_TFER | \
                         UART_STA_RXST | UART_STA_PAER | \
                         UART_STA_STER)

#define UART_BAUD_DIV(freq, baud_rate)  (((freq) / (baud_rate))) + ((((freq) % (baud_rate)) + ((baud_rate) >> 1)) / baud_rate)



static void uart_acts_init_bycfg(const struct device *dev)
{
	struct acts_uart_data * dev_data = DEV_DATA(dev);
	struct acts_uart_controller *uart = UART_STRUCT(dev);
	struct uart_config *cfg = &dev_data->cfg;
	unsigned int div, ctl, ctl_mask;


	ctl = UART_CTL_DATA_WIDTH((cfg->data_bits&0x3));

	if(cfg->stop_bits== UART_CFG_STOP_BITS_2)
		ctl |= UART_CTL_STOP_2BIT;
	else
		ctl |= UART_CTL_STOP_1BIT;

	switch(cfg->parity){
		case UART_CFG_PARITY_NONE:
			ctl |= UART_CTL_PARITY_NONE;
			break;
		case UART_CFG_PARITY_ODD:
			ctl |= UART_CTL_PARITY_ODD;
			break;
		case UART_CFG_PARITY_EVEN:
			ctl |= UART_CTL_PARITY_EVEN;
			break;
		case UART_CFG_PARITY_MARK:
			ctl |= UART_CTL_PARITY_LOGIC_1;
			break;
		case UART_CFG_PARITY_SPACE:
			ctl |= UART_CTL_PARITY_LOGIC_0;
			break;
	}
	ctl_mask = UART_CTL_PARITY_MASK | UART_CTL_STOP_MASK |UART_CTL_DATA_WIDTH_MASK;

	uart->ctrl = (uart->ctrl&(~ctl_mask)) | ctl;

	uart->ctrl &= ~UART_CTL_EN;	/*fix set 2M buad error*/
	div = UART_BAUD_DIV(CONFIG_HOSC_CLK_MHZ * 1000000,cfg->baudrate);
	uart->br = (div | (div << 16));
	uart->ctrl |= UART_CTL_EN;

	/* clear error status */
	uart->stat = UART_STA_ERRS;

}


static int uart_acts_set_config(const struct device *dev, const struct uart_config *cfg)
{
	struct acts_uart_data * dev_data = DEV_DATA(dev);
	struct uart_config *dcfg = &dev_data->cfg;
	memcpy(dcfg, cfg, sizeof(struct uart_config));
	uart_acts_init_bycfg(dev);
	return 0;
}

static int  uart_acts_get_config(const struct device *dev, struct uart_config *cfg)
{
	struct acts_uart_data * dev_data = DEV_DATA(dev);
	struct uart_config *dcfg = &dev_data->cfg;
	memcpy(cfg , dcfg, sizeof(struct uart_config));
	return 0;
}


/**
 * @brief Poll the device for input.
 *
 * @param dev UART device struct
 * @param c Pointer to character
 *
 * @return 0 if a character arrived, -1 if the input buffer if empty.
 */

static int uart_acts_poll_in(const struct device *dev, unsigned char *c)
{
	struct acts_uart_controller *uart = UART_STRUCT(dev);

	/* Wait for transmitter to be ready */
	while (uart->stat & UART_STA_RFEM)
		;

	/* got a character */
	*c = (unsigned char)uart->rxdat;

	return 0;
}

/**
 * @brief Output a character in polled mode.
 *
 * Checks if the transmitter is empty. If empty, a character is written to
 * the data register.
 *
 * @param dev UART device struct
 * @param c Character to send
 *
 * @return Sent character
 */
static void uart_acts_poll_out(const struct device *dev,
					     unsigned char c)
{

	struct acts_uart_controller *uart = UART_STRUCT(dev);

	/* Wait for transmitter to be ready */
	while (uart->stat & UART_STA_TFFU);

	/* send a character */
	uart->txdat = (uint32_t)c;



}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN

static int uart_acts_err_check(const struct device *dev)
{
	struct acts_uart_controller *uart = UART_STRUCT(dev);
	uint32_t error = 0;

	if (uart->stat & UART_STA_ERRS) {
		if (uart->stat & (UART_STA_TFER | UART_STA_TFER))
			error |= UART_ERROR_OVERRUN;

		if (uart->stat & (UART_STA_STER | UART_STA_RXER))
			error |= UART_ERROR_FRAMING;

		if (uart->stat & UART_STA_PAER)
			error |= UART_ERROR_PARITY;

		/* clear error status */
		uart->stat = UART_STA_ERRS;
	}

	return error;
}

/** Interrupt driven FIFO fill function */
static int uart_acts_fifo_fill(const struct device *dev, const uint8_t *tx_data, int len)
{
	struct acts_uart_controller *uart = UART_STRUCT(dev);
	uint8_t num_tx = 0;

	while ((len - num_tx > 0) && !(uart->stat & UART_STA_TFFU)) {
		/* Send a character */
		uart->txdat = (uint8_t)tx_data[num_tx++];
	}

	/* Clear the interrupt */
	uart->stat = UART_STA_TIP;

	return (int)num_tx;
}

/** Interrupt driven FIFO read function */
static int uart_acts_fifo_read(const struct device *dev, uint8_t *rx_data, const int size)
{
	struct acts_uart_controller *uart = UART_STRUCT(dev);
	uint8_t num_rx = 0;

	while ((size - num_rx > 0) && !(uart->stat & UART_STA_RFEM)) {
		/* Receive a character */
		rx_data[num_rx++] = (uint8_t)uart->rxdat;
	}

	/* Clear the interrupt */
	uart->stat = UART_STA_RIP;

	return num_rx;
}

/** Interrupt driven transfer enabling function */
static void uart_acts_irq_tx_enable(const struct device *dev)
{
	struct acts_uart_controller *uart = UART_STRUCT(dev);

	uart->ctrl |= UART_CTL_TX_IE;
}

/** Interrupt driven transfer disabling function */
static void uart_acts_irq_tx_disable(const struct device *dev)
{
	struct acts_uart_controller *uart = UART_STRUCT(dev);

	uart->ctrl &= ~UART_CTL_TX_IE;
}

/** Interrupt driven transfer ready function */
static int uart_acts_irq_tx_ready(const struct device *dev)
{
	struct acts_uart_controller *uart = UART_STRUCT(dev);

	return !(uart->stat & UART_STA_TFFU);
}

/** Interrupt driven receiver enabling function */
static void uart_acts_irq_rx_enable(const struct device *dev)
{
	struct acts_uart_controller *uart = UART_STRUCT(dev);

	uart->ctrl |= UART_CTL_RX_IE;
}

/** Interrupt driven receiver disabling function */
static void uart_acts_irq_rx_disable(const struct device *dev)
{
	struct acts_uart_controller *uart = UART_STRUCT(dev);

	uart->ctrl &= ~UART_CTL_RX_IE;
}

/** Interrupt driven transfer empty function */
static int uart_acts_irq_tx_complete(const struct device *dev)
{
	struct acts_uart_controller *uart = UART_STRUCT(dev);

	return (uart->stat & UART_STA_TFES);
}

/** Interrupt driven receiver ready function */
static int uart_acts_irq_rx_ready(const struct device *dev)
{
	struct acts_uart_controller *uart = UART_STRUCT(dev);

	return !(uart->stat & UART_STA_RFEM);
}

/** Interrupt driven pending status function */
static int uart_acts_irq_is_pending(const struct device *dev)
{
	struct acts_uart_controller *uart = UART_STRUCT(dev);
	int tx_pending, rx_pending;

	tx_pending = (uart->ctrl & UART_CTL_TX_IE) && (uart->stat & UART_STA_TIP);
	rx_pending = (uart->ctrl & UART_CTL_RX_IE) && (uart->stat & UART_STA_RIP);

	return (tx_pending || rx_pending);
}

/** Interrupt driven interrupt update function */
static int uart_acts_irq_update(const struct device *dev)
{
	return 1;
}

/** Set the callback function */
static void uart_acts_irq_callback_set(const struct device *dev, uart_irq_callback_user_data_t cb, void *user_data)
{
	struct acts_uart_data * const dev_data = DEV_DATA(dev);

	dev_data->cb = cb;
	dev_data->user_data = user_data;
}

/**
 * @brief Interrupt service routine.
 *
 * This simply calls the callback function, if one exists.
 *
 * @param arg Argument to ISR.
 *
 * @return N/A
 */
void uart_acts_isr(void *arg)
{
	struct device *dev = arg;
	struct acts_uart_data * const dev_data = DEV_DATA(dev);
	struct acts_uart_controller *uart = UART_STRUCT(dev);

	/* clear error status */
	uart->stat = UART_STA_ERRS;

	if (dev_data->cb) {
		dev_data->cb(dev, dev_data->user_data);
	}
}
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

#ifdef CONFIG_UART_DMA_DRIVEN

#include <drivers/dma.h>

int uart_acts_fifo_switch(struct device *dev, uint32_t is_tx, uint32_t fifo_type)
{
    int tmp;

    struct acts_uart_controller *uart = UART_STRUCT(dev);

    tmp = uart->ctrl;

    if(is_tx){
        if(fifo_type > UART_FIFO_DMA){
            return -EINVAL;
        }
		while(!uart_irq_tx_complete(dev)){}
        //disable uart tx dma, tx threshold
        tmp &= (~(UART_CTL_TX_DST_MASK | UART_CTL_TX_FIFO_THREHOLD_MASK | UART_CTL_TX_DE | UART_CTL_TX_IE));

        tmp |=  UART_CTL_TX_DST(fifo_type);

        if(fifo_type == UART_FIFO_DMA){
            tmp |= UART_CTL_TX_FIFO_THREHOLD_8BYTES | UART_CTL_TX_DE;
        }else{
            tmp |= UART_CTL_TX_FIFO_THREHOLD_1BYTE;
        }
    }else{
        if(fifo_type > UART_FIFO_DMA){
            return -EINVAL;
        }

        //disable uart rx dma, rx threshold
        tmp &= (~(UART_CTL_RX_SRC_MASK | UART_CTL_RX_FIFO_THREHOLD_MASK | UART_CTL_RX_DE | UART_CTL_RX_IE));

        tmp |=  UART_CTL_RX_SRC(fifo_type);

        if(fifo_type == UART_FIFO_DMA){
            //dma
            tmp |= UART_CTL_RX_FIFO_THREHOLD_8BYTES | UART_CTL_RX_DE;
        }else{
            tmp |= UART_CTL_RX_FIFO_THREHOLD_1BYTE;
        }
    }

    uart->ctrl = tmp;

    return 0;
}

int uart_acts_dma_send_init(struct device *dev, dma_callback_t callback, void *arg)
{
    struct dma_config dma_cfg;
    struct dma_block_config head_block;
    struct acts_uart_data *uart_data = DEV_DATA(dev);
    struct acts_uart_config *uart_config = (struct acts_uart_config *)DEV_CFG(dev);

	if(!uart_config->flag_use_txdma){
		printk("err:dma not config by dts\n");
		return -1;
	}


	if(!uart_data->dma_dev){
	    uart_data->dma_dev = (struct device *)device_get_binding(uart_config->dma_dev_name);
		if (!uart_data->dma_dev){
			printk("err:get dma(%s) dev fail\n", uart_config->dma_dev_name);
			return -1;
		}
		uart_data->txdma_chan = dma_request(uart_data->dma_dev, uart_config->txdma_chan);
		printk("uart use txdma chan=%d\n", uart_data->txdma_chan);
	}

    memset(&dma_cfg, 0, sizeof(dma_cfg));
    memset(&head_block, 0, sizeof(head_block));

	if (callback) {
		dma_cfg.dma_callback = callback;
		dma_cfg.user_data = arg;
		dma_cfg.complete_callback_en = 1;
	}

	head_block.source_address = (unsigned int)NULL;
	head_block.dest_address = (unsigned int)NULL;
	head_block.block_size = 0;
	head_block.source_reload_en = 0;

	dma_cfg.block_count = 1;
	dma_cfg.head_block = &head_block;
	head_block.block_size = 0;

	dma_cfg.dma_slot = uart_config->txdma_id;
	dma_cfg.channel_direction = MEMORY_TO_PERIPHERAL;
	dma_cfg.dest_data_size = 1;
    dma_config(uart_data->dma_dev, uart_data->txdma_chan, &dma_cfg);

    return 0;
}

int uart_acts_dma_send(struct device *dev, char *s, int len)
{
    struct acts_uart_data *uart_data = DEV_DATA(dev);
   // struct acts_uart_config *uart_config = (struct acts_uart_config *)DEV_CFG(dev);
	struct acts_uart_controller *uart = UART_STRUCT(dev);
    __ASSERT(uart_data->dma_dev, "uart dma dev not config\n");

    dma_reload(uart_data->dma_dev, uart_data->txdma_chan, (unsigned int) s, (unsigned int)(&uart->txdat), len);
    dma_start(uart_data->dma_dev, uart_data->txdma_chan);

    return 0;
}

int uart_acts_dma_send_complete(struct device *dev)
{
    struct acts_uart_data *uart_data = DEV_DATA(dev);
    //struct acts_uart_config *uart_config = (struct acts_uart_config *)DEV_CFG(dev);
	struct dma_status stat;

    __ASSERT(uart_data->dma_dev, "uart dma dev not config\n");
	dma_get_status(uart_data->dma_dev, uart_data->txdma_chan, &stat);
	if(stat.pending_length==0)
		return 1;
    return 0 ;
}

int uart_acts_dma_send_stop(struct device *dev)
{
    struct acts_uart_data *uart_data = DEV_DATA(dev);
   // struct acts_uart_config *uart_config = (struct acts_uart_config *)DEV_CFG(dev);
    dma_stop(uart_data->dma_dev, uart_data->txdma_chan);
    return 0;
}

int uart_acts_dma_receive_init(struct device *dev, dma_callback_t callback, void *arg)
{
    struct dma_config dma_cfg;
    struct dma_block_config head_block;
    struct acts_uart_data *uart_data = DEV_DATA(dev);
    struct acts_uart_config *uart_config = (struct acts_uart_config *)DEV_CFG(dev);

	if(!uart_config->flag_use_rxdma){
		printk("err:dma rx not config by dts\n");
		return -1;
	}

	if(!uart_data->dma_dev){
	    uart_data->dma_dev = (struct device *)device_get_binding(uart_config->dma_dev_name);
		if (!uart_data->dma_dev){
			printk("err:get dma(%s) dev fail\n", uart_config->dma_dev_name);
			return -1;
		}
	}

    uart_data->rxdma_chan = dma_request(uart_data->dma_dev, uart_config->rxdma_chan);
    printk("uart use rxdma chan=%d\n", uart_data->rxdma_chan);

    memset(&dma_cfg, 0, sizeof(dma_cfg));
    memset(&head_block, 0, sizeof(head_block));

	if (callback) {
		dma_cfg.dma_callback = callback;
		dma_cfg.user_data = arg;
		dma_cfg.complete_callback_en = 1;
	}

	head_block.source_address = (unsigned int)NULL;
	head_block.dest_address = (unsigned int)NULL;
	head_block.block_size = 0;
	head_block.source_reload_en = 0;

	dma_cfg.block_count = 200;
	dma_cfg.head_block = &head_block;
	head_block.block_size = 0;

	dma_cfg.dma_slot = uart_config->rxdma_id;
	dma_cfg.channel_direction = PERIPHERAL_TO_MEMORY;
	dma_cfg.dest_data_size = 200;
    dma_config(uart_data->dma_dev, uart_data->rxdma_chan, &dma_cfg);

    return 0;
}

int uart_acts_dma_receive(struct device *dev, char *d, int len)
{
    struct acts_uart_data *uart_data = DEV_DATA(dev);
   // struct acts_uart_config *uart_config = (struct acts_uart_config *)DEV_CFG(dev);
	struct acts_uart_controller *uart = UART_STRUCT(dev);
    __ASSERT(uart_data->dma_dev, "uart dma dev not config\n");

    dma_reload(uart_data->dma_dev, uart_data->rxdma_chan, (unsigned int)(&uart->rxdat), (unsigned int)d, len);
    dma_start(uart_data->dma_dev, uart_data->rxdma_chan);

    return 0;
}

int uart_acts_dma_receive_complete(struct device *dev)
{
    struct acts_uart_data *uart_data = DEV_DATA(dev);
    //struct acts_uart_config *uart_config = (struct acts_uart_config *)DEV_CFG(dev);
	struct dma_status stat;

    __ASSERT(uart_data->dma_dev, "uart dma dev not config\n");
	dma_get_status(uart_data->dma_dev, uart_data->rxdma_chan, &stat);
	if(stat.pending_length==0)
		return 1;
    return 0 ;
}

int uart_acts_dma_receive_stop(struct device *dev)
{
    struct acts_uart_data *uart_data = DEV_DATA(dev);
	struct dma_status stat;

   // struct acts_uart_config *uart_config = (struct acts_uart_config *)DEV_CFG(dev);
	dma_get_status(uart_data->dma_dev, uart_data->rxdma_chan, &stat);
    dma_stop(uart_data->dma_dev, uart_data->rxdma_chan);

	return stat.pending_length;
}

int uart_acts_dma_receive_drq_switch(struct device *dev, bool drq_enable)
{
	struct acts_uart_controller *uart = UART_STRUCT(dev);

    if (drq_enable)
    {
	    uart->ctrl |= UART_CTL_RX_DE;
    }
    else
    {
	    uart->ctrl &= ~UART_CTL_RX_DE;
    }

    return 0;
}
#endif


static const struct uart_driver_api uart_acts_driver_api = {
	.poll_in          = uart_acts_poll_in,
	.poll_out         = uart_acts_poll_out,
	.configure		  = uart_acts_set_config,
	.config_get	  	  = uart_acts_get_config,
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.err_check        = uart_acts_err_check,
	.fifo_fill        = uart_acts_fifo_fill,
	.fifo_read        = uart_acts_fifo_read,
	.irq_tx_enable    = uart_acts_irq_tx_enable,
	.irq_tx_disable   = uart_acts_irq_tx_disable,
	.irq_tx_ready     = uart_acts_irq_tx_ready,
	.irq_rx_enable    = uart_acts_irq_rx_enable,
	.irq_rx_disable   = uart_acts_irq_rx_disable,
	.irq_tx_complete  = uart_acts_irq_tx_complete,
	.irq_rx_ready     = uart_acts_irq_rx_ready,
	.irq_is_pending   = uart_acts_irq_is_pending,
	.irq_update       = uart_acts_irq_update,
	.irq_callback_set = uart_acts_irq_callback_set,
#endif

};

#ifdef CONFIG_CFG_DRV
static void acts_pinmux_setup_pins_dynamic_uart(void)
{
    struct acts_pin_config pinconf[2];
    unsigned short pin_val;

#if (CONFIG_UART_CONSOLE_ON_DEV_NAME == CONFIG_UART_0_NAME)
    pinconf[0] = UART0_MFP_CFG;
    pinconf[1] = UART0_MFP_CFG;
#endif

#if (CONFIG_UART_CONSOLE_ON_DEV_NAME == CONFIG_UART_1_NAME)
    pinconf[0] = UART1_MFP_CFG;
    pinconf[1] = UART1_MFP_CFG;
#endif

    pin_val = GPIO_NONE;
    if(cfg_get_by_key(ITEM_UART_TX_PIN, &pin_val, 2) && (pin_val != GPIO_NONE)){
        pinconf[0].pin_num = pin_val&0xff;
        pinconf[0].mode = (pinconf[0].mode & (~GPIO_CTL_MFP_MASK)) | ((pin_val>>8) & GPIO_CTL_MFP_MASK);
    }
    pin_val = GPIO_NONE;
    if(cfg_get_by_key(ITEM_UART_RX_PIN, &pin_val, 2) && (pin_val != GPIO_NONE) ){
        pinconf[1].pin_num = pin_val&0xff;
        pinconf[1].mode = (pinconf[0].mode & (~GPIO_CTL_MFP_MASK)) | ((pin_val>>8)&GPIO_CTL_MFP_MASK);
    }
    acts_pinmux_setup_pins(pinconf, 2);
}

#endif

/**
 * @brief Initialize UART channel
 *
 * This routine is called to reset the chip in a quiescent state.
 * It is assumed that this function is called only once per UART.
 *
 * @param dev UART device struct
 *
 * @return 0
 */
static int uart_acts_init(const struct device *dev)
{
	const struct acts_uart_config *dev_cfg = DEV_CFG(dev);
	struct acts_uart_data * dev_data = DEV_DATA(dev);
	struct acts_uart_controller *uart = UART_STRUCT(dev);
	struct uart_config *cfg = &dev_data->cfg;
#ifdef CONFIG_CFG_DRV
	struct acts_pin_config pinconf[2];
	unsigned short pin_val;
#endif
	/* enable uart clock */
	acts_clock_peripheral_enable(dev_cfg->clock_id);
	/* clear reset */
	acts_reset_peripheral_deassert(dev_cfg->reset_id);

	/* wait busy if uart enabled */
	if (uart->ctrl & UART_CTL_EN) {
		while (uart->stat & UART_STA_UTBB);
	}


	/* enable rx/tx, 8/1/n */
	uart->ctrl = UART_CTL_EN |
		UART_CTL_RX_EN | UART_CTL_RX_FIFO_EN | UART_CTL_RX_FIFO_THREHOLD_1BYTE |
		UART_CTL_TX_EN | UART_CTL_TX_FIFO_EN | UART_CTL_TX_FIFO_THREHOLD_1BYTE |
		UART_CTL_DATA_WIDTH_8BIT | UART_CTL_STOP_1BIT | UART_CTL_PARITY_NONE;
#if 0
//#ifdef CONFIG_CFG_DRV
		acts_pinmux_setup_pins_dynamic_uart();
		if(!cfg_get_by_key(ITEM_UART_BAUDRATE,	&cfg->baudrate, 4)){
			cfg->baudrate = dev_cfg->baud_rate;
		}

		printk("drv cfg: console,baud=%d\n", dev_cfg->baud_rate);
//#else
#endif
	cfg->baudrate = dev_cfg->baud_rate;
//#endif
	cfg->parity = UART_CFG_PARITY_NONE;
	cfg->stop_bits = UART_CFG_STOP_BITS_1;
	cfg->data_bits = UART_CFG_DATA_BITS_8;
	cfg->flow_ctrl = UART_CFG_FLOW_CTRL_NONE;
	uart_acts_init_bycfg(dev);

	/* clear error status */
	//uart->stat = UART_STA_ERRS;

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
			DEV_CFG(dev)->irq_config_func(dev);
#endif

	return 0;
}


/*
.rxdma_id =DT_INST_DMAS_CELL_BY_NAME(n, rx, devid),\
.rxdma_chan = DT_INST_DMAS_CELL_BY_NAME(n, rx, channel),\

*/

#define  dma_use(n)  	(\
		.dma_dev_name =  CONFIG_DMA_0_NAME, \
		.txdma_id = CONFIG_UART_##n##_TX_DMA_ID,\
		.txdma_chan = CONFIG_UART_##n##_TX_DMA_CHAN,\
		.rxdma_id = CONFIG_UART_##n##_RX_DMA_ID,\
        .rxdma_chan = CONFIG_UART_##n##_RX_DMA_CHAN,\
		.flag_use_txdma = 1,					\
		.flag_use_rxdma = 1,					\
		)

#define dma_not(n)	(\
		.flag_use_txdma = 0, \
		.flag_use_rxdma = 0, \
		 )



#define UART_ACTS_DEFINE_CONFIG(n)					\
	static const struct device DEVICE_NAME_GET(uart_acts_##n);		\
									\
	static void uart##n##_acts_irq_config(const struct device *port)			\
	{								\
		IRQ_CONNECT(IRQ_ID_UART##n, CONFIG_UART_##n##_IRQ_PRI,	\
			    uart_acts_isr,				\
			    DEVICE_GET(uart_acts_##n), 0);		\
		irq_enable(IRQ_ID_UART##n);				\
	}												\
	static const struct acts_uart_config uart_acts_config_##n = {    \
		   .base = UART##n##_REG_BASE,\
		   .clock_id = CLOCK_ID_UART##n,\
		   .reset_id = RESET_ID_UART##n,\
		   .baud_rate = CONFIG_UART_##n##_SPEED, \
		    COND_CODE_1(CONFIG_UART_##n##_USE_TX_DMA,dma_use(n), dma_not(n))\
		   .irq_config_func = uart##n##_acts_irq_config,	\
	}

#define UART_ACTS_DEVICE_INIT(n)						\
	UART_ACTS_DEFINE_CONFIG(n);					\
	static struct acts_uart_data uart_acts_dev_data_##n ;\
	DEVICE_DEFINE(uart_acts_##n,				\
			    CONFIG_UART_##n##_NAME,				\
			    &uart_acts_init, NULL, &uart_acts_dev_data_##n,	\
			    &uart_acts_config_##n, PRE_KERNEL_1,		\
			    0, &uart_acts_driver_api);



#if IS_ENABLED(CONFIG_UART_0)
	UART_ACTS_DEVICE_INIT(0)
#endif
#if IS_ENABLED(CONFIG_UART_1)
	UART_ACTS_DEVICE_INIT(1)
#endif
#if IS_ENABLED(CONFIG_UART_2)
	UART_ACTS_DEVICE_INIT(2)
#endif
#if IS_ENABLED(CONFIG_UART_3)
	UART_ACTS_DEVICE_INIT(3)
#endif
#if IS_ENABLED(CONFIG_UART_4)
	UART_ACTS_DEVICE_INIT(4)
#endif


