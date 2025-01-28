/*
 * Copyright (c) 2018 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief PWM controller driver for Actions SoC
 */

#include <errno.h>
#include <sys/__assert.h>
#include <stdbool.h>
#include <kernel.h>
#include <device.h>
#include <init.h>
#include <drivers/pwm.h>
#include <soc.h>
#include <drivers/dma.h>
#include <errno.h>
#include <board.h>


#define LOG_LEVEL CONFIG_LOG_PWM_DEV_LEVEL

#include <logging/log.h>

LOG_MODULE_REGISTER(pwm);

#define CMU_PWMCLK_CLKSEL_SHIFT		(9)
#define CMU_PWMCLK_CLKSEL_MASK		(0x3 << CMU_PWMCLK_CLKSEL_SHIFT)
#define CMU_PWMCLK_CLK_SEL(x)		((x) << CMU_PWMCLK_CLKSEL_SHIFT)
#define CMU_PWMCLK_CLKSEL_16K		CMU_PWMCLK_CLK_SEL(0)
#define CMU_PWMCLK_CLKSEL_HOSC		CMU_PWMCLK_CLK_SEL(1)
#define CMU_PWMCLK_CLKSEL_CK64M		CMU_PWMCLK_CLK_SEL(2)
#define CMU_PWMCLK_CLKSEL_32K       CMU_PWMCLK_CLK_SEL(0)

#define CMU_PWMCLK_CLKDIV_SHIFT		(0)
#define CMU_PWMCLK_CLKDIV(x)		((x) << CMU_PWMCLK_CLKDIV_SHIFT)
#define CMU_PWMCLK_CLKDIV_MASK		CMU_PWMCLK_CLKDIV(0x1FF)

#define PWM_CTRL_MODE_SEL_SHIFT		(0)
#define PWM_CTRL_MODE_SEL_MASK		(0x3 << PWM_CTRL_MODE_SEL_SHIFT)
#define PWM_CTRL_MODE_SEL(x)		((x) << PWM_CTRL_MODE_SEL_SHIFT)
#define PWM_CTRL_MODE_SEL_FIXED		PWM_CTRL_MODE_SEL(0)
#define PWM_CTRL_MODE_SEL_BREATH	PWM_CTRL_MODE_SEL(1)
#define PWM_CTRL_MODE_SEL_PROGRAM	PWM_CTRL_MODE_SEL(2)
#define PWM_CTRL_POL_SEL_HIGH		BIT(2)
#define PWM_CTRL_CHANNEL_EN			BIT(3)
#define PWM_CTRL_CHANNEL_START		BIT(4)

#define PWM_BREATH_MODE_C_SHIFT		(0)
#define PWM_BREATH_MODE_C_MASK		(0xFF << PWM_BREATH_MODE_C_SHIFT)
#define PWM_BREATH_MODE_C(x)		((x) << PWM_BREATH_MODE_C_SHIFT)

#define PWM_BREATH_MODE_QU_SHIFT	(0)
#define PWM_BREATH_MODE_QU_MASK		(0xFF << PWM_BREATH_MODE_QU_SHIFT)
#define PWM_BREATH_MODE_QU(x)		((x) << PWM_BREATH_MODE_QU_SHIFT)
#define PWM_BREATH_MODE_QD_SHIFT	(16)
#define PWM_BREATH_MODE_QD_MASK		(0xFF << PWM_BREATH_MODE_QD_SHIFT)
#define PWM_BREATH_MODE_QD(x)		((x) << PWM_BREATH_MODE_QD_SHIFT)

#define PWM_BREATH_MODE_H_SHIFT		(0)
#define PWM_BREATH_MODE_H_MASK		(0xFFFF << PWM_BREATH_MODE_H_SHIFT)
#define PWM_BREATH_MODE_H(x)		((x) << PWM_BREATH_MODE_H_SHIFT)

#define PWM_BREATH_MODE_L_SHIFT		(0)
#define PWM_BREATH_MODE_L_MASK		(0xFFFF << PWM_BREATH_MODE_L_SHIFT)
#define PWM_BREATH_MODE_L(x)		((x) << PWM_BREATH_MODE_L_SHIFT)

#define PWM_DUTYMAX_SHIFT			(0)
#define PWM_DUTYMAX_MASK			(0xFFFF << PWM_DUTYMAX_SHIFT)
#define PWM_DUTYMAX(x)				((x) << PWM_DUTYMAX_SHIFT)

