/*
 * Copyright (c) 1997-2015, Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __DSP_IMAGE_H__
#define __DSP_IMAGE_H__

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __packed
#ifdef __GNUC__
#define __packed __attribute__((__packed__))
#else
#define __packed
#endif
#endif

/* dsp hardware memory resource */
#define CONFIG_PTCM_SIZE (0)
#define CONFIG_DTCM_SIZE (0x8000)

/* dsp coff address layout (in words) */
/* -----------------------------------------------------------------------------
 * | 31 |    30    |    29     |   28-26   |   25-23    |   22-18    |   17-0  |
 * -----------------------------------------------------------------------------
 * | 0  | reserved | bank flag |   type    | bank group | bank index | address |
 * -----------------------------------------------------------------------------
 *
 * bit 31: fixed 0
 * bit 30: reserved, fixed 0
 * bit 29: bank address flag
 * bit 28-26: bank type or image type. Identifier of the current page miss
 * 		owner. It is used to decide which dsp image that request the current
 * 		page miss in mutiple dsp image run environment. So far, only set on
 * 		banked address, that is, bank address flag is set. If type is set, it
 * 		should be equal to the f_type of IMG_FILHDR, though often ignored
 * 		when creating the image.
 * bit 25-23: bank group, only support [0x0, 0x3]
 * bit 22-18: bank index in a bank group, only support [0x0, 0xf]
 * bit 17-0: physical address, may overlap among different banks
 * */
#define NUM_BANK_GROUPS (4)
#define NUM_BANKS_PER_GROUP (16)
#define NUM_BANKS (NUM_BANK_GROUPS * NUM_BANKS_PER_GROUP)
#define BANK_NO(group, index) (group * NUM_BANKS_PER_GROUP + index)
#define BANK_GROUP(num) (num / NUM_BANKS_PER_GROUP)
#define BANK_INDEX(num) (num % NUM_BANKS_PER_GROUP)

#define IMG_BANK_FLAG_MASK (0x1 << 29)
#define IMG_BANK_FLAG(addr) ((addr & IMG_BANK_FLAG_MASK) >> 29)

#define IMG_TYPE_SHIFT (26)
#define IMG_TYPE_MASK (0x7 << IMG_TYPE_SHIFT)
#define IMG_TYPE(addr) ((addr & IMG_TYPE_MASK) >> IMG_TYPE_SHIFT)

#define IMG_BANK_GROUP_SHIFT (23)
#define IMG_BANK_GROUP_MASK (0x7 << IMG_BANK_GROUP_SHIFT)
#define IMG_BANK_GROUP(addr) ((addr & IMG_BANK_GROUP_MASK) >> IMG_BANK_GROUP_SHIFT)

#define IMG_BANK_INDEX_SHIFT (18)
#define IMG_BANK_INDEX_MASK (0x1f << IMG_BANK_INDEX_SHIFT)
#define IMG_BANK_INDEX(addr) ((addr & IMG_BANK_INDEX_MASK) >> IMG_BANK_INDEX_SHIFT)

#define IMG_BANK_NO(addr) BANK_NO(IMG_BANK_GROUP(addr), IMG_BANK_INDEX(addr))

#define IMG_BANK_ADDR(addr) (addr & ~IMG_TYPE_MASK)
//#define IMG_BANK_INNER_ADDR(addr) (addr & 0x4003ffff)

/* image file type */
#define DSP_CODEC_LIB (0)
#define DSP_SOUND_EFFECT_LIB (1)

/* image file sub-type */
#define DSP_DECODER_MP3 (0)
#define DSP_DECODER_WMA (1)
#define DSP_DECODER_WAV (2)
#define DSP_DECODER_AAC (3)
#define DSP_DECODER_APE (4)
#define DSP_DECODER_FLAC (5)
#define DSP_DECODER_OGG (6)
#define DSP_DECODER_SBC (7)
#define DSP_DECODER_MSBC (8)

#define DSP_ENCODER_WAV (40)
#define DSP_ENCODER_MP3 (41)
#define DSP_ENCODER_SBC (42)

//DSP sound effect definition
#define DSP_SOUND_EFFECT_PEQ (0)
#define DSP_SOUND_EFFECT_SRS (1)
#define DSP_SOUND_EFFECT_ACT (2)

/********************** image structure ***************************************/

