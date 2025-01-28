/*
 * Copyright (c) 2021  Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Actions SoC battery and charger control interface
 */

#include <stdlib.h>
#include "bat_charge_private.h"

#ifdef CONFIG_ACTS_BATTERY_SUPPLY_CHARGER_BOX	
#include "dc5v_uart.h"
#else
#include <logging/log.h>
LOG_MODULE_REGISTER(bat_ctl, CONFIG_LOG_DEFAULT_LEVEL);
#endif

/* PMU charger control register update */
void pmu_chg_ctl_reg_write(uint32_t mask, uint32_t value)
{
	bat_charge_context_t *bat_charge = bat_charge_get_context();
	
    bat_charge->bak_PMU_CHG_CTL = (bat_charge->bak_PMU_CHG_CTL & ~mask) | (value & mask);
	sys_write32(bat_charge->bak_PMU_CHG_CTL, CHG_CTL_SVCC);

	LOG_INF("bak_PMU_CHG_CTL:0x%x", bat_charge->bak_PMU_CHG_CTL);
}

/* open charger */
void bat_boot_open_charger(void)
{
	LOG_INF("boot open charger!");
	pmu_chg_ctl_reg_write(
		(0x01<<CHG_CTL_SVCC_CHG_EN),
		(0x1<<CHG_CTL_SVCC_CHG_EN));
}

/**
**	set init charge current and voltage
**/
void bat_init_charge_set(uint8_t ntc_index)
{
	bat_charge_context_t *bat_charge = bat_charge_get_context();
	bat_charge_configs_t *configs = &bat_charge->configs;
	uint8_t init_charge_level;

#ifdef CONFIG_ACTS_BATTERY_SUPPORT_INIT_CHARGER_NTC	
	uint8_t init_charge_current;
#endif

#ifndef CONFIG_ACTS_BATTERY_SUPPORT_INIT_CHARGER_NTC
	if(bat_charge->init_charge_stage == 1) {
		init_charge_level = configs->cfg_charge.Init_Charge_Current_1;
	} else if(bat_charge->init_charge_stage == 2) {
		init_charge_level = configs->cfg_charge.Init_Charge_Current_2;
	} else if(bat_charge->init_charge_stage == 3) {
		init_charge_level = configs->cfg_charge.Init_Charge_Current_3;	
	} else {
		LOG_INF("init charge stage err, set default!");
		init_charge_level = configs->cfg_charge.Init_Charge_Current_1;
		bat_charge->init_charge_stage = 1;
	}
#else
	static uint8_t last_ntc_index = 0xff;

	if(ntc_index != 0xff) {
		if(bat_charge->init_charge_stage == 1) {
			init_charge_current = configs->cfg_bat_init_ntc.init_stage1_NTC_current_ma[ntc_index];
		} else if(bat_charge->init_charge_stage == 2) {
			init_charge_current = configs->cfg_bat_init_ntc.init_stage2_NTC_current_ma[ntc_index];
		} else {
			init_charge_current = configs->cfg_bat_init_ntc.init_stage3_NTC_current_ma[ntc_index];
		}
		
		LOG_INF("init charge NTC change: %dmA", init_charge_current);

		init_charge_level = bat_charge_get_current_level(init_charge_current);

		last_ntc_index = ntc_index;
	} else {
		LOG_INF("init stage change, use last ntc index: %d", last_ntc_index);
		if(last_ntc_index > 4) {
			last_ntc_index = 2;  //normal
		}

		if(bat_charge->init_charge_stage == 1) {
			init_charge_current = configs->cfg_bat_init_ntc.init_stage1_NTC_current_ma[last_ntc_index];
		} else if(bat_charge->init_charge_stage == 2) {
			init_charge_current = configs->cfg_bat_init_ntc.init_stage2_NTC_current_ma[last_ntc_index];
		} else {
			init_charge_current = configs->cfg_bat_init_ntc.init_stage3_NTC_current_ma[last_ntc_index];
		}

		init_charge_level = bat_charge_get_current_level(init_charge_current);
	}
#endif

	pmu_chg_ctl_reg_write(
		((CHG_CTL_SVCC_CHG_CURRENT_MASK)|
		(CHG_CTL_SVCC_CV_OFFSET_MASK) |
		(0x1<<CHG_CTL_SVCC_CV_3V3) | (0x01<<CHG_CTL_SVCC_CHG_EN)),
		((init_charge_level << CHG_CTL_SVCC_CHG_CURRENT_SHIFT) |
		(configs->cfg_charge.Charge_Voltage << CHG_CTL_SVCC_CV_OFFSET_SHIFT) |
		(0x0<<CHG_CTL_SVCC_CV_3V3) |
		(0x1<<CHG_CTL_SVCC_CHG_EN)));

	LOG_INF("init charge, cc: %dma, cv_level: %d", bat_charge_get_current_ma(init_charge_level), configs->cfg_charge.Charge_Voltage);
}

void bat_charger_set_before_enter_s4(void)
{
	bat_charge_context_t *bat_charge = bat_charge_get_context();

	LOG_INF("set before enter s4!");
	pmu_chg_ctl_reg_write(
		((CHG_CTL_SVCC_CHG_CURRENT_MASK)| (0x01<<CHG_CTL_SVCC_CHG_EN)),
		((bat_charge->precharge_current_sel << CHG_CTL_SVCC_CHG_CURRENT_SHIFT) | 
		(0x1<<CHG_CTL_SVCC_CHG_EN)));

}

#ifdef CONFIG_ACTS_BATTERY_SUPPLY_CHARGER_BOX
/* DC5V pulldown disable */
static void dc5v_pull_down_disable(struct k_work *work)
{
	LOG_INF("DC5V pulldown disable");

	/* disable DC5V pulldown and set the minimal pulldown current */
    pmu_chg_ctl_reg_write((1 << CHG_CTL_SVCC_DC5VPD_EN) | CHG_CTL_SVCC_DC5VPD_SET_MASK,
        (0 << CHG_CTL_SVCC_DC5VPD_EN) | (0 << CHG_CTL_SVCC_DC5VPD_SET_SHIFT)
    );
}