#define PWM_DUTY_SHIFT				(0)
#define PWM_DUTY_MASK				(0xFFFF << PWM_DUTY_SHIFT)
#define PWM_DUTY(x)					((x) << PWM_DUTY_SHIFT)

#define PWM_DMA_CTL_START			BIT(0)
#define PWM_FIFO_CLK_SEL_SHIFT		(4)
#define PWM_FIFO_CLK_SEL_MASK		(0xF << PWM_FIFO_CLK_SEL_SHIFT)
#define PWM_FIFO_CLK_SEL(x)			((x) << PWM_FIFO_CLK_SEL_SHIFT)

#define PWM_FIFOSTA_ERROR			BIT(0)
#define PWM_FIFOSTA_FULL			BIT(1)
#define PWM_FIFOSTA_EMPTY			BIT(2)
#define PWM_FIFOSTA_LEVEL_SHIFT		(3)
#define PWM_FIFOSTA_LEVEL_MASK		(0x3 << PWM_FIFOSTA_LEVEL_SHIFT)

#ifdef CHIP_LEOPARD
#define PWM_CLK_CYCLES_PER_SEC		(32000000)
#else
#define PWM_CLK_CYCLES_PER_SEC		(16000)
#endif
#define PWM_PIN_CYCLES_PER_SEC		(8000)
#define PWM_PIN_CLK_PERIOD_USEC		(1000000UL / PWM_PIN_CYCLES_PER_SEC)
#define PWM_DUTYMAX_DEFAULT			(16000) /* 1s / PWM_PIN_CYCLES_PER_SEC x PWM_DUTYMAX_DEFAULT = 2s */

#define PWM_PROGRAM_PIN_INVALID		(0xFF)

#define PWM_BREATH_MODE_DEFAULT_C	(32)

#define PWM_DMACTL_REG_OFFSET		(0x0F00)
#define PWM_DMACTL_REG(x)			((struct acts_pwm_dmactl *)((x)->base + PWM_DMACTL_REG_OFFSET))

#define PWM_CHAN(base, chan_id)		((struct acts_pwm_chan *)((base) + (chan_id * 0x100)))

#define PWM_CYCLES_MAX				(0xFFFF)

#define DMA_IRQ_TC					(0) /* DMA completion flag */
#define DMA_IRQ_HF					(1) /* DMA half-full flag */

#define PWM_MFP						(0x12 | (0x1 << 5) | ((3) << GPIO_CTL_PADDRV_SHIFT))

#define LOW_DIV_FACTOR              1
#define HIG_DIV_FACTOR              2
#define LOW_DIV_MAX_VAL             256
#define BOARD_RC32K_FREQ            32000
#define BOARD_HOSC_FREQ             32000000
#define BOARD_RC64M_FREQ            64000000

enum {
	/**start voltage level low */
	START_VOLTAGE_LOW = 0,
	/**start voltage level high */
	START_VOLTAGE_HIGH,
};


/* pwm control registers */
struct acts_pwm_chan {
	volatile u32_t ctrl;
	volatile u32_t c;
	volatile u32_t q;
	volatile u32_t h;
	volatile u32_t l;
	volatile u32_t duty_max;
	volatile u32_t duty;
};

/* pwm dma control registers */
struct acts_pwm_dmactl {
	volatile u32_t dmactl;
	volatile u32_t fifodat;
	volatile u32_t fifosta;
};

struct pwm_acts_data {
	struct k_mutex mutex;
	const struct device *dma_dev;
	int dma_chan;
	int (*program_callback)(void *cb_data, u8_t reason);
	void *cb_data;
	u8_t program_pin;
	struct acts_pin_config pins_pwm_mfp[9];
};



struct pwm_acts_config {
	u32_t	base;
	u32_t	pwmclk_reg;
	u32_t	cycle;
	u32_t	chans;
#if CHIP_LEOPARD
	u8_t	clock0_id;
	u8_t	clock1_id;
	u8_t	clock2_id;
	u8_t	clock3_id;
#else
	u8_t	clock_id;
#endif
	u8_t	reset_id;
	const char *dma_dev_name;
	u8_t txdma_id;
	u8_t flag_use_dma;
};

struct pwm_acts_chans{
	u8_t chan;
	u8_t gpio_num;
};

/*
 *  Each line represents a set of pwm channels {0, 3}, {0, 4}, {0, 14}, {0, 36}, {0, 49}
 *  The first numeric value of each set represents the pwm channel, and the second numeric value represents the pin number
 *       For example: 0 represents pwm channel 0 3 represents GPIO3
 *  The pwm driver scans the form to confirm if a channel has a gpio available
 */
