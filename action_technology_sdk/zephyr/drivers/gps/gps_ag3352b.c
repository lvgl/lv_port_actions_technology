/*
 * Copyright (c) 2022 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <soc.h>
#include <board.h>
#include <device.h>
#include <drivers/uart.h>
#include <drivers/uart_dma.h>
#include <logging/log.h>
#include <gps/gps.h>

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

LOG_MODULE_REGISTER(gps_3352, LOG_LEVEL_INF);
#define CONFIG_GPS_DEV_NAME "gps"
#define GPS_UERT_DEV_NAME CONFIG_UART_2_NAME

#ifdef CONFIG_SENSOR_GPS_UART_DMA
#define GPS_DATA_BUFFER_SIZE 100
#else
#define UART_FIFO_MAX 16
#define GPS_DATA_BUFFER_SIZE 100
#endif // CONFIG_SENSOR_GPS_UART_DMA

struct ag3352_data {
	const struct device *uart_gpio_dev;
#ifdef CONFIG_SENSOR_GPS_UART_DMA
	uint8_t temp_buff[GPS_DATA_BUFFER_SIZE];
#else
	uint8_t gps_data[2][GPS_DATA_BUFFER_SIZE];
	uint8_t gpa_data_cur;
	uint8_t gpa_data_prev;
#endif // CONFIG_SENSOR_GPS_UART_DMA
	gps_notify_t notify;
	struct k_work gps_work;
};

static struct ag3352_data ag3352_data;

#ifdef CONFIG_SENSOR_GPS_UART_DMA
void uart_ctrl_rx_dma_start(struct device *dev)
{
	uart_dma_receive(dev, ag3352_data.temp_buff, GPS_DATA_BUFFER_SIZE);

	/* timeout???????????????
     */
	// hrtimer_start(&g_tool_data.timer, UART_CTRL_RX_DMA_TIMEOUT_US, 0);
}

static void uart_ctrl_rx_dma_handler(const struct device *dma_dev, void *user_data,
				     uint32_t channel, int status)
{
	LOG_INF("%s", ag3352_data.temp_buff);
	// hrtimer_stop(&g_tool_data.timer);

	// ring_buf_put(&g_tool_data.rx_ringbuf, g_tool_data.temp_buff, UART_CTRL_RX_TEMP_BUF_SIZE);

	uart_ctrl_rx_dma_start((struct device *)ag3352_data.uart_gpio_dev);
}
#else
static void uart_fifo_callback(const struct device *dev, void *user_data)
{
	uint8_t rx_buff[UART_FIFO_MAX];
	int read_size;
	static uint8_t cnt;

	uart_irq_update(dev);
	if (uart_irq_rx_ready(dev)) {
		read_size = uart_fifo_read(dev, rx_buff, UART_FIFO_MAX);
		if (read_size == UART_FIFO_MAX)
			LOG_ERR("uart fifo buffer overflow");

		for (int i = 0; i < read_size; i++) {
			switch (rx_buff[i]) {
			case '$':
				if (ag3352_data.gpa_data_cur)
					ag3352_data.gpa_data_cur = 0;
				else
					ag3352_data.gpa_data_cur = 1;

				memset(ag3352_data.gps_data[ag3352_data.gpa_data_cur], 0,
				       sizeof(ag3352_data.gps_data[ag3352_data.gpa_data_cur]));
				cnt = 0;
				ag3352_data.gps_data[ag3352_data.gpa_data_cur][cnt] = rx_buff[i];
				break;

			case '\n':
				cnt++;
				ag3352_data.gps_data[ag3352_data.gpa_data_cur][cnt] = rx_buff[i];
				ag3352_data.gpa_data_prev = ag3352_data.gpa_data_cur;
				k_work_submit(&ag3352_data.gps_work);
				break;

			default:
				cnt++;
				ag3352_data.gps_data[ag3352_data.gpa_data_cur][cnt] = rx_buff[i];

				/* protection data cannot exceed boundary */
				if (cnt >= GPS_DATA_BUFFER_SIZE)
					cnt--;
				break;
			}
		}
	}
}
#endif // CONFIG_SENSOR_GPS_UART_DMA
static void ag3352_enable(const struct device *dev)
{
	struct ag3352_data *data = (struct ag3352_data *)dev->data;
	sys_write32(0x3, HGLFCLK_CTL);
	sys_write32((sys_read32(CMU_DEVCLKEN1) | (1 << 31)), CMU_DEVCLKEN1);
	sys_write32(0x101e, 0x400680F4); //GPIO61 set 0x1E: LOSCOUT

	gps_reset_pin_ctl();
	gps_wake_up_pin_ctl(true);

#ifdef CONFIG_SENSOR_GPS_UART_DMA
	uart_ctrl_rx_dma_start((struct device *)data->uart_gpio_dev);
#else
	uart_irq_rx_enable(data->uart_gpio_dev);
#endif // CONFIG_SENSOR_GPS_UART_DMA
}

