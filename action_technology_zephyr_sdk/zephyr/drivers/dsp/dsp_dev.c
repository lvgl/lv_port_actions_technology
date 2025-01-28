/*
 * Copyright (c) 1997-2015, Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <device.h>
#include <soc.h>
#include <soc_dsp.h>
#include <drivers/dsp.h>
#include "dsp_inner.h"
#include <soc_regs.h>
#include <board_cfg.h>
#include <dvfs.h>

static struct dsp_acts_data dsp_data __in_section_unique(DSP_SHARE_RAM);

#include <acts_ringbuf.h>
struct acts_ringbuf *dsp_dev_get_print_buffer(void);
static void dsp_print_work(struct k_work *work);
static struct acts_ringbuf debug_buff  __in_section_unique(DSP_SHARE_RAM);
static u8_t debug_data_buff[0x200] __in_section_unique(DSP_SHARE_RAM);
K_DELAYED_WORK_DEFINE(print_work, dsp_print_work);
#ifdef CONFIG_SOC_NO_PSRAM
__in_section_unique(media.noinit.heap)
#endif
char hex_buf[0x200];

int dsp_print_all_buff(void)
{
	int i = 0;
	int length;
	struct acts_ringbuf *debug_buf = &debug_buff;
	if(!debug_buf)
		return 0;

	length = acts_ringbuf_length(debug_buf);

	if(length > acts_ringbuf_size(debug_buf)) {
		acts_ringbuf_drop_all(&debug_buff);
		length = 0;
	}

	if(length >= sizeof(hex_buf)){
		length = sizeof(hex_buf) - 2;
	}

	if(length){
		//int index = 0;
		acts_ringbuf_get(debug_buf, &hex_buf[0], length);
		for(i = 0; i < length / 2; i++){
			hex_buf[i] = hex_buf[2 * i];
		}
		hex_buf[i] = 0;
#ifdef CONFIG_DSP_DEBUG_PRINT
		printk("%s\n",hex_buf);
#endif
		return acts_ringbuf_length(debug_buf);
	}
	return 0;
}
static void dsp_print_work(struct k_work *work)
{
	while(dsp_print_all_buff() > 0) {
		;
	}

	if (dsp_data.pm_status != DSP_STATUS_POWEROFF){
		k_delayed_work_submit(&print_work, K_MSEC(2));
	}
}


static void dsp_acts_isr(void *arg);
static void dsp_acts_irq_enable(void);
static void dsp_acts_irq_disable(void);
static void dsp_acts_irq1_disable(void);
void dsp_acts_set_irq1_callback( dsp_acts_irq_callback cb, void *cb_data);


DEVICE_DECLARE(dsp_acts);

int wait_hw_idle_timeout(int usec_to_wait)
{
	do {
		if (dsp_check_hw_idle())
			return 0;
		if (usec_to_wait-- <= 0)
			break;
		k_busy_wait(1);
	} while (1);

	return -EAGAIN;
}

static int dsp_acts_suspend(struct device *dev)
{
	struct dsp_acts_data *dsp_data = dev->data;

	if (dsp_data->pm_status != DSP_STATUS_POWERON)
		return 0;

	/* already idle ? */
	if (!get_hw_idle()) {
		struct dsp_message message = { .id = DSP_MSG_SUSPEND, };

		k_sem_reset(&dsp_data->msg_sem);

		if (dsp_acts_send_message(dev, &message))
			return -EBUSY;

		/* wait message DSP_MSG_BOOTUP */
		if (k_sem_take(&dsp_data->msg_sem, SYS_TIMEOUT_MS(20))) {
			SYS_LOG_ERR("dsp image <%s> busy\n", dsp_data->images[DSP_IMAGE_MAIN].name);
			return -EBUSY;
		}

		/* make sure hardware is really idle */
		if (wait_hw_idle_timeout(10))
			SYS_LOG_ERR("DSP wait idle signal timeout\n");
	}

	dsp_do_wait();
	acts_clock_peripheral_disable(CLOCK_ID_DSP);
	acts_clock_peripheral_disable(CLOCK_ID_AUDDSPTIMER);

	dsp_data->pm_status = DSP_STATUS_SUSPENDED;
	SYS_LOG_ERR("DSP suspended\n");
	return 0;
}

