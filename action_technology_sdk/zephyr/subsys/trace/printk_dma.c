/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <kernel.h>
#include <device.h>
#include <init.h>
#include <string.h>
#include <drivers/uart.h>
#include <drivers/uart_dma.h>
#include "cbuf.h"
#include <debug/actions_exception.h>
#include <soc.h>

#define TRUE    1
#define FALSE   0


typedef struct
{
	struct device *uart_dev;
	cbuf_t  cbuf;
	//uint8_t  sending;
	uint8_t  init;
	uint8_t  panic;
	#ifdef CONFIG_PRINTK_DMA_FULL_LOST
	uint32_t drop_bytes;
	#endif
	cbuf_dma_t dma_setting;
}printk_ctx_t;

static __act_s2_sleep_data printk_ctx_t g_pr_ctx;
#ifdef CONFIG_SOC_NO_PSRAM
__in_section_unique(trace.printk_buffer.noinit)
#endif
static unsigned char dma_printk_buffer[CONFIG_DMA_PRINTK_BUF_SIZE];
K_SEM_DEFINE(log_uart_dma_sem, 0, 1);


static void dma_stop_tx(printk_ctx_t *ctx)
{
	unsigned int lock;
	lock = irq_lock();
	if (ctx->dma_setting.read_len != 0)
	{
		if(cbuf_is_ptr_valid(&ctx->cbuf, (uint32_t)ctx->dma_setting.start_addr)){
			cbuf_dma_update_read_ptr(&ctx->cbuf, ctx->dma_setting.read_len);
		}
		ctx->dma_setting.read_len = 0;
	}
	//ctx->sending = FALSE;
	uart_acts_dma_send_stop(ctx->uart_dev);
	irq_unlock(lock);
}

static int _dma_start_tx(printk_ctx_t *ctx)
{
	int32_t data_size;
	//if(!ctx->sending){
	if (ctx->dma_setting.read_len == 0){
		data_size = cbuf_get_used_space(&ctx->cbuf);
		if(data_size > 0){
			if(data_size > (CONFIG_DMA_PRINTK_BUF_SIZE/4))  // Prevents printk  threads from blocking for too long
				data_size = (CONFIG_DMA_PRINTK_BUF_SIZE/4);

			cbuf_dma_read(&ctx->cbuf, &ctx->dma_setting, (uint32_t)data_size);
			if(ctx->dma_setting.read_len){
				///ctx->sending = TRUE;
				uart_dma_send(ctx->uart_dev, (char *)ctx->dma_setting.start_addr, ctx->dma_setting.read_len);
				return 1;
			}
		}
		return 0;
	}
	return 1;
}

static int dma_start_tx(printk_ctx_t *ctx)
{
	int ret;
	unsigned int lock;
	lock = irq_lock();
	ret = _dma_start_tx(ctx);
	irq_unlock(lock);
	return ret;
}


#ifdef CONFIG_PRINTK_DMA_FULL_LOST
static void check_drops_output(printk_ctx_t *p_ctx)
{
	uint32_t irq_flag;
	int free_space;
	char tmp_buf[8];
	if(p_ctx->drop_bytes == 0)
		return;

	irq_flag = irq_lock();
	free_space = cbuf_get_free_space(&p_ctx->cbuf);
	if(free_space > 8){
		tmp_buf[0] = '\n';
		tmp_buf[1] = '@';
		p_ctx->drop_bytes = p_ctx->drop_bytes % 10000;
		tmp_buf[2] = '0' + p_ctx->drop_bytes/1000;
		p_ctx->drop_bytes = p_ctx->drop_bytes % 1000;
		tmp_buf[3] = '0' + p_ctx->drop_bytes/100;
		p_ctx->drop_bytes = p_ctx->drop_bytes % 100;
		tmp_buf[4] = '0' + p_ctx->drop_bytes/10;
		p_ctx->drop_bytes = p_ctx->drop_bytes % 10;
		tmp_buf[5] = '0' + p_ctx->drop_bytes;
		tmp_buf[6] = '@';
		tmp_buf[7] = '\n';
		cbuf_write(&p_ctx->cbuf, (void *)tmp_buf, 8);
	}
	p_ctx->drop_bytes = 0;
	irq_unlock(irq_flag);
}
#endif

static void dma_send_finshed(printk_ctx_t *ctx)
{
	dma_stop_tx(ctx);
	dma_start_tx(ctx);
	#ifdef CONFIG_PRINTK_DMA_FULL_LOST
	check_drops_output(ctx);
	#endif
	k_sem_give(&log_uart_dma_sem);
}

