/*
 * Copyright (c) 2017 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file power supply device driver interface
 */

#ifndef __INCLUDE_POWER_SUPPLY_H__
#define __INCLUDE_POWER_SUPPLY_H__

#include <stdint.h>
#include <device.h>

#include <board_cfg.h>


#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup power_supply_apis Power Supply APIs
 * @ingroup driver_apis
 * @{
 */


/** Input Device Names */
//#define CONFIG_ACTS_BATTERY_DEV_NAME         DT_LABEL(DT_INST(0, actions_acts_batadc))

#define BATTERY_CAPCTR_STA_PASSED            (1)
#define BATTERY_CAPCTR_STA_FAILED            (0)
#define BATTERY_CAPCTR_DISABLE               (0)
#define BATTERY_CAPCTR_ENABLE                (1)

/** power supply status enum */
enum  power_supply_status{
	/** power supply unknown */
	POWER_SUPPLY_STATUS_UNKNOWN = 0,
	/** battery not exist */
	POWER_SUPPLY_STATUS_BAT_NOTEXIST,
	/** battery discharge */
	POWER_SUPPLY_STATUS_DISCHARGE,
	/** battery in charge */
	POWER_SUPPLY_STATUS_CHARGING,
	/** battery voltage full */
	POWER_SUPPLY_STATUS_FULL,
	/** dc5v plug out */
	POWER_SUPPLY_STATUS_DC5V_OUT,
	/** dc5v plug in */
	POWER_SUPPLY_STATUS_DC5V_IN,
	/** dc5v pending */
	POWER_SUPPLY_STATUS_DC5V_PENDING,
	/** dc5v standby */
	POWER_SUPPLY_STATUS_DC5V_STANDBY
};

/** power supply property enum */
enum power_supply_property {
	/** get power supply status */
	POWER_SUPPLY_PROP_STATUS = 0,
	/** no use */
	POWER_SUPPLY_PROP_ONLINE,
	/** get current battery voltage */
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	/** get current battery capacity */
	POWER_SUPPLY_PROP_CAPACITY,
	/** get current dc5v plug in or out status */
	POWER_SUPPLY_PROP_DC5V,
	/** get dc5v status for charger box */
	POWER_SUPPLY_PROP_DC5V_STATUS,
	/** get current dc5v voltage */
	POWER_SUPPLY_PROP_DC5V_VOLTAGE,

	/** set power supply feature */
	POWER_SUPPLY_SET_PROP_FEATURE,
	/** set dc5v pulldown, used for charger box */
	POWER_SUPPLY_SET_DC5V_PULLDOWM,
	/** wakeup charger box */
	POWER_SUPPLY_WAKE_CHARGER_BOX,
};


/** battery charge event enum */
typedef enum
{
	/** dc5v plug in event */
	BAT_CHG_EVENT_DC5V_IN = 1,
	/** dc5v plug out event */
	BAT_CHG_EVENT_DC5V_OUT,
	/** charger box enter standby */
	BAT_CHG_EVENT_DC5V_STANDBY,

	/** start charging */
	BAT_CHG_EVENT_CHARGE_START,
	/** stop charging */
	BAT_CHG_EVENT_CHARGE_STOP,
	/** charge full event */
	BAT_CHG_EVENT_CHARGE_FULL,

	/** battery voltage changed */
	BAT_CHG_EVENT_VOLTAGE_CHANGE,
	/** battery capacity changed */
	BAT_CHG_EVENT_CAP_CHANGE,		

	/** battery power low */
	BAT_CHG_EVENT_BATTERY_LOW,
	/** battery power lower */
	BAT_CHG_EVENT_BATTERY_LOW_EX,
	/** battery power too low */
	BAT_CHG_EVENT_BATTERY_TOO_LOW,
	/** battery power full */
	BAT_CHG_EVENT_BATTERY_FULL,		
} bat_charge_event_t;

/** battery event param */
typedef union {
		uint32_t voltage_val;    /**< Battery voltage value*/  
		uint32_t cap;            /**< Battery capacity percent*/
} bat_charge_event_para_t;


typedef void (*bat_charge_callback_t)(bat_charge_event_t event, bat_charge_event_para_t *para);


/** power supply property value */
union power_supply_propval {
	/** property value */
	int intval;
	/** property string */
	const char *strval;
};

struct battery_capctr_info {
	uint8_t capctr_enable_flag;
	uint8_t capctr_minval;
	uint8_t capctr_maxval;
};

/** power supply driver api */
struct power_supply_driver_api {
	/**< get property from power supply driver */
	int (*get_property)(struct device *dev, enum power_supply_property psp,
				union power_supply_propval *val);
	/**< set property to power supply driver */
	void (*set_property)(struct device *dev, enum power_supply_property psp,
				union power_supply_propval *val);
	/**< register notify callback to power supply driver */
	void (*register_notify)(struct device *dev, bat_charge_callback_t cb);
	/**< enable battery charge */
	void (*enable)(struct device *dev);
	/**< disable battery charge */
	void (*disable)(struct device *dev);
};


/**
 * @brief get property from power supply driver
 *
 * This routine calls to get information from power supply driver
 *
 * Example:
 *
 * @code
 *	dev = device_get_binding(CONFIG_ACTS_BATTERY_DEV_NAME);
 *	if (!dev) {
 *		SYS_LOG_ERR("cannot found battery device");
 *		return -ENODEV;
 *	} 
 *  psp = POWER_SUPPLY_PROP_CAPACITY;
 *	ret = power_supply_get_property(dev, psp, &val);
 *	if (ret < 0) {
 *		SYS_LOG_ERR("cannot get property, ret %d", ret);
 *		return -EINVAL;
 *	} 
 * @endcode
 *
 * @param dev pointer to the battery device.
 * @param psp the property need to get from driver
 * @param val pointer to the inf getting from driver
 *
 * @return 0 : succsess.  others: fail
 */

static inline int power_supply_get_property(struct device *dev, enum power_supply_property psp,
				union power_supply_propval *val)
{
	const struct power_supply_driver_api *api = dev->api;

	return api->get_property(dev, psp, val);
}


/**
 * @brief set property to power supply driver
 *
 * This routine calls to set property to power supply driver
 *
 * Example:
 *
 * @code
 *	dev = device_get_binding(CONFIG_ACTS_BATTERY_DEV_NAME);
 *	if (!dev) {
 *		SYS_LOG_ERR("cannot found battery device");
 *		return -ENODEV;
 *	} 
 *  psp = POWER_SUPPLY_SET_DC5V_PULLDOWM;
 *	power_supply_set_property(dev, psp, NULL);
 *
 * @endcode
 *
 * @param dev pointer to the battery device.
 * @param psp the property need to set
 * @param val pointer to the property value which need to set
 * 
 */

static inline void power_supply_set_property(struct device *dev, enum power_supply_property psp,
				union power_supply_propval *val)
{
	const struct power_supply_driver_api *api = dev->api;

	api->set_property(dev, psp, val);
}


/**
 * @brief register notify callback to power supply driver
 *
 * This routine calls to set callback to driver
 *
 * Example:
 *
 * @code
 *	dev = device_get_binding(CONFIG_ACTS_BATTERY_DEV_NAME);
 *	if (!dev) {
 *		SYS_LOG_ERR("cannot found battery device");
 *		return -ENODEV;
 *	} 
 *  
 *	power_supply_register_notify(dev, power_supply_report);
 *
 * @endcode
 *
 * @param dev pointer to the battery device.
 * @param cb  callback function.
 */

static inline void power_supply_register_notify(struct device *dev, bat_charge_callback_t cb)
{
	const struct power_supply_driver_api *api = dev->api;

	api->register_notify(dev, cb);
}


/**
 * @brief enable battery charge
 *
 * This routine calls to enable battery charge.
 *
 * Example:
 *
 * @code
 *	dev = device_get_binding(CONFIG_ACTS_BATTERY_DEV_NAME);
 *	if (!dev) {
 *		SYS_LOG_ERR("cannot found battery device");
 *		return -ENODEV;
 *	} 
 *  
 *	power_supply_enable(dev);
 *
 * @endcode
 *
 * @param dev pointer to the battery device.
 */

static inline void power_supply_enable(struct device *dev)
{
	const struct power_supply_driver_api *api = dev->api;

	api->enable(dev);
}


/**
 * @brief disable battery charge
 *
 * This routine calls to disable battery charge.
 *
 * Example:
 *
 * @code
 *	dev = device_get_binding(CONFIG_ACTS_BATTERY_DEV_NAME);
 *	if (!dev) {
 *		SYS_LOG_ERR("cannot found battery device");
 *		return -ENODEV;
 *	} 
 *  
 *	power_supply_disable(dev);
 *
 * @endcode
 *
 * @param dev pointer to the battery device.
 *
 */

static inline void power_supply_disable(struct device *dev)
{
	const struct power_supply_driver_api *api = dev->api;

	api->disable(dev);
}


#ifdef CONFIG_ACTS_BATTERY_SUPPLY_EXT_COULOMETER

/** extern coulometer driver api */
struct coulometer_driver_api {
	/**< get property from coulometer driver */
	int (*get_property)(const struct device *dev, enum power_supply_property psp,
				union power_supply_propval *val);
	/**< set property to coulometer driver */
	void (*set_property)(const struct device *dev, enum power_supply_property psp,
				union power_supply_propval *val);
	/**< enable coulometer */
	void (*enable)(const struct device *dev);
	/**< disable coulometer */
	void (*disable)(const struct device *dev);	
};


static inline int coulometer_get_property(const struct device *dev, enum power_supply_property psp,
				union power_supply_propval *val)
{
	const struct coulometer_driver_api *api = dev->api;

	return api->get_property(dev, psp, val);
}

static inline void coulometer_set_property(const struct device *dev, enum power_supply_property psp,
				union power_supply_propval *val)
{
	const struct coulometer_driver_api *api = dev->api;

	api->set_property(dev, psp, val);
}

static inline void coulometer_enable(const struct device *dev)
{
	const struct coulometer_driver_api *api = dev->api;

	api->enable(dev);
}

static inline void coulometer_disable(const struct device *dev)
{
	const struct coulometer_driver_api *api = dev->api;

	api->disable(dev);
}

#endif


/**
 * @} end defgroup power_supply_apis
 */

#ifdef __cplusplus
}
#endif

#endif  /* __INCLUDE_POWER_SUPPLY_H__ */
