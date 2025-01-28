/*
 * Copyright (c) 2017 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief SPINOR Flash driver for LARK
 */
#include <irq.h>
#include "spi_internal.h"
#include "spimem.h"
#include "../spi_flash.h"
#include <board_cfg.h>


/* spinor parameters */
#define SPINOR_WRITE_PAGE_SIZE_BITS	8
#define SPINOR_ERASE_SECTOR_SIZE_BITS	12
#define SPINOR_ERASE_BLOCK_SIZE_BITS	16

#define SPINOR_WRITE_PAGE_SIZE		(1 << SPINOR_WRITE_PAGE_SIZE_BITS)
#define SPINOR_ERASE_SECTOR_SIZE	(1 << SPINOR_ERASE_SECTOR_SIZE_BITS)
#define SPINOR_ERASE_BLOCK_SIZE		(1 << SPINOR_ERASE_BLOCK_SIZE_BITS)

#define SPINOR_WRITE_PAGE_MASK		(SPINOR_WRITE_PAGE_SIZE - 1)
#define SPINOR_ERASE_SECTOR_MASK	(SPINOR_ERASE_SECTOR_SIZE - 1)
#define SPINOR_ERASE_BLOCK_MASK		(SPINOR_ERASE_BLOCK_SIZE - 1)

/* spinor commands */
#define	SPINOR_CMD_WRITE_PAGE		0x02	/* write one page */
#define	SPINOR_CMD_DISABLE_WRITE	0x04	/* disable write */
#define	SPINOR_CMD_READ_STATUS		0x05	/* read status1 */
#define	SPINOR_CMD_READ_STATUS2		0x35	/* read status2 */
#define	SPINOR_CMD_READ_STATUS3		0x15	/* read status3 */
#define	SPINOR_CMD_WRITE_STATUS		0x01	/* write status1 */
#define	SPINOR_CMD_WRITE_STATUS2	0x31	/* write status2 */
#define	SPINOR_CMD_WRITE_STATUS3	0x11	/* write status3 */
#define	SPINOR_CMD_ENABLE_WRITE		0x06	/* enable write */
#define	SPINOR_CMD_FAST_READ		0x0b	/* fast read */
#define SPINOR_CMD_ERASE_SECTOR		0x20	/* 4KB erase */
#define SPINOR_CMD_ERASE_BLOCK_32K	0x52	/* 32KB erase */
#define SPINOR_CMD_ERASE_BLOCK		0xd8	/* 64KB erase */
#define	SPINOR_CMD_READ_CHIPID		0x9f	/* JEDEC ID */
#define	SPINOR_CMD_DISABLE_QSPI		0xff	/* disable QSPI */
#define	SPINOR_CMD_PROGRAM_ERASE_RESUME		0x7a  	/* nor resume */
#define	SPINOR_CMD_PROGRAM_ERASE_SUSPEND	0x75   /* nor suspend */



#define SPINOR_CMD_SECURITY_ERASE   0x44	/* erase security registers cmd*/
#define SPINOR_CMD_SECURITY_PROGRAM 0x42	/* program security registers cmd*/
#define SPINOR_CMD_SECURITY_READ    0x48	/* read security registers cmd*/
#define SPINOR_CMD_UID_READ   		0x4B	/* Read Unique ID cmd*/




#define SPINOR_CMD_EN4B              0xB7	/* enter 4-byte address mode */
#define SPINOR_CMD_EXIT4B            0xE9	/* exit 4-byte address mode */
#define SPINOR_CMD_WR_EXTADDR        0xC5	/* write extern address */

#define SPINOR_CMD_WR_NONVOL_CFG	 0xb1  /*Write Nonvolatile Configuration Registe*/
#define SPINOR_CMD_RD_NONVOL_CFG	 0xb5  /*read Nonvolatile Configuration Registe*/
#define SPINOR_CMD_WR_VOL_CFG	 	 0x81  /*Write volatile Configuration Registe*/
#define SPINOR_CMD_RD_VOL_CFG	 	 0x85  /*read  volatile Configuration Registe*/



/* spinor 4byte address commands */
#define	SPINOR_CMD_WRITE_PAGE_4B	0x12	/* write one page by 4bytes address */
#define SPINOR_CMD_ERASE_SECTOR_4B	0x21	/* 4KB erase by 4bytes address */
#define SPINOR_CMD_ERASE_BLOCK_4B	0xdc	/* 64KB erase by 4bytes address */
#define SPINOR_CMD_WIRTE_EXT_ADDR_R	0xc5	/* write Extended Addr. Register  cmd+1BYTE(ext addr)*/
#define SPINOR_CMD_READ_EXT_ADDR_R	0xc8	/* Read Extended Addr. Register  */



