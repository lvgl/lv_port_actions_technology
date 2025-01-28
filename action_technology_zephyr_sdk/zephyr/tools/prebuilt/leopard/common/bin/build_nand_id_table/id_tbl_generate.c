/*
 * Copyright (c) 2021 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Generate SPI NAND ID table binary.
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include "id_tbl.h"

#ifndef ID_TABLE_NAME
#define ID_TABLE_NAME "nand_id.bin"
#endif

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(_array)  (sizeof(_array) / sizeof(_array[0]))
#endif

#define MAX_ID_TABLE_NUMBER (8)

//nand_id.bin list(7)
#define   GD5F1GQ5UEYIG
#define   W25N01KVxxIR
#define   MX35LF1GE4AB
#define   DS35Q1GB
#define   F35SQA001G
#define   F50L1G41LB
//#define   FM25S01A

//ready to use list(11)
//#define W25N01GVZEIG
//#define W25N02KB
//#define W25N512GVxIG
//#define MX35LF2GE4AD
//#define MX35UF2GE4AD
//#define DS35Q1GA
//#define DS35Q2GBS
#define F35SQA512M
//#define F35SQB002G
//#define KANY1D4S2WD
#define GD5F1GM7UEYIG
//#define GD5F2GM7UE
//#define GD5F4GM8UExxG
//#define XT26G01D
//#define XT26G02D
//#define XT26G04D
//#define FM25S01B
//#define HYF1GQ4UT
//#define HYF2GQ4UT
//#define HYF4GQ4UT
//#define XCSP1AAPK
//#define XCSP2AAPK
//#define XCSP4AAPK
//#define ZB35Q01B
//#define ZB35Q02B
//#define HSESYHDSW1G
//#define HSESYHDSW2G
//#define HSESYHDSW4G

static const struct FlashChipInfo FlashChipInfoTbl[] =
{
    /* =================================================================================== */
#ifdef GD5F1GQ5UEYIG
    /* GD5F1GQ5UEYIG 2KB PageSize, 128MB SPINand */
    {
        {0xc8, 0x51, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff},   /* ChipID */
        1,                                                  /* ChipCnt */
        1,                                                  /* DieCntPerChip */
        1,                                                  /* PlaneCntPerDie */
        4,                                                  /* SectNumPerPPage */
        16,                                                 /* SpareBytesPerSector */
        90,                                                 /* Frequence */
        64,                                                 /* PageNumPerPBlk */
        1024,                                               /* BlkNumPerDie */
        984,                                                /* DefaultLBlkNumPer1024PBlk */
        2048,                                               /* userMetaOffset*/
        16,                                                 /* userMetaShift*/
        4,                                                  /* userMetaSize*/
        2,                                                  /* ECCBits */
        0x30,                                               /* eccStatusMask*/
        0x20,                                               /* eccStatusErrVal*/
        16,                                                 /* readAddressBits*/
        /**
         * read mode:
         * 0x0:1xIO(0x03), cpu trans; 0x10:1xIO(0x03), dma trans;
         * 0x1:2xIO-quad(0xbb), cpu trans; 0x11:2xIO-quad(0xbb), dma trans;
         * 0x2:2xIO-dual(0x3b), cpu trans; 0x12:2xIO-dual(0x3b), dma trans;
         * 0x4:4xIO-quad(0x6b), cpu trans; 0x14:4xIO-quad(0x6b), dma trans;
         * 0x8:4xIO-dual(0xeb), cpu trans; 0x18:4xIO-dual(0xeb), dma trans. the most fast.
         */

        0x14,                                               /* readMode 0x10*/
        /**
         * write mode:
         * 0x0:1xIO(0x02), cpu trans; 0x10:1xIO(0x02), dma trans;
         * 0x4:4xIO-quad(0x32), cpu trans; 0x14:4xIO-quad(0x32), dma trans;
         */
        0x14,                                               /* writeMode 0x10*/
        FLASH_CHIP_INFO_VERSION,
        "GD5F1GQ5UEYIG",
        41,                                                 /* delaychain */
        {0},                                                /* Reserved, 12Bytes */
    },
#endif
#ifdef W25N01GVZEIG
    /* W25N01GVZEIG 2KB PageSize, 128MB SPINand */
    {
        {0xef, 0xaa, 0x21, 0x00, 0xff, 0xff, 0xff, 0xff},   /* ChipID */
        1,                                                  /* ChipCnt */
        1,                                                  /* DieCntPerChip */
        1,                                                  /* PlaneCntPerDie */
        4,                                                  /* SectNumPerPPage */
        16,                                                 /* SpareBytesPerSector */
        90,                                                 /* Frequence */
        64,                                                 /* PageNumPerPBlk */
        1024,                                               /* BlkNumPerDie */
        984,                                                /* DefaultLBlkNumPer1024PBlk */
        2048,                                               /* userMetaOffset*/
        16,                                                 /* userMetaShift*/
        4,                                                  /* userMetaSize*/
        1,                                                  /* ECCBits */
        0x30,                                               /* eccStatusMask*/
        0x10,                                               /* eccStatusErrVal*/
        16,                                                 /* readAddressBits*/
        /**
         * read mode:
         * 0x0:1xIO(0x03), cpu trans; 0x10:1xIO(0x03), dma trans;
         * 0x1:2xIO-quad(0xbb), cpu trans; 0x11:2xIO-quad(0xbb), dma trans;
         * 0x2:2xIO-dual(0x3b), cpu trans; 0x12:2xIO-dual(0x3b), dma trans;
         * 0x4:4xIO-quad(0x6b), cpu trans; 0x14:4xIO-quad(0x6b), dma trans;
         * 0x8:4xIO-dual(0xeb), cpu trans; 0x18:4xIO-dual(0xeb), dma trans. the most fast.
         */

        0x14,                                               /* readMode 0x10*/
        /**
         * write mode:
         * 0x0:1xIO(0x02), cpu trans; 0x10:1xIO(0x02), dma trans;
         * 0x4:4xIO-quad(0x32), cpu trans; 0x14:4xIO-quad(0x32), dma trans;
         */
        0x14,                                               /* writeMode 0x10*/
        FLASH_CHIP_INFO_VERSION,
        "W25N01GVZEIG",
        41,                                                  /* delaychain */
        {0},                                                /* Reserved, 12Bytes */
    },
#endif
#ifdef W25N01KVxxIR
    /* W25N01KVxxIR 2KB PageSize, 128MB SPINand */
    {
        {0xef, 0xae, 0x21, 0x00, 0xff, 0xff, 0xff, 0xff},   /* ChipID */
        1,                                                  /* ChipCnt */
        1,                                                  /* DieCntPerChip */
        1,                                                  /* PlaneCntPerDie */
        4,                                                  /* SectNumPerPPage */
        16,                                                 /* SpareBytesPerSector */
        90,                                                 /* Frequence */
        64,                                                 /* PageNumPerPBlk */
        1024,                                               /* BlkNumPerDie */
        984,                                                /* DefaultLBlkNumPer1024PBlk */
        2048,                                               /* userMetaOffset*/
        16,                                                 /* userMetaShift*/
        4,                                                  /* userMetaSize*/
        1,                                                  /* ECCBits */
        0x30,                                               /* eccStatusMask*/
        0x10,                                               /* eccStatusErrVal*/
        16,                                                 /* readAddressBits*/
        /**
         * read mode:
         * 0x0:1xIO(0x03), cpu trans; 0x10:1xIO(0x03), dma trans;
         * 0x1:2xIO-quad(0xbb), cpu trans; 0x11:2xIO-quad(0xbb), dma trans;
         * 0x2:2xIO-dual(0x3b), cpu trans; 0x12:2xIO-dual(0x3b), dma trans;
         * 0x4:4xIO-quad(0x6b), cpu trans; 0x14:4xIO-quad(0x6b), dma trans;
         * 0x8:4xIO-dual(0xeb), cpu trans; 0x18:4xIO-dual(0xeb), dma trans. the most fast.
         */

        0x14,                                               /* readMode 0x10*/
        /**
         * write mode:
         * 0x0:1xIO(0x02), cpu trans; 0x10:1xIO(0x02), dma trans;
         * 0x4:4xIO-quad(0x32), cpu trans; 0x14:4xIO-quad(0x32), dma trans;
         */
        0x14,                                               /* writeMode 0x10*/
        FLASH_CHIP_INFO_VERSION,
        "W25N01KVxxIR",
        41,                                                 /* delaychain */
        {0},                                                /* Reserved, 12Bytes */
    },
#endif
#ifdef W25N02KB
    /* W25N02KB 2KB PageSize, 256MB SPINand */
    {
        {0xef, 0xaa, 0x22, 0x00, 0xff, 0xff, 0xff, 0xff},   /* ChipID */
        1,                                                  /* ChipCnt */
        1,                                                  /* DieCntPerChip */
        1,                                                  /* PlaneCntPerDie */
        4,                                                  /* SectNumPerPPage */
        16,                                                 /* SpareBytesPerSector */
        90,                                                 /* Frequence */
        64,                                                 /* PageNumPerPBlk */
        2048,                                               /* BlkNumPerDie */
        984,                                                /* DefaultLBlkNumPer1024PBlk */
        2048,                                               /* userMetaOffset*/
        16,                                                 /* userMetaShift*/
        4,                                                  /* userMetaSize*/
        1,                                                  /* ECCBits */
        0x30,                                               /* eccStatusMask*/
        0x10,                                               /* eccStatusErrVal*/
        16,                                                 /* readAddressBits*/
        /**
         * read mode:
         * 0x0:1xIO(0x03), cpu trans; 0x10:1xIO(0x03), dma trans;
         * 0x1:2xIO-quad(0xbb), cpu trans; 0x11:2xIO-quad(0xbb), dma trans;
         * 0x2:2xIO-dual(0x3b), cpu trans; 0x12:2xIO-dual(0x3b), dma trans;
         * 0x4:4xIO-quad(0x6b), cpu trans; 0x14:4xIO-quad(0x6b), dma trans;
         * 0x8:4xIO-dual(0xeb), cpu trans; 0x18:4xIO-dual(0xeb), dma trans. the most fast.
         */

        0x14,                                               /* readMode 0x10*/
        /**
         * write mode:
         * 0x0:1xIO(0x02), cpu trans; 0x10:1xIO(0x02), dma trans;
         * 0x4:4xIO-quad(0x32), cpu trans; 0x14:4xIO-quad(0x32), dma trans;
         */
        0x14,                                               /* writeMode 0x10*/
        FLASH_CHIP_INFO_VERSION,
        "W25N02KV",
        41,                                                  /* delaychain */
        {0},                                                /* Reserved, 12Bytes */
    },
