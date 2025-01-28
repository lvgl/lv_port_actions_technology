/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#ifndef __MAS_FRAME_CHECK_H__
#define __MAS_FRAME_CHECK_H__

#include <mas_link.h>
#include <mas_link_char.h>
#include <mas_proto.h>
#include <mas_trans.h>

#define MAS_UART_FRAME_LEN_LEN 3
#define MAS_UART_FRAME_LEN_LEN 3

/* frame and packet total header */

#define MAS_UART_FRAME_HEAD_SIGN     (0xFC)
#define MAS_UART_FRAME_HEAD_SIGN_LEN (1)
#define MAS_UART_FRAME_END_SIGN      (0xCF)
#define MAS_UART_FRAME_END_SIGN_LEN  1

size_t frame_data_check(struct mas_link_char *link_char, size_t size);

mas_err_t check_crc16(uint8_t *buf, size_t begin, size_t end);
#endif