#ifdef CHIP_LEOPARD
#define CONFIG_PWM_PIN_CHAN_MAP		{0, 3}, {0, 14}, {0, 21}, {0, 49}, {0, 53}, {0, 64},\
									{1, 15}, {1, 18}, {1, 22}, {1, 50}, {1, 54}, {1, 57}, {1, 74},\
									{2, 6}, {2, 16}, {2, 19}, {2, 51}, {2, 55}, {2, 75},\
									{3, 7}, {3, 17}, {3, 20}, {3, 52}, {3, 56}, {3, 76}
#else
#define CONFIG_PWM_PIN_CHAN_MAP		{0, 3}, {0, 4}, {0, 14}, {0, 36}, {0, 49},\
									{1, 5}, {1, 15}, {1, 37}, {1, 50},\
									{2, 6}, {2, 21}, {2, 38}, {2, 51},\
									{3, 7}, {3, 17}, {3, 39}, {3, 52},\
									{4, 8}, {4, 18}, {4, 40}, {4, 53},\
									{5, 9}, {5, 19}, {5, 41}, {5, 54},\
									{6, 10}, {6, 20}, {6, 42}, {6, 55},\
									{7, 11}, {7, 21}, {7, 43}, {7, 45}, {7, 56},\
									{8, 12}, {8, 22}, {8, 44}, {8, 46}, {8, 57}
#endif

static const struct pwm_acts_chans pwm_acts_chan_gpio_num[] = {CONFIG_PWM_PIN_CHAN_MAP};

/*
 * Convert frequency and duty to clkdiv/dutymax/duty of pwm register.
 *
 * Input Parameters
 * clk_src_freq: clock source frequency [Hz]
 * pwm_freq: PWM channel to set
 * duty: duty
 * Output Parameters
 * reg_div_index: set value of PWMxCLKDIV in CMU_PWMxCLK register
 * reg_dutymax: set value of PWMx_DUTYMAX register
 * reg_duty: set value of PWMx_DUTY register
 * return 0, or negative errno code
 */
static int convert_freq2pwmreg(uint32_t clk_src_freq, uint32_t pwm_freq,
                                    uint16_t duty, uint16_t *reg_div_index,
                                    uint16_t *reg_dutymax, uint16_t *reg_duty)
{
	uint32_t i;
	uint16_t div = 0;
	uint32_t pwm_clk;
	uint32_t dutymax;
	uint8_t found_presc_flag = 0;
	static const uint16_t high_presc_values[6] =
	{256, 512, 1024, 2048, 4096, 8192};

	if (clk_src_freq < pwm_freq) {
		LOG_ERR("Undesirable input pwm setting frequency\n");
		return -ESPIPE;
	}

	/* Calculate optimal values of the pwmclk prescaler and dutymax for
	* the pwm register.
	* Example:
	*
	*  clk_src_freq = 24 MHz
	*  pwm_freq     = 100 Hz
	*
	*  div          = 24,000,000 / 65,535 / 100
	*               = 3.6 (or 4 -- taking the ceiling always)
	*  pwmclk       = 24,000,000 / 4
	*               = 6,000,000
	*  dutymax      = 6,000,000 / 100
	*               = 60,000
	*/

	div = (clk_src_freq / pwm_freq + 65534) / 65535;

	for (i = 0; i < LOW_DIV_MAX_VAL; i++) {
		if (div <= (i + 1)) {
			found_presc_flag = LOW_DIV_FACTOR;
			break;
		}
	}

	if (found_presc_flag == 0) {
		for (i = 0; i < 6; i++) {
			if (div <= high_presc_values[i]) {
				found_presc_flag = HIG_DIV_FACTOR;
				break;
			}
		}
	}

	if (found_presc_flag == LOW_DIV_FACTOR) {
		div = i + 1;
		*reg_div_index = div - 1;
	} else if (found_presc_flag == HIG_DIV_FACTOR) {
		div = high_presc_values[i];
		*reg_div_index = ((LOW_DIV_MAX_VAL - 1) + i);
	} else {
		LOG_ERR("cannot find proper prescale\n");
		return -EACCES;
	}

	pwm_clk = clk_src_freq / div;

	dutymax = pwm_clk / pwm_freq;
	if (dutymax < 1)
		dutymax = 1;
	else if (dutymax > 65535)
 		dutymax = 65535;

	*reg_dutymax = dutymax;
	*reg_duty = dutymax * duty / 100;

	LOG_DBG("div=%d, dutymax=%d, duty=%d", *reg_div_index, *reg_dutymax, *reg_duty);

	return 0;
}

