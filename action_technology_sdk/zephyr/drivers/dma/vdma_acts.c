/*
 * Copyright (c) 2018 Google LLC.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <soc.h>
#include <drivers/dma.h>
#include "vdma_list.h"
#include <board_cfg.h>
#include <logging/log.h>
LOG_MODULE_REGISTER(vdma_acts, CONFIG_DMA_LOG_LEVEL);


#define DMA_INVALID_CHAN       	(0xff)
#define DMA_VCHAN_START_ID  	32

#define DMA_VCHAN_MAX_NUM	  	CONFIG_DMA_0_VCHAN_NUM
#define DMA_VCHAN_PCHAN_NUM	  	CONFIG_DMA_0_VCHAN_PCHAN_NUM
#define DMA_VCHAN_PCHAN_START  	CONFIG_DMA_0_VCHAN_PCHAN_START
#define DMA_VCHAN_PCHAN_END  	(DMA_VCHAN_PCHAN_NUM + DMA_VCHAN_PCHAN_START)
#define DMA_PCHAN_IS_VCHAN(id)	(((id >= DMA_VCHAN_PCHAN_START) && (id < DMA_VCHAN_PCHAN_END))?1:0)


#define DMA_ID_MEM				0
#define MAX_DMA_CH  			CONFIG_DMA_0_PCHAN_NUM
#define DMA_CHAN(base, ch)		((struct dma_regs*)((base) +  (ch+1) * 0x100))
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


#if 0
#undef LOG_ERR
#undef LOG_DBG
#undef LOG_INF
#define LOG_ERR printk
#define LOG_INF printk
#define LOG_DBG printk
#endif


/* dma channel registers */
struct dma_regs {
	volatile uint32_t ctl;
	volatile uint32_t start;
	volatile uint32_t saddr0;
	volatile uint32_t saddr1;
	volatile uint32_t daddr0;
	volatile uint32_t daddr1;
	volatile uint32_t bc;
	volatile uint32_t rc;
};
struct dma_reg_g {
	volatile uint32_t dma_ip;
	volatile uint32_t dma_ie;
};




struct dma_pchan {
	dma_callback_t cb;
	void   *cb_arg;
    unsigned short reload_count;
	uint8_t  pchan_id;  //init val, not change
	uint8_t  vchan_id;
	uint8_t  complete_callback_en	: 1;
	uint8_t  hcom_callback_en		: 1;
	uint8_t  channel_direction		: 3;
	uint8_t  busy					: 1;  //pchan is for requst, vchan is for used
	uint8_t  isvchan				: 1;
	uint8_t  reserved				: 1;
};

struct dma_vchan {
    struct list_head list;
    struct dma_regs dma_regs;
    struct dma_pchan pchan;
	unsigned char vchan_id; //init val, not change
    unsigned char pchan_id;// =DMA_INVALID_CHAN is not transfer ,else is transfer
    unsigned char busy;  //for request
};

struct vdma_acts_data {
	uint32_t base;

	uint8_t pchan_num;
	uint8_t vchan_num;
	struct dma_pchan  pchan[MAX_DMA_CH];
	struct list_head dma_req_list;
};

#define DEV_DATA(dev) \
	((struct vdma_acts_data *const)(dev)->data)


static struct vdma_acts_data vdmac_data;
static struct dma_vchan dma_vchan_data[DMA_VCHAN_MAX_NUM];

DEVICE_DECLARE(vdma_acts_0);


#if defined(CONFIG_DMA_DBG_DUMP)

