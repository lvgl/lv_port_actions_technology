/*******************************************************************************
 * @file    sensor_bus.c
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
#include <sensor_bus.h>
#include <zephyr.h>
#include <drivers/gpio.h>
#include <drivers/i2cmt.h>
#include <drivers/spimt.h>
#include <os_common_api.h>
#include <board_cfg.h>

/******************************************************************************/
//constants
/******************************************************************************/

/******************************************************************************/
//variables
/******************************************************************************/
#ifdef CONFIG_SENSOR_TASK_CFG
static struct k_sem sensor_sem[NUM_BUS][2];
static __act_s2_sleep_data volatile int8_t sensor_stat[NUM_BUS][2] = { 0 };
#endif

static __act_s2_sleep_data struct device *spimt_dev[2] = { NULL };
static __act_s2_sleep_data struct device *i2cmt_dev[2] = { NULL };

/******************************************************************************/
//functions
/******************************************************************************/
#ifdef CONFIG_SENSOR_TASK_CFG

static void sensor_bus_callback(unsigned char *buf, int len, void *ctx)
{
	const sensor_dev_t *dev = (const sensor_dev_t*)ctx;
	
	// sensor status
	if (buf != NULL) {
		sensor_stat[dev->io.bus_type][dev->io.bus_id] = 0;
	} else {
		sensor_stat[dev->io.bus_type][dev->io.bus_id] = -1;
	}
	
	// sensor sync
	k_sem_give(&sensor_sem[dev->io.bus_type][dev->io.bus_id]);
}

static int sensor_bus_i2c_xfer_task(const sensor_dev_t *dev, uint32_t task_id, i2c_task_t *task)
{
	int ret;
	uint32_t pre_time, cur_time;

	/* reset sem */
	if (!soc_in_sleep_mode()) {
		k_sem_reset(&sensor_sem[dev->io.bus_type][dev->io.bus_id]);
	}

	/* start task */
	i2c_register_callback(i2cmt_dev[dev->io.bus_id], task_id, sensor_bus_callback, (void*)dev);
	sensor_stat[dev->io.bus_type][dev->io.bus_id] = -1;
	ret = i2c_task_start(i2cmt_dev[dev->io.bus_id], task_id, task);
	if(ret) {
		return -1;
	}
	
	/* wait task */
	if (soc_in_sleep_mode()) {
		pre_time = (uint32_t)soc_sys_uptime_get();
		sensor_stat[dev->io.bus_type][dev->io.bus_id] = 0;
		while(!i2c_task_get_data(dev->io.bus_id, task_id, -1, NULL)) {
			// 20ms timeout
			cur_time = (uint32_t)soc_sys_uptime_get();
			if ((cur_time - pre_time) > 20) {
				sensor_stat[dev->io.bus_type][dev->io.bus_id] = -1;
				break;
			}
		}
	} else {
		ret = k_sem_take(&sensor_sem[dev->io.bus_type][dev->io.bus_id], K_MSEC(20));
		if (ret) {
			sensor_stat[dev->io.bus_type][dev->io.bus_id] = -1;
			SYS_LOG_ERR("sensor: %s i2c timeout\n", dev->hw.name);
		}
	}
	
	/* stop task */
	i2c_task_stop(i2cmt_dev[dev->io.bus_id], task_id);
	i2c_register_callback(i2cmt_dev[dev->io.bus_id], task_id, NULL, NULL);
	
	return sensor_stat[dev->io.bus_type][dev->io.bus_id];
}

