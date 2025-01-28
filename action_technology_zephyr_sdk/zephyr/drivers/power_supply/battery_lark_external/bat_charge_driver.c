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
LOG_MODULE_REGISTER(bat_drv, CONFIG_LOG_DEFAULT_LEVEL);


bat_charge_context_t bat_charge_context;


int bat_charge_disable(void)
{
    int ret = 0;
    bat_charge_context_t *bat_charge = bat_charge_get_context();

    if(bat_charge->charger_enabled != 0) {
        ret = ext_charger_disable();
    }

    return ret;
}

int bat_charge_enable(void)
{
    int ret = 0;
    bat_charge_context_t *bat_charge = bat_charge_get_context();

    if(bat_charge->charger_enabled == 0) {
        ret = ext_charger_enable();
    }

    return ret;
}

void bat_check_real_voltage(uint32_t sample_sec)
{
    bat_charge_context_t *bat_charge = bat_charge_get_context();

    /* disable charger */
    bat_charge_disable();

    /* update the wait time counter */
    bat_charge->bat_volt_check_wait_count =
        sample_sec * 1000 / BAT_CHARGE_DRIVER_TIMER_MS;

    bat_charge->need_check_bat_real_volt = 1;
}


void charger_enable_timer_delete(void)
{
    bat_charge_context_t *bat_charge = bat_charge_get_context();

    k_delayed_work_cancel(&bat_charge->charger_enable_timer);
}

void charger_enable_timer_handler(struct k_work *work)
{
    charger_enable_timer_delete();

    ext_charger_enable();
}

const char* get_bat_state_str(uint8_t state)
{
    static const char* const  str[] =
    {
        "INIT",
        "LOW",
        "NORMAL",
        "PRE-CHG",
        "CHGING",
        "FULL",
        "UNKNOWN"
    };

    if(state >= 6) {
        state = 6;
    }

    return str[state];
}

/* get DC5V stat format string */
const char* get_dc5v_state_str(uint8_t state)
{
    static const char* const  str[] =
    {
        "OUT",
        "IN",
        "UNKNOWN"
    };

    if(state >= 2) {
        state = 2;
    }

    return str[state];
}

dc5v_state_t get_dc5v_current_state(void)
{
    bat_charge_context_t *bat_charge = bat_charge_get_context();

    if(bat_charge->dc5v_current_state == DC5V_STATE_IN) {
        if(bat_charge->dc5v_last_state != DC5V_STATE_IN) {
            //plug in
            if(bat_charge->charger_enabled == 0) {
                /* delay 500ms to enable charger */
                k_delayed_work_submit(&bat_charge->charger_enable_timer, K_MSEC(500));
            }

            //clear bat low status
            bat_charge->bat_volt_low	 = 0;
            bat_charge->bat_volt_low_ex  = 0;
            bat_charge->bat_volt_too_low = 0;

            if (bat_charge->callback) {
                bat_charge->callback(BAT_CHG_EVENT_DC5V_IN, NULL);
            }

            if(bat_charge->bat_charge_current_state == BAT_CHG_STATE_FULL) {
                /* DC5V plug in when battery power full */
                if (bat_charge->callback) {
                    bat_charge->callback(BAT_CHG_EVENT_BATTERY_FULL, NULL);
                }
            }
        }
    } else {
        if (bat_charge->dc5v_last_state == DC5V_STATE_IN) {

            /* disable charger enable timer */
            charger_enable_timer_delete();

            /* disable charger anyway */
            if (bat_charge->charger_enabled != 0) {
                ext_charger_disable();
            }

            if (bat_charge->callback) {
                bat_charge->callback(BAT_CHG_EVENT_DC5V_OUT, NULL);
            }
        }
    }

    if (bat_charge->dc5v_last_state != bat_charge->dc5v_current_state) {
        bat_charge->dc5v_last_state = bat_charge->dc5v_current_state;
        LOG_INF("dc5v:%d <%s>", bat_charge->dc5v_current_state, get_dc5v_state_str(bat_charge->dc5v_current_state));
    }

    return bat_charge->dc5v_current_state;
}