static void dma_acts_dump_reg(struct vdma_acts_data *ddev, uint32_t id)
{
	struct dma_regs *cregs = DMA_CHAN(ddev->base, id);
	printk("Using channel: %d \n", id);
	printk("  DMA_CTL:       0x%x \n", cregs->ctl);
	printk("  DMA_SADDR0:    0x%x \n", cregs->saddr0);
	printk("  DMA_SADDR1:    0x%x \n", cregs->saddr1);
	printk("  DMA_DADDR0:    0x%x \n", cregs->daddr0);
	printk("  DMA_DADDR1:    0x%x \n", cregs->daddr1);
	printk("  DMA_LEN:       0x%x \n", cregs->bc);
	printk("  DMA_RMAIN_LEN: 0x%x \n", cregs->rc);
}
void dma_dump_info(void)
{
	int i;
	struct dma_vchan *vchan;
	struct dma_pchan *pchan;
	struct vdma_acts_data *ddev = &vdmac_data;

	list_for_each_entry(vchan, &ddev->dma_req_list, list) {
		LOG_INF("vchan=%d, in req list\n", vchan->vchan_id);
	}
	LOG_INF("----vchan = %d stauts:--------\n", DMA_VCHAN_MAX_NUM);
	for(i = 0; i < DMA_VCHAN_MAX_NUM; i++) {
		vchan = &dma_vchan_data[i];
		printk("%d:vid=%d, busy=%d,start=%d, pch=%d\n",
			i, vchan->vchan_id, vchan->busy, vchan->dma_regs.start,
			vchan->pchan_id);
	}

	LOG_INF("----pchan= %d stauts:--------\n", ddev->pchan_num);
	for(i= 0; i < ddev->pchan_num; i++){
		pchan = &ddev->pchan[i];
		printk("%d:pid=%d, busy=%d,vid=%d, isv=%d:\n",
			i, pchan->pchan_id, pchan->busy, pchan->vchan_id,
			pchan->isvchan);
		dma_acts_dump_reg(ddev, i);
	}

}
#endif


static void vdma_vchan_start_tran(struct dma_vchan *vchan, struct dma_pchan *chan)
{
	struct vdma_acts_data *ddev = &vdmac_data;
	struct dma_regs *cregs = DMA_CHAN(ddev->base, chan->pchan_id);
	struct dma_regs *dma_reg = &vchan->dma_regs;
	struct dma_reg_g *gregs = (struct dma_reg_g *)ddev->base;

	chan->busy = 1;
	chan->vchan_id = vchan->vchan_id;
	vchan->pchan_id = chan->pchan_id;
	chan->channel_direction = vchan->pchan.channel_direction;

	cregs->saddr0 = dma_reg->saddr0;
	cregs->daddr0 = dma_reg->daddr0;
	cregs->saddr1 = dma_reg->saddr0;
	cregs->daddr1 = dma_reg->daddr1;
	cregs->bc = dma_reg->bc;
	cregs->ctl = dma_reg->ctl;
	/* clear old irq pending */
	gregs->dma_ip = DMA_PD_TCIP(chan->pchan_id);
	gregs->dma_ie &= ~( DMA_IE_TCIP(chan->pchan_id) | DMA_IE_HFIP(chan->pchan_id));
	gregs->dma_ie |= DMA_IE_TCIP(chan->pchan_id);

	if(vchan->pchan.hcom_callback_en)
		gregs->dma_ie |= DMA_IE_HFIP(chan->pchan_id);

	/* start dma transfer */
	cregs->start |= DMA_START_START;

}

static void vdma_vchan_free_pchan(struct vdma_acts_data *ddev, struct dma_pchan *chan)
{
	struct dma_vchan *vchan;
	chan->busy = 0;
	chan->vchan_id = DMA_INVALID_CHAN;
	/*start next vchan transfer*/
	if(!list_empty(&ddev->dma_req_list)) {
		vchan = (struct dma_vchan *) list_first_entry(&ddev->dma_req_list, struct dma_vchan, list);
		list_del(&vchan->list);
		vdma_vchan_start_tran(vchan, chan);
	}
}



