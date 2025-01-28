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
#include <soc_atp.h>
#include <drivers/hrtimer.h>

#include <board.h>

#ifdef CONFIG_RTC_ACTS
#include <drivers/rtc.h>
#include <assert.h>
#endif

#include "soc_pmu.h"

#define CHG_CTL_SVCC_DC5VPD_EN                   (27)
#define CHG_CTL_SVCC_DC5VPD_SET_SHIFT            (25)
#define CHG_CTL_SVCC_DC5VPD_SET_MASK             (0x3 << CHG_CTL_SVCC_DC5VPD_SET_SHIFT)
#define CHG_CTL_SVCC_DC5V_LIMIT_SET              (24)
#define CHG_CTL_SVCC_DC5V_LIMIT_EN               (23)
#define CHG_CTL_SVCC_BD_COMP_SHIFT               (21)
#define CHG_CTL_SVCC_BD_COMP_MASK                (0x3 << CHG_CTL_SVCC_BD_COMP_SHIFT)
#define CHG_CTL_SVCC_TK_EN                       (20)
#define CHG_CTL_SVCC_CV_3V3                      (19)
//#define CHG_CTL_SVCC_CC_ADD                    (18)
#define CHG_CTL_SVCC_CV_IN100                    (18)
#define CHG_CTL_SVCC_BAT_PD_SHIFT                (16)
#define CHG_CTL_SVCC_BAT_PD_MASK                 (0x3 << CHG_CTL_SVCC_BAT_PD_SHIFT)
#define CHG_CTL_SVCC_CC_OFFSET_SHIFT             (11)
#define CHG_CTL_SVCC_CC_OFFSET_MASK              (0x1F << CHG_CTL_SVCC_CC_OFFSET_SHIFT)
#define CHG_CTL_SVCC_CHG_EN                      (10)

#define CHG_CTL_SVCC_CV_OFFSET_SHIFT             (5)
#define CHG_CTL_SVCC_CV_OFFSET_MASK              (0x1F << CHG_CTL_SVCC_CV_OFFSET_SHIFT)

#define CHG_CTL_SVCC_CV_DE100                    (4)

#define CHG_CTL_SVCC_CHG_CURRENT_SHIFT           (0)
#define CHG_CTL_SVCC_CHG_CURRENT_MASK            (0xF << CHG_CTL_SVCC_CHG_CURRENT_SHIFT)

#define BDG_CTL_SVCC_CHARGEI_SET_SHIFT           (6)
#define BDG_CTL_SVCC_CHARGEI_SET_MASK            (0x1F << BDG_CTL_SVCC_CHARGEI_SET_SHIFT)

#define VOUT_CTL0_VC18_SW_DCDC                   BIT(18)
#define VOUT_CTL0_VD12_SW_DCDC                   BIT(17)

#define BAT_CHARGE_DRIVER_TIMER_MS               (50)
#define BAT_CHARGE_TIMER_THREAD_MS               (200)

#define BAT_VOLT_CHECK_SAMPLE_SEC                (3)

#define BAT_ADC_SAMPLE_INTERVAL_MS               (100)
#define BAT_ADC_SAMPLE_BUF_TIME_SEC              BAT_VOLT_CHECK_SAMPLE_SEC
#define BAT_ADC_SAMPLE_BUF_SIZE                  (BAT_ADC_SAMPLE_BUF_TIME_SEC * 1000 / BAT_ADC_SAMPLE_INTERVAL_MS)

#define BAT_CHARGEI_SAMPLE_INTERVAL_MS           (1000)
#define BAT_CHARGEI_SAMPLE_BUF_TIME_SEC          (10)
#define BAT_CHARGEI_SAMPLE_BUF_SIZE              (BAT_CHARGEI_SAMPLE_BUF_TIME_SEC * 1000 / BAT_CHARGEI_SAMPLE_INTERVAL_MS)

#define DC5V_DEBOUNCE_BUF_MAX                    (20)

#define DC5V_DEBOUNCE_MAX_TIME_MS                (DC5V_DEBOUNCE_BUF_MAX * BAT_CHARGE_DRIVER_TIMER_MS)

#define DC5V_DEBOUNCE_TIME_DEFAULT_MS            (300)

#define BAT_VOLTAGE_RESERVE                      (0x1fff)
#define BAT_CAP_RESERVE                          (101)
#define INDEX_VOL                                (0)
#define INDEX_CAP                                (INDEX_VOL + 1)
#define BAT_VOL_LSB_MV                           (3)

#define DC5V_STATE_TIMER_PERIOD_MS               (20)
#define DC5V_STATE_EXT_TIMER_PERIOD_MS           (20)

