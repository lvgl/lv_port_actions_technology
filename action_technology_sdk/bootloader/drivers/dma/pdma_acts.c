/*
 * Copyright (c) 2018 Google LLC.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <soc.h>
#include <drivers/dma.h>

#include <board_cfg.h>
#include <logging/log.h>
LOG_MODULE_REGISTER(dma_acts, CONFIG_DMA_LOG_LEVEL);

#define DMA_INVALID_CHAN       	(0xff)


#define DMA_ID_MEM				0
#define MAX_DMA_CH  			CONFIG_DMA_0_PCHAN_NUM
#define DMA_CHAN(base, ch)		((struct acts_dma_chan_reg*)((base) +  (ch+1) * 0x100))
/* Maximum data sent in single transfer (Bytes) */
#define DMA_ACTS_MAX_DATA_ITEMS		0x7ffff

/*dma ctl register*/
#define DMA_CTL_SRC_TYPE_SHIFT			0
#define DMA_CTL_SRC_TYPE(x)			((x) << DMA_CTL_SRC_TYPE_SHIFT)
#define DMA_CTL_SRC_TYPE_MASK			DMA_CTL_SRC_TYPE(0x3f)
#define DMA_CTL_SAM_CONSTANT			(0x1 << 7)
#define DMA_CTL_DST_TYPE_SHIFT			8
#define DMA_CTL_DST_TYPE(x)			((x) << DMA_CTL_DST_TYPE_SHIFT)
#define DMA_CTL_DST_TYPE_MASK			DMA_CTL_DST_TYPE(0x3f)
#define DMA_CTL_DAM_CONSTANT			(0x1 << 15)


#define DMA_CTL_ADUDIO_TYPE_SHIFT			16
#define DMA_CTL_ADUDIO_TYPE(x)		((x) << DMA_CTL_ADUDIO_TYPE_SHIFT)
#define DMA_CTL_ADUDIO_TYPE_MASK	DMA_CTL_ADUDIO_TYPE(0x1)
#define DMA_CTL_ADUDIO_TYPE_INTER	DMA_CTL_ADUDIO_TYPE(0)
#define DMA_CTL_ADUDIO_TYPE_SEP		DMA_CTL_ADUDIO_TYPE(1)
#define DMA_CTL_TRM_SHIFT			17
#define DMA_CTL_TRM(x)				((x) << DMA_CTL_TRM_SHIFT)
#define DMA_CTL_TRM_MASK			DMA_CTL_TRM(0x1)
#define DMA_CTL_TRM_BURST8			DMA_CTL_TRM(0)
#define DMA_CTL_TRM_SINGLE			DMA_CTL_TRM(1)
#define DMA_CTL_RELOAD				(0x1 << 18)
#define DMA_CTL_TWS_SHIFT			20
#define DMA_CTL_TWS(x)				((x) << DMA_CTL_TWS_SHIFT)
#define DMA_CTL_TWS_MASK			DMA_CTL_TWS(0x3)
#define DMA_CTL_TWS_8BIT			DMA_CTL_TWS(2)
#define DMA_CTL_TWS_16BIT			DMA_CTL_TWS(1)
#define DMA_CTL_TWS_32BIT			DMA_CTL_TWS(0)

/*dma pending register*/
#define DMA_PD_TCIP(ch)				(1 << ch)
#define DMA_PD_HFIP(ch)				(1 << (ch+16))

/*dma irq enable register*/
#define DMA_IE_TCIP(ch)				(1 << ch)
#define DMA_IE_HFIP(ch)				(1 << (ch+16))


#define DMA_START_START				(0x1 << 0)



/* dma channel registers */
struct acts_dma_chan_reg {
	volatile uint32_t ctl;
	volatile uint32_t start;
	volatile uint32_t saddr0;
	volatile uint32_t saddr1;
	volatile uint32_t daddr0;
	volatile uint32_t daddr1;
	volatile uint32_t bc;
	volatile uint32_t rc;
};
struct acts_dma_reg {
	volatile uint32_t dma_ip;
	volatile uint32_t dma_ie;
};


