/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_PPI_H_
#define ZEPHYR_INCLUDE_DRIVERS_PPI_H_

/**
 * @brief inter-processor message communication API.
 * @defgroup ipmsg_interface IPMSG Interface
 * @ingroup io_interfaces
 * @{
 */

#include <kernel.h>
#include <device.h>
#include <soc.h>

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************/
//constants
/******************************************************************************/

/******************************************************************************/
//typedefs
/******************************************************************************/
typedef struct task_trig {
	uint32_t en   : 1;    // Channel en
	uint32_t chan : 4;    // Channel id
	uint32_t task : 5;    // Task selection
	uint32_t trig : 6;    // Trigger src
	uint32_t peri : 16;   // Period (ms)
} task_trig_t;

/******************************************************************************/
//functions
/******************************************************************************/

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_PPI_H_ */
