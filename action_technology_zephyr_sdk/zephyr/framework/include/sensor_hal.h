/*******************************************************************************
 * @file    sensor_hal.h
 * @author  MEMS Application Team
 * @version V1.0
 * @date    2020-08-12
 * @brief   sensor testing module
*******************************************************************************/

#ifndef _SENSOR_HAL_H
#define _SENSOR_HAL_H

/******************************************************************************/
//includes
/******************************************************************************/
#include <stdint.h>

/******************************************************************************/
//constants
/******************************************************************************/
typedef enum {
	ID_ACC = 0,
	ID_GYRO,
	ID_MAG,
	ID_BARO,
	ID_TEMP,
	ID_HR,
	ID_GNSS,
	ID_OFFBODY,
	NUM_SENSOR,
} sensor_id_e;

typedef enum {
	EVT_NULL = 0,
	EVT_TASK,
	EVT_IRQ,
} sensor_evt_e;

/******************************************************************************/
//typedefs
/******************************************************************************/
typedef struct sensor_evt_s {
	uint8_t id;			// sensor id
	uint8_t evt;		// event type
	uint16_t pd;   	// data period (ms)
	uint16_t sz;   	// data size
	uint16_t cnt;  	// data count
	uint8_t *buf;   // data buffer
	uint64_t ts;  	// time stamp (ms)
} sensor_dat_t;

typedef void (*sensor_cb_t) (int id, sensor_dat_t *dat, void *ctx);

/******************************************************************************/
//functions
/******************************************************************************/
int sensor_hal_init(void);
int sensor_hal_deinit(void);
int sensor_hal_dump(void);
int sensor_hal_suspend(void);
int sensor_hal_resume(void);

const char *sensor_hal_get_type(int id);
const char *sensor_hal_get_name(int id);

void sensor_hal_add_callback(int id, sensor_cb_t cb, void *ctx);
void sensor_hal_remove_callback(int id);

int sensor_hal_enable(int id);
int sensor_hal_disable(int id);

int sensor_hal_read(int id, uint16_t reg, uint8_t *buf, uint16_t len);
int sensor_hal_write(int id, uint16_t reg, uint8_t *buf, uint16_t len);

void sensor_hal_init_data(int id, sensor_dat_t *dat, uint8_t *buf, int len);
int sensor_hal_poll_data(int id, sensor_dat_t *dat, uint8_t *buf);
int sensor_hal_get_value(int id, sensor_dat_t *dat, uint16_t idx, float *val);
int sensor_hal_cvt_data(int id, uint8_t *buf, uint16_t len, float *val, int16_t *raw);
int sensor_hal_get_data_period(int id);

void sensor_hal_config_tm(int id, int ms);
uint32_t sensor_hal_get_tm(int id);
uint32_t sensor_hal_get_30K_counter(int id);
void sensor_hal_clear_tm_pending(int id);

#endif  /* _SENSOR_HAL_H */

