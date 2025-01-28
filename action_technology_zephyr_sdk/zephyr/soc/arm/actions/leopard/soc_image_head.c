/*
 * Copyright (c) 2021 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Actions LEOPARD family boot related infomation public interfaces.
 */

#include <kernel.h>
#include <device.h>
#include <string.h>
#include "soc_boot.h"
#include <linker/linker-defs.h>

#define IMAGE_MAGIC0      0x48544341
#define IMAGE_MAGIC1      0x41435448

typedef struct image_head {
	uint32_t  ih_magic0;
	uint32_t  ih_magic1;   //
	uint32_t  ih_load_addr;    /*load addr, include header*/
	uint8_t   ih_name[8];  //
	uint32_t  ih_entry;
	uint32_t ih_img_size;
	uint32_t ih_img_chksum;   /*if not sign, use check sum��ih_img_size not include header */ 
	uint32_t ih_hdr_chksum;      /* include header. */
	uint16_t ih_hdr_size;       /* Size of image header (bytes). */
	uint16_t ih_ptlv_size;   /* Size of protected TLV area (bytes). */
	uint16_t ih_tlv_size;    /*tlv size*/
	uint16_t ih_version;       
	uint32_t ih_flags;            
	uint8_t  ih_ext_ictype[7];
	uint8_t  ih_storage_type;   // mbrec
}image_head_t;

#if (CONFIG_ROM_START_OFFSET == 0)

const image_head_t app_head  __in_section_unique(img_head) =
{
	.ih_magic0         = IMAGE_MAGIC0,
	.ih_magic1         = IMAGE_MAGIC1,
	.ih_load_addr      = 0,
	.ih_name           = "app",
	.ih_entry          = 0,
	.ih_img_size       = 0,
	.ih_img_chksum     = 0,
	.ih_hdr_chksum     = 0,
	.ih_hdr_size       = sizeof(app_head),
	.ih_ptlv_size      = 0,
	.ih_tlv_size       = 0,
	.ih_version        = 0x1001,
	.ih_flags          = 0,
	.ih_ext_ictype     = {0,0,0,0,0,0,0},
	.ih_storage_type   = 0,
};

#endif
