/*
 * Copyright (c) 2021  Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Actions SoC battery and charger driver
 */

#include <stdlib.h>
#include <errno.h>
#include <kernel.h>
#include <sys/byteorder.h>
#include <drivers/power_supply.h>
#include <soc.h>
#include <board.h>

#include "bat_charge_private.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(bat_ext, CONFIG_LOG_DEFAULT_LEVEL);


#define _CW6305_REG_NUM         14
#define _CW6305_RW_RETRY_CNT    3


//cw6305 i2c access addr
#define CW6305_SLAVER_ADDR   (0x16>>1)


//cw6305 reg define
#define REG_VERSION         0x00
#define REG_VBUS_VOLT       0x01
#define REG_VBUS_CUR        0x02
#define REG_CHG_VOLT        0x03
#define REG_CHG_CUR1        0x04
#define REG_CHG_CUR2        0x05
#define REG_CHG_SAFETY      0x06
#define REG_CONFIG          0x07
#define REG_SAFETY_CFG      0x08
#define REG_SAFETY_CFG2     0x09
#define REG_INT_SET         0x0a
#define REG_INT_SRC         0x0b
#define REG_IC_STATUS       0x0d
#define REG_IC_STATUS2      0x0e

//reg bit 

/* 01h, DCIN input voltage, input min voltage config */
#define VBUS_VOLT_EN_HIZ               BIT(7)                                    //1: enable Hi-Z mode;  0: disable Hi-Z mode
#define VBUS_VOLT_CHG_EN               BIT(6)                                    //1: enable charger;  0: disable charger
#define VBUS_VOLT_VBUS_DPM_SHIFT       0
#define VBUS_VOLT_VBUS_DPM_MASK        (0x0f<<VBUS_VOLT_VBUS_DPM_SHIFT)
#define VBUS_VOLT_VBUS_DPM_SET(x)      ((x)<<VBUS_VOLT_VBUS_DPM_SHIFT)

/* 02h, DCIN input current config */
#define VBUS_CUR_VSYS_REG_SHIFT        4
#define VBUS_CUR_VSYS_REG_MASK        (0x0f<<VBUS_CUR_VSYS_REG_SHIFT)
#define VBUS_CUR_VSYS_REG_SET(x)      ((x)<<VBUS_CUR_VSYS_REG_SHIFT)
#define VBUS_CUR_VBUS_ILIM_SHIFT       0
#define VBUS_CUR_VBUS_ILIM_MASK       (0x0f<<VBUS_CUR_VBUS_ILIM_SHIFT)
#define VBUS_CUR_VBUS_ILIM_SET(x)     ((x)<<VBUS_CUR_VBUS_ILIM_SHIFT)

/* 03h, Battery charge voltage config */
#define CHG_VOLT_VBREG_SHIFT           2
#define CHG_VOLT_VBREG_MASK            (0x3f<<CHG_VOLT_VBREG_SHIFT)
#define CHG_VOLT_VBREG_SET(x)          ((x)<<CHG_VOLT_VBREG_SHIFT)
#define CHG_VOLT_VRECHG                BIT(1)                                    //0: 100mv;  1: 200mv
#define CHG_VOLT_BATLOWV               BIT(0)                                    //0: 2.8v;   1: 3.0v

/* 04h, Charge current config */
#define CHG_CUR1_TREG_SHIFT            6
#define CHG_CUR1_ICHG_SHIFT            0
#define CHG_CUR1_ICHG_MASK             (0x3f<<CHG_CUR1_ICHG_SHIFT)               //base 10mA, 10mA-470mA
#define CHG_CUR1_ICHG_SET(x)           ((x)<<CHG_CUR1_ICHG_SHIFT)

/* 05h, Charge current config */
#define FRC_PRECHG                     BIT(7)                                    //force to use pre-charge current. 0: disable; 1: enable
#define PRECHG_SEL_SHIFT               4
#define PRECHG_SEL_MASK                (0x07<<PRECHG_SEL_SHIFT)
#define TERM_SEL_SHIFT                 0
#define TERM_SEL_MASK                  (0x07<<TERM_SEL_SHIFT)

/* 06h, charge safety and NTC config */
#define REG_RESET                      BIT(7)                                    //0: disable; 1: reset all register to default value
#define CHG_TIMER_SHIFT                5
#define CHG_TIMER_MASK                 (0x03<<CHG_TIMER_SHIFT)
#define CHG_TIMER_SET(x)               ((x)<<CHG_TIMER_SHIFT)
#define EN_TERM                        BIT(4)
#define EN_PRE_TIMER                   BIT(3)
#define NTC_EN                         BIT(2)
#define NTC_SEL                        BIT(1)
#define NTC_NET_SEL                    BIT(0)

/* 07h, Charger state config */
#define BATFET_OFF                     BIT(7)
#define OFF_DELAY_SHIFT                5
#define OFF_DELAY_MASK                 (0x03<<OFF_DELAY_SHIFT)
#define EN_INT_BUTTON                  BIT(4)
#define BUTTON_FUNC                    BIT(3)
#define OFF_LAST_TIMER                 BIT(2)
#define INT_OFF_TIMER_SHIFT            0
#define INT_OFF_TIMER_MASK             (0x03<<INT_OFF_TIMER_SHIFT)

/* 08h, Safety config */
#define WD_CHG                         BIT(7)
#define WD_DISCHG                      BIT(6)
#define WATCH_TIMER_SHIFT              4
#define WATCH_TIMER_MASK               (0x03<<WATCH_TIMER_SHIFT)
#define BAT_UVLO_SHIFT                 0
#define BAT_UVLO_MASK                  (0x07<<BAT_UVLO_SHIFT)

/* 09h, Safety config II */
#define TMR2X_EN                       BIT(7)
#define SYS_OCP_SHIFT                  0
#define SYS_OCP_MASK                   (0x0f<<SYS_OCP_SHIFT)
#define SYS_OCP_SET(x)                 ((x)<<SYS_OCP_SHIFT)

