/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#include "gx_battery.h"
#include <power_manager.h>
#include <drivers/power_supply.h>

namespace gx {

BatteryInfo get_charge_state() {
	int status = POWER_SUPPLY_STATUS_BAT_NOTEXIST;
	if (status == POWER_SUPPLY_STATUS_BAT_NOTEXIST) {
		return BatteryStatusNotCharging;
	} else if (status == POWER_SUPPLY_STATUS_DISCHARGE) {
		return BatteryStatusDischarging;
	} else if (status == POWER_SUPPLY_STATUS_CHARGING) {
		return BatteryStatusCharging;
	} else if (status == POWER_SUPPLY_STATUS_FULL) {
		return BatteryStatusFull;
	}

	return BatteryStatusNotCharging;
}

int get_battery_level() {
	return power_manager_get_battery_capacity();
}

} // namespace gx
