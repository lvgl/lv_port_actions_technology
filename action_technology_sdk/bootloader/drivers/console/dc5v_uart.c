/*!
 * \file      dc5v_uart.c
 * \brief     DC5V_UART driver interface
 * \details
 * \author    
 * \date
 * \copyright Actions
 */

#include "dc5v_uart.h"


dc5v_uart_context_t  dc5v_uart_context;


static void debug_dump_reg(void)
{
	printk("rst: 0x%x\n", sys_read32((0x40000000)));
	printk("clk: 0x%x\n", sys_read32((0x40001004)));

    //dump uart1 reg
    printk("ctl: 0x%x\n", sys_read32((0x4003c000 + 0x0)));
	printk("rx: 0x%x\n", sys_read32((0x4003c000 + 0x4)));
	printk("tx: 0x%x\n", sys_read32((0x4003c000 + 0x8)));
	printk("stat: 0x%x\n", sys_read32((0x4003c000 + 0x0c)));
	printk("br: 0x%x\n", sys_read32((0x4003c000 + 0x10)));

	//dump pmu reg
	printk("gdc5v: 0x%x\n", sys_read32((0x40068000 + 0x310)));
    printk("sys_set: 0x%x\n", sys_read32((0x40004000 + 0x108)));	

#if 1
	printk("pmu_det: 0x%x\n", sys_read32((0x40004000 + 0x10)));
	printk("ch_ctl: 0x%x\n", sys_read32((0x40004000 + 0x100)));
    printk("pmu sys_set: 0x%x\n", sys_read32((0x40004000 + 0x108)));
	printk("wk_ctl: 0x%x\n", sys_read32((0x40004000 + 0x110)));
	printk("wk_pd: 0x%x\n", sys_read32((0x40004000 + 0x114)));
	printk("cmu_s1: 0x%x\n", sys_read32((0x40001000 + 0xd0)));
#endif

}


static u32_t get_diff_time(u32_t end_time, u32_t begin_time)
{
    u32_t  diff_time;
    
    if (end_time >= begin_time)
    {
        diff_time = (end_time - begin_time);
    }
    else
    {
        diff_time = ((u32_t)-1 - begin_time + end_time + 1);
    }

    return diff_time;
}


/**
**	set register in pmusvcc 
**/
void sys_svcc_reg_write(u32_t reg_addr, u32_t mask, u32_t value)
{
    u32_t  tmp = sys_read32(reg_addr);
    int    i;

    tmp = (tmp & ~mask) | (value & mask);

    for (i = 0; i < 10; i++)
    {
        sys_write32(tmp, reg_addr);

        os_delay(200);

        if ((sys_read32(reg_addr) & mask) == (tmp & mask))
        {
            break;
        }
    }
}

/**
**	设置DC5V Uart切换电压
**/
void dc5v_uart_set_switch_volt(u32_t switch_volt)
{
    if (switch_volt != DC5V_UART_SWITCH_VOLT_NA)
    {
        sys_svcc_reg_write
        (
            (u32_t)PMU_WKUP_CTL_REG, 
            ((WKEN_CTL_DC5V_LHV_VOL_MASK) | 
             WKEN_CTL_DC5VLV_WKEN
            ), 
            ((switch_volt << WKEN_CTL_DC5V_LHV_VOL_SHIFT) | 
             WKEN_CTL_DC5VLV_WKEN)
        );
    }
    else
    {
        sys_svcc_reg_write
        (
            (u32_t)PMU_WKUP_CTL_REG,  
            WKEN_CTL_DC5VLV_WKEN, 
            (0 << WKEN_CTL_DC5VLV_WKEN_SHIFT)
        );
    }
    
    /* Must delay some time to take effect?
     */
    os_sleep(10);

	printk("wkup_ctl: 0x%x", sys_read32(PMU_WKUP_CTL_REG));
}