/* NOR Flash vendors ID */
#define SPINOR_MANU_ID_ALLIANCE		0x52	/* Alliance Semiconductor */
#define SPINOR_MANU_ID_AMD			0x01	/* AMD */
#define SPINOR_MANU_ID_AMIC			0x37	/* AMIC */
#define SPINOR_MANU_ID_ATMEL		0x1f	/* ATMEL */
#define SPINOR_MANU_ID_CATALYST		0x31	/* Catalyst */
#define SPINOR_MANU_ID_ESMT			0x8c	/* ESMT */
#define SPINOR_MANU_ID_EON			0x1c	/* EON */
#define SPINOR_MANU_ID_FD_MICRO		0xa1	/* shanghai fudan microelectronics */
#define SPINOR_MANU_ID_FIDELIX		0xf8	/* FIDELIX */
#define SPINOR_MANU_ID_FMD			0x0e	/* Fremont Micro Device(FMD) */
#define SPINOR_MANU_ID_FUJITSU		0x04	/* Fujitsu */
#define SPINOR_MANU_ID_GIGADEVICE	0xc8	/* GigaDevice */
#define SPINOR_MANU_ID_GIGADEVICE2	0x51	/* GigaDevice2 */
#define SPINOR_MANU_ID_HYUNDAI		0xad	/* Hyundai */
#define SPINOR_MANU_ID_INTEL		0x89	/* Intel */
#define SPINOR_MANU_ID_MACRONIX		0xc2	/* Macronix (MX) */
#define SPINOR_MANU_ID_NANTRONIC	0xd5	/* Nantronics */
#define SPINOR_MANU_ID_NUMONYX		0x20	/* Numonyx, Micron, ST */
#define SPINOR_MANU_ID_PMC			0x9d	/* PMC */
#define SPINOR_MANU_ID_SANYO		0x62	/* SANYO */
#define SPINOR_MANU_ID_SHARP		0xb0	/* SHARP */
#define SPINOR_MANU_ID_SPANSION		0x01	/* SPANSION */
#define SPINOR_MANU_ID_SST			0xbf	/* SST */
#define SPINOR_MANU_ID_SYNCMOS_MVC	0x40	/* SyncMOS (SM) and Mosel Vitelic Corporation (MVC) */
#define SPINOR_MANU_ID_TI			0x97	/* Texas Instruments */
#define SPINOR_MANU_ID_WINBOND		0xda	/* Winbond */
#define SPINOR_MANU_ID_WINBOND_NEX	0xef	/* Winbond (ex Nexcom) */
#define SPINOR_MANU_ID_ZH_BERG		0xe0	/* ZhuHai Berg microelectronics (Bo Guan) */

//#define SPINOR_FLAG_UNLOCK_IRQ_WAIT_READY		(1 << 0)

#define NOR_DELAY_CHAIN 		    0x8

/* system XIP spinor */
static const struct spinor_info system_spi_nor = {
    .spi = {
        .base = SPI0_REG_BASE,
        .bus_width = 1,
        .delay_chain = NOR_DELAY_CHAIN,
        .flag = 0,
#if 0
        .dma_base= 0x4001C600, //DMA5
#endif
    }
};

_nor_fun static unsigned int spinor_read_status(struct spinor_info *sni, unsigned char cmd)
{
    if (!sni)
        sni = (struct spinor_info *)&system_spi_nor;

    return spimem_read_status(&sni->spi, cmd);
}

_nor_fun static int spinor_wait_ready(struct spinor_info *sni)
{
    unsigned char status;

    while (1) {
        status = spinor_read_status(sni, SPINOR_CMD_READ_STATUS);
        if (!(status & 0x1))
            break;
    }

    return 0;
}

