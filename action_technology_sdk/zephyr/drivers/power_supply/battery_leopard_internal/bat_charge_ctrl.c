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

#include <logging/log.h>
LOG_MODULE_REGISTER(bat_ctl, CONFIG_LOG_DEFAULT_LEVEL);

#define PMUADC_WAIT_OVER_SAMPLE_US        (20 * 1000)

void pmusvcc_regbackup(void)
{
	bat_charge_context_t *bat_charge = bat_charge_get_context();
	bat_charge->bak_PMU_CHG_CTL = soc_pmu_get_chg_ctl_svcc();
}

/* PMU charger control register update */
void pmu_chg_ctl_reg_write(uint32_t mask, uint32_t value)
{
	bat_charge_context_t *bat_charge = bat_charge_get_context();
	
    bat_charge->bak_PMU_CHG_CTL = (bat_charge->bak_PMU_CHG_CTL & ~mask) | (value & mask);
    soc_pmu_set_chg_ctl_svcc(mask, value);

	LOG_INF("bak_PMU_CHG_CTL:0x%x; rb_PMU_CHG_CTL:0x%x\n", bat_charge->bak_PMU_CHG_CTL, soc_pmu_get_chg_ctl_svcc());
}

/* open charger */
void bat_charger(bool open)
{
    if (open) {
	    //LOG_INF("boot open charger!");
	    pmu_chg_ctl_reg_write(
	    	(0x1 << CHG_CTL_SVCC_CHG_EN),
	    	(0x1 << CHG_CTL_SVCC_CHG_EN));
    } else {
	    //LOG_INF("boot close charger!");
	    pmu_chg_ctl_reg_write(
	    	(0x1 << CHG_CTL_SVCC_CHG_EN),
	    	(0x0 << CHG_CTL_SVCC_CHG_EN));
    }
}

void bat_set_bd_comp(void)
{
    pmu_chg_ctl_reg_write(CHG_CTL_SVCC_BD_COMP_MASK, (BAT_BD_COMP_50_MO << CHG_CTL_SVCC_BD_COMP_SHIFT));
}

void bat_lowpower_charge_setting(int current_ma)
{
	uint16_t level = bat_charge_get_current_level(current_ma);
    pmu_chg_ctl_reg_write(
    	((CHG_CTL_SVCC_CHG_CURRENT_MASK) |
    	(CHG_CTL_SVCC_CV_OFFSET_MASK) | (CHG_CTL_SVCC_BAT_PD_MASK) |
    	(0x1 << CHG_CTL_SVCC_CV_3V3) | (0x01 << CHG_CTL_SVCC_CHG_EN)),
    	((level << CHG_CTL_SVCC_CHG_CURRENT_SHIFT) |
    	(CHARGE_VOLTAGE_4_35_V << CHG_CTL_SVCC_CV_OFFSET_SHIFT) |
    	(0x0 << CHG_CTL_SVCC_CV_3V3) | (0x1 << CHG_CTL_SVCC_BAT_PD_SHIFT) |
    	(0x1 << CHG_CTL_SVCC_CHG_EN)));
}

void pmuvdd_set_vd12_vc18_mode(bool vd12_dcdc, bool vc18_dcdc)
{
    uint32_t vdd_reg = soc_pmu_get_vout_ctl0();

    LOG_INF("vd12 sw %s; vc18 sw %s.", vd12_dcdc?"dcdc":"ldo", vc18_dcdc?"dcdc":"ldo");
    if (vd12_dcdc) {
        soc_pmu_set_vd12_voltage(soc_pmu_get_vdd_voltage() + 50);
        vdd_reg |= VOUT_CTL0_VD12_SW_DCDC;
    } else {
        soc_pmu_set_vd12_voltage(soc_pmu_get_vdd_voltage() + 50 * 2);
        vdd_reg &= ~(VOUT_CTL0_VD12_SW_DCDC);
    }

    if (vc18_dcdc) {
        vdd_reg |= VOUT_CTL0_VC18_SW_DCDC;
    } else {
        vdd_reg &= ~(VOUT_CTL0_VC18_SW_DCDC);
    }
    soc_pmu_set_vout_ctl0((VOUT_CTL0_VD12_SW_DCDC | VOUT_CTL0_VC18_SW_DCDC), vdd_reg);
}

int pmuadc_set_bat_pd_resistance(bool open)
{
	bat_charge_context_t *bat_charge = bat_charge_get_context();

    pmusvcc_regbackup();

    if (open) {
        //set bat pull down resistance only in charge state.
        if (!(bat_charge->bak_PMU_CHG_CTL & (0x1 << CHG_CTL_SVCC_BAT_PD_SHIFT))) {
            pmu_chg_ctl_reg_write(CHG_CTL_SVCC_BAT_PD_MASK, (0x1 << CHG_CTL_SVCC_BAT_PD_SHIFT));
        }
    } else {
        //remove bat pull down resistance in others state.
        if (bat_charge->bak_PMU_CHG_CTL & (0x1 << CHG_CTL_SVCC_BAT_PD_SHIFT)) {
            pmu_chg_ctl_reg_write(CHG_CTL_SVCC_BAT_PD_MASK, (0x0 << CHG_CTL_SVCC_BAT_PD_SHIFT));
        }
    }

    return 0;
}