#ifdef CONFIG_ACTS_BATTERY_SUPPORT_CHARGER_NTC  
#define BAT_NTC_SAMPLE_INTERVAL_MS               (1000)
#define NTC_DEBOUNCE_BUF_MAX                     (20)
#define NTC_DEBOUNCE_MAX_TIME_S                  (NTC_DEBOUNCE_BUF_MAX * BAT_NTC_SAMPLE_INTERVAL_MS / 1000)
#define NTC_DEBOUNCE_TIME_DEFAULT_S              (5)
#endif

#ifdef CONFIG_ACTS_BATTERY_SUPPORT_FAST_CHARGER  
#define BAT_FAST_CHARGE_CV_LOWER_MV              (4)        //50mv
#endif

#define BAT_ERR_STOP_RESTART_TIME_MS             (10*60*1000) //10 minute

#define BAT_CHARGER_DEBUG_INFO_OUT_MS            (10*1000) // 10 second
//#define BAT_CHARGER_DEBUG_INFO_OUT_MS          (1*1000) // 1 second

#define BAT_MINI_CHARGE_INTERVAL_MS              (1*1000)  // 1 second

#define BAT_REAL_VOLTAGE_SMOOTH_IN_CHARGE        (3)    // limit 3 percent max per minute
#define BAT_REAL_VOLTAGE_SMOOTH_IN_DISCHARGE     (3)    // limit 3 percent max per minute

#define BAT_VOL_DIFF                             (50)
#define BAT_INIT_VOLMV_DIFF_MAX                  (200)

#define BAT_CHARGE_INTERVAL_MS                   (1*1000)
#define BAT_EXTREMELY_LOW_VOLTAGE                (3000)
#define BAT_LOW_VOLTAGE                          (3600)
#define BAT_PROTECT_TEST_VOLTAGE                 (3600)
#define BAT_MINI_CHARGE_OTHER_CONSUME_MA         (30)   //other consume ma in init charge stage.

#define BAT_VOLTAGE_SAMPLE_VALID_MIN             (2200)
#define BAT_VOLTAGE_SAMPLE_VALID_MAX             (4800)

// 0% ~ 100% 
#define CFG_MAX_BATTERY_LEVEL                    (11)

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

enum CFG_TYPE_BAT_BD_COMP
{
    BAT_BD_COMP_DISABLE = 0x00,  // Disable
    BAT_BD_COMP_50_MO   = 0x01,  // 50 mo
    BAT_BD_COMP_100_MO  = 0x02,  // 100 mo
    BAT_BD_COMP_150_MO  = 0x03,  // 150 mo
};

enum CFG_TYPE_CHARGE_CURRENT
{
    CHARGE_CURRENT_20_MA  = 0x00,  // 20 mA
    CHARGE_CURRENT_30_MA  = 0x01,  // 30 mA
    CHARGE_CURRENT_40_MA  = 0x02,  // 40 mA
    CHARGE_CURRENT_50_MA  = 0x03,  // 50 mA
    CHARGE_CURRENT_60_MA  = 0x04,  // 60 mA
    CHARGE_CURRENT_70_MA  = 0x05,  // 70 mA
    CHARGE_CURRENT_80_MA  = 0x06,  // 80 mA
    CHARGE_CURRENT_90_MA  = 0x07,  // 90 mA
    CHARGE_CURRENT_100_MA = 0x08,  // 100 mA
    CHARGE_CURRENT_150_MA = 0x09,  // 150 mA
    CHARGE_CURRENT_200_MA = 0x0A,  // 200 mA
    CHARGE_CURRENT_250_MA = 0x0B,  // 250 mA
    CHARGE_CURRENT_300_MA = 0x0C,  // 300 mA
    CHARGE_CURRENT_350_MA = 0x0D,  // 350 mA
    CHARGE_CURRENT_400_MA = 0x0E,  // 400 mA
    CHARGE_CURRENT_450_MA = 0x0F,  // 450 mA
};

enum CFG_TYPE_CHARGE_VOLTAGE
{
	CHARGE_VOLTAGE_4_1375_V = 0x0,
	CHARGE_VOLTAGE_4_15_V   = 0x1,
    CHARGE_VOLTAGE_4_20_V   = 0x5,
    CHARGE_VOLTAGE_4_25_V   = 0x9,
    CHARGE_VOLTAGE_4_30_V   = 0xd,
    CHARGE_VOLTAGE_4_35_V   = 0x11,
	CHARGE_VOLTAGE_4_4375_V = 0x18,
    CHARGE_VOLTAGE_4_45_V   = 0x19,
};

enum CFG_TYPE_PRECHARGE_STOP_VOLTAGE
{
    PRECHARGE_STOP_3_0_V = 3000,  // 3.0 V
    PRECHARGE_STOP_3_1_V = 3100,  // 3.1 V
    PRECHARGE_STOP_3_2_V = 3200,  // 3.2 V
    PRECHARGE_STOP_3_3_V = 3300,  // 3.3 V
    PRECHARGE_STOP_3_4_V = 3400,  // 3.4 V
    PRECHARGE_STOP_3_5_V = 3500,  // 3.5 V
    PRECHARGE_STOP_3_6_V = 3600,  // 3.6 V
};

