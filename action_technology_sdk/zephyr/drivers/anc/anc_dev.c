/*
 * Copyright (c) 1997-2015, Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <device.h>
#include <soc.h>
#include <soc_anc.h>
#include <drivers/anc.h>
#include "anc_inner.h"
#include <soc_regs.h>

#define CONFIG_ANC_DEBUG_PRINT
#define CONFIG_ANC_COMMAND_ENABLE
#define CONFIG_ANC_ACK_ENABLE

#define ANC_BOOTARGS_CPU_ADDRESS 0x106A600
#define ANC_BOOTARGS_SIZE    0x20


struct anc_bootargs *anc_bootargs = (struct anc_bootargs *)ANC_BOOTARGS_CPU_ADDRESS;

#ifdef CONFIG_ANC_COMMAND_ENABLE
/*command ring buffer*/
static struct acts_ringbuf command_buff  __in_section_unique(ANC_SHARE_RAM);
/*364 bytes data, 8 bytes head*/
static u8_t command_data_buff[0x200] __in_section_unique(ANC_SHARE_RAM);
#ifdef CONFIG_ANC_ACK_ENABLE
/*ack ringbuf*/
static struct acts_ringbuf ack_buff  __in_section_unique(ANC_SHARE_RAM);
static u8_t ack_data_buff[0x20] __in_section_unique(ANC_SHARE_RAM);
#endif
#endif // CONFIG_ANC_COMMAND_ENABLE

#ifdef CONFIG_ANC_DEBUG_PRINT
#include <acts_ringbuf.h>
struct acts_ringbuf *anc_dev_get_print_buffer(void);
static void anc_print_work_func(struct k_work *work);
static struct acts_ringbuf debug_buff  __in_section_unique(ANC_SHARE_RAM);
static u8_t debug_data_buff[0x100] __in_section_unique(ANC_SHARE_RAM);
K_DELAYED_WORK_DEFINE(anc_print_work, anc_print_work_func);
char anc_hex_buf[0x200];

int anc_print_all_buff(void)
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

	if(length >= sizeof(anc_hex_buf)){
		length = sizeof(anc_hex_buf) - 2;
	}

	if(length){
		int index = 0;
		acts_ringbuf_get(debug_buf, &anc_hex_buf[0], length);
		for(i = 0; i < length / 2; i++){
			anc_hex_buf[i] = anc_hex_buf[2 * i];
		}
		anc_hex_buf[i] = 0;

		printk("[ANC PRINT]%s %d %d %d \n",anc_hex_buf,debug_buf->head,debug_buf->tail,debug_buf->mask);

		return acts_ringbuf_length(debug_buf);
	}
	return 0;
}
static void anc_print_work_func(struct k_work *work)
{
	//printk("-----%s-----\n",__func__);
	while(anc_print_all_buff() > 0) {
		;
	}
	k_delayed_work_submit(&anc_print_work, K_MSEC(2));
}
#endif

static void anc_acts_isr(void *arg);
static void anc_acts_irq_enable(void);
static void anc_acts_irq_disable(void);

DEVICE_DECLARE(anc_acts);

int anc_wait_hw_idle_timeout(int usec_to_wait)
{
	do {
		if (anc_check_hw_idle())
			return 0;
		if (usec_to_wait-- <= 0)
			break;
		k_busy_wait(1);
	} while (1);

	return -EAGAIN;
}


/*FIXME*/
static int anc_acts_request_mem(struct device *dev, int type)
{
	return anc_soc_request_mem();
}


/*FIXME*/
static int anc_acts_release_mem(struct device *dev, int type)
{
	return anc_soc_release_mem();
}

