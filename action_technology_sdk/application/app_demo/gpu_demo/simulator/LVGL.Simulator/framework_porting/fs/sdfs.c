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
#include <sdfs.h>

int sd_fmap(const char* filename, void** addr, int* len)
{
	FILE *fp = fopen(filename, "rb");
	if (fp == NULL) {
		printf("open file %s failed\n", filename);
		return -1;
	}

	fseek(fp, 0, SEEK_END);
	int size = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	*addr = malloc(size);
	if (*addr == NULL) {
		printf("malloc for file %s failed\n", filename);
		fclose(fp);
		return -2;
	}

    *len = size;

	fread(*addr, size, 1, fp);
	fclose(fp);
	return 0;
}

int sd_funmap(void* addr)
{
	if (addr)
		free(addr);

	return 0;
}
