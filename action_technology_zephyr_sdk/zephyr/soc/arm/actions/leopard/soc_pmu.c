/*
 * Copyright (c) 2021 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Actions LEOPARD family PMUVDD and PMUSVCC Implementation
 */

#include <kernel.h>
#include <device.h>
#include <string.h>
#include <errno.h>
#include <soc.h>
#include <board_cfg.h>
#include <soc_pmu.h>
#include <logging/log.h>
#include <drivers/adc.h>
#include <sys/byteorder.h>
#include <soc_atp.h>
#include <dvfs.h>
#include <linker/linker-defs.h>

LOG_MODULE_REGISTER(pmu0, CONFIG_LOG_DEFAULT_LEVEL);

#define PMU_INTMASK_COUNTER8HZ_INTEN                     BIT(14)
#define PMU_INTMASK_REMOTE_INTEN                         BIT(12)
#define PMU_INTMASK_BATLV_INTEN                          BIT(11)
#define PMU_INTMASK_DC5VLV_INTEN                         BIT(10)
#define PMU_INTMASK_WIO1LV_INTEN                         BIT(7)
#define PMU_INTMASK_WIO0LV_INTEN                         BIT(6)
#define PMU_INTMASK_ALARM8HZ_INTEN                       BIT(4)
#define PMU_INTMASK_DC5VOUT_INTEN                        BIT(3)
#define PMU_INTMASK_DC5VIN_INTEN                         BIT(2)
#define PMU_INTMASK_ONOFF_S_INTEN                        BIT(1)
#define PMU_INTMASK_ONOFF_L_INTEN                        BIT(0)

#define WAKE_CTL_SVCC_BATLV_VOL_SHIFT                    (27) /* Battery voltage level wakeup threshold */
#define WAKE_CTL_SVCC_BATLV_VOL_MASK                     (0x7 << WAKE_CTL_SVCC_BATLV_VOL_SHIFT)
#define WAKE_CTL_SVCC_DC5VLV_VOL_SHIFT                   (24) /* DC5V voltage level wakeup threshold */
#define WAKE_CTL_SVCC_DC5VLV_VOL_MASK                    (0x7 << WAKE_CTL_SVCC_DC5VLV_VOL_SHIFT)
#define WAKE_CTL_SVCC_WIO1LV_VOL_SHIFT                   (22)
#define WAKE_CTL_SVCC_WIO1LV_VOL_MASK                    (0x3 << WAKE_CTL_SVCC_WIO1LV_VOL_SHIFT)
#define WAKE_CTL_SVCC_WIO0LV_VOL_SHIFT                   (20)
#define WAKE_CTL_SVCC_WIO0LV_VOL_MASK                    (0x3 << WAKE_CTL_SVCC_WIO0LV_VOL_SHIFT)
#define WAKE_CTL_SVCC_REMOTE_WKEN                        BIT(12)
#define WAKE_CTL_SVCC_BATLV_WKEN                         BIT(11)
#define WAKE_CTL_SVCC_DC5VLV_WKEN                        BIT(10)
#define WAKE_CTL_SVCC_WIO1LV_DETEN                       BIT(9)
#define WAKE_CTL_SVCC_WIO0LV_DETEN                       BIT(8)
#define WAKE_CTL_SVCC_WIO1LV_WKEN                        BIT(7)
#define WAKE_CTL_SVCC_WIO0LV_WKEN                        BIT(6)
#define WAKE_CTL_SVCC_WIO_WKEN                           BIT(5)
#define WAKE_CTL_SVCC_ALARM8HZ_WKEN                      BIT(4)
#define WAKE_CTL_SVCC_DC5VOUT_WKEN                       BIT(3)
#define WAKE_CTL_SVCC_DC5VIN_WKEN                        BIT(2)
#define WAKE_CTL_SVCC_SHORT_WKEN                         BIT(1)
#define WAKE_CTL_SVCC_LONG_WKEN                          BIT(0)

#define WAKE_PD_SVCC_REG_SVCC_SHIFT                      (24)
#define WAKE_PD_SVCC_REG_SVCC_MASK                       (0xFF << WAKE_PD_SVCC_REG_SVCC_SHIFT)
#define WAKE_PD_SVCC_SYSRESET_PD                         BIT(20)
#define WAKE_PD_SVCC_POWEROK_PD                          BIT(19)
#define WAKE_PD_SVCC_LB_PD                               BIT(18)
#define WAKE_PD_SVCC_OC_PD                               BIT(17)
#define WAKE_PD_SVCC_LVPRO_PD                            BIT(16)
#define WAKE_PD_TK_PD                                    BIT(15)
#define WAKE_PD_SVCC_BATWK_PD                            BIT(13)
#define WAKE_PD_SVCC_REMOTE_PD                           BIT(12)
#define WAKE_PD_SVCC_BATLV_PD                            BIT(11)
#define WAKE_PD_SVCC_DC5VLV_PD                           BIT(10)
#define WAKE_PD_SVCC_WIO1LV_PD                           BIT(7)
#define WAKE_PD_SVCC_WIO0LV_PD                           BIT(6)
#define WAKE_PD_SVCC_WIO_PD                              BIT(5)
#define WAKE_PD_SVCC_ALARM8HZ_PD                         BIT(4)
#define WAKE_PD_SVCC_DC5VOUT_PD                          BIT(3)
#define WAKE_PD_SVCC_DC5VIN_PD                           BIT(2)
#define WAKE_PD_SVCC_ONOFF_S_PD                          BIT(1)
#define WAKE_PD_SVCC_ONOFF_L_PD                          BIT(0)

#define SYSTEM_SET_SVCC_ONOFF_SEL_SHIFT                  (25)
#define SYSTEM_SET_SVCC_ONOFF_SEL_MASK                   (0x3 << SYSTEM_SET_SVCC_ONOFF_SEL_SHIFT)
#define SYSTEM_SET_SVCC_SIM_A_EN                         BIT(24)
#define SYSTEM_SET_SVCC_ONOFF_PRESS_TIME_SHIFT           (21)
#define SYSTEM_SET_SVCC_ONOFF_PRESS_TIME_MASK            (0x7 << SYSTEM_SET_SVCC_ONOFF_PRESS_TIME_SHIFT)
#define SYSTEM_SET_SVCC_ONOFF_PRESS_TIME(x)              ((x) << SYSTEM_SET_SVCC_ONOFF_PRESS_TIME_SHIFT)
#define SYSTEM_SET_SVCC_ONOFF_RST_EN_SHIFT               (19)
#define SYSTEM_SET_SVCC_ONOFF_RST_EN_MASK                (0x3 << SYSTEM_SET_SVCC_ONOFF_RST_EN_SHIFT)
#define SYSTEM_SET_SVCC_ONOFF_RST_EN(x)                  ((x) << SYSTEM_SET_SVCC_ONOFF_RST_EN_SHIFT)
#define SYSTEM_SET_SVCC_ONOFF_RST_TIME                   BIT(18)
#define SYSTEM_SET_SVCC_ONOFF_PRES                       BIT(17)