enum CFG_TYPE_PRECHARGE_CURRENT
{
    PRECHARGE_CURRENT_5_PERCENT   = 0,    // 5%
    PRECHARGE_CURRENT_10_PERCENT  = 1,    // 10%
	PRECHARGE_CURRENT_15_PERCENT  = 2,    // 15%
	PRECHARGE_CURRENT_20_PERCENT  = 3,    // 20%
	PRECHARGE_CURRENT_25_PERCENT  = 4,    // 25%
	PRECHARGE_CURRENT_30_PERCENT  = 5,    // 30%
	PRECHARGE_CURRENT_35_PERCENT  = 6,    // 35%
	PRECHARGE_CURRENT_40_PERCENT  = 7,    // 40%
	PRECHARGE_CURRENT_45_PERCENT  = 8,    // 45%
	PRECHARGE_CURRENT_50_PERCENT  = 9,    // 50%
};

enum CFG_TYPE_PRECHARGE_CURRENT_MIN_LIMIT
{
    PRECHARGE_CURRENT_MIN_20_MA  = 0x00,  // 20 mA
    PRECHARGE_CURRENT_MIN_30_MA  = 0x01,  // 30 mA
    PRECHARGE_CURRENT_MIN_40_MA  = 0x02,  // 40 mA
    PRECHARGE_CURRENT_MIN_50_MA  = 0x03,  // 50 mA
    PRECHARGE_CURRENT_MIN_60_MA  = 0x04,  // 60 mA
    PRECHARGE_CURRENT_MIN_70_MA  = 0x05,  // 70 mA
    PRECHARGE_CURRENT_MIN_80_MA  = 0x06,  // 80 mA
    PRECHARGE_CURRENT_MIN_90_MA  = 0x07,  // 90 mA
    PRECHARGE_CURRENT_MIN_100_MA = 0x08,  // 100 mA
};

enum CFG_TYPE_CHARGE_STOP_CURRENT
{
    CHARGE_STOP_CURRENT_20_PERCENT = 0,  // 20%
    CHARGE_STOP_CURRENT_5_PERCENT  = 1,  // 5%

    CHARGE_STOP_CURRENT_30_MA,   // 30 mA
    CHARGE_STOP_CURRENT_20_MA,   // 20 mA
    CHARGE_STOP_CURRENT_16_MA,   // 16 mA
    CHARGE_STOP_CURRENT_12_MA,   // 12 mA
    CHARGE_STOP_CURRENT_8_MA,    // 8 mA
    CHARGE_STOP_CURRENT_6_4_MA,  // 6.4 mA
    CHARGE_STOP_CURRENT_5_MA,    // 5 mA
};

