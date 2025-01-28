/*
 * Copyright (c) 2019 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief rbuf uart.
 */

#define RBUF_STRUCT_SIZE				256
#define RBUF_UART_LEN					(RBUF_STRUCT_SIZE - 8)

#define SHARERAM_BASE_ADDR				0x2FF18000
//#define SHARERAM_BASE_ADDR		shareram_802_m4f_base

#define CPU_TO_BT_ADDR					(SHARERAM_BASE_ADDR + 1024)
#define BT_TO_CPU_ADDR					(SHARERAM_BASE_ADDR + 1024 + RBUF_STRUCT_SIZE)

struct rbuf_uart {
	unsigned short head;
	unsigned short tail;
	unsigned short size;
	unsigned short rfu;
	unsigned char buf[RBUF_UART_LEN];
};

static void rbuf_uart_init(unsigned int rbuf_addr)
{
	struct rbuf_uart *u_rbuf = (struct rbuf_uart *)rbuf_addr;

	u_rbuf->head = 0;
	u_rbuf->tail = 0;
	u_rbuf->size = RBUF_UART_LEN;
}

static int rbuf_uart_write(unsigned int rbuf_addr, unsigned char value)
{
	struct rbuf_uart *u_rbuf = (struct rbuf_uart *)rbuf_addr;

	if ((u_rbuf->tail + 1) >= u_rbuf->size) {
		if (((u_rbuf->tail + 1) - u_rbuf->size) == u_rbuf->head) {
			return -1;
		}
	} else {
		if ((u_rbuf->tail + 1) == u_rbuf->head) {
			return -1;
		}
	}

	u_rbuf->buf[u_rbuf->tail] = value;
	if ((u_rbuf->tail + 1) >= u_rbuf->size) {
		u_rbuf->tail = 0;
	} else {
		u_rbuf->tail += 1;
	}

	return 0;
}

static unsigned char rbuf_uart_read(unsigned int rbuf_addr)
{
	unsigned char value;
	struct rbuf_uart *u_rbuf = (struct rbuf_uart *)rbuf_addr;

	while (u_rbuf->head == u_rbuf->tail);
	value = u_rbuf->buf[u_rbuf->head];

	if ((u_rbuf->head + 1) >= u_rbuf->size) {
		u_rbuf->head = 0;
	} else {
		u_rbuf->head += 1;
	}

	return value;
}

static int rbuf_uart_read_no_wait(unsigned int rbuf_addr, unsigned char *value)
{
	struct rbuf_uart *u_rbuf = (struct rbuf_uart *)rbuf_addr;

	if (u_rbuf->head == u_rbuf->tail) {
		return -1;
	}

	*value = u_rbuf->buf[u_rbuf->head];
	if ((u_rbuf->head + 1) >= u_rbuf->size) {
		u_rbuf->head = 0;
	} else {
		u_rbuf->head += 1;
	}

	return 0;
}
