/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __DLHEAP_H_
#define __DLHEAP_H_

#include <stdint.h>

void dlheap_init(void);
void *dlheap_malloc(uint32_t size);
void dlheap_free(void *ptr);
void dlheap_dump(void);

#endif