/* 0Ah, INT Set */
#define MASK_CHG_DET                   BIT(7)
#define MASK_CHG_REMOVE                BIT(6)
#define MASK_CHG_FINISH                BIT(5)
#define MASK_CHG_OV                    BIT(4)
#define MASK_BAT_OT                    BIT(3)
#define MASK_BAT_UT                    BIT(2)
#define MASK_SAFETY_TIMER              BIT(1)
#define MASK_PRECHG_TIMER              BIT(0)

/* 0Bh, INT Source */
#define INT_CHG_DET                    BIT(7)
#define INT_CHG_REMOVE                 BIT(6)
#define INT_CHG_FINISH                 BIT(5)
#define INT_CHG_OV                     BIT(4)
#define INT_BAT_OT                     BIT(3)
#define INT_BAT_UT                     BIT(2)
#define INT_SAFETY_TIMER               BIT(1)
#define INT_PRECHG_TIMER               BIT(0)

/* 0Dh, IC_STATUS */
#define REPORT_THERM_STAT              BIT(7)
#define REPORT_PRE_CHG                 BIT(6)
#define REPORT_CC_STAT                 BIT(5)
#define REPORT_CV_STAT                 BIT(4)
#define REPORT_CHG_FINISH              BIT(3)
#define REPORT_VBUS_VLIM               BIT(2)
#define REPORT_VBUS_ILIM               BIT(1)
#define REPORT_PPMG_STAT               BIT(0)

/* 0Eh, IC_STATUS II */
#define REPORT_VBUS_PG                 BIT(7)
#define REPORT_VBUS_OV                 BIT(6)
#define REPORT_CHRG_STAT               BIT(5)
#define REPORT_IC_OT                   BIT(4)
#define REPORT_BAT_OT                  BIT(3)
#define REPORT_BAT_UT                  BIT(2)
#define REPORT_SAFETY_TIMER_EXP        BIT(1)
#define REPORT_PRECHG_TIMER_EXP        BIT(0)

//#define BAT_CALIBRATION_VOL (4450)
#define BAT_CALIBRATION_VOL (4200)


/* config private param */
#define PARAM_VSYS_REG_VOL 0x09 //4.65V
#define PARAM_VBUS_ILIM 0x0f //500mA

#define PARAM_CHG_TIMER 0x01 //00:disable; 01:4hours; 02:8hours; 03:12hours

#define PARAM_WD_TIMER 0x01 //00:60s; 01:120s; 02:180s; 03:240s
#define PARAM_BAT_UVLO 0x03 //2.8V.  2~7: 2.7~3.2V, step is 0.1V

#define PARAM_SYS_OCP 0x09 //1A.  0~15: 100mA~1600mA, step is 100mA

#if (BAT_CALIBRATION_VOL == 4200)
#define CHARGE_FULL_STOP_VOL      4160      //CV-40mv
#endif

#if (BAT_CALIBRATION_VOL == 4450)
#define CHARGE_FULL_STOP_VOL      4400      //CV-50mv
#endif


/***************************************************************
*	enum & struct 
****************************************************************
**/
enum CFG_TYPE_CHARGE_CURRENT
{
    CHARGE_CURRENT_10_MA  = 0x00,  // 10 mA
    CHARGE_CURRENT_20_MA  = 0x08,  // 20 mA
    CHARGE_CURRENT_30_MA  = 0x0c,  // 30 mA
    CHARGE_CURRENT_40_MA  = 0x10,  // 40 mA
    CHARGE_CURRENT_50_MA  = 0x12,  // 50 mA
    CHARGE_CURRENT_60_MA  = 0x14,  // 60 mA
    CHARGE_CURRENT_70_MA  = 0x16,  // 70 mA
    CHARGE_CURRENT_80_MA  = 0x18,  // 80 mA
    CHARGE_CURRENT_90_MA  = 0x19,  // 90 mA
    CHARGE_CURRENT_100_MA = 0x1a,  // 100 mA
    CHARGE_CURRENT_150_MA = 0x1f,  // 150 mA
    CHARGE_CURRENT_200_MA = 0x24,  // 200 mA
    CHARGE_CURRENT_250_MA = 0x29,  // 250 mA
    CHARGE_CURRENT_300_MA = 0x2e,  // 300 mA
    CHARGE_CURRENT_350_MA = 0x33,  // 350 mA
    CHARGE_CURRENT_400_MA = 0x38,  // 400 mA
    CHARGE_CURRENT_450_MA = 0x3d,  // 450 mA
};

enum CFG_TYPE_CHARGE_VOLTAGE
{
    CHARGE_VOLTAGE_4_20_V = 0x1c,    // 4.2v
    CHARGE_VOLTAGE_4_25_V = 0x20,    // 4.25v
    CHARGE_VOLTAGE_4_30_V = 0x24,    // 4.3v
    CHARGE_VOLTAGE_4_35_V = 0x28,    // 4.35v
    CHARGE_VOLTAGE_4_40_V = 0x2c,    // 4.4v
    CHARGE_VOLTAGE_4_45_V = 0x30,    // 4.4v
    CHARGE_VOLTAGE_4_60_V = 0x3c,// 4.6v
};

enum CFG_TYPE_CHARGE_STOP_CURRENT
{
    CHARGE_STOP_CURRENT_2DOT5_PERCENT  = 0x00,  // 2.5% CC
    CHARGE_STOP_CURRENT_5_PERCENT      = 0x01,  // 5% CC

    CHARGE_STOP_CURRENT_7DOT5_PERCENT  = 0x02,  // 7.5% CC
    CHARGE_STOP_CURRENT_10_PERCENT     = 0x03,  // 10% CC
    CHARGE_STOP_CURRENT_12DOT5_PERCENT = 0x04,  // 12.5% CC
    CHARGE_STOP_CURRENT_15_PERCENT     = 0x05,  // 15% CC
    CHARGE_STOP_CURRENT_17DOT5_PERCENT = 0x06,  // 17.5% CC
    CHARGE_STOP_CURRENT_20_PERCENT     = 0x07,  // 20% CC
};