#endif
#ifdef W25N512GVxIG
    /* W25N512GVxIG/IT 2KB PageSize, 64MB SPINand */
    {
        {0xef, 0xaa, 0x20, 0x00, 0xff, 0xff, 0xff, 0xff},   /* ChipID */
        1,                                                  /* ChipCnt */
        1,                                                  /* DieCntPerChip */
        1,                                                  /* PlaneCntPerDie */
        4,                                                  /* SectNumPerPPage */
        16,                                                 /* SpareBytesPerSector */
        90,                                                 /* Frequence */
        64,                                                 /* PageNumPerPBlk */
        512,                                                /* BlkNumPerDie */
        492,                                                /* DefaultLBlkNumPer1024PBlk */
        2048,                                               /* userMetaOffset*/
        16,                                                 /* userMetaShift*/
        4,                                                  /* userMetaSize*/
        1,                                                  /* ECCBits */
        0x30,                                               /* eccStatusMask*/
        0x10,                                               /* eccStatusErrVal*/
        16,                                                 /* readAddressBits*/
        /**
         * read mode:
         * 0x0:1xIO(0x03), cpu trans; 0x10:1xIO(0x03), dma trans;
         * 0x1:2xIO-quad(0xbb), cpu trans; 0x11:2xIO-quad(0xbb), dma trans;
         * 0x2:2xIO-dual(0x3b), cpu trans; 0x12:2xIO-dual(0x3b), dma trans;
         * 0x4:4xIO-quad(0x6b), cpu trans; 0x14:4xIO-quad(0x6b), dma trans;
         * 0x8:4xIO-dual(0xeb), cpu trans; 0x18:4xIO-dual(0xeb), dma trans. the most fast.
         */

        0x14,                                               /* readMode 0x10*/
        /**
         * write mode:
         * 0x0:1xIO(0x02), cpu trans; 0x10:1xIO(0x02), dma trans;
         * 0x4:4xIO-quad(0x32), cpu trans; 0x14:4xIO-quad(0x32), dma trans;
         */
        0x14,                                               /* writeMode 0x10*/
        FLASH_CHIP_INFO_VERSION,
        "W25N512GVxIG",
        41,                                                  /* delaychain */
        {0},                                                /* Reserved, 12Bytes */
    },
#endif
#ifdef MX35LF1GE4AB
    /* MX35LF1GE4AB 2KB PageSize, 128MB SPINand */
    {
        {0xc2, 0x12, 0xc2, 0x12, 0xff, 0xff, 0xff, 0xff},   /* ChipID */
        1,                                                  /* ChipCnt */
        1,                                                  /* DieCntPerChip */
        1,                                                  /* PlaneCntPerDie */
        4,                                                  /* SectNumPerPPage */
        16,                                                 /* SpareBytesPerSector */
        90,                                                 /* Frequence */
        64,                                                 /* PageNumPerPBlk */
        1024,                                               /* BlkNumPerDie */
        984,                                                /* DefaultLBlkNumPer1024PBlk */
        2048,                                               /* userMetaOffset*/
        16,                                                 /* userMetaShift*/
        4,                                                  /* userMetaSize*/
        1,                                                  /* ECCBits */
        0x30,                                               /* eccStatusMask*/
        0x10,                                               /* eccStatusErrVal*/
        16,                                                 /* readAddressBits*/
        /**
         * read mode:
         * 0x0:1xIO(0x03), cpu trans; 0x10:1xIO(0x03), dma trans;
         * 0x1:2xIO-quad(0xbb), cpu trans; 0x11:2xIO-quad(0xbb), dma trans;
         * 0x2:2xIO-dual(0x3b), cpu trans; 0x12:2xIO-dual(0x3b), dma trans;
         * 0x4:4xIO-quad(0x6b), cpu trans; 0x14:4xIO-quad(0x6b), dma trans;
         * 0x8:4xIO-dual(0xeb), cpu trans; 0x18:4xIO-dual(0xeb), dma trans. the most fast.
         */

        0x14,                                               /* readMode 0x10*/
        /**
         * write mode:
         * 0x0:1xIO(0x02), cpu trans; 0x10:1xIO(0x02), dma trans;
         * 0x4:4xIO-quad(0x32), cpu trans; 0x14:4xIO-quad(0x32), dma trans;
         */
        0x14,                                               /* writeMode 0x10*/
        FLASH_CHIP_INFO_VERSION,
        "MX35LF1GE4AB",
        41,                                                 /* delaychain */
        {0},                                                /* Reserved, 12Bytes */
    },
#endif
#ifdef MX35LF2GE4AD
    /* MX35LF2GE4AD 2KB PageSize, 256MB SPINand */
    {
        {0xc2, 0x26, 0x03, 0xff, 0xff, 0xff, 0xff, 0xff},   /* ChipID */
        1,                                                  /* ChipCnt */
        1,                                                  /* DieCntPerChip */
        1,                                                  /* PlaneCntPerDie */
        4,                                                  /* SectNumPerPPage */
        16,                                                 /* SpareBytesPerSector */
        90,                                                 /* Frequence */
        64,                                                 /* PageNumPerPBlk */
        2048,                                               /* BlkNumPerDie */
        984,                                                /* DefaultLBlkNumPer1024PBlk */
        2048,                                               /* userMetaOffset*/
        16,                                                 /* userMetaShift*/
        4,                                                  /* userMetaSize*/
        1,                                                  /* ECCBits */
        0x30,                                               /* eccStatusMask*/
        0x10,                                               /* eccStatusErrVal*/
        16,                                                 /* readAddressBits*/
        /**
         * read mode:
         * 0x0:1xIO(0x03), cpu trans; 0x10:1xIO(0x03), dma trans;
         * 0x1:2xIO-quad(0xbb), cpu trans; 0x11:2xIO-quad(0xbb), dma trans;
         * 0x2:2xIO-dual(0x3b), cpu trans; 0x12:2xIO-dual(0x3b), dma trans;
         * 0x4:4xIO-quad(0x6b), cpu trans; 0x14:4xIO-quad(0x6b), dma trans;
         * 0x8:4xIO-dual(0xeb), cpu trans; 0x18:4xIO-dual(0xeb), dma trans. the most fast.
         */

        0x14,                                               /* readMode 0x10*/
        /**
         * write mode:
         * 0x0:1xIO(0x02), cpu trans; 0x10:1xIO(0x02), dma trans;
         * 0x4:4xIO-quad(0x32), cpu trans; 0x14:4xIO-quad(0x32), dma trans;
         */
        0x14,                                               /* writeMode 0x10*/
        FLASH_CHIP_INFO_VERSION,
        "MX35LF2GE4AD",
        41,                                                 /* delaychain */
        {0},                                                /* Reserved, 12Bytes */
    },
#endif
#ifdef MX35UF2GE4AD
    /* MX35UF2GE4AD 2KB PageSize, 1.8V, 256MB SPINand */
    {
        {0xc2, 0xa6, 0x03, 0xff, 0xff, 0xff, 0xff, 0xff},   /* ChipID */
        1,                                                  /* ChipCnt */
        1,                                                  /* DieCntPerChip */
        1,                                                  /* PlaneCntPerDie */
        4,                                                  /* SectNumPerPPage */
        16,                                                 /* SpareBytesPerSector */
        90,                                                 /* Frequence */
        64,                                                 /* PageNumPerPBlk */
        2048,                                               /* BlkNumPerDie */
        984,                                                /* DefaultLBlkNumPer1024PBlk */
        2048,                                               /* userMetaOffset*/
        16,                                                 /* userMetaShift*/
        4,                                                  /* userMetaSize*/
        1,                                                  /* ECCBits */
        0x30,                                               /* eccStatusMask*/
        0x10,                                               /* eccStatusErrVal*/
        16,                                                 /* readAddressBits*/
        /**
         * read mode:
         * 0x0:1xIO(0x03), cpu trans; 0x10:1xIO(0x03), dma trans;
         * 0x1:2xIO-quad(0xbb), cpu trans; 0x11:2xIO-quad(0xbb), dma trans;
         * 0x2:2xIO-dual(0x3b), cpu trans; 0x12:2xIO-dual(0x3b), dma trans;
         * 0x4:4xIO-quad(0x6b), cpu trans; 0x14:4xIO-quad(0x6b), dma trans;
         * 0x8:4xIO-dual(0xeb), cpu trans; 0x18:4xIO-dual(0xeb), dma trans. the most fast.
         */

        0x14,                                               /* readMode 0x10*/
        /**
         * write mode:
         * 0x0:1xIO(0x02), cpu trans; 0x10:1xIO(0x02), dma trans;
         * 0x4:4xIO-quad(0x32), cpu trans; 0x14:4xIO-quad(0x32), dma trans;
         */
        0x14,                                               /* writeMode 0x10*/
        FLASH_CHIP_INFO_VERSION,
        "MX35UF2GE4AD",
        41,                                                 /* delaychain */
        {0},                                                /* Reserved, 12Bytes */
    },