_nor_fun static void spinor_write_data(struct spinor_info *sni, unsigned char cmd,
            unsigned int addr, int addr_len, const unsigned char *buf, int len)
{
    struct spi_info *si = &sni->spi;
    unsigned int key = 0;

    if (!(si->flag & SPI_FLAG_NO_IRQ_LOCK)) {
        key = irq_lock();
    }

    spimem_set_write_protect(si, 0);
    spimem_transfer(si, cmd, addr, addr_len, (unsigned char *)buf, len,
            0, SPIMEM_TFLAG_WRITE_DATA);

	if(sni->flag & SPINOR_FLAG_NO_WAIT_READY){
		if (!(si->flag & SPI_FLAG_NO_IRQ_LOCK))
			irq_unlock(key);
	}else{
	    if (!(si->flag & SPI_FLAG_NO_IRQ_LOCK)) {
	        if (sni->flag & SPINOR_FLAG_UNLOCK_IRQ_WAIT_READY) {
	            irq_unlock(key);
	            spinor_wait_ready(sni);
	        } else {
	            spinor_wait_ready(sni);
	            irq_unlock(key);
	        }
	    }else{
	        spinor_wait_ready(sni);
	    }
	}

}
_nor_fun static void spinor_write_status(struct spinor_info *sni, unsigned char cmd,
			unsigned char *status, int len)
{
	if (!sni)
		sni = (struct spinor_info *)&system_spi_nor;

	spinor_write_data(sni, cmd, 0, 0, status, len);
    spinor_wait_ready(sni);
}
_nor_fun static int spinor_erase_internal(struct spinor_info *sni,
            unsigned char cmd, unsigned int addr)
{
	if (sni->flag & SPINOR_FLAG_4BYTE_ADDRESS_MODE_EN) {
		spinor_write_data(sni, cmd, addr, 4, 0, 0);
	} else {
		spinor_write_data(sni, cmd, addr, 3, 0, 0);
	}

	return 0;
}

_nor_fun static int spinor_write_internal(struct spinor_info *sni,
            unsigned int addr, const unsigned char *buf, int len)
{
	if (sni->flag & SPINOR_FLAG_4BYTE_ADDRESS_MODE_EN) {
		spinor_write_data(sni, SPINOR_CMD_WRITE_PAGE_4B, addr, 4, buf, len);
	} else {
		spinor_write_data(sni, SPINOR_CMD_WRITE_PAGE, addr, 3, buf, len);
	}

	return 0;
}

_nor_fun static int spinor_read_internal(struct spinor_info *sni,
            unsigned int addr, unsigned char *buf, int len)
{
	if (sni->flag & SPINOR_FLAG_4BYTE_ADDRESS_MODE_EN) {
		spimem_read_page(&sni->spi, addr, 4, buf, len);
	} else {
		spimem_read_page(&sni->spi, addr, 3, buf, len);
	}
	return 0;
}

_nor_fun int spinor_read(struct spinor_info *sni, unsigned int addr, void *data, int len)
{
    if (!len)
        return 0;

    if (!sni)
        sni = (struct spinor_info *)&system_spi_nor;

    spinor_read_internal(sni, addr, data, len);

    return 0;
}

_nor_fun int spinor_write(struct spinor_info *sni, unsigned int addr,
            const void *data, int len)
{
    int unaligned_len, remaining, write_size;

    if (!len)
        return 0;

    if (!sni)
        sni = (struct spinor_info *)&system_spi_nor;

    /* unaligned write? */
    if (addr & SPINOR_WRITE_PAGE_MASK)
        unaligned_len = SPINOR_WRITE_PAGE_SIZE - (addr & SPINOR_WRITE_PAGE_MASK);
    else
        unaligned_len = 0;

    remaining = len;
    while (remaining > 0) {
        if (unaligned_len) {
            /* write unaligned page data */
            if (unaligned_len > len)
                write_size = len;
            else
                write_size = unaligned_len;
            unaligned_len = 0;
        } else if (remaining < SPINOR_WRITE_PAGE_SIZE)
            write_size = remaining;
        else
            write_size = SPINOR_WRITE_PAGE_SIZE;

        spinor_write_internal(sni, addr, data, write_size);

        addr += write_size;
        data = (unsigned char *)data+write_size;
        remaining -= write_size;
    }

    return 0;
}