static int anc_acts_power_on(struct device *dev)
{
	struct anc_acts_data *anc_data = dev->data;
	int i;

	if (anc_data->pm_status != ANC_STATUS_POWEROFF)
		return 0;

	if (anc_data->images.size == 0) {
		SYS_LOG_ERR("%s: no image loaded\n", __func__);
		return -EFAULT;
	}

	anc_init_clk();

	/* assert anc wait signal */
	anc_do_wait();

	anc_soc_request_mem();

	anc_data->pm_status = ANC_STATUS_POWERON;

	/* clear all pending */
	clear_anc_all_irq_pending();

	/* enable anc irq */
	anc_acts_irq_enable();

	/* deassert anc wait signal */
	anc_undo_wait();

	anc_reset_enable();

	/*anc reg access by anc*/
	anc_soc_request_reg();

	SYS_LOG_ERR("ANC power on\n");

	anc_dump_info();

	return 0;
}

static int anc_acts_power_off(struct device *dev)
{
	const struct anc_acts_config *anc_cfg = dev->config;
	struct anc_acts_data *anc_data = dev->data;

	if (anc_data->pm_status == ANC_STATUS_POWEROFF)
		return 0;

	/* assert anc wait signal */
	anc_do_wait();

	/* disable anc irq */
	anc_acts_irq_disable();

	/* assert reset anc module */
	acts_reset_peripheral_assert(RESET_ID_ANC);

	/* disable anc clock*/
	acts_clock_peripheral_disable(CLOCK_ID_ANC);

	/* deassert anc wait signal */
	anc_undo_wait();

	anc_soc_release_mem();

	anc_soc_release_reg();

	anc_data->pm_status = ANC_STATUS_POWEROFF;
#ifdef CONFIG_ANC_DEBUG_PRINT
	anc_print_all_buff();
	acts_ringbuf_drop_all(&debug_buff);
#endif
	SYS_LOG_INF("ANC power off\n");
	return 0;
}


static int anc_acts_get_status(struct device *dev)
{
	struct anc_acts_data *anc_data = dev->data;

	return anc_data->pm_status;
}

static int anc_acts_send_command(struct device *dev, int type, void *data, int size)
{
	int ret, cnt;
	ack_buf_t ack;
	struct anc_acts_command cmd;
	const struct anc_acts_config *anc_cfg = dev->config;
	struct anc_acts_data *anc_data = dev->data;

	if (anc_data->pm_status == ANC_STATUS_POWEROFF)
	{
		SYS_LOG_ERR("anc dsp haven't been power on\n");
		return -1;
	}

	if(k_mutex_lock(&anc_data->mutex, SYS_TIMEOUT_MS(2000))){
		SYS_LOG_ERR("wait timeout");
		return -1;
	}

	cnt = 0;
	ret = acts_ringbuf_space(&command_buff);
	while(ret < size + sizeof(struct anc_acts_command)){
		cnt++;
		os_sleep(5);
		ret = acts_ringbuf_space(&command_buff);
		if(cnt >= 400){
			SYS_LOG_ERR("wait timeout,send data to anc dsp failed, 0x%x\n",ret);
			goto errexit;
		}
	}

	/*put cmd head*/
	cmd.id = type;
	cmd.data_size = size;
	ret = acts_ringbuf_put(&command_buff, &cmd, sizeof(struct anc_acts_command));
	if(ret != sizeof(struct anc_acts_command)){
		SYS_LOG_ERR("send err");
		goto errexit;
	}

	/*put data*/
	if(size){
		ret = acts_ringbuf_put(&command_buff, data, size);
		if(ret != size){
			SYS_LOG_ERR("send err");
			goto errexit;
		}
	}


	/*wait ack*/
#ifdef CONFIG_ANC_ACK_ENABLE
	cnt = 0;
	ret = acts_ringbuf_length(&ack_buff);
	while(ret < sizeof(ack_buf_t)){
		cnt++;
		if(cnt > 2000){
			SYS_LOG_ERR("wait ack timeout");
			goto errexit;
		}
		os_sleep(2);
		ret = acts_ringbuf_length(&ack_buff);
	}

	acts_ringbuf_get(&ack_buff, &ack, sizeof(ack_buf_t));
	if(ack.cmd != type){
		SYS_LOG_ERR("command send err");
		goto errexit;
	}
#endif

	k_mutex_unlock(&anc_data->mutex);
	return 0;
errexit:
	k_mutex_unlock(&anc_data->mutex);
	return -1;
}