#endif
#ifdef FM25S01A
    /* FM25S01A 2KB PageSize, 128MB SPINand */
    {
        {0xa1, 0xe4, 0x7f, 0x7f, 0xff, 0xff, 0xff, 0xff},   /* ChipID */
        1,                                                  /* ChipCnt */
        1,                                                  /* DieCntPerChip */
        1,                                                  /* PlaneCntPerDie */
        4,                                                  /* SectNumPerPPage */
        16,                                                 /* SpareBytesPerSector */
        90,                                                 /* Frequence */
        64,                                                 /* PageNumPerPBlk */
        1024,                                               /* BlkNumPerDie */
        984,                                                /* DefaultLBlkNumPer1024PBlk */
        2048,                                               /* userMetaOffset*/
        16,                                                 /* userMetaShift*/
        4,                                                  /* userMetaSize*/
        1,                                                  /* ECCBits */
        0x30,                                               /* eccStatusMask*/
        0x10,                                               /* eccStatusErrVal*/
        16,                                                 /* readAddressBits*/
        /**
         * read mode:
         * 0x0:1xIO(0x03), cpu trans; 0x10:1xIO(0x03), dma trans;
         * 0x1:2xIO-quad(0xbb), cpu trans; 0x11:2xIO-quad(0xbb), dma trans;
         * 0x2:2xIO-dual(0x3b), cpu trans; 0x12:2xIO-dual(0x3b), dma trans;
         * 0x4:4xIO-quad(0x6b), cpu trans; 0x14:4xIO-quad(0x6b), dma trans;
         * 0x8:4xIO-dual(0xeb), cpu trans; 0x18:4xIO-dual(0xeb), dma trans. the most fast.
         */

        0x14,                                               /* readMode 0x10*/
        /**
         * write mode:
         * 0x0:1xIO(0x02), cpu trans; 0x10:1xIO(0x02), dma trans;
         * 0x4:4xIO-quad(0x32), cpu trans; 0x14:4xIO-quad(0x32), dma trans;
         */
        0x14,                                               /* writeMode 0x10*/
        FLASH_CHIP_INFO_VERSION,
        "FM25S01A",
        41,                                                 /* delaychain */
        {0},                                                /* Reserved, 12Bytes */
    },
#endif
#ifdef DS35Q1GA
    /* DS35Q1GA 2KB PageSize, 128MB SPINand */
    {
        {0xe5, 0x71, 0xe5, 0x71, 0xff, 0xff, 0xff, 0xff},   /* ChipID */
        1,                                                  /* ChipCnt */
        1,                                                  /* DieCntPerChip */
        1,                                                  /* PlaneCntPerDie */
        4,                                                  /* SectNumPerPPage */
        16,                                                 /* SpareBytesPerSector */
        90,                                                 /* Frequence */
        64,                                                 /* PageNumPerPBlk */
        1024,                                                /* BlkNumPerDie */
        984,                                                /* DefaultLBlkNumPer1024PBlk */
        2048,                                               /* userMetaOffset*/
        16,                                                 /* userMetaShift*/
        4,                                                  /* userMetaSize*/
        1,                                                  /* ECCBits */
        0x30,                                               /* eccStatusMask*/
        0x10,                                               /* eccStatusErrVal*/
        16,                                                 /* readAddressBits*/
        /**
         * read mode:
         * 0x0:1xIO(0x03), cpu trans; 0x10:1xIO(0x03), dma trans;
         * 0x1:2xIO-quad(0xbb), cpu trans; 0x11:2xIO-quad(0xbb), dma trans;
         * 0x2:2xIO-dual(0x3b), cpu trans; 0x12:2xIO-dual(0x3b), dma trans;
         * 0x4:4xIO-quad(0x6b), cpu trans; 0x14:4xIO-quad(0x6b), dma trans;
         * 0x8:4xIO-dual(0xeb), cpu trans; 0x18:4xIO-dual(0xeb), dma trans. the most fast.
         */

        0x14,                                               /* readMode 0x10*/
        /**
         * write mode:
         * 0x0:1xIO(0x02), cpu trans; 0x10:1xIO(0x02), dma trans;
         * 0x4:4xIO-quad(0x32), cpu trans; 0x14:4xIO-quad(0x32), dma trans;
         */
        0x14,                                               /* writeMode 0x10*/
        FLASH_CHIP_INFO_VERSION,
        "DS35Q1GA",
        41,                                                 /* delaychain */
        {0},                                                /* Reserved, 12Bytes */
    },
#endif
#ifdef DS35Q1GB
    /* DS35Q1GB 2KB PageSize, 128MB SPINand */
    {
        {0xe5, 0xf1, 0xe5, 0xf1, 0xff, 0xff, 0xff, 0xff},   /* ChipID */
        1,                                                  /* ChipCnt */
        1,                                                  /* DieCntPerChip */
        1,                                                  /* PlaneCntPerDie */
        4,                                                  /* SectNumPerPPage */
        32,                                                 /* SpareBytesPerSector */
        90,                                                 /* Frequence */
        64,                                                 /* PageNumPerPBlk */
        1024,                                                /* BlkNumPerDie */
        984,                                                /* DefaultLBlkNumPer1024PBlk */
        2048,                                               /* userMetaOffset*/
        16,                                                 /* userMetaShift*/
        4,                                                  /* userMetaSize*/
        1,                                                  /* ECCBits */
        0x70,                                               /* eccStatusMask*/
        0x10,                                               /* eccStatusErrVal*/
        16,                                                 /* readAddressBits*/
        /**
         * read mode:
         * 0x0:1xIO(0x03), cpu trans; 0x10:1xIO(0x03), dma trans;
         * 0x1:2xIO-quad(0xbb), cpu trans; 0x11:2xIO-quad(0xbb), dma trans;
         * 0x2:2xIO-dual(0x3b), cpu trans; 0x12:2xIO-dual(0x3b), dma trans;
         * 0x4:4xIO-quad(0x6b), cpu trans; 0x14:4xIO-quad(0x6b), dma trans;
         * 0x8:4xIO-dual(0xeb), cpu trans; 0x18:4xIO-dual(0xeb), dma trans. the most fast.
         */

        0x14,                                               /* readMode 0x10*/
        /**
         * write mode:
         * 0x0:1xIO(0x02), cpu trans; 0x10:1xIO(0x02), dma trans;
         * 0x4:4xIO-quad(0x32), cpu trans; 0x14:4xIO-quad(0x32), dma trans;
         */
        0x14,                                               /* writeMode 0x10*/
        FLASH_CHIP_INFO_VERSION,
        "DS35Q1GB",
        41,                                                 /* delaychain */
        {0},                                                /* Reserved, 12Bytes */
    },
#endif
#ifdef DS35Q2GBS
    /* DS35Q2GBS 2KB PageSize, 256MB SPINand */
    {
        {0xe5, 0xb2, 0xe5, 0xb2, 0xff, 0xff, 0xff, 0xff},   /* ChipID */
        1,                                                  /* ChipCnt */
        1,                                                  /* DieCntPerChip */
        1,                                                  /* PlaneCntPerDie */
        4,                                                  /* SectNumPerPPage */
        32,                                                 /* SpareBytesPerSector */
        90,                                                 /* Frequence */
        64,                                                 /* PageNumPerPBlk */
        2048,                                                /* BlkNumPerDie */
        984,                                                /* DefaultLBlkNumPer1024PBlk */
        2048,                                               /* userMetaOffset*/
        16,                                                 /* userMetaShift*/
        4,                                                  /* userMetaSize*/
        2,                                                  /* ECCBits */
        0x70,                                               /* eccStatusMask*/
        0x20,                                               /* eccStatusErrVal*/
        16,                                                 /* readAddressBits*/
        /**
         * read mode:
         * 0x0:1xIO(0x03), cpu trans; 0x10:1xIO(0x03), dma trans;
         * 0x1:2xIO-quad(0xbb), cpu trans; 0x11:2xIO-quad(0xbb), dma trans;
         * 0x2:2xIO-dual(0x3b), cpu trans; 0x12:2xIO-dual(0x3b), dma trans;
         * 0x4:4xIO-quad(0x6b), cpu trans; 0x14:4xIO-quad(0x6b), dma trans;
         * 0x8:4xIO-dual(0xeb), cpu trans; 0x18:4xIO-dual(0xeb), dma trans. the most fast.
         */

        0x14,                                               /* readMode 0x10*/
        /**
         * write mode:
         * 0x0:1xIO(0x02), cpu trans; 0x10:1xIO(0x02), dma trans;
         * 0x4:4xIO-quad(0x32), cpu trans; 0x14:4xIO-quad(0x32), dma trans;
         */
        0x14,                                               /* writeMode 0x10*/
        FLASH_CHIP_INFO_VERSION,
        "DS35Q2GBS",
        41,                                                 /* delaychain */
        {0},                                                /* Reserved, 12Bytes */
    },
#endif
#ifdef F35SQA001G
    /* F35SQA001G 2KB PageSize, 128MB SPINand */
    {
        {0xcd, 0x71, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff},   /* ChipID */
        1,                                                  /* ChipCnt */
        1,                                                  /* DieCntPerChip */
        1,                                                  /* PlaneCntPerDie */
        4,                                                  /* SectNumPerPPage */
        16,                                                 /* SpareBytesPerSector */
        90,                                                 /* Frequence */
        64,                                                 /* PageNumPerPBlk */
        1024,                                               /* BlkNumPerDie */
        984,                                                /* DefaultLBlkNumPer1024PBlk */
        2048,                                               /* userMetaOffset*/
        16,                                                 /* userMetaShift*/
        4,                                                  /* userMetaSize*/
        1,                                                  /* ECCBits */
        0x30,                                               /* eccStatusMask*/
        0x10,                                               /* eccStatusErrVal*/
        16,                                                 /* readAddressBits*/
        /**
         * read mode:
         * 0x0:1xIO(0x03), cpu trans; 0x10:1xIO(0x03), dma trans;
         * 0x1:2xIO-quad(0xbb), cpu trans; 0x11:2xIO-quad(0xbb), dma trans;
         * 0x2:2xIO-dual(0x3b), cpu trans; 0x12:2xIO-dual(0x3b), dma trans;
         * 0x4:4xIO-quad(0x6b), cpu trans; 0x14:4xIO-quad(0x6b), dma trans;
         * 0x8:4xIO-dual(0xeb), cpu trans; 0x18:4xIO-dual(0xeb), dma trans. the most fast.
         */

        0x14,                                               /* readMode 0x10*/
        /**
         * write mode:
         * 0x0:1xIO(0x02), cpu trans; 0x10:1xIO(0x02), dma trans;
         * 0x4:4xIO-quad(0x32), cpu trans; 0x14:4xIO-quad(0x32), dma trans;
         */
        0x14,                                               /* writeMode 0x10*/
        FLASH_CHIP_INFO_VERSION,
        "F35SQA001G",
        41,                                                 /* delaychain */
        {0},                                                /* Reserved, 12Bytes */
    },
