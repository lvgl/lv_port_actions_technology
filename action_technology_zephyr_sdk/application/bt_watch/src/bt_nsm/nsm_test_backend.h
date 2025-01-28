/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief NSM test backend interface
 */

#ifndef __NSM_TEST_BACKEND_H__
#define __NSM_TEST_BACKEND_H__

#include "ota_backend_bt.h"
#include <thread_timer.h>
#include <sys/ring_buffer.h>

/** type of backend*/
#define NSM_TEST_BACKEND_TYPE_UNKNOWN       (0)
#define NSM_TEST_BACKEND_TYPE_BLUETOOTH     (1)

#define NSM_TEST_BACKEND_START_STATE        (1)
#define NSM_TEST_BACKEND_UPGRADE_PROGRESS   (2)

#define NSM_TX_RING_BUF_SIZE                (1024)
#define NSM_TRANSFER_TIMER_MS               (10)

typedef void (*nsm_test_notify_cb_t)(int cmd, int state);

struct nsm_test_backend_t
{
    /** backend type */
    int type;
    int sppble_stream_opened;
    nsm_test_notify_cb_t cb;
    io_stream_t sppble_stream;

    // struct thread_timer ttimer;
    // struct ring_buf tx_ringbuf;
    // uint8_t *tx_buff;
    // uint8_t *temp_buff;
};

void nsm_test_backend_init(void *init_param);
void nsm_test_backend_exit(void);
int nsm_test_backend_open(void);
int nsm_test_backend_close(void);

int nsm_test_backend_read(uint8_t *data, uint32_t length, uint32_t timeout_ms);
int nsm_test_backend_write(uint8_t *data, uint32_t length, uint32_t timeout_ms);
int nsm_test_backend_ioctl(int cmd, void *param0, void *param1);
#endif /* __NSM_TEST_BACKEND_H__ */
