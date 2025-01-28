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
#include <drivers/gpio.h>

LOG_MODULE_REGISTER(gps_1165, LOG_LEVEL_INF);
#define CONFIG_GPS_DEV_NAME	"gps"
#define GPS_UERT_DEV_NAME CONFIG_UART_2_NAME
#define UART_FIFO_MAX 16
#define GPS_DATA_BUFFER_SIZE 100

struct cc1165w_data {
    const struct device *uart_gpio_dev;
    uint8_t gps_data[2][GPS_DATA_BUFFER_SIZE];
    uint8_t gpa_data_cur;
    uint8_t gpa_data_prev;
    gps_notify_t notify;
    struct k_work gps_work;
};

static struct cc1165w_data cc1165w_data;

static void uart_fifo_callback(const struct device *dev, void *user_data)
{
    uint8_t rx_buff[UART_FIFO_MAX];
	int read_size;
    static uint8_t cnt;

    uart_irq_update(dev);
    if(uart_irq_rx_ready(dev))
	{
        read_size = uart_fifo_read(dev,rx_buff,UART_FIFO_MAX);

        uart_fifo_fill(dev,rx_buff,read_size);

        for(int i = 0; i < read_size; i++)
        {
            switch (rx_buff[i])
            {
                case '$':
                    if(cc1165w_data.gpa_data_cur)
                        cc1165w_data.gpa_data_cur = 0;
                    else
                        cc1165w_data.gpa_data_cur = 1;

                    memset(cc1165w_data.gps_data[cc1165w_data.gpa_data_cur], 0, sizeof(cc1165w_data.gps_data[cc1165w_data.gpa_data_cur]));
                    cnt = 0;
                    cc1165w_data.gps_data[cc1165w_data.gpa_data_cur][cnt] = rx_buff[i];
                    break;

                case '\n':
                    cnt ++;
                    cc1165w_data.gps_data[cc1165w_data.gpa_data_cur][cnt] = rx_buff[i];
                    cc1165w_data.gpa_data_prev = cc1165w_data.gpa_data_cur;
                    k_work_submit(&cc1165w_data.gps_work);
                    break;

                default:
                    cnt ++;
                    cc1165w_data.gps_data[cc1165w_data.gpa_data_cur][cnt] = rx_buff[i];

                    /* protection data cannot exceed boundary */
                    if(cnt >= GPS_DATA_BUFFER_SIZE)
                        cnt --;
                    break;
            }
        }
    }
}

static void cc1165w_enable(const struct device *dev)
{  
    struct cc1165w_data *data = (struct cc1165w_data *)dev->data;
    printk("cc1165w_enable %s\n",data->uart_gpio_dev->name);
    gps_wake_up_pin_ctl(true);
    uart_irq_rx_enable(data->uart_gpio_dev);
}

static void cc1165w_disable(const struct device *dev)
{
    printk("cc1165 disable \n");
    struct cc1165w_data *data = (struct cc1165w_data *)dev->data;
    gps_wake_up_pin_ctl(false);
    uart_irq_rx_disable(data->uart_gpio_dev);
}

static void cc1165w_register_notify(const struct device *dev, gps_notify_t notify)
{
	struct cc1165w_data *data = (struct cc1165w_data *)dev->data;

	data->notify = notify;
}

static void cc1165w_unregister_notify(const struct device *dev, gps_notify_t notify)
{
	struct cc1165w_data *data = (struct cc1165w_data *)dev->data;

	data->notify = NULL;
}

static const struct  gps_dev_driver_api cc1165w_api = {
	.enable = cc1165w_enable,
	.disable = cc1165w_disable,
    .inquiry = NULL,
	.register_notify = cc1165w_register_notify,
	.unregister_notify = cc1165w_unregister_notify,
};

//发送消息给到gps service
static void _gps_work_handler(struct k_work *work)
{
    struct gps_value val = {0};

    // LOG_INF("%s",cc1165w_data.gps_data[cc1165w_data.gpa_data_prev]);
    if (cc1165w_data.notify)
    {
        val.gps_nmea_data = cc1165w_data.gps_data[cc1165w_data.gpa_data_prev];
        cc1165w_data.notify(NULL, &val);
    }
}


static int cc1165w_init(const struct device *dev)
{
    struct cc1165w_data *data = (struct cc1165w_data *)dev->data;
    LOG_INF("cc1165w_init");
    gps_power_pin_ctl(true);
    gps_power095_pin_ctl(true);
    // gps_reset_pin_ctl();
    data->uart_gpio_dev = device_get_binding(GPS_UERT_DEV_NAME);
    if (data->uart_gpio_dev == NULL) {
        LOG_ERR("Couldn't find gps uart");
        return -ENODEV;
	}
    k_work_init(&data->gps_work, _gps_work_handler);
    printk("cc1165w_init*\n");
    uart_irq_callback_set(data->uart_gpio_dev, uart_fifo_callback);
    data->gpa_data_cur = 0;
    return 0;
}



DEVICE_DEFINE(cc1165w, CONFIG_GPS_DEV_NAME, &cc1165w_init,
			NULL, &cc1165w_data, NULL, POST_KERNEL,
			60, &cc1165w_api);