#endif
#ifdef F35SQA512M
    /* F35SQA512M 2KB PageSize, 64MB SPINand */
    {
        {0xcd, 0x70, 0x70, 0xcd, 0xff, 0xff, 0xff, 0xff},   /* ChipID */
        1,                                                  /* ChipCnt */
        1,                                                  /* DieCntPerChip */
        1,                                                  /* PlaneCntPerDie */
        4,                                                  /* SectNumPerPPage */
        16,                                                 /* SpareBytesPerSector */
        90,                                                 /* Frequence */
        64,                                                 /* PageNumPerPBlk */
        512,                                                /* BlkNumPerDie */
        492,                                                /* DefaultLBlkNumPer1024PBlk */
        2048,                                               /* userMetaOffset*/
        16,                                                 /* userMetaShift*/
        4,                                                  /* userMetaSize*/
        1,                                                  /* ECCBits */
        0x30,                                               /* eccStatusMask*/
        0x10,                                               /* eccStatusErrVal*/
        16,                                                 /* readAddressBits*/
        /**
         * read mode:
         * 0x0:1xIO(0x03), cpu trans; 0x10:1xIO(0x03), dma trans;
         * 0x1:2xIO-quad(0xbb), cpu trans; 0x11:2xIO-quad(0xbb), dma trans;
         * 0x2:2xIO-dual(0x3b), cpu trans; 0x12:2xIO-dual(0x3b), dma trans;
         * 0x4:4xIO-quad(0x6b), cpu trans; 0x14:4xIO-quad(0x6b), dma trans;
         * 0x8:4xIO-dual(0xeb), cpu trans; 0x18:4xIO-dual(0xeb), dma trans. the most fast.
         */

        0x14,                                               /* readMode 0x10*/
        /**
         * write mode:
         * 0x0:1xIO(0x02), cpu trans; 0x10:1xIO(0x02), dma trans;
         * 0x4:4xIO-quad(0x32), cpu trans; 0x14:4xIO-quad(0x32), dma trans;
         */
        0x14,                                               /* writeMode 0x10*/
        FLASH_CHIP_INFO_VERSION,
        "F35SQA512M",
        41,                                                 /* delaychain */
        {0},                                                /* Reserved, 12Bytes */
    },
#endif
#ifdef F35SQB002G
    /* F35SQB002G 2KB PageSize, 64MB SPINand */
    {
        {0xcd, 0x52, 0x52, 0xcd, 0xff, 0xff, 0xff, 0xff},   /* ChipID */
        1,                                                  /* ChipCnt */
        1,                                                  /* DieCntPerChip */
        1,                                                  /* PlaneCntPerDie */
        4,                                                  /* SectNumPerPPage */
        16,                                                 /* SpareBytesPerSector */
        90,                                                 /* Frequence */
        64,                                                 /* PageNumPerPBlk */
        2048,                                                /* BlkNumPerDie */
        984,                                                /* DefaultLBlkNumPer1024PBlk */
        2048,                                               /* userMetaOffset*/
        16,                                                 /* userMetaShift*/
        4,                                                  /* userMetaSize*/
        1,                                                  /* ECCBits */
        0x70,                                               /* eccStatusMask*/
        0x70,                                               /* eccStatusErrVal*/
        16,                                                 /* readAddressBits*/
        /**
         * read mode:
         * 0x0:1xIO(0x03), cpu trans; 0x10:1xIO(0x03), dma trans;
         * 0x1:2xIO-quad(0xbb), cpu trans; 0x11:2xIO-quad(0xbb), dma trans;
         * 0x2:2xIO-dual(0x3b), cpu trans; 0x12:2xIO-dual(0x3b), dma trans;
         * 0x4:4xIO-quad(0x6b), cpu trans; 0x14:4xIO-quad(0x6b), dma trans;
         * 0x8:4xIO-dual(0xeb), cpu trans; 0x18:4xIO-dual(0xeb), dma trans. the most fast.
         */

        0x14,                                               /* readMode 0x10*/
        /**
         * write mode:
         * 0x0:1xIO(0x02), cpu trans; 0x10:1xIO(0x02), dma trans;
         * 0x4:4xIO-quad(0x32), cpu trans; 0x14:4xIO-quad(0x32), dma trans;
         */
        0x14,                                               /* writeMode 0x10*/
        FLASH_CHIP_INFO_VERSION,
        "F35SQB002G",
        41,                                                 /* delaychain */
        {0},                                                /* Reserved, 12Bytes */
    },
#endif
#ifdef F50L1G41LB
    /* F50L1G41LB 2KB PageSize, 128MB SPINand */
    {
        {0xc8, 0x01, 0x7f, 0x7f, 0xff, 0xff, 0xff, 0xff},   /* ChipID */
        1,                                                  /* ChipCnt */
        1,                                                  /* DieCntPerChip */
        1,                                                  /* PlaneCntPerDie */
        4,                                                  /* SectNumPerPPage */
        16,                                                 /* SpareBytesPerSector */
        90,                                                 /* Frequence */
        64,                                                 /* PageNumPerPBlk */
        1024,                                               /* BlkNumPerDie */
        984,                                                /* DefaultLBlkNumPer1024PBlk */
        2048,                                               /* userMetaOffset*/
        16,                                                 /* userMetaShift*/
        4,                                                  /* userMetaSize*/
        1,                                                  /* ECCBits */
        0x30,                                               /* eccStatusMask*/
        0x10,                                               /* eccStatusErrVal*/
        16,                                                 /* readAddressBits*/
        /**
         * read mode:
         * 0x0:1xIO(0x03), cpu trans; 0x10:1xIO(0x03), dma trans;
         * 0x1:2xIO-quad(0xbb), cpu trans; 0x11:2xIO-quad(0xbb), dma trans;
         * 0x2:2xIO-dual(0x3b), cpu trans; 0x12:2xIO-dual(0x3b), dma trans;
         * 0x4:4xIO-quad(0x6b), cpu trans; 0x14:4xIO-quad(0x6b), dma trans;
         * 0x8:4xIO-dual(0xeb), cpu trans; 0x18:4xIO-dual(0xeb), dma trans. the most fast.
         */

        0x14,                                               /* readMode 0x10*/
        /**
         * write mode:
         * 0x0:1xIO(0x02), cpu trans; 0x10:1xIO(0x02), dma trans;
         * 0x4:4xIO-quad(0x32), cpu trans; 0x14:4xIO-quad(0x32), dma trans;
         */
        0x14,                                               /* writeMode 0x10*/
        FLASH_CHIP_INFO_VERSION,
        "F50L1G41LB",
        41,                                                 /* delaychain */
        {0},                                                /* Reserved, 12Bytes */
    },
#endif
#ifdef KANY1D4S2WD
    /* KANY1D4S2WD 2KB PageSize, 128MB SPINand */
    {
        {0x1, 0x15, 0x1, 0x15, 0xff, 0xff, 0xff, 0xff},   /* ChipID */
        1,                                                  /* ChipCnt */
        1,                                                  /* DieCntPerChip */
        1,                                                  /* PlaneCntPerDie */
        4,                                                  /* SectNumPerPPage */
        16,                                                 /* SpareBytesPerSector */
        90,                                                 /* Frequence */
        64,                                                 /* PageNumPerPBlk */
        1024,                                               /* BlkNumPerDie */
        984,                                                /* DefaultLBlkNumPer1024PBlk */
        2048,                                               /* userMetaOffset*/
        16,                                                 /* userMetaShift*/
        4,                                                  /* userMetaSize*/
        1,                                                  /* ECCBits */
        0x30,                                               /* eccStatusMask*/
        0x20,                                               /* eccStatusErrVal*/
        16,                                                 /* readAddressBits*/
        /**
         * read mode:
         * 0x0:1xIO(0x03), cpu trans; 0x10:1xIO(0x03), dma trans;
         * 0x1:2xIO-quad(0xbb), cpu trans; 0x11:2xIO-quad(0xbb), dma trans;
         * 0x2:2xIO-dual(0x3b), cpu trans; 0x12:2xIO-dual(0x3b), dma trans;
         * 0x4:4xIO-quad(0x6b), cpu trans; 0x14:4xIO-quad(0x6b), dma trans;
         * 0x8:4xIO-dual(0xeb), cpu trans; 0x18:4xIO-dual(0xeb), dma trans. the most fast.
         */

        0x14,                                               /* readMode 0x10*/
        /**
         * write mode:
         * 0x0:1xIO(0x02), cpu trans; 0x10:1xIO(0x02), dma trans;
         * 0x4:4xIO-quad(0x32), cpu trans; 0x14:4xIO-quad(0x32), dma trans;
         */
        0x14,                                               /* writeMode 0x10*/
        FLASH_CHIP_INFO_VERSION,
        "KANY1D4S2WD",
        41,                                                 /* delaychain */
        {0},                                                /* Reserved, 12Bytes */
    },
#endif
#ifdef GD5F1GM7UEYIG
    /* GD5F1GM7UEYIG 2KB PageSize, 128MB SPINand */
    {
        {0xc8, 0x91, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff},   /* ChipID */
        1,                                                  /* ChipCnt */
        1,                                                  /* DieCntPerChip */
        1,                                                  /* PlaneCntPerDie */
        4,                                                  /* SectNumPerPPage */
        16,                                                 /* SpareBytesPerSector */
        90,                                                 /* Frequence */
        64,                                                 /* PageNumPerPBlk */
        1024,                                               /* BlkNumPerDie */
        984,                                                /* DefaultLBlkNumPer1024PBlk */
        2048,                                               /* userMetaOffset*/
        16,                                                 /* userMetaShift*/
        4,                                                  /* userMetaSize*/
        2,                                                  /* ECCBits */
        0x30,                                               /* eccStatusMask*/
        0x20,                                               /* eccStatusErrVal*/
        16,                                                 /* readAddressBits*/
        /**
         * read mode:
         * 0x0:1xIO(0x03), cpu trans; 0x10:1xIO(0x03), dma trans;
         * 0x1:2xIO-quad(0xbb), cpu trans; 0x11:2xIO-quad(0xbb), dma trans;
         * 0x2:2xIO-dual(0x3b), cpu trans; 0x12:2xIO-dual(0x3b), dma trans;
         * 0x4:4xIO-quad(0x6b), cpu trans; 0x14:4xIO-quad(0x6b), dma trans;
         * 0x8:4xIO-dual(0xeb), cpu trans; 0x18:4xIO-dual(0xeb), dma trans. the most fast.
         */

        0x14,                                               /* readMode 0x10*/
        /**
         * write mode:
         * 0x0:1xIO(0x02), cpu trans; 0x10:1xIO(0x02), dma trans;
         * 0x4:4xIO-quad(0x32), cpu trans; 0x14:4xIO-quad(0x32), dma trans;
         */
        0x14,                                               /* writeMode 0x10*/
        FLASH_CHIP_INFO_VERSION,
        "GD5F1GM7UEYIG",
        41,                                                 /* delaychain */
        {0},                                                /* Reserved, 12Bytes */
    },
