/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file sensor manager interface
 */

#ifndef __SENSOR_MANGER_H__
#define __SENSOR_MANGER_H__

#include <sensor_algo.h>

/**
 * @defgroup sensor_manager_apis app Sensor Manager APIs
 * @ingroup system_apis
 * @{
 */

enum SENSOR_MSG_ID {
	MSG_SENSOR_DATA,
	MSG_SENSOR_GET_RESULT,
	MSG_SENSOR_ADD_CB,
	MSG_SENSOR_REMOVE_CB,
	MSG_SENSOR_ENABLE,
	MSG_SENSOR_DISABLE,
};

typedef int (*sensor_res_cb_t)(int evt_id, sensor_res_t *res);

int sensor_send_msg(uint32_t cmd, uint32_t len, void *ptr, uint8_t notify);

/* callback for sensor result */
int sensor_manager_add_callback(sensor_res_cb_t cb);
int sensor_manager_remove_callback(sensor_res_cb_t cb);

/* sensor manager enable sensor */
int sensor_manager_enable(uint32_t id, uint32_t func);

/* sensor manager disable sensor */
int sensor_manager_disable(uint32_t id, uint32_t func);

/* sensor manager get algo result funcion */
int sensor_manager_get_result(sensor_res_t *res);

/**
 * @brief sensor manager init funcion
 *
 * This routine calls init sensor manager ,called by main
 *
 * @return 0 if invoked succsess.
 * @return others if invoked failed.
 */
int sensor_manager_init(void);

/**
 * @brief sensor manager deinit funcion
 *
 * This routine calls deinit sensor manager ,called by main
 *
 * @return 0 if invoked succsess.
 * @return others if invoked failed.
 */
int sensor_manager_exit(void);

/**
 * @} end defgroup system_apis
 */
#endif
