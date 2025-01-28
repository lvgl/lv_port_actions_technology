/*
 * Copyright (c) 2017 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Sections overlay public API header file.
 */

#ifndef __INCLUDE_SECTION_OVERLAY_H__
#define __INCLUDE_SECTION_OVERLAY_H__

#define OVERLAY_TABLE_MAGIC		0x4c564f53

/* audio decoder format check */
#define OVERLAY_ID_LIBFMTCHK	0x63746d66	/* "fmt c" */

/* audio decoder */
#define OVERLAY_ID_LIBADACT		0x64656774	/* "act d" */
#define OVERLAY_ID_LIBADAAC		0x64636161	/* "aac d" */
#define OVERLAY_ID_LIBADAMR		0x64726d61	/* "amr d" */
#define OVERLAY_ID_LIBADAPE		0x64657061	/* "ape d" */
#define OVERLAY_ID_LIBADCVS		0x64737663	/* "cvs d" */
#define OVERLAY_ID_LIBADFLA		0x64616c66	/* "fla d" */
#define OVERLAY_ID_LIBADMP3		0x6433706d	/* "mp3 d" */
#define OVERLAY_ID_LIBADSBC		0x64434253	/* "sbc d" */
#define OVERLAY_ID_LIBADWAV		0x64766177	/* "wav d" */
#define OVERLAY_ID_LIBADWMA		0x64616d77	/* "wma d" */

/* audio encoder */
#define OVERLAY_ID_LIBAEAMR		0x65726d61	/* "arm e" */
#define OVERLAY_ID_LIBAECVS		0x65737663	/* "cvs e" */
#define OVERLAY_ID_LIBAEMP3		0x6533706d	/* "mp3 e" */
#define OVERLAY_ID_LIBAEOPU		0x65505553	/* "opu e" */
#define OVERLAY_ID_LIBAEWAV		0x65766177	/* "wav e" */

/* audio parser */
#define OVERLAY_ID_LIBAPAPE		0x70657061	/* "ape p" */
#define OVERLAY_ID_LIBAPFLA		0x64616c66	/* "fla p" */
#define OVERLAY_ID_LIBAPMP3		0x7033706d	/* "mp3 p" */
#define OVERLAY_ID_LIBAPWAV		0x70766177	/* "wav p" */
#define OVERLAY_ID_LIBAPWMA		0x70616d77	/* "wma p" */
#define OVERLAY_ID_LIBAPAAC		0x64636177	/* "aac p" */

/* (hfp speech) plc */
#define OVERLAY_ID_LIBHSPLC		0x70636c70  /* "plc p" */

/* dae */
#define OVERLAY_ID_LIBAPDAE		0x70656164	/* "dae p" */
#define OVERLAY_ID_LIBAPFAD		0x70646166	/* "fad p" */

#define OVERLAY_ID_LIBBTDRV		0x62746472	/* "btdr" */

#define OVERLAY_ID_USB_MASS		0x55764361	/* "usbm" */

#if !defined(_LINKER) && !defined(_ASMLANGUAGE)

int overlay_section_init(unsigned int idcode);
void overlay_section_dump(void);

#endif

#endif  /* __INCLUDE_SECTION_OVERLAY_H__ */
