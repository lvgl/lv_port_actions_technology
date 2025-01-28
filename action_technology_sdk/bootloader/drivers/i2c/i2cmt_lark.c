/*
 * Copyright (c) 2017 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief I2CMT master driver for Actions SoC
 */

//#define DT_DRV_COMPAT actions_acts_i2cmt

#include <errno.h>
#include <sys/__assert.h>
#include <stdbool.h>
#include <kernel.h>
#include <device.h>
#include <init.h>
#include <drivers/ipmsg.h>
#include <drivers/i2cmt.h>
#include <rbuf/rbuf_msg_sc.h>
#include <soc.h>
#include <board_cfg.h>
#include <linker/linker-defs.h>


#define LOG_LEVEL CONFIG_I2C_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(i2cmt_acts);

#define I2CMT_CLOCK				(4000000)

/**
  * @brief I2C Module (I2CMT)
  */
typedef struct {
  volatile uint32_t  CTL;							/*!< (@ 0x00000000) TASK Control Register									   */
  volatile uint32_t  DMA_CTL;						/*!< (@ 0x00000004) TASK DMA Control Register								   */
  volatile uint32_t  DMA_ADD;						/*!< (@ 0x00000008) TASK DMA Address Register								   */
  volatile uint32_t  DMA_CNT;						/*!< (@ 0x0000000C) TASK DMA CNT Register									   */
  volatile uint32_t  DMA_RC;						/*!< (@ 0x00000010) TASK DMA remain counter Register						   */
  volatile uint32_t  RESERVED[11];
} I2CMT_AUTO_TASK_Type; 						/*!< Size = 64 (0x40)														   */

typedef struct {                                /*!< (@ 0x40088000) I2CMT Structure                                            */
  volatile uint32_t  CTL;                          /*!< (@ 0x00000000) I2C Control Register                                       */
  volatile uint32_t  NML_STA;                      /*!< (@ 0x00000004) I2C Status Register                                        */
  volatile uint32_t  NML_DAT;                      /*!< (@ 0x00000008) I2C TX/RX DATA Register                                    */
  volatile uint32_t  AUTO_TASK_STAT;               /*!< (@ 0x0000000C) Auto task status Register                                  */
  volatile uint32_t  AUTO_TASK_IE;                 /*!< (@ 0x00000010) Auto task IRQ enable Register                              */
  volatile uint32_t  AUTO_TASK_IP;                 /*!< (@ 0x00000014) Auto task IRQ pending Register                             */
  volatile uint32_t  RESERVED[58];
  volatile I2CMT_AUTO_TASK_Type AUTO_TASK[4];      /*!< (@ 0x00000100) TASK[0..3] Group                                           */
  volatile uint32_t  RESERVED1[3968];
} I2CMT_Type;                                   /*!< Size = 512 (0x200)                                                        */
typedef I2CMT_Type  I2CMT_ARRAYType[1];         /*!< max. 2 instances available                                                */

#define I2CMT                       ((I2CMT_ARRAYType*)        I2CMT0_REG_BASE)