#define CHG_CTL_SVCC_CV_3V3                              BIT(19)
#define CHG_CTL_SVCC_CV_OFFSET_SHIFT                     (5)
#define CHG_CTL_SVCC_CV_OFFSET_MASK                      (0x1F << CHG_CTL_SVCC_CV_OFFSET_SHIFT)
#define CHG_CTL_SVCC_CV_OFFSET(x)                        ((x) << CHG_CTL_SVCC_CV_OFFSET_SHIFT)

#define PMU_DET_COUNTR8HZ_PD                             BIT(12)
#define PMU_DET_DC5VIN_DET                               BIT(0)

#define VOUT_CTL0_VC18_SW                                BIT(18)
#define VOUT_CTL0_VD12_SW                                BIT(17)
#define VOUT_CTL0_VD12_25X_BITI                          1


#define REG_SENSOR_EFUSE_SHIFT                           (26)
#define REG_SENSOR_EFUSE_MASK                            (0x3F << REG_SENSOR_EFUSE_SHIFT)

#define PMU_COUNTER8HZ_SVCC_CLK_SEL_SHIFT                (27)
#define PMU_COUNTER8HZ_SVCC_CLK_SEL_MASK                 (1 << PMU_COUNTER8HZ_SVCC_CLK_SEL_SHIFT)
#define PMU_COUNTER8HZ_SVCC_CLK_SEL_LOSC                 BIT(27)
#define PMU_COUNTER8HZ_SVCC_COUNTER_EN                   BIT(26)
#define PMU_COUNTER8HZ_SVCC_COUNTER_VAL                  (0x3FFFFFF)

#define PMU_ALARM8HZ_SVCC_PD                             BIT(26)
#define PMU_ALARM8HZ_SVCC_ALARM_VAL_MASK                 (0x3FFFFFF)

#define PMU_MONITOR_DEV_INVALID(x) (((x) != PMU_DETECT_DEV_DC5V) \
			&& ((x) != PMU_DETECT_DEV_REMOTE) \
			&& ((x) != PMU_DETECT_DEV_ONOFF) \
			&& ((x) != PMU_DETECT_DEV_COUNTER8HZ) \
			&& ((x) != PMU_DETECT_DEV_ALARM8HZ) \
            && ((x) != PMU_DETECT_DEV_BAT))

/*
 * @struct acts_pmuvdd
 * @brief Actions PMUVDD controller hardware register
 */
struct acts_pmuvdd {
	volatile uint32_t vout_ctl0;
	volatile uint32_t vout_ctl1_s1;
	volatile uint32_t vout_ctl1_s2;
	volatile uint32_t vout_ctl1_s3;
	volatile uint32_t pmu_det;
	volatile uint32_t reserved0[3];
	volatile uint32_t dcdc_vc18_ctl;
	volatile uint32_t dcdc_vd12_ctl;
	volatile uint32_t dcdc_vdd_ctl;
	volatile uint32_t reserved1;
	volatile uint32_t pwrgate_dig;
	volatile uint32_t pwrgate_dig_ack;
	volatile uint32_t pwrgate_ram;
	volatile uint32_t pwrgate_ram_ack;
	volatile uint32_t pmu_intmask;
};

/*
 * @struct acts_pmusvcc
 * @brief Actions PMUSVCC controller hardware register
 */
struct acts_pmusvcc {
	volatile uint32_t chg_ctl_svcc;
	volatile uint32_t bdg_ctl_svcc;
	volatile uint32_t system_set_svcc;
	volatile uint32_t power_ctl_svcc;
	volatile uint32_t wake_ctl_svcc;
	volatile uint32_t wake_pd_svcc;
	volatile uint32_t counter8hz_svcc;
	volatile uint32_t alarm8hz_svcc;
};

/**
 * struct pmu_drv_data
 * @brief The meta data which related to Actions PMU module.
 */
struct pmu_context_t {
	struct detect_param_t detect_devs[PMU_DETECT_MAX_DEV]; /* PMU monitor peripheral devices */
	uint32_t pmuvdd_base; /* PMUVDD register base address */
	uint32_t pmusvcc_base; /* PMUSVCC register base address */
	uint8_t onoff_short_detect : 1; /* if 1 to enable onoff key short press detection */
	uint8_t onoff_remote_same_wio : 1; /* if 1 to indicate that onoff key and remote key use the same WIO source */
};

/* @brief get the PMU management context handler */
static inline struct pmu_context_t *get_pmu_context(void)
{
	static struct pmu_context_t pmu_context = {0};
	static struct pmu_context_t *p_pmu_context = NULL;

	if (!p_pmu_context) {
		pmu_context.pmuvdd_base = PMUVDD_BASE;
		pmu_context.pmusvcc_base = CHG_CTL_SVCC;
		pmu_context.onoff_short_detect = CONFIG_PMU_ONOFF_SHORT_DETECT;
		pmu_context.onoff_remote_same_wio = CONFIG_PMU_ONOFF_REMOTE_SAME_WIO;
		p_pmu_context = &pmu_context;
	}

	return p_pmu_context;
}

/* @brief get the base address of PMUVDD register */
static inline struct acts_pmuvdd *get_pmuvdd_reg_base(struct pmu_context_t *ctx)
{
	return (struct acts_pmuvdd *)ctx->pmuvdd_base;
}

/* @brief get the base address of PMUSVCC register */
static inline struct acts_pmusvcc *get_pmusvcc_reg_base(struct pmu_context_t *ctx)
{
	return (struct acts_pmusvcc *)ctx->pmusvcc_base;
}

/* @brief find the monitor device parameters by given detect_dev */
static struct detect_param_t *soc_pmu_find_detect_dev(struct pmu_context_t *ctx, uint8_t detect_dev)
{
	uint8_t i;

	for (i = 0; i < PMU_DETECT_MAX_DEV; i++) {
		if (ctx->detect_devs[i].detect_dev == detect_dev) {
			return &ctx->detect_devs[i];
		}
		LOG_DBG("dev slot[%d]:%d", i, ctx->detect_devs[i].detect_dev);
	}

	return NULL;
}

