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

/* period of detection when low power or pre-charge stage */
#define BAT_PRECHARGE_CHECK_PERIOD_SEC 300

/* timeout to check the battery is not existed */
#define BAT_NOT_EXIST_CHECK_TIME_SEC  1800			//300

void bat_charge_state_check_volt(int bat_volt);

/* get the battery real voltage by PMU BATADC */
void bat_check_real_voltage(uint32_t sample_sec)
{
    bat_charge_context_t *bat_charge = bat_charge_get_context();

    if (bat_charge->bat_charge_state == BAT_CHG_STATE_PRECHARGE ||
        bat_charge->bat_charge_state == BAT_CHG_STATE_CHARGE) {
        uint8_t  stop_mode = bat_charge->configs.cfg_charge.Charge_Stop_Mode;

		/**
		* According to the threshold of stop voltage to indicate the full status of battery.
		* It's better to stop charger function when sample battery voltage for accuracy.
		*/
        if (stop_mode == CHARGE_STOP_BY_VOLTAGE ||
            stop_mode == CHARGE_STOP_BY_VOLTAGE_AND_CURRENT) {
			/* disable charger function */
            bat_charge_ctrl_disable();
        } else {
			bat_charge_ctrl_disable();
		}
    } else {
        /* only disable charger function is enough */
        bat_charge_ctrl_disable();
    }

    /* update the wait time counter */
    bat_charge->bat_volt_check_wait_count =
        sample_sec * 1000 / BAT_CHARGE_TIMER_THREAD_MS;

    bat_charge->need_check_bat_real_volt = 1;
}

#ifdef CONFIG_ACTS_BATTERY_SUPPORT_FAST_CHARGER
void bat_charge_fast_charge_start(void)
{
	bat_charge_context_t*  bat_charge = bat_charge_get_context();
	bat_charge_configs_t*  cfg = &bat_charge->configs;

	if(cfg->cfg_charge.Fast_Charge_Enable == 0) {
		bat_charge_state_start(bat_charge->configs.cfg_charge.Charge_Current);
	} else if(bat_charge->bat_real_volt >= (3400 + 100 * (bat_charge->configs.cfg_charge.Fast_Charge_Voltage_Threshold - FAST_CHARGE_3_4_V))) {
		bat_charge_state_start(bat_charge->configs.cfg_charge.Charge_Current);
	} else if(bat_charge->configs.cfg_charge.Fast_Charge_Current <= bat_charge->configs.cfg_charge.Charge_Current) {
		bat_charge_state_start(bat_charge->configs.cfg_charge.Charge_Current);
	} else {
		LOG_INF("fast charge start: level is %d", bat_charge->configs.cfg_charge.Fast_Charge_Current);
		bat_charge_state_start(bat_charge->configs.cfg_charge.Fast_Charge_Current);

		bat_charge->in_fast_charge_stage = 1;
	}
}
#endif

/* charge state init */
void bat_charge_state_init(int bat_volt)
{
    bat_charge_context_t *bat_charge = bat_charge_get_context();
	
	if(bat_charge->err_to_stop_charge != 0) {
		//LOG_INF("err stop,not restart!");
		if((bat_charge->err_status_dc5v_change_flag == 0) && (bat_get_diff_time(k_uptime_get_32(),
			bat_charge->err_stop_begin_time) < BAT_ERR_STOP_RESTART_TIME_MS)) {
			if (bat_charge->dc5v_debounce_state != DC5V_STATE_IN) {
				bat_charge_state_check_volt(bat_volt);
			}
			return;
		} else {
			bat_charge->err_to_stop_charge = 0;
			bat_charge->err_status_dc5v_change_flag = 0;
			LOG_INF("Restart from err stop !");

			bat_check_real_voltage(1);

			bat_charge->restart_from_err_flag = 1;
		}
	}

    if (bat_volt < 0) {
        return;
    }

    /* check if battery is low power by configuration */
    if ((bat_charge->configs.cfg_charge.Precharge_Enable == YES) && (bat_volt <= bat_charge->configs.cfg_charge.Precharge_Stop_Voltage)) {
        LOG_INF("low power cur_vol:%d conf_vol:%d",
				bat_volt, bat_charge->configs.cfg_charge.Precharge_Stop_Voltage);

        bat_charge->bat_charge_state = BAT_CHG_STATE_LOW;
    } else if (bat_volt >= bat_charge->configs.cfg_charge.Charge_Stop_Voltage) { /* check battery is full */
		LOG_INF("full power cur_vol:%d conf_vol:%d",
				bat_volt, bat_charge->configs.cfg_charge.Charge_Stop_Voltage);

		/* Once DC5V plug in will start to charge */
        if (bat_charge->configs.cfg_features & SYS_FORCE_CHARGE_WHEN_DC5V_IN) {
            /* battery in normal state indicates will start to charge */
            bat_charge->bat_charge_state = BAT_CHG_STATE_NORMAL;
        } else {
            bat_charge->bat_charge_state = BAT_CHG_STATE_FULL;
        }
    } else {
        LOG_DBG("normal state cur_vol:%d", bat_volt);
        bat_charge->bat_charge_state = BAT_CHG_STATE_NORMAL;
    }
}