/* Handles DMA interrupts and dispatches to the individual channel */
static void vdma_acts_isr(void *arg)
{
	uint32_t id = (uint32_t) arg;
	const struct device *dev = DEVICE_GET(vdma_acts_0);
	struct vdma_acts_data *ddev = &vdmac_data;
	struct dma_regs *cregs = DMA_CHAN(ddev->base, id);
	struct dma_reg_g *gregs = (struct dma_reg_g *)ddev->base;
	struct dma_pchan *chan = &ddev->pchan[id];
	struct dma_pchan *tmp;
	struct dma_vchan *vchan;
	unsigned int flags;
	uint32_t hf_pending, tc_pending;

	if(id != chan->pchan_id){
		printk("error: chan id=%d\n", chan->pchan_id);
		return;
	}

	if (id >= ddev->pchan_num)
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
	flags = irq_lock();
	if(chan->isvchan){
		if(chan->vchan_id < DMA_VCHAN_MAX_NUM  &&
				dma_vchan_data[chan->vchan_id].busy) {
			vchan = &dma_vchan_data[chan->vchan_id];
			tmp = &vchan->pchan;

			if(tc_pending){
				if(cregs->ctl & DMA_CTL_RELOAD){
					tmp->reload_count++;
				}else{
					vchan->pchan_id = DMA_INVALID_CHAN;//finshed
					vchan->dma_regs.rc = cregs->rc;
					vchan->dma_regs.bc = cregs->bc;
					vchan->dma_regs.start = 0;  // stop
					vdma_vchan_free_pchan(ddev,chan);
				}
			}

			if(tmp->cb && tmp->complete_callback_en) {
				tmp->cb(dev, tmp->cb_arg, chan->vchan_id+DMA_VCHAN_START_ID,
							!!hf_pending);
			}

		}else{
			vdma_vchan_free_pchan(ddev,chan);
			printk("vchan irq error: pid=%d, vid=%d \n", id, chan->vchan_id);
		}

	}else{
		if(chan->cb && chan->complete_callback_en) {
			chan->cb(dev, chan->cb_arg, id, !!hf_pending);
		}
		if(cregs->ctl & DMA_CTL_RELOAD)
			chan->reload_count++;
	}

	irq_unlock(flags);



}

/* Configure a channel */
static int vdma_acts_get_vchan(struct vdma_acts_data *ddev, uint32_t channel,
			   struct dma_vchan **vchan)
{
	struct dma_pchan *pchan;
	uint32_t ch;
	*vchan = NULL;
	if (channel >= DMA_VCHAN_START_ID) {
		if(channel >= DMA_VCHAN_START_ID+DMA_VCHAN_MAX_NUM) {
			LOG_ERR("VDMA error:ch=%d > dma max chan=%d\n", channel,
		       DMA_VCHAN_START_ID+DMA_VCHAN_MAX_NUM);
			return -EINVAL;
		}
		ch = channel-DMA_VCHAN_START_ID;
		if(dma_vchan_data[ch].vchan_id != ch) {
			LOG_ERR("vchanid err:%d != %d\n", dma_vchan_data[ch].vchan_id, ch);
			return -EINVAL;
		}
		if(!dma_vchan_data[ch].busy) {
			LOG_ERR("vchanid err:%d is not request\n", ch);
			return -EINVAL;
		}
		*vchan = &dma_vchan_data[ch];
	}else{
		if(channel >= ddev->pchan_num) {
			LOG_ERR("PDMA error:ch=%d > dma max chan=%d\n", channel,
		       ddev->pchan_num);
			return -EINVAL;
		}
		*vchan = NULL;
		pchan = &ddev->pchan[channel];
		if(pchan->pchan_id != channel) {
			LOG_ERR("err: pchan id :%d != %d\n",pchan->pchan_id, channel);
			return -EINVAL;
		}
		if(pchan->isvchan){
			LOG_ERR("error:pch=%d is vchan\n", channel);
			return -EINVAL;
		}
		if(!pchan->busy){
			LOG_ERR("error:pch=%d is not request\n", channel);
			return -EINVAL;
		}

	}

	return 0;
}


/* Configure a channel */
static int vdma_acts_config(const struct device *dev, uint32_t channel,
			   struct dma_config *config)
{
	struct vdma_acts_data *ddev = DEV_DATA(dev);
	struct dma_regs *cregs;
	struct dma_vchan *vchan = NULL;
	struct dma_pchan *pchan;
	struct dma_block_config *head_block = config->head_block;
	uint32_t ctl;
	int data_width = 0;
	int ret;
	uint32_t key;

	if (head_block->block_size > DMA_ACTS_MAX_DATA_ITEMS) {
		LOG_ERR("DMA error: Data size too big: %d",
		       head_block->block_size);
		return -EINVAL;
	}

	ret = vdma_acts_get_vchan(ddev, channel, &vchan);
	if(ret){
		LOG_DBG("err cfg\n");
		return ret;
	}

	key = irq_lock();
	if (vchan) {
		cregs = &vchan->dma_regs;
		pchan = &vchan->pchan;
	}else{
		cregs = DMA_CHAN(ddev->base, channel);
		pchan = &ddev->pchan[channel];
	}