#ifdef CONFIG_ACTS_BATTERY_SUPPORT_CHARGER_NTC
int pmuadc_set_ntc_resistance(bool open)
{
    bat_charge_context_t *bat_charge = bat_charge_get_context();
    //uint32_t ctl_reg = soc_pmu_get_pmuadc_ctl();
    uint32_t dig_reg = soc_pmu_get_pmuadcdig_ctl();
    uint8_t ntc_chan = bat_charge->configs.cfg_bat_ntc.NTC_Settings.LRADC_Channel;

    if (open) {
	    if((ntc_chan >= PMUADC_ID_LRADC1) && (ntc_chan <= PMUADC_ID_LRADC6)) {
            board_set_ntc_pinmux_info(true, true);
            //set ntc pull up resistance only in charge state.
            if (ntc_chan == PMUADC_ID_LRADC2) {
                if (!(dig_reg & (1 << 5))) {
                    dig_reg |= (1 << 5);
                    soc_pmu_set_pmuadcdig_ctl((1 << 5), dig_reg);
                }
            }
            //enable ntc channel.
            //no need!adc driver will disable ntc chan auto.
            //if (!(ctl_reg & (1 << ntc_chan))) {
	    	//    ctl_reg |= (1 << ntc_chan);
            //    soc_pmu_set_pmuadc_ctl((1 << ntc_chan), ctl_reg);
            //    soc_pmu_clear_pmuadc_pd();
            //}
	    }
    } else {
	    if((ntc_chan >= PMUADC_ID_LRADC1) && (ntc_chan <= PMUADC_ID_LRADC6)) {
            //remove ntc pull up resistance in others state.
            if (ntc_chan == PMUADC_ID_LRADC2) {
                if (dig_reg & (1 << 5)) {
                    dig_reg &= ~(1 << 5);
                    soc_pmu_set_pmuadcdig_ctl((1 << 5), dig_reg);
                }
            }
            //disable ntc channel.
            //no need!adc driver will disable ntc chan auto.
            //if (ctl_reg & (1 << ntc_chan)) {
	    	//    ctl_reg &= ~(1 << ntc_chan);
            //    soc_pmu_set_pmuadc_ctl((1 << ntc_chan), ctl_reg);
            //    soc_pmu_clear_pmuadc_pd();
            //}
            board_set_ntc_pinmux_info(false, false);
        }
    }

    return 0;
}

int pmuadc_set_ntc_ref_resistance(bool open)
{
    bat_charge_context_t *bat_charge = bat_charge_get_context();
    //uint32_t ctl_reg = soc_pmu_get_pmuadc_ctl();
    uint8_t vcc_chan = bat_charge->configs.cfg_bat_ntc.NTC_Settings.Ref_Channel;

    if (open) {
	    if((vcc_chan >= PMUADC_ID_LRADC1) && (vcc_chan <= PMUADC_ID_LRADC6)) {
            board_set_ntc_pinmux_info(true, false);
            //enable ntc channel.
            //no need!adc driver will open vcc chan auto.
            //if (!(ctl_reg & (1 << vcc_chan))) {
	    	//    ctl_reg |= (1 << vcc_chan);
            //    soc_pmu_set_pmuadc_ctl((1 << vcc_chan), ctl_reg);
            //    soc_pmu_clear_pmuadc_pd();
            //}
	    }
    } else {
	    if((vcc_chan >= PMUADC_ID_LRADC1) && (vcc_chan <= PMUADC_ID_LRADC6)) {
            //disable ntc channel.
            //no need!adc driver will open vcc chan auto.
            //if (ctl_reg & (1 << vcc_chan)) {
	    	//    ctl_reg &= ~(1 << vcc_chan);
            //    soc_pmu_set_pmuadc_ctl((1 << vcc_chan), ctl_reg);
            //    soc_pmu_clear_pmuadc_pd();
            //}
            board_set_ntc_pinmux_info(false, false);
        }
    }

    return 0;
}
#endif

int pmuadc_digital_setting(void)
{
	uint32_t reg = soc_pmu_get_pmuadcdig_ctl();

    //set lradc2 over sampling
	reg &= ~(3 << 22);
	reg |= (CONFIG_PMUADC_LRADC2_AVG << 22);
    //set lradc6 over sampling, lradc6 value only use as lradc2 scale value, so over sampling value set the same with lradc2.
	reg &= ~(3 << 30);
	reg |= (CONFIG_PMUADC_LRADC2_AVG << 30);

    soc_pmu_set_pmuadcdig_ctl(((0x03 << 10) | (3 << 22) | (3 << 30)), reg);

	LOG_INF("PMUADC_DIGCTL:0x%08x", soc_pmu_get_pmuadcdig_ctl());
	return 0;
}