static int sensor_bus_i2c_xfer(const sensor_dev_t *dev, uint16_t reg, uint8_t *buf, uint16_t len, uint8_t rd)
{
	i2c_task_t i2c_task, addr_task;
	uint32_t task_id, task_off;
	uint8_t wr_buf[16];
	
	if (dev->task == NULL) {
		return -1;
	}
	
	/* config task */
	task_id = I2C_TASK_NUM - 1;
	task_off = dev->io.bus_id * I2C_TASK_NUM + task_id;
	i2c_task = *(i2c_task_t*)dev->task;
	i2c_task.irq_type = I2C_TASK_IRQ_CMPLT | I2C_TASK_IRQ_NACK;
	i2c_task.ctl.soft = 1;
#if defined(CONFIG_SOC_SERIES_LEOPARD)
	i2c_task.slavedev.rwsel = rd;
	i2c_task.slavedev.sdataddr = reg;
#else
	i2c_task.ctl.rwsel = rd;
	i2c_task.ctl.sdataddr = reg;
#endif
	i2c_task.ctl.rdlen_wsdat = len;
	i2c_task.dma.reload = 0;
	i2c_task.dma.addr = (uint32_t)buf;
	i2c_task.dma.len = len;
	i2c_task.trig.en = 1;
	i2c_task.trig.task = I2CMT0_TASK0 + task_off;
	
	// ignore data address
	if (reg == REG_NULL) {
		i2c_task.ctl.tdataddr = 1;
	} else {
		// process 2 bytes address
		if (dev->hw.adr_len > 1) {
			// write reg_h + reg_l
#if defined(CONFIG_SOC_SERIES_LEOPARD)
			i2c_task.slavedev.sdataddr = (reg >> 8);
#else
			i2c_task.ctl.sdataddr = (reg >> 8);
#endif
			wr_buf[0] = (reg & 0xff);
			if (!rd) {
				if ((len + 1) > sizeof(wr_buf)) {
					return -2;
				}
				// write wr_buf(reg_l + buf)
				if (len > 0) {
					memcpy(&wr_buf[1], buf, len);
				}
				i2c_task.dma.addr = (uint32_t)wr_buf;
				i2c_task.dma.len = len + 1;
			} else {
				// write 2 bytes address
				addr_task = i2c_task;
#if defined(CONFIG_SOC_SERIES_LEOPARD)
				addr_task.slavedev.rwsel = 0;
#else
				addr_task.ctl.rwsel = 0;
#endif
				addr_task.dma.addr = (uint32_t)wr_buf;
				addr_task.dma.len = 1;
				// ignore data address
				i2c_task.ctl.tdataddr = 1;
				sensor_bus_i2c_xfer_task(dev, task_id, &addr_task);
			}
		}
	}
	
	/* start task */
	return sensor_bus_i2c_xfer_task(dev, task_id, &i2c_task);
}

static int sensor_bus_spi_xfer_task(const sensor_dev_t *dev, uint32_t task_id, spi_task_t *task)
{
	int ret;
#if IS_ENABLED(CONFIG_SPIMT_0) || IS_ENABLED(CONFIG_SPIMT_1)
	uint32_t pre_time, cur_time;
#endif

	/* reset sem */
	if (!soc_in_sleep_mode()) {
		k_sem_reset(&sensor_sem[dev->io.bus_type][dev->io.bus_id]);
	}

	/* start task */
	spi_register_callback(spimt_dev[dev->io.bus_id], task_id, sensor_bus_callback, (void*)dev);
	sensor_stat[dev->io.bus_type][dev->io.bus_id] = -1;
	ret = spi_task_start(spimt_dev[dev->io.bus_id], task_id, task);
	if(ret) {
		return -1;
	}

	/* wait task */
	if (soc_in_sleep_mode()) {
	#if IS_ENABLED(CONFIG_SPIMT_0) || IS_ENABLED(CONFIG_SPIMT_1)
		pre_time = (uint32_t)soc_sys_uptime_get();
		sensor_stat[dev->io.bus_type][dev->io.bus_id] = 0;
		while(!spi_task_get_data(dev->io.bus_id, task_id, -1, NULL)) {
			// 20ms timeout
			cur_time = (uint32_t)soc_sys_uptime_get();
			if ((cur_time - pre_time) > 20) {
				sensor_stat[dev->io.bus_type][dev->io.bus_id] = -1;
				break;
			}
		}
	#endif
	} else {
		ret = k_sem_take(&sensor_sem[dev->io.bus_type][dev->io.bus_id], K_MSEC(20));
		if (ret) {
			sensor_stat[dev->io.bus_type][dev->io.bus_id] = -1;
//			SYS_LOG_ERR("sensor: %s spi timeout\n", dev->hw.name);
		}
	}
	
	/* stop task */
	spi_task_stop(spimt_dev[dev->io.bus_id], task_id);
	spi_register_callback(spimt_dev[dev->io.bus_id], task_id, NULL, NULL);
	
	return sensor_stat[dev->io.bus_type][dev->io.bus_id];
}

