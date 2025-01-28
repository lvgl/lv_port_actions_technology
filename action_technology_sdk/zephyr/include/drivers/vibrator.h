/*
 * Copyright (c) 2020 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public API for Vibration motor Drivers
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_VIBRATOR_H_
#define ZEPHYR_INCLUDE_DRIVERS_VIBRATOR_H_
#include <zephyr/types.h>
#include <device.h>
#include <sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif

struct vibrat_driver_api {
	int (*start)(const struct device *dev, u32_t chan, u8_t dutycycle);
	int (*stop)(const struct device *dev, u32_t chan);
	int (*set_freq_param)(const struct device *dev, u32_t chan, u32_t freq);
};

/**
 * @brief Start vibration
 *
 * @param dev: Pointer to the device structure for the driver instance.
 * @param chan: the channel of vibrator corresponding to pwm chan.
 * @param dutycycle: Dutycycle of vibration strength from 0~100.
 *  @return  0 on success, negative errno code on fail.
 */
static inline int vibrat_start(const struct device *dev, u32_t chan, u8_t dutycycle)
{
	const struct vibrat_driver_api *api = (const struct vibrat_driver_api *) dev->api;

	return api->start(dev, chan, dutycycle);
}

/**
 * @brief Stop vibration
 *
 * @param dev: Pointer to the device structure for the driver instance.
 * @param chan: the channel of vibrator corresponding to pwm chan.
 *  @return  0 on success, negative errno code on fail.
 */
static inline int vibrat_stop(const struct device *dev, u32_t chan)
{
	const struct vibrat_driver_api *api = (const struct vibrat_driver_api *) dev->api;

	return api->stop(dev, chan);
}

/**
 * @brief Set input frequency parameter of vibrator
 *
 * @param dev: Pointer to the device structure for the driver instance.
 * @param chan: the channel of vibrator corresponding to pwm chan.
 * @param freq: the input pwm frequency.
 *  @return  0 on success, negative errno code on fail.
 */
static inline int vibrat_set_freq_param(const struct device *dev, u32_t chan, u32_t freq)
{
	const struct vibrat_driver_api *api = (const struct vibrat_driver_api *) dev->api;

	return api->set_freq_param(dev, chan, freq);
}

#ifdef __cplusplus
}
#endif

#endif