enum CFG_TYPE_CHARGE_STOP_MODE
{
    CHARGE_STOP_BY_VOLTAGE = 0,
    CHARGE_STOP_BY_CURRENT = 1,
    CHARGE_STOP_BY_VOLTAGE_AND_CURRENT = 2,
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

enum CFG_TYPE_DC5VPD_CURRENT
{
    DC5VPD_CURRENT_DISABLE = 0xff,
    DC5VPD_CURRENT_2_5_MA  = 0x0,   // 2.5 mA
    DC5VPD_CURRENT_7_5_MA  = 0x1,   // 7.5 mA
    DC5VPD_CURRENT_15_MA   = 0x2,   // 15 mA
    DC5VPD_CURRENT_25_MA   = 0x3,   // 25 mA
};

enum CFG_TYPE_DC5VLV_LEVEL
{
    DC5VLV_LEVEL_0_2_V = 0,
    DC5VLV_LEVEL_0_5_V = 1,
    DC5VLV_LEVEL_1_0_V = 2,
    DC5VLV_LEVEL_1_5_V = 3,
    DC5VLV_LEVEL_2_0_V = 4,
    DC5VLV_LEVEL_2_5_V = 5,
    DC5VLV_LEVEL_3_0_V = 6,
    DC5VLV_LEVEL_4_5_V = 7,
};

enum CFG_TYPE_BAT_RECHARGE_THRESHOLD
{
    BAT_RECHARGE_3_4_V = 0x0,  // 3.4 V
    BAT_RECHARGE_3_5_V = 0x1,  // 3.5 V
    BAT_RECHARGE_3_6_V = 0x2,  // 3.6 V
    BAT_RECHARGE_3_7_V = 0x3,  // 3.7 V
    BAT_RECHARGE_3_8_V = 0x4,  // 3.8 V
    BAT_RECHARGE_3_9_V = 0x5,  // 3.9 V
    BAT_RECHARGE_4_0_V = 0x6,  // 4.0 V
    BAT_RECHARGE_4_1_V = 0x7,  // 4.1 V
    BAT_RECHARGE_4_2_V = 0x8,  // 4.2 V
};

enum CFG_TYPE_FASTCHARGE_VOLTAGE_THRESHOLD
{
    FAST_CHARGE_3_4_V = 0x0,  // <"3.4 V">
    FAST_CHARGE_3_5_V = 0x1,  // <"3.5 V">
    FAST_CHARGE_3_6_V = 0x2,  // <"3.6 V">
    FAST_CHARGE_3_7_V = 0x3,  // <"3.7 V">
    FAST_CHARGE_3_8_V = 0x4,  // <"3.8 V">
    FAST_CHARGE_3_9_V = 0x5,  // <"3.9 V">
    FAST_CHARGE_4_0_V = 0x6,  // <"4.0 V">
    FAST_CHARGE_4_1_V = 0x7,  // <"4.1 V">
};

enum CFG_TYPE_SYS_SUPPORT_FEATURES
{
    SYS_ENABLE_SOFT_WATCHDOG          = (1 << 0),
    SYS_ENABLE_DC5V_IN_RESET          = (1 << 1),
    SYS_ENABLE_DC5VPD_WHEN_DETECT_OUT = (1 << 2),
    SYS_FRONT_CHARGE_DC5V_OUT_REBOOT  = (1 << 3),
    SYS_FORCE_CHARGE_WHEN_DC5V_IN     = (1 << 4),
};

typedef struct
{
    uint8_t   Select_Charge_Mode;
    uint8_t   Charge_Current;
    uint8_t   Charge_Voltage;
	
    uint8_t   Charge_Stop_Mode;
    uint16_t  Charge_Stop_Voltage;
    uint8_t   Charge_Stop_Current;

	uint8_t Mini_Charge_Enable;
    uint8_t Mini_Charge_Current_1;
    uint16_t Mini_Charge_Vol_1;
    uint8_t Mini_Charge_Current_2;
    uint16_t Mini_Charge_Vol_2;	
    uint8_t Mini_Charge_Current_3;
	uint16_t Mini_Charge_Vol_3;
	
	uint8_t   Precharge_Enable;
    uint16_t  Precharge_Stop_Voltage;
    uint8_t   Precharge_Current;	
	uint8_t   Precharge_Current_Min_Limit;
	
    uint8_t   Fast_Charge_Enable;
    uint8_t   Fast_Charge_Current;
    uint16_t  Fast_Charge_Voltage_Threshold;
	
    uint8_t   Enable_Battery_Recharge;
    uint16_t Recharge_Threshold_Low_Temperature;
    uint16_t Recharge_Threshold_Normal_Temperature;
    uint16_t Recharge_Threshold_High_Temperature;
	
    uint8_t Bat_OVP_Enable;
    uint16_t OVP_Voltage_Low_Temperature;
	uint16_t OVP_Voltage_Normal_Temperature;
	uint16_t OVP_Voltage_High_Temperature;
	
	uint16_t  Charge_Total_Time_Limit;
    uint16_t  Battery_Check_Period_Sec;
    uint16_t  Charge_Check_Period_Sec;
    uint16_t  Charge_Full_Continue_Sec;
    uint16_t  Front_Charge_Full_Power_Off_Wait_Sec;
    uint16_t  DC5V_Detect_Debounce_Time_Ms;
	uint16_t Battery_Default_RDrop;
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

typedef struct
{
    uint8_t   Enable_NTC;
    uint8_t   LRADC_Channel;
    uint8_t   Ref_Channel;
    uint8_t   LRADC_Pull_Up;
    uint8_t   LRADC_Value_Test;
	uint16_t NTC_Det_Debounce_Time_S;

} CFG_Type_NTC_Settings;

typedef struct
{
    uint16_t  ADC_Min;
    uint16_t  ADC_Max;
    uint8_t   Adjust_Current_Percent;
	uint8_t   Adjust_CV_Level;
    uint16_t  Charge_Time_Limit;

} CFG_Type_NTC_Range;

typedef struct
{
    CFG_Type_NTC_Settings  NTC_Settings;

    CFG_Type_NTC_Range  NTC_Ranges[5];

} CFG_Struct_NTC_Settings;

typedef struct 
{
	uint16_t init_ntc_ma_too_low;
	uint16_t init_ntc_ma_low;
	uint16_t init_ntc_ma_normal;
	uint16_t init_ntc_ma_high;
	uint16_t init_ntc_ma_too_high;
} CFG_Init_NTC_MA;

typedef struct 
{
    uint16_t init_stage1_NTC_current_ma[5];
	uint16_t init_stage2_NTC_current_ma[5];
	uint16_t init_stage3_NTC_current_ma[5];
	
} CFG_Struct_Init_NTC_Settings;

/* enumation for DC5V state  */
typedef enum
{
	DC5V_STATE_INVALID = 0xff,	/* invalid state */
	DC5V_STATE_OUT     = 0,		/* DC5V plug out */
	DC5V_STATE_IN      = 1,		/* DC5V plug in */
	DC5V_STATE_PENDING = 2,		/* DC5V pending */
	DC5V_STATE_STANDBY = 3,		/* DC5V standby */
} dc5v_state_t;

/* enumation for NTC state */
typedef enum {
	NTC_TEMPERATURE_INVALID = 0xff,
	NTC_TEMPERATURE_HIGH_EX = 0,
	NTC_TEMPERATURE_HIGH    = 1,
	NTC_TEMPERATURE_NORMAL  = 2,
	NTC_TEMPERATURE_LOW     = 3,
	NTC_TEMPERATURE_LOW_EX  = 4,
} NTC_temp_e;

enum BAT_CHARGE_STATE {
	BAT_CHG_STATE_INIT = 0,   /* initial state */
	BAT_CHG_STATE_LOW,        /* low power state */
	BAT_CHG_STATE_PRECHARGE,  /* pre-charging state */
	BAT_CHG_STATE_NORMAL,     /* normal state */
	BAT_CHG_STATE_CHARGE,     /* charging state */
	BAT_CHG_STATE_FULL,       /* full power state */
};

struct acts_battery_info {
	struct device *adc_dev;
	struct adc_sequence bat_sequence;
	struct adc_sequence charge_sequence;
	struct adc_sequence dc5v_sequence;
#ifdef CONFIG_ACTS_BATTERY_SUPPORT_CHARGER_NTC
	struct adc_sequence ntc_sequence;
	struct adc_sequence vcc_sequence;
#endif
	
	uint8_t bat_adcval[2];
	uint8_t charge_adcval[2];
	uint8_t dc5v_adcval[2];
#ifdef CONFIG_ACTS_BATTERY_SUPPORT_CHARGER_NTC
	uint8_t ntc_adcval[2];
	uint8_t vcc_adcval[2];
#endif
	
	uint32_t timestamp;
};

struct acts_battery_config {
	char *adc_name;
	uint8_t batadc_chan;
	uint8_t chargeadc_chan;
	uint8_t dc5v_chan;
	
#ifdef CONFIG_ACTS_BATTERY_SUPPORT_CHARGER_NTC
	uint8_t ntc_chan;
	uint8_t vcc_chan;
#endif

	uint16_t debug_interval_sec;
};

typedef struct {
	uint8_t last_state;
	struct k_delayed_work charger_enable_timer;
} dc5v_check_status_t;

typedef struct {
	uint8_t buf[5];
	uint8_t count;
	uint8_t index;
	s8_t  percent;
} bat_charge_current_percent_buf_t;

typedef struct {
	uint8_t buf[10];
	uint8_t count;
	uint8_t index;
} bat_charge_cv_state_buf_t;

typedef struct {
	uint8_t is_valid;
	uint8_t cv_offset;
	uint8_t adjust_end;
	int last_volt;
} bat_charge_adjust_cv_offset_t;

typedef struct {
	uint8_t is_valid;
	uint8_t charge_current;
	uint8_t stop_current_percent;
	uint16_t stop_current_ma;
} bat_charge_adjust_current_t;

typedef struct {
	CFG_Struct_Battery_Charge cfg_charge;	/* battery charge configuration */
	CFG_Struct_Battery_Level cfg_bat_level;	/* battery quantity level */
	CFG_Struct_Battery_Low cfg_bat_low;		/* battery low power configuration */
#ifdef CONFIG_ACTS_BATTERY_SUPPORT_CHARGER_NTC
    CFG_Struct_NTC_Settings cfg_bat_ntc;    /* battery NTC configuration */
#endif
#ifdef CONFIG_ACTS_BATTERY_SUPPORT_MINI_CHARGER_NTC
    CFG_Struct_Init_NTC_Settings cfg_bat_init_ntc; /* battery mini charge NTC configuration */
#endif
	uint16_t cfg_features;					/* CFG_TYPE_SYS_SUPPORT_FEATURES*/
} bat_charge_configs_t;

//BAT Choose, choose only one type below:
#define USE_BAT_4450MV (0)
#define USE_BAT_4350MV (1)
#define USE_BAT_4200MV (0)

#if (USE_BAT_4450MV == 1)
#define BAT_CHARGE_CFG_DEFAULT { \
				BAT_BACK_CHARGE_MODE, CHARGE_CURRENT_450_MA, CHARGE_VOLTAGE_4_4375_V, \
				CHARGE_STOP_BY_CURRENT, 4400, 23, \
				YES, CHARGE_CURRENT_30_MA, 3100, CHARGE_CURRENT_70_MA, 3400, CHARGE_CURRENT_450_MA, 3600, \
				NO, 2800, PRECHARGE_CURRENT_10_PERCENT, PRECHARGE_CURRENT_MIN_100_MA, \
				NO, CHARGE_CURRENT_450_MA, FAST_CHARGE_4_1_V, \
				YES, 4350, 4350, 4100, \
				YES, 4500, 4500, 4300, \
				240, 60, 300, 420, 10, 300, 500}
			
#define BAT_LEVEL_CFG_DEFAULT { \
				{3600, 3700, 3760, 3800, 3860, 3920, 4000, 4100, 4200, 4280, 4340}}
	