/* reset adjust cv offset */
void bat_charge_reset_adjust_cv_offset(void)
{
    bat_charge_context_t*  bat_charge = bat_charge_get_context();

    bat_charge_adjust_cv_offset_t*  t = &bat_charge->adjust_cv_offset;

    memset(t, 0, sizeof(*t));
}

/* use specific current level to start charge */
void bat_charge_state_start(uint32_t current_sel)
{
    bat_charge_context_t *bat_charge = bat_charge_get_context();

    /* ajust CV_OFFSET when start to charge */
    if (!bat_charge->charge_state_started) {
        bat_charge_reset_adjust_cv_offset();
    }

    /* record the current when start */
    bat_charge->charge_current_sel = current_sel;
    bat_charge->charge_enable_pending = 1;

    bat_charge->need_check_bat_real_volt = 0;

    if (!bat_charge->charge_state_started) {
        bat_charge->charge_state_started = 1;
		if (bat_charge->callback) {
        	bat_charge->callback(BAT_CHG_EVENT_CHARGE_START, NULL);
		}
    }

	LOG_INF("current level: %d", bat_charge->charge_current_sel);

	if(bat_charge->need_add_cc_by_step != 0) {
		LOG_INF("Set new CC: %d", bat_charge->charge_current_sel);
		bat_charge_set_current(bat_charge->charge_current_sel);
	}
}

/* stop charge */
void bat_charge_state_stop(void)
{
    bat_charge_context_t*  bat_charge = bat_charge_get_context();

    /* disable charge */
    bat_charge_ctrl_disable();

    if (bat_charge->charge_state_started) {
        bat_charge->charge_state_started = 0;

		if (bat_charge->callback) {
	        bat_charge->callback(BAT_CHG_EVENT_CHARGE_STOP, NULL);
		}
    }

	LOG_INF("bat_charge_state_stop!");
}

/* battery check */
void bat_charge_state_check_volt(int bat_volt)
{
    bat_charge_context_t *bat_charge = bat_charge_get_context();
    uint32_t  period;
	uint16_t  recharge_threhold;

    if (bat_charge->dc5v_debounce_state == DC5V_STATE_IN &&
        bat_charge->bat_charge_state != BAT_CHG_STATE_FULL) {
        return;
    }

    period = bat_charge->configs.cfg_charge.Battery_Check_Period_Sec *
        1000 / BAT_CHARGE_TIMER_THREAD_MS;

	LOG_DBG("period:%d state_timer_count:%d", period, bat_charge->state_timer_count);

    if (bat_charge->state_timer_count < period) {
        bat_charge->state_timer_count += 1;

        /* check battery real voltage every BAT_VOLT_CHECK_SAMPLE_SEC seconds */
        if (bat_charge->state_timer_count == period) {
			LOG_INF("Check bat real voltage!");
            bat_check_real_voltage(BAT_VOLT_CHECK_SAMPLE_SEC);
        }
        return;
    }

    if (bat_volt < 0) {
        return;
    }

    bat_charge->state_timer_count = 0;

    if (bat_charge->bat_charge_state == BAT_CHG_STATE_FULL) {
		/* if battery in full state and turns to low power state, consider that battery is bad */
        if (bat_volt <= bat_charge->configs.cfg_charge.Precharge_Stop_Voltage) {
            LOG_INF("battery error");

            //bat_charge->bat_exist_state  = 0;
            bat_charge->bat_error_state  = 1;
            bat_charge->bat_charge_state = BAT_CHG_STATE_LOW;
        } else {
			if(bat_charge->configs.cfg_charge.Enable_Battery_Recharge == NO) {
				recharge_threhold = bat_charge->configs.cfg_charge.Charge_Stop_Voltage;
			} else {
				#ifndef CONFIG_ACTS_BATTERY_SUPPORT_CHARGER_NTC
				recharge_threhold = bat_charge->configs.cfg_charge.Recharge_Threshold_Normal_Temperature;
				#else
				if(bat_charge->configs.cfg_bat_ntc.NTC_Settings.Enable_NTC == 0) {
					recharge_threhold = bat_charge->configs.cfg_charge.Recharge_Threshold_Normal_Temperature;
				} else {
					if((bat_charge->ntc_debounce_state == NTC_TEMPERATURE_LOW_EX) || (bat_charge->ntc_debounce_state == NTC_TEMPERATURE_LOW)) {
						recharge_threhold = bat_charge->configs.cfg_charge.Recharge_Threshold_Low_Temperature;
					} else if((bat_charge->ntc_debounce_state == NTC_TEMPERATURE_HIGH_EX) || (bat_charge->ntc_debounce_state == NTC_TEMPERATURE_HIGH)) {
						recharge_threhold = bat_charge->configs.cfg_charge.Recharge_Threshold_High_Temperature;
					} else {
						recharge_threhold = bat_charge->configs.cfg_charge.Recharge_Threshold_Normal_Temperature;
					}
				}
				#endif
			}

			if (bat_volt < recharge_threhold) {
				/* battery full state => not full state */
            	LOG_INF("re-charge start, real: %dmv, threhold: %dmv", bat_volt, recharge_threhold);
            	bat_charge->bat_charge_state = BAT_CHG_STATE_NORMAL;
        	}			
		}
    }
}

