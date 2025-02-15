/*
 * Copyright (c) 2020 Actions Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sys/printk.h>
#include <shell/shell.h>
#include <init.h>
#include <string.h>
#include <stdlib.h>
#include <device.h>
#include <sdfs.h>
#include <soc.h>
#include <board_cfg.h>
#include <fs/fs.h>
#include <rbuf/rbuf_mem.h>
#include <rbuf/rbuf_pool.h>
#include <errno.h>
#include <limits.h>
#include <debug/ramdump.h>
#include <debug/tracedump.h>

#define MAX_LINE_LENGTH_BYTES (64)
#define DEFAULT_LINE_LENGTH_BYTES (16)
#define MAX_MEM_SIZE          (32*1024*1024)

void print_buffer(const struct shell *shell,const char *addr, int width,
                    int count, int linelen, unsigned long disp_addr)
{
    int i, thislinelen;
    const char *data;
    /* linebuf as a union causes proper alignment */
    union linebuf {
        uint32_t ui[MAX_LINE_LENGTH_BYTES/sizeof(uint32_t) + 1];
        uint16_t us[MAX_LINE_LENGTH_BYTES/sizeof(uint16_t) + 1];
        uint8_t  uc[MAX_LINE_LENGTH_BYTES/sizeof(uint8_t) + 1];
    } lb;

    if (linelen * width > MAX_LINE_LENGTH_BYTES)
        linelen = MAX_LINE_LENGTH_BYTES / width;
    if (linelen < 1)
        linelen = DEFAULT_LINE_LENGTH_BYTES / width;

    if (disp_addr == -1)
        disp_addr = (unsigned long)addr;

    while (count) {
        thislinelen = linelen;
        data = (const char *)addr;

        shell_fprintf(shell, SHELL_NORMAL, "%08x:", (unsigned int)disp_addr);

        /* check for overflow condition */
        if (count < thislinelen)
            thislinelen = count;

        /* Copy from memory into linebuf and print hex values */
        for (i = 0; i < thislinelen; i++) {
            if (width == 4) {
                lb.ui[i] = *(volatile uint32_t *)data;
                shell_fprintf(shell, SHELL_NORMAL, " %08x", lb.ui[i]);
            } else if (width == 2) {
                lb.us[i] = *(volatile uint16_t *)data;
                shell_fprintf(shell, SHELL_NORMAL, " %04x", lb.us[i]);
            } else {
                lb.uc[i] = *(volatile uint8_t *)data;
                shell_fprintf(shell, SHELL_NORMAL, " %02x", lb.uc[i]);
            }
            data += width;
        }

        while (thislinelen < linelen) {
            /* fill line with whitespace for nice ASCII print */
            for (i = 0; i < width * 2 + 1; i++)
                shell_fprintf(shell, SHELL_NORMAL, " ");
            linelen--;
        }

        /* Print data in ASCII characters */
        for (i = 0; i < thislinelen * width; i++) {
            if (lb.uc[i] < 0x20 || lb.uc[i] > 0x7e)
                lb.uc[i] = '.';
        }
        lb.uc[i] = '\0';
        shell_fprintf(shell, SHELL_NORMAL, "    %s\n", lb.uc);

        /* update references */
        addr += thislinelen * width;
        disp_addr += thislinelen * width;
        count -= thislinelen;
    }
}

#if defined(CONFIG_CMD_MEMORY)
#define DISP_LINE_LEN	16
static int do_mem_mw(const struct shell *shell, int width, size_t argc, char **argv)
{
    unsigned long writeval;
    unsigned long addr, count;
    char *buf;
    char *pend;

    if (argc < 3)
        return -EINVAL;

    addr = strtoul(argv[1], NULL, 16);
    writeval = strtoul(argv[2], NULL, 16);

    if (argc == 4) {
        errno = 0;
        count = strtoul(argv[3], &pend, 16);
        if ((pend == argv[3] || *pend != '\0') || errno == ERANGE || count > MAX_MEM_SIZE) {
            shell_print(shell, "params invalid.\n");
            return -EINVAL;
        }
    } else {
        count = 1;
    }

    buf = (char *)addr;
	if (count != ULONG_MAX) {
		if(count > 0) {
		    while (count-- > 0) {
		        if (width == 4)
		            *((uint32_t *)buf) = (uint32_t)writeval;
		        else if (width == 2)
		            *((uint16_t *)buf) = (uint16_t)writeval;
		        else
		            *((uint8_t *)buf) = (uint8_t)writeval;
		        buf += width;
		    }
		}
	}
    return 0;
}

static int do_mem_md(const struct shell *shell, int width, size_t argc, char **argv)
{
    unsigned long addr;
    unsigned long count;
    char *pend;

    if (argc < 2)
        return -EINVAL;

    addr = strtoul(argv[1], NULL, 16);

    if (argc == 3) {
        errno = 0;
        count = strtoul(argv[2], &pend, 16);
        if ((pend == argv[2] || *pend != '\0') || errno == ERANGE || count > MAX_MEM_SIZE) {
            shell_print(shell, "params invalid.\n");
			count = 0;
            return -EINVAL;
        }
    } else {
        count = 1;
    }
	if (count != ULONG_MAX) {
    	print_buffer(shell, (char *)addr, width, count, DISP_LINE_LEN / width, -1);
	}
    return 0;
}

static int shell_cmd_mdw(const struct shell *shell, size_t argc, char **argv)
{
    return do_mem_md(shell, 4, argc, argv);
}

static int shell_cmd_mdh(const struct shell *shell, size_t argc, char **argv)
{
    return do_mem_md(shell,2, argc, argv);
}

static int shell_cmd_mdb(const struct shell *shell, size_t argc, char **argv)
{
    return do_mem_md(shell,1, argc, argv);
}

static int shell_cmd_mww(const struct shell *shell, size_t argc, char **argv)
{
    return do_mem_mw(shell,4, argc, argv);
}

static int shell_cmd_mwh(const struct shell *shell, size_t argc, char **argv)
{
    return do_mem_mw(shell, 2, argc, argv);
}

static int shell_cmd_mwb(const struct shell *shell, size_t argc, char **argv)
{
    return do_mem_mw(shell, 1, argc, argv);
}
#endif	/* CONFIG_CMD_MEMORY */


#if defined(CONFIG_SOC_SPICACHE_PROFILE)
#include <spicache.h>

void spicache_profile_dump(const struct shell *shell, struct spicache_profile *profile)
{
    uint32_t interval_ms, total,hit;

    if (!profile)
        return;

    shell_print(shell,"s-e timer:%d, %d\n", profile->start_time, profile->end_time);
    interval_ms = (uint32_t) k_cyc_to_ms_floor32(profile->end_time-profile->start_time);

    shell_print(shell, "spicache profile: addr range 0x%08x ~ 0x%08x, profile time %d ms\n",
        profile->start_addr, profile->end_addr, interval_ms);
    hit = profile->hit_cnt*8;
    total = (hit + profile->miss_cnt);
    if (total != 0)
        shell_print(shell, "       hit: %12d           miss: %12d    hit ratio: %d%%\n",
            hit, profile->miss_cnt,
            hit/ (total / 100));

    else
        shell_print(shell, "cpu not run into the specific address range!\n");

	if (!profile->spi_id) {
	    hit = profile->total_hit_cnt*8;
	    total = (hit + profile->total_miss_cnt);
	    if (total != 0)
	        shell_print(shell, "totoal hit: %12d    totoal miss: %12d    hit ratio: %d%%\n\n",
	            hit, profile->total_miss_cnt,
	            hit / (total / 100));
	} else {
	    hit = profile->dma_hit_cnt*8;
	    total = (hit + profile->dma_miss_cnt);
	    if (total != 0)
	        shell_print(shell, "dma hit: %12d    dma miss: %12d    hit ratio: %d%%\n\n",
	            hit, profile->dma_miss_cnt,
	            hit / (total / 100));
	}
}

static struct spicache_profile __act_s2_notsave profile_data;

/*
 * cmd: spicache_profile
 *   start start_addr end_addr
 *   stop
 */
static int shell_cmd_spicache_profile(const struct shell *shell,
                  size_t argc, char **argv)
{
    struct spicache_profile *profile;
    int len = strlen(argv[1]);

    profile = &profile_data;

    if (!strncmp(argv[1], "start", len)) {
        if (argc < 5)
            return -EINVAL;

		profile->spi_id = strtoul(argv[2], NULL, 0);
		if ((profile->spi_id != 0) && (profile->spi_id != 1)) {
	        shell_print(shell, "invalid spi id:%d\n", profile->spi_id);
			return -EINVAL;
		}

        profile->start_addr = strtoul(argv[3], NULL, 0);
        profile->end_addr = strtoul(argv[4], NULL, 0);

        shell_print(shell, "Start profile: spi_id addr range %08x ~ %08x\n",
            profile->start_addr, profile->end_addr);
        spicache_profile_start(profile);

    } else if (!strncmp(argv[1], "stop", len)) {
        shell_print(shell, "Stop profile\n");
        spicache_profile_stop(profile);
        spicache_profile_dump(shell,profile);
    } else {
        shell_print(shell, "usage:\n");
        shell_print(shell, "  spicache_profile start start_addr end_addr\n");
        shell_print(shell, "  spicache_profile stop\n");

        return -EINVAL;
    }

    return 0;
}
#endif	/* CONFIG_SOC_SPICACHE_PROFILE */


int print_buf(const struct shell *shell, char *ptr, int len)
{
    int i;
    //shell_print(shell,"\n");
    printk("\n");
    for(i = 0; i < len; i++)
    {
        if(i % 16 == 0)
            printk("%8x: ", i);
            //shell_print(shell,"%d: ", i);
        //shell_print(shell,"%x ", *ptr++);
        printk("%2x ", *ptr++);
        if(i % 16 == 15)
            //shell_print(shell,"\n");
            printk("\n");
    }
    //shell_print(shell,"\n");
    printk("\n");
    return 0;
}


#if defined(CONFIG_CMD_SPINOR)
#include <drivers/flash.h>
#define READ_BLOCK_ONCE  512

int test_read_speed(const struct shell *shell, const struct device *nor_dev, uint32_t  offset,  uint32_t size)
{
    uint64_t  start_ms;
    uint32_t  k, btn_ms;
    int ret;
    char *buf = k_malloc(READ_BLOCK_ONCE);
    if(buf == NULL){
        shell_print(shell, "malloc fail\n");
        return -1;
    }

    shell_print(shell, "nor read speed size=%d kb, offset=0x%x\n", size/1024, offset);
    //flash_read(nor_dev, 0x0, buf, READ_BLOCK_ONCE);
    //print_buf(buf);
    start_ms = k_uptime_get();
    for(k = 0; k < size; k += READ_BLOCK_ONCE, offset+=READ_BLOCK_ONCE) {
         ret = flash_read((const struct device *)nor_dev, offset, buf, READ_BLOCK_ONCE);
         if(ret < 0 ) {
            shell_print(shell, "flash read fail\n");
            break;
         }
    }
    btn_ms = k_uptime_get() - start_ms;
    if(btn_ms == 0)
        btn_ms = 1;
    k_free(buf);
    shell_print(shell, "use %d ms, read once=%d,  nor speed =%d kb/s\n", btn_ms, READ_BLOCK_ONCE, (size/1024)*1000/btn_ms );

    return 0;
}