_nor_fun int spinor_write_with_randomizer(struct spinor_info *sni, unsigned int addr, const void *data, int len)
{
    struct spi_info *si;
    u32_t key, page_addr, origin_spi_ctl;
    int wlen;
    unsigned char addr_len;
    unsigned char cmd;

    if (!sni)
        sni = (struct spinor_info *)&system_spi_nor;

    si = &sni->spi;

    key = irq_lock(); //ota diff upgrade, must be irq lock

    origin_spi_ctl = spi_read(si, SSPI_CTL);

    spimem_set_write_protect(si, 0);

    page_addr = (addr + len) & ~SPINOR_WRITE_PAGE_MASK;
    if ((addr & ~SPINOR_WRITE_PAGE_MASK) != page_addr) {
        /* data cross write page bound, need split */
        wlen = page_addr - addr;

		if (sni->flag & SPINOR_FLAG_4BYTE_ADDRESS_MODE_EN) {
			addr_len = 4;
			cmd = SPINOR_CMD_WRITE_PAGE_4B;
		} else {
			addr_len = 3;
			cmd = SPINOR_CMD_WRITE_PAGE;
		}

        spimem_transfer(si, cmd, addr, addr_len, (u8_t *)data, wlen,
                  0, SPIMEM_TFLAG_WRITE_DATA | SPIMEM_TFLAG_ENABLE_RANDOMIZE |
                  SPIMEM_TFLAG_PAUSE_RANDOMIZE);
        spinor_wait_ready(sni);

        data = (unsigned char *)data + wlen;
        len -= wlen;
        addr = page_addr;

        spimem_set_write_protect(si, 0);
    }

	if (sni->flag & SPINOR_FLAG_4BYTE_ADDRESS_MODE_EN) {
		addr_len = 4;
		cmd = SPINOR_CMD_WRITE_PAGE_4B;
	} else {
		addr_len = 3;
		cmd = SPINOR_CMD_WRITE_PAGE;
	}

    spimem_transfer(si, cmd, addr, addr_len, (u8_t *)data, len,
              0, SPIMEM_TFLAG_WRITE_DATA | SPIMEM_TFLAG_ENABLE_RANDOMIZE | SPIMEM_TFLAG_RESUME_RANDOMIZE);
    spinor_wait_ready(sni);

    spi_write(si, SSPI_CTL, origin_spi_ctl);
    spi_delay();

    irq_unlock(key);

    return 0;

}
#ifdef CONFIG_NOR_SECURIYT_SUSPPORT
_nor_fun int spinor_erase_security(struct spinor_info *sni, unsigned int addr)
{
    struct spi_info *si;
    u32_t key, origin_spi_ctl;
    si = &sni->spi;
    key = irq_lock(); //ota diff upgrade, must be irq lock
    origin_spi_ctl = spi_read(si, SSPI_CTL);
    spimem_set_write_protect(si, 0);
    spimem_transfer(si, SPINOR_CMD_SECURITY_ERASE, addr, 3, 0, 0, 0, SPIMEM_TFLAG_WRITE_DATA);
    spinor_wait_ready(sni);
    spi_write(si, SSPI_CTL, origin_spi_ctl);
    spi_delay();
    irq_unlock(key);
    return 0;
}

_nor_fun int spinor_write_security(struct spinor_info *sni, unsigned int addr, const void *data, int len)
{
    struct spi_info *si;
    u32_t key, origin_spi_ctl;
    si = &sni->spi;
    key = irq_lock();
    origin_spi_ctl = spi_read(si, SSPI_CTL);
    spimem_set_write_protect(si, 0);
    spimem_transfer(si, SPINOR_CMD_SECURITY_PROGRAM, addr, 3, (u8_t *)data, len, 0, SPIMEM_TFLAG_WRITE_DATA);
    spinor_wait_ready(sni);
    spi_write(si, SSPI_CTL, origin_spi_ctl);
    spi_delay();
    irq_unlock(key);
    return 0;
}
_nor_fun int spinor_read_security(struct spinor_info *sni, unsigned int addr,  void *data, int len)
{
    struct spi_info *si;
    u32_t key, origin_spi_ctl;
    si = &sni->spi;
    key = irq_lock();
    origin_spi_ctl = spi_read(si, SSPI_CTL);
    spimem_transfer(si, SPINOR_CMD_SECURITY_READ, addr, 3, (u8_t *)data, len, 1, 0);
    spi_write(si, SSPI_CTL, origin_spi_ctl);
    irq_unlock(key);
    return 0;
}

_nor_fun int spinor_read_uid(struct spinor_info *sni,  void *data, int len)
{
    struct spi_info *si;
    u32_t key, origin_spi_ctl;
    si = &sni->spi;
    key = irq_lock();
    origin_spi_ctl = spi_read(si, SSPI_CTL);
    spimem_transfer(si, SPINOR_CMD_UID_READ, 0, 3, (u8_t *)data, len, 1, 0);
    spi_write(si, SSPI_CTL, origin_spi_ctl);
    irq_unlock(key);
    return 0;
}
#endif

