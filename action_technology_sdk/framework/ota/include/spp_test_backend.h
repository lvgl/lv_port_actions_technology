/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief SPP test backend interface
 */

#ifndef __SPP_TEST_BACKEND_H__
#define __SPP_TEST_BACKEND_H__

#include "ota_backend_bt.h"
#include <thread_timer.h>
#include <sys/ring_buffer.h>

/** type of backend*/
#define SPP_TEST_BACKEND_TYPE_UNKNOWN       (0)
#define SPP_TEST_BACKEND_TYPE_BLUETOOTH     (1)

#define SPP_TEST_BACKEND_START_STATE        (1)
#define SPP_TEST_BACKEND_UPGRADE_PROGRESS   (2)

#define SPP_TX_RING_BUF_SIZE                (1024)
#define SPP_TRANSFER_TIMER_MS               (10)

typedef void (*spp_test_notify_cb_t)(int cmd, int state);

struct spp_test_backend_t
{
	/** backend type */
	int type;
	int sppble_stream_opened;
	spp_test_notify_cb_t cb;
    io_stream_t sppble_stream;

    // struct thread_timer ttimer;
    // struct ring_buf tx_ringbuf;
    // uint8_t *tx_buff;
    // uint8_t *temp_buff;
};

void spp_test_backend_init(void *init_param);
void spp_test_backend_exit(void);
int spp_test_backend_open(void);
int spp_test_backend_close(void);

int spp_test_backend_read(uint8_t *data, uint32_t length, uint32_t timeout_ms);
int spp_test_backend_write(uint8_t *data, uint32_t length, uint32_t timeout_ms);
int spp_test_backend_ioctl(int cmd, void *param0, void *param1);
#endif /* __SPP_TEST_BACKEND_H__ */
