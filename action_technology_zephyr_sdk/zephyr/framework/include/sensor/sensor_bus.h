/*******************************************************************************
 * @file    sensor_bus.h
 * @author  MEMS Application Team
 * @version V1.0
 * @date    2020-08-12
 * @brief   sensor testing module
*******************************************************************************/

#ifndef _SENSOR_BUS_H
#define _SENSOR_BUS_H

/******************************************************************************/
//includes
/******************************************************************************/
#include <sensor_dev.h>

/******************************************************************************/
//constants
/******************************************************************************/
typedef enum {
	BUS_I2C = 0,
	BUS_SPI,
	NUM_BUS,
} bus_type_e;

/******************************************************************************/
//typedefs
/******************************************************************************/
typedef void (*sensor_bus_cb_t) (uint8_t *buf, int len, void *ctx);

/******************************************************************************/
//functions
/******************************************************************************/
int sensor_bus_read(const sensor_dev_t *dev, uint16_t reg, uint8_t *buf, uint16_t len);
int sensor_bus_write(const sensor_dev_t *dev, uint16_t reg, uint8_t *buf, uint16_t len);

int sensor_bus_task_start(const sensor_dev_t *dev, sensor_bus_cb_t cb, void *ctx);
int sensor_bus_task_stop(const sensor_dev_t *dev);

#endif  /* _SENSOR_DEV_H */