/**
**	将DC5V Uart 设置到RX模式
**/
void dc5v_uart_set_rx_mode(void)
{
    u32_t  key = irq_lock();

    dc5v_uart_context_t*  dc5v_uart = &dc5v_uart_context;

    printk("Switch to RX!");
	
    dc5v_uart->trx_mode = DC5V_UART_RX_MODE;

    //MFP
    dc5v_uart->bak_GPIO_DC5V_CTL &= ~DC5V_CTL_AD_SEL;
    dc5v_uart->bak_GPIO_DC5V_CTL &= ~DC5V_CTL_MFP_MASK;

    if(strcmp(CONFIG_UART_DC5V_ON_DEV_NAME, "UART_1") == 0)
    {
	    dc5v_uart->bak_GPIO_DC5V_CTL |= DC5V_CTL_MFP_UART1RX;
    }
	else
	{
	    dc5v_uart->bak_GPIO_DC5V_CTL |= DC5V_CTL_MFP_UART2RX;
	}
    //dc5v_uart->bak_GPIO_DC5V_CTL |= DC5V_CTL_10KPU_EN;

	sys_write32(dc5v_uart->bak_GPIO_DC5V_CTL, GPIO_DC5V_CTL_REG);

    irq_unlock(key);
}

/**
**	将DC5V Uart 设置到TX模式
**/
static inline void dc5v_uart_set_tx_mode(void)
{
    dc5v_uart_context_t*  dc5v_uart = &dc5v_uart_context;

	printk("Switch to TX!");

    k_timer_stop(&dc5v_uart->tx_context.tx_check_timer);

    if (dc5v_uart->trx_mode != DC5V_UART_TX_MODE)
    {
        dc5v_uart->trx_mode = DC5V_UART_TX_MODE;
        
        //MFP
        dc5v_uart->bak_GPIO_DC5V_CTL &= ~DC5V_CTL_AD_SEL;
        dc5v_uart->bak_GPIO_DC5V_CTL &= ~DC5V_CTL_MFP_MASK;

        if(strcmp(CONFIG_UART_DC5V_ON_DEV_NAME, "UART_1") == 0)
        {
	        dc5v_uart->bak_GPIO_DC5V_CTL |= DC5V_CTL_MFP_UART1TX;
        }
		else
		{
		    dc5v_uart->bak_GPIO_DC5V_CTL |= DC5V_CTL_MFP_UART2TX;
		}
		//dc5v_uart->bak_GPIO_DC5V_CTL |= DC5V_CTL_10KPU_EN;

	    sys_write32(dc5v_uart->bak_GPIO_DC5V_CTL, GPIO_DC5V_CTL_REG);		

        k_timer_stop(&dc5v_uart->tx_context.tx_switch_timer);

		dc5v_uart->tx_context.tx_switch_finish = false;
        k_timer_start(&dc5v_uart->tx_context.tx_switch_timer, K_USEC(200), K_NO_WAIT);
    }
}

/**
**	使能或关闭DC5V Uart功能
**/
void dc5v_uart_set_enable(bool enable)
{
    u32_t  key = irq_lock();

    dc5v_uart_context_t*  dc5v_uart = &dc5v_uart_context;

    if (enable)
    {
        printk("enable DC5V Uart!\n\n\n\n\n");

		#if 0
        dc5v_uart->bak_PMU_SYSTEM_SET_SVCC |= (SYSSET_UART_SW_SEL | SYSSET_UART_SW_MODE | SYSSET_UART_PWR_SEL);
        #else
		dc5v_uart->bak_PMU_SYSTEM_SET_SVCC &= (~SYSSET_UART_SW_MODE);
        dc5v_uart->bak_PMU_SYSTEM_SET_SVCC |= SYSSET_UART_SW_SEL;
        #endif
		
		sys_write32(dc5v_uart->bak_PMU_SYSTEM_SET_SVCC, PMU_SYS_SET_REG);
	
        /* Default set RX mode
         */
        dc5v_uart_set_rx_mode();
    }
    else
    {
        //switch to dc5v analog func
        printk("disable DC5V Uart!\n\n\n\n\n");
		
        dc5v_uart->bak_GPIO_DC5V_CTL |= DC5V_CTL_AD_SEL;
        sys_write32(dc5v_uart->bak_GPIO_DC5V_CTL, GPIO_DC5V_CTL_REG);
    }
    
    irq_unlock(key);
}

