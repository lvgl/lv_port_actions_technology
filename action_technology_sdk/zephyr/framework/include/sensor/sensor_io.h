/*******************************************************************************
 * @file    sensor_io.h
 * @author  MEMS Application Team
 * @version V1.0
 * @date    2020-08-12
 * @brief   sensor testing module
*******************************************************************************/

#ifndef _SENSOR_IO_H
#define _SENSOR_IO_H

/******************************************************************************/
//includes
/******************************************************************************/
#include <sensor_dev.h>

/******************************************************************************/
//constants
/******************************************************************************/

/******************************************************************************/
//typedefs
/******************************************************************************/
typedef void (*sensor_io_cb_t) (int pin, int id);

/******************************************************************************/
//functions
/******************************************************************************/
void sensor_io_init(const sensor_dev_t *dev);
void sensor_io_deinit(const sensor_dev_t *dev);

void sensor_io_enable_irq(const sensor_dev_t *dev, sensor_io_cb_t cb, int id);
void sensor_io_disable_irq(const sensor_dev_t *dev, sensor_io_cb_t cb, int id);

void sensor_io_enable_trig(const sensor_dev_t *dev, int id);
void sensor_io_disable_trig(const sensor_dev_t *dev, int id);

void sensor_io_config_tm(const sensor_dev_t *dev, int ms);
uint32_t sensor_io_get_tm(const sensor_dev_t *dev);
void sensor_io_clear_tm_pending(int tid);

#endif  /* _SENSOR_DEV_H */