static void dma_send_sync(printk_ctx_t *ctx)
{
	unsigned int lock;
	// lock irq avoid print nested call in isr context, maybe cause lock irq too long.
	lock = irq_lock();
	// output all cache data
	while(1){
	// wait transmit finished
		if(uart_acts_dma_send_complete(ctx->uart_dev)){
		   dma_stop_tx(ctx);
			#ifdef CONFIG_PRINTK_DMA_FULL_LOST
			check_drops_output(ctx);
			#endif
		}else{
		   continue;
		}

		if(!dma_start_tx(ctx)){
			break;
		}
	}

	irq_unlock(lock);

}

static void uart_dma_callback(const struct device *dev, void *arg, uint32_t ch,  int error)
{
	printk_ctx_t *ctx = (printk_ctx_t *)arg;
	dma_send_finshed(ctx);

}

static int uart_dma_switch(printk_ctx_t *ctx, unsigned int use_dma, dma_callback_t dma_handler)
{
	if(use_dma){
		if(uart_dma_send_init(ctx->uart_dev, uart_dma_callback, ctx))
			return -1;
		uart_fifo_switch(ctx->uart_dev, 1, UART_FIFO_TYPE_DMA);
	}else{
		uart_dma_send_stop(ctx->uart_dev);
		uart_fifo_switch(ctx->uart_dev, 1, UART_FIFO_TYPE_CPU);
	}
	return 0;
}

static int cbuf_output(const unsigned char *buf, unsigned int len, printk_ctx_t *p_ctx)
{
	uint32_t irq_flag;
	int free_space;

	if(len == 0)
		return 0;

	while(1){
		irq_flag = irq_lock();
		free_space = cbuf_get_free_space(&p_ctx->cbuf);
		if(free_space > len ){
			break;
		}else{
		#ifdef CONFIG_PRINTK_DMA_FULL_LOST
			p_ctx->drop_bytes += len;
			irq_unlock(irq_flag);
			if(p_ctx->drop_bytes > 0x10000 ) {
				if(uart_acts_dma_send_complete(p_ctx->uart_dev) && p_ctx->dma_setting.read_len){ // lost irq
					dma_send_finshed(p_ctx);
					continue;
				}
			}
			return 0;
		#else
			irq_unlock(irq_flag);
			if(k_is_in_isr()) {
				while(!uart_acts_dma_send_complete(p_ctx->uart_dev));
				dma_send_finshed(p_ctx);
				continue;
	        }

			if(k_sem_take(&log_uart_dma_sem, K_MSEC(500))){
				#if 1
				if(uart_acts_dma_send_complete(p_ctx->uart_dev) && p_ctx->dma_setting.read_len){ // lost irq
					dma_send_finshed(p_ctx);
				}
				#endif
			}
		#endif
		}
	}
	cbuf_write(&p_ctx->cbuf, (void *)buf, len);
	irq_unlock(irq_flag);
    return len;
}

/*must > 32*/
#define CTX_TMP_BUF_LEN 64
struct buf_out_ctx {
	uint8_t count;
	char buf[CTX_TMP_BUF_LEN];
};
static int buf_char_out(int c, void *ctx_p)
{
	struct buf_out_ctx *ctx = ctx_p;
	if(ctx->count >= (CTX_TMP_BUF_LEN-2)){
		cbuf_output(ctx->buf, ctx->count, &g_pr_ctx);
		ctx->count = 0;
	}
	if ('\n' == c) {
		ctx->buf[ctx->count++] =  '\r';
	}
	ctx->buf[ctx->count++] = c;
	return 0;
}

static void ctx_buf_flush(struct buf_out_ctx *ctx)
{
	if(ctx->count){
		cbuf_output(ctx->buf, ctx->count, &g_pr_ctx);
		ctx->count = 0;
	}
}

#ifdef CONFIG_PRINTK_TIME_FREFIX


static const uint8_t digits[] = "0123456789abcdef";
static int hex_to_str_num(char *num_str, int buflen, uint32_t num_val, int mv)
{
	uint8_t ch;
	int num_len, len, i;
	buflen--;
	num_len = buflen;
	while(num_val != 0){
		if(num_len < 0)
			break;
		ch = digits[num_val % 10];
		num_str[num_len--] = ch;
		num_val /= 10;

	}
	len = (buflen-num_len);
	num_len++;
	if(mv) {
		for(i = 0; i < len; i++){
			num_str[i] = num_str[num_len++];
		}
	}else{
		for(i = 0; i < num_len; i++){
			num_str[i] = '0';
		}
		len = buflen+1;
	}

	return len;
}

//#define PRINT_US

