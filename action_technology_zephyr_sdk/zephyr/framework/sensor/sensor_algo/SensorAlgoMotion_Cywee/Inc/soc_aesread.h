/*
 * Copyright (c) 2021 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef	_ACTIONS_SOC_ATP_H_
#define	_ACTIONS_SOC_ATP_H_

#ifndef _ASMLANGUAGE

typedef struct{
    uint32_t data[4];
} GUID;

/**
 * @brief read aes key from efuse region
 *
 * This function is used to obtain AES key from efuse region
 *
 * @param   mgica: read magic
			key_index: AES key index:0~3
 *    		value: AES key value
 * @return 0 if invoked succsess.
 * @return other value if invoked failed.
 */
extern int hal_aes_key_read(uint32_t magic, int key_index, int *value);

/**
 * @brief get IC GUID data
 *
 * This function is used to obtain GUID data
 *
 * @param   magic: read magic
			g: GUID data pointer
 * @return 0 if invoked succsess.
 * @return other value if invoked failed.
 */
extern int hal_GUID_read(uint32_t magic, GUID *g);

/**
 * @brief get IC type data
 *
 * This function is used to obtain ictype data
 *
 * @param   magic: read magic
			g: ictype data pointer
 * @return 0 if invoked succsess.
 * @return other value if invoked failed.
 */
extern int hal_ictype_read(uint32_t magic, uint32_t *ic_type);

#endif /* _ASMLANGUAGE */

#endif /* _ACTIONS_SOC_ATP_H_	*/
