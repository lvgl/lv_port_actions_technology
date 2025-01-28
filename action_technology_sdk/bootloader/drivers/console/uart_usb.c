/** @file
 * @brief USB UART driver
 *
 * A UART driver which use USB CDC ACM protocol implementation.
 */

/*
 * Copyright (c) 2021 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <device.h>
#include <string.h>
#include <drivers/uart.h>
#include <drivers/console/uart_usb.h>
#include <drivers/usb/usb_phy.h>
#include <usb/class/usb_cdc.h>
#include <sys/ring_buffer.h>

#ifdef CONFIG_NVRAM_CONFIG
#include <drivers/nvram_config.h>
#endif

#define CFG_USB_UART_MODE			"USB_UART_MODE"

#define UART_USB_SEND_TIMEOUT_US	(1000)

#define UART_USB_PRINTK_BUFFER		(128)

extern void __stdout_hook_install(int (*hook)(int));
extern void *__stdout_get_hook(void);
extern void __printk_hook_install(int (*fn)(int));
extern void *__printk_get_hook(void);

static uint8_t __act_s2_notsave uart_usb_buffer[UART_USB_PRINTK_BUFFER];

typedef struct {
	const struct device *usb_dev;
	int (*stdout_bak)(int);
	int (*printk_bak)(int);
	uint8_t *buffer; /* buffer for printk */
	uint16_t cursor; /* buffer cursor */
	uint8_t state : 1; /* 1: initialized 0: uninitialized */
	uint8_t tx_done : 1; /* 1: tx done 0: tx not done  */
	uint8_t enable : 1; /* enable flag in nvram */
} uart_usb_dev_t, *p_uart_dev_t;

/* Get the USB UART device */
static p_uart_dev_t uart_usb_get_dev(void)
{
	static uart_usb_dev_t uart_usb_dev = {0};
	return &uart_usb_dev;
}

/* Updata the completion for USB UART TX interface */
void uart_usb_update_tx_done(void)
{
	p_uart_dev_t p_uart_dev = uart_usb_get_dev();
	p_uart_dev->tx_done = 1;
}

/* Send out data by USB UART interface */
int uart_usb_send(const uint8_t *data, int len)
{
	p_uart_dev_t p_uart_dev = uart_usb_get_dev();
	uint32_t start, duration;
	int ret;

	if (!p_uart_dev->state)
		return -ESRCH;

	uart_irq_tx_enable(p_uart_dev->usb_dev);

	p_uart_dev->tx_done = 0;
	ret = uart_fifo_fill(p_uart_dev->usb_dev, data, len);
	if (ret < 0)
		return ret;

	/* wait tx done */
	start = k_cycle_get_32();
	while (!p_uart_dev->tx_done) {
		duration = k_cycle_get_32() - start;
		if (SYS_CLOCK_HW_CYCLES_TO_NS_AVG(duration, 1000)
			> UART_USB_SEND_TIMEOUT_US) {
			uart_irq_tx_disable(p_uart_dev->usb_dev);
			return -ETIMEDOUT;
		}
	}
	uart_irq_tx_disable(p_uart_dev->usb_dev);

	return 0;
}

/* recevice data from USB UART interface */
int uart_usb_receive(uint8_t *data, int max_len)
{
	p_uart_dev_t p_uart_dev = uart_usb_get_dev();
	int rlen;

	memset(data, 0, max_len);

	rlen = uart_fifo_read(p_uart_dev->usb_dev, data, max_len);
	if (rlen < 0) {
		uart_irq_rx_disable(p_uart_dev->usb_dev);
		return -EIO;
	}

	/* exceptional that data received too much and drain all data */
	if (rlen >= max_len) {
		while (uart_fifo_read(p_uart_dev->usb_dev, data, max_len) > 0);
		rlen = 0;
	}

	return rlen;
}

/* Check whether USB UART is enabled or not */
bool uart_usb_is_enabled(void)
{
	p_uart_dev_t p_uart_dev = uart_usb_get_dev();

	return p_uart_dev->enable;
}

/* USB UART early init that controlled by nvram infomation */
static int uart_usb_early_init(const struct device *dev)
{
	char temp[16];
	int ret = 0;
	p_uart_dev_t p_uart_dev = uart_usb_get_dev();

	ARG_UNUSED(dev);

	memset(temp, 0, sizeof(temp));
#ifdef CONFIG_NVRAM_CONFIG
	ret = nvram_config_get(CFG_USB_UART_MODE, temp, 16);
#endif
	if (ret > 0) {
		if (!strcmp(temp, "true"))
			p_uart_dev->enable = 1;
	} else {
		/* by default to enable USB UART function */
		p_uart_dev->enable = 1;
	}
	printk("usb uart mode:%d\n", p_uart_dev->enable);

	return 0;
}

