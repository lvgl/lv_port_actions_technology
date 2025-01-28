/*******************************************************************************
 * @file    sensor_algo_common.h
 * @author  MEMS Application Team
 * @version V1.0
 * @date    2023-12-29
 * @brief   sensor algorithm common api
*******************************************************************************/

#ifndef _SENSOR_ALGO_COMMON_H
#define _SENSOR_ALGO_COMMON_H

/******************************************************************************/
//includes
/******************************************************************************/
#include <stdint.h>
#include <stdarg.h>

/******************************************************************************/
//constants
/******************************************************************************/

/******************************************************************************/
//typedef
/******************************************************************************/

/******************************************************************************/
//function
/******************************************************************************/

/* sensor timestamp(ms) */
uint64_t sensor_get_timestamp_ms(void);

/* sensor timestamp(us) */
uint64_t sensor_get_timestamp_us(void);

/* sensor timestamp(ns) */
uint64_t sensor_get_timestamp_ns(void);

/* sensor udelay */
void sensor_udelay(uint32_t us);

/* sensor mdelay */
void sensor_mdelay(uint32_t ms);

/* sensor heap malloc */
void *sensor_malloc(size_t size);

/* sensor heap free */
void sensor_free(void *ptr);

#endif  /* _SENSOR_ALGO_COMMON_H */