	if (config->complete_callback_en || config->error_callback_en) {
		pchan->cb = config->dma_callback;
		pchan->cb_arg = config->user_data;
		pchan->complete_callback_en = config->complete_callback_en;
	} else {
		pchan->cb = NULL;
		pchan->complete_callback_en = 0;
	}
	pchan->hcom_callback_en = 0;

	cregs->saddr0 = (uint32_t)head_block->source_address;
	cregs->daddr0 = (uint32_t)head_block->dest_address;
	cregs->bc = (uint32_t)head_block->block_size;
	pchan->channel_direction = config->channel_direction;
	pchan->reload_count = 0;

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
		cregs->saddr1 = (uint32_t)head_block->source_address;
		ctl |= DMA_CTL_ADUDIO_TYPE_SEP;
	}else if(config->reserved == 1 && config->channel_direction == PERIPHERAL_TO_MEMORY) {
		cregs->daddr1 = (uint32_t)head_block->dest_address;
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
		pchan->hcom_callback_en = 1;
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
	irq_unlock(key);

	return 0;

}


static struct dma_pchan * vdma_acts_get_free_chan(struct vdma_acts_data *ddev)
{
	uint32_t i, ch;
	struct dma_pchan *pch;

	for(i = 0; i < DMA_VCHAN_PCHAN_NUM; i++){
		ch = i + DMA_VCHAN_PCHAN_START;
		pch = &ddev->pchan[ch];
		if((!pch->isvchan) || (pch->pchan_id !=ch)){
			LOG_ERR("vchan err:isv=%d, id=%d != %d\n",pch->isvchan, pch->pchan_id, ch);
		}
		if(!pch->busy)
			return pch;
	}
	return NULL;
}


static int vdma_acts_start(const struct device *dev, uint32_t channel)
{

	struct vdma_acts_data *ddev = DEV_DATA(dev);
	struct dma_vchan *vchan = NULL;
	struct dma_pchan *pchan, *tmp;
	struct dma_regs *cregs;
	struct dma_reg_g *gregs = (struct dma_reg_g *)ddev->base;
	uint32_t key;
	int ret;

	ret = vdma_acts_get_vchan(ddev, channel, &vchan);
	if(ret){
		LOG_DBG("err start\n");
		return ret;
	}
	key = irq_lock();

	if(vchan) {
		cregs = &vchan->dma_regs;
		if(!cregs->start) {
			pchan = &vchan->pchan;
			cregs->start = 1; // start flag
			tmp = vdma_acts_get_free_chan(ddev);
			if(tmp){
				vdma_vchan_start_tran(vchan, tmp); // start tansfer
			}else{
				list_add_tail(&vchan->list, &ddev->dma_req_list); //wait
			}
		}
	}else{
		cregs = DMA_CHAN(ddev->base, channel);
		pchan = &ddev->pchan[channel];
		/* clear old irq pending */
		gregs->dma_ip = DMA_PD_TCIP(channel) | DMA_PD_HFIP(channel);
		gregs->dma_ie &= ~( DMA_IE_TCIP(channel) | DMA_IE_HFIP(channel));
		/* enable dma channel full complete irq? */
		if (pchan->complete_callback_en) {
			gregs->dma_ie |= DMA_IE_TCIP(channel) ;

			/*DMA_CTL_RELOAD use half complete irq*/
			if(pchan->hcom_callback_en)
				gregs->dma_ie |= DMA_IE_HFIP(channel) ;
		}

		/* start dma transfer */
		cregs->start |= DMA_START_START;
	}
	irq_unlock(key);

	return 0;
}

static void _vdma_acts_stop(struct dma_regs *cregs, struct dma_reg_g *gregs, uint32_t ch)
{
	gregs->dma_ie &= ~( DMA_IE_TCIP(ch) | DMA_IE_HFIP(ch));
	/* clear old irq pending */
	gregs->dma_ip = DMA_PD_TCIP(ch) | DMA_PD_HFIP(ch);
	/* disable reload brefore stop dma */
	cregs->ctl &= ~DMA_CTL_RELOAD;
	cregs->start &= ~DMA_START_START;
}

