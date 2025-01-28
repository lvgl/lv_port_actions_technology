/*
 * Copyright (c) 2017 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief common code for SPI memory (NOR & NAND & PSRAM)
 */

#ifndef __SPIMEM_H__
#define __SPIMEM_H__

#define SPIMEM_TFLAG_MIO_DATA		0x01
#define SPIMEM_TFLAG_MIO_ADDR_DATA	0x02
#define SPIMEM_TFLAG_MIO_CMD_ADDR_DATA	0x04
#define SPIMEM_TFLAG_MIO_MASK		0x07
#define SPIMEM_TFLAG_WRITE_DATA		0x08
#define SPIMEM_TFLAG_ENABLE_RANDOMIZE	0x10
#define SPIMEM_TFLAG_PAUSE_RANDOMIZE	0x20
#define SPIMEM_TFLAG_RESUME_RANDOMIZE	0x40

void spimem_read_chipid(struct spi_info *si, void *chipid, int len);
unsigned char spimem_read_status(struct spi_info *si, unsigned char cmd);

void spimem_read_page(struct spi_info *si,
        unsigned int addr, int addr_len,
        void *buf, int len);
void spimem_continuous_read_reset(struct spi_info *si);

void spimem_set_write_protect(struct spi_info *si, int protect);
int spimem_erase_block(struct spi_info *si, unsigned int page);

int spimem_transfer(struct spi_info *si, unsigned char cmd, unsigned int addr,
            int addr_len, void *buf, int length,
            unsigned char dummy_len, unsigned int flag);

int spimem_write_cmd_addr(struct spi_info *si, unsigned char cmd,
              unsigned int addr, int addr_len);

int spimem_write_cmd(struct spi_info *si, unsigned char cmd);

#endif	/* __SPIMEM_H__ */
