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
#include <sensor_hal.h>
#include <sensor_io.h>
#include <sensor_bus.h>
#include <sensor_devices.h>
#include <zephyr.h>
#include <soc.h>
#include <os_common_api.h>

/******************************************************************************/
//constants
/******************************************************************************/

/******************************************************************************/
//variables
/******************************************************************************/
//__weak __act_s2_sleep_data sensor_dev_t sensor_dev[NUM_SENSOR] = {0};
static __act_s2_sleep_data sensor_dev_t sensor_dev[NUM_SENSOR] = {0};
static __act_s2_sleep_data uint32_t last_32k_counter = 0;
static __act_s2_sleep_data uint32_t current_30k_counter = 0;

static const char* sensor_name[NUM_SENSOR] = 
{
	"ACC", "GYRO", "MAG", "BARO", "TEMP", "HR", "GNSS", "OFFBODY",
};

static sensor_cb_t sensor_cb[NUM_SENSOR] = { NULL };
static void *sensor_cb_ctx[NUM_SENSOR] = { NULL };
static uint8_t sensor_en[NUM_SENSOR] = { 0 };
static uint8_t sensor_task[NUM_SENSOR] = { 0 };
static sensor_dat_t sensor_dat[NUM_SENSOR] = { 0 };

/******************************************************************************/
//functions
/******************************************************************************/
int sensor_hal_init(void)
{
	int id, ret = 0;
	const sensor_dev_t *dev;

	// Init sensor
	for (id = 0; id < NUM_SENSOR; id ++) {
		dev = sensor_dev_list[id];
		if (sensor_dev_is_valid(dev)) {
			sensor_io_init(dev);
		}
		
		while (sensor_dev_is_valid(dev)) {
			ret = sensor_dev_check_chipid(dev);
			if (ret == 0) {
				printk("sensor[%s] = %s\n", sensor_hal_get_type(id), sensor_dev_get_name(dev));
				break;
			}
			dev ++;
		}
		sensor_dev[id] = *dev;
	}

	// write config and probe
	for (id = 0; id < NUM_SENSOR; id ++) {
		dev = &sensor_dev[id];
		if (sensor_dev_is_valid(dev)) {
			// write init config
			sensor_dev_write_config(dev, CFG_INIT);
			
			// init device
			sensor_dev_init(dev);
			
			// self test
			sensor_dev_self_test(dev);
		}
	}

	return ret;
}

int sensor_hal_deinit(void)
{
	int id;
	const sensor_dev_t *dev;
	
	// sensor device deinit
	for (id = 0; id < NUM_SENSOR; id ++) {
		dev = &sensor_dev[id];
		if (sensor_dev_get_name(dev) != NULL) {
			// write off config
			sensor_dev_write_config(dev, CFG_OFF);
			
			// io deinit
			sensor_io_deinit(dev);
		}
	}
	
	return 0;
}

int sensor_hal_dump(void)
{
	int id, ret, idx, off;
	sensor_cfg_t cfg[4];
	const sensor_dev_t *dev;
	
	for (id = 0; id < NUM_SENSOR; id ++) {
		dev = &sensor_dev[id];
		if (sensor_dev_is_valid(dev)) {
			printf("[%s] dump reg\r\n", sensor_dev_get_name(dev));
			
			// read config
			ret = sensor_dev_read_config(dev, CFG_ON, cfg, 4);
			if (ret > 4) {
				ret = 4;
			}
			for (idx = 0; idx < ret; idx ++) {
				printf("[0x%x]=", cfg[idx].reg);
				for (off = 0; off < cfg[idx].len; off ++) {
					printf("0x%x ", cfg[idx].buf[off]);
				}
				printf("\r\n");
			}
			printf("\r\n");
		}
	}
		
	return 0;
}

static int sensor_is_wakeable(int id)
{
	return (id == ID_ACC) || (id == ID_HR);
}

int sensor_hal_suspend(void)
{
	int id;
	const sensor_dev_t *dev;
	
	for (id = 0; id < NUM_SENSOR; id ++) {
		dev = &sensor_dev[id];
		if (sensor_dev_is_valid(dev) && !sensor_is_wakeable(id) && sensor_en[id]) {
			// write off config
			SYS_LOG_INF("[%s] off", sensor_dev_get_name(dev));
			sensor_dev_write_config(dev, CFG_OFF);

			// io deinit
			sensor_io_deinit(dev);
		}
	}
	
	return 0;
}