static int sensor_bus_spi_xfer(const sensor_dev_t *dev, uint16_t reg, uint8_t *buf, uint16_t len, uint8_t rd)
{
	spi_task_t spi_task;
	uint32_t task_id, task_off;
	
	if (dev->task == NULL) {
		return -1;
	}

	/* config task */
	spi_task = *(spi_task_t*)dev->task;
	spi_task.irq_type = SPI_TASK_IRQ_CMPLT;
	spi_task.ctl.soft = 1;
	spi_task.ctl.rwsel = rd;
	spi_task.ctl.sbsh = (dev->hw.adr_len > 1);
	spi_task.ctl.wsdat = reg;
	if (rd) {
		if (dev->hw.adr_len > 1) {
			spi_task.ctl.wsdat |= (1 << 15);
		} else {
			spi_task.ctl.wsdat |= (1 << 7);
		}
	}
	spi_task.ctl.rdlen = len;
	spi_task.dma.reload = 0;
	spi_task.dma.addr = (uint32_t)buf;
	spi_task.dma.len = len;
#if defined(CONFIG_SOC_SERIES_LEOPARD)
	spi_task.task_cs = dev->io.bus_cs;
	task_id = SPI_TASK_NUM / 2 - 1;
#else
	if (dev->io.bus_cs == 0) {
		task_id = SPI_TASK_NUM / 2 - 1;
	} else {
		task_id = SPI_TASK_NUM - 1;
	}
#endif
	task_off = dev->io.bus_id * SPI_TASK_NUM + task_id;
	spi_task.trig.task = SPIMT0_TASK0 + task_off;
	
	/* start task */
	return sensor_bus_spi_xfer_task(dev, task_id, &spi_task);
}

#else

static int sensor_bus_i2c_read(const sensor_dev_t *dev, uint16_t reg, uint8_t *buf, uint16_t len)
{
	int ret = 0;
	uint8_t cmd[4];

	if (reg != REG_NULL) {
		if (dev->hw.adr_len > 1) {
			cmd[0] = (reg >> 8) & 0xff;
			cmd[1] = reg & 0xff;
		} else {
			cmd[0] = reg;
		}
		ret = i2c_write_read(i2cmt_dev[dev->io.bus_id], dev->hw.dev_addr, cmd, dev->hw.adr_len, buf, len);
	} else {
		ret = i2c_read(i2cmt_dev[dev->io.bus_id], buf, len, dev->hw.dev_addr);
	}
	
	return ret;
}

static int sensor_bus_i2c_write(const sensor_dev_t *dev, uint16_t reg, uint8_t *buf, uint16_t len)
{
	int ret = 0;
	uint8_t wr_buf[16];

	if (reg != REG_NULL) {
		if ((dev->hw.adr_len + len) > sizeof(wr_buf)) {
//			SYS_LOG_ERR("sensor: i2c_write %d bytes too long!\n", len);
			return -1;
		}
		if (dev->hw.adr_len > 1) {
			wr_buf[0] = (reg >> 8) & 0xff;
			wr_buf[1] = reg & 0xff;
		} else {
			wr_buf[0] = reg;
		}
		memcpy(wr_buf+dev->hw.adr_len, buf, len);
		ret = i2c_write(i2cmt_dev[dev->io.bus_id], wr_buf, dev->hw.adr_len+len, dev->hw.dev_addr);
	} else {
		ret = i2c_write(i2cmt_dev[dev->io.bus_id], buf, len, dev->hw.dev_addr);
	}

	return ret;
}