int test_write_speed(const struct shell *shell, const struct device *nor_dev, uint32_t  offset,  uint32_t size)
{
    uint64_t  start_ms;
    uint32_t  k, btn_ms, off, i;
    int ret;

    uint32_t *buf = k_malloc(READ_BLOCK_ONCE);
    if(buf == NULL){
        shell_print(shell, "malloc fail\n");
        return -1;
    }

    for(k = 0; k < READ_BLOCK_ONCE/4; k++)
        buf[k] = k;
    shell_print(shell,"nor write speed test, size=%d kb, offset=0x%x\n", size/1024, offset);
    off = offset;
#if 1
    start_ms = k_uptime_get();
	//for(k = 0; k < size; k+=0x10000) {
		ret = flash_erase(nor_dev, offset, size);
    //ret =  flash_erase(nor_dev, offset, size);
	     if(ret < 0 ) {
	        shell_print(shell,"flash erase fail\n");
	     }
	//}
    btn_ms = k_uptime_get() - start_ms;
    if(btn_ms == 0)
        btn_ms = 1;
    shell_print(shell, "erase use %d ms, erase speed=%d kb/s\n", btn_ms,  (size/1024)*1000/btn_ms);

#endif
    start_ms = k_uptime_get();
    for(k = 0; k < size; k += READ_BLOCK_ONCE, offset+=READ_BLOCK_ONCE) {
         ret = flash_write(nor_dev, offset, buf, READ_BLOCK_ONCE);
         if(ret < 0 ) {
            shell_print(shell,"flash write fail\n");
            break;
         }
    }
    btn_ms = k_uptime_get() - start_ms;
    if(btn_ms == 0)
        btn_ms = 1;
    shell_print(shell, "use %d ms, write once=%d,  nor speed =%d kb/s\n", btn_ms, READ_BLOCK_ONCE, (size/1024)*1000/btn_ms );

    shell_print(shell,"--cmp write and read---\n");
    for(k = 0; k < size; k += READ_BLOCK_ONCE, off+=READ_BLOCK_ONCE) {
         ret = flash_read(nor_dev, off, buf, READ_BLOCK_ONCE);
         if(ret < 0 ) {
            shell_print(shell,"flash read fail\n");
            break;
         }
         for(i = 0; i < READ_BLOCK_ONCE/4; i++) {
            if(buf[i] != i){
                shell_print(shell,"cmp fail: off=0x%x,val=0x%x != 0x%x\n", (off+i*4), buf[i], i);
                break;
            }
         }
    }
    shell_print(shell,"--cmp  finshed ---\n");

    k_free(buf);
    return 0;

}

#define SPEEDFFSET     	0x20000
#define SPEEDSIZE       0xe0000
int shell_nor_speed_test(const struct shell *shell,
                  size_t argc, char **argv)
{
    uint32_t  offset,  size;
    char *pend;
    const struct device *nor_dev = device_get_binding(CONFIG_SPI_FLASH_NAME);
    if (!nor_dev) {
        shell_print(shell,"nor dev binding failed!\n");
        return EINVAL;
    }

    if(argc == 3){
        offset =  strtoul(argv[1], &pend, 0);
        size = strtoul(argv[2], &pend, 0);
    }else{
        offset = SPEEDFFSET;
        size = SPEEDSIZE;
    }
    shell_print(shell,"nor test: offset=0x%x, size=0x%x\n", offset, size);
    test_read_speed(shell, nor_dev, offset, size);
    test_write_speed(shell, nor_dev, offset, size);

    return 0;
}


int shell_flash_read(const struct shell *shell,
                    size_t argc, char **argv)
{
    uint32_t  offset, size=1;
    char *pend;
    char *buf;
    int  ret;
    const struct device *f_dev;
    if(argc != 3){
        shell_print(shell,"fread dev_name(sd/spi_flash/spinand) offset\n");
        return -1;
    }
    f_dev = device_get_binding(argv[1]);
    if (!f_dev) {
        shell_print(shell,"flash %s binding failed!\n", argv[1]);
        return EINVAL;
    }
    if(0 == strcmp(argv[1], "spi_flash")) // nor is size else is sector
        size = 512;
    offset =  strtoul(argv[2], &pend, 0);
    buf = k_malloc(READ_BLOCK_ONCE);
    if(buf == NULL){
        shell_print(shell, "malloc fail\n");
        return -1;
    }
    shell_print(shell,"fread 512b: offset=0x%x, buf=0x%x\n", offset, (uint32_t)buf);
    ret = flash_read(f_dev, offset, buf, size);
    if(ret < 0 ) {
        shell_print(shell, "flash read fail\n");
    }else{
        print_buf(shell, buf, 512);
    }
    k_free(buf);
    return 0;

}

#endif

#if defined(CONFIG_CMD_SPINAND)
#include <drivers/flash.h>
#include <drivers/spinand.h>
#include <drivers/disk.h>
#define SPINAND_READ_BLOCK_ONCE  2048
static int shell_nand_read(const struct shell *shell,
                    size_t argc, char **argv)
{
    uint32_t offset, size;
    char *pend;
    char *buf;
    int  ret;
    const struct device *nand_dev = device_get_binding("spinand");
    if (!nand_dev) {
        shell_print(shell,"nand dev binding failed!\n");
        return EINVAL;
    }
    buf = malloc(SPINAND_READ_BLOCK_ONCE);
    if(buf == NULL){
        shell_print(shell, "malloc fail\n");
        return -1;
    }
    if(argc == 3){
        offset =  strtoul(argv[1], &pend, 0);
        size = strtoul(argv[2], &pend, 0);
    }else{
        shell_print(shell, "offset or size param setting wrong, please check!\n");
        return -1;
    }
    shell_print(shell,"nand read %d: offset=0x%x, buf=0x%x\n", size, offset, (uint32_t)buf);
    ret = flash_read(nand_dev, offset, buf, size);
    if(ret < 0 ) {
        shell_print(shell, "flash read fail\n");
    }else{
        print_buf(shell, buf, size);
    }
    free(buf);
    return 0;
}

static char wbuf[50] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, \
                        0xaa, 0xbb, 0xcc, 0xdd, 0xff, 0x23, 0x37, 0x49, 0x53, \
                        0x68, 0x73, 0x81, 0x95, 0xa3, 0xbd, 0xc8, 0xd7, 0x55, 0xaa};
static int shell_nand_write(const struct shell *shell,
                    size_t argc, char **argv)
{
    uint32_t offset, size;
    char *pend;
    int  ret;
    const struct device *nand_dev = device_get_binding("spinand");
    if (!nand_dev) {
        shell_print(shell,"nand dev binding failed!\n");
        return EINVAL;
    }
    if(argc == 3){
        offset = strtoul(argv[1], &pend, 0);
        size = strtoul(argv[2], &pend, 0);
    }else{
        shell_print(shell, "offset or size param setting wrong, please check!\n");
        return -1;
    }
    shell_print(shell,"nand write %d: offset=0x%x \n", size, offset);
        shell_print(shell, "flash write\n");
    ret = flash_write(nand_dev, offset, wbuf, size);
        shell_print(shell, "flash write finish\n");
    if(ret < 0 ) {
        shell_print(shell, "flash read fail\n");
    }else{
        shell_print(shell, "flash write offset=%d; len=%d succeed!\n", offset, size);
    }
    return 0;
}

extern void spinand_change_print_level(const struct device *dev, u8_t level);
static int shell_nand_print_level(const struct shell *shell,
                    size_t argc, char **argv)
{
    int level = 0;
    char *pend;
    const struct device *nand_dev = device_get_binding("spinand");
    if (!nand_dev) {
        shell_print(shell,"nand dev binding failed!\n");
        return EINVAL;
    }

    if (argc == 2){
        level =  strtoul(argv[1], &pend, 0);
    } else {
        shell_print(shell, "level param setting wrong, please check!\n");
        return -1;
    }
    if (level == 0 || level > 4) {
        shell_print(shell, "level param should be in 1--4, please check!\n");
        return -1;
    }

    spinand_change_print_level(nand_dev, level);
    return 0;
}

#if defined(CONFIG_SPINAND_TEST_FRAMEWORKS)
static int shell_nand_ioctrl(const struct shell *shell,
                    size_t argc, char **argv)
{
	int ret;
    uint8_t hw_detect;
	uint32_t sector_cnt;
	uint32_t sector_size;
	uint32_t blk_erase_size;

    const struct device *nand_dev = device_get_binding("spinand");
    if (!nand_dev) {
        shell_print(shell,"nand dev binding failed!\n");
        return EINVAL;
    }

	ret = spinand_storage_ioctl((struct device *)nand_dev, DISK_IOCTL_GET_SECTOR_SIZE, (void *)&sector_size);
    if (ret != 0) {
        shell_print(shell, "spinand ioctrl DISK_IOCTL_GET_SECTOR_SIZE failed!\n");
    } else {
        shell_print(shell, "spinand ioctrl DISK_IOCTL_GET_SECTOR_SIZE get val = %d!\n", sector_size);
    }
	ret = spinand_storage_ioctl((struct device *)nand_dev, DISK_IOCTL_GET_SECTOR_COUNT, (void *)&sector_cnt);
    if (ret != 0) {
        shell_print(shell, "spinand ioctrl DISK_IOCTL_GET_SECTOR_COUNT failed!\n");
    } else {
        shell_print(shell, "spinand ioctrl DISK_IOCTL_GET_SECTOR_COUNT get val = %d!\n", sector_cnt);
    }
	ret = spinand_storage_ioctl((struct device *)nand_dev, DISK_IOCTL_GET_ERASE_BLOCK_SZ, (void *)&blk_erase_size);
    if (ret != 0) {
        shell_print(shell, "spinand ioctrl DISK_IOCTL_GET_ERASE_BLOCK_SZ failed!\n");
    } else {
        shell_print(shell, "spinand ioctrl DISK_IOCTL_GET_ERASE_BLOCK_SZ get val = %d!\n", blk_erase_size);
    }
	ret = spinand_storage_ioctl((struct device *)nand_dev, DISK_IOCTL_HW_DETECT, (void *)&hw_detect);
    if (ret != 0) {
        shell_print(shell, "spinand ioctrl DISK_IOCTL_HW_DETECT failed!\n");
    } else {
        shell_print(shell, "spinand ioctrl DISK_IOCTL_HW_DETECT get val = %d!\n", hw_detect);
        if (hw_detect == 2) {
            shell_print(shell, "spinand disk unormal, no disk!\n");
        } else if (hw_detect == 8) {
            shell_print(shell, "spinand disk normal.\n");
        }
    }
	ret = spinand_storage_ioctl((struct device *)nand_dev, DISK_IOCTL_CTRL_SYNC, NULL);
    if (ret != 0) {
        shell_print(shell, "spinand ioctrl DISK_IOCTL_CTRL_SYNC failed!\n");
    } else {
        shell_print(shell, "spinand ioctrl DISK_IOCTL_CTRL_SYNC success!\n");
    }
    return 0;
}