int pwm_acts_mfp(unsigned int pin, unsigned int mode, bool set)
{
	unsigned int key, ctl_reg, val;

	if (pin >= GPIO_MAX_PIN_NUM)
		return -EINVAL;

	ctl_reg = GPIO_REG_CTL(GPIO_REG_BASE, pin);

	key = irq_lock();

	if(set)
		val = (sys_read32(ctl_reg) & ~PINMUX_MODE_MASK) | mode;
	else
		val = (sys_read32(ctl_reg) & ~PINMUX_MODE_MASK);

	sys_write32(val, ctl_reg);

	irq_unlock(key);

	return 0;
}

static u16_t pwm_acts_check_chan(u32_t pwm, u32_t pin)
{
	for(int i = 0; i < sizeof(pwm_acts_chan_gpio_num)/sizeof(struct pwm_acts_chans); i++)
		if(pwm == pwm_acts_chan_gpio_num[i].chan && pin == pwm_acts_chan_gpio_num[i].gpio_num)
			return 0;

	return -EINVAL;
}

static int pwm_acts_set_pinmux(const struct device *dev, u32_t chan)
{
	//const struct pwm_acts_config *cfg = dev->config;
	struct pwm_acts_data *data = dev->data;

	if(data->pins_pwm_mfp[chan].pin_num == 0xffffffff) {
		LOG_ERR("this pin is not initialized");
		return -1;
	}

	pwm_acts_mfp(data->pins_pwm_mfp[chan].pin_num, data->pins_pwm_mfp[chan].mode, true);


	return 0;
}

static void pwm_acts_dma_reload(const struct device *dev, void *user_data,
					   uint32_t channel, int status)
{
	u32_t _reason;
	int ret;
	struct pwm_acts_data *data = (struct pwm_acts_data *)user_data;

	if (status == DMA_IRQ_HF) {
		_reason = PWM_PROGRAM_DMA_IRQ_HF;
	} else if (status == DMA_IRQ_TC) {
		_reason = PWM_PROGRAM_DMA_IRQ_TC;
	} else {
		LOG_ERR("Unknown DMA reason %d", status);
		dma_stop(dev, data->dma_chan);
		return ;
	}

	ret = data->program_callback(data->cb_data, _reason);
	if (ret < 0)
		dma_stop(dev, data->dma_chan);
}

static void pwm_acts_dma_direct(const struct device *dev, void *user_data,
					   uint32_t channel, int status)
{
	struct pwm_acts_data *data = (struct pwm_acts_data *)user_data;
	ARG_UNUSED(dev);

	if ((status == DMA_IRQ_TC) && data->program_callback) {
		data->program_callback(data->cb_data, PWM_PROGRAM_DMA_IRQ_TC);
	}
}


int pwm_acts_dma_prepare(struct device *dev, u32_t chan, pwm_program_ctrl_t *ctrl)
{
	const struct pwm_acts_config *cfg = dev->config;
	struct pwm_acts_data *data = dev->data;
	struct acts_pwm_dmactl *dmactrl = PWM_DMACTL_REG(cfg);
	struct dma_config dma_cfg = {0};
	struct dma_block_config dma_block_cfg = {0};

	/* request dma channel handle */
	if (data->dma_chan == -1) {
		data->dma_chan = dma_request(data->dma_dev, 0xFF);
		if (!data->dma_chan) {
			LOG_ERR("Failed to request dma channel");
			return -ENXIO;
		}
	}

	dma_cfg.channel_direction = MEMORY_TO_PERIPHERAL;
	dma_cfg.source_burst_length = 8;
	dma_cfg.dest_burst_length = 8;
	dma_cfg.source_data_size = 2;
	dma_cfg.dma_slot = cfg->txdma_id;
	dma_block_cfg.source_address = (u32_t)ctrl->ram_buf;
	dma_block_cfg.dest_address = (u32_t)&dmactrl->fifodat;

	dma_block_cfg.block_size = ctrl->ram_buf_len;
	dma_block_cfg.source_reload_en = ctrl->reload_en;
	dma_cfg.block_count = 1;
	dma_cfg.head_block = &dma_block_cfg;

	if (ctrl->program_callback) {
		if (ctrl->reload_en)
			dma_cfg.dma_callback = pwm_acts_dma_reload;
		else
			dma_cfg.dma_callback = pwm_acts_dma_direct;

		dma_cfg.user_data = data;
		dma_cfg.complete_callback_en = 1;
	//	dma_cfg.half_complete_callback_en = 1;

		data->program_callback = ctrl->program_callback;
		data->cb_data = ctrl->cb_data;
	}

	if (dma_config(data->dma_dev, data->dma_chan, &dma_cfg)) {
		LOG_ERR("DMA config error");
		return -EFAULT;
	}

	return 0;
}

