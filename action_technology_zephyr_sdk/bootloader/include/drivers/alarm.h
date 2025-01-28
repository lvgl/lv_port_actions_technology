/*
 * Copyright (c) 2021 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public API for ALARM Drivers
 */

#ifndef _ALARM_H_
#define _ALARM_H_

#ifdef __cplusplus
extern "C" {
#endif

/*!
 * struct alarm_config
 * @brief The structure to configure the alarm function.
 */
struct alarm_config {
	uint32_t alarm_msec;                /*!< The alarm time in milliseconds setting */
	void (*cb_fn)(const void *cb_data);	/*!< Pointer to function to call when alarm value matches current RTC value */
	const void *cb_data;                /*!< The callback data */
};

/*!
 * struct alarm_status
 * @brief The current alarm status
 */
struct alarm_status {
	uint32_t alarm_msec;                /*!< The alarm time in milliseconds setting */
	bool is_on;                         /*!< Alarm on status */
};

struct alarm_driver_api {
	int (*set_alarm)(const struct device *dev, struct alarm_config *config, bool enable);
	int (*get_alarm)(const struct device *dev, struct alarm_status *sts);
	bool (*is_alarm_wakeup)(const struct device *dev);
};

/**
 * @brief Set the alarm time
 *
 * @param dev: Pointer to the device structure for the driver instance.
 * @param config: Pointer to alarm configuration.
 * @param enable: enable or disable alarm function.
 *
 *  @return  0 on success, negative errno code on fail.
 */
static inline int acts_alarm_set_alarm(const struct device *dev, struct alarm_config *config, bool enable)
{
	const struct alarm_driver_api *api = dev->api;

	return api->set_alarm(dev, config, enable);
}

/**
 * @brief Get the alarm time setting
 *
 * @param dev: Pointer to the device structure for the driver instance.
 * @param sts: Pointer to alarm status structure
 *
 *  @return  0 on success, negative errno code on fail.
 */
static inline int acts_alarm_get_alarm(const struct device *dev, struct alarm_status *sts)
{
	const struct alarm_driver_api *api = dev->api;

	return api->get_alarm(dev, sts);
}

/**
 * @brief Function to get the information that whether wakeup from RTC
 *
 * Moreover, user can distinguish the ALARM wake up event from this API.
 *
 * @param dev Pointer to the device structure for the driver instance.
 *
 * @retval 1 if the rtc interrupt is pending.
 * @retval 0 if no rtc interrupt is pending.
 */
static inline bool acts_is_alarm_wakeup(const struct device *dev)
{
	struct alarm_driver_api *api = (struct alarm_driver_api *)dev->api;
	return api->is_alarm_wakeup(dev);
}

#ifdef __cplusplus
}
#endif

#endif
