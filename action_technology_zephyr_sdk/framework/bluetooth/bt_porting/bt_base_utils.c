/** @file common_internel.c
 * @brief Bluetooth base function.
 *
 * Copyright (c) 2019 Actions Semi Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr.h>
#include <stdlib.h>
#include <stdio.h>
#include "bt_porting_inner.h"

#define STACK_PRINT_BUF_LEN		64
static char print_buf[STACK_PRINT_BUF_LEN];
static uint32_t bt_rand32;

static int stack_print_hex_space(const char *prefix, const void *data, int size, bool space)
{
	int  i = 0, j = 0;
	char c1, c2, *sbuf = print_buf;

	if (prefix != NULL) {
		while ((sbuf[j] = prefix[j]) != '\0' && (j + 6) < STACK_PRINT_BUF_LEN) {
			j++;
		}
	}

	for (i = 0; i < size; i++) {
		c1 = ((uint8_t *)data)[i] >> 4;
		c2 = ((uint8_t *)data)[i] & 0x0F;
		if (space) {
			sbuf[j++] = ' ';
		}
		sbuf[j++] = (c1 >= 10) ? (c1 + 'a' - 10) : (c1 + '0');
		sbuf[j++] = (c2 >= 10) ? (c2 + 'a' - 10) : (c2 + '0');

		if (((i + 1) % 16) == 0 || (j + 6) >= STACK_PRINT_BUF_LEN) {
			sbuf[j] = '\0';
			j = 0;
			printk("%s\n", sbuf);
		}
	}

	if (j > 0) {
		sbuf[j] = '\0';
		printk("%s\n", sbuf);
	}

	return i;
}

int stack_print_hex(const char *prefix, const void *data, int size)
{
	return stack_print_hex_space(prefix, data, size, true);
}

int stack_print_linkkey(const char *prefix, const void *data, int size)
{
	return stack_print_hex_space(prefix, data, size, false);
}

int stack_print_char(const char *prefix, const char *data, int size)
{
	int  i = 0, j = 0;
	char *sbuf = print_buf;

	if (prefix != NULL) {
		while ((sbuf[j] = prefix[j]) != '\0' && (j + 6) < STACK_PRINT_BUF_LEN) {
			j++;
		}
	}

	for (i = 0; i < size; i++) {
		sbuf[j++] = ((uint8_t *)data)[i];
		if (j > (STACK_PRINT_BUF_LEN - 2)) {
			sbuf[j] = '\0';
			j = 0;
			printk("%s\n", sbuf);
		}
	}

	if (j > 0) {
		sbuf[j] = '\0';
		printk("%s\n", sbuf);
	}

	return i;
}

void bt_rand32_set_seed(uint32_t seed)
{
	bt_rand32 = bt_rand32*131 + seed;
}

uint32_t bt_rand32_get(void)
{
	int i;

	if (bt_rand32) {
		bt_rand32 = bt_rand32*131 + bt_get_sys_cycle();
	} else {
		for (i=0; i<10; i++) {
			bt_rand32 = bt_rand32*131 + bt_get_sys_cycle();
		}
	}

	return bt_rand32;
}