#ifdef CONFIG_ACTS_BATTERY_SUPPORT_MINI_CHARGER_NTC
int pmuadc_wait_NTC_sample_complete(void)
{
	bat_charge_context_t *bat_charge = bat_charge_get_context();
	uint8_t ntc_chan;
	uint32_t timestamp = k_cycle_get_32();

	ntc_chan = bat_charge->configs.cfg_bat_ntc.NTC_Settings.LRADC_Channel;

	while((soc_pmu_get_pmuadc_pd() & BIT(ntc_chan)) == 0) {
		if (k_cyc_to_us_floor32(k_cycle_get_32() - timestamp) > PMUADC_WAIT_OVER_SAMPLE_US) {
			LOG_ERR("failed to get ntcadc pending, PD:0x%x CTL:0x%x", soc_pmu_get_pmuadc_pd(), soc_pmu_get_pmuadc_ctl());
			return -1;
		}
	}

	return 0;
}
#endif

/**
**	set init charge current and voltage
**/
void bat_mini_charge_set(uint8_t ntc_index)
{
	bat_charge_context_t *bat_charge = bat_charge_get_context();
	bat_charge_configs_t *configs = &bat_charge->configs;
	uint8_t mini_charge_level;
	uint16_t mini_charge_current;

#ifndef CONFIG_ACTS_BATTERY_SUPPORT_MINI_CHARGER_NTC
	if(bat_charge->mini_charge_stage == 1) {
		mini_charge_level = configs->cfg_charge.Mini_Charge_Current_1;
	} else if(bat_charge->mini_charge_stage == 2) {
		mini_charge_level = configs->cfg_charge.Mini_Charge_Current_2;
	} else if(bat_charge->mini_charge_stage == 3) {
		mini_charge_level = configs->cfg_charge.Mini_Charge_Current_3;	
	} else {
		LOG_INF("mini charge stage err, set default!");
		mini_charge_level = configs->cfg_charge.Mini_Charge_Current_1;
		bat_charge->mini_charge_stage = 1;
	}
    mini_charge_current = bat_charge_get_current_ma(mini_charge_level);
#else
	static uint8_t last_ntc_index = 0xff;

	if(ntc_index != 0xff) {
		if(bat_charge->mini_charge_stage == 1) {
			mini_charge_current = configs->cfg_bat_init_ntc.init_stage1_NTC_current_ma[ntc_index];
		} else if(bat_charge->mini_charge_stage == 2) {
			mini_charge_current = configs->cfg_bat_init_ntc.init_stage2_NTC_current_ma[ntc_index];
		} else {
			mini_charge_current = configs->cfg_bat_init_ntc.init_stage3_NTC_current_ma[ntc_index];
		}
		
		LOG_INF("mini charge NTC change: %dmA", mini_charge_current);

		//mini_charge_level = bat_charge_get_current_level(mini_charge_current);

		last_ntc_index = ntc_index;
	} else {
		LOG_INF("mini stage change, use last ntc index: %d", last_ntc_index);
		if(last_ntc_index > 4) {
			last_ntc_index = 2;  //normal
		}

		if(bat_charge->mini_charge_stage == 1) {
			mini_charge_current = configs->cfg_bat_init_ntc.init_stage1_NTC_current_ma[last_ntc_index];
		} else if(bat_charge->mini_charge_stage == 2) {
			mini_charge_current = configs->cfg_bat_init_ntc.init_stage2_NTC_current_ma[last_ntc_index];
		} else {
			mini_charge_current = configs->cfg_bat_init_ntc.init_stage3_NTC_current_ma[last_ntc_index];
		}

		//mini_charge_level = bat_charge_get_current_level(mini_charge_current);
	}
#endif

    mini_charge_current += bat_charge->bat_charge_system_consume_ma;
    mini_charge_level = bat_charge_get_current_level(mini_charge_current);

	pmu_chg_ctl_reg_write(
		((CHG_CTL_SVCC_CHG_CURRENT_MASK)|
		(CHG_CTL_SVCC_CV_OFFSET_MASK) |
		(0x1 << CHG_CTL_SVCC_CV_3V3) | (0x01 << CHG_CTL_SVCC_CHG_EN)),
		((mini_charge_level << CHG_CTL_SVCC_CHG_CURRENT_SHIFT) |
		(configs->cfg_charge.Charge_Voltage << CHG_CTL_SVCC_CV_OFFSET_SHIFT) |
		(0x0 << CHG_CTL_SVCC_CV_3V3) |
		(0x1 << CHG_CTL_SVCC_CHG_EN)));

	LOG_INF("mini charge, cc: %dma, cv_level: %d", bat_charge_get_current_ma(mini_charge_level), configs->cfg_charge.Charge_Voltage);
}