/* @brief register the function will be called when the state of monitor device change */
int soc_pmu_register_notify(struct detect_param_t *param)
{
	struct pmu_context_t *ctx = get_pmu_context();
	struct acts_pmusvcc *pmusvcc_reg = get_pmusvcc_reg_base(ctx);
	struct acts_pmuvdd *pmuvdd_reg = get_pmuvdd_reg_base(ctx);
	uint8_t i;

	if (!param)
		return -EINVAL;

	if (PMU_MONITOR_DEV_INVALID(param->detect_dev)) {
		LOG_ERR("Invalid monitor dev:%d", param->detect_dev);
		return -EINVAL;
	}

	for (i = 0; i < PMU_DETECT_MAX_DEV; i++) {
		/* new or update a monitor device into PMU context */
		if ((!(ctx->detect_devs[i].detect_dev))
			|| (ctx->detect_devs[i].detect_dev == param->detect_dev)) {
			ctx->detect_devs[i].detect_dev = param->detect_dev;
			ctx->detect_devs[i].cb_data = param->cb_data;
			ctx->detect_devs[i].notify = param->notify;
			break;
		} else {
			LOG_DBG("Busy slot[%d]:%d", i, ctx->detect_devs[i].detect_dev);
		}
	}

	if (i == PMU_DETECT_MAX_DEV) {
		LOG_ERR("no space for dev:0x%x", param->detect_dev);
		return -ENOMEM;
	}

	/* enable detect device interrupt and DC5V is already enabled at the stage of initialization */
	if (PMU_DETECT_DEV_ONOFF == param->detect_dev) {
		if (ctx->onoff_short_detect) {
			pmusvcc_reg->wake_ctl_svcc |= (WAKE_CTL_SVCC_SHORT_WKEN | WAKE_CTL_SVCC_LONG_WKEN);
			pmuvdd_reg->pmu_intmask |= (PMU_INTMASK_ONOFF_S_INTEN | PMU_INTMASK_ONOFF_L_INTEN);
		} else {
			pmusvcc_reg->wake_ctl_svcc |= WAKE_CTL_SVCC_LONG_WKEN;
			pmuvdd_reg->pmu_intmask |= PMU_INTMASK_ONOFF_L_INTEN;
		}
	} else if (PMU_DETECT_DEV_REMOTE == param->detect_dev) {
		pmusvcc_reg->wake_ctl_svcc |= WAKE_CTL_SVCC_REMOTE_WKEN;
		pmuvdd_reg->pmu_intmask |= PMU_INTMASK_REMOTE_INTEN;
	} else if (PMU_DETECT_DEV_COUNTER8HZ == param->detect_dev) {
		pmuvdd_reg->pmu_intmask |= PMU_INTMASK_COUNTER8HZ_INTEN;
	}

	LOG_DBG("pmu register dev:%d", param->detect_dev);

	return 0;
}

/* @brief unregister the notify function for specified monitor device */
void soc_pmu_unregister_notify(uint8_t detect_dev)
{
	struct pmu_context_t *ctx = get_pmu_context();
	uint8_t i;
	uint32_t key;

	for (i = 0; i < PMU_DETECT_MAX_DEV; i++) {
		if (ctx->detect_devs[i].detect_dev == detect_dev) {
			key = irq_lock();
			ctx->detect_devs[i].detect_dev = 0;
			ctx->detect_devs[i].notify = 0;
			ctx->detect_devs[i].cb_data = NULL;
			irq_unlock(key);
		}
	}
}

#ifdef CONFIG_ACTS_LEOPARD_BATTERY_SUPPLY_EXTERNAL
#define DET_DC5VIN 0x1
extern uint8_t get_external_dc5v_state(void);
#endif

/* @brief get the current DC5V status of plug in or out */
bool soc_pmu_get_dc5v_status(void)
{
	bool status;

	/* DC5VIN indicator, 0: DC5V < BAT +33mV; 1: DC5V > BAT + 80mV */
#ifdef CONFIG_ACTS_LEOPARD_BATTERY_SUPPLY_EXTERNAL
	if (get_external_dc5v_state() == DET_DC5VIN) {
		status = true;
	} else {
		status = false;
	}
#else
	struct pmu_context_t *ctx = get_pmu_context();
	struct acts_pmuvdd *pmuvdd_reg = get_pmuvdd_reg_base(ctx);
	if (pmuvdd_reg->pmu_det & PMU_DET_DC5VIN_DET) {
		status = true;
	} else {
		status = false;
	}
#endif

	return status;
}

/* @brief return the wakeup pending by system startup */
uint32_t soc_pmu_get_wakeup_source(void)
{
	struct pmu_context_t *ctx = get_pmu_context();
	struct acts_pmusvcc *pmusvcc_reg = get_pmusvcc_reg_base(ctx);
	static uint32_t pmu_wakeup_pending;

	if (!pmu_wakeup_pending) {
		pmu_wakeup_pending = pmusvcc_reg->wake_pd_svcc;
		LOG_INF("pmu wakeup pending:0x%x", pmu_wakeup_pending);
	}

	return pmu_wakeup_pending;
}

/* @brief return the wakeup setting by system startup */
uint32_t soc_pmu_get_wakeup_setting(void)
{
	struct pmu_context_t *ctx = get_pmu_context();
	struct acts_pmusvcc *pmusvcc_reg = get_pmusvcc_reg_base(ctx);
	static uint32_t pmu_wakeup_ctl;

	if (!pmu_wakeup_ctl) {
		pmu_wakeup_ctl = pmusvcc_reg->wake_ctl_svcc;
		LOG_INF("pmu wakeup setting:0x%x", pmu_wakeup_ctl);
	}

	return pmu_wakeup_ctl;
}

/* @brief check if system wakeup by RTC alarm */
bool soc_pmu_is_alarm_wakeup(void)
{
	uint32_t wk_pd;

	wk_pd = soc_pmu_get_wakeup_source();

	if (wk_pd & WAKE_CTL_SVCC_ALARM8HZ_WKEN)
		return true;

	return false;
}

/* @brief set the max constant current */
void soc_pmu_set_max_current(uint16_t cur_ma)
{
	struct pmu_context_t *ctx = get_pmu_context();
	struct acts_pmusvcc *pmusvcc_reg = get_pmusvcc_reg_base(ctx);
	uint8_t level;

	if (cur_ma <= 100) {
		level = (cur_ma + 9) / 10 - 1;
	} else if (cur_ma < 240) {
		level = (cur_ma + 19) / 20 + 4;
	} else {
		level = 15;
	}

	pmusvcc_reg->chg_ctl_svcc = (pmusvcc_reg->chg_ctl_svcc & ~0xF) | level;
}