enum CFG_TYPE_PRECHARGE_STOP_VOLTAGE
{
    PRECHARGE_STOP_2_8_V = 0,        // 2.8v
    PRECHARGE_STOP_3_0_V = 1,        // 3.0v
};

enum CFG_TYPE_PRECHARGE_CURRENT
{
    PRECHARGE_CURRENT_2DOT5_PERCENT  = 0x00,  // 2.5% CC
    PRECHARGE_CURRENT_5_PERCENT      = 0x01,  // 5% CC

    PRECHARGE_CURRENT_7DOT5_PERCENT  = 0x02,  // 7.5% CC
    PRECHARGE_CURRENT_10_PERCENT     = 0x03,  // 10% CC
    PRECHARGE_CURRENT_12DOT5_PERCENT = 0x04,  // 12.5% CC
    PRECHARGE_CURRENT_15_PERCENT     = 0x05,  // 15% CC
    PRECHARGE_CURRENT_17DOT5_PERCENT = 0x06,  // 17.5% CC
    PRECHARGE_CURRENT_20_PERCENT     = 0x07,  // 20% CC
};

enum CFG_TYPE_RECHARGE_THRD
{
    RECHARGE_THRD_100MV = 0,
	RECHARGE_THRD_200MV = 1,
};

enum CTL_CHARGER_ONOFF
{
    CHARGE_STOP = 0,
	CHARGE_START = 1,
};

enum ERROR_NUMBER
{
	ERROR_NOERR            = 0,
	ERROR_VBUS_PG          = 1,
	ERROR_VBUS_OV          = 2,
	ERROR_IC_OT            = 3,
	ERROR_BAT_OT           = 4,
	ERROR_BAT_UT           = 5,
	ERROR_SAFETY_TM_EXPIR  = 6,
	ERROR_PRECHG_TM_EXPIR  = 7,
};


/* config charger param */

#if (BAT_CALIBRATION_VOL == 4200)
#define BAT_CHARGE_CFG_DEFAULT { \
				BAT_BACK_CHARGE_MODE, CHARGE_CURRENT_250_MA, CHARGE_VOLTAGE_4_20_V, \
				CHARGE_STOP_CURRENT_10_PERCENT, CHARGE_FULL_STOP_VOL,\
				PRECHARGE_STOP_3_0_V, PRECHARGE_CURRENT_10_PERCENT, \
				60, 300, 420, 10, 300, \
				NO, NTC_THERMISTOR_10K, NTC_PULLUP_INTERNAL}

#define BAT_LEVEL_CFG_DEFAULT { \
				{3100, 3400, 3600, 3650, 3700, 3750, 3800, 3900, 4000, 4100}}
		
#define BAT_LOW_CFG_DEFAULT { \
				3100, 3400, 0, 60}
#endif

#if (BAT_CALIBRATION_VOL == 4450)
#define BAT_CHARGE_CFG_DEFAULT { \
				BAT_BACK_CHARGE_MODE, CHARGE_CURRENT_250_MA, CHARGE_VOLTAGE_4_45_V, \
				CHARGE_STOP_CURRENT_10_PERCENT, CHARGE_FULL_STOP_VOL,\
				PRECHARGE_STOP_3_0_V, PRECHARGE_CURRENT_10_PERCENT, \
				60, 300, 420, 10, 300, \
				NO, NTC_THERMISTOR_10K, NTC_PULLUP_INTERNAL}

#define BAT_LEVEL_CFG_DEFAULT { \
				{3600, 3700, 3760, 3800, 3860, 3920, 4000, 4100, 4200, 4300}}
		
#define BAT_LOW_CFG_DEFAULT { \
				3600, 3700, 0, 60}
#endif


/* config isr pin */
static const struct gpio_cfg isr_gpio_cfg = CONFIG_EXT_CHARGER_ISR_GPIO;

#define isr_gpio_mode (GPIO_PULL_UP | GPIO_INPUT | GPIO_INT_DEBOUNCE)


extern s32_t os_sleep(int timeout);

/***************************************************************
*	private functions 
****************************************************************
**/
bool _cw6305_write_bytes(uint8_t reg, uint8_t *data, uint16_t len)
{
	int i2c_op_ret = 0;
	bat_charge_context_t *bat_charge = bat_charge_get_context();
	uint8_t retrycnt = _CW6305_RW_RETRY_CNT;
	uint8_t sendBuf[4];

    sendBuf[0]=reg;
	memcpy(sendBuf + 1, data, len);

	while (retrycnt--) {
		i2c_op_ret = i2c_write(bat_charge->i2c_dev, sendBuf, len+1, CW6305_SLAVER_ADDR);
		if (0 != i2c_op_ret) {
			os_sleep(2);
			LOG_ERR("err = %d\n", i2c_op_ret);
		} else {			
			if(bat_charge->ext_charger_debug_open != 0) {
				LOG_INF("CW6305_W: 0x%x - 0x%x\n", reg, sendBuf[1]);
			}
			return true;
		}
	}

	return false;
}

bool _cw6305_read_bytes(uint8_t reg, uint8_t *value, uint16_t len)
{
	int i2c_op_ret = 0;
	bat_charge_context_t *bat_charge = bat_charge_get_context();
	uint8_t retrycnt = _CW6305_RW_RETRY_CNT;
	uint8_t sendBuf[4];

	sendBuf[0] = reg;	

	while (retrycnt--) {
		i2c_op_ret = i2c_write(bat_charge->i2c_dev, sendBuf, 1, CW6305_SLAVER_ADDR);
		if(0 != i2c_op_ret) {
			os_sleep(2);
			LOG_ERR("err = %d\n", i2c_op_ret);
			if (retrycnt == 1) {
				return false;
			}
		} else {
			break;
		}
	}

	retrycnt = _CW6305_RW_RETRY_CNT;

	while (retrycnt--) {
		i2c_op_ret = i2c_read(bat_charge->i2c_dev, value, len, CW6305_SLAVER_ADDR);
		if(0 != i2c_op_ret) {
			os_sleep(2);
			LOG_ERR("err = %d\n", i2c_op_ret);
		} else {
			if(bat_charge->ext_charger_debug_open != 0) {
				LOG_INF("CW6305_R: 0x%x - 0x%x\n", reg, *value);
			}
			return true;
		}
	}

    return false;
}