/* enable DC5V pulldown and will last specified time in miliseconds to disable */
void dc5v_pull_down_enable(uint32_t current, uint32_t timer_ms)
{
    bat_charge_context_t *bat_charge = bat_charge_get_context();
    struct k_delayed_work *timer = &bat_charge->dc5v_pd_timer;
	uint32_t set_current, set_time;

	/* When DC5V has already enable, cancel the delay work and reschedule new delay work */
	if (bat_charge->bak_PMU_CHG_CTL & (1 << CHG_CTL_SVCC_DC5VPD_EN)) {
		k_delayed_work_cancel(timer);
	}

	if(current > bat_charge->dc5v_pd_max_current) {
		set_current = current;
	} else {
		set_current = bat_charge->dc5v_pd_max_current;
	}

	if(timer_ms > bat_charge->dc5v_pd_max_time) {
		set_time = timer_ms;
	} else {
		set_time = bat_charge->dc5v_pd_max_time;
	}

	LOG_INF("DC5V pulldown current:%d true:%d", current, set_current);
	LOG_INF("DC5V pulldown time:%dms true:%dms", timer_ms, set_time);

	bat_charge->dc5v_pd_max_current = set_current;
	bat_charge->dc5v_pd_max_time = set_time;
	
    pmu_chg_ctl_reg_write((1 << CHG_CTL_SVCC_DC5VPD_EN) | CHG_CTL_SVCC_DC5VPD_SET_MASK,
        (1 << CHG_CTL_SVCC_DC5VPD_EN) | (set_current << CHG_CTL_SVCC_DC5VPD_SET_SHIFT));

	/* sumit a delay work to disable DC5V pull down */
	if ((int)set_time > 0) {
		k_delayed_work_submit(timer, K_MSEC(set_time));
	}
}

/* cancel DC5V state timer */
void dc5v_state_timer_delete(void)
{
    bat_charge_context_t*  bat_charge = bat_charge_get_context();

	k_delayed_work_cancel(&bat_charge->dc5v_check_status.state_timer);
}

/* wakeup charger box */
dc5v_state_t dc5v_wake_charger_box(void)
{
    dc5v_state_t state = get_dc5v_current_state();
    CFG_Struct_Charger_Box *cfg = &(bat_charge_get_configs()->cfg_charger_box);
    uint32_t t;

    /* DC5V state under handling */
    if (state == DC5V_STATE_PENDING) {
        t = k_uptime_get_32();

        while (state == DC5V_STATE_PENDING) {
            k_sleep(K_MSEC(1));

            state = get_dc5v_current_state();

            /* maybe in DC5V UART communication ? */
            if (bat_get_diff_time(k_uptime_get_32(), t) > 1000) {
                return state;
            }
        }
    }

    if (!cfg->Enable_Charger_Box) {
        return state;
    }

    /* forbidden DC5V pulldown to wake up */
    if (cfg->DC5V_Pull_Down_Current == DC5VPD_CURRENT_DISABLE) {
        return state;
    }

    /* enable DC5V pulldown */
    dc5v_pull_down_enable (
        cfg->DC5V_Pull_Down_Current,
        cfg->DC5V_Pull_Down_Hold_Ms + 10
    );

    t = k_uptime_get_32();

    while (1) {
        uint32_t time_ms = bat_get_diff_time(k_uptime_get_32(), t);

        state = get_dc5v_current_state();

        /* already detected DC5V plug-in */
        if (state == DC5V_STATE_IN) {
            break;
        }

        /* wake up timeout */
        if (time_ms >= cfg->DC5V_Pull_Down_Hold_Ms +
                cfg->Charger_Wake_Delay_Ms + 50) {
            break;
        }

        k_sleep(K_MSEC(1));
    }

    return state;
}

/* dc5v state timer handle */
static void dc5v_state_timer_handler(struct k_work *work)
{
    bat_charge_context_t *bat_charge = bat_charge_get_context();
    dc5v_check_status_t *p = &bat_charge->dc5v_check_status;
    CFG_Struct_Charger_Box *cfg = &(bat_charge_get_configs()->cfg_charger_box);
    bool pending = false;

    /* check charger box standby delay time from configuration */
    if (cfg->Enable_Charger_Box) {
        if (bat_charge->dc5v_state_pending_time
			< cfg->Charger_Standby_Delay_Ms) {
            pending = true;
        }
    }

    /* check DC5V UART is under communication */
    if (dc5v_uart_operate(DC5V_UART_CHECK_IO_TIME, NULL, 500, 0)) {
        pending = true;
    }

	/* set the flag for the discharge detect timer (dc5v_state_timer_handler_ex) to run */
	if (pending) {
		bat_charge->dc5v_state_pending_time += DC5V_STATE_TIMER_PERIOD_MS;
		bat_charge->dc5v_state_exit = 0;
		bat_charge->dc5v_state_pending = 1;
	} else {
		bat_charge->dc5v_state_pending_time = 0;
		bat_charge->dc5v_state_exit = 1;
		bat_charge->dc5v_state_pending = 0;
	}

	/* sumit a delay work to check DC5V state */
	if (pending) {
		k_delayed_work_submit(&p->state_timer, K_MSEC(DC5V_STATE_TIMER_PERIOD_MS));
	}
}

/* dc5v state timer handle ex */
void dc5v_state_timer_handler_ex(struct k_work *work)
{
    bat_charge_context_t *bat_charge = bat_charge_get_context();
    dc5v_check_status_t *p = &bat_charge->dc5v_check_status;
	bool pending = false;

    /* check if DC5V UART can be created communication */
    if (dc5v_uart_operate(DC5V_UART_CHECK_IO_TIME, NULL, 500, 0)) {
		pending = true;
    }

	if (pending) {
		bat_charge->dc5v_state_ex_exit = 0;
		bat_charge->dc5v_state_ex_pending = 1;
		k_delayed_work_submit(&p->state_timer_ex, K_MSEC(20));
	} else {
		bat_charge->dc5v_state_ex_exit = 1;
		bat_charge->dc5v_state_ex_pending = 0;
	}
}

#endif