static int vdma_acts_stop(const struct device *dev, uint32_t channel)
{
	struct vdma_acts_data *ddev = DEV_DATA(dev);
	struct dma_vchan *vchan = NULL;
	struct dma_pchan *pchan;
	struct dma_regs *cregs;
	struct dma_reg_g *gregs = (struct dma_reg_g *)ddev->base;
	uint32_t key;
	int ret;

	ret = vdma_acts_get_vchan(ddev, channel, &vchan);
	if(ret){
		LOG_DBG("err stop\n");
		return ret;
	}
	key = irq_lock();
	if(vchan) {
		if(vchan->dma_regs.start){
			if(vchan->pchan_id != DMA_INVALID_CHAN){// is transfering
				cregs = DMA_CHAN(ddev->base, vchan->pchan_id);
				_vdma_acts_stop(cregs, gregs, vchan->pchan_id);
				pchan = &ddev->pchan[vchan->pchan_id];
				vdma_vchan_free_pchan(ddev, pchan);
				vchan->pchan_id = DMA_INVALID_CHAN;
			}else{// del list
				list_del_init(&vchan->list);
			}
			vchan->dma_regs.start = 0;
		}
	}else{
		cregs = DMA_CHAN(ddev->base, channel);
		_vdma_acts_stop(cregs, gregs, channel);
	}
	irq_unlock(key);

	return 0;

}

static int vdma_acts_reload(const struct device *dev, uint32_t channel,
			   uint32_t src, uint32_t dst, size_t size)
{

	struct vdma_acts_data *ddev = DEV_DATA(dev);
	struct dma_vchan *vchan = NULL;
	struct dma_regs *cregs;
	uint32_t key;
	int ret;

	ret = vdma_acts_get_vchan(ddev, channel, &vchan);
	if(ret){
		LOG_DBG("err reload\n");
		return ret;
	}
	key = irq_lock();
	if(vchan){
		cregs = &vchan->dma_regs;
		if(vchan->dma_regs.start && (vchan->pchan_id != DMA_INVALID_CHAN)){
			// is transfering
			cregs->saddr0 = src;
			cregs->daddr0 = dst;
			cregs->bc = size;
			cregs = DMA_CHAN(ddev->base, vchan->pchan_id);
		}
	}else{
		cregs = DMA_CHAN(ddev->base, channel);
	}
	cregs->saddr0 = src;
	cregs->daddr0 = dst;
	cregs->bc = size;
	irq_unlock(key);
	return 0;

}

static int vdma_acts_get_status(const struct device *dev, uint32_t channel,
			       struct dma_status *stat)
{

	struct vdma_acts_data *ddev = DEV_DATA(dev);
	struct dma_vchan *vchan = NULL;
	struct dma_regs *cregs;
	uint32_t key;
	int ret;

	ret = vdma_acts_get_vchan(ddev, channel, &vchan);
	if(ret){
		LOG_DBG("err status\n");
		return ret;
	}
	key = irq_lock();
	if(vchan){
		cregs = &vchan->dma_regs;
		if(vchan->dma_regs.start && (vchan->pchan_id != DMA_INVALID_CHAN)){
			cregs = DMA_CHAN(ddev->base, vchan->pchan_id);//is transfering
		}
	}else{
		cregs = DMA_CHAN(ddev->base, channel);
	}

	if (cregs->start) {
		stat->busy = true;
		stat->pending_length = cregs->rc;
	} else {
		stat->busy = false;
		stat->pending_length = 0;
	}


	irq_unlock(key);
	return 0;

}