static int dsp_acts_resume(struct device *dev)
{
	struct dsp_acts_data *dsp_data = dev->data;
	struct dsp_message message = { .id = DSP_MSG_RESUME, };

	if (dsp_data->pm_status != DSP_STATUS_SUSPENDED)
		return 0;

	acts_clock_peripheral_enable(CLOCK_ID_DSP);
	acts_clock_peripheral_enable(CLOCK_ID_AUDDSPTIMER);

	dsp_undo_wait();

	if (dsp_acts_send_message(dev, &message)) {
		SYS_LOG_ERR("DSP resume failed\n");
		return -EFAULT;
	}

	dsp_data->pm_status = DSP_STATUS_POWERON;
	SYS_LOG_ERR("DSP resumed\n");
	return 0;
}

static int dsp_acts_kick(struct device *dev, uint32_t owner, uint32_t event, uint32_t params)
{
	struct dsp_acts_data *dsp_data = dev->data;
	const struct dsp_acts_config *dsp_cfg = dev->config;
	struct dsp_protocol_userinfo *dsp_userinfo = dsp_cfg->dsp_userinfo;
	struct dsp_message message = {
		.id = DSP_MSG_KICK,
		.owner = owner,
		.param1 = event,
		.param2 = params,
	};

	if (dsp_data->pm_status != DSP_STATUS_POWERON)
		return -EACCES;

	if (dsp_userinfo->task_state != DSP_TASK_SUSPENDED)
		return -EINPROGRESS;

	return dsp_acts_send_message(dev, &message);
}

static int dsp_acts_request_mem(struct device *dev, int type)
{
	return dsp_soc_request_mem(type);
}

static int dsp_acts_release_mem(struct device *dev, int type)
{
	return dsp_soc_release_mem(type);
}

