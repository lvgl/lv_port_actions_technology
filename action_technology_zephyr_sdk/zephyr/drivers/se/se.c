/*
 * Copyright (c) 2017 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <init.h>
#include <device.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <drivers/se/se.h>

K_MUTEX_DEFINE(se_lock);

/****************************************************************************
 * Name: se_memcpy
 *
 * Description:
 *   copy data from src to dst according to memory type.
 *
 ****************************************************************************/

void se_memcpy(void *out, const void *in,
                   size_t size, uint8_t direct)
{
	size_t i;
	size_t wcount = size / 4;

	if (direct == CPY_MEMU8_TO_FIFO) {
		for (i = 0; i < size; i++) {
			*(uint32_t *)out = *(uint8_t *)in;
			in = (uint8_t *)in + 1;
		}
	}
	else if (direct == CPY_MEM_TO_FIFO) {
		for (i = 0; i < wcount; i++) {
			*(uint32_t *)out = *(uint32_t *)in;
			in = (uint8_t *)in + 4;
		}
	} else if (direct == CPY_FIFO_TO_MEM) {
		for (i = 0; i < wcount; i++) {
			*(uint32_t *)out = *(uint32_t *)in;
			out = (uint8_t *)out + 4;
		}
	} else {
		for (i = 0; i < wcount; i++) {
			*(uint32_t *)out = *(uint32_t *)in;
			in  = (uint8_t *)in + 4;
			out = (uint8_t *)out + 4;
		}
	}
}

