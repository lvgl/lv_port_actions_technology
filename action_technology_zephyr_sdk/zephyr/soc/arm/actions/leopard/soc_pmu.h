/*
 * Copyright (c) 2021 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Actions LEOPARD family PMU public APIs
 */

#ifndef _SOC_PMU_H_
#define _SOC_PMU_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Macro of peripheral devices that PMU supervise the devices voltage level change.*/
#define PMU_DETECT_DEV_DC5V			(1)
#define PMU_DETECT_DEV_REMOTE		(2)
#define PMU_DETECT_DEV_ONOFF		(3)
#define PMU_DETECT_DEV_COUNTER8HZ   (4)
#define PMU_DETECT_DEV_ALARM8HZ     (5)
#define PMU_DETECT_DEV_BAT			(6)
#define PMU_DETECT_MAX_DEV			(6)

/* Max cycles of counter8hz */
#define PMU_COUTNER8HZ_MAX          (0x3FFFFFF)

/**
 * enum pmu_hotplug_state
 * @brief Hotplug devices (e.g. DV5V) state change which detected by PMU.
 */
enum pmu_notify_state {
	PMU_NOTIFY_STATE_OUT = 0,
	PMU_NOTIFY_STATE_IN,
	PMU_NOTIFY_STATE_PRESSED,
	PMU_NOTIFY_STATE_LONG_PRESSED,
	PMU_NOTIFY_STATE_TIME_ON,
	PMU_NOTIFY_STATE_BAT_PROTECT
};

typedef void (*pmu_notify_t)(void *cb_data, int state);

/**
 * struct notify_param_t
 * @brief Parameters for PMU notify register.
 */
struct detect_param_t {
	uint8_t detect_dev;
	pmu_notify_t notify;
	void *cb_data;
};

/* @brief Register the device that PMU start to monitor */
int soc_pmu_register_notify(struct detect_param_t *param);

/* @brief Unregister the device that PMU stop to monitor */
void soc_pmu_unregister_notify(uint8_t detect_dev);

/* @brief Get the wakeup source caused by system power up */
uint32_t soc_pmu_get_wakeup_source(void);

/* @brief return the wakeup setting by system startup */
uint32_t soc_pmu_get_wakeup_setting(void);

/* @brief check if system wakeup by RTC alarm */
bool soc_pmu_is_alarm_wakeup(void);

/* @brief get the current DC5V status of plug in or out  and if retval of status is 1 indicates that plug-in*/
bool soc_pmu_get_dc5v_status(void);

/* @brief lock DC5V charging for reading battery voltage */
void soc_pmu_read_bat_lock(void);

/* @brief unlock and restart DC5V charging */
void soc_pmu_read_bat_unlock(void);

/* @brief configure the long press on-off key time */
void soc_pmu_config_onoffkey_function(uint8_t val);

/* @brief check long onoff reset func, if clear, set */
void soc_pmu_check_onoff_reset_func(void);

/* @brief configure the long press on-off key time */
void soc_pmu_config_onoffkey_time(uint8_t val);

/* @brief check if the onoff key has been pressed or not */
bool soc_pmu_is_onoff_key_pressed(void);

/* @brief set const voltage value */
void soc_pmu_set_const_voltage(uint8_t cv);

/* @brief set the max constant current */
void soc_pmu_set_max_current(uint16_t cur_ma);

/* @brief get the max constant current */
uint16_t soc_pmu_get_max_current(void);

/* @brief configure the long press on-off key reset/restart time */
void soc_pmu_config_onoffkey_reset_time(uint8_t val);

/* @brief counter 8hz clock enable */
void soc_pmu_counter8hz_enable(void);

/* @brief get counter8hz and the retval is by cycles */
int soc_pmu_get_counter8hz_cycles(bool is_sync);

/*@brief 8hzchcle cal to ms by rc32k 
rc32k_sum = rc32k_old + rc32k_new 
*/
uint32_t sys_pmu_8hzcycle_to_ms(uint32_t cycle, uint32_t rc32k_sum);

/* @brief enable PMU alarm 8hz */
int soc_pmu_alarm8hz_enable(uint32_t alarm_msec);

/* @brief disable PMU alarm8hz */
void soc_pmu_alarm8hz_disable(void);

/* @brief get alarm8hz cycles */
int soc_pmu_get_alarm8hz(void);

/* @brief get current rc32k freq by hosc count, cal_cyc is rc32k cycle */
uint32_t acts_clock_rc32k_set_cal_cyc(uint32_t cal_cyc);

/* @brief get calibrate mul rc32k by hosc return hosc/rc32k */
uint32_t soc_rc32K_mutiple_hosc(void);

/* @brief get rc32k freq */
uint32_t soc_rc32K_freq(void);

/* @brief get S1 VDD voltage  */
uint32_t soc_pmu_get_vdd_voltage(void);

/* @brief set S1 VDD voltage  */
void soc_pmu_set_vdd_voltage(uint32_t volt_mv);

/* @brief set S1 VD12 voltage  */
void soc_pmu_set_vd12_voltage(uint32_t volt_mv);

/* @brief get temperature degrees (multiply 10) in centigrade and if retval is negative will get error */
int soc_pmu_get_temperature(void);

/* @brief set pmu reg CHG_CTL_SVCC */
void soc_pmu_set_chg_ctl_svcc(uint32_t mask, uint32_t value);

/* @brief get pmu reg CHG_CTL_SVCC */
uint32_t soc_pmu_get_chg_ctl_svcc(void);

/* @brief set pmu reg POWER_CTL_SVCC */
void soc_pmu_sys_poweroff(void);

/* @brief set pmu reg BDG_CTL_SVCC */
void soc_pmu_set_bdg_ctl_svcc(uint32_t mask, uint32_t value);

/* @brief get pmu reg BDG_CTL_SVCC */
uint32_t soc_pmu_get_bdg_ctl_svcc(void);

/* @brief set pmu reg PMUADC_CTL */
void soc_pmu_set_pmuadc_ctl(uint32_t mask, uint32_t value);

/* @brief get pmu reg PMUADC_CTL */
uint32_t soc_pmu_get_pmuadc_ctl(void);

/* @brief set pmu reg PMUADCDIG_CTL */
void soc_pmu_set_pmuadcdig_ctl(uint32_t mask, uint32_t value);

/* @brief get pmu reg PMUADCDIG_CTL */
uint32_t soc_pmu_get_pmuadcdig_ctl(void);

/* @brief set pmu reg VOUT_CTL0 */
void soc_pmu_set_vout_ctl0(uint32_t mask, uint32_t value);

/* @brief get pmu reg VOUT_CTL0 */
uint32_t soc_pmu_get_vout_ctl0(void);

/* @brief clear PMUADC_PD */
void soc_pmu_clear_pmuadc_pd(void);

/* @brief clear PMUADC_INTMASK */
void soc_pmu_clear_pmuadc_intmask(void);

/* @brief get PMUADC_PD */
uint32_t soc_pmu_get_pmuadc_pd(void);

/* @brief get CHARGI_DATA */
uint32_t soc_pmu_get_chargi_data(void);

/* @brief get BATADC_DATA */
uint32_t soc_pmu_get_batadc_data(void);

/* @brief get DC5VADC_DATA */
uint32_t soc_pmu_get_dc5vadc_data(void);

/* @brief get LRADCx_DATA */
uint32_t soc_pmu_get_lradcxadc_data(uint8_t chn);

/* @brief get bat volt */
bool soc_pmu_lowpower_check(void);


#ifdef __cplusplus
}
#endif

#endif /* _SOC_PMU_H_ */
