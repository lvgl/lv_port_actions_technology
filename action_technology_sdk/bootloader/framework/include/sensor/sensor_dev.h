/*******************************************************************************
 * @file    sensor_hal.h
 * @author  MEMS Application Team
 * @version V1.0
 * @date    2020-08-12
 * @brief   sensor testing module
*******************************************************************************/

#ifndef _SENSOR_DEV_H
#define _SENSOR_DEV_H

/******************************************************************************/
//includes
/******************************************************************************/
#include <stdint.h>

/******************************************************************************/
//constants
/******************************************************************************/
#define REG_NULL					(0xffff)
#define REG_DELAY					(0xfffe)

typedef enum {
	CFG_INIT = 0, // init reg config
	CFG_ON,       // enable reg config
	CFG_OFF,      // disable reg config
	NUM_CFG,
} cfg_type_e;

typedef enum {
	FUNC_INIT = 0, // init func
	FUNC_ST,       // self test func
	FUNC_CVT,      // value covertion func
	NUM_FUNC,
} func_type_e;

/******************************************************************************/
//typedefs
/******************************************************************************/
typedef struct sensor_hw_s {
	const char *name;   // sensor name
	uint16_t dev_addr;  // device address
	uint8_t adr_len;    // address length
	uint8_t reg_len;    // register length
	uint16_t chip_reg;  // chip id reg
	uint16_t chip_id;   // chip id val
	uint16_t sta_reg;   // status reg
	uint16_t sta_rdy;   // status ready
	uint16_t data_reg;  // data reg
	uint16_t data_len;  // data length
	uint16_t data_cmd;  // write cmd before read data
	uint16_t timeout;   // timeout for polling
} sensor_hw_t;

typedef struct sensor_io_s {
	uint8_t bus_type; // bus: i2c/spi
	uint8_t bus_id;   // 0/1/2/3
	uint8_t bus_cs;   // cs0/cs1
	uint8_t pwr_io;   // power gpio
	uint8_t pwr_val;  // power gpio val
	uint8_t rst_io;   // reset gpio
	uint8_t rst_val;  // reset gpio val
	uint8_t rst_lt;   // reset low time
	uint8_t rst_ht;   // reset high time
	uint8_t int_io;   // interrupt gpio
	uint8_t int_mode; // interrupt mode
	uint8_t int_mfp;  // interrupt mfp
} sensor_io_t;

typedef struct sensor_cfg_s {
	uint16_t reg;    // register address
	uint16_t len;    // data length
	uint8_t buf[4];  // data list
} sensor_cfg_t;

typedef struct sensor_dev_s {
	sensor_hw_t hw;           // sensor hardware info
	sensor_io_t io;           // gpio and irq config
	void *cfg[NUM_CFG];       // config list
	void *func[NUM_FUNC];     // function list
	void *task;               // task config
} sensor_dev_t;

typedef int (*sensor_func_t)(void);
typedef int (*sensor_cvt_t)(float *val, uint8_t *buf, uint16_t len);

/******************************************************************************/
//functions
/******************************************************************************/
const char *sensor_dev_get_name(const sensor_dev_t *dev);
int sensor_dev_is_valid(const sensor_dev_t *dev);
int sensor_dev_init(const sensor_dev_t *dev);
int sensor_dev_self_test(const sensor_dev_t *dev);
int sensor_dev_write_config(const sensor_dev_t *dev, int type);
int sensor_dev_read_config(const sensor_dev_t *dev, int type, sensor_cfg_t *pcfg, uint16_t len);
int sensor_dev_check_chipid(const sensor_dev_t *dev);
int sensor_dev_get_status(const sensor_dev_t *dev, uint8_t *status);
int sensor_dev_get_data(const sensor_dev_t *dev, uint8_t *buf, uint16_t len);
int sensor_dev_cvt_data(const sensor_dev_t *dev, uint8_t *buf, uint16_t len, float *val);

#endif  /* _SENSOR_DEV_H */