#define BAT_LOW_CFG_DEFAULT { \
				3600, 3700, 0, 60}
#endif

#if (USE_BAT_4350MV == 1)
#define BAT_CHARGE_CFG_DEFAULT { \
				BAT_BACK_CHARGE_MODE, CHARGE_CURRENT_450_MA, CHARGE_VOLTAGE_4_35_V, \
				CHARGE_STOP_BY_VOLTAGE, 4320, 22, \
				YES, CHARGE_CURRENT_30_MA, 3000, CHARGE_CURRENT_70_MA, 3500, CHARGE_CURRENT_300_MA, 3600, \
				NO, PRECHARGE_STOP_3_3_V, PRECHARGE_CURRENT_10_PERCENT, PRECHARGE_CURRENT_MIN_100_MA, \
				NO, CHARGE_CURRENT_450_MA, FAST_CHARGE_4_1_V, \
				YES, 4310, 4300, 4300, \
				YES, 4400, 4425, 4450, \
				240, 60, 300, 420, 10, 300, 500}
			
#define BAT_LEVEL_CFG_DEFAULT { \
				{3450, 3740, 3770, 3797, 3830, 3900, 3958, 4024, 4137, 4263, 4318}}
	
#define BAT_LOW_CFG_DEFAULT { \
				3450, 3740, 0, 60}