static int sensor_bus_spi_read(const sensor_dev_t *dev, uint16_t reg, uint8_t *buf, uint16_t len)
{
	int ret = 0;
	uint8_t cmd[4];
	const struct spi_buf tx_bufs[] = { { .buf = cmd, .len = 1, },	};
	const struct spi_buf rx_bufs[] = { { .buf = buf, .len = len, }, };
	const struct spi_buf_set tx = {	.buffers = tx_bufs,	.count = ARRAY_SIZE(tx_bufs) };
	const struct spi_buf_set rx = {	.buffers = rx_bufs,	.count = ARRAY_SIZE(rx_bufs) };

	if (reg != REG_NULL) {
		cmd[0] = reg | (1 << 7); //rw=1
		ret = spi_transceive(spimt_dev[dev->io.bus_id], NULL, &tx, &rx);
	} else {
		ret = spi_read(spimt_dev[dev->io.bus_id], NULL, &rx);
	}

	return ret;
}

static int sensor_bus_spi_write(const sensor_dev_t *dev, uint16_t reg, uint8_t *buf, uint16_t len)
{
	int ret = 0;
	uint8_t cmd[4];
	const struct spi_buf tx_bufs[] = { { .buf = buf, .len = len, },	};
	const struct spi_buf_set tx = {	.buffers = tx_bufs,	.count = ARRAY_SIZE(tx_bufs) };
	const struct spi_buf tx2_bufs[] = { { .buf = cmd, .len = 1, }, { .buf = buf, .len = len, },	};
	const struct spi_buf_set tx2 = {	.buffers = tx2_bufs,	.count = ARRAY_SIZE(tx2_bufs) };

	if (reg != REG_NULL) {
		cmd[0] = reg & ~(1 << 7); //rw=0
		ret = spi_write(spimt_dev[dev->io.bus_id], NULL, &tx2);
	} else {
		ret = spi_write(spimt_dev[dev->io.bus_id], NULL, &tx);
	}

	return ret;
}

static int sensor_bus_i2c_xfer(const sensor_dev_t *dev, uint16_t reg, uint8_t *buf, uint16_t len, uint8_t rd)
{
	int ret = 0;
	
	if (rd) {
		ret = sensor_bus_i2c_read(dev, reg, buf, len);
	} else {
		ret = sensor_bus_i2c_write(dev, reg, buf, len);
	}
	
	return ret;
}

static int sensor_bus_spi_xfer(const sensor_dev_t *dev, uint16_t reg, uint8_t *buf, uint16_t len, uint8_t rd)
{
	int ret = 0;
	
	spi_cs_select(spimt_dev[dev->io.bus_id], dev->io.bus_cs);
	
	if (rd) {
		ret = sensor_bus_spi_read(dev, reg, buf, len);
	} else {
		ret = sensor_bus_spi_write(dev, reg, buf, len);
	}
	
	return ret;
}

#endif

static int sensor_bus_xfer(const sensor_dev_t *dev, uint16_t reg, uint8_t *buf, uint16_t len, uint8_t rd)
{
	int ret = 0;
	
	if (dev->hw.name == NULL) {
		return -1;
	}
	
	if (dev->io.bus_type == BUS_I2C) {
		ret = sensor_bus_i2c_xfer(dev, reg, buf, len, rd);
	} else if (dev->io.bus_type == BUS_SPI) {
		ret = sensor_bus_spi_xfer(dev, reg, buf, len, rd);
	}
	
	return ret;
}

int sensor_bus_read(const sensor_dev_t *dev, uint16_t reg, uint8_t *buf, uint16_t len)
{
	return sensor_bus_xfer(dev, reg, buf, len, 1);
}

int sensor_bus_write(const sensor_dev_t *dev, uint16_t reg, uint8_t *buf, uint16_t len)
{
	return sensor_bus_xfer(dev, reg, buf, len, 0);
}

static int sensor_bus_i2c_task_start(int bus_id, i2c_task_t *task, sensor_bus_cb_t cb, void *ctx)
{
	uint32_t task_id;

	// get task id
	task_id = ((task->trig.task - SPI_TASK_NUM * 2) % I2C_TASK_NUM);
	
	// add task callback
	i2c_register_callback(i2cmt_dev[bus_id], task_id, cb, ctx);
	
	// start task
	return i2c_task_start(i2cmt_dev[bus_id], task_id, task);
}