/* @brief get the max constant current */
uint16_t soc_pmu_get_max_current(void)
{
	struct pmu_context_t *ctx = get_pmu_context();
	struct acts_pmusvcc *pmusvcc_reg = get_pmusvcc_reg_base(ctx);
	uint8_t level, cur_ma;

	level = pmusvcc_reg->chg_ctl_svcc & 0xF;

	if (level >= 10) {
		cur_ma = (level - 4) * 20;
	} else {
		cur_ma = (level + 1) * 10;
	}

	return cur_ma;
}

/* @brief lock(stop) DC5V charging for reading battery voltage */
void soc_pmu_read_bat_lock(void)
{
	struct pmu_context_t *ctx = get_pmu_context();
	struct acts_pmusvcc *pmusvcc_reg = get_pmusvcc_reg_base(ctx);

	pmusvcc_reg->chg_ctl_svcc |= CHG_CTL_SVCC_CV_3V3;
	k_busy_wait(300);
}

/* @brief unlock(resume) and restart DC5V charging */
void soc_pmu_read_bat_unlock(void)
{
	struct pmu_context_t *ctx = get_pmu_context();
	struct acts_pmusvcc *pmusvcc_reg = get_pmusvcc_reg_base(ctx);

	pmusvcc_reg->chg_ctl_svcc &= ~CHG_CTL_SVCC_CV_3V3;
	k_busy_wait(300);
}

/* @brief set const voltage value */
void soc_pmu_set_const_voltage(uint8_t cv)
{
	struct pmu_context_t *ctx = get_pmu_context();
	struct acts_pmusvcc *pmusvcc_reg = get_pmusvcc_reg_base(ctx);

	if (cv <= 0x1F) {
		pmusvcc_reg->chg_ctl_svcc = \
			(pmusvcc_reg->chg_ctl_svcc & ~CHG_CTL_SVCC_CV_OFFSET_MASK) | CHG_CTL_SVCC_CV_OFFSET(cv);
		k_busy_wait(300);
	}
}

/* @brief configure the long press on-off key time */
void soc_pmu_config_onoffkey_time(uint8_t val)
{
	unsigned int val_reg;
	struct pmu_context_t *ctx = get_pmu_context();
	struct acts_pmusvcc *pmusvcc_reg = get_pmusvcc_reg_base(ctx);

	if (val > 7)
		return ;

	val_reg = pmusvcc_reg->system_set_svcc & (~SYSTEM_SET_SVCC_ONOFF_PRESS_TIME_MASK);
	pmusvcc_reg->system_set_svcc = val_reg | SYSTEM_SET_SVCC_ONOFF_PRESS_TIME(val);

	k_busy_wait(300);
}

/* @brief configure the long press on-off key time */
void soc_pmu_config_onoffkey_function(uint8_t val)
{
	unsigned int val_reg;
	struct pmu_context_t *ctx = get_pmu_context();
	struct acts_pmusvcc *pmusvcc_reg = get_pmusvcc_reg_base(ctx);

	/**
	 * ONOFF long pressed function
	 * 0: no function
	 * 1: reset
	 * 2: restart (S1 => S4 => S1)
	 */
	if (val > 2)
		return ;

	val_reg = pmusvcc_reg->system_set_svcc &(~SYSTEM_SET_SVCC_ONOFF_RST_EN_MASK);
	pmusvcc_reg->system_set_svcc = val_reg | SYSTEM_SET_SVCC_ONOFF_RST_EN(val);

	k_busy_wait(300);
}

void soc_pmu_check_onoff_reset_func(void)
{
	struct pmu_context_t *ctx = get_pmu_context();
	struct acts_pmusvcc *pmusvcc_reg = get_pmusvcc_reg_base(ctx);
	if(!(pmusvcc_reg->system_set_svcc & SYSTEM_SET_SVCC_ONOFF_RST_EN_MASK))
		soc_pmu_config_onoffkey_function(1);
}



/* @brief check if the onoff key has been pressed or not */
bool soc_pmu_is_onoff_key_pressed(void)
{
	struct pmu_context_t *ctx = get_pmu_context();
	struct acts_pmusvcc *pmusvcc_reg = get_pmusvcc_reg_base(ctx);

	/* if 1 indicates that onoff key has been pressed */
	if (pmusvcc_reg->system_set_svcc & SYSTEM_SET_SVCC_ONOFF_PRES)
		return true;

	return false;
}

/* @brief configure the long press on-off key reset/restart time */
void soc_pmu_config_onoffkey_reset_time(uint8_t val)
{
	struct pmu_context_t *ctx = get_pmu_context();
	struct acts_pmusvcc *pmusvcc_reg = get_pmusvcc_reg_base(ctx);

	if (val) {
		pmusvcc_reg->system_set_svcc |= SYSTEM_SET_SVCC_ONOFF_RST_TIME; /* 12s */
	} else {
		pmusvcc_reg->system_set_svcc &= ~SYSTEM_SET_SVCC_ONOFF_RST_TIME; /* 8s */
	}

	k_busy_wait(300);
}

#ifdef CONFIG_PMU_COUNTER8HZ_SYNC_TIMEOUT_US
/* @brief counter 8hz clock enable */
void soc_pmu_counter8hz_enable(void)
{
	struct pmu_context_t *ctx = get_pmu_context();
	struct acts_pmusvcc *pmusvcc_reg = get_pmusvcc_reg_base(ctx);

	LOG_DBG("counter8hz_svcc:0x%x", pmusvcc_reg->counter8hz_svcc);

	/* clock source select LOSC */
	if (!(pmusvcc_reg->counter8hz_svcc & PMU_COUNTER8HZ_SVCC_COUNTER_EN)) {
#if (CONFIG_RTC_CLK_SOURCE == 1)
		pmusvcc_reg->counter8hz_svcc |= \
			(PMU_COUNTER8HZ_SVCC_CLK_SEL_LOSC | PMU_COUNTER8HZ_SVCC_COUNTER_EN);
#else
		pmusvcc_reg->counter8hz_svcc |= PMU_COUNTER8HZ_SVCC_COUNTER_EN;
#endif
		k_busy_wait(300);
	}
}