_nor_fun int spinor_enter_4byte_address_mode(struct spinor_info *sni)
{
#ifdef CONFIG_NOR_CODE_IN_RAM
	printk("spinor code in ram\n");
	return 0;
#else
    printk("spinor enter 4-byte address mode\n");
	sni->flag |= SPINOR_FLAG_4BYTE_ADDRESS_MODE_EN;
    return spimem_transfer(&sni->spi, SPINOR_CMD_EN4B, 0, 0, NULL, 0, 0, 0);
#endif
}

_nor_fun int spinor_erase(struct spinor_info *sni, unsigned int addr, int size)
{
    int remaining, erase_size;
    unsigned char cmd;

    if (!size)
        return 0;

    if (addr & SPINOR_ERASE_SECTOR_MASK || size & SPINOR_ERASE_SECTOR_MASK)
        return -1;

    if (!sni)
        sni = (struct spinor_info *)&system_spi_nor;

    /* write aligned page data */
    remaining = size;
    while (remaining > 0) {
        if (addr & SPINOR_ERASE_BLOCK_MASK || remaining < SPINOR_ERASE_BLOCK_SIZE) {
			if (sni->flag & SPINOR_FLAG_4BYTE_ADDRESS_MODE_EN) {
				cmd = SPINOR_CMD_ERASE_SECTOR_4B;
			} else {
				cmd = SPINOR_CMD_ERASE_SECTOR;
			}
            erase_size = SPINOR_ERASE_SECTOR_SIZE;
        } else {
			if (sni->flag & SPINOR_FLAG_4BYTE_ADDRESS_MODE_EN) {
				cmd = SPINOR_CMD_ERASE_BLOCK_4B;
			} else {
				cmd = SPINOR_CMD_ERASE_BLOCK;
			}
            erase_size = SPINOR_ERASE_BLOCK_SIZE;
        }

        spinor_erase_internal(sni, cmd, addr);

        addr += erase_size;
        remaining -= erase_size;
    }

    return 0;
}
_nor_fun unsigned int spinor_read_chipid(struct spinor_info *sni)
{
	unsigned int chipid;

	if (!sni)
		sni = (struct spinor_info *)&system_spi_nor;

	spimem_read_chipid(&sni->spi, &chipid, 3);

	return chipid;
}


_nor_fun static int spinor_write_vol_cfg(struct spinor_info *sni, unsigned int addr, const void *data, int len)
{
    struct spi_info *si;
    u32_t key, origin_spi_ctl;
    si = &sni->spi;
    key = irq_lock();
    origin_spi_ctl = spi_read(si, SSPI_CTL);
    spimem_set_write_protect(si, 0);
    spimem_transfer(si, SPINOR_CMD_WR_VOL_CFG, addr, 3, (u8_t *)data, len, 0, SPIMEM_TFLAG_WRITE_DATA);
    spinor_wait_ready(sni);
    spi_write(si, SSPI_CTL, origin_spi_ctl);
    spi_delay();
    irq_unlock(key);
    return 0;
}
_nor_fun static int spinor_read_vol_cfg(struct spinor_info *sni, unsigned int addr,  void *data, int len)
{
    struct spi_info *si;
    u32_t key, origin_spi_ctl;
    si = &sni->spi;
    key = irq_lock();
    origin_spi_ctl = spi_read(si, SSPI_CTL);
    spimem_transfer(si, SPINOR_CMD_RD_VOL_CFG, addr, 3, (u8_t *)data, len, 1, 0);
    spi_write(si, SSPI_CTL, origin_spi_ctl);
    irq_unlock(key);
    return 0;
}
#define GD25B512ME_CHIPID  0x1a47c8

void spinor_enable_xip_mode(struct spinor_info *sni)
{
	uint8_t ncfg;
	if(sni->chipid == GD25B512ME_CHIPID){
		spinor_read_vol_cfg(sni, 6, &ncfg, 1);
		//printk("b vol cfg=0x%x\n", ncfg);
		if(ncfg & 0x1) {
			ncfg &=0xfe;/*xip enable*/
			spinor_write_vol_cfg(sni, 6, &ncfg, 1);
			ncfg = 0x0;
		}
		spinor_read_vol_cfg(sni, 6, &ncfg, 1);
		//printk("vol cfg=0x%x\n", ncfg);
	}
}


#ifdef CONFIG_NOR_CODE_IN_RAM
#include "soc.h"
#define  XIP_CODE_ADDR 	(CONFIG_FLASH_BASE_ADDRESS + CONFIG_FLASH_LOAD_OFFSET)
#define  NOR_3B_ADDR_MAXLEN (1<<24)   // 16MB

static unsigned int xip_nor_offset;