/* get DC5V level by specified millivolt  */
uint32_t dc5v_get_low_level(uint32_t mv)
{
    if (mv >= 3100) {
        return DC5VLV_LEVEL_3_0_V;
    } else if (mv >= 2100) {
        return DC5VLV_LEVEL_2_0_V;
    } else if (mv >= 1100) {
        return DC5VLV_LEVEL_1_0_V;
    }

    return DC5VLV_LEVEL_0_2_V;
}

/* DC5V status check initialization */
void dc5v_check_status_init(void)
{
    bat_charge_context_t *bat_charge = bat_charge_get_context();

    dc5v_check_status_t *p = &bat_charge->dc5v_check_status;

    p->last_state = DC5V_STATE_INVALID;
}

/* DC5V pulldown to prevent charger box enter standby */
void dc5v_prevent_charger_box_standby(bool pull_down)
{
#ifdef CONFIG_ACTS_BATTERY_SUPPLY_CHARGER_BOX	
	bat_charge_context_t *bt_charge = bat_charge_get_context();
    uint8_t standby_current = bt_charge->cfg_ex.Charger_Box_Standby_Current;

	LOG_DBG("pull_down:%d standby_current:%d", pull_down, standby_current);

    if (pull_down) {
        uint8_t  dc5v_pd_current = DC5VPD_CURRENT_DISABLE;

        if (standby_current >= 12) {
            dc5v_pd_current = DC5VPD_CURRENT_25_MA;
        } else if (standby_current >= 7) {
            dc5v_pd_current = DC5VPD_CURRENT_15_MA;
        } else if (standby_current >= 4) {
            dc5v_pd_current = DC5VPD_CURRENT_7_5_MA;
        } else if (standby_current >= 1) {
            dc5v_pd_current = DC5VPD_CURRENT_2_5_MA;
        }

        if (dc5v_pd_current != DC5VPD_CURRENT_DISABLE) {
            dc5v_pull_down_enable(dc5v_pd_current, (uint32_t)-1);
        }
    } else {
        if (standby_current >= 1) {
			/* DC5V pull down disable */
			pmu_chg_ctl_reg_write((1 << CHG_CTL_SVCC_DC5VPD_EN) | CHG_CTL_SVCC_DC5VPD_SET_MASK,
				(0 << CHG_CTL_SVCC_DC5VPD_EN) | (0 << CHG_CTL_SVCC_DC5VPD_SET_SHIFT));
        }
    }
#endif
}



/* cancel charger enable timer */
void charger_enable_timer_delete(void)
{
    bat_charge_context_t *bat_charge = bat_charge_get_context();

	k_delayed_work_cancel(&bat_charge->dc5v_check_status.charger_enable_timer);
}

/* charger enable timer handle */
static void charger_enable_timer_handler(struct k_work *work)
{
	bat_charge_context_t *bat_charge = bat_charge_get_context();

    charger_enable_timer_delete();

#if 0
    pmu_chg_ctl_reg_write((1 << CHG_CTL_SVCC_CHG_EN),
			(1 << CHG_CTL_SVCC_CHG_EN));
#else
	pmu_chg_ctl_reg_write(
	((0x1<<CHG_CTL_SVCC_CV_3V3) | (0x01<<CHG_CTL_SVCC_CHG_EN)),
	((0x0<<CHG_CTL_SVCC_CV_3V3) | (0x1<<CHG_CTL_SVCC_CHG_EN)));

	LOG_INF("DC5V_In, charge enable!");
#endif

	bat_charge->charge_begin_time = k_uptime_get_32();
	LOG_INF("charge begin time: 0x%x", bat_charge->charge_begin_time);

    LOG_INF("charger enabled");

	if((bat_charge->configs.cfg_charge.Charge_Current <= CHARGE_CURRENT_FIRST_LEVEL) || \
		 (bat_charge->cv_stage_2_begin_time != 0)) {
		bat_charge->current_cc_sel = bat_charge->configs.cfg_charge.Charge_Current;
		bat_charge->need_add_cc_by_step = 0;
	} else {
		LOG_INF("DC5V_In, Need from first level!");
		bat_charge->current_cc_sel = CHARGE_CURRENT_FIRST_LEVEL;
		bat_charge->need_add_cc_by_step = 1;
	}
}

/* convert from DC5V ADC value to voltage in millivolt */
uint32_t dc5v_adc_get_voltage_mv(uint32_t adc_val)
{
    uint32_t mv;

    if (!adc_val) {
        return 0;
    }
	  
    mv = (adc_val + 1) * 6000 / 4096;

    return mv;
}

/* convert from battery ADC value to voltage in millivolt */
uint32_t bat_adc_get_voltage_mv(uint32_t adc_val)
{
    uint32_t mv;

    if (!adc_val) {
        return 0;
    }
	
    mv = adc_val*300/1024;

    return mv;
}

/* convert from voltage in millivolt to battery ADC value*/
uint32_t bat_adc_get_voltage_adc(uint32_t mv_val)
{
    uint32_t adcval;

    if (!mv_val) {
        return 0;
    }
	
	adcval = mv_val * 1024/300;

    return adcval;
}


/* get the chargei converts to current in milliampere */
uint32_t chargei_adc_get_current_ma(int adc_val)
{
    uint32_t mA;

    if (adc_val <= 0) {
        return 0;
    }

    mA = (adc_val + 1) * 256 / 4096;

    return mA;
}