/* @brief get counter8hz and the retval is by cycles */
int soc_pmu_get_counter8hz_cycles(bool is_sync)
{
	struct pmu_context_t *ctx = get_pmu_context();
	struct acts_pmusvcc *pmusvcc_reg = get_pmusvcc_reg_base(ctx);
	struct acts_pmuvdd *pmuvdd_reg = get_pmuvdd_reg_base(ctx);

	if (!(pmusvcc_reg->counter8hz_svcc
		& PMU_COUNTER8HZ_SVCC_COUNTER_EN)) {
		LOG_ERR("counter8hz does not enable yet");
		return -EACCES;
	}

	if (is_sync) {
		/* clear counter8hz pending */
		pmuvdd_reg->pmu_det |= PMU_DET_COUNTR8HZ_PD;

		/* wait a new counter8hz pending */
		uint32_t cur_time = k_cycle_get_32();
		while (!(pmuvdd_reg->pmu_det & PMU_DET_COUNTR8HZ_PD)) {
			if (k_cyc_to_us_floor32(k_cycle_get_32() - cur_time) >=
				CONFIG_PMU_COUNTER8HZ_SYNC_TIMEOUT_US) {
				LOG_ERR("wait counter8hz pending timeout");
				return -ETIMEDOUT;
			}
			k_sleep(K_MSEC(1));
		}
	}

	return (pmusvcc_reg->counter8hz_svcc & PMU_COUNTER8HZ_SVCC_COUNTER_VAL);
}

#if (CONFIG_RTC_CLK_SOURCE == 2) 
/*@brief 8hzchcle cal to ms by rc32k 
rc32k_sum = rc32k_old + rc32k_new 
*/
 uint32_t sys_pmu_8hzcycle_to_ms(uint32_t cycle, uint32_t rc32k_sum)
{
	uint64_t  cal = cycle;
	uint32_t ms;
	ms = cal*(32768*125*2)/rc32k_sum; // cal	interval ms 
	return ms;
}

#endif

/* @brief enable PMU alarm 8hz */
int soc_pmu_alarm8hz_enable(uint32_t alarm_msec)
{
	struct pmu_context_t *ctx = get_pmu_context();
	struct acts_pmusvcc *pmusvcc_reg = get_pmusvcc_reg_base(ctx);
	struct acts_pmuvdd *pmuvdd_reg = get_pmuvdd_reg_base(ctx);
	uint32_t alarm8hz_cycles, new_alarm8hz_cycles, reg;
	int ret;

	/* clear alarm 8Hz pending */
	if (pmusvcc_reg->wake_pd_svcc & WAKE_PD_SVCC_ALARM8HZ_PD) {
		pmusvcc_reg->wake_pd_svcc |= WAKE_PD_SVCC_ALARM8HZ_PD;
		k_busy_wait(300);
	}

	alarm8hz_cycles = (alarm_msec + 124) / 125;
	#if (CONFIG_RTC_CLK_SOURCE == 2) // rc32k inaccurate, need calibration 
	uint32_t rc32k_freq;
	rc32k_freq = soc_rc32K_freq();
	printk("rc32k_freq=%d set 8hz=%d,", rc32k_freq, alarm8hz_cycles);
	alarm8hz_cycles = (uint64_t)alarm8hz_cycles*rc32k_freq/32768;
	printk("cal alarm8hz=%d  cur_8hz=%d\n", alarm8hz_cycles, soc_pmu_get_counter8hz_cycles(false));
	#endif	

	if (alarm8hz_cycles < 3) {
		LOG_ERR("invalid alarm8hz msec:%d", alarm_msec);
		return -EINVAL;
	}

	ret = soc_pmu_get_counter8hz_cycles(true);
	if (ret < 0) {
		LOG_ERR("failed to get counter8hz");
		return ret;
	}

	new_alarm8hz_cycles = ret + alarm8hz_cycles;

	/* 8Hz counter overflow */
	if (new_alarm8hz_cycles > 0x3FFFFFF)
		new_alarm8hz_cycles = new_alarm8hz_cycles - 0x3FFFFFF;

	reg = pmusvcc_reg->alarm8hz_svcc;

	reg &= ~PMU_ALARM8HZ_SVCC_ALARM_VAL_MASK;
	reg |= (new_alarm8hz_cycles & PMU_ALARM8HZ_SVCC_ALARM_VAL_MASK);

	pmusvcc_reg->alarm8hz_svcc = reg;

	pmusvcc_reg->wake_ctl_svcc |= WAKE_CTL_SVCC_ALARM8HZ_WKEN;

	pmuvdd_reg->pmu_intmask |= PMU_INTMASK_ALARM8HZ_INTEN;

	k_busy_wait(300);

	return 0;
}

/* @brief disable PMU alarm8hz */
void soc_pmu_alarm8hz_disable(void)
{
	struct pmu_context_t *ctx = get_pmu_context();
	struct acts_pmusvcc *pmusvcc_reg = get_pmusvcc_reg_base(ctx);
	struct acts_pmuvdd *pmuvdd_reg = get_pmuvdd_reg_base(ctx);

	/* clear alarm 8Hz pending */
	if (pmusvcc_reg->wake_pd_svcc & WAKE_PD_SVCC_ALARM8HZ_PD) {
		pmusvcc_reg->wake_pd_svcc |= WAKE_PD_SVCC_ALARM8HZ_PD;
		k_busy_wait(300);
	}

	pmuvdd_reg->pmu_intmask &= ~PMU_INTMASK_ALARM8HZ_INTEN;

	pmusvcc_reg->wake_ctl_svcc &= ~WAKE_CTL_SVCC_ALARM8HZ_WKEN;

	pmusvcc_reg->alarm8hz_svcc &= ~PMU_ALARM8HZ_SVCC_ALARM_VAL_MASK;

	k_busy_wait(300);
}

