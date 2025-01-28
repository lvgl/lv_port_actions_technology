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

typedef struct fs_file_t
{
    struct fs_file_t *filep;
}_fs_file_t;

int fs_open(struct fs_file_t *zfp, const char *file_name, int flags);

int fs_close(struct fs_file_t* zfp);

int fs_seek(struct fs_file_t* zfp, uint32_t offset, int whence);

uint32_t fs_tell(struct fs_file_t* zfp);

size_t fs_read(struct fs_file_t* zfp, void* ptr, size_t size);

int sd_fmap(const char* filename, void** addr, int* len);