#endif
#ifdef GD5F2GM7UE
    /* GD5F1GM7UE 2KB PageSize, 256MB SPINand */
    {
        {0xc8, 0x92, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff},   /* ChipID */
        1,                                                  /* ChipCnt */
        1,                                                  /* DieCntPerChip */
        1,                                                  /* PlaneCntPerDie */
        4,                                                  /* SectNumPerPPage */
        16,                                                 /* SpareBytesPerSector */
        90,                                                 /* Frequence */
        64,                                                 /* PageNumPerPBlk */
        2048,                                               /* BlkNumPerDie */
        984,                                                /* DefaultLBlkNumPer1024PBlk */
        2048,                                               /* userMetaOffset*/
        16,                                                 /* userMetaShift*/
        4,                                                  /* userMetaSize*/
        2,                                                  /* ECCBits */
        0x30,                                               /* eccStatusMask*/
        0x20,                                               /* eccStatusErrVal*/
        16,                                                 /* readAddressBits*/
        /**
         * read mode:
         * 0x0:1xIO(0x03), cpu trans; 0x10:1xIO(0x03), dma trans;
         * 0x1:2xIO-quad(0xbb), cpu trans; 0x11:2xIO-quad(0xbb), dma trans;
         * 0x2:2xIO-dual(0x3b), cpu trans; 0x12:2xIO-dual(0x3b), dma trans;
         * 0x4:4xIO-quad(0x6b), cpu trans; 0x14:4xIO-quad(0x6b), dma trans;
         * 0x8:4xIO-dual(0xeb), cpu trans; 0x18:4xIO-dual(0xeb), dma trans. the most fast.
         */

        0x14,                                               /* readMode 0x10*/
        /**
         * write mode:
         * 0x0:1xIO(0x02), cpu trans; 0x10:1xIO(0x02), dma trans;
         * 0x4:4xIO-quad(0x32), cpu trans; 0x14:4xIO-quad(0x32), dma trans;
         */
        0x14,                                               /* writeMode 0x10*/
        FLASH_CHIP_INFO_VERSION,
        "GD5F2GM7UE",
        41,                                                 /* delaychain */
        {0},                                                /* Reserved, 12Bytes */
    },
#endif
#ifdef GD5F4GM8UExxG
    /* GD5F4GM8UExxG 2KB PageSize, 512MB SPINand 3.3v */
    {
        {0xc8, 0x95, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff},   /* ChipID */
        1,                                                  /* ChipCnt */
        1,                                                  /* DieCntPerChip */
        1,                                                  /* PlaneCntPerDie */
        4,                                                  /* SectNumPerPPage */
        16,                                                 /* SpareBytesPerSector */
        90,                                                 /* Frequence */
        64,                                                 /* PageNumPerPBlk */
        4096,                                               /* BlkNumPerDie */
        984,                                                /* DefaultLBlkNumPer1024PBlk */
        2048,                                               /* userMetaOffset*/
        16,                                                 /* userMetaShift*/
        4,                                                  /* userMetaSize*/
        2,                                                  /* ECCBits */
        0x30,                                               /* eccStatusMask*/
        0x20,                                               /* eccStatusErrVal*/
        16,                                                 /* readAddressBits*/
        /**
         * read mode:
         * 0x0:1xIO(0x03), cpu trans; 0x10:1xIO(0x03), dma trans;
         * 0x1:2xIO-quad(0xbb), cpu trans; 0x11:2xIO-quad(0xbb), dma trans;
         * 0x2:2xIO-dual(0x3b), cpu trans; 0x12:2xIO-dual(0x3b), dma trans;
         * 0x4:4xIO-quad(0x6b), cpu trans; 0x14:4xIO-quad(0x6b), dma trans;
         * 0x8:4xIO-dual(0xeb), cpu trans; 0x18:4xIO-dual(0xeb), dma trans. the most fast.
         */

        0x14,                                               /* readMode 0x10*/
        /**
         * write mode:
         * 0x0:1xIO(0x02), cpu trans; 0x10:1xIO(0x02), dma trans;
         * 0x4:4xIO-quad(0x32), cpu trans; 0x14:4xIO-quad(0x32), dma trans;
         */
        0x14,                                               /* writeMode 0x10*/
        FLASH_CHIP_INFO_VERSION,
        "GD5F4GM8UExxG",
        38,                                                 /* delaychain */
        {0},                                                /* Reserved, 12Bytes */
    },
#endif
#ifdef XT26G01C
    /* XT26G01C 2KB PageSize, 128MB SPINand, Not support now! */
    {
        {0x0b, 0x11, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff},   /* ChipID */
        1,                                                  /* ChipCnt */
        1,                                                  /* DieCntPerChip */
        1,                                                  /* PlaneCntPerDie */
        4,                                                  /* SectNumPerPPage */
        16,                                                 /* SpareBytesPerSector */
        90,                                                 /* Frequence */
        64,                                                 /* PageNumPerPBlk */
        1024,                                               /* BlkNumPerDie */
        984,                                                /* DefaultLBlkNumPer1024PBlk */
        2048,                                               /* userMetaOffset*/
        16,                                                 /* userMetaShift*/
        4,                                                  /* userMetaSize*/
        4,                                                  /* ECCBits */
        0xf0,                                               /* eccStatusMask*/
        0x80,                                               /* eccStatusErrVal*/
        16,                                                 /* readAddressBits*/
        /**
         * read mode:
         * 0x0:1xIO(0x03), cpu trans; 0x10:1xIO(0x03), dma trans;
         * 0x1:2xIO-quad(0xbb), cpu trans; 0x11:2xIO-quad(0xbb), dma trans;
         * 0x2:2xIO-dual(0x3b), cpu trans; 0x12:2xIO-dual(0x3b), dma trans;
         * 0x4:4xIO-quad(0x6b), cpu trans; 0x14:4xIO-quad(0x6b), dma trans;
         * 0x8:4xIO-dual(0xeb), cpu trans; 0x18:4xIO-dual(0xeb), dma trans. the most fast.
         */

        0x14,                                               /* readMode 0x10*/
        /**
         * write mode:
         * 0x0:1xIO(0x02), cpu trans; 0x10:1xIO(0x02), dma trans;
         * 0x4:4xIO-quad(0x32), cpu trans; 0x14:4xIO-quad(0x32), dma trans;
         */
        0x14,                                               /* writeMode 0x10*/
        FLASH_CHIP_INFO_VERSION,
        "XT26G01C",
        41,                                                  /* delaychain */
        {0},                                                /* Reserved, 12Bytes */
    },
#endif
#ifdef XT26G01D
    /* XT26G01D 2KB PageSize, 128MB SPINand. */
    {
        {0x0b, 0x31, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff},   /* ChipID */
        1,                                                  /* ChipCnt */
        1,                                                  /* DieCntPerChip */
        1,                                                  /* PlaneCntPerDie */
        4,                                                  /* SectNumPerPPage */
        32,                                                 /* SpareBytesPerSector */
        90,                                                 /* Frequence */
        64,                                                 /* PageNumPerPBlk */
        1024,                                               /* BlkNumPerDie */
        984,                                                /* DefaultLBlkNumPer1024PBlk */
        2048,                                               /* userMetaOffset*/
        16,                                                 /* userMetaShift*/
        4,                                                  /* userMetaSize*/
        2,                                                  /* ECCBits */
        0x30,                                               /* eccStatusMask*/
        0x20,                                               /* eccStatusErrVal*/
        16,                                                 /* readAddressBits*/
        /**
         * read mode:
         * 0x0:1xIO(0x03), cpu trans; 0x10:1xIO(0x03), dma trans;
         * 0x1:2xIO-quad(0xbb), cpu trans; 0x11:2xIO-quad(0xbb), dma trans;
         * 0x2:2xIO-dual(0x3b), cpu trans; 0x12:2xIO-dual(0x3b), dma trans;
         * 0x4:4xIO-quad(0x6b), cpu trans; 0x14:4xIO-quad(0x6b), dma trans;
         * 0x8:4xIO-dual(0xeb), cpu trans; 0x18:4xIO-dual(0xeb), dma trans. the most fast.
         */

        0x14,                                               /* readMode 0x10*/
        /**
         * write mode:
         * 0x0:1xIO(0x02), cpu trans; 0x10:1xIO(0x02), dma trans;
         * 0x4:4xIO-quad(0x32), cpu trans; 0x14:4xIO-quad(0x32), dma trans;
         */
        0x14,                                               /* writeMode 0x10*/
        FLASH_CHIP_INFO_VERSION,
        "XT26G01D",
        41,                                                  /* delaychain */
        {0},                                                /* Reserved, 12Bytes */
    },
#endif
#ifdef XT26G02D
    /* XT26G02D 2KB PageSize, 256MB SPINand. */
    {
        {0x0b, 0x32, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff},   /* ChipID */
        1,                                                  /* ChipCnt */
        1,                                                  /* DieCntPerChip */
        1,                                                  /* PlaneCntPerDie */
        4,                                                  /* SectNumPerPPage */
        32,                                                 /* SpareBytesPerSector */
        90,                                                 /* Frequence */
        64,                                                 /* PageNumPerPBlk */
        2048,                                               /* BlkNumPerDie */
        984,                                                /* DefaultLBlkNumPer1024PBlk */
        2048,                                               /* userMetaOffset*/
        16,                                                 /* userMetaShift*/
        4,                                                  /* userMetaSize*/
        2,                                                  /* ECCBits */
        0x30,                                               /* eccStatusMask*/
        0x20,                                               /* eccStatusErrVal*/
        16,                                                 /* readAddressBits*/
        /**
         * read mode:
         * 0x0:1xIO(0x03), cpu trans; 0x10:1xIO(0x03), dma trans;
         * 0x1:2xIO-quad(0xbb), cpu trans; 0x11:2xIO-quad(0xbb), dma trans;
         * 0x2:2xIO-dual(0x3b), cpu trans; 0x12:2xIO-dual(0x3b), dma trans;
         * 0x4:4xIO-quad(0x6b), cpu trans; 0x14:4xIO-quad(0x6b), dma trans;
         * 0x8:4xIO-dual(0xeb), cpu trans; 0x18:4xIO-dual(0xeb), dma trans. the most fast.
         */

        0x14,                                               /* readMode 0x10*/
        /**
         * write mode:
         * 0x0:1xIO(0x02), cpu trans; 0x10:1xIO(0x02), dma trans;
         * 0x4:4xIO-quad(0x32), cpu trans; 0x14:4xIO-quad(0x32), dma trans;
         */
        0x14,                                               /* writeMode 0x10*/
        FLASH_CHIP_INFO_VERSION,
        "XT26G02D",
        41,                                                  /* delaychain */
        {0},                                                /* Reserved, 12Bytes */
    },