int dump_cw6305_all_reg(void)
{
    uint8_t loop;
	uint8_t reg_addr;
	uint8_t reg_value[_CW6305_REG_NUM];

	for(loop=0; loop<_CW6305_REG_NUM; loop++) {
		if(loop >= 0x0C) {
			reg_addr = loop + 1;
		} else {
			reg_addr = loop;
		}
		
		_cw6305_read_bytes(reg_addr, &reg_value[loop], 1);
	}

	return 0;
}

/* reg 01h VBUS_VOLT */
static int _cw6305_charge_start_or_stop(uint8_t start_or_stop)
{
    int ret = 0;
	uint8_t value;

	if(_cw6305_read_bytes(REG_VBUS_VOLT, &value, 1)) {
		if(start_or_stop != 0) {
			LOG_INF("Charge enable!\n");
			value |= VBUS_VOLT_CHG_EN;
		} else {
			LOG_INF("Charge disable!\n");
			value &= ~VBUS_VOLT_CHG_EN;			
		}
		
		if(!_cw6305_write_bytes(REG_VBUS_VOLT, &value, 1)) {
			ret = -1;
		}
	} else {
		ret = -1;
	}

	if(ret != 0) {		
		LOG_ERR("cw6305 set reg01 fail!\n");
	}

	return ret;
}

/* reg 02h VBUS_CUR */
static int _cw6305_set_vbus_cur(uint8_t value)
{
    int ret = 0;

	if(!_cw6305_write_bytes(REG_VBUS_CUR, &value, 1)) {
		LOG_ERR("cw6305 set reg02 fail!\n");
		ret = -1;
	}

	return ret;
}

/* reg 03h CHG_VOLT */
static int _cw6305_set_cv_voltage(uint8_t value)
{
    int ret = 0;
	uint8_t reg_val;

	if(_cw6305_read_bytes(REG_CHG_VOLT, &reg_val, 1)) {
		reg_val &= (~CHG_VOLT_VBREG_MASK);
		reg_val |= (value << CHG_VOLT_VBREG_SHIFT);
		
		if(!_cw6305_write_bytes(REG_CHG_VOLT, &reg_val, 1)) {
			ret = -1;
		}
	} else {
		ret = -1;
	}

	if(ret != 0) {		
		LOG_ERR("cw6305 set cv fail!\n");
	}

	return ret;    
}

static int _cw6305_set_recharge_threshold(uint8_t value)
{
    int ret = 0;
	uint8_t reg_val;

	if(_cw6305_read_bytes(REG_CHG_VOLT, &reg_val, 1)) {
		if(value != 0) {
			reg_val |= CHG_VOLT_VRECHG;   //200mv
		} else {
			reg_val &= ~CHG_VOLT_VRECHG;  //100mv
		}
		if(!_cw6305_write_bytes(REG_CHG_VOLT, &reg_val, 1)) {
			ret = -1;
		}
	} else {
		ret = -1;
	}

	if(ret != 0) {		
		LOG_ERR("cw6305 set recharge fail!\n");
	}

	return ret;    
}

static int _cw6305_set_precharge_voltage(uint8_t value)
{
    int ret = 0;
	uint8_t reg_val;

	if(_cw6305_read_bytes(REG_CHG_VOLT, &reg_val, 1)) {
		if(value != 0) {
			reg_val |= CHG_VOLT_BATLOWV;   //3.0V
		} else {
			reg_val &= ~CHG_VOLT_BATLOWV;  //2.8V
		}
		if(!_cw6305_write_bytes(REG_CHG_VOLT, &reg_val, 1)) {
			ret = -1;
		}
	} else {
		ret = -1;
	}

	if(ret != 0) {		
		LOG_ERR("cw6305 set pre-charge fail!\n");
	}

	return ret;    
}

/* reg 04h CHG_CUR1 */
static int _cw6305_set_cc_current(uint8_t value)
{
    int ret = 0;
	uint8_t reg_val;

	if(_cw6305_read_bytes(REG_CHG_CUR1, &reg_val, 1)) {
		reg_val &= (~CHG_CUR1_ICHG_MASK);
		reg_val |= (value << CHG_CUR1_ICHG_SHIFT);
		
		if(!_cw6305_write_bytes(REG_CHG_CUR1, &reg_val, 1)) {
			ret = -1;
		}
	} else {
		ret = -1;
	}

	if(ret != 0) {		
		LOG_ERR("cw6305 set cc fail!\n");
	}

	return ret;    
}


/* reg 05h CHG_CUR2 */
static int _cw6305_set_precharge_current(uint8_t value)
{
    int ret = 0;
	uint8_t reg_val;

	if(_cw6305_read_bytes(REG_CHG_CUR2, &reg_val, 1)) {
		reg_val &= (~PRECHG_SEL_MASK);
		reg_val |= (value << PRECHG_SEL_SHIFT);
		
		if(!_cw6305_write_bytes(REG_CHG_CUR2, &reg_val, 1)) {
			ret = -1;
		}
	} else {
		ret = -1;
	}

	if(ret != 0) {		
		LOG_ERR("cw6305 set pre_chg current fail!\n");
	}

	return ret;    
}

static int _cw6305_set_term_current(uint8_t value)
{
    int ret = 0;
	uint8_t reg_val;

	if(_cw6305_read_bytes(REG_CHG_CUR2, &reg_val, 1)) {
		reg_val &= (~TERM_SEL_MASK);
		reg_val |= (value << TERM_SEL_SHIFT);
		
		if(!_cw6305_write_bytes(REG_CHG_CUR2, &reg_val, 1)) {
			ret = -1;
		}
	} else {
		ret = -1;
	}

	if(ret != 0) {		
		LOG_ERR("cw6305 set term current fail!\n");
	}

	return ret;    
}


