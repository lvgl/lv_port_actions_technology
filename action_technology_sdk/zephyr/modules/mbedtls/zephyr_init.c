/** @file
 * @brief mbed TLS initialization
 *
 * Initialize the mbed TLS library like setup the heap etc.
 */

/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <init.h>
#include <app_memory/app_memdomain.h>

#if defined(CONFIG_MBEDTLS)
#if !defined(CONFIG_MBEDTLS_CFG_FILE)
#include "mbedtls/config.h"
#else
#include CONFIG_MBEDTLS_CFG_FILE
#endif /* CONFIG_MBEDTLS_CFG_FILE */
#endif

#if defined(CONFIG_MBEDTLS_ENABLE_HEAP) && \
					defined(MBEDTLS_MEMORY_BUFFER_ALLOC_C)
#include <mbedtls/memory_buffer_alloc.h>

#if !defined(CONFIG_MBEDTLS_HEAP_SIZE)
#error "Please set heap size to be used. Set value to CONFIG_MBEDTLS_HEAP_SIZE \
option."
#endif

static unsigned char _mbedtls_heap[CONFIG_MBEDTLS_HEAP_SIZE];

static void init_heap(void)
{
	mbedtls_memory_buffer_alloc_init(_mbedtls_heap, sizeof(_mbedtls_heap));
}
#else
#define init_heap(...)
#endif /* CONFIG_MBEDTLS_ENABLE_HEAP && MBEDTLS_MEMORY_BUFFER_ALLOC_C */

static int _mbedtls_init(const struct device *device)
{
	ARG_UNUSED(device);

	init_heap();

	return 0;
}

SYS_INIT(_mbedtls_init, POST_KERNEL, 0);

#ifdef MBEDTLS_ENTROPY_HARDWARE_ALT
static int os_get_random(unsigned char *buf, size_t len)
{
    size_t i, j;
    unsigned long tmp;

    for (i = 0; i < ((len + 3) & ~3) / 4; i++)
    {
        tmp = rand();

        for (j = 0; j < 4; j++)
        {
            if ((i * 4 + j) < len)
            {
                buf[i * 4 + j] = (unsigned char)(tmp >> (j * 8));
            } 
            else 
            {
                break;
            }
        }
    }

    return 0;
}

int mbedtls_hardware_poll( void *data, unsigned char *output, size_t len, size_t *olen )
{
    os_get_random(output, len);
    *olen = len;

    return 0;
}
#endif