/**
**	check if battery voltage is low power
**/
void bat_check_voltage_low(uint32_t bat_volt)
{
    bat_charge_context_t *bat_charge = bat_charge_get_context();

    if (bat_volt <= bat_charge->configs.cfg_bat_low.Battery_Too_Low_Voltage) {
        if (!bat_charge->bat_volt_too_low) {
            bat_charge->bat_volt_low     = true;
            bat_charge->bat_volt_low_ex  = true;
            bat_charge->bat_volt_too_low = true;
            if (bat_charge->callback) {
                bat_charge->callback(BAT_CHG_EVENT_BATTERY_TOO_LOW, NULL);
            }
        }
    } else {
        bat_charge->bat_volt_too_low = 0;
    }

    if (bat_volt <= bat_charge->configs.cfg_bat_low.Battery_Low_Voltage_Ex) {
        if (!bat_charge->bat_volt_low_ex) {
            bat_charge->bat_volt_low    = true;
            bat_charge->bat_volt_low_ex = true;

            if (bat_charge->callback) {
                bat_charge->callback(BAT_CHG_EVENT_BATTERY_LOW_EX, NULL);
            }
        }
    } else {
        /* lower voltage configuration may inexistence */
        bat_charge->bat_volt_low_ex = 0;
    }

    if (bat_volt <= bat_charge->configs.cfg_bat_low.Battery_Low_Voltage) {
        if (!bat_charge->bat_volt_low) {
            bat_charge->bat_volt_low = true;
            if (bat_charge->callback) {
                bat_charge->callback(BAT_CHG_EVENT_BATTERY_LOW, NULL);
            }
        }
    } else {
        bat_charge->bat_volt_low     = 0;
        bat_charge->bat_volt_low_ex  = 0;
        bat_charge->bat_volt_too_low = 0;
    }
}


/**
**	check battery voltage change
**/
uint32_t bat_check_voltage_change(uint32_t bat_volt)
{
    bat_charge_context_t *bat_charge = bat_charge_get_context();

    LOG_INF("bat_volt:%d bat_real_volt:%d", bat_volt, bat_charge->bat_real_volt);
    LOG_INF("current_state: %d <%s>", bat_charge->bat_charge_current_state, get_bat_state_str(bat_charge->bat_charge_current_state));

    /* battery voltage can not decrease during charging and pre-charging  */
    if((bat_charge->dc5v_current_state == DC5V_STATE_IN) && ((bat_charge->bat_charge_current_state == BAT_CHG_STATE_PRECHARGE) || \
		  (bat_charge->bat_charge_current_state == BAT_CHG_STATE_CHARGE)) && \
          (bat_charge->bat_exist_state == 1)) {
        if (bat_volt < bat_charge->bat_real_volt) {
			LOG_INF("Vol can not decrease in charge: %dmv--%dmv", bat_volt, bat_charge->bat_real_volt);
            bat_volt = bat_charge->bat_real_volt;
        }
    } else {
        /* battery voltage can not increase without charging */
        if (bat_volt > bat_charge->bat_real_volt) {
			LOG_INF("Vol can not increase in discharge: %dmv--%dmv", bat_volt, bat_charge->bat_real_volt);
            bat_volt = bat_charge->bat_real_volt;
        }
    }

    if (bat_charge->bat_real_volt != bat_volt) {
        LOG_INF("bat real vol change: %d -- %d", bat_charge->bat_real_volt, bat_volt);
        bat_charge->bat_real_volt = bat_volt;
    }

    if (bat_charge->dc5v_current_state != DC5V_STATE_IN) {
        /* check if battery is low power when DC5V not plug-in */
        bat_check_voltage_low(bat_volt);
    }

    return bat_volt;
}


/**
**	get battery voltage in millivolt
**/
uint32_t get_battery_voltage_mv(void)
{
    bat_charge_context_t *bat_charge = bat_charge_get_context();

    return bat_charge->bat_real_volt;
}



#ifndef CONFIG_ACTS_BATTERY_SUPPLY_EXT_COULOMETER

int bat_adc_get_sample(void)
{
    int ret;
    bat_charge_context_t *bat_charge = bat_charge_get_context();
    const struct device *dev = bat_charge->this_dev;

    if ((!dev) || (!bat_charge->adc_dev)) {
        LOG_ERR("no device!!!");
        return -ENXIO;
    }

    ret = adc_read(bat_charge->adc_dev, &bat_charge->bat_sequence);
    if (ret) {
        LOG_ERR("battery ADC read error %d", ret);
        return -EIO;
    }

    ret = sys_get_le16(bat_charge->bat_sequence.buffer);

    //LOG_INF("battery ADC: %d", ret);

    return ret;
}