/* get the current DC5V state */
dc5v_state_t get_dc5v_current_state(void)
{
    bat_charge_context_t *bat_charge = bat_charge_get_context();
    uint8_t state = DC5V_STATE_OUT;
    uint32_t dc5v_mv, bat_mv;
    dc5v_check_status_t *p = &bat_charge->dc5v_check_status;
    int ret;

#ifdef CONFIG_ACTS_BATTERY_SUPPLY_CHARGER_BOX	
    //support charge box
    CFG_Struct_Charger_Box *cfg = &(bat_charge_get_configs()->cfg_charger_box);

	ret = dc5v_adc_get_sample();
	if (ret < 0) {
		return DC5V_STATE_INVALID;
	}
	
    dc5v_mv = dc5v_adc_get_voltage_mv(ret);

	ret = bat_adc_get_sample();
	if (ret < 0) {
		return DC5V_STATE_INVALID;
	}
	
    bat_mv  = bat_adc_get_voltage_mv(ret);

    if (cfg->Enable_Charger_Box &&
        cfg->Charger_Standby_Voltage >= 3800) {
        uint32_t volt = bat_charge->configs.cfg_charge.Charge_Voltage;

		volt = (4200 * 10 + (volt - CHARGE_VOLTAGE_4_20_V) * 125) / 10;

        if (dc5v_mv >= (volt + 100)) {
            state = DC5V_STATE_IN;
        }
    } else {
    	#if 1
        uint32_t volt = (p->last_state == DC5V_STATE_IN) ? 
            (bat_mv + 50) : 
            (bat_mv + 100);
        
        if (dc5v_mv >= volt) {
            state = DC5V_STATE_IN;
        }
		#else 
        if (soc_pmu_get_dc5v_status() && dc5v_mv >= 3800) {
            state = DC5V_STATE_IN;
        }
		#endif
    }

    if (state == DC5V_STATE_IN) {
        if (p->last_state != DC5V_STATE_IN) {
            /* Once DC5V plug in, delay severial milliseconds to enable charger */
			#if 0
            if (!(bat_charge->bak_PMU_CHG_CTL & (1 << CHG_CTL_SVCC_CHG_EN))) {
				/* delay 500ms to enable charger */
				k_delayed_work_submit(&p->charger_enable_timer, K_MSEC(500));
            }
			#else
			/* delay 500ms to enable charger */
			k_delayed_work_submit(&p->charger_enable_timer, K_MSEC(500));
			#endif

            dc5v_uart_operate(DC5V_UART_SET_ENABLE, NULL, ENABLE, 0);

            dc5v_state_timer_delete();
        }
    } else {
        if (p->last_state == DC5V_STATE_IN ||
            p->last_state == DC5V_STATE_INVALID) {

			/* disable charger enable timer */
            charger_enable_timer_delete();

			#if 0
			/* disable charger anyway */
            if (bat_charge->bak_PMU_CHG_CTL & (1 << CHG_CTL_SVCC_CHG_EN)) {
				LOG_INF("disable charge!");
                pmu_chg_ctl_reg_write((1 << CHG_CTL_SVCC_CHG_EN),
					(0 << CHG_CTL_SVCC_CHG_EN));
            }
			#else
			//just switch to little current, not close charger
			LOG_INF("DC5V out, switch to precharge current!");
			//pmu_chg_ctl_reg_write((0x1<<CHG_CTL_SVCC_CV_3V3), (0x1<<CHG_CTL_SVCC_CV_3V3));
			bat_charge_set_current(bat_charge->precharge_current_sel);
			#endif
        }

        if (p->last_state == DC5V_STATE_IN) {
            bool pending = false;

         	/* If DC5V drop down from high voltage, it is necessary to pulldown DC5V for quick power down */
            if (bat_charge->configs.cfg_features & SYS_ENABLE_DC5VPD_WHEN_DETECT_OUT) {
                dc5v_pull_down_enable(0, 20);
                pending = true;
            }

            dc5v_uart_operate(DC5V_UART_SET_ENABLE, NULL, DISABLE, 500);

            if (dc5v_uart_operate(DC5V_UART_CHECK_IO_TIME, NULL, 500, 0)) {
                pending = true;
            }

            if (cfg->Enable_Charger_Box) {
                pending = true;
            }

            if (pending) {
                dc5v_state_timer_delete();
				k_delayed_work_submit(&p->state_timer, K_MSEC(DC5V_STATE_TIMER_PERIOD_MS));
                state = DC5V_STATE_PENDING;
            }
        }

        /* start to detect DC5V UART communication can be created */
        if ((bat_charge->dc5v_state_exit) && (dc5v_mv >= 2500) &&
            (bat_charge->last_dc5v_mv < 2500)) {
            bool  pending = false;

            dc5v_uart_operate(DC5V_UART_SET_ENABLE, NULL, ENABLE, 0);

			/* delay 500ms to disable DC5V UART if disconnected with charger box  */
            dc5v_uart_operate(DC5V_UART_SET_ENABLE, NULL, DISABLE, 500);

            if (dc5v_uart_operate(DC5V_UART_CHECK_IO_TIME, NULL, 500, 0)) {
                pending = true;
            }

            if (pending) {
                dc5v_state_timer_delete();
                k_delayed_work_submit(&p->state_timer_ex, K_MSEC(DC5V_STATE_EXT_TIMER_PERIOD_MS));
                state = DC5V_STATE_PENDING;
            }
        }

		if ((bat_charge->dc5v_state_pending)
			|| (bat_charge->dc5v_state_ex_pending))
			state = DC5V_STATE_PENDING;

        if (state != DC5V_STATE_PENDING) {
            if (cfg->Enable_Charger_Box) {
                uint32_t volt = cfg->Charger_Standby_Voltage;

                if (volt > 3000) {
                    volt = 3000;
                }

                if (volt < 300) {
                    volt = 300;
                }

                if (dc5v_mv >= volt * 3/4) {
                    state = DC5V_STATE_STANDBY;
                } else {
                    state = DC5V_STATE_OUT;
                }
            } else {
                state = DC5V_STATE_OUT;
            }
        }
    }

#else  // not support charge box

	ret = dc5v_adc_get_sample();
	if (ret < 0) {
		return DC5V_STATE_INVALID;
	}
	
    dc5v_mv = dc5v_adc_get_voltage_mv(ret);

	ret = bat_adc_get_sample();
	if (ret < 0) {
		return DC5V_STATE_INVALID;
	}
	
    bat_mv  = bat_adc_get_voltage_mv(ret);

    if (soc_pmu_get_dc5v_status()) {
        state = DC5V_STATE_IN;
    }
	
#endif

    bat_charge->last_dc5v_mv = dc5v_mv;

    if (p->last_state != state) {
        p->last_state = state;
        LOG_INF("dc5v:%dmv bat:%dmv dc5v_det:%d <%s>", dc5v_mv, bat_mv,
				soc_pmu_get_dc5v_status(), get_dc5v_state_str(state));
    }

    return state;
}