/* reg 06h -- CHG_SAFETY */
static int _cw6305_reset_all_reg(void)
{
    int ret = 0;
	uint8_t reg_val;

	if(_cw6305_read_bytes(REG_CHG_SAFETY, &reg_val, 1)) {
		reg_val |= REG_RESET;
		
		if(!_cw6305_write_bytes(REG_CHG_SAFETY, &reg_val, 1)) {
			ret = -1;
		}
	} else {
		ret = -1;
	}

	if(ret != 0) {		
		LOG_ERR("cw6305 reset fail!\n");
	}

	return ret;    
}

static int _cw6305_set_charge_timer(uint8_t value)
{
    int ret = 0;
	uint8_t reg_val;

	if(_cw6305_read_bytes(REG_CHG_SAFETY, &reg_val, 1)) {
		reg_val &= ~CHG_TIMER_MASK;
		reg_val |= (value << CHG_TIMER_SHIFT);
		
		if(!_cw6305_write_bytes(REG_CHG_SAFETY, &reg_val, 1)) {
			ret = -1;
		}
	} else {
		ret = -1;
	}

	if(ret != 0) {		
		LOG_ERR("cw6305 set charge timer fail!\n");
	}

	return ret;    
}

static int _cw6305_set_NTC(void)
{
    int ret = 0;
	uint8_t reg_val, value = 0;
    bat_charge_context_t *bat_charge = bat_charge_get_context();
	bat_charge_configs_t *configs = &bat_charge->configs;

	if(configs->cfg_charge.NTC_Func_Enable) {
		value |= NTC_EN;
	}

	if(configs->cfg_charge.NTC_Thermistor != NTC_THERMISTOR_10K) {
		value |= NTC_SEL;
	}	

	if(configs->cfg_charge.NTC_Ext_PullUp != NTC_PULLUP_INTERNAL) {
		value |= NTC_NET_SEL;
	}		

	if(_cw6305_read_bytes(REG_CHG_SAFETY, &reg_val, 1)) {
		reg_val &= ~(NTC_EN | NTC_SEL | NTC_NET_SEL);
		reg_val |= value ;
		
		if(!_cw6305_write_bytes(REG_CHG_SAFETY, &reg_val, 1)) {
			ret = -1;
		}
	} else {
		ret = -1;
	}

	if(ret != 0) {		
		LOG_ERR("cw6305 set NTC fail!\n");
	}

	return ret;    
}

/* reg 08h SAFETY_CFG */
static int _cw6305_enable_watchdog_timer(uint8_t stage, uint8_t enable)
{
    int ret = 0;
	uint8_t reg_val;

	if(_cw6305_read_bytes(REG_SAFETY_CFG, &reg_val, 1)) {
		if(stage == 0) {
			//during charging 
			reg_val &= ~WD_CHG;
			if(enable != 0) {
				reg_val |= WD_CHG;
			}
		} else {
			//during other condition
			reg_val &= ~WD_DISCHG;
			if(enable != 0) {
				reg_val |= WD_DISCHG;
			}			
		}
		
		if(!_cw6305_write_bytes(REG_SAFETY_CFG, &reg_val, 1)) {
			ret = -1;
		}
	} else {
		ret = -1;
	}

	if(ret != 0) {		
		LOG_ERR("cw6305 enable wd timer fail!\n");
	}

	return ret;    
}


static int _cw6305_set_watchdog_timer(uint8_t wd_timer_s)
{
    int ret = 0;
	uint8_t reg_val;

	if(_cw6305_read_bytes(REG_SAFETY_CFG, &reg_val, 1)) {
		reg_val &= ~WATCH_TIMER_MASK;
		reg_val |= (wd_timer_s << WATCH_TIMER_SHIFT);
		
		if(!_cw6305_write_bytes(REG_SAFETY_CFG, &reg_val, 1)) {
			ret = -1;
		}
	} else {
		ret = -1;
	}

	if(ret != 0) {		
		LOG_ERR("cw6305 set wd timer fail!\n");
	}

	return ret;    
}

static int _cw6305_set_bat_UVLO(uint8_t value)
{
    int ret = 0;
	uint8_t reg_val;

	if(_cw6305_read_bytes(REG_SAFETY_CFG, &reg_val, 1)) {
		reg_val &= ~BAT_UVLO_MASK;
		reg_val |= (value << BAT_UVLO_SHIFT);
		
		if(!_cw6305_write_bytes(REG_SAFETY_CFG, &reg_val, 1)) {
			ret = -1;
		}
	} else {
		ret = -1;
	}

	if(ret != 0) {		
		LOG_ERR("cw6305 set UVLO fail!\n");
	}

	return ret;    
}

/* reg 09h SAFETY_CFG2 */
static int _cw6305_set_sys_OCP(uint8_t value)
{
    int ret = 0;
	uint8_t reg_val;

	if(_cw6305_read_bytes(REG_SAFETY_CFG2, &reg_val, 1)) {
		reg_val &= ~SYS_OCP_MASK;
		reg_val |= (value << SYS_OCP_SHIFT);
		
		if(!_cw6305_write_bytes(REG_SAFETY_CFG2, &reg_val, 1)) {
			ret = -1;
		}
	} else {
		ret = -1;
	}

	if(ret != 0) {		
		LOG_ERR("cw6305 set OCP fail!\n");
	}

	return ret;    
}

/* reg 0ah INT_SET */
static int _cw6305_set_irq_enable(uint8_t mask_bitmap, uint8_t set_bitmap)
{
    int ret = 0;
	uint8_t reg_val;

	if(_cw6305_read_bytes(REG_INT_SET, &reg_val, 1)) {
		reg_val &= ~mask_bitmap;
		reg_val |= set_bitmap;
		
		if(!_cw6305_write_bytes(REG_INT_SET, &reg_val, 1)) {
			ret = -1;
		}
	} else {
		ret = -1;
	}

	if(ret != 0) {		
		LOG_ERR("cw6305 set irq enable fail!\n");
	}

	return ret;    
}