/* battery voltage low , need precharge */
void bat_charge_state_low(int bat_volt)
{
    bat_charge_context_t*  bat_charge = bat_charge_get_context();

	/* DC5V plug in and start pre-charge process */
    if (bat_charge->dc5v_debounce_state == DC5V_STATE_IN) {
		if(bat_charge->configs.cfg_charge.Precharge_Enable == YES) {
			LOG_INF("start precharge");

			bat_charge_state_start(bat_charge->precharge_current_sel);
			bat_charge->bat_charge_state = BAT_CHG_STATE_PRECHARGE;
		} else {
			LOG_INF("change to normal");
			bat_charge->bat_charge_state = BAT_CHG_STATE_NORMAL;			
		}
    } else {
        bat_charge_state_check_volt(bat_volt);
    }
}

/* bat pre-charge */
void bat_charge_state_precharge(int bat_volt)
{
    bat_charge_context_t*  bat_charge = bat_charge_get_context();
    uint32_t  period;

	/* adjust CV_OFFSET when starting to charge */
    if (bat_charge->dc5v_debounce_state != DC5V_STATE_IN) {
        bat_charge_state_stop();

        bat_charge->precharge_time_sec = 0;
        bat_charge->bat_exist_state    = 1;

        bat_charge->bat_charge_state = BAT_CHG_STATE_LOW;

		/* detect battery voltage after DV5V pluged out */
        bat_check_real_voltage(BAT_VOLT_CHECK_SAMPLE_SEC);
        return;
    }

	if(bat_charge->restart_from_err_flag != 0) {
		bat_charge->restart_from_err_flag = 0;
		
		bat_charge->charge_begin_time = k_uptime_get_32();
		LOG_INF("reset charge begin time: 0x%x", bat_charge->charge_begin_time);

#ifdef CONFIG_ACTS_BATTERY_SUPPORT_CHARGER_NTC  
		bat_charge->current_ntc_begin_time = k_uptime_get_32();
		LOG_INF("reset NTC begin time: 0x%x", bat_charge->current_ntc_begin_time);
#endif		
	}

    period = BAT_PRECHARGE_CHECK_PERIOD_SEC * 1000 / BAT_CHARGE_TIMER_THREAD_MS;

    if (bat_charge->state_timer_count < period) {
        bat_charge->state_timer_count += 1;

        /* after some time in pre-charge state and disable charger to detect real battery voltage */
        if (bat_charge->state_timer_count == period) {
            bat_charge->precharge_time_sec += BAT_PRECHARGE_CHECK_PERIOD_SEC;

            bat_check_real_voltage(BAT_VOLT_CHECK_SAMPLE_SEC);
        }
        return;
    }

    if (bat_volt < 0) {
        return;
    }

    /* check battery low power */
    if (bat_volt <= bat_charge->configs.cfg_charge.Precharge_Stop_Voltage) {
        if (bat_charge->bat_exist_state == 1) {
            /* if time under low power is too long and consider that battery is error  */
            if (bat_charge->precharge_time_sec >= BAT_NOT_EXIST_CHECK_TIME_SEC) {
                bat_charge->precharge_time_sec = 0;
                //bat_charge->bat_exist_state    = 0;
                bat_charge->bat_error_state    = 1;

				LOG_INF("Precharge time too long, battery is bad!");
            }
        }

        bat_charge->bat_charge_state = BAT_CHG_STATE_LOW;
    } else {
        LOG_INF("from pre-charge to normal charge!");;

        bat_charge->precharge_time_sec = 0;
        bat_charge->bat_exist_state    = 1;
        bat_charge->bat_error_state    = 0;

        bat_charge->bat_charge_state = BAT_CHG_STATE_NORMAL;
    }
}

/* battery normal */
void bat_charge_state_normal(int bat_volt)
{
    bat_charge_context_t*  bat_charge = bat_charge_get_context();

    /* DC5V plug in to start charging */
    if (bat_charge->dc5v_debounce_state == DC5V_STATE_IN) {
        LOG_DBG("charge");

#ifndef CONFIG_ACTS_BATTERY_SUPPORT_FAST_CHARGER
		if(bat_charge->need_add_cc_by_step != 0) {
			bat_charge_state_start(bat_charge->current_cc_sel);
		} else {
        	bat_charge_state_start(bat_charge->configs.cfg_charge.Charge_Current);
		}
#else
		if(bat_charge->configs.cfg_charge.Fast_Charge_Enable == YES) {
			bat_charge_fast_charge_start();
		} else {
			if(bat_charge->need_add_cc_by_step != 0) {
				bat_charge_state_start(bat_charge->current_cc_sel);
			} else {
        		bat_charge_state_start(bat_charge->configs.cfg_charge.Charge_Current);
			}		
		}
#endif

        bat_charge->bat_charge_state = BAT_CHG_STATE_CHARGE;
        bat_charge->charge_near_full = 0;
    } else {
        bat_charge_state_check_volt(bat_volt);
    }
}

