/**
 *  ***************************************************************************
 *  Copyright (c) 2003-2021 Actions Semiconductor. All rights reserved.
 *
 *  \file       tool_uart.c
 *  \brief      uart access during tool connect
 *  \author     zhouxl
 *  \version    1.00
 *  \date       2021-9-11
 *  ***************************************************************************
 *
 *  \History:
 *  Version 1.00
 *       Initial release
 */

#include "tool_app_inner.h"
#include <drivers/uart.h>
#include <drivers/uart_dma.h>


#define UART_CTRL_RX_TEMP_BUF_SIZE  (200)
#define UART_CTRL_RX_DATA_BUF_SIZE  (1024)

/* DMA传输timeout时间
 */
#define UART_CTRL_RX_DMA_TIMEOUT_US   (2000)

#define UART_CONNECTION_INQUIRY_TIMER_MS    (2500)


/*
void dump_mem(unsigned char *data, unsigned int size)
{
    int i, j, len;
    unsigned char *buf = data;

    len = size;
    while (len)
    {
        printf("0x%08x:", (unsigned int)buf);

        if (len > 16)
        {
            j = 16;
        }
        else
        {
            j = len;
        }
        len -= j;

        for (i = 0; i < j; i++)
        {
            printf(" %02x", *buf);
            buf++;
        }
        printf("\n");
    }
}

void dump_word(unsigned int *data, unsigned int size)
{
    int i, j, len;
    unsigned int *buf = data;

    len = size;
    while (len)
    {
        printf("0x%08x:", (unsigned int)buf);

        if (len > 32)
        {
            j = 32;
        }
        else
        {
            j = len;
        }
        len -= j;

        for (i = 0; i < j; i += 4)
        {
            printf(" %08x", *buf);
            buf++;
        }
        printf("\n");
    }
}

void IO_Reg_Dump(void)
{
    uint32_t addr = 0x4001C100, i;
    for (i = 0; i < 10; i++)
    {
        printf("DMA%d\n", i);
        dump_word((unsigned int *)addr, 32);

        addr += 0x100;
    }

    printk("UART0\n");
    dump_word((unsigned int *)0x40038000, 20);
}
*/

void tool_uart_ctrl_rx_dma_start(struct device *dev)
{
#ifdef CONFIG_ACTIONS_PRINTK_DMA
    uart_dma_receive(dev, g_tool_data.temp_buff, UART_CTRL_RX_TEMP_BUF_SIZE);
#endif
    /* timeout后检查数据是否收完
     */
    hrtimer_start(&g_tool_data.timer, UART_CTRL_RX_DMA_TIMEOUT_US, 0);
}

void tool_uart_ctrl_rx_dma_stop(struct device *dev)
{
    hrtimer_stop(&g_tool_data.timer);
#ifdef CONFIG_ACTIONS_PRINTK_DMA
    uart_dma_receive_stop(dev);
#endif
}
#ifdef CONFIG_ACTIONS_PRINTK_DMA
static void tool_uart_ctrl_rx_dma_handler(const struct device *dma_dev, void *user_data,
			       uint32_t channel, int status)
{
    struct device* dev = get_tool_dev_device();

    // SYS_LOG_INF("%d", __LINE__);
    hrtimer_stop(&g_tool_data.timer);

    ring_buf_put(&g_tool_data.rx_ringbuf, g_tool_data.temp_buff, UART_CTRL_RX_TEMP_BUF_SIZE);

    tool_uart_ctrl_rx_dma_start((struct device*)dev);
}
#endif
static void timer_handler(struct hrtimer *timer, void *expiry_fn_arg)
{
    int count = 0, len;

    struct device* dev = get_tool_dev_device();


    hrtimer_stop(timer);
    // SYS_LOG_INF("%d", __LINE__);

    /* 读取 DMA 接收到的数据之前先禁止 DRQ ?
     */
#ifdef CONFIG_ACTIONS_PRINTK_DMA
    uart_dma_receive_drq_switch(dev, FALSE);
#endif
    /* 等待当前正在进行的 DMA 传输完成?
     */
    k_busy_wait(16);
#ifdef CONFIG_ACTIONS_PRINTK_DMA
    count = uart_dma_receive_stop(dev);
#endif
    /* DMA 当前已接收到的数据?
     */
    len = UART_CTRL_RX_TEMP_BUF_SIZE - count;

    if (len > 0)
    {
        /* 保存至 data_buf
         */
        ring_buf_put(&g_tool_data.rx_ringbuf, g_tool_data.temp_buff, len);
        // SYS_LOG_INF("DMA: %d", len);
    }

    /* 重新启用 DMA 接收数据
        */
#ifdef CONFIG_ACTIONS_PRINTK_DMA
    uart_dma_receive_drq_switch(dev, TRUE);
#endif
    tool_uart_ctrl_rx_dma_start(dev);
}