/* ==========================================================  CTL  ========================================================== */
#define I2CMT_CTL_CLKDIV_Msk              (0xf80UL)                 /*!< CLKDIV (Bitfield-Mask: 0x1f)                          */
#define I2CMT_CTL_MODSEL_Msk              (0x40UL)                  /*!< MODSEL (Bitfield-Mask: 0x01)                          */
#define I2CMT_CTL_EN_Msk                  (0x20UL)                  /*!< EN (Bitfield-Mask: 0x01)                              */
#define I2CMT_CTL_IRQE_Msk                (0x10UL)                  /*!< IRQE (Bitfield-Mask: 0x01)                            */
#define I2CMT_CTL_GBCC_Msk                (0xcUL)                   /*!< GBCC (Bitfield-Mask: 0x03)                            */
#define I2CMT_CTL_RB_Msk                  (0x2UL)                   /*!< RB (Bitfield-Mask: 0x01)                              */
#define I2CMT_CTL_GACK_Msk                (0x1UL)                   /*!< GACK (Bitfield-Mask: 0x01)                            */
#define I2CMT_CTL_CLKDIV_Pos              (7UL)                     /*!< CLKDIV (Bit 7)                                        */
#define I2CMT_CTL_MODSEL_Pos              (6UL)                     /*!< MODSEL (Bit 6)                                        */
#define I2CMT_CTL_EN_Pos                  (5UL)                     /*!< EN (Bit 5)                                            */
#define I2CMT_CTL_IRQE_Pos                (4UL)                     /*!< IRQE (Bit 4)                                          */
#define I2CMT_CTL_GBCC_Pos                (2UL)                     /*!< GBCC (Bit 2)                                          */
#define I2CMT_CTL_RB_Pos                  (1UL)                     /*!< RB (Bit 1)                                            */
#define I2CMT_CTL_GACK_Pos                (0UL)                     /*!< GACK (Bit 0)                                          */
/* ========================================================  NML_STA  ======================================================== */
#define I2CMT_NML_STA_TCB_Msk             (0x100UL)                 /*!< TCB (Bitfield-Mask: 0x01)                             */
#define I2CMT_NML_STA_STPD_Msk            (0x80UL)                  /*!< STPD (Bitfield-Mask: 0x01)                            */
#define I2CMT_NML_STA_STAD_Msk            (0x40UL)                  /*!< STAD (Bitfield-Mask: 0x01)                            */
#define I2CMT_NML_STA_RWST_Msk            (0x20UL)                  /*!< RWST (Bitfield-Mask: 0x01)                            */
#define I2CMT_NML_STA_LBST_Msk            (0x10UL)                  /*!< LBST (Bitfield-Mask: 0x01)                            */
#define I2CMT_NML_STA_IRQP_Msk            (0x8UL)                   /*!< IRQP (Bitfield-Mask: 0x01)                            */
#define I2CMT_NML_STA_BBB_Msk             (0x4UL)                   /*!< BBB (Bitfield-Mask: 0x01)                             */
#define I2CMT_NML_STA_BEB_Msk             (0x2UL)                   /*!< BEB (Bitfield-Mask: 0x01)                             */
#define I2CMT_NML_STA_RACK_Msk            (0x1UL)                   /*!< RACK (Bitfield-Mask: 0x01)                            */
/* =====================================================  AUTO_TASK_IE  ====================================================== */
#define I2CMT_AUTO_TASK_IE_TSK0BERIE_Pos  (12UL)                    /*!< TSK0BERIE (Bit 12)                                    */
#define I2CMT_AUTO_TASK_IE_TSK0NAKIE_Pos  (8UL)                     /*!< TSK0NAKIE (Bit 8)                                     */
#define I2CMT_AUTO_TASK_IE_TSK0HFIE_Pos   (4UL)                     /*!< TSK0HFIE (Bit 4)                                      */
#define I2CMT_AUTO_TASK_IE_TSK0TCIE_Pos   (0UL)                     /*!< TSK0TCIE (Bit 0)                                      */
/* =====================================================  AUTO_TASK_IP  ====================================================== */
#define I2CMT_AUTO_TASK_IP_TSK0BERIP_Pos  (12UL)                    /*!< TSK0BERIP (Bit 12)                                    */
#define I2CMT_AUTO_TASK_IP_TSK0NAKIP_Pos  (8UL)                     /*!< TSK0NAKIP (Bit 8)                                     */
#define I2CMT_AUTO_TASK_IP_TSK0HFIP_Pos   (4UL)                     /*!< TSK0HFIP (Bit 4)                                      */
#define I2CMT_AUTO_TASK_IP_TSK0TCIP_Pos   (0UL)                     /*!< TSK0TCIP (Bit 0)                                      */
/* ==========================================================  CTL  ========================================================== */
#define I2CMT_AUTO_TASK_CTL_SOFT_ST_Msk   (0x80000000UL)            /*!< SOFT_ST (Bitfield-Mask: 0x01)                         */
/* ========================================================  DMA_CTL  ======================================================== */
#define I2CMT_AUTO_TASK_DMA_CTL_DMARELD_Pos (1UL)                   /*!< DMARELD (Bit 1)                                       */
#define I2CMT_AUTO_TASK_DMA_CTL_DMASTART_Msk (0x1UL)                /*!< DMASTART (Bitfield-Mask: 0x01)                        */

// I2C BUS STAT
#define MASK				(I2CMT_CTL_GBCC_Msk | I2CMT_CTL_RB_Msk | I2CMT_CTL_GACK_Msk)
#define START				(1 << I2CMT_CTL_GBCC_Pos)
#define	STOP				(2 << I2CMT_CTL_GBCC_Pos)
#define	RESTA				(3 << I2CMT_CTL_GBCC_Pos)
#define	REBUS				I2CMT_CTL_RB_Msk
#define	ACK					0
#define	NACK				I2CMT_CTL_GACK_Msk

// TIMER
#define CMU_TIMERCLK(id)			(CMU_TIMER0CLK + (id) * 4)
#define T_CTL(id)					(T0_CTL + (id) * 0x20)
#define T_VAL(id)					(T0_VAL + (id) * 0x20)