#endif

#if (USE_BAT_4200MV == 1)
#define BAT_CHARGE_CFG_DEFAULT { \
				BAT_BACK_CHARGE_MODE, CHARGE_CURRENT_450_MA, CHARGE_VOLTAGE_4_20_V, \
				CHARGE_STOP_BY_VOLTAGE_AND_CURRENT, 4160, 20, \
				YES, CHARGE_CURRENT_30_MA, 3000, CHARGE_CURRENT_70_MA, 3200, CHARGE_CURRENT_100_MA, 3400, \
				YES, PRECHARGE_STOP_3_3_V, PRECHARGE_CURRENT_10_PERCENT, PRECHARGE_CURRENT_MIN_100_MA, \
				NO, CHARGE_CURRENT_450_MA, FAST_CHARGE_3_9_V, \
				YES, 4110, 4110, 3900, \
				YES, 4260, 4260, 4100, \
				240, 60, 300, 420, 10, 300, 500}
	
#define BAT_LEVEL_CFG_DEFAULT { \
				{3100, 3400, 3600, 3650, 3700, 3750, 3800, 3900, 3980, 4050, 4100}}
	
#define BAT_LOW_CFG_DEFAULT { \
				3100, 3400, 0, 60}
#endif 

#ifdef CONFIG_ACTS_BATTERY_SUPPORT_CHARGER_NTC    
#define BAT_NTC_SETTING_DEFAULT { \
	1, 0, 0, 1, 5}
/* 52-60 */
#define BAT_NTC_RANGE_0 { \
	0xb7, 0xea, 0, 0xff, 0}
/* 42-52 */
#define BAT_NTC_RANGE_1 { \
	0xeb, 0x13d, 71, CHARGE_VOLTAGE_4_15_V, 60}
/* 14-42 */
#define BAT_NTC_RANGE_2 { \
	0x13e, 0x279, 100, CHARGE_VOLTAGE_4_4375_V, 240}
/* 0-14 */
#define BAT_NTC_RANGE_3 { \
	0x280, 0x313, 57, CHARGE_VOLTAGE_4_4375_V, 60}
/* 0-(-14) */
#define BAT_NTC_RANGE_4 { \
	0x314, 0x37f, 0, 0xff, 0}
#endif

#ifdef CONFIG_ACTS_BATTERY_SUPPORT_MINI_CHARGER_NTC
#define BAT_INIT_STAGE1_NTC_MA { \
	0, 30, 30, 30, 0}
#define BAT_INIT_STAGE2_NTC_MA { \
	0, 70, 70, 70, 0}
#define BAT_INIT_STAGE3_NTC_MA { \
	0, 100, 250, 200, 0}
#endif

#define CHARGE_CURRENT_FIRST_LEVEL    CHARGE_CURRENT_60_MA
#define CHARGE_CURRENT_ADD_STEP_MA    60       