extern int spinand_scan_delaychain(const struct device *dev);
extern int spinand_manual_flush(const struct device *dev);
#if defined(CONFIG_SPINAND_TEST_FRAMEWORKS_ADDITION)
extern int spinand_print_hardware_params(const struct device *dev);
extern int spinand_print_software_params(const struct device *dev);
extern int spinand_phy_page_write_test(const struct device *dev, u16_t blk, u16_t page, u8_t clk);
extern int spinand_phy_page_read_test(const struct device *dev, u16_t blk, u16_t page, u8_t clk);
extern int spinand_phy_blk_erase_test(const struct device *dev, u8_t blk);
extern int spinand_read_single_page_test(const struct device *dev);
extern int spinand_write_single_page_test(const struct device *dev);
extern int spinand_ldl_page_readback_test(const struct device *dev);
extern int spinand_zonetbl_rotation(const struct device *dev);
extern int spinand_rewrite_zonetbl_test(const struct device *dev);
extern int spinand_erase_zonetbl_blk(const struct device *dev);
extern int spinand_api_boundary_test(const struct device *dev);
extern int spinand_read_speed_test(const struct device *dev, u8_t clk);
extern int spinand_write_speed_test(const struct device *dev, u8_t clk);
extern int spinand_pdlinit_time_cost(const struct device *dev);
extern int spinand_zonetbl_init_time_cost(const struct device *dev);
extern int spinand_read_write_udisk(const struct device *dev);
extern int spinand_multi_zonetbl_test(const struct device *dev);
#endif

static int shell_nand_delaychain_scan(const struct shell *shell,
                    size_t argc, char **argv)
{
    int  ret;
    const struct device *nand_dev = device_get_binding("spinand");
    if (!nand_dev) {
        shell_print(shell,"nand dev binding failed!\n");
        return EINVAL;
    }
    ret = spinand_scan_delaychain(nand_dev);
    if(ret < 0 ) {
        shell_print(shell, "nand scan delaychain error!\n");
    }else{
        shell_print(shell, "nand scan delaychain finished!\n");
    }
    return 0;
}

static int shell_nand_flush(const struct shell *shell,
                    size_t argc, char **argv)
{
    const struct device *nand_dev = device_get_binding("spinand");
    if (!nand_dev) {
        shell_print(shell,"nand dev binding failed!\n");
        return EINVAL;
    }
    spinand_manual_flush(nand_dev);

    return 0;
}

#if defined(CONFIG_SPINAND_TEST_FRAMEWORKS_ADDITION)
static int shell_nand_hardware_params(const struct shell *shell,
                    size_t argc, char **argv)
{
    const struct device *nand_dev = device_get_binding("spinand");
    if (!nand_dev) {
        shell_print(shell,"nand dev binding failed!\n");
        return EINVAL;
    }
    spinand_print_hardware_params(nand_dev);
    return 0;
}

static int shell_nand_software_params(const struct shell *shell,
                    size_t argc, char **argv)
{
    int  ret;
    const struct device *nand_dev = device_get_binding("spinand");
    if (!nand_dev) {
        shell_print(shell,"nand dev binding failed!\n");
        return EINVAL;
    }
    ret = spinand_print_software_params(nand_dev);
    if(ret < 0 ) {
        shell_print(shell, "nand get software parms error!\n");
    }
    return 0;
}

static int shell_nand_page_write_test(const struct shell *shell,
                    size_t argc, char **argv)
{
    int ret;
    int clk = 96;
    int blk = 2;
    int page = 0;
    char *pend;
    const struct device *nand_dev = device_get_binding("spinand");
    if (!nand_dev) {
        shell_print(shell,"nand dev binding failed!\n");
        return EINVAL;
    }

    if (argc == 2){
        clk =  strtoul(argv[1], &pend, 0);
    }
    if(argc == 3){
        clk =  strtoul(argv[1], &pend, 0);
        blk = strtoul(argv[2], &pend, 0);
    }
    if(argc == 4){
        clk =  strtoul(argv[1], &pend, 0);
        blk = strtoul(argv[2], &pend, 0);
        page = strtoul(argv[3], &pend, 0);
    }

    ret = spinand_phy_page_write_test(nand_dev, blk, page, clk);
    if(ret != 0 ) {
        shell_print(shell, "spinand page write test error!\n");
    }
    return 0;
}

static int shell_nand_page_read_test(const struct shell *shell,
                    size_t argc, char **argv)
{
    int ret;
    int clk = 96;
    int blk = 2;
    int page = 0;
    char *pend;
    const struct device *nand_dev = device_get_binding("spinand");
    if (!nand_dev) {
        shell_print(shell,"nand dev binding failed!\n");
        return EINVAL;
    }

    if (argc == 2){
        clk =  strtoul(argv[1], &pend, 0);
    }
    if(argc == 3){
        clk =  strtoul(argv[1], &pend, 0);
        blk = strtoul(argv[2], &pend, 0);
    }
    if(argc == 4){
        clk =  strtoul(argv[1], &pend, 0);
        blk = strtoul(argv[2], &pend, 0);
        page = strtoul(argv[3], &pend, 0);
    }

    ret = spinand_phy_page_read_test(nand_dev, blk, page, clk);
    if(ret != 0 ) {
        shell_print(shell, "spinand page read test error!\n");
    }
    return 0;
}

static int shell_nand_blk_erase_test(const struct shell *shell,
                    size_t argc, char **argv)
{
    int  ret;
    int blk;
    char *pend;
    const struct device *nand_dev = device_get_binding("spinand");
    if (!nand_dev) {
        shell_print(shell,"nand dev binding failed!\n");
        return EINVAL;
    }
    if(argc == 2){
        blk = strtoul(argv[1], &pend, 0);
    } else {
        blk = 2;
    }

    shell_print(shell, "phy blk erase blk%d. \n", blk);
    ret = spinand_phy_blk_erase_test(nand_dev, blk);
    if(ret != 0 ) {
        shell_print(shell, "spinand blk%d erase test error!\n", blk);
    }
    return 0;
}

static int shell_nand_api_boundary_test(const struct shell *shell,
                    size_t argc, char **argv)
{
    int  ret;
    const struct device *nand_dev = device_get_binding("spinand");
    if (!nand_dev) {
        shell_print(shell,"nand dev binding failed!\n");
        return EINVAL;
    }
    ret = spinand_api_boundary_test(nand_dev);
    if(ret != 0 ) {
        shell_print(shell, "spinand api boundary test error!\n");
    }
    return 0;
}

static int shell_nand_read_sigle_page_test(const struct shell *shell,
                    size_t argc, char **argv)
{
    int  ret;
    const struct device *nand_dev = device_get_binding("spinand");
    if (!nand_dev) {
        shell_print(shell,"nand dev binding failed!\n");
        return EINVAL;
    }
    ret = spinand_read_single_page_test(nand_dev);
    if(ret != 0 ) {
        shell_print(shell, "spinand read single page test error!\n");
    }
    return 0;
}

static int shell_nand_write_sigle_page_test(const struct shell *shell,
                    size_t argc, char **argv)
{
    int  ret;
    const struct device *nand_dev = device_get_binding("spinand");
    if (!nand_dev) {
        shell_print(shell,"nand dev binding failed!\n");
        return EINVAL;
    }
    ret = spinand_write_single_page_test(nand_dev);
    if(ret != 0 ) {
        shell_print(shell, "spinand write single page test error!\n");
    }
    return 0;
}

static int shell_nand_page_readback_test(const struct shell *shell,
                    size_t argc, char **argv)
{
    int  ret;
    const struct device *nand_dev = device_get_binding("spinand");
    if (!nand_dev) {
        shell_print(shell,"nand dev binding failed!\n");
        return EINVAL;
    }
    ret = spinand_ldl_page_readback_test(nand_dev);
    if(ret != 0 ) {
        shell_print(shell, "spinand logic page readback test error!\n");
    }
    return 0;
}

static int shell_nand_zonetbl_rotation_test(const struct shell *shell,
                    size_t argc, char **argv)
{
    int  ret;
    const struct device *nand_dev = device_get_binding("spinand");
    if (!nand_dev) {
        shell_print(shell,"nand dev binding failed!\n");
        return EINVAL;
    }
    ret = spinand_zonetbl_rotation(nand_dev);
    if(ret != 0 ) {
        shell_print(shell, "spinand zonetbl rotation test error!\n");
    }
    return 0;
}

static int shell_nand_rewrite_zonetbl_test(const struct shell *shell,
                    size_t argc, char **argv)
{
    int  ret;
    const struct device *nand_dev = device_get_binding("spinand");
    if (!nand_dev) {
        shell_print(shell,"nand dev binding failed!\n");
        return EINVAL;
    }
    ret = spinand_rewrite_zonetbl_test(nand_dev);
    if(ret != 0 ) {
        shell_print(shell, "spinand zonetbl rotation test error!\n");
    }
    return 0;
}

static int shell_nand_erase_zonetbl_blk_test(const struct shell *shell,
                    size_t argc, char **argv)
{
    int  ret;
    const struct device *nand_dev = device_get_binding("spinand");
    if (!nand_dev) {
        shell_print(shell,"nand dev binding failed!\n");
        return EINVAL;
    }
    ret = spinand_erase_zonetbl_blk(nand_dev);
    if(ret != 0 ) {
        shell_print(shell, "spinand zonetbl rotation test error!\n");
    }
    return 0;
}

static int shell_nand_read_speed_test(const struct shell *shell,
                    size_t argc, char **argv)
{
    int  ret;
    int clk;
    char *pend;
    const struct device *nand_dev = device_get_binding("spinand");
    if (!nand_dev) {
        shell_print(shell,"nand dev binding failed!\n");
        return EINVAL;
    }

    if (argc == 2){
        clk =  strtoul(argv[1], &pend, 0);
    } else {
        clk = 96;
    }

    ret = spinand_read_speed_test(nand_dev, clk);
    if(ret != 0 ) {
        shell_print(shell, "spinand read speed test error!\n");
    }
    return 0;
}

static int shell_nand_write_speed_test(const struct shell *shell,
                    size_t argc, char **argv)
{
    int  ret;
    int clk;
    char *pend;
    const struct device *nand_dev = device_get_binding("spinand");
    if (!nand_dev) {
        shell_print(shell,"nand dev binding failed!\n");
        return EINVAL;
    }

    if (argc == 2){
        clk =  strtoul(argv[1], &pend, 0);
    } else {
        clk = 96;
    }

    ret = spinand_write_speed_test(nand_dev, clk);
    if(ret != 0 ) {
        shell_print(shell, "spinand write speed test error!\n");
    }
    return 0;
}