struct acts_i2cmt_config {
	uint32_t ctl_reg;
	uint32_t clk_freq;
	uint8_t bus_id;
	uint8_t clock_id;
	uint8_t reset_id;
	void (*irq_config_func)(void);
};

/* Device run time data */
struct acts_i2cmt_data {
	struct k_mutex mutex;
	i2c_task_callback_t task_cb[I2C_TASK_NUM];
	void *task_cb_ctx[I2C_TASK_NUM];
	i2c_task_t *task_attr[I2C_TASK_NUM];
	uint8_t *task_buf[I2C_TASK_NUM];
	uint32_t task_len[I2C_TASK_NUM];
};

#define DEV_NAME(dev) ((dev)->name)
#define DEV_CFG(dev) \
	((const struct acts_i2cmt_config *const)(dev)->config)
#define DEV_DATA(dev) \
	((struct acts_i2cmt_data *const)(dev)->data)

static void i2cmt_set_rate(int i2c_dev, int rate)
{
	unsigned int val, div;

	/* RC4M clock src and div=1 */
	val = (0 << 8) | (0 << 0);
	sys_write32(val, CMU_I2CMT0CLK + i2c_dev * 4);

	div = (I2CMT_CLOCK + rate * 2 - 1) / (rate * 2);
	I2CMT[i2c_dev]->CTL &= ~I2CMT_CTL_CLKDIV_Msk;
	I2CMT[i2c_dev]->CTL |= div << I2CMT_CTL_CLKDIV_Pos;
}

// switch to auto mode
static void i2cmt_auto_mode_set(int i2c_dev)
{
	/* clear task IRQ pending */
	I2CMT[i2c_dev]->AUTO_TASK_IP = I2CMT[i2c_dev]->AUTO_TASK_IP;

	/* set auto mode */
	I2CMT[i2c_dev]->CTL |= I2CMT_CTL_MODSEL_Msk;
}

// switch to normal mode
static void i2cmt_auto_mode_cancel(int i2c_dev)
{
	/* cancel auto mode */
	I2CMT[i2c_dev]->CTL &= ~I2CMT_CTL_MODSEL_Msk;
}

// configure i2c task
static void i2cmt_auto_task_config(int i2c_dev, int task_id, i2c_task_t *task_attr, uint32_t addr)
{
	uint8_t *pdata = (uint8_t *)&task_attr->ctl;
	volatile uint32_t ctl = *(volatile unsigned int*)pdata;

	/* config task dma first */
	I2CMT[i2c_dev]->AUTO_TASK[task_id].DMA_ADD = addr;//task_attr->dma.addr;
	I2CMT[i2c_dev]->AUTO_TASK[task_id].DMA_CNT = task_attr->dma.len;
	if (task_attr->dma.len > 0) {
		I2CMT[i2c_dev]->AUTO_TASK[task_id].DMA_CTL = I2CMT_AUTO_TASK_DMA_CTL_DMASTART_Msk
													| (task_attr->dma.reload << I2CMT_AUTO_TASK_DMA_CTL_DMARELD_Pos);
	}

	if(task_attr->irq_type & I2C_TASK_IRQ_CMPLT) {
		/* enable task DMA transmission complete IRQ */
		I2CMT[i2c_dev]->AUTO_TASK_IE |= 1 << (I2CMT_AUTO_TASK_IE_TSK0TCIE_Pos + task_id);
	} else {
		/* disable task DMA transmission complete IRQ */
		I2CMT[i2c_dev]->AUTO_TASK_IE &= ~(1 << (I2CMT_AUTO_TASK_IE_TSK0TCIE_Pos + task_id));
	}

	if(task_attr->irq_type & I2C_TASK_IRQ_HALF_CMPLT) {
		/* enable task DMA Half transmission complete IRQ */
		I2CMT[i2c_dev]->AUTO_TASK_IE |= 1 << (I2CMT_AUTO_TASK_IE_TSK0HFIE_Pos + task_id);
	} else {
		/* disable task DMA Half transmission complete IRQ */
		I2CMT[i2c_dev]->AUTO_TASK_IE &= ~(1 << (I2CMT_AUTO_TASK_IE_TSK0HFIE_Pos + task_id));
	}

	if(task_attr->irq_type & I2C_TASK_IRQ_NACK) {
		/* enable task DMA receive NACK IRQ */
		I2CMT[i2c_dev]->AUTO_TASK_IE |= 1 << (I2CMT_AUTO_TASK_IE_TSK0NAKIE_Pos + task_id);
	} else {
		/* disable task DMA receive NACK IRQ */
		I2CMT[i2c_dev]->AUTO_TASK_IE &= ~(1 << (I2CMT_AUTO_TASK_IE_TSK0NAKIE_Pos + task_id));
	}

	if(task_attr->irq_type & I2C_TASK_IRQ_BUS_ERROR) {
		/* enable task DMA bus error IRQ */
		I2CMT[i2c_dev]->AUTO_TASK_IE |= 1 << (I2CMT_AUTO_TASK_IE_TSK0BERIE_Pos + task_id);
	} else {
		/* disable task DMA bus error IRQ */
		I2CMT[i2c_dev]->AUTO_TASK_IE &= ~(1 << (I2CMT_AUTO_TASK_IE_TSK0BERIE_Pos + task_id));
	}

	/* config task ctl*/
	I2CMT[i2c_dev]->AUTO_TASK[task_id].CTL = (ctl & ~I2CMT_AUTO_TASK_CTL_SOFT_ST_Msk);
}