/* get DC5V debounce state */
extern dc5v_state_t get_dc5v_debounce_state(void)
{
    bat_charge_context_t*  bat_charge = bat_charge_get_context();

    return bat_charge->dc5v_debounce_state;
}


/* charger enable, use specific current and voltage */
void bat_charge_ctrl_enable(uint32_t current_sel, uint32_t voltage_sel)
{
    bat_charge_context_t *bat_charge = bat_charge_get_context();
    bat_charge_adjust_cv_offset_t *t = &bat_charge->adjust_cv_offset;
    uint32_t adjust_volt = voltage_sel;

    bat_charge->charge_ctrl_enabled = 1;

    bat_charge_current_percent_buf_init();

    bat_charge_cv_state_buf_init();

    if (t->is_valid) {
        adjust_volt = t->cv_offset;
    } else {
		/* adjust const voltage by efuse  */
        LOG_INF("before adjust_volt: 0x%x", adjust_volt);
		if(bat_charge->charge_cv_offset < 0x80) {
			adjust_volt = adjust_volt + bat_charge->charge_cv_offset;
		} else {
		    if(adjust_volt > (bat_charge->charge_cv_offset & 0x7f)) {
			    adjust_volt = adjust_volt - (bat_charge->charge_cv_offset & 0x7f);			
		    } else {
		    	adjust_volt = 0;
		    }
		}
        LOG_INF("after adjust_volt: 0x%x", adjust_volt);
    }

    LOG_INF("cur:0x%x vol:0x%x adjust_vol:0x%x", current_sel, voltage_sel, adjust_volt);

	if(bat_charge->limit_current_percent == 0) {
		/* current limit percent is 0 to set minimal voltage 3.3V */
	    LOG_INF("limit current to 0, cv use 3.3v");
    	pmu_chg_ctl_reg_write(
        	((CHG_CTL_SVCC_CHG_CURRENT_MASK)|
        	(CHG_CTL_SVCC_CV_OFFSET_MASK) |
        	(0x1<<CHG_CTL_SVCC_CV_3V3)),
        	((current_sel << CHG_CTL_SVCC_CHG_CURRENT_SHIFT) |
        	(adjust_volt << CHG_CTL_SVCC_CV_OFFSET_SHIFT) |
        	(0x1<<CHG_CTL_SVCC_CV_3V3))
   		);	
	} else {
    	pmu_chg_ctl_reg_write(
        	((CHG_CTL_SVCC_CHG_CURRENT_MASK)|
        	(CHG_CTL_SVCC_CV_OFFSET_MASK) |
        	(0x1<<CHG_CTL_SVCC_CV_3V3)),
        	((current_sel << CHG_CTL_SVCC_CHG_CURRENT_SHIFT) |
        	(adjust_volt << CHG_CTL_SVCC_CV_OFFSET_SHIFT) |
        	(0x0<<CHG_CTL_SVCC_CV_3V3))
   		);
	}
}

void bat_current_gradual_change(void)
{
	bat_charge_context_t *bat_charge = bat_charge_get_context();
    int normal_cc_ma, last_cc_ma, new_cc_ma;

	if(bat_charge->need_add_cc_by_step == 0) {
		return;
	}

	if (bat_charge->dc5v_debounce_state != DC5V_STATE_IN) {
		return;
	}

	if((bat_charge->bat_charge_state != BAT_CHG_STATE_PRECHARGE) && \
		  (bat_charge->bat_charge_state != BAT_CHG_STATE_CHARGE)) {
		bat_charge->need_add_cc_by_step = 0;
		return;		  
	}

	if(bat_charge->current_cc_sel >= bat_charge->configs.cfg_charge.Charge_Current) {
		bat_charge->current_cc_sel = bat_charge->configs.cfg_charge.Charge_Current;
		bat_charge->need_add_cc_by_step = 0;
		return;
	}

#if 0
	if(bat_charge->cv_stage_2_begin_time != 0) {
		bat_charge->need_add_cc_by_step = 0;
		return;
	}
#endif

	normal_cc_ma = bat_charge_get_current_ma(bat_charge->configs.cfg_charge.Charge_Current);

	last_cc_ma = bat_charge_get_current_ma(bat_charge->current_cc_sel);
	new_cc_ma = last_cc_ma + CHARGE_CURRENT_ADD_STEP_MA;
	if(new_cc_ma >= normal_cc_ma) {
		new_cc_ma = normal_cc_ma;
	}

	bat_charge->current_cc_sel = bat_charge_get_current_level(new_cc_ma);

	LOG_INF("g_change, %dma - %dma, new_level: %d", last_cc_ma, new_cc_ma, bat_charge->current_cc_sel);

	bat_charge_state_start(bat_charge->current_cc_sel);
}


/* battery charge check enable */
void bat_charge_check_enable(void)
{
    bat_charge_context_t *bat_charge = bat_charge_get_context();
    bat_charge_configs_t *cfg = bat_charge_get_configs();
    uint32_t current_sel;
    uint32_t voltage_sel;

    if (bat_charge->charge_enable_pending) {
		/* check if charger is enabled */
        if (!(bat_charge->bak_PMU_CHG_CTL & (1 << CHG_CTL_SVCC_CHG_EN))) {
			LOG_INF("charge not enabled yet");
            return ;
        }

        current_sel = bat_charge->charge_current_sel;
        voltage_sel = cfg->cfg_charge.Charge_Voltage;

#ifdef CONFIG_ACTS_BATTERY_SUPPORT_FAST_CHARGER
		if(bat_charge->in_fast_charge_stage != 0) {
			if(voltage_sel >= BAT_FAST_CHARGE_CV_LOWER_MV) {
				voltage_sel -= BAT_FAST_CHARGE_CV_LOWER_MV;
			} else {
				//use normal CV
				voltage_sel = cfg->cfg_charge.Charge_Voltage;
			}
		}
#endif

        /* limit max charger current */
        if (bat_charge->limit_current_percent < 100) {
            int  level;
            int  ma;

            ma = bat_charge_get_current_ma(cfg->cfg_charge.Charge_Current);

            ma = ma * bat_charge->limit_current_percent / 100;

            level = bat_charge_get_current_level(ma);

            if (level < cfg->cfg_charge.Charge_Current) {
				/* enter const voltage state and current less than 20%, moreover current limit precent  is 0 */
                if (bat_charge->cv_stage_2_begin_time == 0 ||
                    bat_charge->limit_current_percent == 0) {
                    current_sel = level;
                }
            }

            if(bat_charge->limit_cv_level <= 0x1f) {
            	voltage_sel = bat_charge->limit_cv_level;
            } else {
				;  //empty
            }
        }

        bat_charge_ctrl_enable(current_sel, voltage_sel);

        bat_charge->charge_enable_pending = 0;
	}
}