/*
 * Set the period and pulse width for a PWM pin.
 *
 * Parameters
 * dev: Pointer to PWM device structure
 * pwm: PWM channel to set
 * period_cycles: Period (in timer count)
 * pulse_cycles: Pulse width (in timer count).
 * @param flags Flags for pin configuration (polarity).
 * return 0, or negative errno code
 */
static int pwm_acts_pin_set(const struct device *dev, u32_t chan,
			     u32_t period_cycles, u32_t pulse_cycles, pwm_flags_t flags)
{
	const struct pwm_acts_config *cfg = dev->config;
	//struct pwm_acts_data *data = dev->data;
	struct acts_pwm_chan *pwm_chan;
	uint32_t key;

	LOG_INF("PWM@%d set period cycles %d, pulse cycles %d",
		chan, period_cycles, pulse_cycles);

	if (chan >= cfg->chans) {
		LOG_ERR("invalid chan %d", chan);
		return -EINVAL;
	}

	if (pulse_cycles > period_cycles) {
		LOG_ERR("pulse cycles %d is biger than period's %d",
			pulse_cycles, period_cycles);
		return -EINVAL;
	}

	if (period_cycles > PWM_CYCLES_MAX || period_cycles < 1) {
		LOG_ERR("period cycles invalid %d (max %d min 1)",
			period_cycles, PWM_CYCLES_MAX);
		return -EINVAL;
	}

	if (pwm_acts_set_pinmux(dev, chan)) {
		LOG_DBG("set pwm pinmux error");
		return -EFAULT;
	}

	key = irq_lock();

	pwm_chan = PWM_CHAN(cfg->base, chan);

	LOG_DBG("pwm_chan %p", pwm_chan);

	/* disable pwm */
	pwm_chan->ctrl = 0;

	/* setup pwm parameters */
	if (START_VOLTAGE_HIGH == flags)
		pwm_chan->ctrl = PWM_CTRL_POL_SEL_HIGH | PWM_CTRL_MODE_SEL_FIXED;
	else
		pwm_chan->ctrl = PWM_CTRL_MODE_SEL_FIXED;

	/* PWM period = PWM_CLK period x PWM_DUTYMAX */
	pwm_chan->duty_max = PWM_DUTYMAX(period_cycles);

	/* PWM  DUTY = DUTY / PWM_DUTYMAX */
	pwm_chan->duty = pulse_cycles * (pwm_chan->duty_max & PWM_DUTYMAX_MASK) / period_cycles;

	/* enable pwm */
	pwm_chan->ctrl |= (PWM_CTRL_CHANNEL_EN | PWM_CTRL_CHANNEL_START);

	irq_unlock(key);

	return 0;
}

/*
 * Set the frequency and duty for a PWM channel.
 *
 * Parameters
 * dev: Pointer to PWM device structure
 * pwm: PWM channel to set
 * freq: frequency of pwm.
 * duty: duty cycle from 0~100.
 * @param flags Flags for pin configuration (polarity).
 * return 0, or negative errno code
 */