void bat_adc_sample_buf_put(uint32_t bat_adc)
{
    bat_charge_context_t *bat_charge = bat_charge_get_context();

    int buf_size = BAT_ADC_SAMPLE_BUF_SIZE;

    bat_charge->bat_adc_sample_buf[bat_charge->bat_adc_sample_index] = bat_adc;
    bat_charge->bat_adc_sample_index += 1;

    if (bat_charge->bat_adc_sample_index >= buf_size)
    {
        bat_charge->bat_adc_sample_index = 0;
    }
}

void bat_adc_sample_buf_init(void)
{
    bat_charge_context_t *bat_charge = bat_charge_get_context();
    int  buf_size = BAT_ADC_SAMPLE_BUF_SIZE;
    int  i;

    /* sample BAT ADC data in 10ms */
    for (i = 0; i < buf_size; i++)
    {
        bat_charge->bat_adc_sample_buf[i] = bat_adc_get_sample();

        k_usleep(10000 / buf_size);
    }

    bat_charge->bat_adc_sample_timer_count = 0;
    bat_charge->bat_adc_sample_index = 0;
}

/* convert from battery ADC value to voltage in millivolt */
uint32_t bat_adc_get_voltage_mv(uint32_t adc_val)
{
    uint32_t mv;

    if (!adc_val) {
        return 0;
    }

    //use DC5VADC to detect battery voltage
    mv = adc_val * 6000 / 4096;

    return mv;
}

static void bat_volt_selectsort(uint16_t *arr, uint16_t len)
{
    uint16_t i, j, min;

    for (i = 0; i < len-1; i++)	{
        min = i;
        for (j = i+1; j < len; j++) {
            if (arr[min] > arr[j])
                min = j;
        }
        /* swap */
        if (min != i) {
            arr[min] ^= arr[i];
            arr[i] ^= arr[min];
            arr[min] ^= arr[i];
        }
    }
}

void bat_check_battery_exist(uint32_t bat_volt)
{
    bat_charge_context_t *bat_charge = bat_charge_get_context();

    if(bat_charge->dc5v_current_state != DC5V_STATE_IN) {
        bat_charge->bat_exist_state = 1;
    } else {
        if(bat_volt < BAT_CHECK_EXIST_THRESHOLD) {
            LOG_ERR("battery is not exist: %dmv\n", bat_volt);
            bat_charge->bat_exist_state = 0;
        } else {
            bat_charge->bat_exist_state = 1;
        }
    }
}


/* get the average battery voltage during specified time */
uint32_t get_battery_voltage_by_time(uint32_t sec)
{
    bat_charge_context_t *bat_charge = bat_charge_get_context();
    int  buf_size = BAT_ADC_SAMPLE_BUF_SIZE;
    int  index = bat_charge->bat_adc_sample_index;
    int  adc_count = (sec * 1000 / BAT_ADC_SAMPLE_INTERVAL_MS);
    int  bat_adc, volt_mv, i;
    uint16_t tmp_buf[BAT_ADC_SAMPLE_BUF_SIZE] = {0};

    for (i = 0; i < adc_count; i++) {
        if (index > 0) {
            index -= 1;
        } else {
            index = buf_size - 1;
        }

        tmp_buf[i] = bat_charge->bat_adc_sample_buf[index];
    }

    /* sort the ADC data by ascending sequence */
    bat_volt_selectsort(tmp_buf, adc_count);

    /* get the last 4/5 position data */
    bat_adc = tmp_buf[adc_count * 4/5 - 1];

    volt_mv = bat_adc_get_voltage_mv(bat_adc);

    return volt_mv;
}