/* synchronize cpu and dsp clocks, both base hw timer */
static int dsp_acts_synchronize_clock(void)
{
	uint32_t flags;
	uint16_t sync_clock_status, sync_clock_status_last;
	uint32_t sync_clock_cycles_cpu, sync_clock_cycles_cpu_end;
	uint32_t sync_clock_cycles_cpu_diff, sync_clock_cycles_cpu_diff_last = 0xffff0000;
	uint32_t sync_clock_cycles_dsp;
	uint32_t sync_clock_cycles_offset;
	uint32_t try_count = 0, match_count = 0;
	uint32_t jitter_cycles_threshold = 16;
	
	printk("synchronize cpu and dsp clocks start\n");
	
	flags = irq_lock();

	sync_clock_cycles_cpu = k_cycle_get_32();
	*(volatile uint32_t *)(DSP_SYNC_CLOCK_CYCLES_BASE) = sync_clock_cycles_cpu;
	*(volatile uint16_t *)(DSP_SYNC_CLOCK_STATUS_BASE) = DSP_SYNC_CLOCK_START;
	sync_clock_status_last = DSP_SYNC_CLOCK_START;

	while (1) {
		sync_clock_status = *(volatile uint16_t *)(DSP_SYNC_CLOCK_STATUS_BASE);
		if (sync_clock_status_last == DSP_SYNC_CLOCK_START) {
			if (sync_clock_status == DSP_SYNC_CLOCK_REPLY) {
				sync_clock_cycles_dsp = *(volatile uint32_t *)(DSP_SYNC_CLOCK_CYCLES_BASE);
				sync_clock_cycles_cpu_end = k_cycle_get_32();
				sync_clock_cycles_cpu_diff = sync_clock_cycles_cpu_end - sync_clock_cycles_cpu;
				
				if ((sync_clock_cycles_cpu_diff >= sync_clock_cycles_cpu_diff_last && 
				sync_clock_cycles_cpu_diff <= sync_clock_cycles_cpu_diff_last + jitter_cycles_threshold) ||
					(sync_clock_cycles_cpu_diff <= sync_clock_cycles_cpu_diff_last && 
				sync_clock_cycles_cpu_diff + jitter_cycles_threshold >= sync_clock_cycles_cpu_diff_last)) {
					match_count++;
				} else {
					match_count = 0;
				}
				try_count++;
				
				sync_clock_cycles_cpu_diff_last = sync_clock_cycles_cpu_diff;
				
				if (match_count >= 5) {
					sync_clock_cycles_offset = sync_clock_cycles_cpu - sync_clock_cycles_dsp;
					*(volatile uint32_t *)(DSP_SYNC_CLOCK_CYCLES_BASE) = sync_clock_cycles_offset;
					*(volatile uint16_t *)(DSP_SYNC_CLOCK_STATUS_BASE) = DSP_SYNC_CLOCK_OFFSET;
					sync_clock_status_last = DSP_SYNC_CLOCK_OFFSET;
				} else {
					sync_clock_cycles_cpu = k_cycle_get_32();
					*(volatile uint32_t *)(DSP_SYNC_CLOCK_CYCLES_BASE) = sync_clock_cycles_cpu;
					*(volatile uint16_t *)(DSP_SYNC_CLOCK_STATUS_BASE) = DSP_SYNC_CLOCK_START;
				}

				if (try_count > 20) {
					try_count = 0;
					match_count = 0;
					jitter_cycles_threshold += 16;
				}
			}
		} else {
			if (sync_clock_status == DSP_SYNC_CLOCK_NULL) {
				break;
			}
		}
	}

	irq_unlock(flags);

	printk("sync clock : 0x%x,%d,%d\n", k_cycle_get_32(), sync_clock_cycles_cpu_diff_last, jitter_cycles_threshold);

	return 0;
}
static int dsp_acts_power_on(struct device *dev, void *cmdbuf)
{
	struct dsp_acts_data *dsp_data = dev->data;
	const struct dsp_acts_config *dsp_cfg = dev->config;
	struct dsp_protocol_userinfo *dsp_userinfo = dsp_cfg->dsp_userinfo;
	int i;

	if (dsp_data->pm_status != DSP_STATUS_POWEROFF)
		return 0;

	if (dsp_data->images[DSP_IMAGE_MAIN].size == 0) {
		SYS_LOG_ERR("%s: no image loaded\n", __func__);
		return -EFAULT;
	}

	if (cmdbuf == NULL) {
		SYS_LOG_ERR("%s: must assign a command buffer\n", __func__);
		return -EINVAL;
	}

	memset((void *)DSP_MAILBOX_REG_BASE, 0, 0x80);

    dsp_init_clk();

#ifdef CONFIG_DSP_DEBUG_PRINT
	acts_ringbuf_init(&debug_buff, debug_data_buff, sizeof(debug_data_buff));
	debug_buff.dsp_ptr = mcu_to_dsp_address(debug_buff.cpu_ptr, 0);
	k_delayed_work_submit(&print_work, K_MSEC(2));
#endif

	/* enable dsp clock*/
	acts_clock_peripheral_enable(CLOCK_ID_DSP);
	acts_clock_peripheral_enable(CLOCK_ID_AUDDSPTIMER);

	/* assert reset */
	acts_reset_peripheral_assert(RESET_ID_DSP);

	/* set bootargs */
	dsp_data->bootargs.command_buffer = mcu_to_dsp_address((uint32_t)cmdbuf, DATA_ADDR);
	dsp_data->bootargs.sub_entry_point = dsp_data->images[1].entry_point;

	/* assert dsp wait signal */
	dsp_do_wait();

	dsp_acts_request_mem(dev, 0);

	/* intialize shared registers */
	dsp_cfg->dsp_mailbox->msg = MAILBOX_MSG(DSP_MSG_NULL, MSG_FLAG_ACK) ;
	dsp_cfg->dsp_mailbox->owner = 0;
	dsp_cfg->cpu_mailbox->msg = MAILBOX_MSG(DSP_MSG_NULL, MSG_FLAG_ACK) ;
	dsp_cfg->cpu_mailbox->owner = 0;

	dsp_userinfo->task_state = DSP_TASK_PRESTART;
	dsp_userinfo->error_code = DSP_NO_ERROR;

	/* set dsp vector_address */
	//set_dsp_vector_addr((unsigned int)DSP_ADDR);

	dsp_data->pm_status = DSP_STATUS_POWERON;

	/* clear all pending */
	clear_dsp_all_irq_pending();

	/* enable dsp irq */
	dsp_acts_irq_enable();

	/* deassert dsp wait signal */
	dsp_undo_wait();

	k_sem_reset(&dsp_data->msg_sem);
	/* deassert reset */
	acts_reset_peripheral_deassert(RESET_ID_DSP);

	/* wait message DSP_MSG_STATE_CHANGED.DSP_TASK_STARTED */
	if (k_sem_take(&dsp_data->msg_sem, SYS_TIMEOUT_MS(100))) {
		SYS_LOG_ERR("dsp image <%s> cannot boot up\n", dsp_data->images[DSP_IMAGE_MAIN].name);
		dsp_dump_info();
		goto cleanup;
	}

	if (dsp_data->need_sync_clock == 'C')
	{
		dsp_acts_synchronize_clock();
	}

	/* FIXME: convert userinfo address from dsp to cpu here, since
	 * dsp will never touch it again.
	 */
	dsp_userinfo->func_table = dsp_to_mcu_address(dsp_userinfo->func_table, DATA_ADDR);
	for (i = 0; i < dsp_userinfo->func_size; i++) {
		volatile uint32_t addr = dsp_userinfo->func_table + i * 4;
		if (addr > 0)   /* NULL, no information provided */
			sys_write32(dsp_to_mcu_address(sys_read32(addr), DATA_ADDR), addr);
	}

	//SYS_LOG_ERR("DSP power on %d %p\n", dsp_userinfo->func_size, dsp_userinfo->func_table);

	return 0;
cleanup:
	dsp_acts_irq_disable();
	acts_clock_peripheral_disable(CLOCK_ID_DSP);
	acts_clock_peripheral_disable(CLOCK_ID_AUDDSPTIMER);
	acts_reset_peripheral_assert(RESET_ID_DSP);

	return -ETIMEDOUT;
}