/* @brief get alarm8hz cycles */
int soc_pmu_get_alarm8hz(void)
{
	struct pmu_context_t *ctx = get_pmu_context();
	struct acts_pmusvcc *pmusvcc_reg = get_pmusvcc_reg_base(ctx);

	return pmusvcc_reg->alarm8hz_svcc & PMU_ALARM8HZ_SVCC_ALARM_VAL_MASK;
}
#endif
static void soc_pmu_isr(void *arg)
{
	struct pmu_context_t *ctx = get_pmu_context();
	struct acts_pmusvcc *pmusvcc_reg = get_pmusvcc_reg_base(ctx);
	struct acts_pmuvdd *pmuvdd_reg = get_pmuvdd_reg_base(ctx);
	struct detect_param_t *detect_param = NULL;

	ARG_UNUSED(arg);

	uint32_t pending = pmusvcc_reg->wake_pd_svcc;

	LOG_DBG("pmu pending:0x%x", pending);

	if (pending & WAKE_PD_SVCC_REMOTE_PD) {

		/* Both REMOTE and ONOFF key are connected with WIO0.
		  * Once the ONOFF key are pressed, the REMOTE key pending will also trigger.
		  */
		if (pending & WAKE_PD_SVCC_WIO0LV_PD) {
			if (ctx->onoff_short_detect) {
				detect_param = soc_pmu_find_detect_dev(ctx, PMU_DETECT_DEV_ONOFF);
				if (detect_param)
					detect_param->notify(detect_param->cb_data, PMU_NOTIFY_STATE_PRESSED);
			}

			/* If ONOFF and REMOTE key use the same WIO pin, ignore the REMOTE key pending when WIO low voltage. */
			if (ctx->onoff_remote_same_wio)
				goto out;
		}

		/* disable remote interrupt and the remote device will enable interrupt through register interface again */
		pmuvdd_reg->pmu_intmask &= ~PMU_INTMASK_REMOTE_INTEN;
		detect_param = soc_pmu_find_detect_dev(ctx, PMU_DETECT_DEV_REMOTE);
		if (detect_param)
			detect_param->notify(detect_param->cb_data, PMU_NOTIFY_STATE_PRESSED);
	} else if (pending & WAKE_PD_SVCC_ONOFF_L_PD) {
		detect_param = soc_pmu_find_detect_dev(ctx, PMU_DETECT_DEV_ONOFF);
		if (detect_param)
			detect_param->notify(detect_param->cb_data, PMU_NOTIFY_STATE_LONG_PRESSED);
	} else if (pending & WAKE_PD_SVCC_ONOFF_S_PD) {
		detect_param = soc_pmu_find_detect_dev(ctx, PMU_DETECT_DEV_ONOFF);
		if (detect_param)
			detect_param->notify(detect_param->cb_data, PMU_NOTIFY_STATE_PRESSED);
	} else if (pending & WAKE_PD_SVCC_WIO0LV_PD) {
		if (ctx->onoff_short_detect) {
			detect_param = soc_pmu_find_detect_dev(ctx, PMU_DETECT_DEV_ONOFF);
			if (detect_param)
				detect_param->notify(detect_param->cb_data, PMU_NOTIFY_STATE_PRESSED);
		}
	}

	if (pending & WAKE_PD_SVCC_DC5VOUT_PD) {
		detect_param = soc_pmu_find_detect_dev(ctx, PMU_DETECT_DEV_DC5V);
		if (detect_param)
			detect_param->notify(detect_param->cb_data, PMU_NOTIFY_STATE_OUT);
	}

	if (pending & WAKE_PD_SVCC_DC5VIN_PD) {
		detect_param = soc_pmu_find_detect_dev(ctx, PMU_DETECT_DEV_DC5V);
		if (detect_param)
			detect_param->notify(detect_param->cb_data, PMU_NOTIFY_STATE_IN);
	}

#ifdef CONFIG_ACTS_LEOPARD_BATTERY_SUPPLY_INTERNAL
	/* check whether bat protect pending */
	if (pending & WAKE_PD_TK_PD) {
		/* clear bat protect pending */
		pmusvcc_reg->wake_pd_svcc |= WAKE_PD_TK_PD;
		detect_param = soc_pmu_find_detect_dev(ctx, PMU_DETECT_DEV_BAT);
		if (detect_param)
			detect_param->notify(detect_param->cb_data, PMU_NOTIFY_STATE_BAT_PROTECT);
	}
#endif

	/* check whether counter8hz pending */
	if (pmuvdd_reg->pmu_det & PMU_DET_COUNTR8HZ_PD) {
		/* clear counter8hz pending */
		pmuvdd_reg->pmu_det |= PMU_DET_COUNTR8HZ_PD;
		detect_param = soc_pmu_find_detect_dev(ctx, PMU_DETECT_DEV_COUNTER8HZ);
		if (detect_param)
			detect_param->notify(detect_param->cb_data, PMU_NOTIFY_STATE_TIME_ON);
	}

	/* check whether alarm8hz pending */
	if (pmusvcc_reg->alarm8hz_svcc & PMU_ALARM8HZ_SVCC_PD) {
		/* clear alarm8hz pending */
		pmusvcc_reg->wake_pd_svcc |= WAKE_PD_SVCC_ALARM8HZ_PD;
		detect_param = soc_pmu_find_detect_dev(ctx, PMU_DETECT_DEV_ALARM8HZ);
		if (detect_param)
			detect_param->notify(detect_param->cb_data, PMU_NOTIFY_STATE_TIME_ON);
	}

out:
	/* counter8Hz irq pending is shared with WAKE_PD_SVCC  */
	if (pending)
		pmusvcc_reg->wake_pd_svcc = pending;
}

static void soc_pmu_irq_config(void)
{
	IRQ_CONNECT(IRQ_ID_PMU, CONFIG_PMU_IRQ_PRI,
			soc_pmu_isr,
			NULL, 0);
	irq_enable(IRQ_ID_PMU);
}

/*return rc32k freq*/
uint32_t acts_clock_rc32k_set_cal_cyc(uint32_t cal_cyc)
{
	uint32_t cnt;
	uint64_t  tmp = 32000000;
	sys_write32(0, RC32K_CAL);
	soc_udelay(300);
    sys_write32((cal_cyc << 8)|1, RC32K_CAL);
	soc_udelay(300);

    /* wait calibration done */
    while (!(sys_read32(RC32K_CAL) & (1 << 4))){}

	cnt =  sys_read32(RC32K_COUNT);
	if(cnt){
		cnt = tmp*cal_cyc/cnt;
	}
	LOG_INF("cal freq=%d, ctl=0x%x\n", cnt, sys_read32(RC32K_CTL)>>1);
	return cnt;
}

static uint32_t g_rc32_mul;
static __act_s2_sleep_data uint32_t g_rc32_freq;

static void acts_clock_rc32k_calibrate(void)
{
	uint32_t freq;
	//uint32_t freqs, freqe, setf;

	//sys_write32((0x2c << 1), RC32K_CTL);// defaut set 
	//soc_udelay(300);
	freq = acts_clock_rc32k_set_cal_cyc(100);
	LOG_INF("cur freq=%d\n", freq);
	#if 0
	if(freq < 32768){
		freqs = 44;
		freqe = 63;
	}else{
		freqs = 10;
		freqe = 44;
	}

	while(freqs < (freqe-1)){
 		setf = (freqs + freqe)/2;
		sys_write32((setf << 1), RC32K_CTL);
		soc_udelay(300);
		freq = acts_clock_rc32k_set_cal_cyc(100);
		if(freq < 32768){
			freqs = setf;
		}else{
			freqe = setf;
		}
	}
	#endif

	if(freq){
		g_rc32_mul = (CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC/freq);
		g_rc32_freq = freq;
	}else{
		g_rc32_mul = (CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC/32768);
		g_rc32_freq = 32768;
	}
	LOG_INF("calibrate freq=%d, mul=%d, ctl=0x%x\n", freq, g_rc32_mul, sys_read32(RC32K_CTL) >> 1);

}