/**
**	get the percent of battery capacity
**/
uint32_t get_battery_percent(void)
{
    bat_charge_context_t *bat_charge = bat_charge_get_context();

    uint32_t bat_volt = get_battery_voltage_mv();
    int i;
    uint32_t level_begin;
    uint32_t level_end;
    uint32_t percent;

    if (bat_volt >= bat_charge->configs.cfg_charge.Charge_Stop_Voltage) {
        if(bat_charge->bat_charge_current_state == BAT_CHG_STATE_CHARGE) {
            LOG_INF("not full, soc display 99: %dmv", bat_volt);
            return 99;
        } else {
            return 100;
        }
    }

    for (i = CFG_MAX_BATTERY_LEVEL - 1; i > 0; i--) {
        if (bat_volt >= bat_charge->configs.cfg_bat_level.Level[i])
        {
            break;
        }
    }

    level_begin = bat_charge->configs.cfg_bat_level.Level[i];

    if (i < CFG_MAX_BATTERY_LEVEL - 1) {
        level_end = bat_charge->configs.cfg_bat_level.Level[i+1];
    } else {
        level_end = bat_charge->configs.cfg_charge.Charge_Stop_Voltage;
    }

    if (bat_volt > level_end) {
        bat_volt = level_end;
    }

    percent = 100 * i / CFG_MAX_BATTERY_LEVEL;

    if (bat_volt >  level_begin &&
        bat_volt <= level_end) {
        percent += 10 * (bat_volt - level_begin) / (level_end - level_begin);
    }

    LOG_INF("bat percent: %d", percent);

    return percent;
}


#else

uint32_t get_battery_voltage_by_coulometer(void)
{
    int result;
    union power_supply_propval val;
    bat_charge_context_t *bat_charge = bat_charge_get_context();

    if(bat_charge->coulometer_dev == NULL) {
        LOG_ERR("No coulometer device!");
        return 0;
    }

    result = coulometer_get_property(bat_charge->coulometer_dev, POWER_SUPPLY_PROP_VOLTAGE_NOW, &val);
    if(result != 0) {
        LOG_ERR("get property fail!");
        return 0;
    }

    return val.intval;
}

/* get the percent of battery capacity  */
uint32_t get_battery_percent(void)
{
    int result;
    union power_supply_propval val;
    bat_charge_context_t *bat_charge = bat_charge_get_context();

    if(bat_charge->coulometer_dev == NULL) {
        LOG_ERR("No coulometer device!");
        return 0;
    }

    result = coulometer_get_property(bat_charge->coulometer_dev, POWER_SUPPLY_PROP_CAPACITY, &val);
    if(result != 0) {
        LOG_ERR("get property fail!");
        return 0;
    }

    return val.intval;
}

void bat_check_battery_exist(uint32_t bat_volt)
{
    bat_charge_context_t *bat_charge = bat_charge_get_context();

    bat_charge->bat_exist_state = 1;
}


#endif