/**
**	dc5v uart 将buffer数据输出
**/
int dc5v_uart_writedata(struct device *uart, const u8_t* buf, u32_t len, u32_t wait_end)
{
	dc5v_uart_context_t*  dc5v_uart = &dc5v_uart_context;

    while(uart_acts_dma_send_complete((struct device *)dc5v_uart->tx_context.tx_dev) == 0)
    {
	    printk("wait tx dma finish!");
    }

	uart_dma_send((struct device *)dc5v_uart->tx_context.tx_dev, (char*)buf, len);

    if (wait_end != 0)
    {
        /* 等待 TX FIFO 所有数据传输完成 */
        while (uart_irq_tx_complete(dc5v_uart->tx_context.tx_dev) == 0)
            ;
    }

    return len;
}

/**
**	dc5v uart 切换到tx模式完成
**/
void dc5v_uart_tx_switch_complete(struct k_timer *timer)
{
    u32_t key = irq_lock();

    dc5v_uart_context_t*  dc5v_uart = &dc5v_uart_context;

	k_timer_stop(&dc5v_uart->tx_context.tx_switch_timer);
	
    if (dc5v_uart->tx_context.tx_buf != NULL)
    {
        u8_t*  tx_buf = dc5v_uart->tx_context.tx_buf;
        u16_t  tx_len = dc5v_uart->tx_context.tx_len;

        dc5v_uart->tx_context.tx_buf = NULL;
        dc5v_uart->tx_context.tx_len = 0;
        
        dc5v_uart_writedata((struct device *)dc5v_uart->tx_context.tx_dev, tx_buf, tx_len, 0);
    }

	dc5v_uart->tx_context.tx_switch_finish = true;

    irq_unlock(key);
}

/**
**	延时到后，关闭dc5v uart
**/
void dc5v_uart_delay_disable(struct k_timer *timer)
{
    u32_t  key = irq_lock();

    dc5v_uart_context_t*  dc5v_uart = &dc5v_uart_context;

    dc5v_uart->enabled = NO;

    k_timer_stop(&dc5v_uart->disable_timer);

    dc5v_uart_set_enable(NO);

    irq_unlock(key);
}


/**
**	重启disable timer
**/
static inline void dc5v_uart_reset_delay_disable(void)
{
    dc5v_uart_context_t*  dc5v_uart = &dc5v_uart_context;

    if (dc5v_uart->disable_delay_ms > 0)
    {
        k_timer_stop(&dc5v_uart->disable_timer);
        k_timer_start(&dc5v_uart->disable_timer, K_USEC(dc5v_uart->disable_delay_ms * 1000), K_NO_WAIT);
    }
}

static inline int dc5v_uart_tx_write(void* buf, int len)
{
    u32_t  key = irq_lock();
    
    dc5v_uart_context_t*  dc5v_uart = &dc5v_uart_context;
    
    int  ret_val = 0;
    
	// last dma not finish
	if(uart_acts_dma_send_complete((struct device *)dc5v_uart->tx_context.tx_dev) == 0)	
    {
        goto end;
    }
    
    dc5v_uart_set_tx_mode();
    
    if (!dc5v_uart->tx_context.tx_switch_finish)
    {
        dc5v_uart->tx_context.tx_buf = (u8_t*)buf;
        dc5v_uart->tx_context.tx_len = (u16_t)len;
    }
    else
    {
        dc5v_uart_writedata((struct device *)dc5v_uart->tx_context.tx_dev, buf, len, 0);
    }

    dc5v_uart->last_io_time = k_uptime_get_32();
    
    dc5v_uart_reset_delay_disable();
    
    ret_val = len;

end:
    irq_unlock(key);

    return ret_val;
}


/**
**	检查DC5V Uart的写操作是否完成
**/
bool dc5v_uart_check_write_finish(void)
{
    dc5v_uart_context_t*  dc5v_uart = &dc5v_uart_context;

	if(uart_acts_dma_send_complete((struct device *)dc5v_uart->tx_context.tx_dev) == 0)
	{
	    return NO;
	}
	
    if(uart_irq_tx_complete(dc5v_uart->tx_context.tx_dev) == 0)
    {
        return NO;
    }
    
    return YES;
}


/**
**	DC5V Uart 写接口
**/
int dc5v_uart_api_write(struct device* uart, const u8_t* buf, u32_t len, u32_t flags)
{
    int   ret_val;
        
    if (buf == NULL || len == 0)
    {
        return 0;
    }
	
    do
    {
        ret_val = dc5v_uart_tx_write((void*)buf, len);
    }
    while (ret_val == 0 && (!k_is_in_isr()));

    if (ret_val != 0 &&
        flags   != 0)
    {
        while (!dc5v_uart_check_write_finish())
            ;
    }

    return ret_val;
}