#endif
#ifdef XT26G04D
    /* XT26G04D 2KB PageSize, 512MB SPINand. */
    {
        {0x0b, 0x33, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff},   /* ChipID */
        1,                                                  /* ChipCnt */
        1,                                                  /* DieCntPerChip */
        1,                                                  /* PlaneCntPerDie */
        8,                                                  /* SectNumPerPPage */
        32,                                                 /* SpareBytesPerSector */
        90,                                                 /* Frequence */
        64,                                                 /* PageNumPerPBlk */
        2048,                                               /* BlkNumPerDie */
        984,                                                /* DefaultLBlkNumPer1024PBlk */
        4096,                                               /* userMetaOffset*/
        16,                                                 /* userMetaShift*/
        4,                                                  /* userMetaSize*/
        2,                                                  /* ECCBits */
        0x30,                                               /* eccStatusMask*/
        0x20,                                               /* eccStatusErrVal*/
        16,                                                 /* readAddressBits*/
        /**
         * read mode:
         * 0x0:1xIO(0x03), cpu trans; 0x10:1xIO(0x03), dma trans;
         * 0x1:2xIO-quad(0xbb), cpu trans; 0x11:2xIO-quad(0xbb), dma trans;
         * 0x2:2xIO-dual(0x3b), cpu trans; 0x12:2xIO-dual(0x3b), dma trans;
         * 0x4:4xIO-quad(0x6b), cpu trans; 0x14:4xIO-quad(0x6b), dma trans;
         * 0x8:4xIO-dual(0xeb), cpu trans; 0x18:4xIO-dual(0xeb), dma trans. the most fast.
         */

        0x14,                                               /* readMode 0x10*/
        /**
         * write mode:
         * 0x0:1xIO(0x02), cpu trans; 0x10:1xIO(0x02), dma trans;
         * 0x4:4xIO-quad(0x32), cpu trans; 0x14:4xIO-quad(0x32), dma trans;
         */
        0x14,                                               /* writeMode 0x10*/
        FLASH_CHIP_INFO_VERSION,
        "XT26G04D",
        41,                                                  /* delaychain */
        {0},                                                /* Reserved, 12Bytes */
    },
#endif
#ifdef FM25S01B
    /* FM25S01B 2KB PageSize, 128MB SPINand */
    {
        {0xa1, 0xd4, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff},   /* ChipID */
        1,                                                  /* ChipCnt */
        1,                                                  /* DieCntPerChip */
        1,                                                  /* PlaneCntPerDie */
        4,                                                  /* SectNumPerPPage */
        16,                                                 /* SpareBytesPerSector */
        90,                                                 /* Frequence */
        64,                                                 /* PageNumPerPBlk */
        1024,                                               /* BlkNumPerDie */
        984,                                                /* DefaultLBlkNumPer1024PBlk */
        2048,                                               /* userMetaOffset*/
        16,                                                 /* userMetaShift*/
        4,                                                  /* userMetaSize*/
        1,                                                  /* ECCBits */
        0x30,                                               /* eccStatusMask*/
        0x10,                                               /* eccStatusErrVal*/
        16,                                                 /* readAddressBits*/
        /**
         * read mode:
         * 0x0:1xIO(0x03), cpu trans; 0x10:1xIO(0x03), dma trans;
         * 0x1:2xIO-quad(0xbb), cpu trans; 0x11:2xIO-quad(0xbb), dma trans;
         * 0x2:2xIO-dual(0x3b), cpu trans; 0x12:2xIO-dual(0x3b), dma trans;
         * 0x4:4xIO-quad(0x6b), cpu trans; 0x14:4xIO-quad(0x6b), dma trans;
         * 0x8:4xIO-dual(0xeb), cpu trans; 0x18:4xIO-dual(0xeb), dma trans. the most fast.
         */

        0x14,                                               /* readMode 0x10*/
        /**
         * write mode:
         * 0x0:1xIO(0x02), cpu trans; 0x10:1xIO(0x02), dma trans;
         * 0x4:4xIO-quad(0x32), cpu trans; 0x14:4xIO-quad(0x32), dma trans;
         */
        0x14,                                               /* writeMode 0x10*/
        FLASH_CHIP_INFO_VERSION,
        "FM25S01B",
        41,                                                 /* delaychain */
        {0},                                                /* Reserved, 12Bytes */
    },
#endif
#ifdef HYF1GQ4UT
    /* HYF1GQ4UT 2KB PageSize, 128MB SPINand */
    {
        {0x01, 0x15, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff},   /* ChipID */
        1,                                                  /* ChipCnt */
        1,                                                  /* DieCntPerChip */
        1,                                                  /* PlaneCntPerDie */
        4,                                                  /* SectNumPerPPage */
        16,                                                 /* SpareBytesPerSector */
        90,                                                 /* Frequence */
        64,                                                 /* PageNumPerPBlk */
        1024,                                                /* BlkNumPerDie */
        984,                                                /* DefaultLBlkNumPer1024PBlk */
        2048,                                               /* userMetaOffset*/
        16,                                                 /* userMetaShift*/
        4,                                                  /* userMetaSize*/
        1,                                                  /* ECCBits */
        0x30,                                               /* eccStatusMask*/
        0x20,                                               /* eccStatusErrVal*/
        16,                                                 /* readAddressBits*/
        /**
         * read mode:
         * 0x0:1xIO(0x03), cpu trans; 0x10:1xIO(0x03), dma trans;
         * 0x1:2xIO-quad(0xbb), cpu trans; 0x11:2xIO-quad(0xbb), dma trans;
         * 0x2:2xIO-dual(0x3b), cpu trans; 0x12:2xIO-dual(0x3b), dma trans;
         * 0x4:4xIO-quad(0x6b), cpu trans; 0x14:4xIO-quad(0x6b), dma trans;
         * 0x8:4xIO-dual(0xeb), cpu trans; 0x18:4xIO-dual(0xeb), dma trans. the most fast.
         */

        0x14,                                               /* readMode 0x10*/
        /**
         * write mode:
         * 0x0:1xIO(0x02), cpu trans; 0x10:1xIO(0x02), dma trans;
         * 0x4:4xIO-quad(0x32), cpu trans; 0x14:4xIO-quad(0x32), dma trans;
         */
        0x14,                                               /* writeMode 0x10*/
        FLASH_CHIP_INFO_VERSION,
        "HYF1GQ4UT",
        41,                                                 /* delaychain */
        {0},                                                /* Reserved, 12Bytes */
    },
#endif
#ifdef HYF2GQ4UT
    /* HYF2GQ4UT 2KB PageSize, 256MB SPINand */
    {
        {0x01, 0x25, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff},   /* ChipID */
        1,                                                  /* ChipCnt */
        1,                                                  /* DieCntPerChip */
        1,                                                  /* PlaneCntPerDie */
        4,                                                  /* SectNumPerPPage */
        16,                                                 /* SpareBytesPerSector */
        90,                                                 /* Frequence */
        64,                                                 /* PageNumPerPBlk */
        2048,                                                /* BlkNumPerDie */
        984,                                                /* DefaultLBlkNumPer1024PBlk */
        2048,                                               /* userMetaOffset*/
        16,                                                 /* userMetaShift*/
        4,                                                  /* userMetaSize*/
        1,                                                  /* ECCBits */
        0x30,                                               /* eccStatusMask*/
        0x20,                                               /* eccStatusErrVal*/
        16,                                                 /* readAddressBits*/
        /**
         * read mode:
         * 0x0:1xIO(0x03), cpu trans; 0x10:1xIO(0x03), dma trans;
         * 0x1:2xIO-quad(0xbb), cpu trans; 0x11:2xIO-quad(0xbb), dma trans;
         * 0x2:2xIO-dual(0x3b), cpu trans; 0x12:2xIO-dual(0x3b), dma trans;
         * 0x4:4xIO-quad(0x6b), cpu trans; 0x14:4xIO-quad(0x6b), dma trans;
         * 0x8:4xIO-dual(0xeb), cpu trans; 0x18:4xIO-dual(0xeb), dma trans. the most fast.
         */

        0x14,                                               /* readMode 0x10*/
        /**
         * write mode:
         * 0x0:1xIO(0x02), cpu trans; 0x10:1xIO(0x02), dma trans;
         * 0x4:4xIO-quad(0x32), cpu trans; 0x14:4xIO-quad(0x32), dma trans;
         */
        0x14,                                               /* writeMode 0x10*/
        FLASH_CHIP_INFO_VERSION,
        "HYF2GQ4UT",
        41,                                                 /* delaychain */
        {0},                                                /* Reserved, 12Bytes */
    },