typedef struct {
	struct device *dev;

#ifdef CONFIG_RTC_ACTS
	const struct device *rtc_dev;
#endif

	bat_charge_configs_t configs;

	struct k_delayed_work minitimer;
	struct k_delayed_work timer;
	struct k_work wakeup_timer;

	uint8_t dc5v_debounce_buf[DC5V_DEBOUNCE_BUF_MAX];
	uint16_t bat_adc_sample_buf[BAT_ADC_SAMPLE_BUF_SIZE];
	uint16_t charge_current_ma_buf[BAT_CHARGEI_SAMPLE_BUF_SIZE];
#ifdef CONFIG_ACTS_BATTERY_SUPPORT_CHARGER_NTC  	
	uint8_t NTC_debounce_buf[NTC_DEBOUNCE_BUF_MAX];
#endif

	uint8_t inited                   : 1;
	uint8_t charge_state_started     : 1;
	uint8_t charge_ctrl_enabled      : 1;
	uint8_t need_check_bat_real_volt : 1;
	uint8_t charge_near_full         : 1;
	uint8_t bat_volt_low             : 1;
	uint8_t bat_volt_low_ex          : 1;
	uint8_t bat_volt_too_low         : 1;
	uint8_t bat_exist_state          : 1;
	uint8_t bat_error_state          : 1;
	uint8_t dc5v_state_exit          : 1;
	uint8_t dc5v_state_ex_exit       : 1;
	uint8_t dc5v_state_pending 		 : 1;
	uint8_t dc5v_state_ex_pending    : 1;
	uint8_t enabled                  : 1;
	uint8_t use_config_system_consume :1;
	uint8_t need_add_cc_by_step      : 1;
	uint8_t bat_check_protected      : 1;
	uint8_t bat_is_protected         : 1;
	uint8_t work_queue_start         : 1;
	uint8_t bat_lowpower             : 1;
	uint8_t is_after_minicharger     : 1;

	uint8_t dc5v_debounce_state;
	uint8_t bat_full_dc5v_last_state;
	uint8_t bat_charge_state;

	uint8_t charge_current_sel;
	uint8_t current_cc_sel;
    uint8_t precharge_current_sel;
	uint8_t charge_enable_pending;
	uint8_t limit_current_percent;
	uint8_t limit_cv_level;
	
	uint16_t last_dc5v_mv;	

	uint8_t mini_charge_stage_last;
	uint8_t mini_charge_stage;
    uint8_t ntc_current_index;
	uint16_t mini_charge_current_ma_1;
	uint16_t mini_charge_current_ma_2;

	uint16_t bat_adc_sample_timer_count;
	uint16_t bat_chargei_sample_timer_count;
	uint8_t bat_adc_sample_index;
	uint8_t bat_chargei_sample_index;

	uint16_t state_timer_count;
	uint16_t precharge_time_sec;

	uint16_t bat_volt_check_wait_count;                              /* wait count to check battery real voltage */
	uint16_t bat_real_volt;                                          /* battery real voltage */

	uint16_t bat_real_charge_current;
	bat_charge_callback_t callback;

	dc5v_check_status_t dc5v_check_status;
	bat_charge_current_percent_buf_t charge_current_percent_buf;
	bat_charge_cv_state_buf_t charge_cv_state_buf;
	bat_charge_adjust_cv_offset_t adjust_cv_offset;

	bat_charge_adjust_current_t adjust_current;

	uint32_t cv_stage_1_begin_time;                            /* in constant voltage stage1 (current level less than 50%)  */
	uint32_t cv_stage_2_begin_time;                            /* in constant voltage stage2 (current level less than 20%)  */

	uint32_t dc5v_state_pending_time;                          /* DC5V state pending time */

	uint32_t bak_PMU_CHG_CTL;                                  /* backup PMU_CHG_CTL as SVCC domain which take low effect */

	uint8_t last_cap_report;                                   /* record last battery capacity report */
	uint8_t last_cap_detect;
	uint16_t last_voltage_report;                              /* record last battery voltage report */

#ifdef CONFIG_ACTS_BATTERY_SUPPORT_CHARGER_NTC  
	uint8_t bat_ntc_sample_timer_count;
	NTC_temp_e ntc_debounce_state;
	uint32_t current_ntc_begin_time;
	uint16_t current_ntc_charge_time_limit;                    /* charge time limit in current temperature region */
#endif

#ifdef CONFIG_ACTS_BATTERY_SUPPORT_FAST_CHARGER
	uint8_t in_fast_charge_stage;
#endif

	uint32_t charge_begin_time;
	uint32_t err_stop_begin_time;
	uint8_t  err_to_stop_charge;
	uint8_t err_status_dc5v_change_flag;
	uint8_t restart_from_err_flag;

	uint8_t  charge_cv_offset;

	uint16_t last_saved_bat_volt;
	uint32_t last_time_check_bat_volt;

	int16_t adc_bat_offset;

	uint32_t bat_charge_system_consume_ma;

	struct hrtimer standby_wakeup_timer;
} bat_charge_context_t;