/**
**	charger timer handle, run each 50ms
**/
void bat_charge_timer_handler(struct k_work *work)
{
    bat_charge_context_t *bat_charge = bat_charge_get_context();
    int err_no;
    uint32_t bat_v_mv;
	int info_1, info_2;

#ifndef CONFIG_ACTS_BATTERY_SUPPLY_EXT_COULOMETER
    uint32_t  bat_adc;
#endif

    bat_charge->bat_adc_sample_timer_count += 1;
	bat_charge->bat_get_errno_count += 1;

    //Clear irq pending
    if((bat_charge->bat_adc_sample_timer_count %
            (EXT_IRQ_PD_CLEAR_INTERVAL_MS / BAT_CHARGE_DRIVER_TIMER_MS)) == 0) {
        if(bat_charge->irq_pending_bitmap != 0) {
            ext_charger_clear_irq_pending(bat_charge->irq_pending_bitmap);
            bat_charge->irq_pending_bitmap = 0;
        }
    }

	if(bat_charge->bat_get_errno_count >= (BAT_GET_ERRNO_INTERVAL_MS/BAT_CHARGE_DRIVER_TIMER_MS)) {
        err_no = ext_charger_get_errno();
        if(err_no != 0) {
            LOG_INF("ext_charger err happen: %d\n", err_no);
        }

		bat_charge->bat_get_errno_count = 0;
	}

#ifndef CONFIG_ACTS_BATTERY_SUPPLY_EXT_COULOMETER
    if ((bat_charge->bat_adc_sample_timer_count %
            (BAT_ADC_SAMPLE_INTERVAL_MS / BAT_CHARGE_DRIVER_TIMER_MS)) == 0) {
        bat_adc = bat_adc_get_sample();

        bat_adc_sample_buf_put(bat_adc);

        if (bat_charge->bat_adc_sample_timer_count >= (BAT_CHARGER_DEBUG_INFO_OUT_MS / BAT_CHARGE_DRIVER_TIMER_MS)) {
            bat_charge->bat_adc_sample_timer_count = 0;
            bat_v_mv = bat_adc_get_voltage_mv(bat_adc);

            LOG_INF("state:%d <%s>, dc5v:%d <%s>, bat:%dmv, real: %dmv, chg_en:%d",
                bat_charge->bat_charge_current_state,
                get_bat_state_str(bat_charge->bat_charge_current_state),
                bat_charge->dc5v_current_state,
                get_dc5v_state_str(bat_charge->dc5v_current_state),
                bat_v_mv,
                bat_charge->bat_real_volt,
                ext_charger_check_enable());

            info_1 = ext_charger_dump_info1();
			info_2 = ext_charger_dump_info2();
            LOG_INF("Therm:%d, Pre-ch:%d, CC:%d, CV:%d", 
				(info_1&0x80)>>7,
				(info_1&0x40)>>6,
				(info_1&0x20)>>5,
				(info_1&0x10)>>4);
		    LOG_INF("Finish:%d, VLIM:%d, ILIM:%d, PPMG:%d", 
			    (info_1&0x08)>>3,
			    (info_1&0x04)>>2,
			    (info_1&0x02)>>1,
			    (info_1&0x01)>>0);

            LOG_INF("PG:%d, OV:%d, CHRG:%d, IC_OT:%d", 
				(info_2&0x80)>>7,
				(info_2&0x40)>>6,
				(info_2&0x20)>>5,
				(info_2&0x10)>>4);
		    LOG_INF("BATOT:%d, BATUT:%d, SAFT_EXP:%d, PRET_EXP:%d", 
			    (info_2&0x08)>>3,
			    (info_2&0x04)>>2,
			    (info_2&0x02)>>1,
			    (info_2&0x01)>>0);				
        }
    }
#endif			

    /* DC5V detect */
    get_dc5v_current_state();

    /* process the battery and charegr status */
    bat_charge_status_proc();

    k_delayed_work_submit(&bat_charge->timer, K_MSEC(BAT_CHARGE_DRIVER_TIMER_MS));
}

/**
**	battery and charger function initialization
**/
static int bat_charge_init(const struct device *dev)
{
    bat_charge_context_t *bat_charge = bat_charge_get_context();

    if (bat_charge->inited) {
        LOG_INF("already inited");
        return 0;
    }

    LOG_INF("bat charge init start!");

    ext_charger_get_config(dev);

    k_delayed_work_init(&bat_charge->charger_enable_timer, charger_enable_timer_handler);
    k_delayed_work_init(&bat_charge->timer, bat_charge_timer_handler);

    bat_charge->inited = 1;

    LOG_INF("bat charge init end!");

    return 0;
}

/**
**	suspend battery and charger
**/
void bat_charge_suspend(void)
{
    bat_charge_context_t *bat_charge = bat_charge_get_context();

    if (!bat_charge->inited
        || !bat_charge->enabled) {
        return;
    }

    bat_charge_disable();

    k_delayed_work_cancel(&bat_charge->timer);

    bat_charge->enabled = 0;
}