/* initialize charger current percent buffer */
void bat_charge_current_percent_buf_init(void)
{
    bat_charge_context_t *bat_charge = bat_charge_get_context();
    bat_charge_current_percent_buf_t *p = &bat_charge->charge_current_percent_buf;

    memset(p, 0, sizeof(*p));

    p->percent = -1;
}

bool bat_charge_check_stop_current(void)
{
    bat_charge_context_t *bat_charge = bat_charge_get_context();
    //bat_charge_current_percent_buf_t *p = &bat_charge->charge_current_percent_buf;
    bat_charge_adjust_current_t *adjust_current = &bat_charge->adjust_current;

	if((bat_charge->bat_real_charge_current != 0) && (bat_charge->bat_real_charge_current < adjust_current->stop_current_ma)) {
		LOG_INF("stop current: %dma - %dma", bat_charge->bat_real_charge_current, adjust_current->stop_current_ma);
		return true;
	}

    return false;
}

/* CV state buffer initialization */
void bat_charge_cv_state_buf_init(void)
{
    bat_charge_context_t *bat_charge = bat_charge_get_context();

    bat_charge_cv_state_buf_t *p = &bat_charge->charge_cv_state_buf;

    memset(p, 0, sizeof(*p));
}

/**
 * CV stage 1: constant voltage 1 (current less than 50%)
 * CV stage 2: constant voltage 2 (current less than 20%)
 */
int bat_charge_cv_state_buf_get(void)
{
    bat_charge_context_t *bat_charge = bat_charge_get_context();
    bat_charge_cv_state_buf_t *p = &bat_charge->charge_cv_state_buf;
    int i, v;

    if (p->count < ARRAY_SIZE(p->buf)) {
        return -1;
    }

    for (i = 0, v = 0; i < ARRAY_SIZE(p->buf); i++) {
        if (p->buf[i] == 0) {
            return 0;
        }

        v += p->buf[i];
    }

    if (v == 2 * ARRAY_SIZE(p->buf)) {
        return 2;
    }

    return 1;
}

void bat_charge_cv_state_buf_put(void)
{
    bat_charge_context_t *bat_charge = bat_charge_get_context();

    bat_charge_cv_state_buf_t *p = &bat_charge->charge_cv_state_buf;

    if (!bat_charge->charge_ctrl_enabled) {
        return;
    }

    p->buf[p->index] = (uint8_t)bat_charge_get_cv_state();

    p->index += 1;

    if (p->index >= ARRAY_SIZE(p->buf)) {
        p->index = 0;
    }

    if (p->count < ARRAY_SIZE(p->buf)) {
        p->count += 1;
    }
}

/* bat charge check cv */
void bat_charge_check_cv_state(void)
{
    bat_charge_context_t *bat_charge = bat_charge_get_context();

    bat_charge_adjust_cv_offset_t *t = &bat_charge->adjust_cv_offset;

    bat_charge_adjust_current_t *adjust_current = &bat_charge->adjust_current;

    int  cv_stage;

    /* Do not check CV state if battery near power full */
    if (bat_charge->charge_near_full) {
        return;
    }

    bat_charge_cv_state_buf_put();

    cv_stage = bat_charge_cv_state_buf_get();

    /* CV stage1: constant voltage stage 1 (current less than 50%) */
    if (cv_stage == 1 &&
        adjust_current->is_valid == 0) {
        bat_charge_cv_state_buf_init();

        if (bat_charge->cv_stage_1_begin_time == 0) {
            LOG_INF("cv_stage:%d", cv_stage);
            bat_charge->cv_stage_1_begin_time = k_uptime_get_32();
		    LOG_INF("cv1_begin: %d", bat_charge->cv_stage_1_begin_time);
        } else {
			/* check charger current less than 80mA from configuration */
            if (bat_charge->configs.cfg_charge.Charge_Current <=
                    CHARGE_CURRENT_80_MA) {
             	/* if time in constant voltage stage 1 larger than 1 hour will enter stage 2 */
                if (bat_get_diff_time(k_uptime_get_32(),
                        bat_charge->cv_stage_1_begin_time) >
                        60 * 60 * 1000) {
                    LOG_INF("cv1 too long, change to cv2!");
                    cv_stage = 2;
                }
            }
        }
    }

    /* CV stage 2: constant voltage 2 (current less than 20%) */
    if (cv_stage == 2) {
        bat_charge_cv_state_buf_init();

        if (!t->adjust_end) {
            bat_charge_current_percent_buf_init();

            bat_charge_adjust_cv_offset();
            return ;
        }

        if (bat_charge->cv_stage_2_begin_time == 0) {
            LOG_INF("cv stage:%d", cv_stage);
            bat_charge->cv_stage_2_begin_time = k_uptime_get_32();
		    LOG_INF("cv2_begin: %d", bat_charge->cv_stage_2_begin_time);
        }

        bat_charge_adjust_current();
	}
}

