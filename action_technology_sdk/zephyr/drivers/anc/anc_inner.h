/*
 * Copyright (c) 1997-2015, Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __ANC_INNER_H__
#define __ANC_INNER_H__

#include <kernel.h>
#include <stdint.h>
#include <soc.h>
#include "drivers/anc.h"
#include <acts_ringbuf.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ANC_COMMAND_FRAME_SIZE 0x1A0



struct anc_acts_data {
	/* power status */
	int pm_status;

	/* message semaphore */
	struct k_sem sem;
	struct k_mutex mutex;

	/* one is decoder, the other is digital audio effect */
	struct anc_imageinfo images;
	/* image bootargs */
	struct anc_bootargs bootargs;
};

struct anc_acts_config {
	int reserve;
};

struct anc_acts_command{
    uint32_t id;
    uint32_t data_size;
};

typedef struct ack_buf{
	uint32_t cmd;
}ack_buf_t;


int anc_wait_hw_idle_timeout(int usec_to_wait);

int anc_acts_request_image(struct device *dev, const struct anc_imageinfo *image);
int anc_acts_release_image(struct device *dev);

#ifdef __cplusplus
}
#endif

#endif /* __ANC_INNER_H__ */
