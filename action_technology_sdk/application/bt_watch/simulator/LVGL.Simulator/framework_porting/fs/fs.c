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
#include <fs/fs.h>

int fs_open(struct fs_file_t *zfp, const char *file_name, int flags)
{
	zfp->filep = fopen(file_name, "rb");
	if(zfp->filep == NULL)
	{
		printf("open file %s failed\n", file_name);
		return -1;
	}
	
	return 0;
}

int fs_close(struct fs_file_t* zfp)
{
    return fclose(zfp->filep);
}

int fs_seek(struct fs_file_t* zfp, uint32_t offset, int whence)
{
    return fseek(zfp->filep, offset, whence);
}

uint32_t fs_tell(struct fs_file_t* zfp)
{
    return ftell(zfp->filep);
}

size_t fs_read(struct fs_file_t* zfp, void* ptr, size_t size)
{
    return fread(ptr, 1, size, zfp->filep);
}