int sensor_hal_resume(void)
{
	int id;
	const sensor_dev_t *dev;
	
	for (id = 0; id < NUM_SENSOR; id ++) {
		dev = &sensor_dev[id];
		if (sensor_dev_is_valid(dev) && !sensor_is_wakeable(id) && sensor_en[id]) {
			// io init
			sensor_io_init(dev);

			// write on config
			SYS_LOG_INF("[%s] on", sensor_dev_get_name(dev));
			sensor_dev_write_config(dev, CFG_ON);
		}
	}
	
	return 0;
}

const char *sensor_hal_get_type(int id)
{
	return sensor_name[id];
}

const char *sensor_hal_get_name(int id)
{
	return sensor_dev_get_name(&sensor_dev[id]);
}

void sensor_hal_add_callback(int id, sensor_cb_t cb, void *ctx)
{
	uint32_t key;
	
	key = irq_lock();
	sensor_cb[id] = cb;
	sensor_cb_ctx[id] = ctx;
	irq_unlock(key);
}

void sensor_hal_remove_callback(int id)
{
	uint32_t key;
	
	key = irq_lock();
	sensor_cb[id] = NULL;
	sensor_cb_ctx[id] = NULL;
	irq_unlock(key);
}

void sensor_hal_init_data(int id, sensor_dat_t *dat, uint8_t *buf, int len)
{
	const sensor_dev_t *dev = &sensor_dev[id];

	if (!sensor_dev_is_valid(dev)) {
		return;
	}

	dat->id = id;
	if (buf) {
		dat->evt = EVT_TASK;
	} else {
		dat->evt = EVT_IRQ;
	}
	dat->pd = dev->hw.period_ms;
	dat->sz = dev->hw.data_len;
	if (dat->sz > 0) {
		dat->cnt = len / dat->sz;
	} else {
		dat->cnt = 0;
	}
	dat->buf = buf;
	dat->ts = (uint64_t)soc_sys_uptime_get();
}

static void sensor_irq_callback(int pin, int id)
{
	if (sensor_cb[id] != NULL) {
		// sensor irq event
		sensor_hal_init_data(id, &sensor_dat[id], NULL, 0);
		
		// callback
		sensor_cb[id](id, &sensor_dat[id], sensor_cb_ctx[id]);
	}
}

static void sensor_task_callback(uint8_t *buf, int len, void *ctx)
{
	int id = (int)ctx;
	
	if (sensor_cb[id] != NULL) {
		// sensor task event
		sensor_hal_init_data(id, &sensor_dat[id], buf, len);
		
		// callback
		sensor_cb[id](id, &sensor_dat[id], sensor_cb_ctx[id]);
	}
}

int sensor_hal_enable(int id)
{
	int ret, task_poll = 0;
	uint8_t buf[16];	
	const sensor_dev_t *dev = &sensor_dev[id];

	if (!sensor_dev_is_valid(dev)) {
		return -1;
	}
	
	if (sensor_en[id]) {
		return 0;
	}
	
	// write on config
	ret = sensor_dev_write_config(dev, CFG_ON);
	if (ret != 0) {
		return -1;
	}
	sensor_en[id] = 1;
	
#ifdef CONFIG_SENSOR_TASK_POLL
	if (sensor_dev_is_trigger_enabled(dev)) {
		task_poll = 1;
	}
#endif

	// start task
	if (task_poll) {
		sensor_task[id] = 1;
		
		// disable irq
		sensor_io_disable_irq(dev, sensor_irq_callback, id);
		
		// enable trigger
		sensor_io_enable_trig(dev, id);
		
		/* start task */
		ret = sensor_bus_task_start(dev, sensor_task_callback, (void*)id);
		if (ret != 0) {
			task_poll = 0;
		}
	}

	// enable irq
	if (!task_poll) {
		sensor_task[id] = 0;
		
		// disable trigger
		sensor_io_disable_trig(dev, id);
		
		// enable irq
		sensor_io_enable_irq(dev, sensor_irq_callback, id);
	}
	
	// read data to clear irq
	sensor_dev_get_data(dev, buf, sizeof(buf));
	
	return 0;
}

