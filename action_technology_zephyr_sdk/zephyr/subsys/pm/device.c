/*
 * Copyright (c) 2018 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <kernel.h>
#include <string.h>
#include <device.h>
#include <pm/policy.h>

#define LOG_LEVEL CONFIG_PM_LOG_LEVEL /* From power module Kconfig */
#include <logging/log.h>
LOG_MODULE_DECLARE(power);

#if defined(CONFIG_PM_DEVICE)
extern const struct device *__pm_device_slots_start[];

#define EARLY_DEV_PM_NUM 	30
static const struct device *g_pm_device_early[EARLY_DEV_PM_NUM];

/* Number of devices successfully suspended. */
static size_t num_susp, early_num_susp;

//#define CONFIG_DEV_DEBUG

#ifdef CONFIG_DEV_DEBUG
static uint32_t timer_start_cyc;
static inline void dev_debug_start_timer(void)
{
	timer_start_cyc = k_cycle_get_32();
}

static  void dev_debug_stop_timer(const char *dev_name,const char *msg)
{
	uint32_t res = k_cycle_get_32() - timer_start_cyc;
	printk("%s: %s use %d us\n",msg, dev_name, k_cyc_to_us_ceil32(res));
}


#else
static inline void dev_debug_start_timer(void) { }
static  void dev_debug_stop_timer(const char *dev_name,const char *msg) 
{ 
}
#endif


static int _pm_devices(enum pm_device_state state)
{
	const struct device *devs;
	size_t devc;

	devc = z_device_get_all_static(&devs);

	num_susp = 0;

	for (const struct device *dev = devs + devc - 1; dev >= devs; dev--) {
		int ret;

		/* ignore busy devices */
		if( PM_DEVICE_STATE_OFF != state) { // power off not check dev busy
			if (pm_device_is_busy(dev) || pm_device_wakeup_is_enabled(dev)) {
				continue;
			}
		}
		dev_debug_start_timer();
		ret = pm_device_state_set(dev, state);
		dev_debug_stop_timer(dev->name,"--sus--");
		/* ignore devices not supporting or already at the given state */
		if ((ret == -ENOSYS) || (ret == -ENOTSUP) || (ret == -EALREADY)) {
			continue;
		} else if (ret < 0) {
			LOG_ERR("Device %s did not enter %s state (%d)",
				dev->name, pm_device_state_str(state), ret);
			return ret;
		}

		__pm_device_slots_start[num_susp] = dev;
		num_susp++;
	}

	return 0;
}

int pm_suspend_devices(void)
{
	return _pm_devices(PM_DEVICE_STATE_SUSPENDED);
}

int pm_low_power_devices(void)
{
	return _pm_devices(PM_DEVICE_STATE_LOW_POWER);
}


void pm_resume_devices(void)
{
	int32_t i;

	for (i = (num_susp - 1); i >= 0; i--) {
		dev_debug_start_timer();
		pm_device_state_set(__pm_device_slots_start[i],
				    PM_DEVICE_STATE_ACTIVE);
		dev_debug_stop_timer(__pm_device_slots_start[i]->name,"--res--");
	}

	num_susp = 0;
}
#endif /* defined(CONFIG_PM_DEVICE) */

const char *pm_device_state_str(enum pm_device_state state)
{
	switch (state) {
	case PM_DEVICE_STATE_ACTIVE:
		return "active";
	case PM_DEVICE_STATE_LOW_POWER:
		return "low power";
	case PM_DEVICE_STATE_SUSPENDED:
		return "suspended";
	case PM_DEVICE_STATE_OFF:
		return "off";
	default:
		return "";
	}
}

int pm_device_state_set(const struct device *dev,
			enum pm_device_state state)
{
	int ret;
	enum pm_device_action action;

	if (dev->pm_control == NULL) {
		return -ENOSYS;
	}

	if (atomic_test_bit(&dev->pm->flags, PM_DEVICE_FLAG_TRANSITIONING)) {
		return -EBUSY;
	}