static int dsp_acts_power_off(struct device *dev)
{
	const struct dsp_acts_config *dsp_cfg = dev->config;
	struct dsp_acts_data *dsp_data = dev->data;

	if (dsp_data->pm_status == DSP_STATUS_POWEROFF)
		return 0;

	/* assert dsp wait signal */
	dsp_do_wait();

	/* disable dsp irq */
	dsp_acts_irq_disable();

	/* disable dsp irq 1 */
	dsp_acts_irq1_disable();
	/* assert reset dsp module */
	acts_reset_peripheral_assert(RESET_ID_DSP);

	/* disable dsp clock*/
	acts_clock_peripheral_disable(CLOCK_ID_DSP);
	acts_clock_peripheral_disable(CLOCK_ID_AUDDSPTIMER);

	/* deassert dsp wait signal */
	dsp_undo_wait();

	/* clear page mapping */
	clear_dsp_pageaddr();

	dsp_cfg->dsp_userinfo->task_state = DSP_TASK_DEAD;
	dsp_data->pm_status = DSP_STATUS_POWEROFF;

	dsp_print_all_buff();
	acts_ringbuf_drop_all(&debug_buff);

#ifdef CONFIG_DSP_DEBUG_PRINT
	k_delayed_work_cancel(&print_work);
#endif

	SYS_LOG_INF("DSP power off\n");
	return 0;
}

static int acts_request_userinfo(struct device *dev, int request, void *info)
{
	const struct dsp_acts_config *dsp_cfg = dev->config;
	struct dsp_protocol_userinfo *dsp_userinfo = dsp_cfg->dsp_userinfo;
	union {
		struct dsp_request_session *ssn;
		struct dsp_request_function *func;
	} req_info;

	switch (request) {
	case DSP_REQUEST_TASK_STATE:
		*(int *)info = dsp_userinfo->task_state;
		break;
	case DSP_REQUEST_ERROR_CODE:
		*(int *)info = dsp_userinfo->error_code;
		break;
	case DSP_REQUEST_SESSION_INFO:
		req_info.ssn = info;
		req_info.ssn->func_enabled = dsp_userinfo->func_enabled;
		req_info.ssn->func_runnable = dsp_userinfo->func_runnable;
		req_info.ssn->func_counter = dsp_userinfo->func_counter;
		req_info.ssn->info = (void *)dsp_to_mcu_address(dsp_userinfo->ssn_info, DATA_ADDR);
		break;
	case DSP_REQUEST_FUNCTION_INFO:
		req_info.func = info;
		if (req_info.func->id < dsp_userinfo->func_size)
			req_info.func->info = (void *)sys_read32(dsp_userinfo->func_table + req_info.func->id * 4);
		break;
	case DSP_REQUEST_USER_DEFINED:
    	{
            uint32_t index = *(uint32_t*)info;
            uint32_t debug_info = *(volatile uint32_t*)(DSP_DEBUG_REGION_REGISTER_BASE + 4 * index);
            *(uint32_t*)info = debug_info;
            break;
    	}
	default:
		return -EINVAL;
	}

	return 0;
}

static void dsp_acts_isr(void *arg)
{
	/* clear irq pending bits */
	clear_dsp_irq_pending(IRQ_ID_DSP);

	dsp_acts_recv_message(arg);
}