static int sensor_bus_i2c_task_stop(int bus_id, i2c_task_t *task)
{
	uint32_t task_id;
	
	// get task id
	task_id = ((task->trig.task - SPI_TASK_NUM * 2) % I2C_TASK_NUM);
	
	// remove task callback
	i2c_register_callback(i2cmt_dev[bus_id], task_id, NULL, NULL);
	
	// stop task
	return i2c_task_stop(i2cmt_dev[bus_id], task_id);
}

static int sensor_bus_spi_task_start(int bus_id, spi_task_t *task, sensor_bus_cb_t cb, void *ctx)
{
	uint32_t task_id;

	// get task id
	task_id = (task->trig.task % SPI_TASK_NUM);
	
	// add task callback
	spi_register_callback(spimt_dev[bus_id], task_id, cb, ctx);
	
	// start task
	return spi_task_start(spimt_dev[bus_id], task_id, task);
}

static int sensor_bus_spi_task_stop(int bus_id, spi_task_t *task)
{
	uint32_t task_id;
	
	// get task id
	task_id = (task->trig.task % SPI_TASK_NUM);
		
	// remove task callback
	spi_register_callback(spimt_dev[bus_id], task_id, NULL, NULL);
	
	// stop task
	return spi_task_stop(spimt_dev[bus_id], task_id);
}

int sensor_bus_task_start(const sensor_dev_t *dev, sensor_bus_cb_t cb, void *ctx)
{
	int ret = -1;
	
	if ((dev->task == NULL) || (cb == NULL)) {
		return -1;
	}
	
	if (dev->io.bus_type == BUS_I2C) {
		ret = sensor_bus_i2c_task_start(dev->io.bus_id, (i2c_task_t*)dev->task, cb, ctx);
	} else if (dev->io.bus_type == BUS_SPI) {
		ret = sensor_bus_spi_task_start(dev->io.bus_id, (spi_task_t*)dev->task, cb, ctx);
	}
	
	return ret;
}

int sensor_bus_task_stop(const sensor_dev_t *dev)
{
	int ret = -1;
	
	if (dev->task == NULL) {
		return -1;
	}

	if (dev->io.bus_type == BUS_I2C) {
		ret = sensor_bus_i2c_task_stop(dev->io.bus_id, (i2c_task_t*)dev->task);
	} else if (dev->io.bus_type == BUS_SPI) {
		ret = sensor_bus_spi_task_stop(dev->io.bus_id, (spi_task_t*)dev->task);
	}
	
	return ret;
}

static int sensor_bus_init(const struct device *dev)
{
#ifdef CONFIG_SENSOR_TASK_CFG
	uint32_t i, j;
#endif

	#ifdef CONFIG_SENSOR_TASK_CFG
	/* init sem */
	for (i = 0; i < NUM_BUS; i ++) {
		for (j = 0; j < 2; j ++) {
			k_sem_init(&sensor_sem[i][j], 0, UINT_MAX);
		}
	}
	#endif
	
	ARG_UNUSED(dev);

	/* get device */
	spimt_dev[0] = (struct device*)device_get_binding("SPIMT_0");
	if (!spimt_dev[0]) {
		//SYS_LOG_ERR("[spimt0] get device failed!");
	}

	/* get device */
	spimt_dev[1] = (struct device*)device_get_binding("SPIMT_1");
	if (!spimt_dev[1]) {
		//SYS_LOG_ERR("[spimt0] get device failed!");
	}

	/* get device */
	i2cmt_dev[0] = (struct device*)device_get_binding("I2CMT_0");
	if (!i2cmt_dev[0]) {
		SYS_LOG_ERR("[i2cmt0] get device failed!");
		//return -1;
	}

	/* get device */
	i2cmt_dev[1] = (struct device*)device_get_binding("I2CMT_1");
	if (!i2cmt_dev[1]) {
		SYS_LOG_ERR("[i2cmt1] get device failed!");
		//return -1;
	}
	


	return 0;
}

SYS_INIT(sensor_bus_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