// force trigger i2c task by software if don't use externtal trigger sources
static void i2cmt_auto_task_soft_start(int i2c_dev, int task_id)
{
	/* trigger task by software */
	I2CMT[i2c_dev]->AUTO_TASK[task_id].CTL &= ~I2CMT_AUTO_TASK_CTL_SOFT_ST_Msk;
	I2CMT[i2c_dev]->AUTO_TASK[task_id].CTL |= I2CMT_AUTO_TASK_CTL_SOFT_ST_Msk;
}

// force trigger i2c task by software if don't use externtal trigger sources
static void i2cmt_auto_task_soft_stop(int i2c_dev, int task_id)
{
	/* trigger task by software */
	I2CMT[i2c_dev]->AUTO_TASK[task_id].CTL &= ~I2CMT_AUTO_TASK_CTL_SOFT_ST_Msk;
}

//get the irq pending
static __sleepfunc int i2cmt_auto_task_irq_get_pending(int i2c_dev)
{
	return I2CMT[i2c_dev]->AUTO_TASK_IP & I2CMT[i2c_dev]->AUTO_TASK_IE;
}

static __sleepfunc int i2cmt_auto_task_irq_mask(int task_id, int task_irq_type)
{
	int irq_pending_mask = 0;

	switch (task_irq_type) {
		case I2C_TASK_IRQ_CMPLT:
			irq_pending_mask = 1 << (I2CMT_AUTO_TASK_IP_TSK0TCIP_Pos + task_id);
			break;
		case I2C_TASK_IRQ_HALF_CMPLT:
			irq_pending_mask = 1 << (I2CMT_AUTO_TASK_IP_TSK0HFIP_Pos + task_id);
			break;
		case I2C_TASK_IRQ_NACK:
			irq_pending_mask = 1 << (I2CMT_AUTO_TASK_IP_TSK0NAKIP_Pos + task_id);
			break;
		case I2C_TASK_IRQ_BUS_ERROR:
			irq_pending_mask = 1 << (I2CMT_AUTO_TASK_IP_TSK0BERIP_Pos + task_id);
			break;
		default:
			break;
	}

	return irq_pending_mask;
}

//check if the irq is pending
//static __sleepfunc int i2cmt_auto_task_irq_is_pending(int i2c_dev, int task_id, int task_irq_type)
//{
//	return I2CMT[i2c_dev]->AUTO_TASK_IP & i2cmt_auto_task_irq_mask(task_id, task_irq_type);
//}

//clear task irq pending
static __sleepfunc void i2cmt_auto_task_irq_clr_pending(int i2c_dev, int task_id, int task_irq_type)
{
	I2CMT[i2c_dev]->AUTO_TASK_IP = i2cmt_auto_task_irq_mask(task_id, task_irq_type);
}

static int i2cmt_wait_start(int i2c_dev)
{
	int ret = 0;
	int pre_time, cur_time;

	// set a 50ms timeout mechanism
	pre_time = k_uptime_get_32();

	// wait for start
	while(!(I2CMT[i2c_dev]->NML_STA & I2CMT_NML_STA_STAD_Msk)) {
		cur_time = k_uptime_get_32();

		// stop retry when timeout
		if ((cur_time - pre_time) > 50) {
			ret = -1;
			break;
		}
	}

	if (!ret) {
		// clear start bit
		I2CMT[i2c_dev]->NML_STA = I2CMT_NML_STA_STAD_Msk;
	}

	return ret;
}

static int i2cmt_wait_stop(int i2c_dev)
{
	int ret = 0;
	int pre_time, cur_time;

	// set a 50ms timeout mechanism
	pre_time = k_uptime_get_32();

	// wait for stop
	while(!(I2CMT[i2c_dev]->NML_STA & I2CMT_NML_STA_STPD_Msk)) {
		cur_time = k_uptime_get_32();

		// stop retry when timeout
		if ((cur_time - pre_time) > 50) {
			ret = -1;
			break;
		}
	}

	if (!ret) {
		// clear stop bit
		I2CMT[i2c_dev]->NML_STA = I2CMT_NML_STA_STPD_Msk;
	}

	return ret;
}