/* reg 0bh INT_SRC */
static int _cw6305_clear_irq_source(uint8_t clr_bitmap)
{
    int ret = 0;
	uint8_t reg_val;

	if(_cw6305_read_bytes(REG_INT_SRC, &reg_val, 1)) {
		reg_val &= ~clr_bitmap;

		if(!_cw6305_write_bytes(REG_INT_SRC, &reg_val, 1)) {
			ret = -1;
		}
	} else {
		ret = -1;
	}

	if(ret != 0) {		
		LOG_ERR("cw6305 clr irq source fail!\n");
	}

	return ret;    
}

/* reg 0dh IC_STATUS */
static int _cw6305_get_status(uint8_t *status)
{
    int ret = 0;

	if(!_cw6305_read_bytes(REG_IC_STATUS, status, 1)) {
		ret = -1;
	}

	if(ret != 0) {		
		LOG_ERR("cw6305 get status fail!\n");
	}

	return ret;    
}

/* reg 0eh IC_STATUS2 */
static int _cw6305_get_status_II(uint8_t *status)
{
    int ret = 0;

	if(!_cw6305_read_bytes(REG_IC_STATUS2, status, 1)) {
		ret = -1;
	}

	if(ret != 0) {		
		LOG_ERR("cw6305 get status II fail!\n");
	}

	return ret;    
}

static int i2c_async_write_cb(void *cb_data, struct i2c_msg *msgs,
					uint8_t num_msgs, bool is_err)
{
	if (is_err) {
		LOG_ERR("cw6305 i2c write err\n");
	}
	return 0;
}

static int i2c_async_read_cb(void *cb_data, struct i2c_msg *msgs,
					uint8_t num_msgs, bool is_err)
{
    bat_charge_context_t *bat_charge = bat_charge_get_context();
	uint8_t int_src_value = msgs->buf[0];
	
	if (!is_err) {
		LOG_INF("cw6305 irq src: 0x%x\n", int_src_value);

        //chg_det
		if((int_src_value & INT_CHG_DET) != 0) {
			bat_charge->dc5v_current_state = DC5V_STATE_IN;
			LOG_INF("irq: dc5v_in!");
		}

	    //chg_remove
		if((int_src_value & INT_CHG_REMOVE) != 0) {
			bat_charge->dc5v_current_state = DC5V_STATE_OUT;
			LOG_INF("irq: dc5v_out!");
		} 

		//chg_finish
		if((int_src_value & INT_CHG_FINISH) != 0) {
			bat_charge->is_charge_finished = 1;
			LOG_INF("irq: chg_finish!");
		}

		//over temperature
		if((int_src_value & INT_BAT_OT) != 0) {
			LOG_INF("irq: over_temp!");
		}

		//under temperature
		if((int_src_value & INT_BAT_UT) != 0) {
			LOG_INF("irq: under_temp!");
		}		

        //safety_timer expired
		if((int_src_value & INT_SAFETY_TIMER) != 0) {
			bat_charge->safety_timer_expired = 1;
			LOG_INF("irq: safety_timer_expired!");
		}

		//pre-charge tiimer expired
		if((int_src_value & INT_PRECHG_TIMER) != 0) {
			bat_charge->prechg_timer_expired = 1;
			LOG_INF("irq: prechg_timer_expired!");
		}

        bat_charge->irq_pending_bitmap = int_src_value;
		
	} else {
		LOG_ERR("i2c read err\n");
	}
	return 0;
}


static void _cw6305_irq_callback(const struct device *port, struct gpio_callback *cb, uint32_t pins)
{
	static uint8_t write_cmd[1] = {0};
	static uint8_t read_cmd[1] = {0};
	bat_charge_context_t *bat_charge = bat_charge_get_context();	
	int ret = 0;

    LOG_INF("cw6305_irq!!\n");

	write_cmd[0] = REG_INT_SRC;
	ret = i2c_write_async(bat_charge->i2c_dev, write_cmd, 1, CW6305_SLAVER_ADDR, i2c_async_write_cb, NULL);
	if (ret == -1) {
		LOG_ERR("cw6305 i2c async write err!\n");
		return;
	}

	ret = i2c_read_async(bat_charge->i2c_dev, read_cmd, 1, CW6305_SLAVER_ADDR, i2c_async_read_cb, NULL);
	if (ret == -1) {
		LOG_ERR("cw6305 i2c async read err!\n");
	}

	return;
}


static void _cw6305_irq_config(void)
{
    bat_charge_context_t *bat_charge = bat_charge_get_context();	

	bat_charge->isr_gpio_dev = device_get_binding(isr_gpio_cfg.gpio_dev_name);
	gpio_pin_configure(bat_charge->isr_gpio_dev, isr_gpio_cfg.gpion, isr_gpio_mode);	
	gpio_pin_interrupt_configure(bat_charge->isr_gpio_dev, isr_gpio_cfg.gpion, GPIO_INT_EDGE_FALLING);
	gpio_init_callback(&bat_charge->ext_charger_gpio_cb , _cw6305_irq_callback, BIT(isr_gpio_cfg.gpion));
	gpio_add_callback(bat_charge->isr_gpio_dev , &bat_charge->ext_charger_gpio_cb);

}

static void _cw6305_get_vbus_init(void)
{
    bat_charge_context_t *bat_charge = bat_charge_get_context();
	uint8_t value;

	if(_cw6305_read_bytes(REG_INT_SRC, &value, 1)) {
		LOG_INF("get vbus init by irq!");
        //chg_det
		if((value & INT_CHG_DET) != 0) {
			bat_charge->dc5v_current_state = DC5V_STATE_IN;
		}

	    //chg_remove
		if((value & INT_CHG_REMOVE) != 0) {
			bat_charge->dc5v_current_state = DC5V_STATE_OUT;
		}

		//LOG_INF("dc5v init: %d <%s>", bat_charge->dc5v_current_state, get_dc5v_state_str(bat_charge->dc5v_current_state));
	}

	if((bat_charge->dc5v_current_state != DC5V_STATE_IN) && (bat_charge->dc5v_current_state != DC5V_STATE_OUT)) {
		LOG_INF("get vbus init by status!");
		if(_cw6305_read_bytes(REG_IC_STATUS2, &value, 1)) {
			if((value & REPORT_VBUS_PG) != 0) {
				bat_charge->dc5v_current_state = DC5V_STATE_IN;
			} else {
				bat_charge->dc5v_current_state = DC5V_STATE_OUT;
			}
		}
	}

	LOG_INF("dc5v init: %d <%s>", bat_charge->dc5v_current_state, get_dc5v_state_str(bat_charge->dc5v_current_state));

}


