/*
 * Copyright (c) 2021 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief The common SPI NAND chip infomation structure.
 */

#ifndef __ID_TBL_H__
#define __ID_TBL_H__

#define NAND_CHIPID_LENGTH              (8)     /* nand flash chipid length */

#define FLASH_CHIP_INFO_VERSION         (0x01) /* ChipInfoFile Version define */

#define FLASH_TYPE_SPI_NAND             (0x00) /* ChipInfoFile Version define */

/*
 * struct FlashChipInfo takes total 60 bytes size
 * read mode:
 * 0x0:1xIO(0x03), cpu trans; 0x10:1xIO(0x03), dma trans;
 * 0x1:2xIO-quad(0xbb), cpu trans; 0x11:2xIO-quad(0xbb), dma trans;
 * 0x2:2xIO-dual(0x3b), cpu trans; 0x12:2xIO-dual(0x3b), dma trans;
 * 0x4:4xIO-quad(0x6b), cpu trans; 0x14:4xIO-quad(0x6b), dma trans;
 * 0x8:4xIO-dual(0xeb), cpu trans; 0x18:4xIO-dual(0xeb), dma trans.
 * write mode:
 * 0x0:1xIO(0x02), cpu trans; 0x10:1xIO(0x02), dma trans;
 * 0x4:4xIO-quad(0x32), cpu trans; 0x14:4xIO-quad(0x32), dma trans;
 */
struct FlashChipInfo
{
    uint8_t  ChipID[NAND_CHIPID_LENGTH]; /* nand flash id */
    uint8_t  ChipCnt;                    /* the count of chip connecting in flash*/
    uint8_t  DieCntPerChip;              /* the die count in a chip */
    uint8_t  PlaneCntPerDie;             /* the plane count in a die */
    uint8_t  SectorNumPerPage;             /* page size, based on sector */
    uint8_t  SpareBytesPerSector;        /* spare bytes per sector */
    uint8_t  Frequence;                  /* spi clk, may be based on xMHz */
    uint16_t PageNumPerPhyBlk;              /* the page number of physic block */
    uint16_t TotalBlkNumPerDie;               /* total number of the physic block in a die */
    uint16_t DefaultLBlkNumPer1024Blk;   /* Logical number per 1024 physical block */
    uint16_t userMetaOffset;             /*user meta data offset, add for spinand*/
    uint8_t  userMetaShift;              /*user meta data shift, add for spinand*/
    uint8_t  userMetaSize;               /*user meta data size, add for spinand*/
    uint8_t  ECCBits;
    uint8_t  eccStatusMask;              /*ecc status mask, add for spinand*/
    uint8_t  eccStatusErrVal;            /*ecc status err value, add for spinand*/
    uint8_t  readAddressBits;            /*read address bit, 16bit or 24bit address, add for spinand*/
    uint8_t  readMode;
    uint8_t  writeMode;
    uint8_t  Version;                    /* flash chipinfo version */
    uint8_t  FlashMark[16];              /* the name of spinand */
    uint8_t  delayChain;                 /* the delaychain of spinand, value from 0 to 63 */
    uint8_t  Reserved1[12];              /* reserved */
} __attribute__ ((packed));

struct NandIdTblHeader
{
    uint8_t num;
    uint32_t magic;
    uint32_t checksum;
    uint8_t  Reserved;
} __attribute__ ((packed));

#endif /* __ID_TBL_H__ */