static int pwm_acts_freq_duty_set(const struct device *dev, u32_t chan,
			     u32_t freq, u8_t duty, pwm_flags_t flags)
{
	const struct pwm_acts_config *cfg = dev->config;
	struct acts_pwm_chan *pwm_chan;
	uint16_t reg_div_index;
	uint16_t reg_dutymax;
	uint16_t reg_duty;
	uint32_t reg_pwmclk = 0;
	uint32_t key;
	int ret;

	LOG_DBG("PWM@%d set frequency %d, duty %%%d", chan, freq, duty);

	if (chan >= cfg->chans) {
		LOG_ERR("invalid chan %d", chan);
		return -EINVAL;
	}

	if (duty > 100) {
		LOG_ERR("duty cycle is not valid");
		return -EINVAL;
	}

	if (pwm_acts_set_pinmux(dev, chan)) {
		LOG_ERR("set pwm pinmux error");
		return -EFAULT;
	}

	ret = convert_freq2pwmreg(PWM_CLK_CYCLES_PER_SEC, freq, duty,
                           &reg_div_index, &reg_dutymax, &reg_duty);
	if (ret < 0)
		return -EINVAL;

	key = irq_lock();

	switch (PWM_CLK_CYCLES_PER_SEC) {
	case BOARD_RC64M_FREQ:
		reg_pwmclk = CMU_PWMCLK_CLKSEL_CK64M;
		break;

	case BOARD_RC32K_FREQ:
		reg_pwmclk = CMU_PWMCLK_CLKSEL_32K;
		break;

	case BOARD_HOSC_FREQ:
		reg_pwmclk = CMU_PWMCLK_CLKSEL_HOSC;
		break;

	default:
		reg_pwmclk = CMU_PWMCLK_CLKSEL_HOSC;
		break;
	}

	sys_write32(reg_pwmclk | CMU_PWMCLK_CLKDIV(reg_div_index),
                PWM_CLK0_BASE + 4 * chan);

	pwm_chan = PWM_CHAN(cfg->base, chan);

	/* disable pwm */
	pwm_chan->ctrl = 0;

	/* setup pwm parameters */
	if (START_VOLTAGE_HIGH == flags)
		pwm_chan->ctrl = PWM_CTRL_POL_SEL_HIGH | PWM_CTRL_MODE_SEL_FIXED;
	else
		pwm_chan->ctrl = PWM_CTRL_MODE_SEL_FIXED;

	pwm_chan->duty_max = reg_dutymax;

	pwm_chan->duty = reg_duty;

	/* enable pwm */
	pwm_chan->ctrl |= (PWM_CTRL_CHANNEL_EN | PWM_CTRL_CHANNEL_START);

	irq_unlock(key);

	return 0;
}

/*
 * Get the clock rate (cycles per second) for a PWM pin.
 *
 * Parameters
 * dev: Pointer to PWM device structure
 * pwm: PWM port number
 * cycles: Pointer to the memory to store clock rate (cycles per second)
 *
 * return 0, or negative errno code
 */
static int pwm_acts_get_cycles_per_sec(const struct device *dev, u32_t chan,
					u64_t *cycles)
{
	const struct pwm_acts_config *cfg = dev->config;

	if (chan >= cfg->chans) {
		LOG_ERR("invalid pwm chan %d", chan);
		return -EINVAL;
	}

	*cycles = (u64_t)cfg->cycle;

	return 0;
}

static int pwm_acts_set_breath_mode(const struct device *dev, u32_t chan, pwm_breath_ctrl_t *ctrl)
{
	const struct pwm_acts_config *cfg = dev->config;
	struct pwm_acts_data *data = dev->data;
	struct acts_pwm_chan *pwm_chan;
	u32_t period = PWM_PIN_CLK_PERIOD_USEC, qd, qu, high, low;
	pwm_breath_ctrl_t breath_ctrl = {0};

	if (!ctrl) {
		breath_ctrl.rise_time_ms = PWM_BREATH_RISE_TIME_DEFAULT;
		breath_ctrl.down_time_ms = PWM_BREATH_DOWN_TIME_DEFAULT;
		breath_ctrl.high_time_ms = PWM_BREATH_HIGH_TIME_DEFAULT;
		breath_ctrl.low_time_ms = PWM_BREATH_LOW_TIME_DEFAULT;
	} else {
		memcpy(&breath_ctrl, ctrl, sizeof(pwm_breath_ctrl_t));
	}

	LOG_INF("PWM@%d rise %dms, down %dms, high %dms, low %dms",
		chan, breath_ctrl.rise_time_ms, breath_ctrl.down_time_ms,
		breath_ctrl.high_time_ms, breath_ctrl.low_time_ms);

	if (chan >= cfg->chans) {
		LOG_ERR("invalid chan %d", chan);
		return -EINVAL;
	}

	if (pwm_acts_set_pinmux(dev, chan)) {
		LOG_DBG("set pwm pinmux error");
		return -EFAULT;
	}

	k_mutex_lock(&data->mutex, K_FOREVER);

	pwm_chan = PWM_CHAN(cfg->base, chan);

	/* disable pwm */
	pwm_chan->ctrl = 0;

	/* setup pwm parameters */
	pwm_chan->ctrl = PWM_CTRL_POL_SEL_HIGH | PWM_CTRL_MODE_SEL_BREATH;

	/* rise time T1 = QU x C x C x t; C=32, t=PWM_PIN_CLK_PERIOD_USEC */
	qu = breath_ctrl.rise_time_ms * 1000 / PWM_BREATH_MODE_DEFAULT_C / PWM_BREATH_MODE_DEFAULT_C;
	qu = (qu + period - 1) / period*3/2;

	/* down time T2 = QD x C x C x t; C=32, t=PWM_PIN_CLK_PERIOD_USEC */
	qd = breath_ctrl.down_time_ms * 1000 / PWM_BREATH_MODE_DEFAULT_C / PWM_BREATH_MODE_DEFAULT_C;
	qd = (qd + period - 1) / period*3/2;

	/* high level time T3 = H x C x t; C=32, t = PWM_PIN_CLK_PERIOD_USEC */
	high = breath_ctrl.high_time_ms * 1000 / PWM_BREATH_MODE_DEFAULT_C;
	high = (high + period - 1) / period*3/2;

	/* high level time T3 = L x C x t; C=32, t = PWM_PIN_CLK_PERIOD_USEC */
	low = breath_ctrl.low_time_ms * 1000 / PWM_BREATH_MODE_DEFAULT_C;
	low = (low + period - 1) / period*3/2;

	LOG_INF("QU:%d QD:%d high:%d low:%d", qu, qd, high, low);

	pwm_chan->c = PWM_BREATH_MODE_C(PWM_BREATH_MODE_DEFAULT_C);
	pwm_chan->q = PWM_BREATH_MODE_QU(qu) | PWM_BREATH_MODE_QD(qd);
	pwm_chan->h = PWM_BREATH_MODE_H(high);
	pwm_chan->l = PWM_BREATH_MODE_L(low);

	/* enable pwm */
	pwm_chan->ctrl |= (PWM_CTRL_CHANNEL_EN | PWM_CTRL_CHANNEL_START);

	k_mutex_unlock(&data->mutex);

	return 0;
}