static int i2cmt_wait_complete(int i2c_dev)
{
	int ret = 0;
	int pre_time, cur_time;

	// set a 50ms timeout mechanism
	pre_time = k_uptime_get_32();

	// wait for complete
	while(!(I2CMT[i2c_dev]->NML_STA & I2CMT_NML_STA_TCB_Msk)) {
		cur_time = k_uptime_get_32();

		// stop retry when timeout
		if ((cur_time - pre_time) > 50) {
			ret = -1;
			break;
		}
	}

	if (!ret) {
		// clear complete bit
		I2CMT[i2c_dev]->NML_STA = I2CMT_NML_STA_TCB_Msk;
	}

	return ret;
}

static int i2cmt_xfer(int i2c_dev, i2c_xfer_t *xfer, int wr)
{
	int ret = I2C_XFER_OK;
	int i, sta_mode;

	/* cancel auto mode */
	i2cmt_auto_mode_cancel(i2c_dev);

	/* i2c clear status */
	I2CMT[i2c_dev]->NML_STA = I2CMT[i2c_dev]->NML_STA;

	/* has cmd or write mode */
	if ((xfer->cmd_len > 0) || wr) {
		/* write device addr + w */
		I2CMT[i2c_dev]->NML_DAT = (xfer->dev << 1);

		/* send start and addr */
		I2CMT[i2c_dev]->CTL = (I2CMT[i2c_dev]->CTL & ~MASK) | START | REBUS;
		if (i2cmt_wait_start(i2c_dev)) {
			return I2C_XFER_START_FAILED;
		}

		/* write for completed */
		if (i2cmt_wait_complete(i2c_dev)) {
			return I2C_XFER_NO_TCB;
		}

		/* check bus error */
		if (I2CMT[i2c_dev]->NML_STA & I2CMT_NML_STA_BEB_Msk) {
			ret = I2C_XFER_BUS_ERR;
			goto _xfer_out;
		}

		/* check nack */
		if (!(I2CMT[i2c_dev]->NML_STA & I2CMT_NML_STA_RACK_Msk)) {
			ret = I2C_XFER_NACK;
			goto _xfer_out;
		}

		/* write cmd and wait for transfer complete */
		for (i = 0; i < xfer->cmd_len; i ++) {
			I2CMT[i2c_dev]->NML_DAT = xfer->cmd_buf[i];
			I2CMT[i2c_dev]->CTL = (I2CMT[i2c_dev]->CTL & ~MASK) | REBUS;
			if (i2cmt_wait_complete(i2c_dev)) {
				return I2C_XFER_NO_TCB;
			}
		}
	}

	/* write mode */
	if (wr) {
		/* write data and wait for transfer complete */
		for (i = 0; i < xfer->dat_len; i ++) {
			I2CMT[i2c_dev]->NML_DAT = xfer->dat_buf[i];
			I2CMT[i2c_dev]->CTL = (I2CMT[i2c_dev]->CTL & ~MASK) | REBUS;
			if (i2cmt_wait_complete(i2c_dev)) {
				return I2C_XFER_NO_TCB;
			}
		}
	} else {
		/* read mode: restart after write cmd */
		if (xfer->cmd_len > 0) {
			sta_mode = RESTA;
		} else {
			sta_mode = START;
		}

		/* write device addr + r */
		I2CMT[i2c_dev]->NML_DAT = ((xfer->dev << 1) | 0x1);

		/* send start/restart and addr */
		I2CMT[i2c_dev]->CTL = (I2CMT[i2c_dev]->CTL & ~MASK) | sta_mode | REBUS;
		if (i2cmt_wait_complete(i2c_dev)) {
			return I2C_XFER_NO_TCB;
		}

		/* check bus error */
		if (I2CMT[i2c_dev]->NML_STA & I2CMT_NML_STA_BEB_Msk) {
			ret = I2C_XFER_BUS_ERR;
			goto _xfer_out;
		}

		/* check nack */
		if (!(I2CMT[i2c_dev]->NML_STA & I2CMT_NML_STA_RACK_Msk)) {
			ret = I2C_XFER_NACK;
			goto _xfer_out;
		}

		/* read data and wait for transfer complete */
		for (i = 0; i < xfer->dat_len; i ++) {
			/* ack/nack */
			if (i == xfer->dat_len - 1) {
				I2CMT[i2c_dev]->CTL = (I2CMT[i2c_dev]->CTL & ~MASK) | NACK | REBUS;
			} else {
				I2CMT[i2c_dev]->CTL = (I2CMT[i2c_dev]->CTL & ~MASK) | ACK | REBUS;
			}
			if (i2cmt_wait_complete(i2c_dev)) {
				return I2C_XFER_NO_TCB;
			}

			/* read data */
			xfer->dat_buf[i] = I2CMT[i2c_dev]->NML_DAT;
		}
	}

_xfer_out:
	/* send stop */
	I2CMT[i2c_dev]->CTL = (I2CMT[i2c_dev]->CTL & ~MASK) | STOP | REBUS;
	if (i2cmt_wait_stop(i2c_dev)) {
		ret = I2C_XFER_STOP_FAILED;
	}

	/* enable auto mode */
	i2cmt_auto_mode_set(i2c_dev);

	return ret;
}

