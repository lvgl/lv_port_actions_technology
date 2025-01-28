/*
 * Copyright (c) 2018 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

 /**
 * @file Trace dump core
 *
 * NOTE: All Trace dump functions cannot be called in interrupt context.
 */

#ifndef _TRACEDUMP_CORE__H_
#define _TRACEDUMP_CORE__H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
// 1. tracedump structure
+========+=========+=========+=====+=========+
| header | trace 1 | trace 2 | ... | trace N |
+========+=========+=========+=====+=========+

// 2. trace info
+=========+========+========+========+=====+========+
| uniq id | data 1 | data 2 | func 1 | ... | func N |
+=========+========+========+========+=====+========+
**/

/* tracedump config */
#define TRACED_DATA_CNT         (2)
#define TRACED_FUNC_CNT         (9)

/* tracedump magic and version */
#define TRACED_MAGIC	        (0x44435254) /* TRACED (trace dump) */

/* trace info */
typedef struct traced_info_s {
	uint32_t uniq_id;                       /* uniq id */
	uint32_t data[TRACED_DATA_CNT];         /* user data */
	uint32_t func[TRACED_FUNC_CNT];         /* backtrace */
} traced_info_t;

typedef struct traced_data_s {
	uint32_t magic;         /* magic (file format) */
	uint16_t max_cnt;       /* max count for saving */
	uint16_t cur_cnt;       /* trace count for saving */
	uint16_t start_idx;     /* start index for saving */
	uint16_t end_idx;       /* end index for saving */
	uint32_t uniq_id;       /* uniq id for saving */
	uint32_t data_off;      /* data offset for filter */
	uint32_t data_sz;       /* data size for filter */
	atomic_t locked;        /* lock counter for saving*/
	uint8_t event_mask;     /* event mask for tracing */
	uint8_t drop_flag : 1;  /* drop flag */
	uint8_t  reserved[2];   /* reserved for extension */
} traced_data_t;

#ifdef __cplusplus
}
#endif

#endif /* _TRACEDUMP_CORE__H_ */