static int shell_nand_pdlinit_time_cost_test(const struct shell *shell,
                    size_t argc, char **argv)
{
    int  ret;
    const struct device *nand_dev = device_get_binding("spinand");
    if (!nand_dev) {
        shell_print(shell,"nand dev binding failed!\n");
        return EINVAL;
    }
    ret = spinand_pdlinit_time_cost(nand_dev);
    if(ret != 0 ) {
        shell_print(shell, "spinand pdl init time cost test error!\n");
    }
    return 0;
}

static int shell_nand_zonetbl_init_time_cost_test(const struct shell *shell,
                    size_t argc, char **argv)
{
    int  ret;
    const struct device *nand_dev = device_get_binding("spinand");
    if (!nand_dev) {
        shell_print(shell,"nand dev binding failed!\n");
        return EINVAL;
    }
    ret = spinand_zonetbl_init_time_cost(nand_dev);
    if(ret != 0 ) {
        shell_print(shell, "spinand zonetbl init time cost test error!\n");
    }
    return 0;
}

static int shell_nand_read_write_udisk(const struct shell *shell,
                    size_t argc, char **argv)
{
    int  ret;
    const struct device *nand_dev = device_get_binding("spinand");
    if (!nand_dev) {
        shell_print(shell,"nand dev binding failed!\n");
        return EINVAL;
    }
    ret = spinand_read_write_udisk(nand_dev);
    if(ret != 0 ) {
        shell_print(shell, "spinand zonetbl init time cost test error!\n");
    }
    return 0;
}
#endif
#endif
#endif

#if defined(CONFIG_DMA_DBG_DUMP)
extern void dma_dump_info(void);
static int shell_dma_info(const struct shell *shell,
                    size_t argc, char **argv)
{
    dma_dump_info();
    return 0;
}
#endif

#if defined(CONFIG_KERNEL_SHOW_STACK)
extern void show_stack(void);
static int shell_show_stack(const struct shell *shell,
                    size_t argc, char **argv)
{
    show_stack();
    return 0;
}
#endif


#if defined(CONFIG_SD_FS)

static int shell_sdfs_dump(const struct shell *shell,
                    size_t argc, char **argv)
{
    struct sd_file *sf;
    uint32_t  size, fsize, i;
    char *pend;
    char buf[32];

    if (argc != 3)
        return -EINVAL;
    sf = sd_fopen(argv[1]);
    if(sf == NULL){
        shell_print(shell, "sdfs open %s fail\n", argv[1]);
        return -EINVAL;
    }
    fsize= sd_fsize(argv[1]);
    size = strtoul(argv[2], &pend, 0);
    if(size > fsize)
        size = fsize;
    shell_print(shell, "dump %s, size=0x%x, all=0x%x:\n", argv[1], size, fsize);
    for(i = 0; i < size; i+=32){
        sd_fread(sf, buf, 32);
        print_buf(shell, buf, 32);
    }
    sd_fclose(sf);

    return 0;
}

#endif

#ifdef CONFIG_DISPLAY_ENGINE
extern void de_dump(void);

static int shell_de_dump(const struct shell *shell,
                    size_t argc, char **argv)
{
    de_dump();
    return 0;
}
#endif /* CONFIG_DISPLAY_ENGINE */

#if defined(CONFIG_DISPLAY_LCDC)
extern void lcdc_dump(void);

static int shell_lcd_dump(const struct shell *shell,
                    size_t argc, char **argv)
{
    lcdc_dump();
#ifdef CONFIG_DISPLAY_ENGINE
    de_dump();
#endif
    return 0;
}
#endif /* CONFIG_DISPLAY_LCDC */

#if defined(CONFIG_NVRAM_CONFIG)
#include <drivers/nvram_config.h>
static int shell_nvram_dump(const struct shell *shell,
                    size_t argc, char **argv)
{
    nvram_config_dump(shell);
    return 0;
}

static int shell_nvram_cmd(const struct shell *shell,
                    size_t argc, char **argv)
{
    char rdata[64];
    int ret;
    if(argc == 2){
        memset(rdata, 0 ,64);
        ret = nvram_config_get(argv[1], rdata, 63);
        if(ret < 0 ) {
            shell_print(shell, "nvram get: %s fail\n", argv[1]);
        }else{
            shell_print(shell, "nvram get(len=%d): %s\n", ret, argv[1]);
            print_buffer(shell, rdata, 1, ret, 16, 0);
        }
    }else if (argc == 3){
        ret = nvram_config_set(argv[1], argv[2],  strlen(argv[2]));
        if(ret < 0 ){
            shell_print(shell, "nvram set: %s=%s fail\n", argv[1],  argv[2]);
        }else{
            shell_print(shell, "nvram set: %s=%s ok\n", argv[1],  argv[2]);
        }
    }else{
        return -1;
    }
    return 0;

}
#endif

#if defined(CONFIG_BT_DRV)
int bt_vs_write_bb_reg(uint32_t addr, uint32_t val);
int bt_vs_read_bb_reg(uint32_t addr, uint8_t size);
int bt_vs_write_rf_reg(uint16_t addr, uint16_t val);
int bt_vs_read_rf_reg(uint16_t addr, uint8_t size);

static int shell_cmd_bqb_bb(const struct shell *shell,
                    size_t argc, char **argv)
{
    uint32_t addr = strtoul(argv[2], NULL, 16);
    uint32_t val = strtoul(argv[3], NULL, 16);
    uint8_t size = strtoul(argv[3], NULL, 10);

    if (!strcmp(argv[1], "read")) {
        bt_vs_read_bb_reg(addr, size);
    } else if (!strcmp(argv[1], "write")) {
        bt_vs_write_bb_reg(addr, val);
    } else {
        return -EINVAL;
    }

    return 0;
}

static int shell_cmd_bqb_rf(const struct shell *shell,
                    size_t argc, char **argv)
{
    uint16_t addr = strtoul(argv[2], NULL, 16);
    uint16_t val = strtoul(argv[3], NULL, 16);
    uint8_t size = strtoul(argv[3], NULL, 10);

    if (!strcmp(argv[1], "read")) {
        bt_vs_read_rf_reg(addr, size);
    } else if (!strcmp(argv[1], "write")) {
        bt_vs_write_rf_reg(addr, val);
    } else {
        return -EINVAL;
    }

    return 0;
}
#endif

#if defined(CONFIG_PM_LOG_LEVEL_INF)
#include <pm/pm.h>
static int shell_cmd_sleep(const struct shell *shell,
                    size_t argc, char **argv)
{
    static struct pm_state_info  pm_stat;
	if(argc != 2)
		return 0;

    if( !strcmp(argv[1], "s3")) {
        shell_print(shell, "enter deep sleep\n");
        pm_stat.state = PM_STATE_SUSPEND_TO_RAM;
        //pm_power_state_force(pm_stat);
         sys_pm_enter_deep_sleep();
        shell_print(shell, "exit deep sleep\n");
    }else if( !strcmp(argv[1], "s2"))  {
        shell_print(shell, "enter sleep\n");
        pm_stat.state = PM_STATE_STANDBY;
        pm_power_state_force(pm_stat);
        shell_print(shell, "exit sleep\n");

    }else{
		uint8_t mode = strtoul(argv[1], NULL, 10);
		soc_set_sleep_mode(mode);
    }
    return 0;
}

#endif


static int shell_cmd_reboot(const struct shell *shell,
                    size_t argc, char **argv)
{
    if(argc == 2) {
        if(!strcmp(argv[1], "adfu")){
            shell_print(shell, "reboot adfu\n");
            sys_pm_reboot(REBOOT_TYPE_GOTO_ADFU);
        }else if(!strcmp(argv[1], "poweroff")){
            shell_print(shell, "poweroff\n");
            sys_pm_poweroff();
        }else{
        #if defined(CONFIG_SOC_LARK)
            if(!strcmp(argv[1], "jtag")){
                shell_print(shell, "reboot jtag\n");
                sys_pm_reboot(REBOOT_TYPE_GOTO_SWJTAG);
            }

        #endif
        }
    }
    k_msleep(200);
    shell_print(shell, "reboot\n");
    sys_pm_reboot(REBOOT_TYPE_NORMAL);
    return 0;
}

extern void libc_heap_dump(void);

static int shell_cmd_khead_dump(const struct shell *shell,
                    size_t argc, char **argv)
{
    struct k_heap * k_h;
    if(argc == 2){
        k_h = (struct k_heap *) strtoul(argv[1], NULL, 16);
        STRUCT_SECTION_FOREACH(k_heap, h) {
            if(h == k_h){
                shell_print(shell, "dump heap=%p:\n", h);
                sys_heap_dump(&h->heap);
            }else{
                shell_print(shell, "heap=%p != %p\n", h, k_h);
            }
        }
    }else{
        shell_print(shell, "----dump libc heap:---\n");
        libc_heap_dump();

        STRUCT_SECTION_FOREACH(k_heap, h) {
            shell_print(shell, "----dump heap=%p:---\n", h);
            sys_heap_dump(&h->heap);
        }
    }

    return 0;
}

#if defined(CONFIG_TOOL)
int shell_data_discard(void)
{
    int count = 0;
#if defined(CONFIG_SHELL_BACKEND_SERIAL)
    uint32_t bytes, temp;
    const struct shell *shell;

    extern const struct shell *shell_backend_uart_get_ptr(void);
    shell = shell_backend_uart_get_ptr();
    while (1)
    {
        (void)shell->iface->api->read(shell->iface, &temp, sizeof(temp), &bytes);
        if (bytes < sizeof(temp))
        {
            break;
        }

        count += bytes;
    }
#endif
    return count;
}

#if 0
int tool_init(char *type, unsigned int dev_type);
static int shell_cmd_tool_connect(const struct shell *shell,
                    size_t argc, char **argv)
{
    char *tool_type;

    if(argc >= 2)
    {
        tool_type = argv[1];
    }
    else
    {
        tool_type = "aset";
    }

    shell_print(shell, "%s\n", tool_type);

    shell_data_discard();
    (void)shell->iface->api->uninit(shell->iface);

    tool_init(tool_type, 3);
    return 0;
}
#endif

int shell_data_write(const uint8_t *data, uint32_t size, uint32_t timeout_ms)
{
    int count = 0;
#if defined(CONFIG_SHELL_BACKEND_SERIAL)
    const struct shell *shell;

    extern const struct shell *shell_backend_uart_get_ptr(void);
    shell = shell_backend_uart_get_ptr();
    (void)shell->iface->api->write(shell->iface, data, size, &count);
#endif
    return count;
}


void shell_dbg_disable(void)
{
#if defined(CONFIG_SHELL_BACKEND_SERIAL)
    const struct shell *shell;

    extern const struct shell *shell_backend_uart_get_ptr(void);
    shell = shell_backend_uart_get_ptr();

    shell_data_discard();
    (void)shell->iface->api->uninit(shell->iface);
#endif
}
void shell_dbg_restore(void)
{
#if defined(CONFIG_SHELL_BACKEND_SERIAL)
#include <shell/shell_uart.h>
    const struct shell *shell;
    const struct shell_uart *sh_uart;
    extern const struct shell *shell_backend_uart_get_ptr(void);

    shell = shell_backend_uart_get_ptr();
    sh_uart = (const struct shell_uart *)shell->iface->ctx;
    (void)shell->iface->api->init(shell->iface, sh_uart->ctrl_blk->dev,
        sh_uart->ctrl_blk->handler, sh_uart->ctrl_blk->context);
#endif
}
#endif