int sensor_hal_disable(int id)
{
	int ret = 0;
	const sensor_dev_t *dev = &sensor_dev[id];

	if (!sensor_dev_is_valid(dev)) {
		return -1;
	}
	
	if (!sensor_en[id]) {
		return 0;
	}
	
	// write off config
	ret = sensor_dev_write_config(dev, CFG_OFF);
	if (ret != 0) {
		return -1;
	}
	sensor_en[id] = 0;
	
	// disable trigger
	sensor_io_disable_trig(dev, id);
	
	// disable irq
	sensor_io_disable_irq(dev, sensor_irq_callback, id);
	
	// stop task
	if (sensor_task[id]) {
		sensor_task[id] = 0;
		ret = sensor_bus_task_stop(dev);
	}
	
	return ret;
}

int sensor_hal_read(int id, uint16_t reg, uint8_t *buf, uint16_t len)
{
	const sensor_dev_t *dev = &sensor_dev[id];

	if (!sensor_dev_is_valid(dev)) {
		return -1;
	}

	return sensor_bus_read(dev, reg, buf, len);
}

int sensor_hal_write(int id, uint16_t reg, uint8_t *buf, uint16_t len)
{
	const sensor_dev_t *dev = &sensor_dev[id];

	if (!sensor_dev_is_valid(dev)) {
		return -1;
	}

	return sensor_bus_write(dev, reg, buf, len);
}

int sensor_hal_poll_data(int id, sensor_dat_t *dat, uint8_t *buf)
{
	const sensor_dev_t *dev = &sensor_dev[id];
	int ret = -1;
	uint16_t status;

	if (!sensor_dev_is_valid(dev)) {
		return -1;
	}

	// check buffer
	if ((buf == NULL) && (dat->buf == NULL)) {
		return -1;
	}
	
	// check status
	if (dev->hw.sta_reg != REG_NULL) {
		ret = sensor_bus_read(dev, dev->hw.sta_reg, (uint8_t*)&status, dev->hw.reg_len);
		if((ret != 0) || !(status & dev->hw.sta_rdy)) {
			return -2;
		}
	}
	
	// init data
	sensor_hal_init_data(id, dat, NULL, 0);
	dat->cnt = 1;
	if (buf != NULL) {
		dat->buf = buf;
	}
	
	// get data	
	return sensor_dev_get_data(dev, dat->buf, dat->sz);
}

int sensor_hal_get_value(int id, sensor_dat_t *dat, uint16_t idx, float *val)
{
	// check args
	if ((dat->buf == NULL) || (idx >= dat->cnt) || (val == NULL)) {
		return -1;
	}
	
	// convert data
	return sensor_hal_cvt_data(id, dat->buf + dat->sz * idx, dat->sz, val, NULL);
}

int sensor_hal_cvt_data(int id, uint8_t *buf, uint16_t len, float *val, int16_t *raw)
{
	const sensor_dev_t *dev = &sensor_dev[id];

	if (!sensor_dev_is_valid(dev)) {
		return -1;
	}

	// convert data
	return sensor_dev_cvt_data(dev, buf, len, val, raw);
}

int sensor_hal_get_data_period(int id)
{
	const sensor_dev_t *dev = &sensor_dev[id];

	if (!sensor_dev_is_valid(dev)) {
		return 20; // default: 50Hz(20ms)
	}

	return sensor_dev_get_data_period(dev);
}

void sensor_hal_config_tm(int id, int ms)
{
	const sensor_dev_t *dev = &sensor_dev[id];

	if (!sensor_dev_is_valid(dev)) {
		return;
	}

	// config timer period
	sensor_io_config_tm(dev, ms);
}

uint32_t sensor_hal_get_tm(int id)
{
	const sensor_dev_t *dev = &sensor_dev[id];

	if (!sensor_dev_is_valid(dev)) {
		return 0;
	}

	// read timer cycle
	return sensor_io_get_tm(dev);
}

uint32_t sensor_hal_get_30K_counter(int id)
{
	uint32_t current_32k_counter = 0;

	current_32k_counter = sensor_hal_get_tm(id);

	if(last_32k_counter > current_32k_counter){
		current_30k_counter += (uint32_t)(((uint64_t)(0xffffffff - last_32k_counter + current_32k_counter) * 30000) / soc_rc32K_freq());
	}else{
		current_30k_counter += (uint32_t)(((uint64_t)(current_32k_counter - last_32k_counter) * 30000) / soc_rc32K_freq());
	}
	last_32k_counter = current_32k_counter;
	return current_30k_counter;
}

void sensor_hal_clear_tm_pending(int id)
{
	// clear timer pending
	sensor_io_clear_tm_pending(id);
}