unsigned int spi_nor_get_xip_offset(void)
{
	return xip_nor_offset;
}

#define PY25Q256HB_CHIPID  0x192085
_nor_fun static void spinor_write_ext_addr_reg(struct spinor_info *sni, unsigned char ex_addr)
{
	spimem_set_write_protect(&sni->spi, 0);
	spimem_write_cmd_addr(&sni->spi, SPINOR_CMD_WR_EXTADDR, ex_addr, 1);	
}



_nor_fun static void spinor_4byte_address_mode(struct spinor_info *sni, bool enter)
{
	if(enter){
		sni->flag |= SPINOR_FLAG_4BYTE_ADDRESS_MODE_EN;
	    //spimem_write_cmd(&sni->spi, SPINOR_CMD_EN4B);
	}else{
		sni->flag &= ~SPINOR_FLAG_4BYTE_ADDRESS_MODE_EN;
	    //spimem_write_cmd(&sni->spi, SPINOR_CMD_EXIT4B);
	    if(sni->chipid == PY25Q256HB_CHIPID) // fix PY25Q256HB nor ic bug
			spinor_write_ext_addr_reg(sni, 0);
	}
}

#ifdef CONFIG_SPINOR_TEST_DELAYCHAIN
#undef  CONFIG_NOR_SUSPEND_RESUME
#else
#define CONFIG_NOR_XIP_READ
#endif

#ifdef CONFIG_NOR_XIP_READ
int spinor_xip_read(unsigned int addr, void *data, int len)
{
	unsigned int xip_start;
	xip_start = 0x10000000 + addr - xip_nor_offset;
	pbrom_libc_api->p_memcpy(data, (void *)xip_start, len);
	return 0;
}
_nor_fun int spinor_ram_read(struct spinor_info *sni, unsigned int addr, void *data, int len)
{
	int ret;
	unsigned int tlen;
	uint32_t key;

	if(addr < xip_nor_offset){
		tlen = xip_nor_offset - addr;
		if(tlen > len)
			tlen = len;
		key = irq_lock();
		spinor_read(sni, addr, data, tlen);
		irq_unlock(key);
		data =(void *) ((unsigned int)data + tlen);
		len -= tlen;
		addr += tlen;
	}

	if(len <= 0)
		return 0;
	
	if(addr+len > NOR_3B_ADDR_MAXLEN){
		if(addr < NOR_3B_ADDR_MAXLEN){
			tlen = NOR_3B_ADDR_MAXLEN - addr;
			spinor_xip_read(addr, data, tlen);
			data =(void *) ((unsigned int)data + tlen);
			len -= tlen;
			addr = NOR_3B_ADDR_MAXLEN;
		}
		key = irq_lock();
		spinor_4byte_address_mode(sni, true);

		ret = spinor_read(sni, addr, data, len);

		spinor_4byte_address_mode(sni, false);
		irq_unlock(key);
	}else{
		ret = spinor_xip_read(addr, data, len);
	}

	return ret;
}

static int spinor_xip_init(const struct device *arg)
{
	xip_nor_offset = soc_boot_get_info()->nor_offset & (NOR_3B_ADDR_MAXLEN-1);
	printk("xip_nor_offset=0x%x-0x%x\n", xip_nor_offset, XIP_CODE_ADDR);
	return 0;
}

SYS_INIT(spinor_xip_init, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_OBJECTS);

#else
_nor_fun int spinor_ram_read(struct spinor_info *sni, unsigned int addr, void *data, int len)
{
	int ret;
	uint32_t key;
	key = irq_lock();
	if(addr+len > NOR_3B_ADDR_MAXLEN)
		spinor_4byte_address_mode(sni, true);
	ret = spinor_read(sni, addr, data, len);
	if(addr+len > NOR_3B_ADDR_MAXLEN)
		spinor_4byte_address_mode(sni, false);
	irq_unlock(key);
	return ret;
}
#endif


#ifdef CONFIG_NOR_SUSPEND_RESUME
_nor_fun static void spinor_suspend(struct spinor_info *sni)
{
	int i, j;
	// program/erase suspend
	for(j = 0; j < 3; j++){
		spimem_write_cmd(&sni->spi, SPINOR_CMD_PROGRAM_ERASE_SUSPEND);
		for(i = 0; i < 100; i++) { //max 500us, tSUS must 30us
			soc_udelay(5); 
			if (0 == (spinor_read_status(sni, SPINOR_CMD_READ_STATUS) & 0x1)){
				break;
			}
		}
		if(i != 100){
			break;
		}
	}
	
}