#endif
#ifdef HYF4GQ4UT
    /* HYF4GQ4UT 2KB PageSize, 512MB SPINand */
    {
        {0x01, 0x35, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff},   /* ChipID */
        1,                                                  /* ChipCnt */
        1,                                                  /* DieCntPerChip */
        1,                                                  /* PlaneCntPerDie */
        4,                                                  /* SectNumPerPPage */
        16,                                                 /* SpareBytesPerSector */
        90,                                                 /* Frequence */
        64,                                                 /* PageNumPerPBlk */
        4096,                                                /* BlkNumPerDie */
        984,                                                /* DefaultLBlkNumPer1024PBlk */
        2048,                                               /* userMetaOffset*/
        16,                                                 /* userMetaShift*/
        4,                                                  /* userMetaSize*/
        1,                                                  /* ECCBits */
        0x30,                                               /* eccStatusMask*/
        0x20,                                               /* eccStatusErrVal*/
        16,                                                 /* readAddressBits*/
        /**
         * read mode:
         * 0x0:1xIO(0x03), cpu trans; 0x10:1xIO(0x03), dma trans;
         * 0x1:2xIO-quad(0xbb), cpu trans; 0x11:2xIO-quad(0xbb), dma trans;
         * 0x2:2xIO-dual(0x3b), cpu trans; 0x12:2xIO-dual(0x3b), dma trans;
         * 0x4:4xIO-quad(0x6b), cpu trans; 0x14:4xIO-quad(0x6b), dma trans;
         * 0x8:4xIO-dual(0xeb), cpu trans; 0x18:4xIO-dual(0xeb), dma trans. the most fast.
         */

        0x14,                                               /* readMode 0x10*/
        /**
         * write mode:
         * 0x0:1xIO(0x02), cpu trans; 0x10:1xIO(0x02), dma trans;
         * 0x4:4xIO-quad(0x32), cpu trans; 0x14:4xIO-quad(0x32), dma trans;
         */
        0x14,                                               /* writeMode 0x10*/
        FLASH_CHIP_INFO_VERSION,
        "HYF4GQ4UT",
        41,                                                 /* delaychain */
        {0},                                                /* Reserved, 12Bytes */
    },
#endif
#ifdef XCSP1AAPK
    /* XCSP1AAPK 2KB PageSize, 128MB SPINand */
    {
        {0x8c, 0x01, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff},   /* ChipID */
        1,                                                  /* ChipCnt */
        1,                                                  /* DieCntPerChip */
        1,                                                  /* PlaneCntPerDie */
        4,                                                  /* SectNumPerPPage */
        16,                                                 /* SpareBytesPerSector */
        90,                                                 /* Frequence */
        64,                                                 /* PageNumPerPBlk */
        1024,                                               /* BlkNumPerDie */
        984,                                                /* DefaultLBlkNumPer1024PBlk */
        2048,                                               /* userMetaOffset*/
        16,                                                 /* userMetaShift*/
        4,                                                  /* userMetaSize*/
        1,                                                  /* ECCBits */
        0x30,                                               /* eccStatusMask*/
        0x20,                                               /* eccStatusErrVal*/
        16,                                                 /* readAddressBits*/
        /**
         * read mode:
         * 0x0:1xIO(0x03), cpu trans; 0x10:1xIO(0x03), dma trans;
         * 0x1:2xIO-quad(0xbb), cpu trans; 0x11:2xIO-quad(0xbb), dma trans;
         * 0x2:2xIO-dual(0x3b), cpu trans; 0x12:2xIO-dual(0x3b), dma trans;
         * 0x4:4xIO-quad(0x6b), cpu trans; 0x14:4xIO-quad(0x6b), dma trans;
         * 0x8:4xIO-dual(0xeb), cpu trans; 0x18:4xIO-dual(0xeb), dma trans. the most fast.
         */

        0x14,                                               /* readMode 0x10*/
        /**
         * write mode:
         * 0x0:1xIO(0x02), cpu trans; 0x10:1xIO(0x02), dma trans;
         * 0x4:4xIO-quad(0x32), cpu trans; 0x14:4xIO-quad(0x32), dma trans;
         */
        0x14,                                               /* writeMode 0x10*/
        FLASH_CHIP_INFO_VERSION,
        "XCSP1AAPK",
        41,                                                 /* delaychain */
        {0},                                                /* Reserved, 12Bytes */
    },
#endif
#ifdef XCSP2AAPK
    /* XCSP2AAPK 2KB PageSize, 256MB SPINand */
    {
        {0x8c, 0xa1, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff},   /* ChipID */
        1,                                                  /* ChipCnt */
        1,                                                  /* DieCntPerChip */
        1,                                                  /* PlaneCntPerDie */
        4,                                                  /* SectNumPerPPage */
        16,                                                 /* SpareBytesPerSector */
        90,                                                 /* Frequence */
        64,                                                 /* PageNumPerPBlk */
        2048,                                               /* BlkNumPerDie */
        984,                                                /* DefaultLBlkNumPer1024PBlk */
        2048,                                               /* userMetaOffset*/
        16,                                                 /* userMetaShift*/
        4,                                                  /* userMetaSize*/
        2,                                                  /* ECCBits */
        0x30,                                               /* eccStatusMask*/
        0x20,                                               /* eccStatusErrVal*/
        16,                                                 /* readAddressBits*/
        /**
         * read mode:
         * 0x0:1xIO(0x03), cpu trans; 0x10:1xIO(0x03), dma trans;
         * 0x1:2xIO-quad(0xbb), cpu trans; 0x11:2xIO-quad(0xbb), dma trans;
         * 0x2:2xIO-dual(0x3b), cpu trans; 0x12:2xIO-dual(0x3b), dma trans;
         * 0x4:4xIO-quad(0x6b), cpu trans; 0x14:4xIO-quad(0x6b), dma trans;
         * 0x8:4xIO-dual(0xeb), cpu trans; 0x18:4xIO-dual(0xeb), dma trans. the most fast.
         */

        0x14,                                               /* readMode 0x10*/
        /**
         * write mode:
         * 0x0:1xIO(0x02), cpu trans; 0x10:1xIO(0x02), dma trans;
         * 0x4:4xIO-quad(0x32), cpu trans; 0x14:4xIO-quad(0x32), dma trans;
         */
        0x14,                                               /* writeMode 0x10*/
        FLASH_CHIP_INFO_VERSION,
        "XCSP2AAPK",
        41,                                                 /* delaychain */
        {0},                                                /* Reserved, 12Bytes */
    },
#endif
#ifdef XCSP4AAPK
    /* XCSP4AAPK 2KB PageSize, 512MB SPINand */
    {
        {0x8c, 0xb1, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff},   /* ChipID */
        1,                                                  /* ChipCnt */
        1,                                                  /* DieCntPerChip */
        1,                                                  /* PlaneCntPerDie */
        4,                                                  /* SectNumPerPPage */
        16,                                                 /* SpareBytesPerSector */
        90,                                                 /* Frequence */
        128,                                                 /* PageNumPerPBlk */
        2048,                                               /* BlkNumPerDie */
        984,                                                /* DefaultLBlkNumPer1024PBlk */
        2048,                                               /* userMetaOffset*/
        16,                                                 /* userMetaShift*/
        4,                                                  /* userMetaSize*/
        2,                                                  /* ECCBits */
        0x30,                                               /* eccStatusMask*/
        0x20,                                               /* eccStatusErrVal*/
        16,                                                 /* readAddressBits*/
        /**
         * read mode:
         * 0x0:1xIO(0x03), cpu trans; 0x10:1xIO(0x03), dma trans;
         * 0x1:2xIO-quad(0xbb), cpu trans; 0x11:2xIO-quad(0xbb), dma trans;
         * 0x2:2xIO-dual(0x3b), cpu trans; 0x12:2xIO-dual(0x3b), dma trans;
         * 0x4:4xIO-quad(0x6b), cpu trans; 0x14:4xIO-quad(0x6b), dma trans;
         * 0x8:4xIO-dual(0xeb), cpu trans; 0x18:4xIO-dual(0xeb), dma trans. the most fast.
         */

        0x14,                                               /* readMode 0x10*/
        /**
         * write mode:
         * 0x0:1xIO(0x02), cpu trans; 0x10:1xIO(0x02), dma trans;
         * 0x4:4xIO-quad(0x32), cpu trans; 0x14:4xIO-quad(0x32), dma trans;
         */
        0x14,                                               /* writeMode 0x10*/
        FLASH_CHIP_INFO_VERSION,
        "XCSP4AAPK",
        41,                                                 /* delaychain */
        {0},                                                /* Reserved, 12Bytes */
    },
#endif
#ifdef ZB35Q01B
    /* ZB35Q01B 2KB PageSize, 128MB SPINand, 20240528 */
    {
        {0x5e, 0xa1, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff},   /* ChipID */
        1,                                                  /* ChipCnt */
        1,                                                  /* DieCntPerChip */
        1,                                                  /* PlaneCntPerDie */
        4,                                                  /* SectNumPerPPage */
        16,                                                 /* SpareBytesPerSector */
        90,                                                 /* Frequence */
        64,                                                 /* PageNumPerPBlk */
        1024,                                               /* BlkNumPerDie */
        984,                                                /* DefaultLBlkNumPer1024PBlk */
        2048,                                               /* userMetaOffset*/
        16,                                                 /* userMetaShift*/
        4,                                                  /* userMetaSize*/
        1,                                                  /* ECCBits */
        0x30,                                               /* eccStatusMask*/
        0x20,                                               /* eccStatusErrVal*/
        16,                                                 /* readAddressBits*/
        /**
         * read mode:
         * 0x0:1xIO(0x03), cpu trans; 0x10:1xIO(0x03), dma trans;
         * 0x1:2xIO-quad(0xbb), cpu trans; 0x11:2xIO-quad(0xbb), dma trans;
         * 0x2:2xIO-dual(0x3b), cpu trans; 0x12:2xIO-dual(0x3b), dma trans;
         * 0x4:4xIO-quad(0x6b), cpu trans; 0x14:4xIO-quad(0x6b), dma trans;
         * 0x8:4xIO-dual(0xeb), cpu trans; 0x18:4xIO-dual(0xeb), dma trans. the most fast.
         */

        0x14,                                               /* readMode 0x10*/
        /**
         * write mode:
         * 0x0:1xIO(0x02), cpu trans; 0x10:1xIO(0x02), dma trans;
         * 0x4:4xIO-quad(0x32), cpu trans; 0x14:4xIO-quad(0x32), dma trans;
         */
        0x14,                                               /* writeMode 0x10*/
        FLASH_CHIP_INFO_VERSION,
        "ZB35Q01B",
        41,                                                 /* delaychain */
        {0},                                                /* Reserved, 12Bytes */
    },