#if defined(CONFIG_UI_MEMORY_MANAGER)
#include <ui_mem.h>

static int shell_cmd_dump_uimem(const struct shell *shell,
                    size_t argc, char **argv)
{
    uint8_t type;

    if (argc > 1) {
        type = (uint8_t)atoi(argv[1]);
        ui_mem_dump(type);
    } else {
        ui_mem_dump_all();
    }

    return 0;
}
#endif

#if defined(CONFIG_UI_SERVICE)
#include <view_stack.h>
#include <msgbox_cache.h>

static int shell_cmd_dump_uiview(const struct shell *shell,
                    size_t argc, char **argv)
{
    view_manager_dump();
    view_stack_dump();
    view_cache_dump();
    msgbox_cache_dump();
    return 0;
}
#endif

#if defined(CONFIG_THREAD_RUNTIME_STATS)

/*
 * cmd: cpuload
 *   start
 *   stop
 */
void cpuload_stat_start(int interval_ms);
void cpuload_stat_stop(void);

static int shell_cmd_cpuload(const struct shell *shell,
                    size_t argc, char **argv)
{
    int interval_ms;
    int len = strlen(argv[1]);

    if (!strncmp(argv[1], "start", len)) {
        if (argc > 2)
            interval_ms = strtoul(argv[2], NULL, 0);
        else
            interval_ms = 2000;	/* default interval: 2s */

        shell_print(shell,"Start cpu load statistic, interval %d ms\n",
            interval_ms);

        cpuload_stat_start(interval_ms);

    } else if (!strncmp(argv[1], "stop", len)) {
        shell_print(shell,"Stop cpu load statistic\n");
        cpuload_stat_stop();
    } else {
        shell_print(shell,"usage:\n");
        shell_print(shell,"  cpuload start\n");
        shell_print(shell,"  cpuload stop\n");

        return -EINVAL;
    }

    return 0;
}
#endif	/* CONFIG_CPU_LOAD_STAT */

#if defined(CONFIG_FAT_FILESYSTEM_ELM)
static int shell_cmd_dumpbuf(const struct shell *shell,
                    size_t argc, char **argv)
{
    struct fs_file_t zfp;
    uint32_t addr, len;
    int res;

    if (argc < 4) {
        shell_print(shell, "usage:\n");
        shell_print(shell, "  dumpbuf addr len path\n");
        return -EINVAL;
    }

    fs_file_t_init(&zfp);
    res = fs_open(&zfp, argv[3], FS_O_WRITE | FS_O_CREATE);
    if (res < 0) {
        shell_print(shell,"fail to open %s\n", argv[3]);
        return res;
    }

    addr = strtoul(argv[1], NULL, 0);
    len = strtoul(argv[2], NULL, 0);

    fs_write(&zfp, (void *)addr, len);
    fs_close(&zfp);

    shell_print(shell,"done dumping to %s\n", argv[3]);

    return 0;
}
#endif /* CONFIG_FAT_FILESYSTEM_ELM */

#if defined(CONFIG_MSG_MANAGER)
extern void msg_manager_dump_busy_msg(void);

static int shell_cmd_dumpmsg(const struct shell *shell,
                    size_t argc, char **argv)
{
    msg_manager_dump_busy_msg();
    return 0;
}
#endif /* defined(CONFIG_MSG_MANAGER) */


#if RBUF_POOL_DBG && defined(CONFIG_RBUF_LIB)
static int shell_cmd_dumprbuf(const struct shell *shell,
                    size_t argc, char **argv)
{
    rbuf_pool_dump((rbuf_pool_t *)RB_ST_POOL);
    return 0;
}
#endif

#if defined(CONFIG_UI_INPUT_RECORDER)
#include <input_recorder.h>

#if defined(CONFIG_FILE_STREAM)
#include <file_stream.h>

#define INPUTREC_BUFFER_SIZE 0
#ifdef CONFIG_MASS_STORAGE_DISK_NAME
#define INPUTREC_DEFAULT_FILE_PATH "/" CONFIG_MASS_STORAGE_DISK_NAME ":/.input.rec"
#else
#define INPUTREC_DEFAULT_FILE_PATH "/SD:/.input.rec"
#endif

static int shell_cmd_input_record(const struct shell *shell,
                    size_t argc, char **argv)
{
#if INPUTREC_BUFFER_SIZE > 0
    static uint8_t rec_buffer[INPUTREC_BUFFER_SIZE];
    static uint32_t rec_size;
#else
    static io_stream_t rec_stream;
#endif
    bool to_capture;
    bool to_start;
    bool repeat = false;
    int res = 0;

    if (argc < 3) {
        shell_print(shell, "usage:\n");
        shell_print(shell, "  inputrec {record|play} {repeat|start|stop} [file]\n");
        return -EINVAL;
    }

    if (strcmp(argv[1], "record") == 0) {
        to_capture = true;
    } else if (strcmp(argv[1], "play") == 0) {
        to_capture = false;
    } else {
        return -EINVAL;
    }

    if (strcmp(argv[2], "start") == 0) {
        to_start = true;
    } else if (strcmp(argv[2], "stop") == 0) {
        to_start = false;
    } else if (!to_capture && strcmp(argv[2], "repeat") == 0) {
        to_start = true;
        repeat = true;
    } else {
        return -EINVAL;
    }

#if INPUTREC_BUFFER_SIZE > 0
    if (to_capture) {
        if (to_start) {
            res = input_capture_buffer_start(rec_buffer, sizeof(rec_buffer));
        } else {
            int num = input_capture_stop();
            if (num >= 0) {
                rec_size = num * sizeof(input_rec_data_t);
                shell_print(shell, "save %d records\n", num);
            }
        }
    } else {
        if (to_start) {
            res = input_playback_buffer_start(rec_buffer, rec_size, repeat);
            //input_playback_slide_fixstep_start(10, 300, 10, true);
        } else {
            input_playback_stop();
        }
    }
#else /* INPUTREC_BUFFER_SIZE > 0 */
    if (to_start) {
        const char *path = (argc > 3) ? argv[3] : INPUTREC_DEFAULT_FILE_PATH;

        if (rec_stream) {
            shell_print(shell, "must stop last record/play first\n");
            return -EPERM;
        }

        rec_stream = file_stream_create(path);
        if (rec_stream) {
            if (stream_open(rec_stream, to_capture ? MODE_OUT : MODE_IN)) {
                stream_destroy(rec_stream);
                rec_stream = NULL;
            }
        }

        if (rec_stream == NULL) {
            shell_print(shell, "failed to open file %s\n", path);
            return -EPERM;
        }
    } else if (rec_stream == NULL) {
        return 0;
    }

    if (to_capture) {
        if (to_start) {
            res = input_capture_stream_start(rec_stream);
        } else {
            shell_print(shell, "save %d records\n", input_capture_stop());
        }
    } else {
        if (to_start) {
            res = input_playback_stream_start(rec_stream, repeat);
            //input_playback_slide_fixstep_start(10, 300, 10, true);
        } else {
            input_playback_stop();
        }
    }

    if (!to_start || res) {
        if (rec_stream) {
            stream_close(rec_stream);
            stream_destroy(rec_stream);
            rec_stream = NULL;
        }
    }
#endif /* INPUTREC_BUFFER_SIZE > 0 */

    return res;
}
#endif /* defined(CONFIG_STREAM) */

static int shell_cmd_input_autoscrl(const struct shell *shell,
                    size_t argc, char **argv)
{
    bool to_start = true;
    bool is_vert = true;
    int16_t start, stop, step;
    int res = -EINVAL;

    if (argc < 2) {
        goto exit;
    }

    if (strcmp(argv[1], "stop") == 0) {
        to_start = false;
    } else {
        if (argc < 5) {
            goto exit;
        }

        if (strcmp(argv[1], "hstart") == 0) {
            is_vert = false;
        } else if (strcmp(argv[1], "vstart") == 0) {
            is_vert = true;
        } else {
            goto exit;
        }
    }

    start = (int16_t)atoi(argv[2]);
    stop = (int16_t)atoi(argv[3]);
    step = (int16_t)atoi(argv[4]);

    if (to_start) {
        input_playback_slide_fixstep_start(start, stop, step, is_vert);
    } else {
        input_playback_stop();
    }

    res = 0;
exit:
    if (res) {
        shell_print(shell, "usage:\n");
        shell_print(shell, "  inputscrl {hstart|vstart|stop} [{start_coord} {end_coord} {step}]\n");
    }

    return 0;
}

#endif /* defined(CONFIG_UI_INPUT_RECORDER) */

#if defined(CONFIG_INPUT_DEV_ACTS_CST816S_TP_KEY)
extern void tpkey_acts_dump(void);

static int shell_cmd_tp(const struct shell *shell,
                    size_t argc, char **argv)
{
    tpkey_acts_dump();
    return 0;
}
#endif

#if defined(CONFIG_EXTEND_GPIO_ET6416_SHELL)
#include <drivers/gpio.h>
static int shell_cmd_exgpio(const struct shell *shell,
                    size_t argc, char **argv)
{

    int gpio, val = 0;
    const struct device *gpio_dev;

    if (argc < 3) {
        shell_print(shell, "exgpio in/out num [val]\n");
        return -EINVAL;
    }
    gpio = strtoul(argv[2], NULL, 0);
    if (argc >= 4)
        val = strtoul(argv[3], NULL, 0);

    gpio_dev = device_get_binding(CONFIG_EXTEND_GPIO_NAME);
    if (!gpio_dev){
        shell_print(shell,"extern gpio dev get fail\n");
        return -EINVAL;
    }

    if(strcmp(argv[1], "out") == 0){
        gpio_pin_configure(gpio_dev, gpio, GPIO_OUTPUT);
        gpio_pin_set(gpio_dev, gpio, val);
        shell_print(shell,"exgpio%d,out, val=%d\n", gpio, val);

    }else{

        gpio_pin_configure(gpio_dev, gpio, GPIO_INPUT);
        k_msleep(100);
        val = gpio_pin_get(gpio_dev, gpio);
        shell_print(shell,"exgpio%d, in, val=%d\n", gpio, val);
    }

    return 0;

}

#endif



#if defined(CONFIG_ACTIONS_PRINTK_DMA)

static int shell_cmd_printk_by_dma_cpu(const struct shell *shell,
                    size_t argc, char **argv)
{
    if(argc != 2){
        shell_print(shell, "dbgsw cpu[dma]\n");
        return -EINVAL;
    }
    if (strcmp(argv[1], "cpu") == 0){
        shell_print(shell, "switch to cpu print\n");
        printk_dma_switch(0);
    }else{
        shell_print(shell, "switch to dma print\n");
        printk_dma_switch(1);
    }

    return 0;
}

