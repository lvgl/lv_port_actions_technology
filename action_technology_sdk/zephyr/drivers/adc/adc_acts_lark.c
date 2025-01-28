/*
 * Copyright (c) 2021 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Actions PMU ADC implementation
 */

#include <kernel.h>
#include <device.h>
#include <string.h>
#include <errno.h>
#include <soc.h>
#include <soc_atp.h>
#include <board_cfg.h>
#include <sys/util.h>
#include <sys/byteorder.h>
#include <drivers/adc.h>
#include <logging/log.h>

LOG_MODULE_REGISTER(pmuadc0, CONFIG_ADC_LOG_LEVEL);

/***************************************************************************************************
 * PMUADC_CTL
 */
#define PMUADC_CTL_REG_SENSOR_EFUSE_SHIFT                     (26)
#define PMUADC_CTL_REG_SENSOR_EFUSE_MASK                      (0x3F << PMUADC_CTL_REG_SENSOR_EFUSE_SHIFT)
#define PMUADC_CTL_EN_TEST_SEN_CURR                           BIT(25)
#define PMUADC_CTL_PMUADC_EN                                  BIT(23)

#define PMUADC_CTL_REG_IBIAS_BUF_SHIFT                        (21)
#define PMUADC_CTL_REG_IBIAS_BUF_MASK                         (0x3 << PMUADC_CTL_REG_IBIAS_BUF_SHIFT)
#define PMUADC_CTL_REG_IBIAS_BUF(x)                           ((x) << PMUADC_CTL_REG_IBIAS_BUF_SHIFT)
#define PMUADC_CTL_REG_IBIAS_ADC_SHIFT                        (19)
#define PMUADC_CTL_REG_IBIAS_ADC_MASK                         (0x3 << PMUADC_CTL_REG_IBIAS_ADC_SHIFT)
#define PMUADC_CTL_REG_IBIAS_ADC(x)                           ((x) << PMUADC_CTL_REG_IBIAS_ADC_SHIFT)
#define PMUADC_CTL_TEST_SARAD                                 BIT(18)
#define PMUADC_CTL_LRADC7_SCAL                                BIT(17)
#define PMUADC_CTL_LRADC6_SCAL                                BIT(16)
#define PMUADC_CTL_LRADC5_SCAL                                BIT(15)
#define PMUADC_CTL_LRADC4_SCAL                                BIT(14)
#define PMUADC_CTL_LRADC3_SCAL                                BIT(13)

#define PMUADC_CTL_LRADC7_CHEN                                BIT(11)
#define PMUADC_CTL_LRADC6_CHEN                                BIT(10)
#define PMUADC_CTL_LRADC5_CHEN                                BIT(9)
#define PMUADC_CTL_LRADC4_CHEN                                BIT(8)
#define PMUADC_CTL_LRADC3_CHEN                                BIT(7)
#define PMUADC_CTL_LRADC2_CHEN                                BIT(6)
#define PMUADC_CTL_LRADC1_CHEN                                BIT(5)
#define PMUADC_CTL_SVCC_CHEN                                  BIT(4)
#define PMUADC_CTL_SENSOR_CHEN                                BIT(3)
#define PMUADC_CTL_DC5V_CHEN                                  BIT(2)
#define PMUADC_CTL_BATV_CHEN                                  BIT(1)
#define PMUADC_CTL_CHARGI_CHEN                                BIT(0)

/***************************************************************************************************
 * PMUADC_INTMASK
 */
#define PMUADC_INTMASK_LRADC7_INTEN                           BIT(11)
#define PMUADC_INTMASK_LRADC6_INTEN                           BIT(10)
#define PMUADC_INTMASK_LRADC5_INTEN                           BIT(9)
#define PMUADC_INTMASK_LRADC4_INTEN                           BIT(8)
#define PMUADC_INTMASK_LRADC3_INTEN                           BIT(7)
#define PMUADC_INTMASK_LRADC2_INTEN                           BIT(6)
#define PMUADC_INTMASK_LRADC1_INTEN                           BIT(5)
#define PMUADC_INTMASK_SVCC_INTEN                             BIT(4)
#define PMUADC_INTMASK_SENSOR_INTEN                           BIT(3)
#define PMUADC_INTMASK_DC5V_INTEN                             BIT(2)
#define PMUADC_INTMASK_BATV_INTEN                             BIT(1)
#define PMUADC_INTMASK_CHARGI_INTEN                           BIT(0)

/***************************************************************************************************
 * PMUADC_PD
 */
#define PMUADC_PD_LRADC7_PD                                   BIT(11)
#define PMUADC_PD_LRADC6_PD                                   BIT(10)
#define PMUADC_PD_LRADC5_PD                                   BIT(9)
#define PMUADC_PD_LRADC4_PD                                   BIT(8)
#define PMUADC_PD_LRADC3_PD                                   BIT(7)
#define PMUADC_PD_LRADC2_PD                                   BIT(6)
#define PMUADC_PD_LRADC1_PD                                   BIT(5)
#define PMUADC_PD_SVCC_PD                                     BIT(4)
#define PMUADC_PD_SENSOR_PD                                   BIT(3)
#define PMUADC_PD_DC5V_PD                                     BIT(2)
#define PMUADC_PD_BATV_PD                                     BIT(1)
#define PMUADC_PD_CHARGI_PD                                   BIT(0)

