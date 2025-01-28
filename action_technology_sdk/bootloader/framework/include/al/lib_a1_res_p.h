/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __LIB_K1_RES_P_H__
#define __LIB_K1_RES_P_H__

/**
 * @brief upsample from 16/32 kHz to 48 kHz
 *
 * This routine provides upsampling from 16/32 kHz to 48 kHz
 *
 * @param input input pcm buffer of 16/32 kHz
 * @param output output pcm buffer of 48 kHz
 * @param len number of input pcm samples
 * @param down resample type. 1: 16kHz->48kHz; 2: 32kHz->48kHz
 * @param temp_up temporary buffer for resample library, whose
                  size should be "input buffer size + 52 * sizeof(short)"
 *
 * @return number of output pcm samples of 48 kHz
 */
int upsample3(short *input, short *output, int len, int down, short *temp_up);

/**
 * @brief upsample from 8 kHz to 48 kHz
 *
 * This routine provides upsampling from 8 kHz to 48 kHz
 *
 * @param input input pcm buffer of 8 kHz
 * @param output output pcm buffer of 48 kHz
 * @param len number of input pcm samples
 * @param temp_up temporary buffer for resample library, whose
                  size should be "input buffer size + 52 * sizeof(short)"
 *
 * @return number of output pcm samples of 48 kHz
 */
int upsample6(short *input, short *output, int len, short *temp_up);

#endif /* __LIB_K1_RES_P_H__ */