static inline bat_charge_context_t *bat_charge_get_context(void)
{
    extern bat_charge_context_t bat_charge_context;
    return &bat_charge_context;
}

/* get interval time from begin time to end time */
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

/* get the battery average voltage in mv during by specified seconds */
uint32_t get_battery_voltage_by_time(uint32_t sec);

/* enable battery charging function */
void bat_charge_ctrl_enable(uint32_t current_sel, uint32_t voltage_sel);

/* check battery charging status */
void bat_charge_check_enable(void);

/* disable battery charging function */
void bat_charge_ctrl_disable(void);

/* set the time in seconds to sample the real battery voltage */
void bat_check_real_voltage(uint32_t sample_sec);

/* check if battery voltage is low power and callback msg if needed. */
void bat_check_voltage_low(uint32_t bat_volt);
/* check and report the battery voltage changed infomation */
uint32_t bat_check_voltage_change(uint32_t bat_volt);

/* process according to the battery change status */
void bat_charge_status_proc(void);

/* charging current percent buffer initialization */
void bat_charge_current_percent_buf_init(void);

/* get the charging current precent value */
int get_charge_current_percent(void);

//int get_charge_current_percent_ex(int percent);

/* buffer for constant voltage state initialization */
void bat_charge_cv_state_buf_init(void);

/* get the state of constant voltage */
int bat_charge_get_cv_state(void);

/* adjust the constant voltage value */
void bat_charge_adjust_cv_offset(void);

/* adjust the constant current value */
bool bat_charge_adjust_current(void);

/* get the DC5V state by string format */
const char *get_dc5v_state_str(uint8_t state);

/* get the charge current in milliamps */
int bat_charge_get_current_ma(int level);

/* get the charge current level */
int bat_charge_get_current_level(int ma);

/* battery and charger initialization */
void bat_charge_ctrl_init(void);

/* adjust bandgap */
void sys_ctrl_adjust_bandgap(uint8_t adjust);

/* sample a DC5V ADC value */
int dc5v_adc_get_sample(void);

/* sample a battery ADC value */
int bat_adc_get_sample(void);

/* sample a chargei ADC value */
int chargei_adc_get_sample(void);

/* get the current DC5V state */
dc5v_state_t get_dc5v_current_state(void);

/* get the configration of battery and charger */
bat_charge_configs_t *bat_charge_get_configs(void);

/* convert from DC5V ADC value to voltage in millivolt */
uint32_t dc5v_adc_get_voltage_mv(uint32_t adc_val);

/* convert from battery ADC value to voltage in millivolt */
uint32_t bat_adc_get_voltage_mv(uint32_t adc_val);

/* get the chargei converts to current in milliampere */
uint32_t chargei_adc_get_current_ma(int adc_val);

/* enable DC5V pulldown and will last specified time in miliseconds to disable */
void dc5v_pull_down_enable(uint32_t current, uint32_t timer_ms);

/* get the percent of battery capacity  */
uint32_t get_battery_percent(void);

void bat_charge_state_start(uint32_t current_sel);

const char* get_charge_state_str(uint8_t state);

void bat_charge_limit_current(uint32_t percent);

void bat_mini_charge_set(uint8_t ntc_index);

int pmuadc_digital_setting(void);
int pmuadc_set_bat_pd_resistance(bool open);
#ifdef CONFIG_ACTS_BATTERY_SUPPORT_MINI_CHARGER_NTC
int pmuadc_wait_NTC_sample_complete(void);
#endif
void pmusvcc_regbackup(void);
void bat_charger(bool open);
void bat_set_bd_comp(void);
void bat_lowpower_charge_setting(int current_ma);

void bat_charger_set_before_enter_s4(void);

void charger_enable_timer_delete(void);
void pmu_chg_ctl_reg_write(uint32_t mask, uint32_t value);
void bat_current_gradual_change(void);

void bat_charge_set_current(uint32_t current_sel);

#ifdef CONFIG_ACTS_BATTERY_SUPPORT_CHARGER_NTC
int pmuadc_set_ntc_resistance(bool open);
int pmuadc_set_ntc_ref_resistance(bool open);
int bat_ntc_adc_get_sample(void);
#endif

uint32_t bat_check_voltage_change_discharge(uint32_t bat_volt);

uint32_t bat_adc_get_voltage_adc(uint32_t mv_val);
void bat_adc_sample_buf_put(uint32_t bat_adc);

void bat_charge_update_real_voltage(struct k_work *work);

void pmuvdd_set_vd12_vc18_mode(bool vd12_dcdc, bool vc18_dcdc);

bool battery_is_lowpower(void);
uint32_t get_capacity_from_pstore();
void set_capacity_to_pstore(uint32_t bat_vol);
#endif /* __BAT_CHARGE_PRIVATE_H__ */