static void ttimer_handler(os_timer *ttimer)
{
    if (g_tool_data.connect)
    {
        g_tool_data.connect = 0;
        os_timer_start(ttimer, K_MSEC(UART_CONNECTION_INQUIRY_TIMER_MS), K_MSEC(0));
    }
    else
    {
        SYS_LOG_INF("uart disconnect");
        g_tool_data.quit = 1;
    }
}

void tool_uart_init(uint32_t buffer_size)
{
    struct device* dev = get_tool_dev_device();

    g_tool_data.rx_fbuf = app_mem_malloc(buffer_size);
	if (!g_tool_data.rx_fbuf) return;
	
    g_tool_data.temp_buff = app_mem_malloc(UART_CTRL_RX_TEMP_BUF_SIZE);
	if (!g_tool_data.temp_buff) return;
	
    g_tool_data.connect = 1;

    ring_buf_init(&g_tool_data.rx_ringbuf, buffer_size, g_tool_data.rx_fbuf);
    hrtimer_init(&g_tool_data.timer, timer_handler, NULL);
#ifdef CONFIG_ACTIONS_PRINTK_DMA
    uart_dma_receive_init(dev, tool_uart_ctrl_rx_dma_handler, NULL);

    uart_rx_dma_switch(dev, TRUE, NULL, NULL);
#endif
    tool_uart_ctrl_rx_dma_start(dev);

    os_timer_init(&g_tool_data.ttimer, ttimer_handler, NULL);
    os_timer_start(&g_tool_data.ttimer, K_MSEC(UART_CONNECTION_INQUIRY_TIMER_MS), K_MSEC(0));
}

void tool_uart_exit(void)
{
    struct device* dev = get_tool_dev_device();

    uart_irq_rx_disable(dev);
    tool_uart_ctrl_rx_dma_stop(dev);
#ifdef CONFIG_ACTIONS_PRINTK_DMA
    uart_rx_dma_switch(dev, FALSE, NULL, NULL);
    uart_dma_receive_exit(dev);
#endif
    os_timer_stop(&g_tool_data.ttimer);

    app_mem_free(g_tool_data.rx_fbuf);
    app_mem_free(g_tool_data.temp_buff);
	g_tool_data.init_work = false;
    g_tool_data.rx_fbuf = NULL;
    g_tool_data.temp_buff = NULL;
#ifdef CONFIG_SHELL_DBG
    void shell_dbg_restore(void);
    shell_dbg_restore();
#endif
}

int tool_uart_read(uint8_t *data, uint32_t size, uint32_t timeout_ms)
{
    int count, rx_size;
    uint32_t prev_uptime, cur_uptime;
    //uint8_t *data_ori = data;
    uint32_t size_ori = size;

    prev_uptime = k_uptime_get_32();

    rx_size = 0;
    while (0 != size)
    {
        count = ring_buf_get(&g_tool_data.rx_ringbuf, data, size);

        rx_size += count;

        if(data){
            data += count;
        }

        size -= count;

        if (0 == size)
            break;

        cur_uptime = k_uptime_get_32();
        if (cur_uptime - prev_uptime >= timeout_ms)
            break;

        k_msleep(1);
    }

    if(rx_size < size_ori)
    {
       SYS_LOG_DBG("read: %d, %d ", size_ori, rx_size);
    }

    return rx_size;
}

int tool_uart_write(const uint8_t *data, uint32_t size, uint32_t timeout_ms)
{
	int ret;
	g_tool_data.connect = 1;
#ifdef CONFIG_SHELL_DBG
    int shell_data_write(const uint8_t *data, uint32_t size, uint32_t timeout_ms);
    ret = shell_data_write(data, size, timeout_ms);
#else
#ifdef CONFIG_ACTIONS_PRINTK_DMA
	ret = uart_dma_send_buf(data, size);	
#endif
#endif

	if(ret != size){
		SYS_LOG_DBG("tool write %d expected: %d\n", ret, size);
	}
	
	return ret;
}

int tool_uart_ioctl(uint32_t mode, void *para1, void *para2)
{
    int ret = 0;
    // uart configuration
    struct uart_config uart_cfg =
    {
        .baudrate = (uint32_t)para1,
        .parity = UART_CFG_PARITY_NONE,
        .stop_bits = UART_CFG_STOP_BITS_1,
        .data_bits = UART_CFG_DATA_BITS_8,
        .flow_ctrl = UART_CFG_FLOW_CTRL_NONE,
    };

    switch (mode)
    {
        case USP_IOCTL_SET_BAUDRATE:
        ret = uart_configure(get_tool_dev_device(), &uart_cfg);
        break;

        default:
        break;
    }

    return ret;
}