/* disable charger function */
void bat_charge_ctrl_disable(void)
{
    bat_charge_context_t *bat_charge = bat_charge_get_context();

    bat_charge->charge_enable_pending = 0;

    if (bat_charge->charge_ctrl_enabled) {
        bat_charge->charge_ctrl_enabled = 0;
    }
	
	LOG_INF("Switch 3.3v to stop charge!");
    /* setup the minimal charger voltage as 3.3V to stop charger function */
    pmu_chg_ctl_reg_write((0x1<<CHG_CTL_SVCC_CV_3V3), (0x1<<CHG_CTL_SVCC_CV_3V3));
}


/* configure the charge current */
void bat_charge_set_current(uint32_t current_sel)
{
    pmu_chg_ctl_reg_write(CHG_CTL_SVCC_CHG_CURRENT_MASK,
        (current_sel << CHG_CTL_SVCC_CHG_CURRENT_SHIFT));
}

#if 0
/* get charer current percent */
int get_charge_current_percent(void)
{
	bat_charge_context_t *bat_charge = bat_charge_get_context();
	uint32_t current_sel = bat_charge->configs.cfg_charge.Charge_Current;
	
	int ma1 = bat_charge_get_current_ma(current_sel);
	int ma2 = chargei_adc_get_current_ma(chargei_adc_get_sample());

	int percent = ma2 * 100 / ma1;

    return percent;
}
#endif

#if 0
int get_charge_current_percent_ex(int percent)
{
	bat_charge_context_t *bat_charge = bat_charge_get_context();
    bat_charge_configs_t *cfg = bat_charge_get_configs();

    uint32_t  current_sel =
        (bat_charge->bak_PMU_CHG_CTL & CHG_CTL_SVCC_CHG_CURRENT_MASK) >> CHG_CTL_SVCC_CHG_CURRENT_SHIFT;

    if (current_sel != cfg->cfg_charge.Charge_Current) {
        int ma1 = bat_charge_get_current_ma(current_sel);
        int ma2 = bat_charge_get_current_ma(cfg->cfg_charge.Charge_Current);

        percent = ma1 * percent / ma2;
    }

    return percent;
}
#endif

/* get the CV state
 * const voltage state and current less than 50% return 1
 * const voltage state and current less than 20% return 2
 */
int bat_charge_get_cv_state(void)
{
	bat_charge_context_t *bat_charge = bat_charge_get_context();
    uint32_t cv_3v3 =
        (bat_charge->bak_PMU_CHG_CTL & (1 << CHG_CTL_SVCC_CV_3V3));
    uint32_t chg_en =
        (bat_charge->bak_PMU_CHG_CTL & (1 << CHG_CTL_SVCC_CHG_EN));	
    //int percent;
    int cv1_current_ma;
	int cv2_current_ma;
	int normal_cc_ma;
	
    /* charger disable or limit current percent is 0 */
    if ((cv_3v3 != 0) || (chg_en == 0)) {
		LOG_INF("no charge, cv ret 0!");
        return 0;
    }

#if 0
    percent = get_charge_current_percent();

    if (percent < 20) {
        return 2;
    } else if (percent < 50) {
        return 1;
    }
#endif

	normal_cc_ma = bat_charge_get_current_ma(bat_charge->configs.cfg_charge.Charge_Current);

	cv1_current_ma = normal_cc_ma * 50/100;
	cv2_current_ma = normal_cc_ma * 20/100;

	LOG_INF("cv check: %dma-%dma-%dma, real: %dma", normal_cc_ma, cv1_current_ma, cv2_current_ma, bat_charge->bat_real_charge_current);
	
	if(bat_charge->bat_real_charge_current < cv2_current_ma) {
		return 2;
	} else if(bat_charge->bat_real_charge_current < cv1_current_ma) {
		return 1;
	}

    return 0;
}

/* adjust CV_OFFSET */
void bat_charge_adjust_cv_offset(void)
{
    bat_charge_context_t *bat_charge = bat_charge_get_context();
    bat_charge_adjust_cv_offset_t *t = &bat_charge->adjust_cv_offset;
    //uint32_t voltage_sel = bat_charge->configs.cfg_charge.Charge_Voltage;
    //int  curr_volt = (int)get_battery_voltage_by_time(BAT_VOLT_CHECK_SAMPLE_SEC);
    //int  diff_volt =
    //     4200*10 + (voltage_sel - CHARGE_VOLTAGE_4_20_V) * 125 - curr_volt*10;

    uint32_t cv_offset =
        (bat_charge->bak_PMU_CHG_CTL & CHG_CTL_SVCC_CV_OFFSET_MASK) >> CHG_CTL_SVCC_CV_OFFSET_SHIFT;

#if 0
    if (curr_volt < t->last_volt) {
        /* voltage exception */
        t->adjust_end = 1;
    }
    else if (diff_volt >= 125)
    {
        if (cv_offset < 0x1f)
        {
            cv_offset += 1;
        } else {
            t->adjust_end = 1;
        }
    } else {
        t->adjust_end = 1;
    }

    LOG_INF("curr_volt:%d diff_volt:%d cv_offset:0x%x", curr_volt, diff_volt, cv_offset);

    if (t->adjust_end) {
        LOG_INF("adjust_end");
    }
    
    pmu_chg_ctl_reg_write (
        (CHG_CTL_SVCC_CV_OFFSET_MASK),
        (cv_offset << CHG_CTL_SVCC_CV_OFFSET_SHIFT)
	);

    if (t->last_volt < curr_volt) {
        t->last_volt = curr_volt;
    }

    t->is_valid  = 1;
    t->cv_offset = cv_offset;    
#else
    t->adjust_end = 1;
    t->is_valid = 0;
    t->cv_offset = cv_offset;    
#endif    
}

