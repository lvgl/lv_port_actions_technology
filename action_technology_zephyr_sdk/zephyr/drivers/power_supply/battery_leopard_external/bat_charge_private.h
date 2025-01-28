/*
 * Copyright (c) 2021  Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __BAT_CHARGE_PRIVATE_H__
#define __BAT_CHARGE_PRIVATE_H__

#include <kernel.h>
#include <soc.h>
#include <drivers/adc.h>
#include <drivers/power_supply.h>
#include <drivers/gpio.h>
#include <drivers/i2c.h>

#define BAT_CHECK_EXIST_THRESHOLD                (1000)

#define BAT_CHARGE_DRIVER_TIMER_MS               (50)

#define BAT_VOLT_CHECK_SAMPLE_SEC                (3)

#define BAT_ADC_SAMPLE_INTERVAL_MS               (100)
#define BAT_ADC_SAMPLE_BUF_TIME_SEC              BAT_VOLT_CHECK_SAMPLE_SEC
#define BAT_ADC_SAMPLE_BUF_SIZE                  (BAT_ADC_SAMPLE_BUF_TIME_SEC * 1000 / BAT_ADC_SAMPLE_INTERVAL_MS)

#define BAT_GET_ERRNO_INTERVAL_MS (1000)

#define EXT_IRQ_PD_CLEAR_INTERVAL_MS             (50)

#define BAT_CHARGER_DEBUG_INFO_OUT_MS (10*1000) // 10 second
//#define BAT_CHARGER_DEBUG_INFO_OUT_MS (1*1000) // 1 second

#define BAT_VOLTAGE_RESERVE                      (0x1fff)
#define BAT_CAP_RESERVE                          (101)
#define INDEX_VOL                                (0)
#define INDEX_CAP                                (INDEX_VOL + 1)
#define BAT_VOL_LSB_MV                           (3)

#define DC5V_STATE_TIMER_PERIOD_MS               (20)
#define DC5V_STATE_EXT_TIMER_PERIOD_MS           (20)

#define CFG_MAX_BATTERY_LEVEL                    (11)
#define BAT_VOL_DIFF                             (50)
#define BAT_INIT_VOLMV_DIFF_MAX                  (200)
#define BAT_LOW_VOLTAGE                          (3600)

enum CFG_TYPE_BOOL
{
    YES = 1,
    NO  = 0,
};

enum CFG_TYPE_BAT_CHARGE_MODE
{
    BAT_BACK_CHARGE_MODE,
    BAT_FRONT_CHARGE_MODE,
};

enum CFG_TYPE_BATTERY_LOW_VOLTAGE
{
    BATTERY_LOW_3_0_V = 3000,  // 3.0 V
    BATTERY_LOW_3_1_V = 3100,  // 3.1 V
    BATTERY_LOW_3_2_V = 3200,  // 3.2 V
    BATTERY_LOW_3_3_V = 3300,  // 3.3 V
    BATTERY_LOW_3_4_V = 3400,  // 3.4 V
    BATTERY_LOW_3_5_V = 3500,  // 3.5 V
    BATTERY_LOW_3_6_V = 3600,  // 3.6 V
};

enum CFG_TYPE_SYS_SUPPORT_FEATURES
{
    SYS_ENABLE_SOFT_WATCHDOG          = (1 << 0),
    SYS_ENABLE_DC5V_IN_RESET          = (1 << 1),
    SYS_ENABLE_DC5VPD_WHEN_DETECT_OUT = (1 << 2),
    SYS_FRONT_CHARGE_DC5V_OUT_REBOOT  = (1 << 3),
    SYS_FORCE_CHARGE_WHEN_DC5V_IN     = (1 << 4),
};

enum NTC_THERMISTOR
{
	NTC_THERMISTOR_10K = 0,
	NTC_THERMISTOR_100K = 1,
};

enum NTC_PULLUP_SEL
{
	NTC_PULLUP_INTERNAL = 0,
	NTC_PULLUP_EXTERNAL = 1,
};


typedef struct
{
    uint8_t   Select_Charge_Mode;

    uint16_t  Charge_Current;
    uint16_t  Charge_Voltage;
    uint16_t  Charge_Stop_Current;
	uint16_t  Charge_Stop_Voltage;
    uint16_t  Precharge_Stop_Voltage;
	uint16_t  Precharge_Current;
	uint16_t  Recharge_Voltage;

    uint16_t  Battery_Check_Period_Sec;
    uint16_t  Charge_Check_Period_Sec;
    uint16_t  Charge_Full_Continue_Sec;

    uint16_t  Front_Charge_Full_Power_Off_Wait_Sec;

    uint16_t  DC5V_Detect_Debounce_Time_Ms;

	bool NTC_Func_Enable;
	uint8_t NTC_Thermistor;
	uint8_t NTC_Ext_PullUp;
} CFG_Struct_Battery_Charge;


typedef struct
{
    uint16_t  Level[CFG_MAX_BATTERY_LEVEL];

} CFG_Struct_Battery_Level;


typedef struct
{
    uint16_t  Battery_Too_Low_Voltage;
    uint16_t  Battery_Low_Voltage;
    uint16_t  Battery_Low_Voltage_Ex;

    uint16_t  Battery_Low_Prompt_Interval_Sec;

} CFG_Struct_Battery_Low;


/* enumation for DC5V state  */
typedef enum
{
	DC5V_STATE_INVALID = 0xff,	/* invalid state */
	DC5V_STATE_OUT     = 0,		/* DC5V plug out */
	DC5V_STATE_IN      = 1,		/* DC5V plug in */
	DC5V_STATE_PENDING = 2,		/* DC5V pending */
	DC5V_STATE_STANDBY = 3,		/* DC5V standby */
} dc5v_state_t;

