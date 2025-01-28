/*
 * Copyright (c) 2022 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <soc.h>
#include <board.h>
#include <device.h>
#include <drivers/uart.h>
#include <logging/log.h>
#include <gps/gps.h>

LOG_MODULE_REGISTER(gps_3352, LOG_LEVEL_INF);
#define CONFIG_GPS_DEV_NAME	"gps"
#define GPS_UERT_DEV_NAME CONFIG_UART_2_NAME
#define UART_FIFO_MAX 16
#define GPS_DATA_BUFFER_SIZE 100

struct ag3352_data {
    const struct device *uart_gpio_dev;
    uint8_t gps_data[2][GPS_DATA_BUFFER_SIZE];
    uint8_t gpa_data_cur;
    uint8_t gpa_data_prev;
    gps_notify_t notify;
    struct k_work gps_work;
};

static struct ag3352_data ag3352_data;

static void uart_fifo_callback(const struct device *dev, void *user_data)
{
    uint8_t rx_buff[UART_FIFO_MAX];
	int read_size;
    static uint8_t cnt;

    uart_irq_update(dev);
    if(uart_irq_rx_ready(dev))
	{
        read_size = uart_fifo_read(dev,rx_buff,UART_FIFO_MAX);

        for(int i = 0; i < read_size; i++)
        {
            switch (rx_buff[i])
            {
                case '$':
                    if(ag3352_data.gpa_data_cur)
                        ag3352_data.gpa_data_cur = 0;
                    else
                        ag3352_data.gpa_data_cur = 1;

                    memset(ag3352_data.gps_data[ag3352_data.gpa_data_cur], 0, sizeof(ag3352_data.gps_data[ag3352_data.gpa_data_cur]));
                    cnt = 0;
                    ag3352_data.gps_data[ag3352_data.gpa_data_cur][cnt] = rx_buff[i];
                    break;

                case '\n':
                    cnt ++;
                    ag3352_data.gps_data[ag3352_data.gpa_data_cur][cnt] = rx_buff[i];
                    ag3352_data.gpa_data_prev = ag3352_data.gpa_data_cur;
                    k_work_submit(&ag3352_data.gps_work);
                    break;

                default:
                    cnt ++;
                    ag3352_data.gps_data[ag3352_data.gpa_data_cur][cnt] = rx_buff[i];

                    /* protection data cannot exceed boundary */
                    if(cnt >= GPS_DATA_BUFFER_SIZE)
                        cnt --;
                    break;
            }
        }
    }
}

static void ag3352_enable(const struct device *dev)
{
    struct ag3352_data *data = (struct ag3352_data *)dev->data;
    gps_wake_up_pin_ctl(true);
    uart_irq_rx_enable(data->uart_gpio_dev);
}

static void ag3352_disable(const struct device *dev)
{
    struct ag3352_data *data = (struct ag3352_data *)dev->data;
    gps_wake_up_pin_ctl(false);
    uart_irq_rx_disable(data->uart_gpio_dev);
}

static void ag3352_register_notify(const struct device *dev, gps_notify_t notify)
{
	struct ag3352_data *data = (struct ag3352_data *)dev->data;

	data->notify = notify;
}

static void ag3352_unregister_notify(const struct device *dev, gps_notify_t notify)
{
	struct ag3352_data *data = (struct ag3352_data *)dev->data;

	data->notify = NULL;
}

static const struct  gps_dev_driver_api ag3352_api = {
	.enable = ag3352_enable,
	.disable = ag3352_disable,
    .inquiry = NULL,
	.register_notify = ag3352_register_notify,
	.unregister_notify = ag3352_unregister_notify,
};

static void _gps_work_handler(struct k_work *work)
{
    struct gps_value val = {0};

    // LOG_INF("%s",ag3352_data.gps_data[ag3352_data.gpa_data_prev]);
    if (ag3352_data.notify)
    {
        val.gps_nmea_data = ag3352_data.gps_data[ag3352_data.gpa_data_prev];
        ag3352_data.notify(NULL, &val);
    }
}


static int ag3352_init(const struct device *dev)
{
    struct ag3352_data *data = (struct ag3352_data *)dev->data;
    LOG_INF("ag3352_init");
    gps_power_pin_ctl(true);
    gps_power095_pin_ctl(true);
    // gps_reset_pin_ctl();
    data->uart_gpio_dev = device_get_binding(GPS_UERT_DEV_NAME);
    if (data->uart_gpio_dev == NULL) {
        LOG_ERR("Couldn't find gps uart");
        return -ENODEV;
	}
    k_work_init(&data->gps_work, _gps_work_handler);

    uart_irq_callback_set(data->uart_gpio_dev, uart_fifo_callback);
    data->gpa_data_cur = 0;
    return 0;
}



DEVICE_DEFINE(ag3352, CONFIG_GPS_DEV_NAME, &ag3352_init,
			NULL, &ag3352_data, NULL, POST_KERNEL,
			60, &ag3352_api);