/* battery charge state */
void bat_charge_state_charge(int bat_volt)
{
    bat_charge_context_t *bat_charge = bat_charge_get_context();
    uint32_t period, stop_mode;
    bool stop_charge;
	bool need_exit_charge = false;

    bat_charge_adjust_cv_offset_t *t = &bat_charge->adjust_cv_offset;

    bat_charge_adjust_current_t *adjust_current = &bat_charge->adjust_current;

    /* DC5V plug out and stop charging */
    if (bat_charge->dc5v_debounce_state != DC5V_STATE_IN)
    {
        bat_charge_state_stop();

        bat_charge->bat_charge_state = BAT_CHG_STATE_NORMAL;

        bat_charge->cv_stage_1_begin_time = 0;
        bat_charge->cv_stage_2_begin_time = 0;

        adjust_current->is_valid = 0;

		/* detect battery voltage when DC5V plug out  */
        bat_check_real_voltage(BAT_VOLT_CHECK_SAMPLE_SEC);
        return;
    }

	if(bat_charge->restart_from_err_flag != 0) {
		bat_charge->restart_from_err_flag = 0;
			
		bat_charge->charge_begin_time = k_uptime_get_32();
		LOG_INF("reset charge begin time: 0x%x", bat_charge->charge_begin_time);
	
#ifdef CONFIG_ACTS_BATTERY_SUPPORT_CHARGER_NTC  
		bat_charge->current_ntc_begin_time = k_uptime_get_32();
		LOG_INF("reset NTC begin time: 0x%x", bat_charge->current_ntc_begin_time);
#endif		
	}

#ifdef CONFIG_ACTS_BATTERY_SUPPORT_FAST_CHARGER
	if(bat_charge->in_fast_charge_stage != 0) {
		if(bat_charge->bat_real_volt >= (3400 + 100 * (bat_charge->configs.cfg_charge.Fast_Charge_Voltage_Threshold - FAST_CHARGE_3_4_V))) {
			LOG_INF("Exit fast-charge to normal charge!");
			bat_charge->in_fast_charge_stage = 0;

			bat_charge_state_start(bat_charge->configs.cfg_charge.Charge_Current);
			return;
		}
	}
#endif

    if (bat_charge->charge_near_full) {
        period = bat_charge->configs.cfg_charge.Charge_Full_Continue_Sec *
            		1000 / BAT_CHARGE_TIMER_THREAD_MS;
        if(period == 0) {
            LOG_INF("no need continue, charge full!");
            goto charge_full;
        }
    } else {
        period = bat_charge->configs.cfg_charge.Charge_Check_Period_Sec *
            		1000 / BAT_CHARGE_TIMER_THREAD_MS;
    }

    if (bat_charge->state_timer_count < period) {
        bat_charge->state_timer_count += 1;

        /* check interval 1s  */
        if ((bat_charge->state_timer_count %
                (1000 / BAT_CHARGE_TIMER_THREAD_MS)) == 0) {
            if (get_dc5v_current_state() != DC5V_STATE_PENDING) {
                //bat_charge_current_percent_buf_put();
                bat_charge_check_cv_state();
            }
        }

		/* check ovp each 10s */
		if	((bat_charge->state_timer_count %
                (10000 / BAT_CHARGE_TIMER_THREAD_MS)) == 0)	{
            if(bat_charge->configs.cfg_charge.Bat_OVP_Enable == YES) {
				uint16_t temp_vol;
				uint32_t bat_adc;
				int bat_adc_value;

#ifdef CONFIG_ACTS_BATTERY_SUPPORT_CHARGER_NTC
				if(bat_charge->configs.cfg_bat_ntc.NTC_Settings.Enable_NTC == 0){
					temp_vol = bat_charge->configs.cfg_charge.OVP_Voltage_Normal_Temperature;
				} else if((bat_charge->ntc_debounce_state == NTC_TEMPERATURE_LOW_EX) || (bat_charge->ntc_debounce_state == NTC_TEMPERATURE_LOW)) {
					temp_vol = bat_charge->configs.cfg_charge.OVP_Voltage_Low_Temperature;
				} else if((bat_charge->ntc_debounce_state == NTC_TEMPERATURE_HIGH_EX) || (bat_charge->ntc_debounce_state == NTC_TEMPERATURE_HIGH)) {
					temp_vol = bat_charge->configs.cfg_charge.OVP_Voltage_High_Temperature;
				} else {
					temp_vol = bat_charge->configs.cfg_charge.OVP_Voltage_Normal_Temperature;
				}
#else
				temp_vol = bat_charge->configs.cfg_charge.OVP_Voltage_Normal_Temperature;
#endif

				bat_adc_value = bat_adc_get_sample();
				if(bat_adc_value > 0) {
					bat_adc = (uint32_t)bat_adc_value;
					if(bat_adc_get_voltage_mv(bat_adc) >= temp_vol) {
						LOG_INF("OVP: %dmv -- %d -- %dmv", temp_vol, bat_adc, bat_adc_get_voltage_mv(bat_adc));
						need_exit_charge = true;
						goto charge_err_out;
					}
				}
            }
        }

		/* check the real voltage after charging for a while */
        if (bat_charge->state_timer_count == period) {
            bat_check_real_voltage(BAT_VOLT_CHECK_SAMPLE_SEC);
        }
        return;
    }

    if (bat_volt < 0) {
        return;
    }

    if (bat_charge->charge_near_full) {
        goto charge_full;
    }

    stop_mode = bat_charge->configs.cfg_charge.Charge_Stop_Mode;
    stop_charge = 0;

	/* limit current percent 0 */
    if (bat_charge->limit_current_percent == 0) {
        stop_charge = 0;
    } else if (!t->adjust_end) {
		/* CV_OFFSET adjustment does not complete */
        stop_charge = 0;
    } else if (bat_charge->cv_stage_2_begin_time == 0) {
        stop_charge = 0;
    } else if (bat_get_diff_time(k_uptime_get_32(),
                bat_charge->cv_stage_2_begin_time) >
                60 * 60 * 1000) {
		/* in constant voltage stage2 (current level less than 20%)
		 * and moreover take at least 1 hour to stop charge.
		 */
		LOG_INF("cv2 too long, exit!"); 
        stop_charge = 1;
    } else if (stop_mode == CHARGE_STOP_BY_VOLTAGE) {
        if (bat_volt >= bat_charge->configs.cfg_charge.Charge_Stop_Voltage) {
			LOG_INF("full by vol!"); 
            stop_charge = 1;
        }
    } else if (stop_mode == CHARGE_STOP_BY_CURRENT) {
        if (bat_charge_check_stop_current() == 1) {
			LOG_INF("full by current!"); 
            stop_charge = 1;
        }
    } else if (stop_mode == CHARGE_STOP_BY_VOLTAGE_AND_CURRENT) {
        if (bat_volt >= bat_charge->configs.cfg_charge.Charge_Stop_Voltage) {
            if (bat_charge_check_stop_current() == 1) {
				LOG_INF("check finish ok!\n");
                stop_charge = 1;
            }
        }
    }

    /* continue to charge if battery power is not full */
    if (!stop_charge) {
		if((bat_charge->configs.cfg_charge.Charge_Total_Time_Limit != 0) && \
			bat_get_diff_time(k_uptime_get_32(),
                bat_charge->charge_begin_time) >
                bat_charge->configs.cfg_charge.Charge_Total_Time_Limit * 60 * 1000) {
        	LOG_INF("charge time too long, need stop: %d", bat_charge->configs.cfg_charge.Charge_Total_Time_Limit);
			need_exit_charge = true;            
        }
		
#ifdef CONFIG_ACTS_BATTERY_SUPPORT_CHARGER_NTC
		if ((bat_charge->current_ntc_charge_time_limit != 0) && \
			  bat_get_diff_time(k_uptime_get_32(),
                bat_charge->current_ntc_begin_time) >
                bat_charge->current_ntc_charge_time_limit * 60 * 1000) {
        	LOG_INF("charge in NTC too long, need stop: %d", bat_charge->current_ntc_charge_time_limit);
			need_exit_charge = true;
		}
#endif	

		if(need_exit_charge) {
charge_err_out:			
			/* disable charger */
    		bat_charge_ctrl_disable();
			bat_charge->err_stop_begin_time = k_uptime_get_32();
            bat_charge->err_to_stop_charge = 1;
			bat_charge->bat_charge_state = BAT_CHG_STATE_INIT;
			LOG_INF("err stop begin: %d", bat_charge->err_stop_begin_time);
			return;
		}

        if(bat_charge->need_add_cc_by_step != 0) {
			bat_charge_state_start(bat_charge->current_cc_sel);
		} else {
            bat_charge_state_start(bat_charge->configs.cfg_charge.Charge_Current);
		}

        bat_charge->state_timer_count = 0;
        bat_charge->charge_near_full  = 0;
        return;
    }

    if (!bat_charge->charge_near_full) {
        /* continue charging when near battery full */
        LOG_INF("near full");

        if(bat_charge->need_add_cc_by_step != 0) {
			bat_charge_state_start(bat_charge->current_cc_sel);
		} else {
            bat_charge_state_start(bat_charge->configs.cfg_charge.Charge_Current);
		}

        bat_charge->state_timer_count = 0;
        bat_charge->charge_near_full  = true;
        return;
    }

charge_full:

    /* continue charging to make sure battery power full */
    LOG_INF("charge full");

	bat_charge->last_cap_detect = 100;

    /* disable charger */
    bat_charge_ctrl_disable();

	if (bat_charge->callback)
    	bat_charge->callback(BAT_CHG_EVENT_CHARGE_FULL, NULL);

    bat_charge->bat_full_dc5v_last_state = bat_charge->dc5v_debounce_state;

    bat_charge->bat_charge_state = BAT_CHG_STATE_FULL;

    bat_charge->cv_stage_1_begin_time = 0;
    bat_charge->cv_stage_2_begin_time = 0;

    adjust_current->is_valid = 0;
}