/*
 *                Image Layout
 * --------------------------------------------------------------
 * | magic (4 bytes, "yqhx")                                    |
 * |------------------------------------------------------------|
 * | version (4 byte)                                           |
 * |------------------------------------------------------------|
 * | f_type (1 byte)                                            |
 * |------------------------------------------------------------|
 * | f_subtype (1 byte)                                         |
 * -------------------------------------------------------------|
 * | f_reserved (reserved, 2 byte)                              |
 * -------------------------------------------------------------|
 * | f_version (4 byte)                                         |
 * |------------------------------------------------------------|
 * | entry point (4 bytes, dsp word address)                    |
 * |------------------------------------------------------------|
 * | code & data section table pointer                          |
 * |------------------------------------------------------------|
 * | bank group 1 index 1 section table pointer                 |
 * |------------------------------------------------------------|
 * | bank group 1 index 2 section table pointer                 |
 * |------------------------------------------------------------|
 * |                 ......                                     |
 * |------------------------------------------------------------|
 * | bank group 1 index N1 section table pointer                |
 * |------------------------------------------------------------|
 * | bank group 2 index 1 section table pointer                 |
 * |------------------------------------------------------------|
 * | bank group 2 index 2 section table pointer                 |
 * |------------------------------------------------------------|
 * |                 ......                                     |
 * |------------------------------------------------------------|
 * | bank group 2 index N2 section table pointer                |
 * |------------------------------------------------------------|
 * | bank group n index 1 section table pointer                 |
 * |------------------------------------------------------------|
 * | bank group n index 2 section table pointer                 |
 * |------------------------------------------------------------|
 * |                 ......                                     |
 * |------------------------------------------------------------|
 * | bank group n index Nn section table pointer                |
 * |------------------------------------------------------------|
 * |                                                            |
 * |               section tables                               |
 * |                                                            |
 * --------------------------------------------------------------
 */

/*
 *                Image section table Layout
 * |------------------------------------------------------------|
 * | Section 1 size (4 bytes)                                   |
 * | Section 1 load address (4 bytes, dsp word address)         |
 * | Section 1 raw data (8 x n bytes, padded to 8 bytes)        |
 * |------------------------------------------------------------|
 * | Section 2 size (4 bytes)                                   |
 * | Section 2 load address (4 bytes, dsp word address)         |
 * | Section 2 raw data (8 x n bytes, padded to 8 bytes)        |
 * |------------------------------------------------------------|
 * |                 ......                                     |
 * |------------------------------------------------------------|
 * | Section N size (4 bytes)                                   |
 * | Section N load address (4 bytes, dsp word address)         |
 * | Section N raw data (8 x n bytes, padded to 8 bytes)        |
 * |------------------------------------------------------------|
 * | 0x00000000 (end flag, 4 bytes)                             |
 * --------------------------------------------------------------
 */

/* section entry in image section table */
struct IMG_scnhdr {
	uint32_t size;
	uint32_t addr;
	uint8_t data[0];
} __packed;

#define IMG_SCNHDR struct IMG_scnhdr

#define IMAGE_MAGIC(a, b, c, d)                                            \
	(((uint32_t)(a) << 24) | ((uint32_t)(b) << 16) |                       \
	 ((uint32_t)(c) << 8) | (d))

#define IMAGE_VERSION(major, minor, patchlevel)                           \
	(((uint32_t)(major) << 24) | ((uint32_t)(minor) << 16) |               \
	 ((uint32_t)(patchlevel) << 8))

#define IMG_VER_MAJOR(ver) (((ver) >> 24) & 0xFF)
#define IMG_VER_MINOR(ver) (((ver) >> 16) & 0xFF)
#define IMG_VER_PATCHLEVEL(ver) (((ver) >> 8) & 0xFF)

typedef struct IMG_filehdr {
	/* image format magic */
	uint32_t magic;
	/* image packing tool version */
	uint32_t version;

	/* image file information */
	uint8_t f_type; /* file (algorithm) type */
	uint8_t f_subtype;
	uint8_t f_reserved[2];
	uint32_t f_version; /* file (algorithm) version */

	/* dsp program entry point */
	uint32_t entry_point;

	/* dsp code & data sections */
	uint32_t code_scnptr;
	uint32_t data_scnptr;

	/* dsp page miss and internal tcm */
	uint32_t bank_scnptr[NUM_BANKS][2];
} IMG_FILHDR;

#define IMG_FILHSZ sizeof(IMG_FILHDR)

//dtcm data head info
struct IMG_dtcmhdr {
	uint32_t dsp_addr; //DSP Virtual destination address, move to address, word unit
	uint32_t mem_addr; //DSP virtual source address, data source address, word unit
	uint32_t length; //DSP data size, byte unit
	uint32_t reserved;
} __packed; //16 byte

#define IMG_DTCMHDR struct IMG_dtcmhdr

int load_dsp_image(const void *image, size_t size, uint32_t *entry_point);
int load_dsp_image_bank(const void *image, size_t size, unsigned int bank_no);
#ifdef CONFIG_LOAD_IMAGE_FROM_FS
int load_dsp_image_from_file(void *filp, uint32_t *entry_point);
int load_dsp_image_bank_from_file(void *filp, unsigned int bank_no);
#endif

#ifdef __cplusplus
}
#endif

#endif /* __DSP_IMAGE_H__ */
