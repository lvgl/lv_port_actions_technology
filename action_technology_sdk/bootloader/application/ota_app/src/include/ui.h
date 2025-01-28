/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file ui interface
 */

#ifndef _UI_H_
#define _UI_H_

void display_init(void);
void display_pic(void *pic_addr, int pic_width, int pic_height, uint32_t bg_color);

#endif
