/*
 * Copyright (c) 2018 Intel Corporation.
 * Copyright (c) 2020 Peter Bigot Consulting, LLC
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __SDFS_H__
#define __SDFS_H__

int sd_fmap(const char* filename, void** addr, int* len);
int sd_funmap(void* addr);

#endif /* __SDFS_H__ */