/**
**	resume battery and charger
**/
void bat_charge_resume(void)
{
    bat_charge_context_t *bat_charge = bat_charge_get_context();

    if (!bat_charge->inited) {
        bat_charge_init(bat_charge->this_dev);
    }

    if (bat_charge->enabled) {
        LOG_INF("already enabled!\n");
        return;
    }

    bat_charge->last_voltage_report = BAT_VOLTAGE_RESERVE;
    bat_charge->last_cap_report = BAT_CAP_RESERVE;

    bat_charge->dc5v_last_state = DC5V_STATE_INVALID;
    bat_charge->dc5v_current_state = DC5V_STATE_INVALID;

    bat_charge->bat_charge_last_state = BAT_CHG_STATE_INIT;
    bat_charge->bat_charge_current_state = BAT_CHG_STATE_INIT;

    bat_charge->bat_exist_state = 1;

    /* init the extern charger */
    ext_charger_init();

    /* enable extern coulometer */
#ifdef CONFIG_ACTS_BATTERY_SUPPLY_EXT_COULOMETER
    if(bat_charge->coulometer_dev != NULL) {
        coulometer_enable(bat_charge->coulometer_dev);
    } else {
        LOG_ERR("no coulometer device!");
        return;
    }
#endif

    /* check and get the real voltage of battery firstly */
    bat_check_real_voltage(1);

	bat_charge->bat_get_errno_count = 0;

#ifndef CONFIG_ACTS_BATTERY_SUPPLY_EXT_COULOMETER
    /* BATADC sample buffer initialization */
    memset(bat_charge->bat_adc_sample_buf, 0, sizeof(bat_charge->bat_adc_sample_buf));

    /* initialize the buffer for sampling battery voltage */
    bat_adc_sample_buf_init();

	LOG_INF("Ext_en1: %d", ext_charger_check_enable());

    if (bat_charge->last_saved_bat_volt != 0) {
        bat_charge->bat_real_volt = bat_charge->last_saved_bat_volt;
		LOG_INF("use last saved volt for init!");
    } else {
		bat_charge->bat_real_volt = get_battery_voltage_by_time(BAT_VOLT_CHECK_SAMPLE_SEC);
    }

    bat_charge->last_time_check_bat_volt = k_uptime_get_32();	

	LOG_INF("Ext_en2: %d", ext_charger_check_enable());
#else
    if (bat_charge->last_saved_bat_volt != 0) {
        bat_charge->bat_real_volt = bat_charge->last_saved_bat_volt;
		LOG_INF("use last saved volt for init!");
    } else {
		bat_charge->bat_real_volt = get_battery_voltage_by_coulometer();
    }

	bat_charge->last_time_check_bat_volt = k_uptime_get_32();	
#endif

    LOG_INF("battery voltage:%dmv", bat_charge->bat_real_volt);

    if (get_dc5v_current_state() != DC5V_STATE_IN) {
        /* check if battery is low power when DC5V not plug-in */
        bat_check_voltage_low(bat_charge->bat_real_volt);
    } else {
        /* check if battery is exist when DC5V in */
        bat_check_battery_exist(bat_charge->bat_real_volt);
    }

    k_delayed_work_submit(&bat_charge->timer, K_MSEC(BAT_CHARGE_DRIVER_TIMER_MS));

    bat_charge->state_timer_count = 0;
    bat_charge->enabled = 1;

}



static int battery_acts_get_property(struct device *dev,
                       enum power_supply_property psp,
                       union power_supply_propval *val)
{
    bat_charge_context_t *bat_charge = bat_charge_get_context();

    switch (psp) {
    case POWER_SUPPLY_PROP_STATUS:
    {
        if (!bat_charge->bat_exist_state) {
            val->intval = POWER_SUPPLY_STATUS_BAT_NOTEXIST;
        } else if ((bat_charge->bat_charge_current_state >= BAT_CHG_STATE_PRECHARGE)
            && (bat_charge->bat_charge_current_state <= BAT_CHG_STATE_CHARGE)) {
            val->intval = POWER_SUPPLY_STATUS_CHARGING;
        } else if (bat_charge->bat_charge_current_state == BAT_CHG_STATE_FULL) {
            val->intval = POWER_SUPPLY_STATUS_FULL;
        } else {
            val->intval = POWER_SUPPLY_STATUS_DISCHARGE;
        }
        return 0;
    }
    case POWER_SUPPLY_PROP_ONLINE:
        val->intval = 1;
        return 0;
    case POWER_SUPPLY_PROP_VOLTAGE_NOW:
        val->intval = get_battery_voltage_mv()*1000;
        return 0;
    case POWER_SUPPLY_PROP_CAPACITY:
        val->intval = get_battery_percent();
        return 0;
    case POWER_SUPPLY_PROP_DC5V:
        val->intval = bat_charge->dc5v_current_state;
        return 0;
    default:
        return -EINVAL;
    }
}

static void battery_acts_set_property(struct device *dev,
                       enum power_supply_property psp,
                       union power_supply_propval *val)
{
    bat_charge_context_t *bat_charge = bat_charge_get_context();