/***************************************************************************************************
 * CHARGI_DATA
 */
#define CHARGI_DATA_CHARGI_SHIFT                              (0)
#define CHARGI_DATA_CHARGI_MASK                               (0xFFF << CHARGI_DATA_CHARGI_SHIFT)

/***************************************************************************************************
 * BATADC_DATA
 */
#define BATADC_DATA_BATV_SHIFT                                (0)
#define BATADC_DATA_BATV_MASK                                 (0x3FFF << BATADC_DATA_BATV_SHIFT)

/***************************************************************************************************
 * DC5VADC_DATA
 */
#define DC5VADC_DATA_DC5V_SHIFT                               (0)
#define DC5VADC_DATA_DC5V_MASK                                (0xFFF << DC5VADC_DATA_DC5V_SHIFT)

/***************************************************************************************************
 * SENSADC_DATA
 */
#define SENSADC_DATA_SENSOR_SHIFT                             (0)
#define SENSADC_DATA_SENSOR_MASK                              (0xFFF << SENSADC_DATA_SENSOR_SHIFT)

/***************************************************************************************************
 * SVCCADC_DATA
 */
#define SVCCADC_DATA_SVCC_SHIFT                               (0)
#define SVCCADC_DATA_SVCC_MASK                                (0xFFF << SVCCADC_DATA_SVCC_SHIFT)

/***************************************************************************************************
 * LRADC_DATA
 */
#define LRADC_DATA_LRADC_SHIFT                                (0)
#define LRADC_DATA_LRADC_MASK                                 (0x3FFF << LRADC_DATA_LRADC_SHIFT)

/***************************************************************************************************
 * PMUADCDIG_CTL
 */
#define PMUADCDIG_CTL_LRADC7_AVG_SHIFT                        (22)
#define PMUADCDIG_CTL_LRADC7_AVG_MASK                         (0x3 << PMUADCDIG_CTL_LRADC7_AVG_SHIFT)
#define PMUADCDIG_CTL_LRADC7_AVG(x)                           ((x) << PMUADCDIG_CTL_LRADC7_AVG_SHIFT)
#define PMUADCDIG_CTL_LRADC6_AVG_SHIFT                        (20)
#define PMUADCDIG_CTL_LRADC6_AVG_MASK                         (0x3 << PMUADCDIG_CTL_LRADC6_AVG_SHIFT)
#define PMUADCDIG_CTL_LRADC6_AVG(x)                           ((x) << PMUADCDIG_CTL_LRADC6_AVG_SHIFT)
#define PMUADCDIG_CTL_LRADC5_AVG_SHIFT                        (18)
#define PMUADCDIG_CTL_LRADC5_AVG_MASK                         (0x3 << PMUADCDIG_CTL_LRADC5_AVG_SHIFT)
#define PMUADCDIG_CTL_LRADC5_AVG(x)                           ((x) << PMUADCDIG_CTL_LRADC5_AVG_SHIFT)
#define PMUADCDIG_CTL_LRADC4_AVG_SHIFT                        (16)
#define PMUADCDIG_CTL_LRADC4_AVG_MASK                         (0x3 << PMUADCDIG_CTL_LRADC4_AVG_SHIFT)
#define PMUADCDIG_CTL_LRADC4_AVG(x)                           ((x) << PMUADCDIG_CTL_LRADC4_AVG_SHIFT)
#define PMUADCDIG_CTL_LRADC3_AVG_SHIFT                        (14)
#define PMUADCDIG_CTL_LRADC3_AVG_MASK                         (0x3 << PMUADCDIG_CTL_LRADC3_AVG_SHIFT)
#define PMUADCDIG_CTL_LRADC3_AVG(x)                           ((x) << PMUADCDIG_CTL_LRADC3_AVG_SHIFT)
#define PMUADCDIG_CTL_LRADC2_AVG_SHIFT                        (12)
#define PMUADCDIG_CTL_LRADC2_AVG_MASK                         (0x3 << PMUADCDIG_CTL_LRADC2_AVG_SHIFT)
#define PMUADCDIG_CTL_LRADC2_AVG(x)                           ((x) << PMUADCDIG_CTL_LRADC2_AVG_SHIFT)
#define PMUADCDIG_CTL_LRADC1_AVG_SHIFT                        (10)
#define PMUADCDIG_CTL_LRADC1_AVG_MASK                         (0x3 << PMUADCDIG_CTL_LRADC1_AVG_SHIFT)
#define PMUADCDIG_CTL_LRADC1_AVG(x)                           ((x) << PMUADCDIG_CTL_LRADC1_AVG_SHIFT)
#define PMUADCDIG_CTL_BAT_AVG_SHIFT                           (8)
#define PMUADCDIG_CTL_BAT_AVG_MASK                            (0x3 << PMUADCDIG_CTL_BAT_AVG_SHIFT)
#define PMUADCDIG_CTL_BAT_AVG(x)                              ((x) << PMUADCDIG_CTL_BAT_AVG_SHIFT)
#define PMUADCDIG_CTL_ADCFIFOSEL                              BIT(7)
#define PMUADCDIG_CTL_ADC_DRQEN                               BIT(6)
#define PMUADCDIG_CTL_ADCFIFOLV_SHIFT                         (4)
#define PMUADCDIG_CTL_ADCFIFOLV_MASK                          (0x3 << PMUADCDIG_CTL_ADCFIFOLV_SHIFT)
#define PMUADCDIG_CTL_ADCFIFOLV(x)                            ((x) << PMUADCDIG_CTL_ADCFIFOLV_SHIFT)
#define PMUADCDIG_CTL_ADCFIFOEM                               BIT(3)
#define PMUADCDIG_CTL_ADCFIFOFU                               BIT(2)
#define PMUADCDIG_CTL_ADCFIFOER                               BIT(1)
#define PMUADCDIG_CTL_ADCFIFOEN                               BIT(0)