void re_config_uart_param(u32_t baudrate)
{
	struct uart_config uart_cfg;
	dc5v_uart_context_t*  dc5v_uart = &dc5v_uart_context;

    uart_cfg.baudrate = baudrate;
	uart_cfg.parity = dc5v_uart->cfg.DC5V_UART_Parity_Select;
    uart_cfg.stop_bits = UART_CFG_STOP_BITS_1;
	uart_cfg.data_bits = UART_CFG_DATA_BITS_8;
	uart_cfg.flow_ctrl = UART_CFG_FLOW_CTRL_NONE;	

	printk("config DC5V Uart BR: %d\n", uart_cfg.baudrate);

    uart_configure(dc5v_uart->tx_context.tx_dev, &uart_cfg);

	dc5v_uart->baud_rate = uart_cfg.baudrate;
}


int dc5v_uart_api_ioctl(struct device* uart, u32_t mode, void* param1, void* param2)
{
    int ret = 0;
	dc5v_uart_context_t*  dc5v_uart = &dc5v_uart_context;

    if (dc5v_uart->dc5v_uart_init == 0)
    {
        return 0;
    }

    switch (mode)
    {
        case UART_SET_BAUDRATE:
		if(param1 == (void*)0)
		{
		    param1 = (void*)dc5v_uart->cfg.DC5V_UART_Comm_Baudrate;
		}
		if((u32_t)param1 != dc5v_uart->baud_rate)
		{
            re_config_uart_param((u32_t)param1);
		}
        break;

        case UART_CHECK_WRITE_FINISH:
        ret = dc5v_uart_check_write_finish();
        break;

        case UART_IS_RX_FIFO_EMPTY:
        if(uart_irq_rx_ready(dc5v_uart->rx_context.rx_dev))
		{
			ret = 0;
		}
		else
		{
		    ret = 1;
		}
        break;

        case UART_RX_DMA_ACCESS_SWITCH:
        if ((u32_t)param1)
        {
            uart_rx_dma_switch((struct device *)dc5v_uart->rx_context.rx_dev, TRUE, NULL, NULL);
        }
        else
        {
            uart_rx_dma_switch((struct device *)dc5v_uart->rx_context.rx_dev, FALSE, NULL, NULL);
        }
        break;

        default:
        break;
    }

    return ret;
}


/**
**	tx finish check timer
**/
void dc5v_uart_check_tx_complete(struct k_timer *timer)
{
    dc5v_uart_context_t*  dc5v_uart = &dc5v_uart_context;
    
	k_timer_stop(&dc5v_uart->tx_context.tx_check_timer);

    if(uart_irq_tx_complete(dc5v_uart->tx_context.tx_dev) != 0)
    {
        dc5v_uart_set_rx_mode();
    }
    else
    {
		k_timer_start(&dc5v_uart->tx_context.tx_check_timer, K_USEC(200), K_NO_WAIT);	
    }
}


/**
**	tx dma handle
**/
void dc5v_uart_tx_dma_handler(const struct device *dev, void *user_data,
					   u32_t channel, int status)
{
    if (status == DC5V_UART_DMA_IRQ_TC)
    {
		u32_t key = irq_lock();
        
        dc5v_uart_context_t*  dc5v_uart = &dc5v_uart_context;

		k_timer_stop(&dc5v_uart->tx_context.tx_check_timer);
		k_timer_start(&dc5v_uart->tx_context.tx_check_timer, K_USEC(200), K_NO_WAIT);

        irq_unlock(key);
    }
}

/**
**	start dma to receive data
**/
void uart_ctrl_rx_dma_start(struct device *dev)
{
    uart_rx_dma_switch(dev, TRUE, NULL, NULL);
    uart_dma_receive(dev, dc5v_uart_context.rx_context.dma_buf, UART_CTRL_RX_DMA_BUF_SIZE);

    /* 定时一小段时间后将 DMA 接收的数据保存至 data_buf
     */
    k_timer_start(&dc5v_uart_context.rx_context.rx_timer, K_USEC(UART_CTRL_RX_DMA_TIMER_US), K_NO_WAIT);
}