struct dma_acts_channel {
	dma_callback_t cb;
	void   *cb_arg;
    uint16_t reload_count;
	uint16_t  complete_callback_en	: 1;
	uint16_t  hcom_callback_en		: 1;
	uint16_t  channel_direction	    : 3;
	uint16_t  busy					: 1;
	uint16_t  reserved				: 10;

};

struct dma_acts_data {
	uint32_t base;
	int chan_num;
	struct dma_acts_channel channels[MAX_DMA_CH];
};

#define DEV_DATA(dev) \
	((struct dma_acts_data *const)(dev)->data)


static struct dma_acts_data dmac_data;

DEVICE_DECLARE(dma_acts_0);


#if defined(CONFIG_DMA_DBG_DUMP)

static void dma_acts_dump_reg(struct dma_acts_data *ddev, uint32_t id)
{
	struct acts_dma_chan_reg *cregs = DMA_CHAN(ddev->base, id);
	LOG_INF("Using channel: %d", id);
	LOG_INF("  DMA_CTL:      0x%x", cregs->ctl);
	LOG_INF("  DMA_SADDR0:    0x%x", cregs->saddr0);
	LOG_INF("  DMA_SADDR1:    0x%x", cregs->saddr1);
	LOG_INF("  DMA_DADDR0:    0x%x", cregs->daddr0);
	LOG_INF("  DMA_DADDR1:    0x%x", cregs->daddr1);
	LOG_INF("  DMA_LEN:       0x%x", cregs->bc);
	LOG_INF("  DMA_RMAIN_LEN: 0x%x", cregs->rc);
}
void dma_dump_info(void)
{
	int i;
	struct dma_acts_channel *pchan;
	struct dma_acts_data *ddev = &dmac_data;

	LOG_INF("----pchan= %d stauts:--------\n", ddev->chan_num);
	for(i= 0; i < ddev->chan_num; i++){
		pchan = &ddev->channels[i];
		printk("chan%d: isbusy=%d\n",	i, pchan->busy);
		dma_acts_dump_reg(ddev, i);
	}

}
#endif


/* Handles DMA interrupts and dispatches to the individual channel */
static void dma_acts_isr(void *arg)
{
	uint32_t id = (uint32_t) arg;
	const struct device *dev = DEVICE_GET(dma_acts_0);
	struct dma_acts_data *ddev = &dmac_data;
	struct acts_dma_chan_reg *cregs = DMA_CHAN(ddev->base, id);
	struct acts_dma_reg *gregs = (struct acts_dma_reg *)ddev->base;
	struct dma_acts_channel *chan = &ddev->channels[id];
	uint32_t hf_pending, tc_pending;

	if (id >= ddev->chan_num)
		return;

	hf_pending = DMA_PD_HFIP(id) &
		  gregs->dma_ip & gregs->dma_ie;

	tc_pending = DMA_PD_TCIP(id) &
		  gregs->dma_ip & gregs->dma_ie;

	/* clear pending */
	gregs->dma_ip = tc_pending | hf_pending;

	if((tc_pending|hf_pending) == 0)
		return;
	/* process full complete callback */
	if ( chan->complete_callback_en && chan->cb) {
		chan->cb(dev, chan->cb_arg, id, !!hf_pending);

	}
	if(cregs->ctl & DMA_CTL_RELOAD)
		chan->reload_count++;
}


/* Configure a channel */
static int dma_acts_config(const struct device *dev, uint32_t channel,
			   struct dma_config *config)
{
	struct dma_acts_data *ddev = DEV_DATA(dev);
	struct dma_acts_channel *chan  = &ddev->channels[channel];
	struct acts_dma_chan_reg *cregs = DMA_CHAN(ddev->base, channel);
	struct dma_block_config *head_block = config->head_block;
	uint32_t ctl;
	int data_width = 0;

	if (channel >= ddev->chan_num) {
		LOG_ERR("DMA error:ch=%d > dma max chan=%d\n",channel,
		       ddev->chan_num);

		return -EINVAL;
	}

