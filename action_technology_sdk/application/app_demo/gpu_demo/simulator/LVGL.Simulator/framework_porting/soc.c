/*
 * Copyright (c) 2018 Intel Corporation.
 * Copyright (c) 2020 Peter Bigot Consulting, LLC
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>


bool buf_is_psram(int buff)
{
	return false;
}

bool buf_is_psram_un(int buff)
{
	return false;
}

void* cache_to_uncache(void* vaddr)
{
    return NULL;
}

void* uncache_to_cache(void* paddr)
{
    return NULL;
}

int sys_cpu_to_le32(uint32_t value)
{

}

unsigned int find_lsb_set(uint32_t op)
{

}
