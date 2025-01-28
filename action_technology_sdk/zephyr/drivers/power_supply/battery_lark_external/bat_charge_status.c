/*
 * Copyright (c) 2021  Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Actions SoC battery and charger status
 */

#include "bat_charge_private.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(bat_sts, CONFIG_LOG_DEFAULT_LEVEL);


/**
**	check real battery voltage in charging
**/
void bat_charge_state_check_volt(int bat_volt)
{
    bat_charge_context_t *bat_charge = bat_charge_get_context();
    uint32_t  period;

	if((bat_charge->bat_charge_current_state == BAT_CHG_STATE_PRECHARGE) || \
		 (bat_charge->bat_charge_current_state == BAT_CHG_STATE_CHARGE)) {
        period = bat_charge->configs.cfg_charge.Charge_Check_Period_Sec *
            1000 / BAT_CHARGE_DRIVER_TIMER_MS;		 
    } else {
        period = bat_charge->configs.cfg_charge.Battery_Check_Period_Sec *
            1000 / BAT_CHARGE_DRIVER_TIMER_MS;
    }
	
    if (bat_charge->state_timer_count < period) {
        bat_charge->state_timer_count += 1;

        /* check battery real voltage every Battery_Check_Period_Sec seconds */
        if (bat_charge->state_timer_count == period) {
			LOG_INF("Check bat real voltage, period:%d", period*BAT_CHARGE_DRIVER_TIMER_MS/1000);
            bat_check_real_voltage(BAT_VOLT_CHECK_SAMPLE_SEC);

			if((bat_charge->bat_charge_current_state == BAT_CHG_STATE_PRECHARGE) || \
		 		 (bat_charge->bat_charge_current_state == BAT_CHG_STATE_CHARGE)) {
				bat_charge->bat_check_real_in_charge = 1;
			} else {
				bat_charge->bat_check_real_in_charge = 0;
			}
        }
        return;
    }

    if (bat_volt < 0) {
        return;
    }

    bat_charge->state_timer_count = 0;

    if (bat_charge->bat_charge_current_state == BAT_CHG_STATE_FULL) {
		/* if battery in full state and turns to low power state, consider that battery is not existed */
        if (bat_volt <= bat_charge->configs.cfg_charge.Precharge_Stop_Voltage) {
            LOG_INF("battery inexistance");

            bat_charge->bat_exist_state  = 0;
            bat_charge->bat_charge_current_state = BAT_CHG_STATE_LOW;
        }
    }
}

#ifndef CONFIG_ACTS_BATTERY_SUPPLY_EXT_COULOMETER	
void bat_check_battery_plugin(void)
{
    bat_charge_context_t *bat_charge = bat_charge_get_context();
    uint32_t bat_adc;

	bat_adc = bat_adc_get_sample();
	if((bat_charge->bat_real_volt < BAT_CHECK_EXIST_THRESHOLD) && (bat_adc > BAT_CHECK_EXIST_THRESHOLD) && (bat_charge->bat_exist_state == 0)) {
		//bat plug-in
		LOG_INF("Battery plug-in");
		bat_charge->bat_exist_state = 1;
	} else if((bat_charge->bat_real_volt > BAT_CHECK_EXIST_THRESHOLD) && (bat_adc < BAT_CHECK_EXIST_THRESHOLD) && (bat_charge->bat_exist_state == 1)) {
		//bat plug-out
		LOG_INF("Battery plug-out");
		bat_charge->bat_exist_state = 0;
		bat_charge->bat_charge_current_state = BAT_CHG_STATE_LOW;
	}
}
#endif

/**
**	report battery voltage and capacity to user
**/
void bat_charge_report(uint32_t cur_voltage, uint8_t cur_cap)
{
	bat_charge_context_t *bat_charge = bat_charge_get_context();
	bat_charge_event_para_t para = {0};

    if (bat_charge->callback && (bat_charge->last_cap_report != cur_cap)) {
        para.voltage_val = cur_voltage*1000;
        bat_charge->callback(BAT_CHG_EVENT_VOLTAGE_CHANGE, &para);
        para.cap = cur_cap;
        bat_charge->callback(BAT_CHG_EVENT_CAP_CHANGE, &para);

		LOG_INF("** battery voltage:%dmV capacity:%d%% **\n", cur_voltage, cur_cap);
    }

	bat_charge->last_voltage_report = cur_voltage;
	bat_charge->last_cap_report = cur_cap;
}

/**
**	battery and charger status process 
**/
void bat_charge_status_proc(void)
{
    bat_charge_context_t *bat_charge = bat_charge_get_context();
    int bat_volt = -1;
	int ret;

    if (bat_charge->need_check_bat_real_volt) {
        if (bat_charge->bat_volt_check_wait_count > 0) {
            bat_charge->bat_volt_check_wait_count -= 1;
        }

		/* wait some time after disable charger */
        if (bat_charge->bat_volt_check_wait_count == 0) {

#ifndef CONFIG_ACTS_BATTERY_SUPPLY_EXT_COULOMETER				
			/* within a short time after disable charger, get the last 2 seconds sample voltage */
            bat_volt = get_battery_voltage_by_time(2);
#else
            bat_volt = get_battery_voltage_by_coulometer();
#endif

            bat_charge->need_check_bat_real_volt = 0;

            /* check voltage change */
            bat_volt = bat_check_voltage_change(bat_volt);

            if(bat_charge->bat_exist_state == 0) {
				LOG_INF("bat is not exist!\n");
				bat_charge->bat_charge_current_state = BAT_CHG_STATE_LOW;
            } else {
				/* In case of battery voltage or capacity percent changed to notify user */
				bat_charge_report(bat_volt, get_battery_percent());

            	LOG_INF("check real vol: %d, stop vol: %d", bat_volt, bat_charge->configs.cfg_charge.Charge_Stop_Voltage);
				if((bat_charge->dc5v_current_state == DC5V_STATE_IN) && \
					  ((bat_charge->bat_charge_current_state == BAT_CHG_STATE_PRECHARGE) || \
		                (bat_charge->bat_charge_current_state == BAT_CHG_STATE_CHARGE))) {
					LOG_INF("charge after check vol!\n");
					bat_charge_enable();

					bat_charge->bat_check_real_in_charge = 0;
				}
            }
        }
    }

#ifndef CONFIG_ACTS_BATTERY_SUPPLY_EXT_COULOMETER	
    bat_check_battery_plugin();
#endif

	if(bat_charge->bat_check_real_in_charge == 0) {
    	ret = ext_charger_get_state();
		if(ret >= 0) {
			bat_charge->bat_charge_current_state = (uint8_t)ret;
		}
	}
	
	if (bat_charge->bat_charge_last_state != bat_charge->bat_charge_current_state) {
		LOG_INF("last state: %d <%s>; new state: %d <%s>", bat_charge->bat_charge_last_state, get_bat_state_str(bat_charge->bat_charge_last_state), \
			bat_charge->bat_charge_current_state, get_bat_state_str(bat_charge->bat_charge_current_state));
		
		bat_charge->bat_charge_last_state = bat_charge->bat_charge_current_state;
        bat_charge->state_timer_count = 0;

		if((bat_charge->bat_charge_current_state == BAT_CHG_STATE_FULL) || (bat_charge->is_charge_finished != 0)) {
			bat_charge->is_charge_finished = 0;
			
			if (bat_charge->callback) {
            	bat_charge->callback(BAT_CHG_EVENT_CHARGE_FULL, NULL);
			}
		}		
    }	

	bat_charge_state_check_volt(bat_volt);
	
}

