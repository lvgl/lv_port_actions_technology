/*******************************************************************************
 * @file    sensor_dev.c
 * @author  MEMS Application Team
 * @version V1.0
 * @date    2020-08-12
 * @brief   sensor testing module
*******************************************************************************/

/******************************************************************************/
//includes
/******************************************************************************/
#include <stdio.h>
#include <string.h>
#include <sensor_dev.h>
#include <sensor_bus.h>
#include <zephyr.h>

/******************************************************************************/
//constants
/******************************************************************************/

/******************************************************************************/
//variables
/******************************************************************************/

/******************************************************************************/
//functions
/******************************************************************************/
const char *sensor_dev_get_name(const sensor_dev_t *dev)
{
	return (dev != NULL) ? dev->hw.name : NULL;
}

int sensor_dev_is_valid(const sensor_dev_t *dev)
{
	return (sensor_dev_get_name(dev) != NULL);
}

int sensor_dev_init(const sensor_dev_t *dev)
{
	sensor_func_t init = (sensor_func_t)dev->func[FUNC_INIT];
	int ret = 0;
	
	if (init != NULL) {
		ret = init();
	}
	
	return ret;
}

int sensor_dev_self_test(const sensor_dev_t *dev)
{
	sensor_func_t self_test = (sensor_func_t)dev->func[FUNC_ST];
	int ret = 0;
	
	if (self_test != NULL) {
		ret = self_test();
	}
	
	return ret;
}

int sensor_dev_write_config(const sensor_dev_t *dev, int type)
{
	const sensor_cfg_t *cfg = dev->cfg[type];
	int ret = 0;
	
	// init sensor reg
	while (cfg && (cfg->reg != REG_NULL)) {
		if (cfg->reg == REG_DELAY) {
			k_msleep(cfg->len);
		} else {
			ret = sensor_bus_write(dev, cfg->reg, (uint8_t*)cfg->buf, cfg->len);
		}
		cfg ++;
	}
	
	return ret;
}

int sensor_dev_read_config(const sensor_dev_t *dev, int type, sensor_cfg_t *pcfg, uint16_t len)
{
	const sensor_cfg_t *cfg = dev->cfg[type];
	int ret = 0, cnt = 0;
	
	while (cfg && (cfg->reg != REG_NULL) && (cnt < len)) {
		if ((cfg->reg != REG_DELAY) && (cfg->len > 0)) {
			*pcfg = *cfg;
			memset(pcfg->buf, 0, sizeof(pcfg->buf));
			
			// read reg
			ret = sensor_bus_read(dev, pcfg->reg, pcfg->buf, pcfg->len);
			if (ret != 0) {
				break;
			}
			
			pcfg ++;
			cnt ++;
		}
		cfg ++;
	}
	
	return cnt;
}

int sensor_dev_check_chipid(const sensor_dev_t *dev)
{
	uint16_t chip_id = 0xffff;
	int ret = -1;
	
	if (dev->hw.name != NULL) {
		if (dev->hw.chip_reg == 0xffff) {
			return 0;
		}
		ret = sensor_bus_read(dev, dev->hw.chip_reg, (uint8_t*)&chip_id, dev->hw.reg_len);
		if (dev->hw.reg_len == 1) {
			chip_id &= 0xff;
		}
		if ((ret == 0) && (chip_id == dev->hw.chip_id)) {
			ret = 0;
		} else {
			ret = -1;
		}
	}
	
	return ret;
}

int sensor_dev_get_status(const sensor_dev_t *dev, uint8_t *status)
{
	int ret = 0;
	
	if (dev->hw.sta_reg != REG_NULL) {
		ret = sensor_bus_read(dev, dev->hw.sta_reg, status, dev->hw.reg_len);
	} else {
		*status = 0;
	}
	
	return ret;
}

int sensor_dev_get_data(const sensor_dev_t *dev, uint8_t *buf, uint16_t len)
{
	unsigned int start, end;
	int ret = -1;
	
	// check data length
	if ((dev->hw.data_len > 0) && (len > dev->hw.data_len)) {
		len = dev->hw.data_len;
	}
	
	// polling mode
	if (dev->hw.data_cmd > 0) {
		// write measure command
		sensor_bus_write(dev, dev->hw.data_cmd, NULL, 0);
		
		// wait for ack
		start = k_uptime_get_32();
		do {
			ret = sensor_bus_read(dev, dev->hw.data_reg, buf, len);
			end = k_uptime_get_32();
		} while ((ret != 0) && ((end - start) < dev->hw.period_ms));
	} else if (dev->hw.data_reg != REG_NULL) {
		// read data
		ret = sensor_bus_read(dev, dev->hw.data_reg, buf, len);
	}
	
	return (ret == 0) ? len : 0;
}

int sensor_dev_cvt_data(const sensor_dev_t *dev, uint8_t *buf, uint16_t len, float *val, int16_t *raw)
{
	sensor_cvt_t cvt = (sensor_cvt_t)dev->func[FUNC_CVT];
	int ret = 0;
	
	// convert data
	if (cvt != NULL) {
		ret = cvt(val, buf, len, raw);
	}
	
	return ret;
}

int sensor_dev_is_trigger_enabled(const sensor_dev_t *dev)
{
	task_trig_t *task_trig = (task_trig_t*)dev->task;

	return (task_trig && task_trig->en);
}

int sensor_dev_get_data_period(const sensor_dev_t *dev)
{
	return dev->hw.period_ms;
}

