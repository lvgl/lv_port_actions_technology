/*
 * Copyright (c) 2018 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

 /**
 * @file Ram dump core
 *
 * NOTE: All Ram dump functions cannot be called in interrupt context.
 */

#ifndef _RAMDUMP_CORE__H_
#define _RAMDUMP_CORE__H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
// 1. ramdump structure
+===========+===============+===============+
| ramd_head | ramd_region 1 | ramd_region 2 | ...
+===========+===============+===============+

// 2. ramd_region
+=============+================+================+
| region_head | compr_stream 1 | compr_stream 2 | ...
+=============+================+================+

// 3. compr_stream
+============+=============+
| compr_head | compr block | ...
+============+=============+
*/

/* ramdump version */
#define RAMD_VERSION	(0x0001)    /* ramdump version */

/* file magic */
enum file_magic {
	MAGIC_RAMD = 0x444d4152,    /* RAMD (ram dump) */
	MAGIC_RAMR = 0x524d4152,    /* RAMR (ram region) */
	MAGIC_LZ4 = 0x20345a4c,     /* LZ4 (lz4) */
	MAGIC_FLZ = 0x205a4c46,     /* FLZ (fast-lz) */
};

/* file type */
enum file_type {
	TYPE_ADDR = 0,           /* memory/register with address */
	TYPE_MCPU_DBG = 1,       /* main cpu debug info */
	TYPE_BTCPU_DBG = 2,      /* bt cpu debug info */
	TYPE_TRACEDUMP_DBG = 3,  /* trace dump info */
};

/* cpu part type */
enum cpu_type {
	CORTEX_M4 = 0xc24,      /* cortex-m4 */
	CORTEX_M33 = 0x132,     /* cortex-m33 star */
};

/* ramd header */
typedef struct ramd_head_s {
	uint32_t magic;         /* magic (file format) */
	uint16_t version;       /* file version */
	uint16_t hdr_sz;        /* header size */
	uint32_t img_sz;        /* Size of image body (bytes). */
	uint32_t org_sz;        /* Size of origin data (bytes) */
	uint8_t  datetime[16];  /* date & time (yymmdd-hhmmss) */
	uint16_t inc_uid;       /* unique id (auto increment) */
	uint16_t cpu_id;        /* cpu id (cortex-m4/cortex-m33) */
	uint8_t  reserved[4];   /* reserved for extension */
	uint32_t img_chksum;    /* image body checksum */
	uint32_t hdr_chksum;    /* header checksum */
} ramd_head_t;

/* ramd region */
typedef struct ramd_region_s {
	uint32_t magic;         /* magic (file format) */
	uint16_t type;          /* file type */
	uint16_t hdr_sz;        /* header size */
	uint32_t img_sz;        /* Size of image body (bytes). */
	uint32_t org_sz;        /* Size of origin data (bytes) */
	uint32_t address;       /* memory/function address */
	uint8_t  reserved[12];  /* reserved for extension */
} ramd_region_t;

/* compressed stream header */
typedef struct compr_head_s {
	uint32_t magic;     /* magic (file format) */
	uint32_t hdr_size;  /* Size of header (bytes) */
	uint32_t img_sz;    /* Size of image body (bytes). */
	uint32_t org_sz;    /* Size of origin data (bytes) */
} compr_head_t;

/* mcpu dbg info */
typedef struct mcpu_dbg_s {
	uint32_t r[16];         /* R0~R15 */
	uint32_t xpsr;          /* XPSR */
	uint32_t msp;           /* main stack pointer */
	uint32_t psp;           /* process stack pointer */
	uint32_t exc_ret;       /* EXC_RETURN: bit2-SPSEL, bit4-frame type */
	uint32_t control;       /* CONTROL: bit1-SPSEL, bit2-FPCA */
	uint32_t reserved[11];  /* reserved for extension */
} mcpu_dbg_t;

/* btcpu dbg info */
typedef struct btcpu_dbg_s {
	uint8_t bt_info[128];  /* btcpu debug info (128bytes) */
} btcpu_dbg_t;

/* address info */
typedef struct ramd_addr_s {
	uintptr_t	start;      /* start address */
	uintptr_t	next;       /* next address */
	uint32_t	filter;     /* filter type */
} ramd_addr_t;

#define RAMD_COMPR_BLK_SZ		(32*1024)
#define RAMD_HEADER_BLK_SZ		(64*1024)
#define RAMD_ERASE_BLK_SZ		(64*1024)
#define RAMD_XFER_BLK_SZ		(512)
#define RAMD_SECTOR_SZ			(512)

#define RAMD_AHB_START			(0x40000000)

extern const ramd_addr_t ramd_mem_regions[];

#ifdef __cplusplus
}
#endif

#endif /* CONFIG_RAM_DUMP */