static int pwm_acts_pin_stop(const struct device *dev, u32_t chan)
{
	const struct pwm_acts_config *cfg = dev->config;
	//struct pwm_acts_data *data = dev->data;
	struct acts_pwm_chan *pwm_chan;

	if (chan >= cfg->chans) {
		LOG_ERR("invalid pwm chan %d", chan);
		return -EINVAL;
	}

	pwm_chan = PWM_CHAN(cfg->base, chan);

	/* disable pwm */
	pwm_chan->ctrl = 0;

	LOG_INF("PWM@%d pin stop", chan);

	return 0;
}

static int pwm_acts_pin_mfp_set(const struct device *dev, u32_t pwm, u32_t pin)
{
	//const struct pwm_acts_config *cfg = dev->config;
	struct pwm_acts_data *data = dev->data;

	if(pwm_acts_check_chan(pwm, pin)) {
		LOG_ERR("invalid pwm %d: %d", pwm, pin);
		return -1;
	}

	if(data->pins_pwm_mfp[pwm].pin_num != 0)//this chan has been used
		pwm_acts_mfp(data->pins_pwm_mfp[pwm].pin_num, 0, false);

	data->pins_pwm_mfp[pwm].pin_num = pin;
	data->pins_pwm_mfp[pwm].mode = PWM_MFP;

	LOG_INF("PWM@%d pin mfp set", pin);

	return 0;
}

