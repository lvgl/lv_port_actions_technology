/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file power manager interface
 */
#if defined(CONFIG_SYS_LOG)
#define SYS_LOG_NO_NEWLINE
#ifdef SYS_LOG_DOMAIN
#undef SYS_LOG_DOMAIN
#endif
#define SYS_LOG_DOMAIN "power"
#endif

#include <os_common_api.h>
#include <mem_manager.h>
#include <msg_manager.h>
#include <sys_event.h>
#include <sys_monitor.h>
#include <property_manager.h>

#ifndef CONFIG_SIMULATOR
#include <drivers/power_supply.h>
#endif

#ifdef CONFIG_BLUETOOTH
#include <bt_manager.h>
#endif

#ifdef CONFIG_SYS_WAKELOCK
#include <sys_wakelock.h>
#endif

#include <stdlib.h>
#include <stdio.h>


#define DEFAULT_BOOTPOWER_LEVEL	3100000
#define DEFAULT_NOPOWER_LEVEL	    3400000
#define DEFAULT_LOWPOWER_LEVEL	3600000

#define DEFAULT_REPORT_PERIODS	(60*1000)

struct power_manager_info {
	struct device *dev;
	bat_charge_callback_t cb;
	bat_charge_event_t event;
	bat_charge_event_para_t *para;
	int nopower_level;
	int lowpower_level;
	int current_vol;
	int current_cap;
	int slave_vol;
	int slave_cap;
	int battary_changed;
	uint32_t charge_full_flag;
	uint32_t report_timestamp;
	uint32_t wakelock_flag;
};

struct power_manager_info *power_manager = NULL;

void power_supply_report(bat_charge_event_t event, bat_charge_event_para_t *para)
{
	struct app_msg  msg = {0};

	if (!power_manager) {
		return;
	}
	SYS_LOG_INF("event %d\n", event);
	switch (event) {
	case BAT_CHG_EVENT_DC5V_IN:
		break;
	case BAT_CHG_EVENT_DC5V_OUT:
		break;
	case BAT_CHG_EVENT_CHARGE_START:
		break;
	case BAT_CHG_EVENT_CHARGE_FULL:
		power_manager->charge_full_flag = 1;
		break;

	case BAT_CHG_EVENT_VOLTAGE_CHANGE:
		SYS_LOG_INF("voltage change %u uV\n", para->voltage_val);
		power_manager->current_vol = para->voltage_val;
		break;

	case BAT_CHG_EVENT_CAP_CHANGE:
		SYS_LOG_INF("cap change %u\n", para->cap);
		power_manager->current_cap = para->cap;
		power_manager->battary_changed = 1;
		break;

    case BAT_CHG_EVENT_EXIT_MINI_CHARGE:
        break;

	default:
		break;
	}

	msg.type = MSG_BAT_CHARGE_EVENT;
	msg.cmd = event;
	send_async_msg("main", &msg);

}

static int get_system_bat_info(int property)
{
	union power_supply_propval val;

	int ret;

	if (!power_manager || !power_manager->dev) {
		SYS_LOG_ERR("dev not found\n");
		return -ENODEV;
	}

	ret = power_supply_get_property(power_manager->dev, property, &val);
	if (ret < 0) {
		SYS_LOG_ERR("get property err %d\n", ret);
		return -ENODEV;
	}

	return val.intval;
}

static int set_system_bat_info(int property, union power_supply_propval *val)
{

	if (!power_manager || !power_manager->dev) {
		SYS_LOG_ERR("dev not found\n");
		return -ENODEV;
	}

	power_supply_set_property(power_manager->dev, property, val);

	return 0;
}

int power_manager_get_battery_capacity(void)
{
#ifdef CONFIG_TWS
	int report_cap = get_system_bat_info(POWER_SUPPLY_PROP_CAPACITY);
	if (bt_manager_tws_get_dev_role() == BTSRV_TWS_MASTER) {
		if (report_cap < power_manager->slave_cap)
			report_cap = power_manager->slave_cap;		
	}
	return report_cap;
#else
	return get_system_bat_info(POWER_SUPPLY_PROP_CAPACITY);
#endif
}

int power_manager_get_battery_capacity_local(void)
{
	return get_system_bat_info(POWER_SUPPLY_PROP_CAPACITY);
}

int power_manager_get_charge_status(void)
{
	return get_system_bat_info(POWER_SUPPLY_PROP_STATUS);
}

int power_manager_get_battery_vol(void)
{
	return get_system_bat_info(POWER_SUPPLY_PROP_VOLTAGE_NOW);
}

int power_manager_get_dc5v_status(void)
{
	return get_system_bat_info(POWER_SUPPLY_PROP_DC5V);
}

int power_manager_get_dc5v_status_chargebox(void)
{
	return get_system_bat_info(POWER_SUPPLY_PROP_DC5V_STATUS);
}

int power_manager_get_dc5v_voltage(void)
{
	return get_system_bat_info(POWER_SUPPLY_PROP_DC5V_VOLTAGE);
}

int power_manger_set_dc5v_pulldown(void)
{	
	return set_system_bat_info(POWER_SUPPLY_SET_DC5V_PULLDOWM, NULL);
}

