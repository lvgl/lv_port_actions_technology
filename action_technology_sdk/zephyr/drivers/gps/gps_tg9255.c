/*
 * Copyright (c) 2022 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <soc.h>
#include <board.h>
#include <device.h>
#include <drivers/gpio.h>
#include <drivers/uart.h>
#include <logging/log.h>
#include <gps/gps.h>

LOG_MODULE_REGISTER(gps_9255, LOG_LEVEL_INF);

#define GPS_UART_DEV_NAME CONFIG_UART_2_NAME
#define UART_FIFO_MAX 16
#define GPS_DATA_BUFFER_SIZE 100

struct tg9255_data {
    const struct device *gpio_dev;
    const struct device *uart_dev;
    uint8_t gps_data[2][GPS_DATA_BUFFER_SIZE];
    uint8_t gpa_data_cur;
    uint8_t gpa_data_prev;
    gps_notify_t notify;
    struct k_work gps_work;
};

static const struct gpio_cfg power_gpio_cfg = CONFIG_GPS_POWER_GPIO;
static struct tg9255_data tg9255_data;

static void uart_fifo_callback(const struct device *dev, void *user_data)
{
    uint8_t rx_buff[UART_FIFO_MAX];
	int read_size;
    static uint8_t cnt;

    uart_irq_update(dev);
    if(uart_irq_rx_ready(dev))
	{
        read_size = uart_fifo_read(dev,rx_buff,UART_FIFO_MAX);
        if(read_size == UART_FIFO_MAX)
            LOG_ERR("uart fifo buffer overflow");

        for(int i = 0; i < read_size; i++)
        {
            switch (rx_buff[i])
            {
                case '$':
                    if(tg9255_data.gpa_data_cur)
                        tg9255_data.gpa_data_cur = 0;
                    else
                        tg9255_data.gpa_data_cur = 1;

                    memset(tg9255_data.gps_data[tg9255_data.gpa_data_cur], 0, sizeof(tg9255_data.gps_data[tg9255_data.gpa_data_cur]));
                    cnt = 0;
                    tg9255_data.gps_data[tg9255_data.gpa_data_cur][cnt] = rx_buff[i];
                    break;

                case '\n':
                    cnt ++;
                    tg9255_data.gps_data[tg9255_data.gpa_data_cur][cnt] = rx_buff[i];
                    tg9255_data.gpa_data_prev = tg9255_data.gpa_data_cur;
                    k_work_submit(&tg9255_data.gps_work);
                    break;

                default:
                    cnt ++;
                    tg9255_data.gps_data[tg9255_data.gpa_data_cur][cnt] = rx_buff[i];

                    /* protection data cannot exceed boundary */
                    if(cnt >= GPS_DATA_BUFFER_SIZE)
                        cnt --;
                    break;
            }
        }
    }
}

static void tg9255_enable(const struct device *dev)
{
    struct tg9255_data *data = (struct tg9255_data *)dev->data;
    gpio_pin_set(data->gpio_dev, power_gpio_cfg.gpion, 1);
    uart_irq_rx_enable(data->uart_dev);
}

static void tg9255_disable(const struct device *dev)
{
    struct tg9255_data *data = (struct tg9255_data *)dev->data;
    uart_irq_rx_disable(data->uart_dev);
    gpio_pin_set(data->gpio_dev, power_gpio_cfg.gpion, 0);
}

static void tg9255_register_notify(const struct device *dev, gps_notify_t notify)
{
	struct tg9255_data *data = (struct tg9255_data *)dev->data;

	data->notify = notify;
}

static void tg9255_unregister_notify(const struct device *dev, gps_notify_t notify)
{
	struct tg9255_data *data = (struct tg9255_data *)dev->data;

	data->notify = NULL;
}

static const struct  gps_dev_driver_api tg9255_api = {
	.enable = tg9255_enable,
	.disable = tg9255_disable,
    .inquiry = NULL,
	.register_notify = tg9255_register_notify,
	.unregister_notify = tg9255_unregister_notify,
};

static void _gps_work_handler(struct k_work *work)
{
    struct gps_value val = {0};

    // LOG_INF("%s",tg9255_data.gps_data[tg9255_data.gpa_data_prev]);
    if (tg9255_data.notify)
    {
        val.gps_nmea_data = tg9255_data.gps_data[tg9255_data.gpa_data_prev];
        tg9255_data.notify(NULL, &val);
    }
}


static int tg9255_init(const struct device *dev)
{
    struct tg9255_data *data = (struct tg9255_data *)dev->data;

    LOG_INF("tg9255_init");
    data->gpio_dev = device_get_binding(power_gpio_cfg.gpio_dev_name);
    if (data->gpio_dev == NULL) {
        LOG_ERR("Couldn't find gpio dev");
        return -ENODEV;
    }
    gpio_pin_configure(data->gpio_dev, power_gpio_cfg.gpion, GPIO_OUTPUT);
    gpio_pin_set(data->gpio_dev, power_gpio_cfg.gpion, 0);

    data->uart_dev = device_get_binding(GPS_UART_DEV_NAME);
    if (data->uart_dev == NULL) {
        LOG_ERR("Couldn't find gps uart");
        return -ENODEV;
	}
    k_work_init(&data->gps_work, _gps_work_handler);

    uart_irq_callback_set(data->uart_dev, uart_fifo_callback);
    data->gpa_data_cur = 0;
    return 0;
}



DEVICE_DEFINE(tg9255, CONFIG_GPS_DEV_NAME, &tg9255_init,
			NULL, &tg9255_data, NULL, POST_KERNEL,
			60, &tg9255_api);