#endif

#if defined(CONFIG_SOC_SPICACHE_PROFILE_STAT)
extern int shell_cmd_spicache_profile_stat(const struct shell *shell,
                  size_t argc, char **argv);
#endif

#if defined(CONFIG_TRACING_IRQ_PROFILER)

#include <arch/cpu.h>
#if defined(CONFIG_CPU_CORTEX_M)
#include <arch/arm/aarch32/cortex_m/cmsis.h>
#elif defined(CONFIG_CPU_CORTEX_A) || defined(CONFIG_CPU_CORTEX_R)
#include <drivers/interrupt_controller/gic.h>
#endif

void dump_irqstat(void)
{
    struct _isr_table_entry *ite;
    int i;

    printk("IRQ statistics:\n");
    printk("irq  prio    count        max_time (us)        total (us) isr\n");

    for (i = 0; i < IRQ_TABLE_SIZE; i++) {
        ite = &_sw_isr_table[i];

        if (ite->isr != z_irq_spurious) {
            if(irq_is_enabled(i)){
                printk("%2d(*) %2d   %10d", i, NVIC_GetPriority(i), ite->irq_cnt);
            }else{
                printk("%2d    %2d   %10d", i, NVIC_GetPriority(i), ite->irq_cnt);
            }
            printk("    %10d",
                   (u32_t)(k_cyc_to_ns_floor64(ite->max_irq_cycles) / 1000));

            printk("    %10d",
                   ite->irq_total_us);

#ifdef CONFIG_KALLSYMS
            print_symbol("            %s", (unsigned long)ite->isr);
#endif
            printk("\n");
        }
    }
}

static int shell_cmd_irqstat(const struct shell *shell,int argc, char *argv[])
{
    struct _isr_table_entry *ite;
    int i;
    unsigned int key;

    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    dump_irqstat();

    if (argc >= 2 && !strncmp(argv[1], "clear", strlen(argv[1]))) {
        key = irq_lock();

        /* clear irq statistics */
        for (i = 0; i < IRQ_TABLE_SIZE; i++) {
            ite = &_sw_isr_table[i];

            if (ite->isr != z_irq_spurious) {
                ite->irq_cnt = 0;
                ite->max_irq_cycles = 0;
                ite->irq_total_us = 0;
            }
        }
        irq_unlock(key);
    }

    return 0;
}
#endif /* CONFIG_IRQ_STAT */

#if defined(CONFIG_RES_MANAGER)
extern void res_manager_dump_info(void);
extern void bitmap_font_dump_info(void);

static int shell_cmd_dump_res(const struct shell *shell,
                    size_t argc, char **argv)
{
    res_manager_dump_info();
    bitmap_font_dump_info();
    return 0;
}
#endif

#ifdef CONFIG_ACTS_HRTIMER
#include <drivers/hrtimer.h>

static struct hrtimer g_ht;
static void htimer_fun(struct hrtimer *ttimer, void *expiry_fn_arg)
{
	static int t;
	printk("%d ---htimer--\n", t++);
}

static void htimer_test(unsigned int ms)
{
	hrtimer_init(&g_ht, htimer_fun, NULL);
	hrtimer_start(&g_ht, 1000*ms, 1000*ms);
}


static int shell_cmd_wksrc(const struct shell *shell,
                    size_t argc, char **argv)
{
	unsigned int ms=1000;
	if(argc == 2)
		ms = strtoul(argv[1], NULL, 10);
	htimer_test(ms);
    sys_s3_wksrc_set(SLEEP_WK_SRC_T1);
    shell_print(shell, "set t1 %dms wakeup\n", ms);
    return 0;
}
#endif

#if defined(CONFIG_RTC_ACTS)

#include <drivers/rtc.h>

static void rtc_help(void)
{
    printk("usage:\n");
    printk("Get the current rtc time e.g. rtc\n");
    printk("Set the rtc time e.g. rtc set 2019-09-11 18:14:54\n");
}

static void show_rtc_time(void)
{
    struct rtc_time tm;
    const struct device *dev = device_get_binding(CONFIG_RTC_0_NAME);
    if (!dev) {
        printk("rtc dev binding failed!\n");
        return;
    }
    rtc_get_time(dev, &tm);
    print_rtc_time(&tm);
}

static void ymdstring_to_tm(const char *timestr, struct rtc_time *tm)
{
#ifdef CONFIG_MINIMAL_LIBC
    char *p, *s;
    const char *split = "-";

    if (!timestr || !tm)
        return ;

    p = strtok_r((char *)timestr, split, &s);
    if (!p)
        return;

    tm->tm_year = strtoul(p, NULL, 10);
    tm->tm_year -= 1900;

    p = strtok_r(NULL, split, &s);
    if (!p)
        return;

    tm->tm_mon = strtoul(p, NULL, 10);
    tm->tm_mon -= 1;

    p = strtok_r(NULL, split, &s);
    if (!p)
        return;

    tm->tm_mday = strtoul(p, NULL, 10);
#else
    char *p;
    const char *split = "-";

    if (!timestr || !tm)
        return ;

    p = strtok((char *)timestr, split);
    if (!p)
        return;

    tm->tm_year = strtoul(p, NULL, 10);
    tm->tm_year -= 1900;

    p = strtok(NULL, split);
    if (!p)
        return;

    tm->tm_mon = strtoul(p, NULL, 10);
    tm->tm_mon -= 1;

    p = strtok(NULL, split);
    if (!p)
        return;

    tm->tm_mday = strtoul(p, NULL, 10);
#endif
}

static void hmsstring_to_tm(const char *timestr, struct rtc_time *tm)
{
#ifdef CONFIG_MINIMAL_LIBC
    char *p, *s;
    const char *split = ":";

    if (!timestr || !tm)
        return ;

    p = strtok_r((char *)timestr, split, &s);
    if (!p)
        return;

    tm->tm_hour = strtoul(p, NULL, 10);

    p = strtok_r(NULL, split, &s);
    if (!p)
        return;

    tm->tm_min = strtoul(p, NULL, 10);

    p = strtok_r(NULL, split, &s);
    if (!p)
        return;

    tm->tm_sec = strtoul(p, NULL, 10);
#else
    char *p;
    const char *split = ":";

    if (!timestr || !tm)
        return ;

    p = strtok((char *)timestr, split);
    if (!p)
        return;

    tm->tm_hour = strtoul(p, NULL, 10);

    p = strtok(NULL, split);
    if (!p)
        return;

    tm->tm_min = strtoul(p, NULL, 10);

    p = strtok(NULL, split);
    if (!p)
        return;

    tm->tm_sec = strtoul(p, NULL, 10);
#endif
}

static int shell_cmd_rtc(const struct shell *shell, size_t argc, char **argv)
{
    struct rtc_time tm = {0};
    const struct device *dev = device_get_binding(CONFIG_RTC_0_NAME);
    if (!dev) {
        printk("Failed to get the RTC device\n");
        return -ENXIO;
    }

    if (!strcmp(argv[1], "help")) {
        rtc_help();
    } else if (!strcmp(argv[1], "set")) {
        if (argc != 4) {
            rtc_help();
            return -EPERM;
        }
        ymdstring_to_tm(argv[2], &tm);
        hmsstring_to_tm(argv[3], &tm);
        print_rtc_time(&tm);
        if (rtc_set_time(dev, &tm))
            printk("Failed to set rtc time\n");
        else
            printk("rtc set time successfully\n");
    } else {
        show_rtc_time();
    }

    return 0;
}
#endif

#ifdef CONFIG_ALARM8HZ_ACTS

#include <drivers/alarm.h>

void shell_cmd_alarm8hz_cb_func(const void *cb_data)
{
    const struct device *dev = (const struct device *)cb_data;
    struct alarm_config config = {0};

    config.alarm_msec = 1000;
    config.cb_fn = shell_cmd_alarm8hz_cb_func;
    config.cb_data = dev;

    printk("** On alarm8hz **\n");

    acts_alarm_set_alarm(dev, &config, true);
}

static int shell_cmd_alarm8hz(const struct shell *shell, size_t argc, char **argv)
{
    const struct device *dev = device_get_binding(CONFIG_ALARM8HZ_0_NAME);
    struct alarm_config config = {0};
    uint32_t alarm_msec = 1000;

    if (!dev) {
        printk("failed to get alarm8hz device:%s\n", CONFIG_ALARM8HZ_0_NAME);
        return -ENXIO;
    }

    if ((argc > 1) && (!strcmp(argv[1], "enable"))) {
        config.alarm_msec = alarm_msec;
        config.cb_fn = shell_cmd_alarm8hz_cb_func;
        config.cb_data = dev;
        acts_alarm_set_alarm(dev, &config, true);
    } else if ((argc > 1) && (!strcmp(argv[1], "disable"))) {
        acts_alarm_set_alarm(dev, NULL, false);
    }

    return 0;
}
#endif

#if defined(CONFIG_ACTIONS_ARM_MPU)
#include <act_arm_mpu.h>
static int shell_mpu_set(const struct shell *shell, size_t argc, char **argv)
{
	uint32_t mpu_base, mpu_size, attr;
	if (argc != 4 ) {
		shell_print(shell, "usage: mpu_set mem_base size ro[no/rw/ro] \n");
		return -1;
	}
	mpu_base = strtoul(argv[1], NULL, 0);
	mpu_size = strtoul(argv[2], NULL, 0);
	if(!strcmp("no", argv[3]))
		attr = 0;
	else if (!strcmp("rw", argv[3]))
		attr = 1;
	else
		attr = 2;
	if(mpu_base)
		act_mpu_set(MPU_CHAN_SHELL, mpu_base, mpu_size, attr);
	else
		act_mpu_unset(MPU_CHAN_SHELL);

	return 0;
}

#endif

#if defined(CONFIG_ACTS_DVFS_DYNAMIC_LEVEL)
#include <dvfs.h>

static int shell_cmd_dvfs(const struct shell *shell, size_t argc, char **argv)
{
	uint32_t level;

	if (argc != 3) {
		printk("usage: dvfs set/unset level\n");
		return -1;
	}

	level = strtoul(argv[2], NULL, 10);

	if (!strcmp(argv[1], "set")) {
		dvfs_set_level(level, "shell_dvfs");
	} else if (!strcmp(argv[1], "unset")) {
		dvfs_unset_level(level, "shell_dvfs");
	}

	return 0;
}
#endif