_nor_fun static bool spinor_resume_and_check_idle(struct spinor_info *sni)
{
	bool ret;
	uint32_t key, i;
	key = irq_lock();
	// program/erase resum
	spimem_write_cmd(&sni->spi, SPINOR_CMD_PROGRAM_ERASE_RESUME);
	for(i = 0; i < 100; i++){ // wait to exit suspend
		soc_udelay(5);
		if (0 == (spinor_read_status(sni, SPINOR_CMD_READ_STATUS2) & 0x80)){
			break;
		}
	}

	if (0 == (spinor_read_status(sni, SPINOR_CMD_READ_STATUS) & 0x1)) {
		ret = true;
	}else {
		for(i = 0; i < 10; i++){ // handle 500 us
			soc_udelay(50);
			if (0 == (spinor_read_status(sni, SPINOR_CMD_READ_STATUS) & 0x1)){
				break;
			}
		}
		if(i != 10){
			 ret = true;
		}else{		
			spinor_suspend(sni);
			ret = false;
		}
	}
	irq_unlock(key);
	return ret;
}
_nor_fun static void spinor_wait_finished(struct spinor_info *sni)
{
	int i;
	for(i = 0; i < 2000; i++){ //2000*500us= 1000ms overtimer
		if (spinor_resume_and_check_idle(sni))
			break;
		if(!k_is_in_isr()){
			if((i & 0x1) == 0)
				k_msleep(1);
		}
	}
	if(i == 2000){
		printk("nor resume error\n");
	}
}

K_MUTEX_DEFINE(spinor_w_mutex);
_nor_fun  void spinor_resume_finished(struct spinor_info *sni)
{
	printk("nor is suspend, wait resume finished\n");
	spimem_write_cmd(&sni->spi, SPINOR_CMD_PROGRAM_ERASE_RESUME);
	soc_udelay(5);
	spinor_wait_ready(sni);
}
#endif

_nor_fun int spinor_ram_write(struct spinor_info *sni, unsigned int addr,
            const void *data, int len)
{
	int ret = 0;
	uint32_t key;
	int wlen;
#ifdef CONFIG_NOR_SUSPEND_RESUME
	k_mutex_lock(&spinor_w_mutex, K_FOREVER);
	spinor_wait_finished(sni);
#endif
	while(len > 0) {
		if(len > SPINOR_WRITE_PAGE_SIZE)
			wlen = SPINOR_WRITE_PAGE_SIZE;
		else
			wlen = len;
		key = irq_lock();
		if(addr+wlen > NOR_3B_ADDR_MAXLEN)
			spinor_4byte_address_mode(sni, true);
		ret = spinor_write(sni, addr, data, wlen);
		if(addr+wlen > NOR_3B_ADDR_MAXLEN)
			spinor_4byte_address_mode(sni, false);
#ifdef CONFIG_SPINOR_TEST_DELAYCHAIN
		soc_udelay(50000); // try fail, nor status may not finished, delay erase finished
#endif
		irq_unlock(key);
		addr += wlen;
		data  = (void *)((unsigned int )data + wlen);
		len -= wlen;
	}
	soc_memctrl_cache_invalid();
#ifdef CONFIG_NOR_SUSPEND_RESUME
	k_mutex_unlock(&spinor_w_mutex);
#endif

	return ret;
}


#ifdef CONFIG_NOR_SUSPEND_RESUME
//#define NOR_ERASE_CHECK
#endif
#ifdef NOR_ERASE_CHECK
_nor_fun int spinor_erase_chcek(struct spinor_info *sni,unsigned int addr, int len)
{
	unsigned int i, j;
	unsigned int *p;
	if(addr < xip_nor_offset || len <= 0 || (addr+len > NOR_3B_ADDR_MAXLEN)){
		return 0;
	}
	p = (unsigned int *)(0x10000000 + addr - xip_nor_offset);
	for(i = 0; i < len/4; i++) {
		if(p[i] != 0xffffffff){
			printk("nor offset=0x%x, 0x%x!=0xffffffff\n", addr+i*4, p[i]);
			printk("status=0x%x 0x%x\n", spinor_read_status(sni, SPINOR_CMD_READ_STATUS), spinor_read_status(sni, SPINOR_CMD_READ_STATUS2));
			for(j = 0; j < 16; j++)
				printk("%d 0x%x=0x%x\n",j, addr+(i+j-8)*4, p[i+j-8]);
			k_panic();
		}
	}
	printk("erase check ok, off=0x%x,len=0x%x\n", addr, len);
	return 0;
}
#endif