	switch (state) {
	case PM_DEVICE_STATE_SUSPENDED:
		if (dev->pm->state == PM_DEVICE_STATE_SUSPENDED) {
			return -EALREADY;
		} else if (dev->pm->state == PM_DEVICE_STATE_OFF) {
			return -ENOTSUP;
		}

		action = PM_DEVICE_ACTION_SUSPEND;
		break;
	case PM_DEVICE_STATE_ACTIVE:
		if (dev->pm->state == PM_DEVICE_STATE_ACTIVE) {
			return -EALREADY;
		}

		action = PM_DEVICE_ACTION_RESUME;
		break;
	case PM_DEVICE_STATE_LOW_POWER:
		if (dev->pm->state == state) {
			return -EALREADY;
		}

		action = PM_DEVICE_ACTION_LOW_POWER;
		break;
	case PM_DEVICE_STATE_OFF:
		if (dev->pm->state == state) {
			return -EALREADY;
		}

		action = PM_DEVICE_ACTION_TURN_OFF;
		break;
	default:
		return -ENOTSUP;
	}

	ret = dev->pm_control(dev, action);
	if (ret < 0) {
		return ret;
	}

	dev->pm->state = state;

	return 0;
}

int pm_device_state_get(const struct device *dev,
			enum pm_device_state *state)
{
	if (dev->pm_control == NULL) {
		return -ENOSYS;
	}

	*state = dev->pm->state;

	return 0;
}

bool pm_device_is_any_busy(void)
{
	const struct device *devs;
	size_t devc;

	devc = z_device_get_all_static(&devs);

	for (const struct device *dev = devs; dev < (devs + devc); dev++) {
		if (atomic_test_bit(&dev->pm->flags, PM_DEVICE_FLAG_BUSY)) {
			return true;
		}
	}

	return false;
}

bool pm_device_is_busy(const struct device *dev)
{
	return atomic_test_bit(&dev->pm->flags, PM_DEVICE_FLAG_BUSY);
}

void pm_device_busy_set(const struct device *dev)
{
	atomic_set_bit(&dev->pm->flags, PM_DEVICE_FLAG_BUSY);
}

void pm_device_busy_clear(const struct device *dev)
{
	atomic_clear_bit(&dev->pm->flags, PM_DEVICE_FLAG_BUSY);
}

bool pm_device_wakeup_enable(struct device *dev, bool enable)
{
	atomic_val_t flags, new_flags;

	flags =	 atomic_get(&dev->pm->flags);

	if ((flags & BIT(PM_DEVICE_FLAGS_WS_CAPABLE)) == 0U) {
		return false;
	}

	if (enable) {
		new_flags = flags |
			BIT(PM_DEVICE_FLAGS_WS_ENABLED);
	} else {
		new_flags = flags & ~BIT(PM_DEVICE_FLAGS_WS_ENABLED);
	}

	return atomic_cas(&dev->pm->flags, flags, new_flags);
}

bool pm_device_wakeup_is_enabled(const struct device *dev)
{
	return atomic_test_bit(&dev->pm->flags,
			       PM_DEVICE_FLAGS_WS_ENABLED);
}

bool pm_device_wakeup_is_capable(const struct device *dev)
{
	return atomic_test_bit(&dev->pm->flags,
			       PM_DEVICE_FLAGS_WS_CAPABLE);
}


int pm_early_suspend(void)
{
	const struct device *devs;
	size_t devc;

	devc = z_device_get_all_static(&devs);

	early_num_susp = 0;

	for (const struct device *dev = devs + devc - 1; dev >= devs; dev--) {
		int ret;

		if (dev->pm_control == NULL) {
			continue;
		}
		dev_debug_start_timer();
		ret = dev->pm_control(dev, PM_DEVICE_ACTION_EARLY_SUSPEND);
		dev_debug_stop_timer(dev->name,"early");
		/* ignore devices not supporting or already at the given state */
		if ((ret == -ENOSYS) || (ret == -ENOTSUP) || (ret == -EALREADY)) {
			continue;
		} else if (ret < 0) {
			LOG_ERR("Device %s did not enter early state (%d)",
				dev->name, ret);
			return ret;
		}
		if(early_num_susp < EARLY_DEV_PM_NUM){
			g_pm_device_early[early_num_susp] = dev;
			early_num_susp++;
		}else{
			LOG_ERR("early suspend: number of dev exceeds the upper limit\n");
			while(1);
		}
	}

	return 0;
}
void pm_late_resume(void)
{
	int32_t i;

	for (i = (early_num_susp - 1); i >= 0; i--) {
		dev_debug_start_timer();
		g_pm_device_early[i]->pm_control(g_pm_device_early[i],
				    PM_DEVICE_ACTION_LATE_RESUME);
		dev_debug_stop_timer(g_pm_device_early[i]->name,"late");
	}

	early_num_susp = 0;
}

int pm_power_off_devices(void)
{
	return _pm_devices(PM_DEVICE_STATE_OFF);
}