	if (head_block->block_size > DMA_ACTS_MAX_DATA_ITEMS) {
		LOG_ERR("DMA error: Data size too big: %d",
		       head_block->block_size);
		return -EINVAL;
	}

	if (config->complete_callback_en || config->error_callback_en) {
		chan->cb = config->dma_callback;
		chan->cb_arg = config->user_data;
		chan->complete_callback_en = config->complete_callback_en;
	} else {
		chan->cb = NULL;
		chan->complete_callback_en = 0;
	}
	chan->hcom_callback_en = 0;

	cregs->saddr0 = (uint32_t)head_block->source_address;
	cregs->daddr0 = (uint32_t)head_block->dest_address;
	cregs->bc = (uint32_t)head_block->block_size;

	chan->channel_direction = config->channel_direction;
	chan->reload_count = 0;

	if (config->channel_direction == MEMORY_TO_PERIPHERAL) {
		ctl = DMA_CTL_SRC_TYPE(DMA_ID_MEM) |
		      DMA_CTL_DST_TYPE(config->dma_slot) |
		      DMA_CTL_DAM_CONSTANT;
	} else if (config->channel_direction == PERIPHERAL_TO_MEMORY)  {
		ctl = DMA_CTL_SRC_TYPE(config->dma_slot) |
		      DMA_CTL_SAM_CONSTANT |
		      DMA_CTL_DST_TYPE(DMA_ID_MEM);
	} else {
		ctl = DMA_CTL_SRC_TYPE(DMA_ID_MEM) |
		      DMA_CTL_DST_TYPE(DMA_ID_MEM);
	}
	/** extern for actions dma interleaved mode */
	if (config->reserved == 1 && config->channel_direction == MEMORY_TO_PERIPHERAL) {
		ctl |= DMA_CTL_ADUDIO_TYPE_SEP;
	}else if(config->reserved == 1 && config->channel_direction == PERIPHERAL_TO_MEMORY) {
		ctl |= DMA_CTL_ADUDIO_TYPE_SEP;
	}

	if (config->source_burst_length == 1 || config->dest_burst_length == 1) {
		ctl |= DMA_CTL_TRM_SINGLE;
	}

	if (config->source_data_size) {
		data_width = config->source_data_size;
	}

	if (config->dest_data_size) {
		data_width = config->dest_data_size;
	}

	if (head_block->source_reload_en || head_block->dest_reload_en) {
		ctl |= DMA_CTL_RELOAD;
		chan->hcom_callback_en = 1;
	}

	switch (data_width) {
	case 2:
		ctl |= DMA_CTL_TWS_16BIT;
		break;
	case 4:
		ctl |= DMA_CTL_TWS_32BIT;
		break;
	case 1:
	default:
		ctl |= DMA_CTL_TWS_8BIT;
		break;
	}
	cregs->ctl = ctl;
	return 0;

}

static int dma_acts_start(const struct device *dev, uint32_t channel)
{

	struct dma_acts_data *ddev = DEV_DATA(dev);
	struct dma_acts_channel *chan  = &ddev->channels[channel];
	struct acts_dma_chan_reg *cregs = DMA_CHAN(ddev->base, channel);
	struct acts_dma_reg *gregs = (struct acts_dma_reg *)ddev->base;

	uint32_t key;

	if (channel >= ddev->chan_num) {
		return -EINVAL;
	}
	key = irq_lock();
	/* clear old irq pending */
	gregs->dma_ip = DMA_PD_TCIP(channel) | DMA_PD_HFIP(channel);

	gregs->dma_ie &= ~( DMA_IE_TCIP(channel) | DMA_IE_HFIP(channel));

	/* enable dma channel full complete irq? */
	if (chan->complete_callback_en) {
		gregs->dma_ie |= DMA_IE_TCIP(channel) ;
			/*DMA_CTL_RELOAD use half complete irq*/
		if(chan->hcom_callback_en)
			gregs->dma_ie |= DMA_IE_HFIP(channel) ;
	}

	/* set memory type such as interleaved or seperated for audio */
	if (cregs->ctl & DMA_CTL_ADUDIO_TYPE_SEP) {
		if ((cregs->ctl & DMA_CTL_DST_TYPE_MASK) == 0) {
			/* PERIPHERAL_TO_MEMORY */
			cregs->daddr1 = cregs->saddr0;
		} else {
			/* MEMORY_TO_PERIPHERAL */
			cregs->saddr1 = cregs->daddr0;
		}
	}

	/* start dma transfer */
	cregs->start |= DMA_START_START;
	irq_unlock(key);

	return 0;
}