#ifdef PRINT_US
static struct k_timer ktimer_printk;
static uint32_t  g_low_cycle, g_high_cycle;

static void timer_printk_update(uint32_t *low, uint32_t *high)
{
	uint32_t cyc;
	unsigned int lock;
	lock = irq_lock();
	cyc = k_cycle_get_32();
	if(cyc <  g_low_cycle)
		g_high_cycle++;
	g_low_cycle = cyc;
	if(low != NULL)
		*low = g_low_cycle;
	if(high != NULL)
		*high = g_high_cycle;
	irq_unlock(lock);
}


static void timer_printk_cyc(struct k_timer *timer)
{
	timer_printk_update(NULL, NULL);
}

static void timer_printk_init(void)
{
	g_low_cycle = k_cycle_get_32();
	g_high_cycle = 0;
	k_timer_init(&ktimer_printk, timer_printk_cyc, NULL);
	k_timer_start(&ktimer_printk, K_MSEC(1000), K_MSEC(1000));
}


static void get_time_s_us(uint32_t *s, uint32_t *us)
{
	uint32_t hcyc, lcyc;
	uint64_t cyc, sec;
	timer_printk_update(&lcyc, &hcyc);
	cyc = hcyc;
	cyc = (cyc << 32) + lcyc;
	cyc =cyc/(sys_clock_hw_cycles_per_sec()/1000000); // us
	sec = cyc/1000000; //second
	*us = cyc - sec*1000000; //us
	*s = sec;
}
static int get_time_prefix(char *num_str)
{
	uint32_t sec, us;
	char *pc = num_str;
	get_time_s_us(&sec, &us);

	*pc++='[';
	if(sec) {
		pc += hex_to_str_num(pc, 9, sec, 1);
		*pc++=':';
	}else{
		*pc++='0';
		*pc++=':';
	}
	sec = us/1000;
	pc += hex_to_str_num(pc, 3, sec , 0);
	*pc++='.';
	pc += hex_to_str_num(pc, 3, us-sec*1000 , 0);
	*pc++=']';
	return (pc-num_str);
}

#else
static int get_time_prefix(char *num_str)
{
	int64_t ms_cnt;
	uint32_t sec, ms;
	char *pc = num_str;
	ms_cnt = k_uptime_get();
	sec = ms_cnt/1000;
	ms = ms_cnt - sec*1000;
	*pc++='[';
	if(sec) {
		pc += hex_to_str_num(pc, 9, sec, 1);
		*pc++=':';
	}else{
		*pc++='0';
		*pc++=':';
	}
	pc += hex_to_str_num(pc, 3, ms , 0);
	*pc++=']';
	return (pc-num_str);

}

#endif


#endif

//typedef int (*out_func_t)(int c, void *ctx);
//extern void z_vprintk(out_func_t out, void *ctx, const char *fmt, va_list ap);
#include <sys/cbprintf.h>
extern void __vprintk(const char *fmt, va_list ap);
//const char panic_inf[] = "----printk switch to cpu print panic-----\r\n";
#ifdef CONFIG_PRINTK
void vprintk(const char *fmt, va_list args)
{
	printk_ctx_t *pctx = &g_pr_ctx;
	struct buf_out_ctx ctx_buf;

	if (soc_in_sleep_mode()) {
		sl_vprintk(fmt, args);
		return ;
	}

	if(!pctx->init){
		__vprintk(fmt, args);
		return ;
	}
	if(pctx->panic){
		//cbuf_output(panic_inf, sizeof(panic_inf), pctx);
		dma_send_sync(pctx);
		k_busy_wait(100000); //wait fifo send finshed
		uart_dma_switch(pctx, 0, NULL); // CPU
		pctx->init = FALSE;
		__vprintk(fmt, args);
		return;
	}

	#ifdef CONFIG_PRINTK_TIME_FREFIX
	ctx_buf.count = get_time_prefix(ctx_buf.buf);
	#else
	ctx_buf.count = 0;
	#endif
	cbvprintf(buf_char_out, &ctx_buf, fmt, args);
 	ctx_buf_flush(&ctx_buf);
	dma_start_tx(pctx);
}
#endif

int uart_dma_send_buf(const uint8_t *buf, int len)
{
	printk_ctx_t *pctx = &g_pr_ctx;
    int ret;
	if(!pctx->init){
		k_str_out((char *)buf, len);
		return len;
	}
	ret = cbuf_output(buf ,len, pctx);
	dma_start_tx(pctx);
    return ret;
}