    switch (psp) {
        case POWER_SUPPLY_SET_PROP_FEATURE:
            bat_charge->configs.cfg_features = val->intval;
            LOG_INF("set new power supply feature:0x%x", val->intval);
            break;

		case POWER_SUPPLY_SET_INIT_VOL:
			bat_charge->last_saved_bat_volt = (uint16_t)val->intval;
			LOG_INF("set bat int voltage: %dmv", bat_charge->last_saved_bat_volt);
			break;		

		case POWER_SUPPLY_SET_BEFORE_ENTER_S4:
			//bat_charger_set_before_enter_s4();
			break;			

        default:
            LOG_ERR("invalid psp cmd:%d", psp);
    }
}

static void battery_acts_register_notify(struct device *dev, bat_charge_callback_t cb)
{
    bat_charge_context_t *bat_charge = bat_charge_get_context();
    int flag;

    LOG_DBG("callback %p", cb);

    flag = irq_lock();
    if ((bat_charge->callback == NULL) && cb) {
        bat_charge->callback = cb;
    } else {
        LOG_ERR("notify func already exist!\n");
    }
    irq_unlock(flag);
}

static void battery_acts_enable(struct device *dev)
{
    bat_charge_resume();
}

static void battery_acts_disable(struct device *dev)
{
    bat_charge_suspend();
}

static const struct power_supply_driver_api battery_acts_driver_api = {
    .get_property = battery_acts_get_property,
    .set_property = battery_acts_set_property,
    .register_notify = battery_acts_register_notify,
    .enable = battery_acts_enable,
    .disable = battery_acts_disable,
};


/**
**	battery driver init
**/
static int battery_acts_init(const struct device *dev)
{
    const struct acts_battery_config *cfg = dev->config;
    bat_charge_context_t *bat_charge = bat_charge_get_context();

#ifndef CONFIG_ACTS_BATTERY_SUPPLY_EXT_COULOMETER
    struct adc_channel_cfg channel_cfg = {0};
#endif

#ifdef CONFIG_ACTS_BATTERY_SUPPLY_EXT_COULOMETER
    bat_charge->coulometer_dev = (struct device *)device_get_binding(cfg->coulometer_name);
    if (!bat_charge->coulometer_dev) {
        LOG_ERR("cannot found ext coulometer device!\n");
        return -ENODEV;
    }
#else
    bat_charge->adc_dev = (struct device *)device_get_binding(cfg->adc_name);
    if (!bat_charge->adc_dev) {
        LOG_ERR("cannot found ADC device!\n");
        return -ENODEV;
    }

    channel_cfg.channel_id = cfg->batadc_chan;
    if (adc_channel_setup(bat_charge->adc_dev, &channel_cfg)) {
        LOG_ERR("setup channel_id %d error", channel_cfg.channel_id);
        return -EFAULT;
    }

    bat_charge->bat_sequence.channels = BIT(cfg->batadc_chan);
    bat_charge->bat_sequence.buffer = &bat_charge->bat_adcval;
    bat_charge->bat_sequence.buffer_size = sizeof(bat_charge->bat_adcval);
#endif

    bat_charge->this_dev = (struct device *)dev;

    bat_charge->i2c_dev = (struct device *)device_get_binding(CONFIG_EXT_CHARGER_I2C_NAME);
    if (!bat_charge->i2c_dev) {
        LOG_ERR("can not access EXT charger i2c device\n");
        return -1;
    }

    return 0;
}


static const struct acts_battery_config battery_acts_cdata = {
#ifdef CONFIG_ACTS_BATTERY_SUPPLY_EXT_COULOMETER
    .coulometer_name = CONFIG_ACTS_EXT_COULOMETER_DEV_NAME,
    .adc_name = NULL,
    .batadc_chan = 0xff,
#else
    .coulometer_name = NULL,
    .adc_name = CONFIG_PMUADC_NAME,
    .batadc_chan = PMUADC_ID_DC5V,				//PMUADC_ID_BATV,
#endif

    .debug_interval_sec = CONFIG_BATTERY_DEBUG_INTERVAL_SEC,
};

#if IS_ENABLED(CONFIG_ACTS_BATTERY)
DEVICE_DEFINE(battery, CONFIG_ACTS_BATTERY_DEV_NAME, battery_acts_init, NULL,
        NULL, &battery_acts_cdata, POST_KERNEL,
        CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &battery_acts_driver_api);
#endif