int power_manger_set_init_charge_consume_ma(int consume)
{	
    union power_supply_propval val;
    val.intval = consume;
	SYS_LOG_INF("setting init charge consume ma %d\n", consume);
	return set_system_bat_info(POWER_SUPPLY_SET_CONSUME_MA, &val);
}

int power_manger_wake_charger_box(void)
{
	int ret;
	union power_supply_propval val;

	ret = set_system_bat_info(POWER_SUPPLY_WAKE_CHARGER_BOX, &val);
	if (ret < 0)
	{
		return ret;
	}
	return val.intval;
}

int power_manager_set_slave_battery_state(int capacity, int vol)
{
	power_manager->slave_vol = vol;
	power_manager->slave_cap = capacity;
	power_manager->battary_changed = 1;
	SYS_LOG_INF("vol %dmv cap %d\n", vol, capacity);
	return 0;
}

bool power_manager_check_is_no_power(void)
{
	if (power_manager_get_dc5v_status())
		return false;

	if (power_manager_get_battery_vol() <= power_manager->nopower_level) {
		SYS_LOG_INF("%d %d too low", power_manager_get_battery_vol(), power_manager_get_battery_capacity_local());
		return true;
	}

	return false;
}

bool power_manager_check_bat_is_lowpower(void)
{
	int state = get_system_bat_info(POWER_SUPPLY_PROP_STATUS);
	if(state == POWER_SUPPLY_STATUS_BAT_LOWPOWER)
		return true;
	return false;
}

bool power_manager_check_bat_is_full(void)
{
	int state = get_system_bat_info(POWER_SUPPLY_PROP_STATUS);
	if(state == POWER_SUPPLY_STATUS_FULL)
		return true;
	return false;
}

bool power_manager_check_bat_exit(void)
{
	int state = get_system_bat_info(POWER_SUPPLY_PROP_STATUS);
	if(state == POWER_SUPPLY_STATUS_BAT_NOTEXIST)
		return false;
	return true;
}


int power_manager_set_before_poweroff(void)
{
	if (!power_manager || !power_manager->dev) {
		SYS_LOG_ERR("dev not found\n");
		return -ENODEV;
	}

	power_supply_set_property(power_manager->dev, POWER_SUPPLY_SET_BEFORE_ENTER_S4, NULL);

	return 0;
}


int power_manager_sync_slave_battery_state(void)
{
	uint32_t send_value;

#ifdef CONFIG_BT_TWS_US281B
	send_value = power_manager->current_cap / 10;
#else
	send_value = (power_manager->current_cap << 24) | power_manager->current_vol;
#endif

#ifdef CONFIG_TWS
	bt_manager_tws_send_event(TWS_BATTERY_EVENT, send_value);
#endif

	return 0;
}

static int _power_manager_work_handle(void)
{
	if (!power_manager)
		return -ESRCH;

	if (power_manager->battary_changed) {
		power_manager->battary_changed = 0;
#ifdef CONFIG_BT_HFP_HF
	#ifdef CONFIG_TWS
		if (bt_manager_tws_get_dev_role() == BTSRV_TWS_SLAVE) {
			power_manager_sync_slave_battery_state();
		} else if (bt_manager_tws_get_dev_role() == BTSRV_TWS_MASTER) {
			int report_cap = power_manager->current_cap;

			if (report_cap < power_manager->slave_cap)
				report_cap = power_manager->slave_cap;
		
			bt_manager_hfp_battery_report(BT_BATTERY_REPORT_VAL, report_cap);
		} else {
			bt_manager_hfp_battery_report(BT_BATTERY_REPORT_VAL, power_manager->current_cap);
		}
	#else
		bt_manager_hfp_battery_report(BT_BATTERY_REPORT_VAL, power_manager->current_cap);
	#endif
#endif
	}
	
	if (power_manager_get_dc5v_status()) {
		if(power_manager->wakelock_flag == 0) {
			#ifdef CONFIG_SYS_WAKELOCK
			sys_wake_lock_ext(PARTIAL_WAKE_LOCK,POWER_WAKE_LOCK_USER);
			#endif		

			power_manager->wakelock_flag = 1;
			SYS_LOG_INF("dc5v in, wake lock!");
		}
		
		return 0;
	} else {
		if(power_manager->wakelock_flag != 0) {
			#ifdef CONFIG_SYS_WAKELOCK
			sys_wake_unlock_ext(PARTIAL_WAKE_LOCK,POWER_WAKE_LOCK_USER);
			#endif		

			power_manager->wakelock_flag = 0;
			SYS_LOG_INF("dc5v out, wake unlock!");
		}		
	}
	
	if (power_manager->charge_full_flag) {
		power_manager->charge_full_flag = 0;
		sys_event_notify(SYS_EVENT_CHARGE_FULL);
	}

	if ((power_manager->current_vol <= power_manager->lowpower_level)
		&& ((os_uptime_get_32() - power_manager->report_timestamp) >=  DEFAULT_REPORT_PERIODS
			|| !power_manager->report_timestamp)) {
		SYS_LOG_INF("%d %d low", power_manager->current_vol, power_manager->lowpower_level);
	#ifdef CONFIG_TWS
		if (bt_manager_tws_get_dev_role() != BTSRV_TWS_SLAVE) {
			sys_event_notify(SYS_EVENT_BATTERY_LOW);
		}
	#else
		sys_event_notify(SYS_EVENT_BATTERY_LOW);
	#endif
		power_manager->report_timestamp = os_uptime_get_32();
	}

	if (power_manager->current_vol <= power_manager->nopower_level) {
		SYS_LOG_INF("%d %d too low", power_manager->current_vol, power_manager->nopower_level);
		sys_event_notify(SYS_EVENT_BATTERY_TOO_LOW);
		return 0;
	}

	return 0;
}