int pwm_acts_init(const struct device *dev)
{
	const struct pwm_acts_config *cfg = dev->config;
	struct pwm_acts_data *data = dev->data;
	int i, chan;
#ifdef CHIP_LEOPARD
	u16_t reg_div;
#else
	u8_t clk_div;
#endif
	struct board_pwm_pinmux_info pinmux_info;

	board_get_pwm_pinmux_info(&pinmux_info);

	memset(data->pins_pwm_mfp, 0xff, sizeof(data->pins_pwm_mfp));
	for (i = 0; i < pinmux_info.pins_num; i++) {
		chan = pinmux_info.pins_config[i].pin_chan;
		data->pins_pwm_mfp[chan].pin_num = pinmux_info.pins_config[i].pin_num;
		data->pins_pwm_mfp[chan].mode = pinmux_info.pins_config[i].mode;
	}

#ifdef CHIP_LEOPARD
	/* enable pwm controller clock */
	acts_clock_peripheral_enable(cfg->clock0_id);
	acts_clock_peripheral_enable(cfg->clock1_id);
	acts_clock_peripheral_enable(cfg->clock2_id);
	acts_clock_peripheral_enable(cfg->clock3_id);
#else
	/* enable pwm controller clock */
	acts_clock_peripheral_enable(cfg->clock_id);
#endif

	/* reset pwm controller */
	acts_reset_peripheral(cfg->reset_id);

#ifdef CHIP_LEOPARD
	/* clock source: 32Mhz, pwm clock fs 8KHz period 125us */
	reg_div = PWM_CLK_CYCLES_PER_SEC / PWM_PIN_CYCLES_PER_SEC;

	/* Segmental calculate reg val according div factor */
	if (reg_div < 256) {
		reg_div -= 1;
	} else if ((reg_div >= 256) && (reg_div < 384)) {/* div: 256 */
		reg_div = 255;
	} else if ((reg_div >= 384) && (reg_div < 768)) {/* div: 512 */
		reg_div = 256;
	} else if ((reg_div >= 768) && (reg_div < 1536)) {/* div: 1024 */
		reg_div = 257;
	} else if ((reg_div >= 1536) && (reg_div < 3072)) {/* div: 2048 */
		reg_div = 258;
	} else if ((reg_div >= 3072) && (reg_div < 6144)) {/* div: 4096 */
		reg_div = 259;
	} else if (reg_div >= 6144) {/* div: 8192 */
		reg_div = 260;
	}

	/* init PWM clock */
	for (i = 0; i < cfg->chans; i++)
		sys_write32(CMU_PWMCLK_CLKSEL_HOSC | CMU_PWMCLK_CLKDIV(reg_div), \
                    cfg->pwmclk_reg + 4 * i);
#else
	/* clock source: 16K, div= / 2, pwm clock fs 8KHz period 125us */
	clk_div = (PWM_CLK_CYCLES_PER_SEC / PWM_PIN_CYCLES_PER_SEC) - 1;

	/* init PWM clock */
	for (i = 0; i < cfg->chans; i++) {
		sys_write32(CMU_PWMCLK_CLKSEL_16K | CMU_PWMCLK_CLKDIV(clk_div), \
			cfg->pwmclk_reg + 4 * i);
	}
#endif

	k_mutex_init(&data->mutex);

	if (cfg->flag_use_dma) {

		data->dma_dev = device_get_binding(cfg->dma_dev_name);
		if (!data->dma_dev) {
			LOG_ERR("Bind DMA device %s error", cfg->dma_dev_name);
			return -ENOENT;
		}
		data->program_pin = PWM_PROGRAM_PIN_INVALID;
		data->dma_chan = dma_request(data->dma_dev, 0xff);
		if(data->dma_chan < 0){
			LOG_ERR("dma-dev rxchan config err chan=%d\n", data->dma_chan);
			return -ENODEV;
		}
	}

	//sys_write32(sys_read32(0x40068000)|(1<<16), 0x40068000);

	return 0;
}
const struct pwm_driver_api pwm_acts_driver_api = {
		.pin_set = pwm_acts_pin_set,
		.get_cycles_per_sec = pwm_acts_get_cycles_per_sec,
		.set_breath = pwm_acts_set_breath_mode,
		.pin_mfp = pwm_acts_pin_mfp_set,
	//	.set_program = pwm_acts_set_program_mode,
		.pin_stop = pwm_acts_pin_stop,
		.pin_set_freq_duty = pwm_acts_freq_duty_set,
};

static struct pwm_acts_data pwm_acts_data;



static const struct pwm_acts_config pwm_acts_config = {
	.base = (uint32_t)PWM_REG_BASE,
	.pwmclk_reg = PWM_CLK0_BASE,
	.cycle = CONFIG_PWM_CYCLE,
	.chans = CONFIG_PWM_CHANS,
#if CHIP_LEOPARD
	.clock0_id = CLOCK_ID_PWM0,
	.clock1_id = CLOCK_ID_PWM1,
	.clock2_id = CLOCK_ID_PWM2,
	.clock3_id = CLOCK_ID_PWM3,
#else
	.clock_id = CLOCK_ID_PWM,
#endif
	.reset_id = RESET_ID_PWM,
	.dma_dev_name = CONFIG_DMA_0_NAME,
	.txdma_id = CONFIG_PWM_DMA_CHAN,
	.flag_use_dma = CONFIG_PWM_USE_DMA,
};

#if IS_ENABLED(CONFIG_PWM)
DEVICE_DEFINE(pwm_acts, CONFIG_PWM_NAME,
		    pwm_acts_init, NULL,
		    &pwm_acts_data, &pwm_acts_config,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &pwm_acts_driver_api);
#endif