/* battery charge full */
void bat_charge_state_full(int bat_volt)
{
    bat_charge_context_t *bat_charge = bat_charge_get_context();

    if (bat_charge->bat_full_dc5v_last_state != bat_charge->dc5v_debounce_state) {
        if (bat_charge->dc5v_debounce_state == DC5V_STATE_IN) {
			/* DC5V plug in when battery power full */
			if (bat_charge->callback) {
				LOG_INF("DC5V IN When bat is full!");
            	bat_charge->callback(BAT_CHG_EVENT_BATTERY_FULL, NULL);
			}
        } else if (bat_charge->bat_full_dc5v_last_state == DC5V_STATE_IN) {
			/* DC5V plug out when battery power full */
			if (bat_charge->callback) {
            	bat_charge->callback(BAT_CHG_EVENT_CHARGE_STOP, NULL);
			}
			
            if (bat_charge->configs.cfg_features & SYS_FORCE_CHARGE_WHEN_DC5V_IN) {
				/* DC5V plug in when battery under normal state to enable charger */
                bat_charge->bat_charge_state = BAT_CHG_STATE_NORMAL;
            }
        }

        bat_charge->bat_full_dc5v_last_state = bat_charge->dc5v_debounce_state;
        bat_charge->state_timer_count = 0;
    } else {
        bat_charge_state_check_volt(bat_volt);
    }
}