#define  CALC_CYCLE_NUM  16
#define  CMU_ANADEBUG   (0x40000100+0x60)
#define  RC4M_ADJUST_HZ 4000000
#define  RC4M_STEP_ONE_HZ 80000


static void soc_pmu_rc4m_cal(void)
{
	uint32_t fr_val,reg_val, cm_reg_dbg_bak, step, step_num;
	uint64_t  tmp = 32000000;

	sys_write32(0, RC32K_CAL);
	cm_reg_dbg_bak = sys_read32(CMU_ANADEBUG);
	reg_val = cm_reg_dbg_bak&(~(0xf<<16));
	sys_write32(reg_val , CMU_ANADEBUG);
	sys_write32(reg_val|(3<<16) , CMU_ANADEBUG);

	reg_val = sys_read32(RC4M_CTL);
	step = (reg_val>>1) & 0x3f;
	reg_val &= ~(0x3f<<1);
	while(1){
		soc_udelay(100);
		sys_write32(0x01|(CALC_CYCLE_NUM<<8) , RC32K_CAL); //cal 16  cycle of 32K
		soc_udelay(300);
	    /* wait calibration done */
	    while (!(sys_read32(RC32K_CAL) & (1 << 4))){}
		fr_val =  sys_read32(RC32K_COUNT);
		if(fr_val){
			fr_val = tmp*CALC_CYCLE_NUM*1024/fr_val;
		}
		sys_write32(0, RC32K_CAL);
		printk("fr=0x%x, rc4m=%d HZ\n", step,  fr_val);
		if(fr_val <= RC4M_ADJUST_HZ)
			break;
		if(step  == 0)
			break;
		step_num = (fr_val - RC4M_ADJUST_HZ)/RC4M_STEP_ONE_HZ;
		if(step_num  == 0)
			step--;
		else if(step_num > step)
			step = 0;
		else
			step -= step_num;
		sys_write32(reg_val | (step<<1), RC4M_CTL);
	}
	sys_write32(cm_reg_dbg_bak , CMU_ANADEBUG);

}

uint32_t soc_rc32K_mutiple_hosc(void)
{
	return g_rc32_mul;
}

uint32_t soc_rc32K_freq(void)
{
	return g_rc32_freq;
}


uint32_t soc_pmu_get_vdd_voltage(void)
{
	uint32_t sel, volt_mv;

	sel = sys_read32(VOUT_CTL1_S1M) & 0xf;

	volt_mv = 550 + sel * 50;

	return volt_mv;
}

void soc_pmu_set_vdd_voltage(uint32_t volt_mv)
{
	unsigned int sel;

	if (volt_mv < 550 || volt_mv > 1300)
		return ;

	sel = (volt_mv - 550) / 50;
	sel = (sel << 4) | sel;  // set vdd s1 adn s1ml
	sys_write32((sys_read32(VOUT_CTL1_S1M) & ~(0xff)) | (sel), VOUT_CTL1_S1M);

	k_busy_wait(33);
}

void soc_pmu_set_vd12_voltage(uint32_t volt_mv)
{
	unsigned int sel;

	if (volt_mv < 600 || volt_mv > 1400)
		return ;

	sel = (volt_mv - 600) / 50;
	if (sel > 0xff)
		sel = 0xff;

	sys_write32((sys_read32(VOUT_CTL1_S1M) & ~(0xf<<8)) | (sel<<8), VOUT_CTL1_S1M);

	k_busy_wait(33);
}

#if defined(CONFIG_S1ML_LITTLE_BIAS)

#define EN_LGBIAS_S1ML        (1<<21)
#define CLOCK0_ID_HPWR        ((1<<CLOCK_ID_GPU) \
                              |(1<<CLOCK_ID_JPEG) \
                              |(1<<CLOCK_ID_DE) \
                              |(1<<CLOCK_ID_LRADC))
#define CLOCK1_ID_HPWR        ((1<<(CLOCK_ID_DSP-32)))

/* run in sram, so no need take care of nor and psram delaychain */
__sleepfunc void soc_change_s1ml_bias(void)
{
	/* <true: enter s1ml; false: exit s1ml> */
	static __act_s2_sleep_data bool enter_s1ml  = true;

	/* before entering s1ml/wfi */
	if (enter_s1ml) {
		/* when high pwr periphers all not work, we can use little bias in s1ml state */
		if ((!(sys_read32(CMU_DEVCLKEN0) & CLOCK0_ID_HPWR))
			&& (!(sys_read32(CMU_DEVCLKEN1) & CLOCK1_ID_HPWR))){
			/* disable large bias in s1ml to reduce consumption */
			sys_write32(sys_read32(VOUT_CTL1_S1M) & ~EN_LGBIAS_S1ML, VOUT_CTL1_S1M);
		}
		enter_s1ml = false;
	}

	/* after exiting s1ml/wfi */
	if (!enter_s1ml) {
		/* resume large bias for s1ml state */
		sys_write32(sys_read32(VOUT_CTL1_S1M) | EN_LGBIAS_S1ML, VOUT_CTL1_S1M);

		enter_s1ml = true;
	}
}
#endif

/* @brief get temperature degrees (multiply 10) in centigrade and if retval is negative will get error */
int soc_pmu_get_temperature(void)
{
	static uint8_t is_pmu_temperature_init = 0;
	const struct device* adc_dev = device_get_binding(CONFIG_PMUADC_NAME);
	struct adc_sequence adc_data = {0};
	uint8_t adc_buf[2];
	uint16_t adc_temp;
	int ret;

	if (adc_dev == NULL) {
		LOG_ERR("cannot get pmuadc device");
		return -1;
	}

	if (!is_pmu_temperature_init) {
		struct adc_channel_cfg adc_cfg = {0};
		adc_cfg.channel_id = PMUADC_ID_SENSOR;
		adc_channel_setup(adc_dev, &adc_cfg);
		is_pmu_temperature_init = 1;
	}

	adc_data.channels	 = BIT(PMUADC_ID_SENSOR);
	adc_data.buffer 	 = adc_buf;
	adc_data.buffer_size = sizeof(adc_buf);

	ret = adc_read(adc_dev, &adc_data);
	if (ret) {
		LOG_ERR("ADC read temperature error %d", ret);
		return -1;
	}

	adc_temp = sys_get_le16(adc_data.buffer);

	/* sample temerature formula: temperature = 4532 - (adc_temp x 1751 / 1024);
	 * precision: 0.1degree
	 * sample result multiply 10
	 */
	ret = 4532 - (1751 * adc_temp / 1024);

	return ret;
}

/* @brief set pmu reg CHG_CTL_SVCC */
void soc_pmu_set_chg_ctl_svcc(uint32_t mask, uint32_t value)
{
	sys_write32(((sys_read32(CHG_CTL_SVCC) & ~mask) | (value & mask)), CHG_CTL_SVCC);
	k_busy_wait(300);
}