/**
**	dc5v uart rx irq handle
**/
void dc5v_uart_rx_irq_handler(struct device *dev, void *user_data)
{
    u32_t  key = irq_lock();
	
    /* 暂时禁止 RX 中断
     */
    uart_irq_rx_disable(dev);

    /* 启用 DMA 进行数据接收
     */
    uart_ctrl_rx_dma_start(dev);

    irq_unlock(key);
}

/**
**	dc5v uart irq handle
**/
static void dc5v_uart_irq_callback(const struct device *dev, void *user_data)
{
    //printk("rx irq!\n");

	uart_irq_update(dev);

	if (uart_irq_rx_ready(dev)) {
		dc5v_uart_rx_irq_handler((struct device *)dev, user_data);
	}
}

void uart_ctrl_rx_timer_handler(struct k_timer *timer)
{
    u32_t  key = irq_lock();

	dc5v_uart_context_t*  dc5v_uart = &dc5v_uart_context;
	struct device* dev = (struct device *)dc5v_uart->rx_context.rx_dev;

    int   len, count;


    /* 读取 DMA 接收到的数据之前先禁止 DRQ ?
     */
	uart_dma_receive_drq_switch(dev, FALSE);

    /* 等待当前正在进行的 DMA 传输完成?
     */
    k_busy_wait(1);

    k_timer_stop(&dc5v_uart->rx_context.rx_timer);

	count = uart_dma_receive_stop(dev);

    /* DMA 当前已接收到的数据?
     */
	len = UART_CTRL_RX_DMA_BUF_SIZE - count;

    if (len > 0)
    {
        /* 保存至 data_buf
         */
        //printk("put: 0x%x--%d\n", dc5v_uart->rx_context.dma_buf[0], len);
		ring_buf_put(&dc5v_uart->rx_context.rx_rbuf, dc5v_uart->rx_context.dma_buf, len);
    }
    /* RX_FIFO 中还有剩余数据时使用 CPU 进行读取
     */
    else
    {
        uart_rx_dma_switch(dev, FALSE, NULL, NULL);
        count = uart_fifo_read(dev, dc5v_uart->rx_context.dma_buf, UART_CTRL_RX_DMA_BUF_SIZE);

        if(count > 0)
        {
		    //printk("put2: 0x%x--%d\n", dc5v_uart->rx_context.dma_buf[0],count);
            ring_buf_put(&dc5v_uart->rx_context.rx_rbuf, dc5v_uart->rx_context.dma_buf, count);
			len += count;
        }
    }

    if (len > 0)
    {
        /* 重新使能 DRQ
        */
        uart_dma_receive_drq_switch(dev, TRUE);
        /* 重新启用 DMA 接收数据
         */
        uart_ctrl_rx_dma_start(dev);
    }
    else
    {
        /* 重新使能 RX 中断
         */
        uart_irq_rx_enable(dev);
    }

    irq_unlock(key);
}


/**
**	rx timer handle
**/
void dc5v_uart_rx_timer_handler(struct k_timer *timer)
{
    u32_t  key = irq_lock();
    
    dc5v_uart_context_t*  dc5v_uart = &dc5v_uart_context;

    u32_t  data_count = UART_CTRL_RX_DATA_BUF_SIZE - ring_buf_space_get(&dc5v_uart->rx_context.rx_rbuf);

    uart_ctrl_rx_timer_handler(timer);

    if ((UART_CTRL_RX_DATA_BUF_SIZE - ring_buf_space_get(&dc5v_uart->rx_context.rx_rbuf)) > data_count)
    {
        dc5v_uart->last_io_time = k_uptime_get_32();
        dc5v_uart_reset_delay_disable();
    }

    irq_unlock(key);
}


/**
**	read data by dc5v uart
**/
int dc5v_uart_rx_timed_read(void* buf, u32_t len, u32_t timeout_ms)
{
    dc5v_uart_context_t*  dc5v_uart = &dc5v_uart_context;

    int    count = 0;
    u32_t  t;

    if (dc5v_uart->dc5v_uart_init == 0)
    {
        return 0;
    }

    printk("DC5V Uart Read!");

    t = k_uptime_get_32();

    while (count < len)
    {
        u8_t*  ptr;
        int    n;

        if (buf != NULL)
        {
            ptr = (u8_t*)buf + count;
        }
        else
        {
            ptr = NULL;
        }
        
        n = ring_buf_get(&dc5v_uart->rx_context.rx_rbuf, ptr, len - count);
        
        if (n <= 0)
        {
            os_sleep(1);
        
            n = ring_buf_get(&dc5v_uart->rx_context.rx_rbuf, ptr, len - count);
        }
        
        if (n > 0)
        {
            count += n;
            t = k_uptime_get_32();
        }
        else
        {
            if (get_diff_time(k_uptime_get_32(), t) >= timeout_ms)
            {
                break;
            }
        }
    }

    return count;
}