static int i2cmt_acts_configure(const struct device *dev, uint32_t config)
{
	// code implement by user
	return 0;
}

static int i2cmt_acts_transfer(const struct device *dev, struct i2c_msg *msgs,
			uint8_t num_msgs, uint16_t addr)
{
	const struct acts_i2cmt_config *cfg = DEV_CFG(dev);
	struct acts_i2cmt_data *data =  DEV_DATA(dev);
	i2c_xfer_t xfer;
	uint8_t flags;
	int ret;

	if (!num_msgs)
		return 0;

	/* config xfer */
	xfer.dev = addr;
	if (num_msgs >= 2) {
		xfer.cmd_buf = msgs[0].buf;
		xfer.cmd_len = msgs[0].len;
		xfer.dat_buf = msgs[1].buf;
		xfer.dat_len = msgs[1].len;
		flags = msgs[1].flags;
	} else {
		xfer.cmd_buf = NULL;
		xfer.cmd_len = 0;
		xfer.dat_buf = msgs[0].buf;
		xfer.dat_len = msgs[0].len;
		flags = msgs[0].flags;
	}

	k_mutex_lock(&data->mutex, K_FOREVER);

	pm_device_busy_set(dev);

	if ((flags & I2C_MSG_RW_MASK) == I2C_MSG_READ) {
		ret = i2cmt_xfer(cfg->bus_id, &xfer, 0);
	} else {
		ret = i2cmt_xfer(cfg->bus_id, &xfer, 1);
	}

	pm_device_busy_clear(dev);

	k_mutex_unlock(&data->mutex);

	return ret;
}


static void i2cmt_acts_register_callback(struct device *dev, int task_id,
					i2c_task_callback_t cb, void *context)
{
	struct acts_i2cmt_data *data = DEV_DATA(dev);

	data->task_cb[task_id] = cb;
	data->task_cb_ctx[task_id] = context;
}

static uint8_t* i2cmt_task_buf_start(struct acts_i2cmt_data *data,
					int task_id, uint32_t addr, uint16_t len, uint8_t rd)
{
	rbuf_t *rbuf;
	uint8_t *sbuf;
	uint16_t slen;

	/* get task buf */
	sbuf = data->task_buf[task_id];

	/* check task buf len */
	if ((sbuf != NULL) && (len > data->task_len[task_id])) {
		/* free task buf */
		rbuf = RBUF_FR_BUF(sbuf);
		RB_MSG_FREE(rbuf);
		sbuf = NULL;
	}

	/* alloc task buf */
	if (sbuf == NULL) {
		slen = (len < 16) ? 16 : len;
		rbuf = RB_MSG_ALLOC(slen);
		sbuf = (uint8_t*)RBUF_TO_BUF(rbuf);
		data->task_buf[task_id] = sbuf;
		data->task_len[task_id] = slen;
	}

	/* copy buf before writing */
	if (!rd && (addr != 0)) {
		memcpy(sbuf, (void*)addr, len);
	}

	return sbuf;
}

static uint8_t* i2cmt_task_buf_stop(struct acts_i2cmt_data *data,
					int task_id, uint32_t addr, uint32_t len, uint8_t rd)
{
	uint8_t *sbuf;

	sbuf = data->task_buf[task_id];

	/* copy buf after reading */
	if (rd && (addr != 0)) {
		memcpy((void*)addr, sbuf, len);
	}

	return sbuf;
}