#if defined(CONFIG_ACTS_POWER_ONOFF_TEST)
#include <drivers/nvram_config.h>
static struct k_delayed_work  power_work;
static void power_off_work(struct k_work *work)
{
	unsigned int power_cnt = 0;
	if(nvram_config_get("power_cnt", &power_cnt, 4) < 0){
		printk("nvram get power_cnt fail\n");
	}
	power_cnt++;
	nvram_config_set("power_cnt", &power_cnt, 4);
	printk("------power off count=%d----------\n", power_cnt);
	soc_pmu_alarm8hz_enable(4000); //4 s wakeup
	sys_pm_poweroff();
}
static void power_off_test(void)
{
	unsigned int power_en = 0;
	if(nvram_config_get("power_en", &power_en, 4) < 0){
		printk("nvram get power_en fail\n");
	}else{
		printk("nvram get power_en=%d\n", power_en);
	}
	if(power_en){
		printk("----power_off_test---\n");
		k_delayed_work_init(&power_work, power_off_work);
		k_delayed_work_submit(&power_work, K_MSEC(8000));
	}
}
void power_onoff_test(unsigned int enable)
{
	unsigned int power_cnt = 0;
	nvram_config_set("power_en", &enable, 4);
	if(enable){
		nvram_config_set("power_cnt", &power_cnt, 4);
		printk("******power onoff test start*********\n");
		power_off_work(NULL);
	}else{
		printk("******power onoff test stop*********\n");
	}
}
static int shell_cmd_onoff(const struct shell *shell, size_t argc, char **argv)
{
	if (argc != 2) {
		printk("usage: onoff  enable/disable\n");
		return -1;
	}
	if (!strcmp(argv[1], "enable")) {
		power_onoff_test(1);
	}else if (!strcmp(argv[1], "disable")){
		power_onoff_test(0);
	}else{
		printk("err usage: onoff  enable/disable\n");
	}
	return 0;
}

static int power_onoff_init(const struct device *arg)
{
	power_off_test();
	return 0;
}
SYS_INIT(power_onoff_init, APPLICATION, 91);

#endif

#if defined(CONFIG_DEBUG_RAMDUMP)
static int shell_cmd_ramdump(const struct shell *shell, size_t argc, char **argv)
{
	if (argc != 2) {
		printk("usage: ramdump save/dump/reset\n");
		return -1;
	}
	if (!strcmp(argv[1], "save")) {
		ramdump_save(NULL, 0);
	}else if (!strcmp(argv[1], "dump")){
		ramdump_dump();
	}else if (!strcmp(argv[1], "reset")){
		ramdump_reset();
	}else{
		printk("usage: ramdump save/dump/reset\n");
	}
	return 0;
}
#endif

#if defined(CONFIG_POWER_TEST)
#include <sys_wakelock.h>
#include <sys_manager.h>
#define HOSC_CTL_READY_SHIFT        28
#define HOSC_CTL_READY_MASK         (0x1 << HOSC_CTL_READY_SHIFT)
const uint32_t backup_regs_addr[] = {
	 CMU_S1CLKCTL,
	 NVIC_ISER0,
	 NVIC_ISER1,
	 CMU_SPI0CLK,
};
static uint32_t reg_backups[ARRAY_SIZE(backup_regs_addr)];
__act_s2_sleep_data void cpu_test_mode_check(uint8_t mode)
{
	uint32_t corepll_backup, sysclk_bak, key = 0;
	int i;
	bool need_switch_clk = false;
	bool need_enter_wfi = false;
	static bool lock_flag = false;

	if (mode == 1) {
		need_enter_wfi = true;
		need_switch_clk = false;
	} else if (mode == 2) {
		need_enter_wfi = true;
		need_switch_clk = true;
	} else if (mode == 3) {
		if (lock_flag == false) {
			sys_wake_lock(PARTIAL_WAKE_LOCK);
			lock_flag = true;
		}
		return;
	} else {
		if (lock_flag == true) {
			sys_wake_unlock(PARTIAL_WAKE_LOCK);
			lock_flag = false;
		}
		return;
	}

	printk("test mode: %d\n", mode);
	soc_udelay(10000);
	k_sched_lock();
	key = irq_lock();

	if (need_enter_wfi == true) {
		for (i = 0; i < ARRAY_SIZE(backup_regs_addr); i++) {
			reg_backups[i] = sys_read32(backup_regs_addr[i]);
		}

		soc_pmu_check_onoff_reset_func();
		sys_write32(sys_read32(PMU_INTMASK) | (1 << 1), PMU_INTMASK); /* ONOFF SHORT WAKEUP */
		sys_write32(sys_read32(NVIC_ISER0), NVIC_ICER0);
		sys_write32(sys_read32(NVIC_ISER1), NVIC_ICER1);
		sys_write32(1 << (IRQ_ID_PMU - 32), NVIC_ISER1);
		/* wksrc additional settings for version-B */
		if (soc_dvfs_opt()) {
			sys_write32((sys_read32(MEMORYCTL2) & ~(0x1ff<<15)) | WK_ID_PMU, MEMORYCTL2);
		}
	}

	if (need_switch_clk == true) {
		sysclk_bak =  sys_read32(CMU_SYSCLK);
		/*first switch cpu clk source (hosc)*/
		sys_write32((sysclk_bak&(~0x7)) | 0x1, CMU_SYSCLK);
		soc_udelay(1);
		/*cpu clk select HOSC*/
		if (soc_dvfs_opt())
			sys_write32(0x1, CMU_SYSCLK); /* AHB /2 */
		else
			sys_write32(0x201, CMU_SYSCLK); /* AHB /1 */

		corepll_backup = sys_read32(COREPLL_CTL);

		/* disable COREPLL */
		sys_write32(sys_read32(COREPLL_CTL) & ~(1 << 7), COREPLL_CTL);
		sys_write32(0, COREPLL_CTL);

		/*disable avdd, corepll use must enable*/
		sys_clear_bit(AVDDLDO_CTL, 0);

		/* RC64M enable for spi0 clk switch to rc64m*/
		sys_set_bit(CMU_S1CLKCTL,2);
		/*cpu clk select rc4M*/
		if (soc_dvfs_opt())
			sys_write32(0x0, CMU_SYSCLK);
		else
			sys_write32(0x02000230, CMU_SYSCLK);

		soc_udelay(100);
		/* NOR use RC64M */
		sys_write32((sys_read32(CMU_SPI0CLK) & ~(0x3 << 8) & ~(0xf << 0)) | 0x301, CMU_SPI0CLK);
		soc_udelay(100);
	}

	__asm__ volatile("cpsid	i");
	irq_unlock(key);
	if (need_enter_wfi == true) {
		__asm__ volatile("wfi");

	}
	key = irq_lock();
	__asm__ volatile("cpsie	i");

	if (need_switch_clk == true) {
		/* CPU USE RC64M */
		if (soc_dvfs_opt())
			sys_write32(0x03, CMU_SYSCLK);//AHB /2

		/*enable avdd, for pll*/
		sys_write32(((sys_read32(AVDDLDO_CTL)
					& (~(1<<AVDDLDO_CTL_AVDD_PD_EN)))
					| (1<<AVDDLDO_CTL_AVDD_EN)), AVDDLDO_CTL);
		soc_udelay(1);

		for(i = 0; i < 300; i++){
			if(sys_read32(HOSC_CTL) & HOSC_CTL_READY_MASK)
				break;
			soc_udelay(5);
		}
		if (!soc_dvfs_opt())
			soc_udelay(200);

		sys_write32(corepll_backup, COREPLL_CTL);
		/*spi0 clk switch to hosc*/
		sys_write32(sys_read32(CMU_SPI0CLK) & ~(0x3 << 8) & ~(0xf << 0), CMU_SPI0CLK);
		for(i = 0; i < 300; i++){
			if(sys_read32(COREPLL_CTL) & (1<<8))
				break;
			soc_udelay(5);
		}

		/*first switch clk ahb div*/
		sys_write32((sys_read32(CMU_SYSCLK)&0x7) | (sysclk_bak & (~0x7)) , CMU_SYSCLK);
		soc_udelay(1);
		sys_write32(sysclk_bak, CMU_SYSCLK);
		soc_udelay(100);
	}

	if (need_enter_wfi == true) {
		for (i = ARRAY_SIZE(backup_regs_addr) - 1; i >= 0; i--) {
			sys_write32(reg_backups[i], backup_regs_addr[i]);
		}
	}
	irq_unlock(key);
	k_sched_unlock();
	printk("out\n");
}

static int shell_cmd_powertest(const struct shell *shell, size_t argc, char **argv)
{
	uint8_t mode = strtoul(argv[1], NULL, 10);
	system_request_fast_standby();
	os_sleep(1000);
	if (system_is_screen_on() == false) {
		cpu_test_mode_check(mode);
	}
	return 0;
}
#endif

#if defined(CONFIG_DEBUG_TRACEDUMP)
static int shell_cmd_tracedump(const struct shell *shell, size_t argc, char **argv)
{
	static uint32_t trace_id = 0;
	uint32_t offset, size;

	if (argc < 2) {
		printk("usage: tracedump enable/save/dump/reset/filter\n");
		return -1;
	}
	if (!strcmp(argv[1], "enable")) {
		if (argc >= 4) {
			tracedump_set_enable(atoi(argv[2]), atoi(argv[3]));
		}
	}else if (!strcmp(argv[1], "save")) {
		tracedump_save(TRACE_HEAP, trace_id++, (uint32_t)argv[1]);
	}else if (!strcmp(argv[1], "dump")){
		tracedump_dump();
	}else if (!strcmp(argv[1], "reset")) {
		tracedump_reset();
	}else if (!strcmp(argv[1], "filter")) {
		if (argc >= 4) {
			offset = strtoul(argv[2], NULL, 16);
			size = strtoul(argv[3], NULL, 16);
			tracedump_set_filter(offset, size);
		}
	}else{
		printk("usage: tracedump enable/save/dump/reset/filter\n");
	}
	return 0;
}
#endif

#if defined(CONFIG_INPUT_DEV_ACTS_TEST_FPS_FAKE_TP)
static int shell_cmd_fps_fake_tp(const struct shell *shell, size_t argc, char **argv)
{
    bool to_hor = 0;
    int32_t all_move = -466;
    int32_t move_pix = 10;
    if(argc >= 2) {
        if(argv[1][0] != '0')
            to_hor = 1;
        if(argc >= 3) {
            char *c_move = argv[2];
            all_move = 1;
            if(argv[2][0] == '-') {
                c_move++;
                all_move = -1;
            }   
            all_move = strtoul(c_move, NULL, 10) * all_move;
            if(argc >= 4) {
                c_move = argv[3];
                if(argv[3][0] == '-')
                    c_move++;
                move_pix = strtoul(c_move, NULL, 10);
            }
        }
    }
    extern void tpkey_start_fps_fake_tp(bool to_hor, int32_t all_move, int16_t move_pix);
    tpkey_start_fps_fake_tp(to_hor, all_move, move_pix);

	return 0;
}
#endif