/**
**	clear dc5v uart rx buffer
**/
void dc5v_uart_clear_rx_buf(u32_t wait_ms)
{
    u32_t  key;
    dc5v_uart_context_t*  dc5v_uart = &dc5v_uart_context;
    struct ring_buf*  rx_buf = &dc5v_uart->rx_context.rx_rbuf;

    if (wait_ms > 0)
    {
        os_sleep(wait_ms);
    }

    key = irq_lock();

	ring_buf_reset(rx_buf);

    irq_unlock(key);
}

/**
**	dc5v uart suspend
**/
void dc5v_uart_ctrl_suspend(void)
{
    dc5v_uart_context_t*  dc5v_uart = &dc5v_uart_context;

    if (!dc5v_uart->suspended)
    {
        dc5v_uart->suspended = YES;
    
        while (!dc5v_uart_check_write_finish())
            ;

        dc5v_uart_set_enable(NO);
        dc5v_uart_set_switch_volt(DC5V_UART_SWITCH_VOLT_NA);

    }
}


void dc5v_uart_ctrl_resume(void)
{
    dc5v_uart_context_t*  dc5v_uart = &dc5v_uart_context;

    if (dc5v_uart->suspended)
    {
        dc5v_uart->suspended = NO;
    
        dc5v_uart_set_switch_volt(dc5v_uart->cfg.DC5V_UART_Switch_Voltage);
        dc5v_uart_set_enable(dc5v_uart->enabled);
        
        dc5v_uart_clear_rx_buf(10);
    }
}

void dc5v_uart_set_rx_buf_size(u32_t buf_size)
{
    u32_t  key = irq_lock();

    dc5v_uart_context_t*  dc5v_uart = &dc5v_uart_context;

    if ((u32_t)dc5v_uart->rx_context.rx_rbuf.buf.buf32 != (u32_t)dc5v_uart->rx_context.data_buf)
    {
        app_mem_free((void*)dc5v_uart->rx_context.rx_rbuf.buf.buf32);
    }

    if (buf_size <= UART_CTRL_RX_DATA_BUF_SIZE)
    {
        ring_buf_init(&dc5v_uart->rx_context.rx_rbuf, UART_CTRL_RX_DATA_BUF_SIZE, dc5v_uart->rx_context.data_buf);
    }
    else
    {
        void*  data_buf = app_mem_malloc(buf_size);
        
        ring_buf_init(&dc5v_uart->rx_context.rx_rbuf, buf_size, data_buf);
    }

    irq_unlock(key);
}


void dc5v_uart_set_enable_ex(bool enable, u32_t delay_disable)
{
    u32_t  key = irq_lock();

    dc5v_uart_context_t*  dc5v_uart = &dc5v_uart_context;

    if (enable)
    {
        //dc5v_uart->disable_delay_ms = 0;
        k_timer_stop(&dc5v_uart->disable_timer);

        if (!dc5v_uart->enabled)
        {
            dc5v_uart->enabled = YES;
            dc5v_uart_set_enable(YES);
        }
    }
    else
    {
        dc5v_uart->disable_delay_ms = delay_disable;		
        k_timer_stop(&dc5v_uart->disable_timer);
        k_timer_start(&dc5v_uart->disable_timer, K_USEC(dc5v_uart->disable_delay_ms * 1000), K_NO_WAIT);
    }

    irq_unlock(key);
}

bool dc5v_uart_check_io_time(u32_t ms)
{
    dc5v_uart_context_t*  dc5v_uart = &dc5v_uart_context;

    if (dc5v_uart->disable_delay_ms > 0)
    {
        return YES;
    }

    if (dc5v_uart->redirect_console_print &&
        dc5v_uart->enabled &&
        dc5v_uart->suspended == NO)
    {
        return YES;
    }

    if (dc5v_uart->last_io_time != 0 &&
        get_diff_time(k_uptime_get_32(), dc5v_uart->last_io_time) < ms)
    {
        return YES;
    }

    return NO;
}