enum BAT_CHARGE_STATE {
	BAT_CHG_STATE_INIT = 0,   /* initial state */
	BAT_CHG_STATE_LOW,        /* low power state */
	BAT_CHG_STATE_NORMAL,     /* normal state */	
	BAT_CHG_STATE_PRECHARGE,  /* pre-charging state */
	BAT_CHG_STATE_CHARGE,     /* charging state */
	BAT_CHG_STATE_FULL,       /* full power state */
};

/* battery config for extern charger */
struct acts_battery_config {
	char *coulometer_name;
	char *adc_name;
	uint8_t batadc_chan;
	uint16_t debug_interval_sec;
};


/**
**	Config for extern charger
**/
typedef struct {
	CFG_Struct_Battery_Charge cfg_charge;	/* battery charge configuration */
	CFG_Struct_Battery_Level cfg_bat_level;	/* battery quantity level */
	CFG_Struct_Battery_Low cfg_bat_low;		/* battery low power configuration */
	uint16_t cfg_features;					/* CFG_TYPE_SYS_SUPPORT_FEATURES*/
} bat_charge_configs_t;

/**
**	extern charger context
**/
typedef struct {
	const struct device *this_dev;                    //extern charger device
	const struct device *coulometer_dev;              //extern coulometer device
	const struct device *adc_dev;                     //intern coulometer device
	const struct device *i2c_dev;                     //i2c device for extern charger
	const struct device *isr_gpio_dev;                //isr gpio device 
	
	struct gpio_callback ext_charger_gpio_cb;         //isr gpio cb
	
	struct adc_sequence bat_sequence;                 //for intern coulometer
	uint8_t bat_adcval[2];

	bat_charge_configs_t configs;

	struct k_delayed_work timer;
	struct k_delayed_work minicharger_timer;
	struct k_delayed_work charger_enable_timer;

	uint16_t bat_adc_sample_buf[BAT_ADC_SAMPLE_BUF_SIZE];

	uint8_t inited                   : 1;
	uint8_t is_in_charging           : 1;
	uint8_t is_in_precharge          : 1;
	uint8_t is_in_cc_state           : 1;
    uint8_t is_in_cv_state           : 1;
	uint8_t is_charge_finished       : 1;
	uint8_t safety_timer_expired     : 1;
	uint8_t prechg_timer_expired     : 1;

	uint8_t bat_exist_state          : 1;
	uint8_t charger_enabled          : 1;   
	uint8_t need_check_bat_real_volt : 1;
	uint8_t bat_volt_low             : 1;
	uint8_t bat_volt_low_ex          : 1;
	uint8_t bat_volt_too_low         : 1;
	uint8_t enabled                  : 1;
	uint8_t ext_charger_debug_open   : 1;
	uint8_t bat_check_real_in_charge :1;
	uint8_t bat_lowpower             : 1;
	uint8_t is_after_minicharger     : 1;

	uint8_t dc5v_last_state;
	uint8_t dc5v_current_state;
	
	uint8_t bat_full_dc5v_last_state;
	uint8_t bat_charge_last_state;
	uint8_t bat_charge_current_state;

	uint16_t bat_get_errno_count;
	uint16_t bat_adc_sample_timer_count;
	uint8_t bat_adc_sample_index;

	uint16_t state_timer_count;
	uint16_t precharge_time_sec;

	uint16_t bat_volt_check_wait_count;                      /* wait several second to get real voltage */
	uint16_t bat_real_volt;                                  /* battery real voltage, mv */

	uint8_t  irq_pending_bitmap;

	bat_charge_callback_t callback;

	uint8_t last_cap_report;                                 /* record last battery capacity report */
	uint8_t last_cap_detect;
	uint16_t last_voltage_report;                            /* record last battery voltage report */

	uint16_t last_saved_bat_volt;
	uint32_t last_time_check_bat_volt;

} bat_charge_context_t;