/* report current real voltage and capacity */
void bat_charge_report(uint32_t cur_voltage, uint8_t cur_cap)
{
	bat_charge_context_t *bat_charge = bat_charge_get_context();
	struct device *dev = bat_charge->dev;
	struct acts_battery_info *bat = dev->data;
	const struct acts_battery_config *cfg = dev->config;
	bat_charge_event_para_t para = {0};

    if (bat_charge->callback && (bat_charge->last_cap_report != cur_cap)) {
        para.voltage_val = cur_voltage * 1000;
        bat_charge->callback(BAT_CHG_EVENT_VOLTAGE_CHANGE, &para);
        para.cap = cur_cap;
        bat_charge->callback(BAT_CHG_EVENT_CAP_CHANGE, &para);
        set_capacity_to_pstore(cur_voltage);
    }

	LOG_DBG("current voltage: %d", cur_voltage);
	LOG_DBG("current cap: %d", cur_cap);

	bat_charge->last_voltage_report = cur_voltage;
	bat_charge->last_cap_report = cur_cap;

	if (cfg->debug_interval_sec) {
		if ((k_uptime_get_32() - bat->timestamp) > (cfg->debug_interval_sec * 1000)) {
			LOG_INF("** battery voltage:%dmV capacity:%d%% **\n", cur_voltage, cur_cap);
			bat->timestamp = k_uptime_get_32();
		}
	}
}

void bat_charge_update_real_voltage(struct k_work *work)
{
	int bat_adc_value;
	uint32_t bat_volt;
	bool bat_val_ok = false;
    bat_charge_context_t *bat_charge = bat_charge_get_context();

	if(bat_charge->bat_charge_state != BAT_CHG_STATE_PRECHARGE
		&& bat_charge->bat_charge_state != BAT_CHG_STATE_CHARGE) {

		bat_adc_value = bat_adc_get_sample();
		if(bat_adc_value > 0) {
			bat_volt = bat_adc_get_voltage_mv((uint32_t)bat_adc_value);
			if((bat_volt > BAT_VOLTAGE_SAMPLE_VALID_MIN) && (bat_volt < BAT_VOLTAGE_SAMPLE_VALID_MAX)) {
				bat_val_ok = true;
			}
		}		
		
		if(bat_val_ok) {
			/* check voltage change */
			bat_volt = bat_check_voltage_change_discharge(bat_volt);
			
        	LOG_INF("** bat_volt:%d **\n", bat_volt);

			/* put bat sample to buffer */
			bat_adc_value = bat_adc_get_voltage_adc(bat_volt);
			if(bat_adc_value != 0) {
				LOG_INF("put_2: %d, %dmv\n\n", bat_adc_value, bat_adc_get_voltage_mv(bat_adc_value));
				bat_adc_sample_buf_put((uint32_t)bat_adc_value);
			}
			bat_check_voltage_low(bat_charge->bat_real_volt);
		} else {
			if(bat_adc_value < 0) {
				LOG_ERR("Sample BATADC Err2!");
			} else {
				LOG_ERR("bat_vol invalid: %d", bat_adc_value);
			}
		}
	}
}