int dc5v_uart_rx_read_byte(u8_t* byte)
{
    dc5v_uart_context_t*  dc5v_uart = &dc5v_uart_context;
	uart_ctrl_rx_context_t*  rx = &dc5v_uart->rx_context;

    if (ring_buf_get(&rx->rx_rbuf, byte, 1) != 0)
    {
        printk("Read 1byte: 0x%x\n", *byte);
        return 1;
    }

    return 0;
}


void dc5v_uart_rx_deal(void)
{
    dc5v_uart_context_t*  dc5v_uart = &dc5v_uart_context;
	uart_ctrl_rx_context_t*  rx = &dc5v_uart->rx_context;
	u8_t  byte;

    while(dc5v_uart->rxdeal_need_quit == 0)
    {
        if(ring_buf_is_empty(&dc5v_uart->rx_context.rx_rbuf))
        {
            os_sleep(20);
            continue;
        }

        if (dc5v_uart_rx_read_byte(&byte) == 0)
        {
            continue;
        }

        if (rx->rx_data_handler != NULL)
        {
            //printk("Handle: %d", byte);
            rx->rx_data_handler(byte);
        }
    }

    dc5v_uart->rxdeal_quited = 1;

	printk("RX deal thread Exit!\n\n");
	
    return;
}


/**	init
**/
bool dc5v_uart_ctrl_init(CFG_Type_DC5V_UART_Comm_Settings* cfg)
{
    dc5v_uart_context_t*  dc5v_uart = &dc5v_uart_context;

    if (dc5v_uart_context.dc5v_uart_init != 0)
    {
        return YES;
    }

    if (cfg->Enable_DC5V_UART_Comm_Mode == NO ||
        cfg->DC5V_UART_Switch_Voltage == DC5V_UART_SWITCH_VOLT_NA ||
        cfg->DC5V_UART_Comm_Baudrate < 1000)
    {
        return NO;
    }

    cfg->Redirect_Console_Print = 0;

    memset(dc5v_uart, 0, sizeof(dc5v_uart_context_t));

	dc5v_uart->bak_PMU_SYSTEM_SET_SVCC = sys_read32(PMU_SYS_SET_REG);
	dc5v_uart->bak_GPIO_DC5V_CTL = sys_read32(GPIO_DC5V_CTL_REG);	

    dc5v_uart->cfg = *cfg;

    /* DC5V_UART_DEV
     */
	dc5v_uart->tx_context.tx_dev = (struct device *)device_get_binding(CONFIG_UART_DC5V_ON_DEV_NAME);
    dc5v_uart->rx_context.rx_dev = (struct device *)device_get_binding(CONFIG_UART_DC5V_ON_DEV_NAME);

    /* tx config
     */
    k_timer_init(&dc5v_uart->tx_context.tx_check_timer, dc5v_uart_check_tx_complete, NULL);
	k_timer_init(&dc5v_uart->tx_context.tx_switch_timer, dc5v_uart_tx_switch_complete, NULL);
	k_timer_init(&dc5v_uart->disable_timer, dc5v_uart_delay_disable, NULL);

    uart_dma_send_init((struct device *)dc5v_uart->tx_context.tx_dev, dc5v_uart_tx_dma_handler, NULL);
	uart_tx_dma_switch((struct device *)dc5v_uart->tx_context.tx_dev, TRUE, NULL, NULL);

    /* rx config
     */
	ring_buf_init(&dc5v_uart->rx_context.rx_rbuf, UART_CTRL_RX_DATA_BUF_SIZE, dc5v_uart->rx_context.data_buf);	

    k_timer_init(&dc5v_uart->rx_context.rx_timer, dc5v_uart_rx_timer_handler, NULL);

	uart_dma_receive_init((struct device *)dc5v_uart->rx_context.rx_dev,	NULL, NULL);
	uart_rx_dma_switch((struct device *)dc5v_uart->rx_context.rx_dev, FALSE, NULL, NULL);

	uart_irq_callback_set(dc5v_uart->rx_context.rx_dev, dc5v_uart_irq_callback);
	uart_irq_rx_enable(dc5v_uart->rx_context.rx_dev);

    /* reconfig uart param
    */
	re_config_uart_param(dc5v_uart->cfg.DC5V_UART_Comm_Baudrate);
	
    /*	set switch volt
     */
    dc5v_uart_set_switch_volt(cfg->DC5V_UART_Switch_Voltage);
	
    if (cfg->Redirect_Console_Print)
    {
        dc5v_uart_set_enable((dc5v_uart->enabled = YES));
        dc5v_uart_clear_rx_buf(10);
    }
    else
    {
        dc5v_uart_set_enable((dc5v_uart->enabled = NO));
    }

	dc5v_uart->dc5v_uart_init = 1;

    return YES;
}