bool bat_charge_adjust_current(void)
{
    bat_charge_context_t *bat_charge = bat_charge_get_context();

    uint32_t chg_current =
	(bat_charge->bak_PMU_CHG_CTL & CHG_CTL_SVCC_CHG_CURRENT_MASK) >> CHG_CTL_SVCC_CHG_CURRENT_SHIFT;

    bat_charge_adjust_current_t *adjust_current = &bat_charge->adjust_current;

    if (bat_charge->cv_stage_2_begin_time == 0) {
        return false;
    }


	adjust_current->is_valid = 0;
	adjust_current->charge_current =
		bat_charge->configs.cfg_charge.Charge_Current;

#if 0
    switch (bat_charge->configs.cfg_charge.Charge_Stop_Current) {
        case CHARGE_STOP_CURRENT_20_PERCENT:
        {
			adjust_current->stop_current_ma = 	bat_charge_get_current_ma(bat_charge->configs.cfg_charge.Charge_Current) * 20/100;
            break;
        }

		case CHARGE_STOP_CURRENT_5_PERCENT:		
        {
			adjust_current->stop_current_ma = 	bat_charge_get_current_ma(bat_charge->configs.cfg_charge.Charge_Current) * 5/100;
            break;
        }

        case CHARGE_STOP_CURRENT_30_MA:
        {
			adjust_current->stop_current_ma = 30;
            break;
        }

        case CHARGE_STOP_CURRENT_20_MA:
        {
			adjust_current->stop_current_ma = 20;
            break;
        }

        case CHARGE_STOP_CURRENT_16_MA:
        {
			adjust_current->stop_current_ma = 16;
            break;
        }

        case CHARGE_STOP_CURRENT_12_MA:
        {
			adjust_current->stop_current_ma = 12;
            break;
        }

        case CHARGE_STOP_CURRENT_8_MA:
        {
			adjust_current->stop_current_ma = 8;
            break;
        }

        case CHARGE_STOP_CURRENT_6_4_MA:
        {
			adjust_current->stop_current_ma = 6;
            break;
        }

        case CHARGE_STOP_CURRENT_5_MA:
        {
			adjust_current->stop_current_ma = 5;
            break;
        }
    }
#endif

	adjust_current->stop_current_ma = bat_charge->configs.cfg_charge.Charge_Stop_Current;
	LOG_INF("charge stop current: %dma", adjust_current->stop_current_ma);
	

    if (chg_current != adjust_current->charge_current) {
        chg_current = adjust_current->charge_current;

        LOG_INF("new_chg_current:0x%x", chg_current);

        bat_charge_current_percent_buf_init();

        pmu_chg_ctl_reg_write (
            CHG_CTL_SVCC_CHG_CURRENT_MASK,
            (chg_current << CHG_CTL_SVCC_CHG_CURRENT_SHIFT)
        );

        return true;
    }

    return false;
}

static const struct {
    uint8_t   level;
    uint16_t  ma;
} bat_charge_current_table[] = {
    { CHARGE_CURRENT_10_MA,  10  },
    { CHARGE_CURRENT_20_MA,  20  },
    { CHARGE_CURRENT_30_MA,  30  },
    { CHARGE_CURRENT_40_MA,  40  },
	{ CHARGE_CURRENT_50_MA,  50  },
    { CHARGE_CURRENT_60_MA,  60  },
    { CHARGE_CURRENT_70_MA,  70  },
    { CHARGE_CURRENT_80_MA,  80  },
    { CHARGE_CURRENT_90_MA,  90  },
    { CHARGE_CURRENT_100_MA, 100 },
    { CHARGE_CURRENT_120_MA, 120 },
    { CHARGE_CURRENT_140_MA, 140 },
    { CHARGE_CURRENT_160_MA, 160 },
    { CHARGE_CURRENT_180_MA, 180 },
    { CHARGE_CURRENT_200_MA, 200 },
    { CHARGE_CURRENT_240_MA, 240 },
};

/* get the charger current in milliampere by specified level */
int bat_charge_get_current_ma(int level)
{
    int  i;

    for (i = ARRAY_SIZE(bat_charge_current_table) - 1; i > 0; i--) {
        if (level >= bat_charge_current_table[i].level) {
            break;
        }
    }

    return bat_charge_current_table[i].ma;
}

/* get the charger current level by specified milliampere */
int bat_charge_get_current_level(int ma)
{
    int  i;

    for (i = ARRAY_SIZE(bat_charge_current_table) - 1; i > 0; i--) {
        if (ma >= bat_charge_current_table[i].ma) {
            break;
        }
    }

    if (i < ARRAY_SIZE(bat_charge_current_table) - 1) {
        int  diff1 = ma - bat_charge_current_table[i].ma;
        int  diff2 = ma - bat_charge_current_table[i+1].ma;

        if (abs(diff1) > abs(diff2)) {
            i = i+1;
        }
    }

    return bat_charge_current_table[i].level;
}


/* limit charger current by specified percent (ranger from 0 ~ 100) */
void bat_charge_limit_current(uint32_t percent)
{
    bat_charge_context_t *bat_charge = bat_charge_get_context();

    if (percent > 100) {
        percent = 100;
    }

    if (bat_charge->limit_current_percent != percent) {
        LOG_INF("limit percent:%d%%", percent);
		
		/* reset CV state calculate time if previous percent is 0 */
        if (bat_charge->limit_current_percent == 0) {
            if (bat_charge->cv_stage_1_begin_time != 0) {
                bat_charge->cv_stage_1_begin_time = k_uptime_get_32();
            }

            if (bat_charge->cv_stage_2_begin_time != 0) {
                bat_charge->cv_stage_2_begin_time = k_uptime_get_32();
            }
        }

        bat_charge->limit_current_percent = percent;

        if (bat_charge->charge_ctrl_enabled) {
            bat_charge->charge_enable_pending = 1;
        }
    }
}