void bat_charger_set_before_enter_s4(void)
{
	bat_charge_context_t *bat_charge = bat_charge_get_context();

	LOG_INF("set before enter s4!");
	pmu_chg_ctl_reg_write(
		((CHG_CTL_SVCC_CHG_CURRENT_MASK)| (0x01 << CHG_CTL_SVCC_CHG_EN)),
		((bat_charge->precharge_current_sel << CHG_CTL_SVCC_CHG_CURRENT_SHIFT) | 
		(0x1 << CHG_CTL_SVCC_CHG_EN)));

}

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

	pmu_chg_ctl_reg_write(
	((0x1 << CHG_CTL_SVCC_CV_3V3) | (0x01 << CHG_CTL_SVCC_CHG_EN)),
	((0x0 << CHG_CTL_SVCC_CV_3V3) | (0x1 << CHG_CTL_SVCC_CHG_EN)));

	LOG_INF("DC5V_In, charge enable!");

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
	
    mv = adc_val * 300 / 1024;

    return mv;
}

/* convert from voltage in millivolt to battery ADC value*/
uint32_t bat_adc_get_voltage_adc(uint32_t mv_val)
{
    uint32_t adcval;

    if (!mv_val) {
        return 0;
    }
	
	adcval = mv_val * 1024 / 300;

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
        	((CHG_CTL_SVCC_CHG_CURRENT_MASK) |
        	(CHG_CTL_SVCC_CV_OFFSET_MASK) |
        	(0x1 << CHG_CTL_SVCC_CV_3V3)),
        	((current_sel << CHG_CTL_SVCC_CHG_CURRENT_SHIFT) |
        	(adjust_volt << CHG_CTL_SVCC_CV_OFFSET_SHIFT) |
        	(0x1 << CHG_CTL_SVCC_CV_3V3))
   		);	
	} else {
    	pmu_chg_ctl_reg_write(
        	((CHG_CTL_SVCC_CHG_CURRENT_MASK) |
        	(CHG_CTL_SVCC_CV_OFFSET_MASK) |
        	(0x1 << CHG_CTL_SVCC_CV_3V3)),
        	((current_sel << CHG_CTL_SVCC_CHG_CURRENT_SHIFT) |
        	(adjust_volt << CHG_CTL_SVCC_CV_OFFSET_SHIFT) |
        	(0x0 << CHG_CTL_SVCC_CV_3V3))
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
    pmu_chg_ctl_reg_write((0x1 << CHG_CTL_SVCC_CV_3V3), (0x1 << CHG_CTL_SVCC_CV_3V3));
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

	cv1_current_ma = normal_cc_ma * 50 / 100;
	cv2_current_ma = normal_cc_ma * 20 / 100;

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
    { CHARGE_CURRENT_20_MA,  20  },
    { CHARGE_CURRENT_30_MA,  30  },
    { CHARGE_CURRENT_40_MA,  40  },
    { CHARGE_CURRENT_50_MA,  50  },
	{ CHARGE_CURRENT_60_MA,  60  },
    { CHARGE_CURRENT_70_MA,  70  },
    { CHARGE_CURRENT_80_MA,  80  },
    { CHARGE_CURRENT_90_MA,  90  },
    { CHARGE_CURRENT_100_MA, 100 },
    { CHARGE_CURRENT_150_MA, 150 },
    { CHARGE_CURRENT_200_MA, 200 },
    { CHARGE_CURRENT_250_MA, 250 },
    { CHARGE_CURRENT_300_MA, 300 },
    { CHARGE_CURRENT_350_MA, 350 },
    { CHARGE_CURRENT_400_MA, 400 },
    { CHARGE_CURRENT_450_MA, 450 },
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
	val = (soc_pmu_get_bdg_ctl_svcc() & BDG_CTL_SVCC_CHARGEI_SET_MASK) >> BDG_CTL_SVCC_CHARGEI_SET_SHIFT;

	/* get the calibrated charge convert value from efuse */
	if (!bat_charge_chargiadc_calibration(&val_new)) {
		LOG_INF("new chargiadc:%d", val_new);
		val = val_new;
	}

	LOG_INF("charge adc convert ajustment:%d", val);
    soc_pmu_set_bdg_ctl_svcc(BDG_CTL_SVCC_CHARGEI_SET_MASK, val << BDG_CTL_SVCC_CHARGEI_SET_SHIFT);
	LOG_DBG("BDG_CTL_SVCC:0x%x", soc_pmu_get_bdg_ctl_svcc());

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
}