static int vdma_acts_request(const struct device *dev, uint32_t channel)
{
	struct vdma_acts_data *ddev = DEV_DATA(dev);
	struct dma_vchan *vchan;
	uint32_t key;
	int ret = -EINVAL;
	int i;

	if (channel != DMA_INVALID_CHAN) { //pchan
		if((channel >= ddev->pchan_num) || DMA_PCHAN_IS_VCHAN(channel)){
			printk("request pchan=%d  err\n", channel);
			return -EINVAL;
		}
		key = irq_lock();
		if(ddev->pchan[channel].busy){
			printk("request pchan id%d already used\n", channel);
			ret = -EINVAL;
		}else{
			ret = channel;
			ddev->pchan[channel].busy = 1;
		}
		irq_unlock(key);
	} else {// vchan
		key = irq_lock();
		for(i= 0; i < DMA_VCHAN_MAX_NUM; i++){
			vchan = &dma_vchan_data[i];
			if(vchan->vchan_id != i){
				printk("err err: vchan id=%d\n", i);
			}
			if(!vchan->busy){
				vchan->busy = 1;
				vchan->pchan_id = DMA_INVALID_CHAN;
				ret = i+ DMA_VCHAN_START_ID;
				break;
			}
		}
		irq_unlock(key);
		if(ret < 0){
			printk("request vchan fail\n");
		}
	}
	return ret;
}
static void vdma_acts_free(const struct device *dev, uint32_t channel)
{
	struct vdma_acts_data *ddev = DEV_DATA(dev);
	struct dma_vchan *vchan = NULL;
	struct dma_pchan *pchan;
	int ret;
	uint32_t key;
	vdma_acts_stop(dev, channel);
	ret = vdma_acts_get_vchan(ddev, channel, &vchan);
	if(ret){
		LOG_DBG("err free\n");
		return;
	}
	key = irq_lock();
	if(vchan){
		vchan->busy = 0;
		vchan->pchan_id = DMA_INVALID_CHAN;
	}else{
		pchan = &ddev->pchan[channel];
		pchan->busy = 0;
	}
	irq_unlock(key);
}





#define DMA_ACTS_IRQ_CONNECT(n)						 \
	do {								 \
		IRQ_CONNECT((IRQ_ID_DMA0+n),		 \
			    CONFIG_DMA_IRQ_PRI,		 \
			    vdma_acts_isr, n, 0);	 \
		irq_enable((IRQ_ID_DMA0+n));		 \
	} while (0)

#define DMA_NOT_RESERVE(chan) ((CONFIG_DMA_LCD_RESEVER_CHAN!=chan) && (CONFIG_DMA_SPINAND_RESEVER_CHAN!=chan))

static int vdma_acts_init(const struct device *dev)
{
	struct vdma_acts_data *data = DEV_DATA(dev);
	int i;
	struct dma_pchan *pchan;
	struct dma_vchan *vchan;

	data->base = DMA_REG_BASE;
	acts_clock_peripheral_enable(CLOCK_ID_DMA);
	acts_reset_peripheral(RESET_ID_DMA);
	data->pchan_num = MAX_DMA_CH;

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

	printk("pchan=%d; vchan=%d, pnum=%d,start=%d\n", data->pchan_num,
		DMA_VCHAN_MAX_NUM, DMA_VCHAN_PCHAN_NUM, DMA_VCHAN_PCHAN_START);

	//if(DMA_VCHAN_PCHAN_END > data->pchan_num){
		//printk("error vchan config\n");
		//return -1;
	//}
	INIT_LIST_HEAD(&data->dma_req_list);
	for(i= 0; i < data->pchan_num; i++){
		pchan = &data->pchan[i];
		pchan->pchan_id = i;
		pchan->vchan_id = DMA_INVALID_CHAN;
		pchan->busy = 0;
		if(DMA_PCHAN_IS_VCHAN(i))
			pchan->isvchan = 1;
		else
			pchan->isvchan = 0;
	}
	for(i= 0; i < DMA_VCHAN_MAX_NUM; i++){
		vchan = &dma_vchan_data[i];
		vchan->busy = 0;
		vchan->vchan_id = i;
		vchan->pchan_id = DMA_INVALID_CHAN;
		INIT_LIST_HEAD(&vchan->list);
	}
#if CONFIG_DMA_LCD_RESEVER_CHAN < MAX_DMA_CH
	data->pchan[CONFIG_DMA_LCD_RESEVER_CHAN].busy = 1; //reserve for LCD
#endif
#if CONFIG_DMA_SPINAND_RESEVER_CHAN < MAX_DMA_CH
	data->pchan[CONFIG_DMA_SPINAND_RESEVER_CHAN].busy = 1; //reserve for SPINAND
#endif


	return 0;
}


static const struct dma_driver_api vdma_acts_api = {
	.config = vdma_acts_config,
	.start = vdma_acts_start,
	.stop = vdma_acts_stop,
	.reload = vdma_acts_reload,
	.get_status = vdma_acts_get_status,
	.request = vdma_acts_request,
	.free = vdma_acts_free,
};

DEVICE_DEFINE(vdma_acts_0, CONFIG_DMA_0_NAME, &vdma_acts_init, NULL,
		    &vdmac_data, NULL, POST_KERNEL,
		    1, &vdma_acts_api);