#define PMUADC_MAX_CHANNEL_ID                                 (11) 	/* MAX valid channel ID */
#define PMUADC_WAIT_TIMEOUT_MS                                (50) /* Timeout to wait for PMU ADC sampling */


#define PMUADC_WAIT_OVER_SAMPLE_US                            (3000)		//(1000)

#if (CONFIG_PMUADC_LRADC1_AVG == 0)
#define LRADCx_PENDING(ch, n) \
			if ((ch) & PMUADC_PD_LRADC##n##_PD) { \
				data->mdata.v.lradc##n##_voltage = pmuadc_reg->lradc##n##_data & LRADC_DATA_LRADC_MASK; \
				data->channels &= ~(BIT(n + 4)); \
				LOG_DBG("New LRADC%d voltage: 0x%x", n, data->mdata.v.lradc##n##_voltage); \
			}
#else
#define LRADCx_PENDING(ch, n) \
			if ((ch) & PMUADC_PD_LRADC##n##_PD) { \
				ret = pmuadc_wait_sample_complete(dev, ch); \
				if (ret) \
					return ret; \
				data->mdata.v.lradc##n##_voltage = pmuadc_reg->lradc##n##_data & LRADC_DATA_LRADC_MASK; \
				data->channels &= ~(BIT(n + 4)); \
				LOG_DBG("New LRADC%d voltage: 0x%x", n, data->mdata.v.lradc##n##_voltage); \
			}
#endif

#define LRADCx_PENDING_ALL(x) \
		{ \
			LRADCx_PENDING(x, 1); \
			LRADCx_PENDING(x, 2); \
			LRADCx_PENDING(x, 3); \
			LRADCx_PENDING(x, 4); \
			LRADCx_PENDING(x, 5); \
			LRADCx_PENDING(x, 6); \
			LRADCx_PENDING(x, 7); \
		}

/*
 * @struct acts_pmu_adc
 * @brief Actions PMU ADC controller hardware register
 */
struct acts_pmu_adc {
	volatile uint32_t ctl; /* PMUADC control */
	volatile uint32_t intmask; /* PMUADC sample interrupt set */
	volatile uint32_t pending; /* PMUADC sample pending */
	volatile uint32_t dig_ctl; /* PMUADC digital control */
	volatile uint32_t charge_data; /* Measure charging current */
	volatile uint32_t bat_data; /* Measure battery voltage */
	volatile uint32_t dc5v_data; /* Measure DC5V voltage */
	volatile uint32_t sensor_data; /* IC temperature sensor */
	volatile uint32_t svcc_data; /* Measure SVCC voltage */
	volatile uint32_t lradc1_data; /* Measure LRADC1 IO voltage */
	volatile uint32_t lradc2_data; /* Measure LRADC2 IO voltage */
	volatile uint32_t lradc3_data; /* Measure LRADC3 IO voltage */
	volatile uint32_t lradc4_data; /* Measure LRADC4 IO voltage */
	volatile uint32_t lradc5_data; /* Measure LRADC5 IO voltage */
	volatile uint32_t lradc6_data; /* Measure LRADC6 IO voltage */
	volatile uint32_t lradc7_data; /* Measure LRADC7 IO voltage */
	volatile uint32_t fifo_data; /* PMUADC FIFO data */
};

/**
 * struct pmuadc_measure_data
 * @brief The measure result data from PMU ADC sampling.
 */
union pmuadc_measure_data {
	uint16_t data[PMUADC_MAX_CHANNEL_ID + 1];
	struct {
		uint16_t charging_current;
		uint16_t battery_voltage;
		//uint16_t charging_current;
		uint16_t dc5v_voltage;
		uint16_t sensor_temperature;
		uint16_t svcc_voltage;
		uint16_t lradc1_voltage;
		uint16_t lradc2_voltage;
		uint16_t lradc3_voltage;
		uint16_t lradc4_voltage;
		uint16_t lradc5_voltage;
		uint16_t lradc6_voltage;
		uint16_t lradc7_voltage;
	} v;
};

/**
 * struct pmuadc_drv_data
 * @brief The meta data which related to Actions PMU ADC.
 */
struct pmuadc_drv_data {
	uint16_t channel_bitmap;
#ifndef CONFIG_ADC_ACTS_ALWAYS_ON
	struct k_sem completion; /* ADC sample synchronization completion semaphare */
#endif
	struct k_sem lock; /* ADC read lock */
	union pmuadc_measure_data mdata; /* measuared data */
	uint16_t channels; /* active channels */
	uint16_t sample_cnt; /* sample counter */
};

/**
 * struct pmuadc_config_data
 * @brief The hardware data that related to Actions PMU ADC
 */
struct pmuadc_config_data {
	uint32_t reg_base; /* PMU ADC controller register base address */
	uint8_t clk_id; /* LRADC devclk id */
	uint8_t clk_src; /* LRADC clock source */
	uint8_t clk_div; /* LRADC clock divisor */
	uint8_t debounce; /* PMU ADC debounce as the first sample data is not correct */
	void (*irq_config)(void); /* IRQ configuration function */
};

/* @brief get the base address of PMU ADC register */
static inline struct acts_pmu_adc *get_pmuadc_reg_base(const struct device *dev)
{
	const struct pmuadc_config_data *cfg = dev->config;
	return (struct acts_pmu_adc *)cfg->reg_base;
}

/* @brief dump pmu dac controller register */
void pmudac_dump_register(const struct device *dev)
{
	struct acts_pmu_adc *pmuadc_reg = get_pmuadc_reg_base(dev);

	LOG_INF("** pmudac contoller regster **");
	LOG_INF("         BASE: %08x", (uint32_t)pmuadc_reg);
	LOG_INF("          CTL: %08x", pmuadc_reg->ctl);
	LOG_INF("      INTMASK: %08x", pmuadc_reg->intmask);
	LOG_INF("      PENDING: %08x", pmuadc_reg->pending);
	LOG_INF("      DIG_CTL: %08x", pmuadc_reg->dig_ctl);
	LOG_INF("  CHARGE_DATA: %08x", pmuadc_reg->charge_data);
	LOG_INF("     BAT_DATA: %08x", pmuadc_reg->bat_data);
	LOG_INF("    DC5V_DATA: %08x", pmuadc_reg->dc5v_data);
	LOG_INF("  SENSOR_DATA: %08x", pmuadc_reg->sensor_data);
	LOG_INF("    SVCC_DATA: %08x", pmuadc_reg->svcc_data);
	LOG_INF("  LRADC1_DATA: %08x", pmuadc_reg->lradc1_data);
	LOG_INF("  LRADC2_DATA: %08x", pmuadc_reg->lradc2_data);
	LOG_INF("  LRADC3_DATA: %08x", pmuadc_reg->lradc3_data);
	LOG_INF("  LRADC4_DATA: %08x", pmuadc_reg->lradc4_data);
	LOG_INF("  LRADC5_DATA: %08x", pmuadc_reg->lradc5_data);
	LOG_INF("  LRADC6_DATA: %08x", pmuadc_reg->lradc6_data);
	LOG_INF("  LRADC7_DATA: %08x", pmuadc_reg->lradc7_data);
	LOG_INF("    FIFO_DATA: %08x", pmuadc_reg->fifo_data);
	LOG_INF(" CMU_LRADCCLK: %08x", sys_read32(CMU_LRADCCLK));
}

static s16_t adc_sensor_offset, adc_dc5v_offset, adc_bat_offset;

/* @brief PMU ADC channel lock */
static inline void pmuadc_lock(const struct device *dev)
{
	struct pmuadc_drv_data *data = dev->data;
	k_sem_take(&data->lock, K_FOREVER);
}

/* @brief PMU ADC channel unlock */
static inline void pmuadc_unlock(const struct device *dev)
{
	struct pmuadc_drv_data *data = dev->data;
	k_sem_give(&data->lock);
}

#ifndef CONFIG_ADC_ACTS_ALWAYS_ON
/* @brief wait for PMU ADC sample IRQ completion */
static inline int pmuadc_wait_for_completion(const struct device *dev)
{
	struct pmuadc_drv_data *data = dev->data;
	return k_sem_take(&data->completion, K_MSEC(PMUADC_WAIT_TIMEOUT_MS));
}

/* @brief signal that PMU ADC sample data completly */
static inline void pmuadc_complete(const struct device *dev)
{
	struct pmuadc_drv_data *data = dev->data;
	k_sem_give(&data->completion);
}
#endif

/* @brief check the buffer size */
static int check_buffer_size(const struct adc_sequence *sequence,
			     uint8_t active_channels)
{
	uint32_t needed_buffer_size;

	needed_buffer_size = active_channels * sizeof(uint16_t);

	if (sequence->buffer_size < needed_buffer_size) {
		LOG_ERR("Provided buffer is too small (%u/%u)",
				sequence->buffer_size, needed_buffer_size);
		return -ENOMEM;
	}

	return 0;
}

/* @brief validate the selected PMU ADC channels */
static int pmuadc_check_channels(const struct device *dev,
			const struct adc_sequence *sequence)
{
	struct pmuadc_drv_data *data = dev->data;
	uint32_t channels = sequence->channels;
	uint8_t i, active_channels = 0;

	if (!channels) {
		LOG_ERR("null channels");
		return -EINVAL;
	}

	for (i = 0; i <= PMUADC_MAX_CHANNEL_ID; i++) {
		if (channels & BIT(i)) {
			if (!(data->channel_bitmap & BIT(i))) {
				LOG_ERR("ADC channel@%d has not setuped yet", i);
				return -ENXIO;
			} else {
				++active_channels;
			}
		}
	}

	return check_buffer_size(sequence, active_channels);
}

/* @brief enable specified PMU ADC channels to sample data */
static int pmuadc_enable_channels(const struct device *dev, uint16_t channels)
{
	struct pmuadc_drv_data *data = dev->data;
	struct acts_pmu_adc *pmuadc_reg = get_pmuadc_reg_base(dev);
	uint8_t i;
	uint16_t ctl = 0;

	data->channels = channels;
	data->sample_cnt = 0;

	LOG_DBG("Active channels:0x%x", data->channels);

#ifdef CONFIG_ADC_ACTS_ALWAYS_ON
	/* If enable new channels, need to disable PMUADC and then enable */
	if (pmuadc_reg->ctl & PMUADC_CTL_PMUADC_EN) {
		pmuadc_reg->ctl &= ~PMUADC_CTL_PMUADC_EN;
		k_busy_wait(300);
	}
#endif

	/* enable PMU ADC channles */
	for (i = 0; i <= PMUADC_MAX_CHANNEL_ID; i++) {
		if (channels & BIT(i))
			ctl |= BIT(i);
	}

	pmuadc_reg->ctl |= ctl;

	/* enable PMU ADC function */
	pmuadc_reg->ctl |= PMUADC_CTL_PMUADC_EN;
	pmuadc_reg->pending = pmuadc_reg->pending;

#ifndef CONFIG_ADC_ACTS_ALWAYS_ON
	uint16_t intmask = 0;
	/* enable channels interrupts */
	for (i = 0; i <= PMUADC_MAX_CHANNEL_ID; i++) {
		if (channels & BIT(i)) {
				intmask |= BIT(i);
		}
	}
	pmuadc_reg->intmask = intmask;
#endif

	return 0;
}

/* @brief disable all PMU ADC channels */
static int pmuadc_disable_channels(const struct device *dev)
{
	struct acts_pmu_adc *pmuadc_reg = get_pmuadc_reg_base(dev);

	/* disable all ADC channels */
	pmuadc_reg->intmask = 0;
	pmuadc_reg->pending = pmuadc_reg->pending;
	pmuadc_reg->ctl &= ~(PMUADC_CTL_PMUADC_EN | 0x7FF);
	k_busy_wait(300);

	return 0;
}

#ifdef CONFIG_ADC_ACTS_ALWAYS_ON
static int pmuadc_wait_sample_complete(const struct device *dev, uint8_t ch_index)
{
	uint32_t timestamp = k_cycle_get_32();
	struct acts_pmu_adc *pmuadc_reg = get_pmuadc_reg_base(dev);

	while(!(ch_index & pmuadc_reg->pending)) {
		if (k_cyc_to_us_floor32(k_cycle_get_32() - timestamp)
			> PMUADC_WAIT_OVER_SAMPLE_US) {
			LOG_ERR("failed to get ADC channel:%d PD:0x%x CTL:0x%x", ch_index, pmuadc_reg->pending, pmuadc_reg->ctl);
			return -ETIMEDOUT;
		}
	}

	LOG_DBG("pmuadc wait channel:%d use %dus", ch_index,
			k_cyc_to_us_floor32(k_cycle_get_32() - timestamp));

	return 0;
}
#endif

/* @brief Implementation of the ADC driver API function: adc_channel_setup. */
static int pmuadc_channel_setup(const struct device *dev,
				  const struct adc_channel_cfg *channel_cfg)
{
	struct pmuadc_drv_data *data = dev->data;
	uint8_t channel_id = channel_cfg->channel_id;

	if (channel_id > PMUADC_MAX_CHANNEL_ID) {
		LOG_ERR("Invalid channel id %d", channel_id);
		return -EINVAL;
	}

	pmuadc_lock(dev);
	if (!(data->channel_bitmap & BIT(channel_id))) {
		data->channel_bitmap |= BIT(channel_id);
		LOG_INF("Enable PMU ADC channel@%d", channel_id);
#ifdef CONFIG_ADC_ACTS_ALWAYS_ON
		pmuadc_enable_channels(dev, BIT(channel_id));
#if (CONFIG_PMUADC_BAT_WAIT_AVG_COMPLETE == 0) && (CONFIG_PMUADC_BAT_AVG_CNT == 1)
		/* Ignore the first BAT Voltage sample data */
		if (channel_id == PMUADC_ID_BATV)
			pmuadc_wait_sample_complete(dev, BIT(PMUADC_ID_BATV));
#endif

		k_busy_wait(300);
#endif
	}
	pmuadc_unlock(dev);

	return 0;
}

static u16_t pmu_adc_cal(int32_t adcval, int32_t offset)
{
	adcval = adcval + offset;
	return (u16_t)adcval;
}
/* @brief start to read the PMU ADC measure data */
static int pmuadc_start_read(const struct device *dev,
			const struct adc_sequence *sequence)
{
	struct pmuadc_drv_data *data = dev->data;
	int ret = 0;
	uint32_t channels = sequence->channels;
	uint8_t i;

#ifdef CONFIG_ADC_ACTS_ALWAYS_ON
	struct acts_pmu_adc *pmuadc_reg = get_pmuadc_reg_base(dev);

	/* clear all channels pending */
	pmuadc_reg->pending = pmuadc_reg->pending;

	if (channels & PMUADC_PD_BATV_PD) {
#if (CONFIG_PMUADC_BAT_WAIT_AVG_COMPLETE == 1) && (CONFIG_PMUADC_BAT_AVG_CNT == 1)
		ret = pmuadc_wait_sample_complete(dev, BIT(PMUADC_ID_BATV));
		if (ret)
			return ret;
#endif
		data->mdata.v.battery_voltage = pmuadc_reg->bat_data & BATADC_DATA_BATV_MASK;
		data->mdata.v.battery_voltage = pmu_adc_cal(data->mdata.v.battery_voltage, adc_bat_offset);
	}

	if (channels & PMUADC_PD_CHARGI_PD)
		data->mdata.v.charging_current = pmuadc_reg->charge_data & CHARGI_DATA_CHARGI_MASK;

	if (channels & PMUADC_PD_DC5V_PD){
		data->mdata.v.dc5v_voltage = pmuadc_reg->dc5v_data & DC5VADC_DATA_DC5V_MASK;
		data->mdata.v.dc5v_voltage = pmu_adc_cal(data->mdata.v.dc5v_voltage, adc_dc5v_offset);
	}

	if (channels & PMUADC_PD_SENSOR_PD){
		data->mdata.v.sensor_temperature = pmuadc_reg->sensor_data & SENSADC_DATA_SENSOR_MASK;
		data->mdata.v.sensor_temperature = pmu_adc_cal(data->mdata.v.sensor_temperature, adc_sensor_offset);
	}

	if (channels & PMUADC_PD_SVCC_PD)
		data->mdata.v.svcc_voltage = pmuadc_reg->svcc_data & SVCCADC_DATA_SVCC_MASK;

	LRADCx_PENDING_ALL(channels);

#else
	//uint32_t timestamp = k_cycle_get_32();

	ret = pmuadc_enable_channels(dev, channels);
	if (ret)
		return ret;

	ret = pmuadc_wait_for_completion(dev);

	/*LOG_DBG("pmuadc wait channel:%d use %dus", channels,
		k_cyc_to_us_floor32(k_cycle_get_32() - timestamp));*/

#endif /* CONFIG_ADC_ACTS_ALWAYS_ON */

	if (!ret) {
		for (i = 0; i <= PMUADC_MAX_CHANNEL_ID; i++) {
			if (channels & BIT(i)) {
				sys_put_le16(data->mdata.data[i], sequence->buffer);
			}
		}
	}

#ifndef CONFIG_ADC_ACTS_ALWAYS_ON
	pmuadc_disable_channels(dev);
#endif

	return ret;
}

/* @brief Implementation of the ADC driver API function: adc_read. */
static int pmuadc_read(const struct device *dev,
			 const struct adc_sequence *sequence)
{
	int ret;

	ret = pmuadc_check_channels(dev, sequence);
	if (ret)
		return ret;

#ifndef CONFIG_ADC_ACTS_ALWAYS_ON
	pmuadc_lock(dev);
#endif

	ret = pmuadc_start_read(dev, sequence);

#ifndef CONFIG_ADC_ACTS_ALWAYS_ON
	pmuadc_unlock(dev);
#endif

	return ret;
}

#ifdef CONFIG_ADC_ASYNC
/* @brief Implementation of the ADC driver API function: adc_read_sync. */
static int pmuadc_read_async(const struct device *dev,
			       const struct adc_sequence *sequence,
			       struct k_poll_signal *async)
{
	return -ENOTSUP;
}
#endif /* CONFIG_ADC_ASYNC */

static const struct adc_driver_api pmuadc_driver_api = {
	.channel_setup = pmuadc_channel_setup,
	.read          = pmuadc_read,
#ifdef CONFIG_ADC_ASYNC
	.read_async    = pmuadc_read_async,
#endif
};

#ifndef CONFIG_ADC_ACTS_ALWAYS_ON
static void pmuadc_isr(void *arg)
{
	struct device *dev = (struct device *)arg;
	struct pmuadc_drv_data *data = dev->data;
	const struct pmuadc_config_data *cfg = dev->config;
	struct acts_pmu_adc *pmuadc_reg = get_pmuadc_reg_base(dev);
	uint32_t pending = pmuadc_reg->pending;
	uint32_t val;

	LOG_DBG("ctl:0x%x mask:0x%x pending:0x%x channels:0x%x",
			pmuadc_reg->ctl, pmuadc_reg->intmask, pending, data->channels);

	if (pending & PMUADC_PD_CHARGI_PD) {
		val = pmuadc_reg->charge_data & CHARGI_DATA_CHARGI_MASK;
		data->mdata.v.charging_current = val;
		data->channels &= ~BIT(0);
		LOG_DBG("New charging current: 0x%x", val);
	}

	if (pending & PMUADC_PD_BATV_PD) {
		val = pmuadc_reg->bat_data & BATADC_DATA_BATV_MASK;
		data->mdata.v.battery_voltage = pmu_adc_cal(val, adc_bat_offset);
		data->channels &= ~BIT(1);
		LOG_DBG("New battery voltage: 0x%x", val);
	}

	if (pending & PMUADC_PD_DC5V_PD) {
		val = pmuadc_reg->dc5v_data & DC5VADC_DATA_DC5V_MASK;
		data->mdata.v.dc5v_voltage = pmu_adc_cal(val, adc_dc5v_offset);
		data->channels &= ~BIT(2);
		LOG_DBG("New DC5V voltage: 0x%x", val);
	}

	if (pending & PMUADC_PD_SENSOR_PD) {
		val = pmuadc_reg->sensor_data & SENSADC_DATA_SENSOR_MASK;
		data->mdata.v.sensor_temperature = pmu_adc_cal(val, adc_sensor_offset);;
		data->channels &= ~BIT(3);
		LOG_DBG("New sensor temperature: 0x%x", val);
	}

	if (pending & PMUADC_PD_SVCC_PD) {
		val = pmuadc_reg->svcc_data & SVCCADC_DATA_SVCC_MASK;
		data->mdata.v.svcc_voltage = val;
		data->channels &= ~BIT(4);
		LOG_DBG("New SVCC voltage: 0x%x", val);
	}

	LRADCx_PENDING_ALL(pending);

	pmuadc_reg->pending = pending;

	if ((!data->channels) && (++data->sample_cnt > cfg->debounce)) {
		LOG_DBG("complete");
		pmuadc_reg->intmask = 0;
		pmuadc_complete(dev);
	}
}
#endif

/* @brief set the LRADC clock source and divisor */
static int pmuadc_clk_source_set(const struct device *dev)
{
	const struct pmuadc_config_data *cfg = dev->config;

	sys_write32(((cfg->clk_src & 0x7) << 4) | ((cfg->clk_div & 0x3)), CMU_LRADCCLK);
	LOG_DBG("LRADCCLK:0x%08x", sys_read32(CMU_LRADCCLK));

	return 0;
}

/* @brief ADC core current bias setting */
static int pmuadc_bias_setting(const struct device *dev)
{
	struct acts_pmu_adc *pmuadc_reg = get_pmuadc_reg_base(dev);

	uint32_t reg = pmuadc_reg->ctl;

	reg &= ~PMUADC_CTL_REG_IBIAS_BUF_MASK;
	reg |= PMUADC_CTL_REG_IBIAS_BUF(CONFIG_PMUADC_IBIAS_BUF_SEL);

	reg &= ~PMUADC_CTL_REG_IBIAS_ADC_MASK;
	reg |= PMUADC_CTL_REG_IBIAS_ADC(CONFIG_PMUADC_IBIAS_ADC_SEL);

	pmuadc_reg->ctl = reg;

	return 0;
}

/* @brief ADC channels over sampling times setting */
static int pmuadc_digital_setting(const struct device *dev)
{
	struct acts_pmu_adc *pmuadc_reg = get_pmuadc_reg_base(dev);
	uint32_t reg = pmuadc_reg->dig_ctl;

	reg &= ~PMUADCDIG_CTL_BAT_AVG_MASK;
	reg |= PMUADCDIG_CTL_BAT_AVG(CONFIG_PMUADC_BAT_AVG_CNT);

	reg &= ~PMUADCDIG_CTL_LRADC1_AVG_MASK;
	reg |= PMUADCDIG_CTL_LRADC1_AVG(CONFIG_PMUADC_LRADC1_AVG);

	pmuadc_reg->dig_ctl = reg;

	return 0;
}


static void adc_cal_init(void)
{
	unsigned int offset;
	adc_bat_offset = 0;
	adc_sensor_offset = 0;
	adc_dc5v_offset = 0;
	if (!soc_atp_get_pmu_calib(4, &offset)){
		LOG_DBG("get batadc cal=0x%x\n", offset);
		if(offset & 0x10) //转换成adc 校准值
			adc_bat_offset = -(0x20-offset)*16;
		else
			adc_bat_offset = offset*16;
	}

	if (!soc_atp_get_pmu_calib(5, &offset)){ //offset校准按摄氏度，补码 单位0.1度
		LOG_DBG("get sensoradc cal=0x%x\n", offset);
		if(offset & 0x10)
			adc_sensor_offset = -(0x20-offset)*10;
		else
			adc_sensor_offset = offset*10;
		/*把校准值转成adc 值*/
		adc_sensor_offset = -adc_sensor_offset*1000/1745;
	}

	if (!soc_atp_get_pmu_calib(7, &offset)){
		LOG_DBG("get dv5v cal=0x%x\n", offset);
		if(offset & 0x10)
			adc_dc5v_offset = -(0x20-offset)*4;
		else
			adc_dc5v_offset = offset*4;
	}
	LOG_DBG("adc:bat=%d,sensor=%d,dc5v=%d\n", adc_bat_offset, adc_sensor_offset, adc_dc5v_offset);

}


/* @brief PMU ADC initialization */
static int pmuadc_init(const struct device *dev)
{
	const struct pmuadc_config_data *cfg = dev->config;
	struct pmuadc_drv_data *data = dev->data;
	struct acts_pmu_adc *pmuadc_reg = get_pmuadc_reg_base(dev);

	adc_cal_init();
	/* configure the LRADC clock source */
	pmuadc_clk_source_set(dev);

	acts_clock_peripheral_enable(cfg->clk_id);

	/* disable all ADC channels */
	pmuadc_reg->intmask = 0;
	pmuadc_reg->pending = pmuadc_reg->pending;
	pmuadc_reg->ctl &= ~0x3FFFFFF;

	pmuadc_bias_setting(dev);

	pmuadc_digital_setting(dev);

	k_busy_wait(300);

#ifndef CONFIG_ADC_ACTS_ALWAYS_ON
	k_sem_init(&data->completion, 0, 1);
#endif

	k_sem_init(&data->lock, 1, 1);

	if (cfg->irq_config)
		cfg->irq_config();

	return 0;
}


#ifdef CONFIG_PM_DEVICE
int adc_pm_control(const struct device *device, enum pm_device_action action)
{
	int ret;
	const struct pmuadc_config_data *cfg = device->config;

#ifdef CONFIG_ADC_ACTS_ALWAYS_ON
	static uint32_t pmuadc_ctl_bak;
	struct acts_pmu_adc *pmuadc_reg = get_pmuadc_reg_base(device);
#endif

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		LOG_INF("adc wakeup\n");
		acts_clock_peripheral_enable(cfg->clk_id);
#ifdef CONFIG_ADC_ACTS_ALWAYS_ON
		pmuadc_reg->ctl = (pmuadc_ctl_bak & ~PMUADC_CTL_PMUADC_EN);
		LOG_DBG("%d PMUADC_CTL:0x%x", __LINE__, pmuadc_reg->ctl);
		k_busy_wait(300);
		pmuadc_reg->ctl |= PMUADC_CTL_PMUADC_EN;
		LOG_DBG("%d PMUADC_CTL:0x%x", __LINE__, pmuadc_reg->ctl);
		k_busy_wait(300);
#else
		irq_enable(IRQ_ID_LRADC);
#endif
		break;
	case PM_DEVICE_ACTION_SUSPEND:
		LOG_INF("adc sleep\n");
#ifdef CONFIG_ADC_ACTS_ALWAYS_ON
		pmuadc_ctl_bak = pmuadc_reg->ctl;
		pmuadc_disable_channels(device);
		acts_clock_peripheral_disable(cfg->clk_id);
#else
		irq_disable(IRQ_ID_LRADC);
#endif
		break;
	case PM_DEVICE_ACTION_EARLY_SUSPEND:

		break;
	case PM_DEVICE_ACTION_LATE_RESUME:

		break;
	default:
		ret = -EINVAL;

	}
	return 0;
}
#else
#define adc_pm_control 	NULL
#endif

#ifndef CONFIG_ADC_ACTS_ALWAYS_ON
static void pmuadc_irq_config(void);
#endif

/* pmu adc driver data */
static struct pmuadc_drv_data pmuadc_drv_data0;

/* pmu adc config data */
static const struct pmuadc_config_data pmuadc_config_data0 = {
	.reg_base = PMUADC_CTL,
	.clk_id = CLOCK_ID_LRADC,
	.clk_src = CONFIG_PMUADC_CLOCK_SOURCE,
	.clk_div = CONFIG_PMUADC_CLOCK_DIV,
#ifndef CONFIG_ADC_ACTS_ALWAYS_ON
	.debounce = CONFIG_PMUADC_DEBOUNCE,
	.irq_config = pmuadc_irq_config,
#endif
};

DEVICE_DEFINE(pmuadc0, CONFIG_PMUADC_NAME, pmuadc_init, adc_pm_control,
	    &pmuadc_drv_data0, &pmuadc_config_data0,
	    PRE_KERNEL_2, CONFIG_KERNEL_INIT_PRIORITY_OBJECTS, &pmuadc_driver_api);

#ifndef CONFIG_ADC_ACTS_ALWAYS_ON
static void pmuadc_irq_config(void)
{
	IRQ_CONNECT(IRQ_ID_LRADC, CONFIG_PMUADC_IRQ_PRI,
			pmuadc_isr,
			DEVICE_GET(pmuadc0), 0);
	irq_enable(IRQ_ID_LRADC);
}
#endif