static inline bat_charge_context_t *bat_charge_get_context(void)
{
    extern bat_charge_context_t bat_charge_context;

    return &bat_charge_context;
}

static inline uint32_t bat_get_diff_time(uint32_t end_time, uint32_t begin_time)
{
    uint32_t diff_time;

    if (end_time >= begin_time) {
        diff_time = (end_time - begin_time);
    } else {
        diff_time = ((uint32_t)-1 - begin_time + end_time + 1);
    }

    return diff_time;
}


//functions

int ext_charger_init(void);
int ext_charger_enable(void);
int ext_charger_disable(void);
int ext_charger_get_config(const struct device *dev);
int ext_charger_get_state(void);
int ext_charger_get_errno(void);
int ext_charger_clear_irq_pending(uint8_t clr_bitmap);

int ext_charger_dump_info1(void);
int ext_charger_dump_info2(void);
int ext_charger_check_enable(void);



/* get the battery average voltage in mv during by specified seconds */
uint32_t get_battery_voltage_by_time(uint32_t sec);

/* set the time in seconds to sample the real battery voltage */
void bat_check_real_voltage(uint32_t sample_sec);

/* check and report the battery voltage changed infomation */
uint32_t bat_check_voltage_change(uint32_t bat_volt);

/* process according to the battery change status */
void bat_charge_status_proc(void);

/* get the DC5V state by string format */
const char *get_dc5v_state_str(uint8_t state);

/* get the battery state by string format */
const char* get_bat_state_str(uint8_t state);

/* sample a battery ADC value */
int bat_adc_get_sample(void);

/* get the current DC5V state */
dc5v_state_t get_dc5v_current_state(void);

/* convert from battery ADC value to voltage in millivolt */
uint32_t bat_adc_get_voltage_mv(uint32_t adc_val);

/* get the percent of battery capacity  */
uint32_t get_battery_percent(void);

/* enable charge */
int bat_charge_enable(void);

/* disable charge */
int bat_charge_disable(void);

/* check if battery exist */
void bat_check_battery_exist(uint32_t bat_volt);

#ifdef CONFIG_ACTS_BATTERY_SUPPLY_EXT_COULOMETER				
uint32_t get_battery_voltage_by_coulometer(void);
#endif

bool battery_is_lowpower(void);
uint32_t get_capacity_from_pstore();
void set_capacity_to_pstore(uint32_t bat_vol);

#endif /* __BAT_CHARGE_PRIVATE_H__ */