static void ag3352_disable(const struct device *dev)
{
	struct ag3352_data *data = (struct ag3352_data *)dev->data;
	// gps_wake_up_pin_ctl(false);
#ifdef CONFIG_SENSOR_GPS_UART_DMA
	uart_dma_receive_stop((struct device *)data->uart_gpio_dev);
#else
	uart_irq_rx_disable(data->uart_gpio_dev);
#endif // CONFIG_SENSOR_GPS_UART_DMA
}

static void ag3352_register_notify(const struct device *dev, gps_notify_t notify)
{
	LOG_INF("ag3352_init");
	struct ag3352_data *data = (struct ag3352_data *)dev->data;

	data->notify = notify;
}

static void ag3352_unregister_notify(const struct device *dev, gps_notify_t notify)
{
	struct ag3352_data *data = (struct ag3352_data *)dev->data;

	data->notify = NULL;
}

static const struct gps_dev_driver_api ag3352_api = {
	.enable = ag3352_enable,
	.disable = ag3352_disable,
	.inquiry = NULL,
	.register_notify = ag3352_register_notify,
	.unregister_notify = ag3352_unregister_notify,
};

static void _gps_work_handler(struct k_work *work)
{
	struct gps_value val = { 0 };

	//LOG_INF("gps nmea: %s\r\n", ag3352_data.gps_data[ag3352_data.gpa_data_prev]);
	if (ag3352_data.notify) {
#ifdef CONFIG_SENSOR_GPS_UART_DMA
#else
		val.gps_nmea_data = ag3352_data.gps_data[ag3352_data.gpa_data_prev];
#endif // CONFIG_SENSOR_GPS_UART_DMA
		ag3352_data.notify(NULL, &val);
	}
}

static int ag3352_init(const struct device *dev)
{
	struct ag3352_data *data = (struct ag3352_data *)dev->data;
	LOG_INF("ag3352_init");
	gps_power_pin_ctl(true);
	gps_power095_pin_ctl(true);
	gps_reset_pin_ctl();
	data->uart_gpio_dev = device_get_binding(GPS_UERT_DEV_NAME);
	if (data->uart_gpio_dev == NULL) {
		LOG_ERR("Couldn't find gps uart");
		return -ENODEV;
	}
	k_work_init(&data->gps_work, _gps_work_handler);
#ifdef CONFIG_SENSOR_GPS_UART_DMA
	uart_dma_receive_init((struct device *)data->uart_gpio_dev, uart_ctrl_rx_dma_handler, NULL);
	uart_rx_dma_switch((struct device *)data->uart_gpio_dev, TRUE, NULL, NULL);
#else
	uart_irq_callback_set(data->uart_gpio_dev, uart_fifo_callback);
	data->gpa_data_cur = 0;
#endif // CONFIG_SENSOR_GPS_UART_DMA
	return 0;
}

#if IS_ENABLED(CONFIG_GPS)
DEVICE_DEFINE(ag3352, CONFIG_GPS_DEV_NAME, &ag3352_init, NULL, &ag3352_data, NULL, POST_KERNEL, 60,
	      &ag3352_api);
#endif