#endif
#ifdef ZB35Q02B
    /* ZB35Q02B 2KB PageSize, 256MB SPINand, 20240528 */
    {
        {0x5e, 0xa2, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff},   /* ChipID */
        1,                                                  /* ChipCnt */
        1,                                                  /* DieCntPerChip */
        1,                                                  /* PlaneCntPerDie */
        4,                                                  /* SectNumPerPPage */
        16,                                                 /* SpareBytesPerSector */
        90,                                                 /* Frequence */
        64,                                                 /* PageNumPerPBlk */
        2048,                                               /* BlkNumPerDie */
        984,                                                /* DefaultLBlkNumPer1024PBlk */
        2048,                                               /* userMetaOffset*/
        16,                                                 /* userMetaShift*/
        4,                                                  /* userMetaSize*/
        1,                                                  /* ECCBits */
        0x30,                                               /* eccStatusMask*/
        0x20,                                               /* eccStatusErrVal*/
        16,                                                 /* readAddressBits*/
        /**
         * read mode:
         * 0x0:1xIO(0x03), cpu trans; 0x10:1xIO(0x03), dma trans;
         * 0x1:2xIO-quad(0xbb), cpu trans; 0x11:2xIO-quad(0xbb), dma trans;
         * 0x2:2xIO-dual(0x3b), cpu trans; 0x12:2xIO-dual(0x3b), dma trans;
         * 0x4:4xIO-quad(0x6b), cpu trans; 0x14:4xIO-quad(0x6b), dma trans;
         * 0x8:4xIO-dual(0xeb), cpu trans; 0x18:4xIO-dual(0xeb), dma trans. the most fast.
         */

        0x14,                                               /* readMode 0x10*/
        /**
         * write mode:
         * 0x0:1xIO(0x02), cpu trans; 0x10:1xIO(0x02), dma trans;
         * 0x4:4xIO-quad(0x32), cpu trans; 0x14:4xIO-quad(0x32), dma trans;
         */
        0x14,                                               /* writeMode 0x10*/
        FLASH_CHIP_INFO_VERSION,
        "ZB35Q02B",
        41,                                                 /* delaychain */
        {0},                                                /* Reserved, 12Bytes */
    },
#endif
#ifdef HSESYHDSW1G
    /* HSESYHDSW1G 2KB PageSize, 128MB SPINand, 20240528 */
    {
        {0x3c, 0xd1, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff},   /* ChipID */
        1,                                                  /* ChipCnt */
        1,                                                  /* DieCntPerChip */
        1,                                                  /* PlaneCntPerDie */
        4,                                                  /* SectNumPerPPage */
        16,                                                 /* SpareBytesPerSector */
        90,                                                 /* Frequence */
        64,                                                 /* PageNumPerPBlk */
        1024,                                               /* BlkNumPerDie */
        984,                                                /* DefaultLBlkNumPer1024PBlk */
        2048,                                               /* userMetaOffset*/
        16,                                                 /* userMetaShift*/
        4,                                                  /* userMetaSize*/
        1,                                                  /* ECCBits */
        0x30,                                               /* eccStatusMask*/
        0x20,                                               /* eccStatusErrVal*/
        16,                                                 /* readAddressBits*/
        /**
         * read mode:
         * 0x0:1xIO(0x03), cpu trans; 0x10:1xIO(0x03), dma trans;
         * 0x1:2xIO-quad(0xbb), cpu trans; 0x11:2xIO-quad(0xbb), dma trans;
         * 0x2:2xIO-dual(0x3b), cpu trans; 0x12:2xIO-dual(0x3b), dma trans;
         * 0x4:4xIO-quad(0x6b), cpu trans; 0x14:4xIO-quad(0x6b), dma trans;
         * 0x8:4xIO-dual(0xeb), cpu trans; 0x18:4xIO-dual(0xeb), dma trans. the most fast.
         */

        0x14,                                               /* readMode 0x10*/
        /**
         * write mode:
         * 0x0:1xIO(0x02), cpu trans; 0x10:1xIO(0x02), dma trans;
         * 0x4:4xIO-quad(0x32), cpu trans; 0x14:4xIO-quad(0x32), dma trans;
         */
        0x14,                                               /* writeMode 0x10*/
        FLASH_CHIP_INFO_VERSION,
        "HSESYHDSW1G",
        41,                                                 /* delaychain */
        {0},                                                /* Reserved, 12Bytes */
    },
#endif
#ifdef HSESYHDSW2G
    /* HSESYHDSW2G 2KB PageSize, 256MB SPINand, 20240528 */
    {
        {0x3c, 0xd2, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff},   /* ChipID */
        1,                                                  /* ChipCnt */
        1,                                                  /* DieCntPerChip */
        1,                                                  /* PlaneCntPerDie */
        4,                                                  /* SectNumPerPPage */
        16,                                                 /* SpareBytesPerSector */
        90,                                                 /* Frequence */
        64,                                                 /* PageNumPerPBlk */
        2048,                                               /* BlkNumPerDie */
        984,                                                /* DefaultLBlkNumPer1024PBlk */
        2048,                                               /* userMetaOffset*/
        16,                                                 /* userMetaShift*/
        4,                                                  /* userMetaSize*/
        1,                                                  /* ECCBits */
        0x30,                                               /* eccStatusMask*/
        0x20,                                               /* eccStatusErrVal*/
        16,                                                 /* readAddressBits*/
        /**
         * read mode:
         * 0x0:1xIO(0x03), cpu trans; 0x10:1xIO(0x03), dma trans;
         * 0x1:2xIO-quad(0xbb), cpu trans; 0x11:2xIO-quad(0xbb), dma trans;
         * 0x2:2xIO-dual(0x3b), cpu trans; 0x12:2xIO-dual(0x3b), dma trans;
         * 0x4:4xIO-quad(0x6b), cpu trans; 0x14:4xIO-quad(0x6b), dma trans;
         * 0x8:4xIO-dual(0xeb), cpu trans; 0x18:4xIO-dual(0xeb), dma trans. the most fast.
         */

        0x14,                                               /* readMode 0x10*/
        /**
         * write mode:
         * 0x0:1xIO(0x02), cpu trans; 0x10:1xIO(0x02), dma trans;
         * 0x4:4xIO-quad(0x32), cpu trans; 0x14:4xIO-quad(0x32), dma trans;
         */
        0x14,                                               /* writeMode 0x10*/
        FLASH_CHIP_INFO_VERSION,
        "HSESYHDSW2G",
        41,                                                 /* delaychain */
        {0},                                                /* Reserved, 12Bytes */
    },
#endif
#ifdef HSESYHDSW4G
    /* HSESYHDSW4G 2KB PageSize, 512MB SPINand, 20240528 */
    {
        {0x3c, 0xd4, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff},   /* ChipID */
        1,                                                  /* ChipCnt */
        1,                                                  /* DieCntPerChip */
        1,                                                  /* PlaneCntPerDie */
        4,                                                  /* SectNumPerPPage */
        16,                                                 /* SpareBytesPerSector */
        90,                                                 /* Frequence */
        64,                                                 /* PageNumPerPBlk */
        4096,                                               /* BlkNumPerDie */
        984,                                                /* DefaultLBlkNumPer1024PBlk */
        2048,                                               /* userMetaOffset*/
        16,                                                 /* userMetaShift*/
        4,                                                  /* userMetaSize*/
        1,                                                  /* ECCBits */
        0x30,                                               /* eccStatusMask*/
        0x20,                                               /* eccStatusErrVal*/
        16,                                                 /* readAddressBits*/
        /**
         * read mode:
         * 0x0:1xIO(0x03), cpu trans; 0x10:1xIO(0x03), dma trans;
         * 0x1:2xIO-quad(0xbb), cpu trans; 0x11:2xIO-quad(0xbb), dma trans;
         * 0x2:2xIO-dual(0x3b), cpu trans; 0x12:2xIO-dual(0x3b), dma trans;
         * 0x4:4xIO-quad(0x6b), cpu trans; 0x14:4xIO-quad(0x6b), dma trans;
         * 0x8:4xIO-dual(0xeb), cpu trans; 0x18:4xIO-dual(0xeb), dma trans. the most fast.
         */

        0x14,                                               /* readMode 0x10*/
        /**
         * write mode:
         * 0x0:1xIO(0x02), cpu trans; 0x10:1xIO(0x02), dma trans;
         * 0x4:4xIO-quad(0x32), cpu trans; 0x14:4xIO-quad(0x32), dma trans;
         */
        0x14,                                               /* writeMode 0x10*/
        FLASH_CHIP_INFO_VERSION,
        "HSESYHDSW4G",
        41,                                                 /* delaychain */
        {[1]=0x3},                                                /* Reserved, 12Bytes */
    },
#endif
};

uint32_t checksum(void *buffer, uint32_t len)
{
    uint32_t *pdat = buffer;
    uint32_t ck32 = 0;

    while (len > 0) {
        ck32 += *pdat++;
        len -= 4;
    }

    return ck32;
}

int main(int argc, char *argv[])
{
    FILE *fp;
    int ret, i;
    struct NandIdTblHeader TblHeader;

    uint32_t id_num = ARRAY_SIZE(FlashChipInfoTbl);

    fprintf(stdout, "Generate nand id table: %s\n", ID_TABLE_NAME);

    if ((id_num == 0) || (id_num > MAX_ID_TABLE_NUMBER)) {
        fprintf(stderr, "invalid flash id number %d", id_num);
        return -EINVAL;
    }

    TblHeader.num = id_num;
    //Notice: magic value is stabled and can't be changed!
    TblHeader.magic = 0x53648673;
    fprintf(stdout, "sizeof(chiptbl) = %ld\n", sizeof(FlashChipInfoTbl));
    TblHeader.checksum = checksum((void *)FlashChipInfoTbl, sizeof(FlashChipInfoTbl));

    for (i = 0; i < id_num; i++)
        fprintf(stdout, "[%d] chipid:0x%lx chipname:%s\n",
            i, *(uint64_t *)FlashChipInfoTbl[i].ChipID, FlashChipInfoTbl[i].FlashMark);

    fp = fopen(ID_TABLE_NAME, "wb");
    if (!fp) {
        fprintf(stderr, "failed to open id table:%s, error=%s", ID_TABLE_NAME, strerror(errno));
        return -errno;
    }

    ret = fwrite(&TblHeader, sizeof(TblHeader), 1, fp);
    ret = fwrite(FlashChipInfoTbl, sizeof(FlashChipInfoTbl), 1, fp);
    if (ret != 1) {
        fprintf(stderr, "write nand id table failed, error=%s", strerror(errno));
        fclose(fp);
        return -errno;
    }

    fclose(fp);

    return 0;
}