static void dc5v_uart_start_rxdeal(void)
{
	dc5v_uart_context_t*  dc5v_uart = &dc5v_uart_context;

    if (dc5v_uart->dc5v_uart_init != 0)
    {
        dc5v_uart->rxdeal_need_quit = 0;
		dc5v_uart->rxdeal_quited = 0;
	    dc5v_uart->rxdeal_thread_stack = app_mem_malloc(1024);

	    os_thread_create(dc5v_uart->rxdeal_thread_stack, UART_RX_DATADEAL_STACK,
			    (void *)dc5v_uart_rx_deal, NULL, NULL, NULL, UART_RX_THREAD_PRIO, 0, 0);

        #if 1
        dc5v_uart_set_enable((dc5v_uart->enabled = YES));
	    dc5v_uart_clear_rx_buf(10);	
        #endif

        debug_dump_reg();
    }
	else
	{
	    printk("Err: dc5v uart not init!\n");
	}
}

static void dc5v_uart_stop_rxdeal(void)
{
    dc5v_uart_context_t*  dc5v_uart = &dc5v_uart_context;
	
	if (!dc5v_uart->rxdeal_thread_stack)
	{
		return;
	}

	dc5v_uart->rxdeal_need_quit = 1;
	while (!dc5v_uart->rxdeal_quited)
		os_sleep(1);

	app_mem_free(dc5v_uart->rxdeal_thread_stack);
	dc5v_uart->rxdeal_thread_stack = NULL;
}

int dc5v_uart_operate(u32_t cmd, void* param1, u32_t param2, u32_t param3)
{
    int  ret_val = 0;
    
    if (cmd == DC5V_UART_INIT)
    {
        ret_val = dc5v_uart_ctrl_init(param1);
        return ret_val;
    }

    if (dc5v_uart_context.dc5v_uart_init == 0)
    {
        printk("not init yet!");
        return 0;
    }

    switch (cmd)
    {
        case DC5V_UART_SUSPEND:
        {
            dc5v_uart_ctrl_suspend();
            break;
        }
        
        case DC5V_UART_RESUME:
        {
            dc5v_uart_ctrl_resume();
            break;
        }
        
        case DC5V_UART_SET_RX_DATA_HANDLER:
        {
            dc5v_uart_context.rx_context.rx_data_handler = param1;
            break;
        }

        case DC5V_UART_SET_RX_BUF_SIZE:
        {
            dc5v_uart_set_rx_buf_size(param2);
            break;
        }
        
        case DC5V_UART_READ:          
        {
            ret_val = dc5v_uart_rx_timed_read(param1, param2, param3);
            break;
        }

        case DC5V_UART_WRITE:         
        {
            ret_val = dc5v_uart_api_write(NULL, param1, param2, param3);
            break;
        }

        case DC5V_UART_IOCTL:
        {
            ret_val = dc5v_uart_api_ioctl
            (
                NULL, (u32_t)param1, (void*)param2, (void*)param3
            );
            break;
        }

        case DC5V_UART_CHECK_IO_TIME:
        {
            ret_val = dc5v_uart_check_io_time(param2);
            break;
        }

        case DC5V_UART_SET_ENABLE:
        {
            dc5v_uart_set_enable_ex(param2, param3);
            break;
        }

		case DC5V_UART_RUN_RXDEAL:
		{
		    dc5v_uart_start_rxdeal();
			break;
		}

		case DC5V_UART_STOP_RXDEAL:
		{
		    dc5v_uart_stop_rxdeal();
			break;
		}		
    }
    
    return ret_val;
}

