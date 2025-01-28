/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief OTA bluetooth NSM backend interface
 */

#include <kernel.h>
#include <string.h>
#include <stream.h>
#include <soc.h>
#include <mem_manager.h>
#include "nsm_test_backend.h"
#include <bt_manager.h>


static struct nsm_test_backend_t *g_test_backend;
/*
static void ttimer_handler(struct thread_timer *ttimer, void *expiry_fn_arg)
{
    int count, ret;

    count = ring_buf_get(&g_test_backend->tx_ringbuf, g_test_backend->temp_buff, NSM_TX_RING_BUF_SIZE / 2);
    ret = stream_write(g_test_backend->nsmble_stream, g_test_backend->temp_buff, count);
    if (count != ret)
    {
        SYS_LOG_ERR("NSM tx fail %d(%d)", ret, count);
    }

    thread_timer_start(ttimer, NSM_TRANSFER_TIMER_MS, 0);
}
*/
void nsm_test_backend_init(void *init_param)
{
    g_test_backend->sppble_stream = sppble_stream_create(init_param);
    if (!g_test_backend->sppble_stream) {
        SYS_LOG_ERR("stream_create failed");
    }
}

void nsm_test_backend_exit(void)
{
    SYS_LOG_DBG("");

    if (g_test_backend->sppble_stream) {
        stream_destroy(g_test_backend->sppble_stream);
    }

    mem_free(g_test_backend);
    /* avoid connect again after exit */
    g_test_backend = NULL;
}

int nsm_test_backend_open(void)
{
    int err = 0;
    SYS_LOG_DBG("open nsmble_stream");

    if (!g_test_backend->sppble_stream_opened)
    {
        err = stream_open(g_test_backend->sppble_stream, MODE_IN_OUT);
        if (err)
        {
            SYS_LOG_ERR("stream_open Failed");
        }
        else
        {
            g_test_backend->sppble_stream_opened = 1;

            // g_test_backend->tx_buff = mem_malloc(NSM_TX_RING_BUF_SIZE);
            // g_test_backend->temp_buff = mem_malloc(NSM_TX_RING_BUF_SIZE / 2);
            // ring_buf_init(&g_test_backend->tx_ringbuf, NSM_TX_RING_BUF_SIZE, g_test_backend->tx_buff);
            // thread_timer_init(&g_test_backend->ttimer, ttimer_handler, NULL);
            // thread_timer_start(&g_test_backend->ttimer, NSM_TRANSFER_TIMER_MS, 0);
        }
    }

    return err;
}

int nsm_test_backend_close(void)
{
    int err = 0;
    SYS_LOG_DBG("close nsmble_stream");

    if (g_test_backend->sppble_stream_opened)
    {
        err = stream_close(g_test_backend->sppble_stream);
        if (err)
        {
            SYS_LOG_ERR("stream_close Failed");
        }
        else
        {
            g_test_backend->sppble_stream_opened = 0;
            // thread_timer_stop(&g_test_backend->ttimer);
            // mem_free(g_test_backend->tx_buff);
            // mem_free(g_test_backend->temp_buff);
        }
    }

    return err;
}

int nsm_test_backend_read(uint8_t *buf, uint32_t size, uint32_t timeout_ms)
{
    int read_size, total_size = size;
    uint32_t prev_uptime, cur_uptime;
    uint8_t c;

    if (NULL == buf)
    {
        read_size = stream_tell(g_test_backend->sppble_stream);
        if (size > read_size)
        {
            size = read_size;
        }

        total_size = size;
        for (; 0 != size; size--)
        {
            stream_read(g_test_backend->sppble_stream, &c, 1);
        }
    }
    else
    {
        prev_uptime = k_uptime_get_32();
        while (size > 0) {
            read_size = stream_read(g_test_backend->sppble_stream, buf, size);
            if (read_size > 0)
            {
                size -= read_size;
                buf += read_size;
            }

            cur_uptime = k_uptime_get_32();
            if (cur_uptime - prev_uptime >= timeout_ms)
                break;

            os_sleep(1);
        }
    }

    if (size > 0)
    {
        read_size = total_size - size;
        SYS_LOG_DBG("need read %d bytes, but only got %d bytes", total_size, read_size);
        total_size = read_size;
    }
    return total_size;
}

int nsm_test_backend_write(uint8_t *buf, uint32_t size, uint32_t timeout_ms)
{
    int write_size, total_size = size;
    uint32_t prev_uptime, cur_uptime;

    prev_uptime = k_uptime_get_32();
    while (size > 0)
    {
        write_size = stream_write(g_test_backend->sppble_stream, buf, size);
        if (write_size > 0)
        {
            size -= write_size;
            buf += write_size;
        }

        cur_uptime = k_uptime_get_32();
        if (cur_uptime - prev_uptime >= timeout_ms)
            break;

        os_sleep(1);
    }

    if (size > 0)
    {
        write_size = total_size - size;
        SYS_LOG_ERR("write %d bytes, discard %d bytes", write_size, total_size - write_size);
        total_size = write_size;
    }
    return total_size;
}

int nsm_test_backend_ioctl(int cmd, void *param0, void *param1)
{
    return 0;
}

static void nsm_test_connect_cb(int connected, uint8_t connect_type)
{
    SYS_LOG_INF("connect: %d", connected);

    /* avoid connect again after exit */
    if (g_test_backend) {
        if (g_test_backend->cb) {
            if (connected)
            {
                nsm_test_backend_open();
            }
            g_test_backend->cb(NSM_TEST_BACKEND_START_STATE, connected);

            if (!connected)
            {
                nsm_test_backend_close();
            }

        }
    }
}

bool nsm_test_backend_load_bt_init(nsm_test_notify_cb_t cb, struct ota_backend_bt_init_param *param)
{
    struct sppble_stream_init_param init_param;

    if (!g_test_backend)
    {
        g_test_backend = mem_malloc(sizeof(struct nsm_test_backend_t));
        if (!g_test_backend) {
            SYS_LOG_ERR("malloc failed");
            return false;
        }
    }

    memset(g_test_backend, 0x0, sizeof(struct nsm_test_backend_t));
    g_test_backend->type = OTA_BACKEND_TYPE_BLUETOOTH;
    g_test_backend->cb = cb;

    memset(&init_param, 0x0, sizeof(struct sppble_stream_init_param));
    init_param.spp_uuid = (uint8_t *)param->spp_uuid;
    init_param.gatt_attr = param->gatt_attr;
    init_param.attr_size = param->attr_size;
    init_param.tx_chrc_attr = param->tx_chrc_attr;
    init_param.tx_attr = param->tx_attr;
    init_param.tx_ccc_attr = param->tx_ccc_attr;
    init_param.rx_attr = param->rx_attr;
    init_param.connect_cb = nsm_test_connect_cb;
    init_param.read_timeout = param->read_timeout;  /* K_FOREVER, K_NO_WAIT,  K_MSEC(ms) */
    init_param.write_timeout = param->write_timeout;

    /* Just call stream_create once, for register nsm/ble service
     * not need call stream_destroy
     */
    nsm_test_backend_init(&init_param);

    return true;
}

