/*
 * Copyright (c) 2020, Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 ******************************************************************************
 * @file    jpeg_parser.h
 * @brief   JPEG parser module driver.
 *
 */
#ifndef __JPEG_PARSER_H__
#define __JPEG_PARSER_H__
#include <string.h>
#include <assert.h>
#include <video/jpeg_hw/jpeg_hw.h>

typedef enum{
	EN_NOSUPPORT = -5,
	EN_DECERR,
	EN_FILEISEND,
	EN_FILESTARTPOS,
	EN_MEMERR,
	EN_NORMAL,
}rt_status_t;

typedef struct jpeg_parser_info {
	uint8_t *jpeg_base;
	uint32_t jpeg_size;
	uint32_t jpeg_current_offset;
	uint32_t thumbnailoffset;
	struct jpeg_info_t jpeg_info;
	uint32_t  nodata:1;
} jpeg_parser_info_t;


typedef enum {			/* JPEG marker codes */
  M_SOF0  = 0xc0,
  M_SOF1  = 0xc1,
  M_SOF2  = 0xc2,
  M_SOF3  = 0xc3,
  
  M_SOF5  = 0xc5,
  M_SOF6  = 0xc6,
  M_SOF7  = 0xc7,
  
  M_JPG   = 0xc8,
  M_SOF9  = 0xc9,
  M_SOF10 = 0xca,
  M_SOF11 = 0xcb,
  
  M_SOF13 = 0xcd,
  M_SOF14 = 0xce,
  M_SOF15 = 0xcf,
  
  M_DHT   = 0xc4,
  
  M_DAC   = 0xcc,
  
  M_RST0  = 0xd0,
  M_RST1  = 0xd1,
  M_RST2  = 0xd2,
  M_RST3  = 0xd3,
  M_RST4  = 0xd4,
  M_RST5  = 0xd5,
  M_RST6  = 0xd6,
  M_RST7  = 0xd7,
  
  M_SOI   = 0xd8,
  M_EOI   = 0xd9,
  M_SOS   = 0xda,
  M_DQT   = 0xdb,
  M_DNL   = 0xdc,
  M_DRI   = 0xdd,
  M_DHP   = 0xde,
  M_EXP   = 0xdf,
  
  M_APP0  = 0xe0,
  M_APP1  = 0xe1,
  M_APP2  = 0xe2,
  M_APP3  = 0xe3,
  M_APP4  = 0xe4,
  M_APP5  = 0xe5,
  M_APP6  = 0xe6,
  M_APP7  = 0xe7,
  M_APP8  = 0xe8,
  M_APP9  = 0xe9,
  M_APP10 = 0xea,
  M_APP11 = 0xeb,
  M_APP12 = 0xec,
  M_APP13 = 0xed,
  M_APP14 = 0xee,
  M_APP15 = 0xef,
  
  M_JPG0  = 0xf0,
  M_JPG13 = 0xfd,
  M_COM   = 0xfe,
  
  M_TEM   = 0x01,
  
  M_ERROR = 0x100,
  M_NODATA = 0x200
} JPEG_MARKER;

typedef struct{
	JPEG_MARKER marker;
	int (*marker_pro)(struct jpeg_parser_info *);
}jpeg_marker_handle_t;


#define ACHuf_0 (JPEG_VLCTABLE_RAM)
#define ACHuf_1 (ACHuf_0 + 162)
#define DCHuf_0 (ACHuf_1 + 162)
#define DCHuf_1 (DCHuf_0 + 12)

#define QT_0     (JPEG_IQTABLE_RAM)
#define QT_1     (JPEG_IQTABLE_RAM+64)
#define QT_2     (JPEG_IQTABLE_RAM+128)


#define SHIFTVAL 10

int jpeg_parser_process(struct jpeg_parser_info *parser_info, int mode);

#endif