/* @brief get pmu reg CHG_CTL_SVCC */
uint32_t soc_pmu_get_chg_ctl_svcc(void)
{
    return sys_read32(CHG_CTL_SVCC);
}

/* @brief set pmu reg POWER_CTL_SVCC */
void soc_pmu_sys_poweroff(void)
{
    sys_write32(0, POWER_CTL_SVCC);
	k_busy_wait(300);
}

/* @brief set pmu reg BDG_CTL_SVCC */
void soc_pmu_set_bdg_ctl_svcc(uint32_t mask, uint32_t value)
{
	sys_write32(((sys_read32(BDG_CTL_SVCC) & ~mask) | (value & mask)), BDG_CTL_SVCC);
	k_busy_wait(300);
}

/* @brief get pmu reg BDG_CTL_SVCC */
uint32_t soc_pmu_get_bdg_ctl_svcc(void)
{
    return sys_read32(BDG_CTL_SVCC);
}

/* @brief set pmu reg PMUADC_CTL */
void soc_pmu_set_pmuadc_ctl(uint32_t mask, uint32_t value)
{
	sys_write32(((sys_read32(PMUADC_CTL) & ~mask) | (value & mask)), PMUADC_CTL);

    if ((value & mask) & REG_SENSOR_EFUSE_MASK) {
	    k_busy_wait(300);
    }
}

/* @brief get pmu reg PMUADC_CTL */
uint32_t soc_pmu_get_pmuadc_ctl(void)
{
    return sys_read32(PMUADC_CTL);
}

/* @brief set pmu reg PMUADCDIG_CTL */
void soc_pmu_set_pmuadcdig_ctl(uint32_t mask, uint32_t value)
{
	sys_write32(((sys_read32(PMUADCDIG_CTL) & ~mask) | (value & mask)), PMUADCDIG_CTL);
}

/* @brief get pmu reg PMUADC_CTL */
uint32_t soc_pmu_get_pmuadcdig_ctl(void)
{
    return sys_read32(PMUADCDIG_CTL);
}

/* @brief set pmu reg VOUT_CTL0 */
void soc_pmu_set_vout_ctl0(uint32_t mask, uint32_t value)
{
	sys_write32(((sys_read32(VOUT_CTL0) & ~mask) | (value & mask)), VOUT_CTL0);
}

/* @brief get pmu reg VOUT_CTL0 */
uint32_t soc_pmu_get_vout_ctl0(void)
{
    return sys_read32(VOUT_CTL0);
}

/* @brief clear PMUADC_PD */
void soc_pmu_clear_pmuadc_pd(void)
{
	sys_write32(sys_read32(PMUADC_PD), PMUADC_PD);
}

/* @brief clear PMUADC_INTMASK */
void soc_pmu_clear_pmuadc_intmask(void)
{
	sys_write32(0x0, PMUADC_INTMASK);
}

/* @brief get PMUADC_PD */
uint32_t soc_pmu_get_pmuadc_pd(void)
{
	return sys_read32(PMUADC_PD);
}

/* @brief get CHARGI_DATA */
uint32_t soc_pmu_get_chargi_data(void)
{
	return (sys_read32(CHARGI_DATA) & 0x1FFF);
}

/* @brief get BATADC_DATA */
uint32_t soc_pmu_get_batadc_data(void)
{
	return (sys_read32(BATADC_DATA) & 0x3FFF);
}

/* @brief get DC5VADC_DATA */
uint32_t soc_pmu_get_dc5vadc_data(void)
{
	return (sys_read32(DC5VADC_DATA) & 0xFFF);
}

/* @brief get LRADCx_DATA */
uint32_t soc_pmu_get_lradcxadc_data(uint8_t chn)
{
	return (sys_read32(LRADC1_DATA + 4 * (chn - PMUADC_ID_LRADC1)) & 0x3FFF);
}

#if defined(CONFIG_ACTS_LEOPARD_BATTERY_SUPPLY_EXTERNAL) || defined(CONFIG_ACTS_LEOPARD_BATTERY_SUPPLY_INTERNAL)
extern bool battery_is_lowpower(void);
#endif
bool soc_pmu_lowpower_check(void)
{
#if defined(CONFIG_ACTS_LEOPARD_BATTERY_SUPPLY_EXTERNAL) || defined(CONFIG_ACTS_LEOPARD_BATTERY_SUPPLY_INTERNAL)
	return battery_is_lowpower();
#else
	return false;
#endif
}

/* @brief PMUVDD and PMUSVCC initialization */
static int soc_pmu_init(const struct device *dev)
{
	struct pmu_context_t *ctx = get_pmu_context();
	struct acts_pmusvcc *pmusvcc_reg = get_pmusvcc_reg_base(ctx);
	struct acts_pmuvdd *pmuvdd_reg = get_pmuvdd_reg_base(ctx);
	unsigned int val = 0;

	ARG_UNUSED(dev);

	soc_pmu_get_wakeup_source();

	/* by default to enable DC5V plug in/out detection */
	pmusvcc_reg->wake_ctl_svcc |= (WAKE_CTL_SVCC_DC5VOUT_WKEN | WAKE_CTL_SVCC_DC5VIN_WKEN);
	pmuvdd_reg->pmu_intmask |= (PMU_INTMASK_DC5VIN_INTEN | PMU_INTMASK_DC5VOUT_INTEN);

	if (ctx->onoff_short_detect) {
		pmusvcc_reg->wake_ctl_svcc |= WAKE_CTL_SVCC_WIO0LV_WKEN;
		pmuvdd_reg->pmu_intmask |= PMU_INTMASK_WIO0LV_INTEN;
	}

	pmusvcc_reg->wake_pd_svcc = pmusvcc_reg->wake_pd_svcc;

	//if (!ctx->onoff_remote_same_wio)
		//sys_write32(0, WIO0_CTL);
	

	if (!soc_atp_get_pmu_calib(11, &val)){ // cali vd12
		val &= 0x3;
		printk("soc cali vd12 =%d\n", val);
		pmuvdd_reg->vout_ctl0 = (pmuvdd_reg->vout_ctl0 & (~(0x3<<VOUT_CTL0_VD12_25X_BITI))) | (val << VOUT_CTL0_VD12_25X_BITI);		
	}else{
		printk("soc get cali vd12 fail\n");
	}

	soc_pmu_irq_config();
	acts_clock_rc32k_calibrate();
	soc_pmu_rc4m_cal();
	return 0;
}

SYS_INIT(soc_pmu_init, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_OBJECTS);