static int dma_acts_stop(const struct device *dev, uint32_t channel)
{
	struct dma_acts_data *ddev = DEV_DATA(dev);
	struct acts_dma_chan_reg *cregs = DMA_CHAN(ddev->base, channel);
	struct acts_dma_reg *gregs = (struct acts_dma_reg *)ddev->base;
	uint32_t key;

	if (channel >= ddev->chan_num) {
		return -EINVAL;
	}
	key = irq_lock();
	gregs->dma_ie &= ~( DMA_IE_TCIP(channel) | DMA_IE_HFIP(channel));
	/* clear old irq pending */
	gregs->dma_ip = DMA_PD_TCIP(channel) | DMA_PD_HFIP(channel);

	/* disable reload brefore stop dma */
	cregs->ctl &= ~DMA_CTL_RELOAD;
	cregs->start &= ~DMA_START_START;
	irq_unlock(key);

	return 0;

}

static int dma_acts_reload(const struct device *dev, uint32_t channel,
			   uint32_t src, uint32_t dst, size_t size)
{

	struct dma_acts_data *ddev = DEV_DATA(dev);
	struct acts_dma_chan_reg *cregs = DMA_CHAN(ddev->base, channel);
	uint32_t key;

	if (channel >= ddev->chan_num) {
		return -EINVAL;
	}
	key = irq_lock();
	cregs->saddr0 = src;
	cregs->daddr0 = dst;
	cregs->bc = size;
	irq_unlock(key);
	return 0;

}

static int dma_acts_get_status(const struct device *dev, uint32_t channel,
			       struct dma_status *stat)
{
	struct dma_acts_data *ddev = DEV_DATA(dev);
	struct acts_dma_chan_reg *cregs = DMA_CHAN(ddev->base, channel);
	struct dma_acts_channel *chan  = &ddev->channels[channel];

	if (channel >= ddev->chan_num || stat == NULL) {
		return -EINVAL;
	}

	if (cregs->start) {
		stat->busy = true;
		stat->pending_length = cregs->rc;
	} else {
		stat->busy = false;
		stat->pending_length = 0;
	}
	stat->dir = chan->channel_direction;
	return 0;
}

static int dma_acts_request(const struct device *dev, uint32_t channel)
{
	struct dma_acts_data *ddev = DEV_DATA(dev);
	int i;
	uint32_t key;
	int ret = -EINVAL;
	//printk("-------requset:dma chan%d \n", channel);
	if (channel != DMA_INVALID_CHAN) {
		if(channel >= ddev->chan_num){
			printk("request chan=%d max err\n", channel);
			return -EINVAL;
		}
		key = irq_lock();
		if(ddev->channels[channel].busy){
			printk("request chan id%d already used\n", channel);
			ret = -EINVAL;
		}else{
			ret = channel;
			ddev->channels[channel].busy = 1;
		}
		irq_unlock(key);
	}else{
		key = irq_lock();
		for(i = ddev->chan_num-1; i >= 0; i--){
			if(!ddev->channels[i].busy)
				break;
		}
		if(i >= 0){
			ret = i;
			ddev->channels[i].busy = 1;
		}
		irq_unlock(key);
	}
	//printk("--- alloc dma chan%d \n", ret);

	return ret;
}
static void dma_acts_free(const struct device *dev, uint32_t channel)
{
	struct dma_acts_data *ddev = DEV_DATA(dev);
	uint32_t key;

	key = irq_lock();
	if(!ddev->channels[channel].busy){
		printk("err:dma chan%d is free\n", channel);
	}else{
		ddev->channels[channel].busy = 0;
	}
	irq_unlock(key);
}