_nor_fun int spinor_ram_erase(struct spinor_info *sni, unsigned int addr, int len)
{
	int ret = 0;
	uint32_t key;
	int erase_size = 0;
	uint32_t t0,t1, t2;
#ifdef CONFIG_NOR_SUSPEND_RESUME
	bool b_suspend = true;
#endif

#ifdef NOR_ERASE_CHECK
	unsigned int bak_len, bak_addr;
	bak_len = len;
	bak_addr = addr;
#endif
	//printk("nor_e:off=0x%x,len=0x%x\n", addr, len);

#ifdef CONFIG_NOR_SUSPEND_RESUME
	if(len >= SPINOR_ERASE_BLOCK_SIZE*4) // erase 256kb, not suspend &resume
		b_suspend = false;	
	k_mutex_lock(&spinor_w_mutex, K_FOREVER);
	spinor_wait_finished(sni);
	if(b_suspend)
		sni->flag |= SPINOR_FLAG_NO_WAIT_READY;
#endif

	while (len > 0) {
		if (len < SPINOR_ERASE_BLOCK_SIZE) {
			erase_size = SPINOR_ERASE_SECTOR_SIZE;
        } else if (addr & SPINOR_ERASE_BLOCK_MASK) {
			erase_size = SPINOR_ERASE_SECTOR_SIZE;
		} else {
			erase_size = SPINOR_ERASE_BLOCK_SIZE;
		}
		key = irq_lock();
		t0 = k_cycle_get_32();
		if(addr >= NOR_3B_ADDR_MAXLEN)
			spinor_4byte_address_mode(sni, true);
		ret = spinor_erase(sni, addr, erase_size);
		#ifdef CONFIG_NOR_SUSPEND_RESUME
		if(b_suspend)
			spinor_suspend(sni);
		#endif
		if(addr >= NOR_3B_ADDR_MAXLEN)
			spinor_4byte_address_mode(sni, false);
		t1 = k_cycle_get_32();
#ifdef CONFIG_SPINOR_TEST_DELAYCHAIN
		soc_udelay(100000); // try fail, nor status may not finished, delay erase finished
#endif
		irq_unlock(key);
		#ifdef CONFIG_NOR_SUSPEND_RESUME
		if(b_suspend)
			spinor_wait_finished(sni);
		#endif
		t2 = k_cycle_get_32();
		//printk("nor_e:off=0x%x,len=0x%x, tran=%d us, wait=%d\n",addr, erase_size,
				//k_cyc_to_us_ceil32(t1-t0), k_cyc_to_us_ceil32(t2-t1));
		len -= erase_size;
		addr += erase_size;
	}
	soc_memctrl_cache_invalid();
#ifdef CONFIG_NOR_SUSPEND_RESUME
	sni->flag &= ~SPINOR_FLAG_NO_WAIT_READY;
	k_mutex_unlock(&spinor_w_mutex);
#endif
#ifdef NOR_ERASE_CHECK
	spinor_erase_chcek(sni, bak_addr,bak_len);
#endif
	return ret;
}

const struct spinor_operation_api spinor_4b_addr_op_api = {
	.read_chipid	= spinor_read_chipid,
	.read_status	= spinor_read_status,
	.write_status	= spinor_write_status,
    .read		= spinor_ram_read,
    .write		= spinor_ram_write,
    .erase		= spinor_ram_erase,
};
#if IS_ENABLED(CONFIG_SPI_FLASH_2)
const struct spinor_operation_api spinor_3_addr_op_api = {
	.read_chipid	= spinor_read_chipid,
	.read_status	= spinor_read_status,
	.write_status	= spinor_write_status,
    .read		= spinor_read,
    .write		= spinor_write,
    .erase		= spinor_erase,
};
const struct spinor_operation_api *spi3nor_get_api(void)
{
	return &spinor_3_addr_op_api;
}
#endif

#else
const struct spinor_operation_api spinor_4b_addr_op_api = {
	.read_chipid	= spinor_read_chipid,
	.read_status	= spinor_read_status,
	.write_status	= spinor_write_status,
    .read		= spinor_read,
    .write		= spinor_write,
    .erase		= spinor_erase,
};
#if IS_ENABLED(CONFIG_SPI_FLASH_2)
const struct spinor_operation_api *spi3nor_get_api(void)
{
	return &spinor_4b_addr_op_api;
}
#endif


#endif