static int i2cmt_acts_task_start(struct device *dev, int task_id,
					const i2c_task_t *attr)
{
	const struct acts_i2cmt_config *cfg =DEV_CFG(dev);
	struct acts_i2cmt_data *data = DEV_DATA(dev);
	uint8_t *buf;

	/* start dma buffer */
	buf = i2cmt_task_buf_start(data, task_id, attr->dma.addr, attr->dma.len, attr->ctl.rwsel);

	/* save attr */
	data->task_attr[task_id] = (i2c_task_t*)attr;

	/* select i2c auto mode */
	i2cmt_auto_mode_set(cfg->bus_id);

	/* config i2c task */
	i2cmt_auto_task_config(cfg->bus_id, task_id, (i2c_task_t*)attr, (uint32_t)buf);

	/* config ppi */
	if (!attr->ctl.soft) {
		/* disable ppi trigger */
		ppi_trig_src_en(attr->trig.trig, 0);

		/* clear ppi pending */
		ppi_trig_src_clr_pending(attr->trig.trig);

		/* config ppi trigger */
		ppi_task_trig_config(attr->trig.chan, attr->trig.task, attr->trig.trig);

		/* enable ppi trigger */
		ppi_trig_src_en(attr->trig.trig, attr->trig.en);
		if (attr->trig.en) {
			if (attr->trig.trig <= TIMER4) {
				sys_write32(0x1, CMU_TIMERCLK(attr->trig.trig)); // select hosc/32=1M
				sys_write32(attr->trig.peri*1000, T_VAL(attr->trig.trig));
				sys_write32(0x24, T_CTL(attr->trig.trig));
			}
		}
	} else {
		/* soft trigger */
		i2cmt_auto_task_soft_start(cfg->bus_id, task_id);
	}

	return 0;
}

static int i2cmt_acts_task_stop(struct device *dev, int task_id)
{
	const struct acts_i2cmt_config *cfg =DEV_CFG(dev);
	struct acts_i2cmt_data *data = DEV_DATA(dev);
	const i2c_task_t *attr = data->task_attr[task_id];

	/* disable ppi trigger */
	if ((attr != NULL) && (!attr->ctl.soft)) {
		ppi_trig_src_en(attr->trig.trig, 0);
		if (attr->trig.trig <= TIMER4) {
			sys_write32(0x0, T_CTL(attr->trig.trig));
		}
	} else {
		i2cmt_auto_task_soft_stop(cfg->bus_id, task_id);
	}

	/* stop dma buffer */
	i2cmt_task_buf_stop(data, task_id, attr->dma.addr, attr->dma.len, attr->ctl.rwsel);

	/* clear attr */
	data->task_attr[task_id] = NULL;

	return 0;
}

static const unsigned short i2c_irq_list[4] = {
	I2C_TASK_IRQ_CMPLT,
	I2C_TASK_IRQ_HALF_CMPLT,
	I2C_TASK_IRQ_NACK,
	I2C_TASK_IRQ_BUS_ERROR,
};

static void i2cmt_acts_isr(struct device *dev)
{
	const struct acts_i2cmt_config *cfg = DEV_CFG(dev);;
	struct acts_i2cmt_data *data = DEV_DATA(dev);
	int task_id, irq_type, len;
	int pending = i2cmt_auto_task_irq_get_pending(cfg->bus_id);
	int pos = find_msb_set(pending) - 1;
	i2c_task_callback_t cb;
	const i2c_task_t *attr;
	uint8_t *buf;
	void *ctx;

	while (pos >= 0) {
		task_id = (pos % 4);
		irq_type = i2c_irq_list[pos / 4];
		attr = data->task_attr[task_id];

		/* clear task pending */
		i2cmt_auto_task_irq_clr_pending(cfg->bus_id, task_id, irq_type);

		/* clear ppi pending */
		ppi_trig_src_clr_pending(SPIMT0_TASK0_CIP + attr->trig.task);
		if (!attr->ctl.soft) {
			ppi_trig_src_clr_pending(attr->trig.trig);
			if (attr->trig.trig <= TIMER4) {
				//timer_clear_pd(attr->trig.trig);
			}
		}

		/* call handler */
		cb = data->task_cb[task_id];
		if (cb != NULL) {
			/* get buffer */
			ctx = data->task_cb_ctx[task_id];
			buf = data->task_buf[task_id];
			len = attr->dma.len / 2;
			switch(irq_type) {
				case I2C_TASK_IRQ_CMPLT:
					buf += len;
					break;
				case I2C_TASK_IRQ_HALF_CMPLT:
					break;
				case I2C_TASK_IRQ_NACK:
				case I2C_TASK_IRQ_BUS_ERROR:
					buf = NULL;
					break;
			}
			cb(buf, len, ctx);
		}

		/* find msb */
		pending = i2cmt_auto_task_irq_get_pending(cfg->bus_id);
		pos = find_msb_set(pending) - 1;
	}
}