/* calibrate the charger internal contant current by efuse */
static int bat_charge_cc_calibration(uint8_t *cc_offset)
{
	int result;
	uint32_t read_val;

	result = soc_atp_get_pmu_calib(1, &read_val);
    if(result != 0) {
        LOG_ERR("ChargeCC cal fail!");
		return -1;
    }

	*cc_offset = (uint8_t)read_val;

	return 0;
}


/* calibrate the current convert from charger to ADC by efuse */
static int bat_charge_chargiadc_calibration(uint8_t *chargi)
{
	int result;
	uint32_t read_val;

	result = soc_atp_get_pmu_calib(3, &read_val);
    if(result != 0) {
        LOG_ERR("Chargiadc cal fail!");
		return -1;
    }

	*chargi = (uint8_t)read_val;

	return 0;
}

/* calibrate for B version */
static int bat_charge_chargiset1_calibration(uint8_t *chargi)
{
	int result;
	uint32_t read_val;

	result = soc_atp_get_pmu_calib(8, &read_val);
    if(result != 0) {
        LOG_ERR("Chargiset1 cal fail!");
		return -1;
    }

	*chargi = (uint8_t)read_val;

	return 0;
}


/* calibrate the charger internal contant voltage by efuse */
static int bat_charge_cv_calibration(uint8_t *cv_offset)
{
	int result;
	uint32_t read_val;

	result = soc_atp_get_pmu_calib(0, &read_val);
    if(result != 0) {
        LOG_ERR("ChargeCV cal fail!");
		return -1;
    }

	*cv_offset = (uint8_t)read_val;

	return 0;
}


/*	battery and charger control initialization */
void bat_charge_ctrl_init(void)
{
    bat_charge_context_t *bat_charge = bat_charge_get_context();
	uint8_t val, val_new;

	//cc offset
	val = (bat_charge->bak_PMU_CHG_CTL & CHG_CTL_SVCC_CC_OFFSET_MASK) >> CHG_CTL_SVCC_CC_OFFSET_SHIFT;
	/* get the calibrated constant current from efuse */
	if (!bat_charge_cc_calibration(&val_new)) {
		LOG_INF("new const current:%d", val_new);
		val = val_new;
		pmu_chg_ctl_reg_write(CHG_CTL_SVCC_CC_OFFSET_MASK, val << CHG_CTL_SVCC_CC_OFFSET_SHIFT);
	}

	LOG_INF("const current:%d", val);

    //cv offset 
    val = (bat_charge->bak_PMU_CHG_CTL & CHG_CTL_SVCC_CV_OFFSET_MASK) >> CHG_CTL_SVCC_CV_OFFSET_SHIFT;
	/* get the calibrated constant voltage from efuse */
	if (!bat_charge_cv_calibration(&val_new)) {
		LOG_INF("new const voltage:%d", val_new);
		val = val_new;
		pmu_chg_ctl_reg_write(CHG_CTL_SVCC_CV_OFFSET_MASK, val << CHG_CTL_SVCC_CV_OFFSET_SHIFT);
	}

	LOG_INF("const voltage:%d", val);
	
    //calibrate by 4200mv
    if(val >= CHARGE_VOLTAGE_4_20_V) {
		bat_charge->charge_cv_offset = val - CHARGE_VOLTAGE_4_20_V;            //add
    } else {
    	bat_charge->charge_cv_offset = 0x80 + CHARGE_VOLTAGE_4_20_V - val;     //sub
    }

	LOG_INF("const voltage cal:%d, offset: 0x%x", val, bat_charge->charge_cv_offset);
    //chargei set, need load by software
	val = (sys_read32(BDG_CTL_SVCC) & BDG_CTL_SVCC_CHARGEI_SET_MASK) >> BDG_CTL_SVCC_CHARGEI_SET_SHIFT;

	/* get the calibrated charge convert value from efuse */
	if (!bat_charge_chargiadc_calibration(&val_new)) {
		LOG_INF("new chargiadc:%d", val_new);
		val = val_new;
	}

	LOG_INF("charge adc convert ajustment:%d", val);
	sys_write32((sys_read32(BDG_CTL_SVCC) & ~BDG_CTL_SVCC_CHARGEI_SET_MASK)
				| (val << BDG_CTL_SVCC_CHARGEI_SET_SHIFT), BDG_CTL_SVCC);

	k_busy_wait(300);
	LOG_DBG("BDG_CTL_SVCC:0x%x", sys_read32(BDG_CTL_SVCC));

	//chargei set1
	if (!bat_charge_chargiset1_calibration(&val_new)) {
		LOG_INF("new chargi_set1:%d", val_new);
		if(val_new != 0) {
			// cc don't add 18%
			pmu_chg_ctl_reg_write((1<<CHG_CTL_SVCC_CC_ADD), 0<<CHG_CTL_SVCC_CC_ADD);
			LOG_INF("do not add cc: 0x%x", sys_read32(CHG_CTL_SVCC));
		}
	}

	/* set the charger current to avoid power supply insufficiently */
	if(bat_charge->configs.cfg_charge.Charge_Current <= CHARGE_CURRENT_FIRST_LEVEL) {
		bat_charge->current_cc_sel = bat_charge->configs.cfg_charge.Charge_Current;
		bat_charge->need_add_cc_by_step = 0;
	} else {
		bat_charge->current_cc_sel = CHARGE_CURRENT_FIRST_LEVEL;
		bat_charge->need_add_cc_by_step = 1;
	}
	LOG_INF("set init charger const current:%d", bat_charge->current_cc_sel);
    bat_charge_set_current(bat_charge->current_cc_sel);

    /* check DC5V status */
    dc5v_check_status_init();

	k_delayed_work_init(&bat_charge->dc5v_check_status.charger_enable_timer, charger_enable_timer_handler);
	
#ifdef CONFIG_ACTS_BATTERY_SUPPLY_CHARGER_BOX
	k_delayed_work_init(&bat_charge->dc5v_pd_timer, dc5v_pull_down_disable);
	k_delayed_work_init(&bat_charge->dc5v_check_status.state_timer, dc5v_state_timer_handler);
	k_delayed_work_init(&bat_charge->dc5v_check_status.state_timer_ex, dc5v_state_timer_handler_ex);
#endif
}