static void anc_acts_isr(void *arg)
{
	/* clear irq pending bits */
	//clear_anc_irq_pending(DT_INST_IRQN(0));
	clear_anc_irq_pending(0);

	anc_acts_recv_message(arg);
}

static int anc_acts_init(const struct device *dev)
{
	const struct anc_acts_config *anc_cfg = dev->config;
	struct anc_acts_data *anc_data = dev->data;

	memset(anc_bootargs, 0, sizeof(struct anc_bootargs));

#ifdef CONFIG_ANC_COMMAND_ENABLE
	acts_ringbuf_init(&command_buff, command_data_buff, sizeof(command_data_buff));
	command_buff.dsp_ptr = mcu_to_anc_address(command_buff.cpu_ptr, 0);
	anc_bootargs->cmd_buf = mcu_to_anc_address(POINTER_TO_UINT(&command_buff), DATA_ADDR);
#ifdef CONFIG_ANC_ACK_ENABLE
	acts_ringbuf_init(&ack_buff, ack_data_buff, sizeof(ack_data_buff));
	ack_buff.dsp_ptr = mcu_to_anc_address(ack_buff.cpu_ptr, 0);
	anc_bootargs->ack_buf = mcu_to_anc_address(POINTER_TO_UINT(&ack_buff), DATA_ADDR);
#endif
#endif

#ifdef CONFIG_ANC_DEBUG_PRINT
	acts_ringbuf_init(&debug_buff, debug_data_buff, sizeof(debug_data_buff));
	debug_buff.dsp_ptr = mcu_to_anc_address(debug_buff.cpu_ptr, 0);
	anc_bootargs->debug_buf = mcu_to_anc_address(POINTER_TO_UINT(&debug_buff), DATA_ADDR);
	k_delayed_work_submit(&anc_print_work, K_MSEC(10));
#endif
	k_sem_init(&anc_data->sem, 0, 1);
	k_mutex_init(&anc_data->mutex);

	anc_data->pm_status = ANC_STATUS_POWEROFF;
	return 0;
}

static struct anc_acts_data anc_acts_data;

static const struct anc_acts_config anc_acts_config = {

};

const struct anc_driver_api anc_drv_api = {
	.poweron = anc_acts_power_on,
	.poweroff = anc_acts_power_off,
	.request_image = anc_acts_request_image,
	.release_image = anc_acts_release_image,
	.request_mem = anc_acts_request_mem,
	.release_mem = anc_acts_release_mem,
	.get_status = anc_acts_get_status,
	.send_command = anc_acts_send_command,
};

DEVICE_DEFINE(anc_acts, CONFIG_ANC_NAME,
		anc_acts_init, NULL, &anc_acts_data, &anc_acts_config,
		POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &anc_drv_api);

/*FIXME*/
static void anc_acts_irq_enable(void)
{
	// IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority), anc_acts_isr, DEVICE_GET(anc_acts), 0);

	// irq_enable(DT_INST_IRQN(0));
}

/*FIXME*/
static void anc_acts_irq_disable(void)
{
	// irq_disable(DT_INST_IRQN(0));
}

#ifdef CONFIG_ANC_DEBUG_PRINT
#define DEV_DATA(dev) \
	((struct anc_acts_data * const)(dev)->data)

struct acts_ringbuf *anc_dev_get_print_buffer(void)
{
	struct anc_acts_data *data = DEV_DATA(DEVICE_GET(anc_acts));

	if(!data->bootargs.debug_buf){
		return NULL;
	}

	return (struct acts_ringbuf *)anc_to_mcu_address(data->bootargs.debug_buf,DATA_ADDR);
}
#endif