__sleepfunc uint8_t* i2c_task_get_data(int bus_id, int task_id, int trig, int *plen)
{
	int len = 0;
	uint8_t *buf = NULL;
	int pending = i2cmt_auto_task_irq_get_pending(bus_id);

	/* clear task pending */
	if (pending & i2cmt_auto_task_irq_mask(task_id, I2C_TASK_IRQ_HALF_CMPLT)) {
		i2cmt_auto_task_irq_clr_pending(bus_id, task_id, I2C_TASK_IRQ_HALF_CMPLT);
		len = I2CMT[bus_id]->AUTO_TASK[task_id].DMA_CNT / 2;
		buf = (uint8_t *)I2CMT[bus_id]->AUTO_TASK[task_id].DMA_ADD;
	} else if (pending & i2cmt_auto_task_irq_mask(task_id, I2C_TASK_IRQ_CMPLT)) {
		i2cmt_auto_task_irq_clr_pending(bus_id, task_id, I2C_TASK_IRQ_CMPLT);
		len = I2CMT[bus_id]->AUTO_TASK[task_id].DMA_CNT / 2;
		buf = (uint8_t *)I2CMT[bus_id]->AUTO_TASK[task_id].DMA_ADD + len;
	}

	/* clear ppi pending */
	if (buf) {
		ppi_trig_src_clr_pending(I2CMT0_TASK0_CIP+task_id);
		if (trig >= 0) {
			ppi_trig_src_clr_pending(trig);
		}
	}

	if (plen) {
		*plen = len;
	}
	return buf;
}

int i2cmt_acts_init(const struct device *dev)
{
	const struct acts_i2cmt_config *cfg = DEV_CFG(dev);;
	struct acts_i2cmt_data *data = DEV_DATA(dev);

	// enable clock
	acts_clock_peripheral_enable(cfg->clock_id);

	// reset spimt
	acts_reset_peripheral(cfg->reset_id);

	/* enable i2c */
	I2CMT[cfg->bus_id]->CTL |= I2CMT_CTL_EN_Msk;

	// set clock
	i2cmt_set_rate(cfg->bus_id, cfg->clk_freq);

	/* irq init */
	cfg->irq_config_func();

	k_mutex_init(&data->mutex);

	return 0;
}

const struct i2cmt_driver_api i2cmt_acts_driver_api = {
	.i2c_api = {
		.configure = i2cmt_acts_configure,
		.transfer = i2cmt_acts_transfer,
	},
	.register_callback = i2cmt_acts_register_callback,
	.task_start = i2cmt_acts_task_start,
	.task_stop = i2cmt_acts_task_stop,
};

#define I2CMT_ACTS_DEFINE_CONFIG(n)					\
	static const struct device DEVICE_NAME_GET(i2cmt##n##_acts);		\
									\
	static void i2cmt##n##_acts_irq_config(void)			\
	{								\
		IRQ_CONNECT(IRQ_ID_IIC##n##MT, CONFIG_I2CMT_##n##_IRQ_PRI,	\
			    i2cmt_acts_isr,				\
			    DEVICE_GET(i2cmt##n##_acts), 0);		\
		irq_enable(IRQ_ID_IIC##n##MT);				\
	}								\
	static const struct acts_i2cmt_config i2cmt##n##_acts_config = {    \
		.ctl_reg = I2CMT##n##_REG_BASE,\
		.clk_freq = CONFIG_I2CMT_##n##_CLK_FREQ,		\
		.bus_id = n,\
		.clock_id = CLOCK_ID_I2CMT##n,\
		.reset_id = RESET_ID_I2CMT##n,\
		.irq_config_func = i2cmt##n##_acts_irq_config, \
	}

#define I2CMT_ACTS_DEFINE_DATA(n)						\
	static struct acts_i2cmt_data i2cmt##n##_acts_dev_data;

#define I2CMT_ACTS_DEVICE_INIT(n)					\
	I2CMT_ACTS_DEFINE_CONFIG(n);					\
	I2CMT_ACTS_DEFINE_DATA(n);						\
	DEVICE_DEFINE(i2cmt##n##_acts,				\
						CONFIG_I2CMT_##n##_NAME,					\
						&i2cmt_acts_init, NULL, &i2cmt##n##_acts_dev_data,	\
						&i2cmt##n##_acts_config, POST_KERNEL,		\
						CONFIG_I2C_INIT_PRIORITY, &i2cmt_acts_driver_api);

#if IS_ENABLED(CONFIG_I2CMT_0)
I2CMT_ACTS_DEVICE_INIT(0)
#endif
#if IS_ENABLED(CONFIG_I2CMT_1)
I2CMT_ACTS_DEVICE_INIT(1)
#endif