/* battery and charger status process */
void bat_charge_status_proc(void)
{
    bat_charge_context_t *bat_charge = bat_charge_get_context();
    int last_state = bat_charge->bat_charge_state;
    int bat_volt = -1;

    /* check the battery real voltage.
	 *  #need_check_bat_real_volt: every Battery_Check_Period_Sec change.
	 *  #bat_volt_check_wait_count: every BAT_VOLT_CHECK_SAMPLE_SEC change.
	 */
    if (bat_charge->need_check_bat_real_volt) {
        if (bat_charge->bat_volt_check_wait_count > 0) {
            bat_charge->bat_volt_check_wait_count -= 1;
        }

		/* wait some time after disable charger */
        if (bat_charge->bat_volt_check_wait_count == 0) {
			LOG_INF("check real voltage!");
			/* within a short time after disable charger, get the last 1 seconds sample voltage */
            bat_volt = get_battery_voltage_by_time(1);

            bat_charge->need_check_bat_real_volt = 0;

#if 0
			//consider irdrop
			if((bat_charge->bat_charge_state == BAT_CHG_STATE_PRECHARGE) || (bat_charge->bat_charge_state == BAT_CHG_STATE_CHARGE)) {
				if((bat_volt > 3350) && (bat_charge->bat_real_charge_current != 0)) {
					LOG_INF("IRdrop_before: %dmv", bat_volt);
					bat_volt = bat_volt - bat_charge->bat_real_charge_current * bat_charge->configs.cfg_charge.Battery_Default_RDrop/1000;
					LOG_INF("IRdrop_after: %dmv", bat_volt);
				}
			}
#endif

            /* check voltage change */
            bat_volt = bat_check_voltage_change(bat_volt);

			/* In case of battery voltage or capacity percent changed to notify user */
			bat_charge_report(bat_volt, get_battery_percent());

			if (bat_charge->dc5v_debounce_state == DC5V_STATE_IN) {
				//after detect real bat voltage, need set current by step
				if((bat_charge->configs.cfg_charge.Charge_Current <= CHARGE_CURRENT_FIRST_LEVEL) || \
					 (bat_charge->cv_stage_2_begin_time != 0)) {
					bat_charge->current_cc_sel = bat_charge->configs.cfg_charge.Charge_Current;
					bat_charge->need_add_cc_by_step = 0;
				} else {
					LOG_INF("After Real_check, Need from first level!");
					bat_charge->current_cc_sel = CHARGE_CURRENT_FIRST_LEVEL;
					bat_charge->need_add_cc_by_step = 1;
				}
			}
        }
    }

    switch (bat_charge->bat_charge_state)
    {
        case BAT_CHG_STATE_INIT:
        {
            pmuadc_set_bat_pd_resistance(false);
            bat_charge_state_init(bat_volt);
            break;
        }

        case BAT_CHG_STATE_LOW:
        {
            pmuadc_set_bat_pd_resistance(false);
            bat_charge_state_low(bat_volt);
            break;
        }

        case BAT_CHG_STATE_PRECHARGE:
        {
            pmuadc_set_bat_pd_resistance(true);
            bat_charge_state_precharge(bat_volt);
            break;
        }

        case BAT_CHG_STATE_NORMAL:
        {
            pmuadc_set_bat_pd_resistance(false);
            bat_charge_state_normal(bat_volt);
            break;
        }

        case BAT_CHG_STATE_CHARGE:
        {
            if (chargei_adc_get_current_ma(chargei_adc_get_sample()) == 0) {
                pmuadc_set_bat_pd_resistance(false);
            } else {
                pmuadc_set_bat_pd_resistance(true);
            }
            bat_charge_state_charge(bat_volt);
            break;
        }

        case BAT_CHG_STATE_FULL:
        {
            pmuadc_set_bat_pd_resistance(false);
            bat_charge_state_full(bat_volt);
            break;
        }
    }

    if (bat_charge->bat_charge_state != last_state) {
		LOG_INF("last state: %s; new state: %s", get_charge_state_str(last_state), get_charge_state_str(bat_charge->bat_charge_state));
        bat_charge->state_timer_count = 0;

		if((last_state != BAT_CHG_STATE_CHARGE) && (bat_charge->bat_charge_state == BAT_CHG_STATE_CHARGE)) {
			//reset charge begin time
			bat_charge->charge_begin_time = k_uptime_get_32();
			LOG_INF("charge begin time: 0x%x", bat_charge->charge_begin_time);

			//reset NTC charge begin time
#ifdef CONFIG_ACTS_BATTERY_SUPPORT_CHARGER_NTC  
			bat_charge->current_ntc_begin_time = k_uptime_get_32();
			LOG_INF("reset NTC begin time: 0x%x", bat_charge->current_ntc_begin_time);
#endif		
		}
    }

    bat_charge_check_enable();
}