static int _cw6305_init(void)
{
    bat_charge_context_t *bat_charge = bat_charge_get_context();
	bat_charge_configs_t *configs = &bat_charge->configs;

    bat_charge->ext_charger_debug_open = 0;

    _cw6305_reset_all_reg();

	//config all register
	_cw6305_set_vbus_cur(VBUS_CUR_VSYS_REG_SET(PARAM_VSYS_REG_VOL) | VBUS_CUR_VBUS_ILIM_SET(PARAM_VBUS_ILIM));

	_cw6305_set_cv_voltage(configs->cfg_charge.Charge_Voltage);
	_cw6305_set_recharge_threshold(RECHARGE_THRD_200MV);
	_cw6305_set_precharge_voltage(configs->cfg_charge.Precharge_Stop_Voltage);

	_cw6305_set_cc_current(configs->cfg_charge.Charge_Current);

	_cw6305_set_precharge_current(configs->cfg_charge.Precharge_Current);
	_cw6305_set_term_current(configs->cfg_charge.Charge_Stop_Current);

	_cw6305_set_charge_timer(PARAM_CHG_TIMER);

	_cw6305_enable_watchdog_timer(0, 1);               //enable charge timer
	_cw6305_enable_watchdog_timer(1, 0);               //disable dis-charge timer
	_cw6305_set_watchdog_timer(PARAM_WD_TIMER);
	_cw6305_set_bat_UVLO(PARAM_BAT_UVLO);

	_cw6305_set_sys_OCP(PARAM_SYS_OCP);

	_cw6305_get_vbus_init();

	_cw6305_clear_irq_source(0xff);                    //clear all irq source

	//config irq
	_cw6305_irq_config();

	//irq set
	if(configs->cfg_charge.NTC_Func_Enable == NO) {
		_cw6305_set_irq_enable(MASK_CHG_OV|MASK_BAT_OT|MASK_BAT_UT, \
			MASK_CHG_DET|MASK_CHG_REMOVE|MASK_CHG_FINISH|MASK_SAFETY_TIMER|MASK_PRECHG_TIMER);
	} else {
		_cw6305_set_NTC();
		_cw6305_set_irq_enable(MASK_CHG_OV, \
			MASK_CHG_DET|MASK_CHG_REMOVE|MASK_CHG_FINISH|MASK_SAFETY_TIMER|MASK_PRECHG_TIMER|MASK_BAT_OT|MASK_BAT_UT);		
	}

	bat_charge->irq_pending_bitmap = 0;

    bat_charge->ext_charger_debug_open = 1;
	
	dump_cw6305_all_reg();	

	bat_charge->ext_charger_debug_open = 0;

	return 0;
}

void _cw6305_dump_cfg_info(void)
{
	bat_charge_context_t *bat_charge = bat_charge_get_context();
	bat_charge_configs_t *configs = &bat_charge->configs;

	LOG_INF("Bat Config Start:");
	LOG_INF("Charge Mode: %s", (configs->cfg_charge.Select_Charge_Mode == BAT_BACK_CHARGE_MODE) ? "back" : "front");

	if(configs->cfg_charge.Charge_Current < CHARGE_CURRENT_80_MA) {
		LOG_INF("CC little than 80ma!");
	} else {
		LOG_INF("CC:%dma!", 10*(configs->cfg_charge.Charge_Current - CHARGE_CURRENT_80_MA)+80);
	}

	if(configs->cfg_charge.Charge_Voltage < CHARGE_VOLTAGE_4_20_V) {
		LOG_INF("CV little than 4200mv!");
	} else if(configs->cfg_charge.Charge_Voltage > CHARGE_VOLTAGE_4_60_V) {
		LOG_INF("CV is 4600mv!");
	} else {
		LOG_INF("CV:%dmv!", (4200 * 10 + (configs->cfg_charge.Charge_Voltage - CHARGE_VOLTAGE_4_20_V) * 125) / 10);
	}

	LOG_INF("Stop: %d, %dmv", configs->cfg_charge.Charge_Stop_Current, \
		configs->cfg_charge.Charge_Stop_Voltage);

	LOG_INF("Pre-Charge: %d, %dmv", configs->cfg_charge.Precharge_Current, \
		(configs->cfg_charge.Precharge_Stop_Voltage == PRECHARGE_STOP_3_0_V)?3000:2800);

	LOG_INF("Check_Period: %ds", configs->cfg_charge.Battery_Check_Period_Sec);
	LOG_INF("Charge_Check_Period: %ds", configs->cfg_charge.Charge_Check_Period_Sec);
	LOG_INF("Full_Continue: %ds", configs->cfg_charge.Charge_Full_Continue_Sec);
	LOG_INF("Poff_Wait: %ds", configs->cfg_charge.Front_Charge_Full_Power_Off_Wait_Sec);
	LOG_INF("DC5V_Dbs: %dms", configs->cfg_charge.DC5V_Detect_Debounce_Time_Ms);

	LOG_INF("NTC Func: %d", configs->cfg_charge.NTC_Func_Enable);
	LOG_INF("NTC_Thermistor: %dK", (configs->cfg_charge.NTC_Thermistor ==NTC_THERMISTOR_10K)?10:100);
	LOG_INF("NTC_Ext_PullUp: %s", (configs->cfg_charge.NTC_Ext_PullUp == NTC_PULLUP_EXTERNAL) ? "Ext" : "Int");
	
	LOG_INF("Bat Config End!");
}



/*************************************************************************
*	ext charger output api
**************************************************************************
**/
int ext_charger_init(void)
{
    LOG_INF("cw6305 init start!\n");

	_cw6305_init();

    LOG_INF("cw6305 init end!\n");

	return 0;
}