void cpu_trace_enable(int enable);
void trace_set_panic(void)
{
	printk_ctx_t *pctx = &g_pr_ctx;
	//printk("$$---trace_set_panic-----##\n");
	pctx->panic = 1;
	//printk("---trace_set_panic end-----\n");
#ifdef CONFIG_SOC_LEOPARD
	cpu_trace_enable(0);
#endif

	if (!soc_in_sleep_mode()) {
		exception_init();
	}
}
int check_panic_exe(void)
{
	return g_pr_ctx.panic;
}


#if defined(CONFIG_STDOUT_CONSOLE)
extern void __stdout_hook_install(int (*hook)(int));
#else
#define __stdout_hook_install(x) \
	do {    /* nothing */	 \
	} while ((0))
#endif

static struct buf_out_ctx __act_s2_notsave g_std_buf;
K_MUTEX_DEFINE(std_buf_dma_mutex);

static int dma_std_out(int c)
{
	struct buf_out_ctx *ctx = &g_std_buf;
	if(!g_pr_ctx.init)
		return 0;

	k_mutex_lock(&std_buf_dma_mutex, K_FOREVER);
	if(ctx->count >= (CTX_TMP_BUF_LEN-2)){
		cbuf_output(ctx->buf, ctx->count, &g_pr_ctx);
		ctx->count = 0;
	}
	if ('\n' == c) {
		ctx->buf[ctx->count++] =  '\r';
		ctx->buf[ctx->count++] = c;
		cbuf_output(ctx->buf, ctx->count, &g_pr_ctx);
		dma_start_tx(&g_pr_ctx);
		ctx->count = 0;
	}else{
		ctx->buf[ctx->count++] = c;
	}
	k_mutex_unlock(&std_buf_dma_mutex);
	return c;
}

void printk_dma_switch(int sw_dma)
{
	printk_ctx_t *pctx = &g_pr_ctx;
	if((pctx->uart_dev == NULL) || soc_in_sleep_mode())
		return;

	if(sw_dma){
		g_std_buf.count = 0;
		sl_dbg("printk use dma\n");
		uart_dma_switch(pctx, 1, uart_dma_callback);
		pctx->init = TRUE;
	}else{
		sl_dbg("printk use cpu\n");
		dma_send_sync(pctx);
		#ifdef CONFIG_SLEEP_DBG
		k_busy_wait(1000); //wait fifo send finshed
		#endif
		uart_dma_switch(pctx, 0, NULL); // CPU
		pctx->init = FALSE;
	}
}


#if defined(CONFIG_PM)
#include <pm/pm.h>
/*call before enter sleep*/
static void printk_pm_notifier_entry(enum pm_state state)
{
	printk_dma_switch(0);
	#if 0
	printk_ctx_t *pctx = &g_pr_ctx;
	printk("printk use cpu\n");
	dma_send_sync(pctx);
	k_busy_wait(1000); //wait fifo send finshed
	uart_dma_switch(pctx, 0, NULL); // CPU
	pctx->init = FALSE;
	#endif
}

/*call after exit sleep*/
static void printk_pm_notifier_exit(enum pm_state state)
{
	printk_dma_switch(1);
	#if 0
	printk_ctx_t *pctx = &g_pr_ctx;
	g_std_buf.count = 0;
	printk("printk use dma\n");
	uart_dma_switch(pctx, 1, uart_dma_callback);
	pctx->init = TRUE;
	#endif
}

static struct pm_notifier printk_notifier = {
	.state_entry = printk_pm_notifier_entry,
	.state_exit = printk_pm_notifier_exit,
};

#endif


static int printk_dma_init(const struct device *dev)
{
	printk_ctx_t *pctx = &g_pr_ctx;
	printk("printk_dma_init\n");

	g_std_buf.count = 0;
	pctx->uart_dev = (struct device *)device_get_binding(CONFIG_UART_CONSOLE_ON_DEV_NAME);
	if(pctx->uart_dev == NULL){
		printk("printk_dma_init fail\n");
		return -1;
	}
	cbuf_init(&pctx->cbuf, (void *)dma_printk_buffer, CONFIG_DMA_PRINTK_BUF_SIZE);
	if(uart_dma_switch(pctx, 1, uart_dma_callback)) {
		printk("printk_dma_init,uart dma not config\n");
		return -1;
	}
	__stdout_hook_install(dma_std_out);
    pctx->init = TRUE;
#if defined(CONFIG_PRINTK_TIME_FREFIX)
	#ifdef PRINT_US
	timer_printk_init();
	#endif
#endif
#if defined(CONFIG_PM)
	pm_notifier_register(&printk_notifier);
#endif
	return 0;
}



SYS_INIT(printk_dma_init, APPLICATION, 1);