static void dsp_acts_isr1(void *arg)
{
	struct device *dev = (struct device *)arg;
	struct dsp_acts_data *dsp_data = dev->data;

	/* clear irq pending bits */
	clear_dsp_irq1_pending(IRQ_ID_DSP1);

	if (dsp_data->cb) {
		dsp_data->cb(dsp_data->cb_data, 1, NULL);
	}
}

void dsp_acts_set_irq1_callback(dsp_acts_irq_callback cb, void *cb_data)
{

	struct device *dev = (struct device *)DEVICE_GET(dsp_acts);

	if (!dev) {
		SYS_LOG_ERR("dev handle NULL");
		return;
	}


	struct dsp_acts_data *dsp_data = dev->data;

	if (dsp_data) {
		dsp_data->cb = cb;
		dsp_data->cb_data = cb_data;
	}

}

static int dsp_acts_init(const struct device *dev)
{
	const struct dsp_acts_config *dsp_cfg = dev->config;
	struct dsp_acts_data *dsp_data = dev->data;

	acts_ringbuf_init(&debug_buff, debug_data_buff, sizeof(debug_data_buff));
	debug_buff.dsp_ptr = mcu_to_dsp_address(debug_buff.cpu_ptr, 0);
	dsp_data->bootargs.debug_buf = mcu_to_dsp_address(POINTER_TO_UINT(&debug_buff), DATA_ADDR);

	dsp_cfg->dsp_userinfo->task_state = DSP_TASK_DEAD;
	k_sem_init(&dsp_data->msg_sem, 0, 1);
	k_mutex_init(&dsp_data->msg_mutex);

	dsp_data->pm_status = DSP_STATUS_POWEROFF;
	memset(dsp_cfg->dsp_mailbox, 0, sizeof(struct dsp_protocol_mailbox));
	memset(dsp_cfg->cpu_mailbox, 0, sizeof(struct dsp_protocol_mailbox));
	memset(dsp_cfg->dsp_userinfo, 0, sizeof(struct dsp_protocol_userinfo));
	return 0;
}

static const struct dsp_acts_config dsp_acts_config = {
	.dsp_mailbox = (struct dsp_protocol_mailbox *)DSP_M2D_MAILBOX_REGISTER_BASE,
	.cpu_mailbox = (struct dsp_protocol_mailbox *)DSP_D2M_MAILBOX_REGISTER_BASE,
	.dsp_userinfo = (struct dsp_protocol_userinfo *)DSP_USER_REGION_REGISTER_BASE,
};

const struct dsp_driver_api dsp_drv_api = {
	.poweron = dsp_acts_power_on,
	.poweroff = dsp_acts_power_off,
	.suspend = dsp_acts_suspend,
	.resume = dsp_acts_resume,
	.kick = dsp_acts_kick,
	.register_message_handler = dsp_acts_register_message_handler,
	.unregister_message_handler = dsp_acts_unregister_message_handler,
	.request_image = dsp_acts_request_image,
	.release_image = dsp_acts_release_image,
	.request_mem = dsp_acts_request_mem,
	.release_mem = dsp_acts_release_mem,
	.send_message = dsp_acts_send_message,
	.request_userinfo = acts_request_userinfo,
};

DEVICE_DEFINE(dsp_acts, CONFIG_DSP_NAME,
		dsp_acts_init, NULL, &dsp_data, &dsp_acts_config,
		POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &dsp_drv_api);

static void dsp_acts_irq_enable(void)
{
	IRQ_CONNECT(IRQ_ID_DSP, CONFIG_DSP_IRQ_PRI, dsp_acts_isr, DEVICE_GET(dsp_acts), 0);

	irq_enable(IRQ_ID_DSP);

	IRQ_CONNECT(IRQ_ID_DSP1, CONFIG_DSP_IRQ_PRI, dsp_acts_isr1, DEVICE_GET(dsp_acts), 0);

	irq_enable(IRQ_ID_DSP1);
}

static void dsp_acts_irq_disable(void)
{
	irq_disable(IRQ_ID_DSP);
}

static void dsp_acts_irq1_disable(void)
{
	irq_disable(IRQ_ID_DSP1);
}


#define DEV_DATA(dev) \
	((struct dsp_acts_data * const)(dev)->data)

struct acts_ringbuf *dsp_dev_get_print_buffer(void)
{
	struct dsp_acts_data *data = DEV_DATA(DEVICE_GET(dsp_acts));

	if(!data->bootargs.debug_buf){
		return NULL;
	}

	return (struct acts_ringbuf *)dsp_to_mcu_address(data->bootargs.debug_buf,DATA_ADDR);
}