SHELL_STATIC_SUBCMD_SET_CREATE(sub_dbg,
#if defined(CONFIG_CMD_MEMORY)
    SHELL_CMD(mdw, NULL, "display memory by word: mdw address [,count]" , shell_cmd_mdw ),
    SHELL_CMD(mdh, NULL, "display memory by half-word: mdh address [,count]" , shell_cmd_mdh),
    SHELL_CMD(mdb, NULL, "display memory by byte: mdb address [,count]" , shell_cmd_mdb),

    SHELL_CMD(mww, NULL, "memory write (fill) by word: mww address value [,count]" , shell_cmd_mww),
    SHELL_CMD(mwh, NULL, "memory write (fill) by half-word: mwh address value [,count]" , shell_cmd_mwh),
    SHELL_CMD(mwb, NULL,"memory write (fill) by byte: mwb address value [,count]" , shell_cmd_mwb),
#endif
#if defined(CONFIG_SOC_SPICACHE_PROFILE)
    SHELL_CMD(spicache_profile, NULL, "spicache_profile.", shell_cmd_spicache_profile),
#endif

#if defined(CONFIG_SOC_SPICACHE_PROFILE_STAT)
    SHELL_CMD(spicache_stat, NULL, "spicache_profile stat.", shell_cmd_spicache_profile_stat),
#endif

#if defined(CONFIG_CMD_SPINOR)
    SHELL_CMD(snort, NULL, "nor test : snort address size", shell_nor_speed_test),
    SHELL_CMD(fread, NULL, "flash read : fread dev[sd/spi_flash/spinand] offset", shell_flash_read),
#endif
#if defined(CONFIG_CMD_SPINAND)
    SHELL_CMD(snandr, NULL, "nand logic read: offset start address; size read len;", shell_nand_read),
    SHELL_CMD(snandw, NULL, "nand logic write: offset start address; size read len;", shell_nand_write),
    SHELL_CMD(snand_printlevel, NULL, "set spinand lib print level(1:err;2:warn;3:info;4:debug);", shell_nand_print_level),
#if defined(CONFIG_SPINAND_TEST_FRAMEWORKS)
    SHELL_CMD(snand_ioctrl, NULL, "spinand ioctrl test.", shell_nand_ioctrl),
    //SHELL_CMD(snandlt, NULL, "nand logic test: offset start address; size read len;", shell_nand_logic_speed_test),
    SHELL_CMD(snand_sdc, NULL, "nand delaychain scan.", shell_nand_delaychain_scan),
    SHELL_CMD(snand_flush, NULL, "nand manual flush.", shell_nand_flush),
#if defined(CONFIG_SPINAND_TEST_FRAMEWORKS_ADDITION)
    SHELL_CMD(snand_hardware_params, NULL, "spinand print hardware params.", shell_nand_hardware_params),
    SHELL_CMD(snand_software_params, NULL, "spinand print software params.", shell_nand_software_params),
    SHELL_CMD(snand_blk_erase_test, NULL, "spinand blk phy erase and readback test.", shell_nand_blk_erase_test),
    SHELL_CMD(snand_phy_page_write_test, NULL, "spinand phy page write test.", shell_nand_page_write_test),
    SHELL_CMD(snand_phy_page_read_test, NULL, "spinand phy page read test.", shell_nand_page_read_test),
    SHELL_CMD(snand_api_boundary_test, NULL, "spinand api boundary test.", shell_nand_api_boundary_test),
    SHELL_CMD(snand_ldl_page_read_test, NULL, "spinand ldl single page read test.", shell_nand_read_sigle_page_test),
    SHELL_CMD(snand_ldl_page_write_test, NULL, "spinand ldl single page write test.", shell_nand_write_sigle_page_test),
    SHELL_CMD(snand_page_readback_test, NULL, "spinand logic page readback test.", shell_nand_page_readback_test),
    SHELL_CMD(snand_zonetbl_rotation_test, NULL, "spinand zonetbl rotation test.", shell_nand_zonetbl_rotation_test),
    SHELL_CMD(snand_zonetbl_rewrite_test, NULL, "spinand zonetbl rewrite test.", shell_nand_rewrite_zonetbl_test),
    SHELL_CMD(snand_erase_zonetbl_blk_test, NULL, "spinand erase zonetbl blk test.", shell_nand_erase_zonetbl_blk_test),
    SHELL_CMD(snand_read_speed_test, NULL, "spinand read speed test.", shell_nand_read_speed_test),
    SHELL_CMD(snand_write_speed_test, NULL, "spinand write speed test.", shell_nand_write_speed_test),
    SHELL_CMD(snand_pdlinit_time_cost_test, NULL, "spinand pdl init time cost test.", shell_nand_pdlinit_time_cost_test),
    SHELL_CMD(snand_zonetbl_init_time_cost_test, NULL, "spinand zonetbl init time cost test.", shell_nand_zonetbl_init_time_cost_test),
    SHELL_CMD(snand_read_write_udisk, NULL, "spinand write and read udisk test.", shell_nand_read_write_udisk),
#endif
#endif
#endif
#if defined(CONFIG_DMA_DBG_DUMP)
    SHELL_CMD(dma_dump, NULL, "dma info", shell_dma_info),
#endif
#if defined(CONFIG_SD_FS)
    SHELL_CMD(sdfs, NULL, "sdfs name size", shell_sdfs_dump),
#endif
#if defined(CONFIG_NVRAM_CONFIG)
    SHELL_CMD(nvdump, NULL, "nvram dump", shell_nvram_dump),
    SHELL_CMD(nvram, NULL, "nvram name (val)", shell_nvram_cmd),
#endif

#if defined(CONFIG_DISPLAY_ENGINE)
    SHELL_CMD(de_dump, NULL, "de dump", shell_de_dump),
#endif

#if defined(CONFIG_DISPLAY_LCDC)
    SHELL_CMD(lcd_dump, NULL, "lcdc dump", shell_lcd_dump),
#endif

#if defined(CONFIG_BT_DRV)
    SHELL_CMD(bb-reg, NULL, "read/write btc bb ctl", shell_cmd_bqb_bb),
    SHELL_CMD(rf-reg, NULL, "read/write btc rf ctl", shell_cmd_bqb_rf),
#endif
#if defined(CONFIG_PM_LOG_LEVEL_INF)
    SHELL_CMD(sleep, NULL, "sleep", shell_cmd_sleep),
#endif
#if defined(CONFIG_KERNEL_SHOW_STACK)
    SHELL_CMD(show_stack, NULL, "show kernel stack", shell_show_stack),
#endif
    SHELL_CMD(reboot, NULL, "reboot [adfu]", shell_cmd_reboot),
    SHELL_CMD(kheap, NULL, "kheap [addr]", shell_cmd_khead_dump),

#if defined(CONFIG_UI_MEMORY_MANAGER)
    SHELL_CMD(uimem, NULL, "uimem dump [0|1|2], where 0-FB, 1-LVGL, 2-RES", shell_cmd_dump_uimem),
#endif
#if defined(CONFIG_UI_SERVICE)
    SHELL_CMD(uiview, NULL, "view dump", shell_cmd_dump_uiview),
#endif
#if defined(CONFIG_THREAD_RUNTIME_STATS)
    SHELL_CMD(cpuload, NULL, "show cpu load statistic preriodically", shell_cmd_cpuload),
#endif
#if defined(CONFIG_FAT_FILESYSTEM_ELM)
    SHELL_CMD(dumpbuf, NULL, "dump buffer to file system: dumpbuf addr len path", shell_cmd_dumpbuf),
#endif
#if defined(CONFIG_MSG_MANAGER)
    SHELL_CMD(dumpmsg, NULL, "dump message: dumpmsg", shell_cmd_dumpmsg),
#endif

#if RBUF_POOL_DBG && defined(CONFIG_RBUF_LIB)
	SHELL_CMD(dumprbuf, NULL, "dump rbuf in share ram", shell_cmd_dumprbuf),
#endif

#if defined(CONFIG_UI_INPUT_RECORDER)
    SHELL_CMD(inputscrl, NULL, "input scroll forth and back at hort/vert dir:"
            "inputscrl {hstart|vstart|stop} {start_coord} {end_coord} {step}",
            shell_cmd_input_autoscrl),

#if defined(CONFIG_FILE_STREAM)
    SHELL_CMD(inputrec, NULL, "input record: inputrec {record|play} {repeat|start|stop} [file]", shell_cmd_input_record),
#endif
#endif

#if defined(CONFIG_INPUT_DEV_ACTS_CST816S_TP_KEY)
    SHELL_CMD(tpnotify, NULL, "dump gesture and location", shell_cmd_tp),
#endif
#if defined(CONFIG_EXTEND_GPIO_ET6416_SHELL)
    SHELL_CMD(exgpio, NULL, "exgpio in/out num [val]", shell_cmd_exgpio),
#endif
#if defined(CONFIG_ACTIONS_PRINTK_DMA)
SHELL_CMD(dbgsw, NULL, "dbgsw cpu[dma]", shell_cmd_printk_by_dma_cpu),
#endif

#if defined(CONFIG_TRACING_IRQ_PROFILER)
    SHELL_CMD(irqstat, NULL, "irqstat (clear)", shell_cmd_irqstat),
#endif

#if defined(CONFIG_TOOL)
    //SHELL_CMD(tool, NULL, "tool [aset]", shell_cmd_tool_connect),
#endif

#if defined(CONFIG_RES_MANAGER)
    SHELL_CMD(resman, NULL, "resource dump", shell_cmd_dump_res),
#endif

#if defined(CONFIG_ACTS_HRTIMER)
    SHELL_CMD(wksrct, NULL, "wksrct  ms", shell_cmd_wksrc),
#endif

#if defined(CONFIG_RTC_ACTS)
    SHELL_CMD(rtc, NULL, "rtc get/set current time", shell_cmd_rtc),
#endif

#if defined(CONFIG_ALARM8HZ_ACTS)
    SHELL_CMD(alarm8hz, NULL, "alarm8hz set alarm time", shell_cmd_alarm8hz),
#endif

#if defined(CONFIG_ACTS_DVFS_DYNAMIC_LEVEL)
	SHELL_CMD(dvfs, NULL, "dvfs set/unset level", shell_cmd_dvfs),
#endif
#if defined(CONFIG_ACTIONS_ARM_MPU)
	SHELL_CMD(mpu_set, NULL, "mpu_set mem_base size ro[no/rw/ro]", shell_mpu_set),
#endif
#if defined(CONFIG_ACTS_POWER_ONOFF_TEST)
	SHELL_CMD(onoff, NULL, "onoff enable/disable", shell_cmd_onoff),
#endif
#if defined(CONFIG_DEBUG_RAMDUMP)
	SHELL_CMD(ramdump, NULL, "dump ram to flash", shell_cmd_ramdump),
#endif
#if defined(CONFIG_POWER_TEST)
	SHELL_CMD(powertest, NULL, "power consumption test", shell_cmd_powertest),
#endif
#if defined(CONFIG_DEBUG_TRACEDUMP)
	SHELL_CMD(tracedump, NULL, "dump trace info", shell_cmd_tracedump),
#endif
#if defined(CONFIG_INPUT_DEV_ACTS_TEST_FPS_FAKE_TP)
    SHELL_CMD(fps_fake_tp, NULL, "test fake tp", shell_cmd_fps_fake_tp),
#endif
    SHELL_SUBCMD_SET_END /* Array terminated. */
);

SHELL_CMD_REGISTER(dbg, &sub_dbg, "dbg commands", NULL);