#define DMA_ACTS_IRQ_CONNECT(n)						 \
	do {								 \
		IRQ_CONNECT((IRQ_ID_DMA0+n),		 \
			    CONFIG_DMA_IRQ_PRI,		 \
			    dma_acts_isr, n, 0);	 \
		irq_enable((IRQ_ID_DMA0+n));		 \
	} while (0)

#define DMA_NOT_RESERVE(chan) ((CONFIG_DMA_LCD_RESEVER_CHAN!=chan) \
								&& (CONFIG_DMA_SPINAND_RESEVER_CHAN!=chan) \
								&& (CONFIG_DMA_SPINOR_RESEVER_CHAN!=chan))

static int dma_acts_init(const struct device *dev)
{
	struct dma_acts_data *data = DEV_DATA(dev);

	data->base = DMA_REG_BASE;
	acts_clock_peripheral_enable(CLOCK_ID_DMA);
	acts_reset_peripheral(RESET_ID_DMA);
	data->chan_num = MAX_DMA_CH;

#if MAX_DMA_CH > 0
#if	DMA_NOT_RESERVE(0)
	DMA_ACTS_IRQ_CONNECT(0);
#endif
#endif

#if MAX_DMA_CH > 1
#if	DMA_NOT_RESERVE(1)
	DMA_ACTS_IRQ_CONNECT(1);
#endif
#endif

#if MAX_DMA_CH > 2
#if	DMA_NOT_RESERVE(2)
	DMA_ACTS_IRQ_CONNECT(2);
#endif
#endif

#if MAX_DMA_CH > 3
#if	DMA_NOT_RESERVE(3)
	DMA_ACTS_IRQ_CONNECT(3);
#endif
#endif

#if MAX_DMA_CH > 4
#if	DMA_NOT_RESERVE(4)
	DMA_ACTS_IRQ_CONNECT(4);
#endif
#endif

#if MAX_DMA_CH > 5
#if	DMA_NOT_RESERVE(5)
	DMA_ACTS_IRQ_CONNECT(5);
#endif
#endif

#if MAX_DMA_CH > 6
#if	DMA_NOT_RESERVE(6)
	DMA_ACTS_IRQ_CONNECT(6);
#endif
#endif

#if MAX_DMA_CH > 7
#if	DMA_NOT_RESERVE(7)
	DMA_ACTS_IRQ_CONNECT(7);
#endif
#endif

#if MAX_DMA_CH > 8
#if	DMA_NOT_RESERVE(8)
	DMA_ACTS_IRQ_CONNECT(8);
#endif
#endif


#if MAX_DMA_CH > 9
#if	DMA_NOT_RESERVE(9)
	DMA_ACTS_IRQ_CONNECT(9);
#endif
#endif
	printk("dma-num=%d\n", data->chan_num);
#if CONFIG_DMA_LCD_RESEVER_CHAN < MAX_DMA_CH
	data->channels[CONFIG_DMA_LCD_RESEVER_CHAN].busy = 1; //reserve for LCD
#endif
#if CONFIG_DMA_SPINAND_RESEVER_CHAN < MAX_DMA_CH
	data->channels[CONFIG_DMA_SPINAND_RESEVER_CHAN].busy = 1; //reserve for SPINAND
#endif

#if CONFIG_DMA_SPINOR_RESEVER_CHAN < MAX_DMA_CH
	data->channels[CONFIG_DMA_SPINOR_RESEVER_CHAN].busy = 1; // reserve for SPINOR
#endif

	return 0;
}


static const struct dma_driver_api dma_acts_api = {
	.config = dma_acts_config,
	.start = dma_acts_start,
	.stop = dma_acts_stop,
	.reload = dma_acts_reload,
	.get_status = dma_acts_get_status,
	.request = dma_acts_request,
	.free = dma_acts_free,

};

DEVICE_DEFINE(dma_acts_0, CONFIG_DMA_0_NAME, &dma_acts_init, NULL,
		    &dmac_data, NULL, POST_KERNEL,
		    1, &dma_acts_api);