int ext_charger_enable(void)
{
	int ret;
	bat_charge_context_t *bat_charge = bat_charge_get_context();
	
	ret = _cw6305_charge_start_or_stop(CHARGE_START);
	
    if(ret == 0) {
		bat_charge->charger_enabled = 1;
		
		if (bat_charge->callback) {
        	bat_charge->callback(BAT_CHG_EVENT_CHARGE_START, NULL);
		}
    }
	
	return ret;
}

int ext_charger_disable(void)
{
	int ret;
	bat_charge_context_t *bat_charge = bat_charge_get_context();

	LOG_INF("ext charger disable start!");
	ret = _cw6305_charge_start_or_stop(CHARGE_STOP);

    if(ret == 0) {
		bat_charge->charger_enabled = 0;

		if (bat_charge->callback) {
        	bat_charge->callback(BAT_CHG_EVENT_CHARGE_STOP, NULL);
		}
    }	

	LOG_INF("ext charger disable end: %d", ret);

	return ret;
}

int ext_charger_get_config(const struct device *dev)
{
    bat_charge_context_t *bat_charge = bat_charge_get_context();
	bat_charge_configs_t *configs = &bat_charge->configs;

	CFG_Struct_Battery_Charge cfg_charge = BAT_CHARGE_CFG_DEFAULT;
	CFG_Struct_Battery_Level cfg_level = BAT_LEVEL_CFG_DEFAULT;
	CFG_Struct_Battery_Low cfg_low = BAT_LOW_CFG_DEFAULT;	

	memcpy(&configs->cfg_charge, &cfg_charge, sizeof(CFG_Struct_Battery_Charge));
	memcpy(&configs->cfg_bat_level, &cfg_level, sizeof(CFG_Struct_Battery_Level));
	memcpy(&configs->cfg_bat_low, &cfg_low, sizeof(CFG_Struct_Battery_Low));

	configs->cfg_features = SYS_FORCE_CHARGE_WHEN_DC5V_IN;

	_cw6305_dump_cfg_info();

	return 0;
}

int ext_charger_get_state(void)
{
    uint8_t status1, status2;
    bat_charge_context_t *bat_charge = bat_charge_get_context();	
    int ret;

    ret = _cw6305_get_status(&status1);
	if(ret != 0) {
		return ret;
	}

	ret = _cw6305_get_status_II(&status2);
	if(ret != 0) {
		return ret;
	}	

    if(bat_charge->dc5v_current_state != DC5V_STATE_IN) {
		if((status2 & REPORT_CHRG_STAT) == 0) {
			ret = BAT_CHG_STATE_NORMAL;
		} else {
			LOG_ERR("the CHRG_STAT error!\n");
			ret = -1;
		}
	} else {
		if(bat_charge->bat_real_volt < BAT_CHECK_EXIST_THRESHOLD) {
			ret = BAT_CHG_STATE_LOW;
		} /*else if(bat_charge->bat_real_volt >= bat_charge->configs.cfg_charge.Charge_Stop_Voltage) {
			ret = BAT_CHG_STATE_FULL;
		} */else if((status2 & REPORT_CHRG_STAT) == 0) {
			ret = BAT_CHG_STATE_NORMAL;
	    } else if((status1 & REPORT_CHG_FINISH) != 0) {
	    	ret = BAT_CHG_STATE_FULL;
		} else if((status1 & REPORT_PRE_CHG) != 0) {
			ret = BAT_CHG_STATE_PRECHARGE;
		} else if((status1 & REPORT_CV_STAT) != 0) {
			ret = BAT_CHG_STATE_CHARGE;
		} else if((status1 & REPORT_CC_STAT) != 0) {
			ret = BAT_CHG_STATE_CHARGE;
		} else {
			if(bat_charge->bat_charge_current_state != BAT_CHG_STATE_FULL) {
				ret = BAT_CHG_STATE_NORMAL;
			} else {
				ret = BAT_CHG_STATE_FULL;
			}
		}
	}

	return ret;
}


int ext_charger_get_errno(void)
{
    uint8_t status2;
	int ret;

	ret = _cw6305_get_status_II(&status2);
	if(ret != 0) {
		return ret;
	}

	if((status2 & REPORT_VBUS_PG) == 0) {
		//ret = ERROR_VBUS_PG;
		ret = 0;
	} else if((status2 & REPORT_VBUS_OV) != 0) {
		ret = ERROR_VBUS_OV;
	} else if((status2 & REPORT_IC_OT) != 0) {
		ret = ERROR_IC_OT;
	} else if((status2 & REPORT_BAT_OT) != 0) {
		ret = ERROR_BAT_OT;
	} else if((status2 & REPORT_BAT_UT) != 0) {
		ret = ERROR_BAT_UT;
	} else if((status2 & REPORT_SAFETY_TIMER_EXP) != 0) {
		ret = ERROR_SAFETY_TM_EXPIR;
	} else if((status2 & REPORT_PRECHG_TIMER_EXP) != 0) {
		ret = ERROR_PRECHG_TM_EXPIR;
	} else {
		ret = ERROR_NOERR;
	}

	return ret;
}

int ext_charger_clear_irq_pending(uint8_t clr_bitmap)
{
    LOG_INF("cw6305_clr: 0x%x", clr_bitmap);
    return _cw6305_clear_irq_source(clr_bitmap);
}


int ext_charger_dump_info1(void)
{
    uint8_t status1;
    int ret;

    ret = _cw6305_get_status(&status1);
	if(ret != 0) {
		return ret;
	}

	return status1;
}

int ext_charger_dump_info2(void)
{
    uint8_t status2;
    int ret;

    ret = _cw6305_get_status_II(&status2);
	if(ret != 0) {
		return ret;
	}

	return status2;
}

int ext_charger_check_enable(void)
{
    int ret = 0;
    uint8_t value;

	if(!_cw6305_read_bytes(REG_VBUS_VOLT, &value, 1)) {
		LOG_ERR("cw6305 read reg01 fail!\n");
		return -1;
	}

    if((value & VBUS_VOLT_CHG_EN) != 0) {
		//charge is enabled
		ret = 1;
    }
		
	return ret;
}