/* flash all printk buffer out */
static void uart_usb_flush_out(void)
{
	p_uart_dev_t p_uart_dev = uart_usb_get_dev();

	uart_usb_send(p_uart_dev->buffer, p_uart_dev->cursor);

	p_uart_dev->cursor = 0;
}

/* put the character into printk buffer or flush out data */
static int uart_usb_char_out(int c)
{
	p_uart_dev_t p_uart_dev = uart_usb_get_dev();

	/* If the cursor of buffer is up to the max boundary, flush out all data.
	  * Note that buffer reserves 2 characters for '\n\r'.
	  */
	if (p_uart_dev->cursor >= (UART_USB_PRINTK_BUFFER - 3)) {
		if ('\n' == c) {
			p_uart_dev->buffer[UART_USB_PRINTK_BUFFER - 3] = c;
			p_uart_dev->buffer[UART_USB_PRINTK_BUFFER - 2] = '\r';
			p_uart_dev->cursor = UART_USB_PRINTK_BUFFER - 1;
		} else {
			p_uart_dev->buffer[UART_USB_PRINTK_BUFFER - 3] = c;
			p_uart_dev->buffer[UART_USB_PRINTK_BUFFER - 2] = '\n';
			p_uart_dev->buffer[UART_USB_PRINTK_BUFFER - 1] = '\r';
			p_uart_dev->cursor = UART_USB_PRINTK_BUFFER;
		}
		uart_usb_flush_out();
	}

	/* check the endline indicator('\n') and flush line data */
	if ('\n' == c) {
		p_uart_dev->buffer[p_uart_dev->cursor++] = c;
		p_uart_dev->buffer[p_uart_dev->cursor++] = '\r';
		uart_usb_flush_out();
	} else {
		p_uart_dev->buffer[p_uart_dev->cursor++] = c;
	}

	return 0;
}

/* USB UART buffer init */
static void uart_usb_buf_init(void)
{
	p_uart_dev_t p_uart_dev = uart_usb_get_dev();
	p_uart_dev->buffer = uart_usb_buffer;
	p_uart_dev->cursor = 0;
}

/* USB UART initialization */
int uart_usb_init(void)
{
	p_uart_dev_t p_uart_dev = uart_usb_get_dev();

	/* check if has been initialized before */
	if (!p_uart_dev->enable)
		return -EPERM;

	if (!p_uart_dev->state) {
		p_uart_dev->usb_dev = device_get_binding(CONFIG_CDC_ACM_PORT_NAME);
		if (!p_uart_dev->usb_dev) {
			printk("failed to bind usb dev:%s\n", CONFIG_CDC_ACM_PORT_NAME);
			return -ENODEV;
		}

		/* USB CDC ACM class low level init */
		if (usb_cdc_acm_init((struct device *)p_uart_dev->usb_dev)) {
			printk("failed to init CDC ACM device\n");
			return -EFAULT;
		}

		/* buffer initialzation */
		uart_usb_buf_init();

		p_uart_dev->state = 1;
		p_uart_dev->tx_done = 0;

		/* backup stdout/printk hook for uninit stage */
		p_uart_dev->printk_bak = __printk_get_hook();
		p_uart_dev->stdout_bak = __stdout_get_hook();
		__stdout_hook_install(uart_usb_char_out);
		__printk_hook_install(uart_usb_char_out);

		printk("uart usb init ok\n");
	}

	return 0;
}

/* Un-init USB UART resources */
void uart_usb_uninit(void)
{
	p_uart_dev_t p_uart_dev = uart_usb_get_dev();

	if (p_uart_dev->state) {
		usb_cdc_acm_exit();
		p_uart_dev->state = 0;
		p_uart_dev->tx_done = 0;
		/* restore the origin stdout hook */
		__stdout_hook_install(p_uart_dev->stdout_bak);
		__printk_hook_install(p_uart_dev->printk_bak);
		printk("uart usb uninit ok\n");
	}
}

SYS_INIT(uart_usb_early_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_OBJECTS);

#if !defined(CONFIG_ACTIONS_PRINTK_DMA) && !defined(CONFIG_USB_HOTPLUG)
static int uart_usb_later_init(const struct device *dev)
{
	int ret;
	ARG_UNUSED(dev);

	/* usb device resource initialzation */
	usb_phy_enter_b_idle();
	usb_phy_init();

	ret = uart_usb_init();
	if (ret)
		return ret;

	return 0;
}
SYS_INIT(uart_usb_later_init, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_OBJECTS);
#endif