int power_manager_save_last_voltage(bool is_clear)
{
	char temp[6];
	uint16_t bat_vol_saved = power_manager_get_battery_vol()/1000;

	if(is_clear) {
		bat_vol_saved = 0;
	}

	snprintf(temp, sizeof(temp), "%d", bat_vol_saved);

	SYS_LOG_INF("last vol_s: %s", temp);

#ifdef CONFIG_PROPERTY
	property_set(CFG_BAT_LAST_VOLTAGE, temp, strlen(temp) + 1);
	property_flush(CFG_BAT_LAST_VOLTAGE);
	return 0;
#else
	return 0;
#endif
}


static int power_supply_set_init_vol(void)
{
	//uint32_t bat_vol_saved_flag = 0;

	if (!power_manager || !power_manager->dev) {
		SYS_LOG_ERR("dev not found\n");
		return -ENODEV;
	}

	SYS_LOG_INF("%s: %d", __func__, __LINE__);

#if 0
	soc_pstore_get(SOC_PSTORE_TAG_FLAG_CAP, &bat_vol_saved_flag);
	if(bat_vol_saved_flag != 0) {
		uint32_t bat_vol_saved = 0;

		union power_supply_propval val;

		soc_pstore_get(SOC_PSTORE_TAG_CAPACITY, &bat_vol_saved);
		val.intval = bat_vol_saved;
		power_supply_set_property(power_manager->dev, POWER_SUPPLY_SET_INIT_VOL, &val);

		SYS_LOG_INF("bat init vol from pstore: %dmv\n", bat_vol_saved);

		soc_pstore_set(SOC_PSTORE_TAG_FLAG_CAP, 0);
	}
#else
#ifdef CONFIG_PROPERTY
	{
		char temp[8];
		uint16_t bat_vol_saved = 0;
		union power_supply_propval val;
		
		memset(temp, 0, sizeof(temp));
		
		int ret = property_get(CFG_BAT_LAST_VOLTAGE, temp, sizeof(temp));
		
		if (ret >= 0) {
			bat_vol_saved = atoi(temp);
			SYS_LOG_INF("%s, bat_vol: %dmv\n", __func__, bat_vol_saved);
		}
			
		if ((bat_vol_saved > 1000) && (bat_vol_saved < 5000)) {
			val.intval = bat_vol_saved;
			power_supply_set_property(power_manager->dev, POWER_SUPPLY_SET_INIT_VOL, &val);
	
			SYS_LOG_INF("bat init vol from nvram: %dmv\n", bat_vol_saved);			
	
			power_manager_save_last_voltage(1);
		}
			
	}
#endif

#endif

	return 0;
}


static struct power_manager_info global_power_manager;

int power_manager_init(void)
{
	power_manager = &global_power_manager;

	memset(power_manager, 0, sizeof(struct power_manager_info));

	power_manager->dev = (struct device *)device_get_binding(CONFIG_ACTS_BATTERY_DEV_NAME);
	if (!power_manager->dev) {
		SYS_LOG_ERR("dev not found\n");
		return -ENODEV;
	}

#if 0
	if ((power_manager_get_battery_vol() <= DEFAULT_BOOTPOWER_LEVEL)
			&& (!power_manager_get_dc5v_status())) {
		SYS_LOG_INF("no power ,shundown: %d\n", power_manager_get_battery_vol());
		sys_pm_poweroff();
		return 0;
	}
#endif

	power_supply_register_notify(power_manager->dev, power_supply_report);

	power_supply_set_init_vol();
	
	power_supply_enable(power_manager->dev);
	
#ifdef CONFIG_PROPERTY
	power_manager->lowpower_level = property_get_int(CFG_LOW_POWER_WARNING_LEVEL, DEFAULT_LOWPOWER_LEVEL);
	power_manager->nopower_level = property_get_int(CFG_SHUTDOWN_POWER_LEVEL, DEFAULT_NOPOWER_LEVEL);
#else
	power_manager->lowpower_level = DEFAULT_LOWPOWER_LEVEL;
	power_manager->nopower_level = DEFAULT_NOPOWER_LEVEL;
#endif
	power_manager->current_vol = power_manager_get_battery_vol();
	power_manager->current_cap = power_manager_get_battery_capacity_local();
	power_manager->slave_vol = 4200000;
	power_manager->slave_cap = 100;
	power_manager->report_timestamp = 0;

	sys_monitor_add_work(_power_manager_work_handle);

	return 0;
}
