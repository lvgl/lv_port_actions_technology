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

#define MAX_LINE_LENGTH_BYTES (64)
#define DEFAULT_LINE_LENGTH_BYTES (16)
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

	if (argc < 3)
		return -EINVAL;

	addr = strtoul(argv[1], NULL, 16);
	writeval = strtoul(argv[2], NULL, 16);

	if (argc == 4)
		count = strtoul(argv[3], NULL, 16);
	else
		count = 1;

	buf = (char *)addr;
	while (count-- > 0) {
		if (width == 4)
			*((uint32_t *)buf) = (uint32_t)writeval;
		else if (width == 2)
			*((uint16_t *)buf) = (uint16_t)writeval;
		else
			*((uint8_t *)buf) = (uint8_t)writeval;
		buf += width;
	}

	return 0;
}

static int do_mem_md(const struct shell *shell, int width, size_t argc, char **argv)
{
	unsigned long addr;
	int count;

	if (argc < 2)
		return -EINVAL;

	addr = strtoul(argv[1], NULL, 16);

	if (argc == 3)
		count = strtoul(argv[2], NULL, 16);
	else
		count = 1;

	print_buffer(shell, (char *)addr, width, count, DISP_LINE_LEN / width, -1);

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
	interval_ms = (uint32_t) (profile->end_time-profile->start_time);

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

	hit = profile->total_hit_cnt*8;
	total = (hit + profile->total_miss_cnt);
	if (total != 0)
		shell_print(shell, "totoal hit: %12d    totoal miss: %12d    hit ratio: %d%%\n\n",
			hit, profile->total_miss_cnt,
			hit / (total / 100));
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
		if (argc < 4)
			return -EINVAL;

		profile->start_addr = strtoul(argv[2], NULL, 0);
		profile->end_addr = strtoul(argv[3], NULL, 0);

		shell_print(shell, "Start profile: addr range %08x ~ %08x\n",
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
	shell_print(shell,"\n");
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
	ret =  flash_erase(nor_dev, offset, size);
	 if(ret < 0 ) {
		shell_print(shell,"flash erase fail\n");
	 }
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
		shell_print(shell,"nor dev binding failed!\n");
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
		shell_print(shell,"nor dev binding failed!\n");
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

#if defined(CONFIG_PM_DIRECT_FORCE_MODE)
#include <pm/pm.h>
static int shell_cmd_sleep(const struct shell *shell,
					size_t argc, char **argv)
{
	static struct pm_state_info  pm_stat;

	if((argc == 2) && !strcmp(argv[1], "s3")) {
		shell_print(shell, "enter deep sleep\n");
		pm_stat.state = PM_STATE_SUSPEND_TO_RAM;
		pm_power_state_force(pm_stat);
		shell_print(shell, "exit deep sleep\n");
	}else{
		shell_print(shell, "enter sleep\n");
		pm_stat.state = PM_STATE_STANDBY;
		pm_power_state_force(pm_stat);
		shell_print(shell, "exit sleep\n");

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
	shell_print(shell, "reboot\n");
	sys_pm_reboot(REBOOT_TYPE_NORMAL);
	return 0;
}

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
		STRUCT_SECTION_FOREACH(k_heap, h) {
			shell_print(shell, "----dump heap=%p:---\n", h);
			sys_heap_dump(&h->heap);
		}
	}

	return 0;
}

#if defined(CONFIG_TOOL)
int tool_init(char *type);
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

    (void)shell->iface->api->uninit(shell->iface);

    tool_init(tool_type);
	return 0;
}

int shell_data_write(const uint8_t *data, uint32_t size, uint32_t timeout_ms)
{
    int count = 0;
    const struct shell *shell;

#if defined(CONFIG_SHELL_BACKEND_SERIAL)
    extern const struct shell *shell_backend_uart_get_ptr(void);
    shell = shell_backend_uart_get_ptr();
    (void)shell->iface->api->write(shell->iface, data, size, &count);
#endif
    return count;
}
#endif


#if defined(CONFIG_UI_MEMORY_MANAGER)
extern void ui_memory_dump_info(uint32_t index);

static int shell_cmd_dump_uimem(const struct shell *shell,
					size_t argc, char **argv)
{
	ui_memory_dump_info(-1);
	return 0;
}
#endif

#if defined(CONFIG_UI_SERVICE)
extern void view_manager_dump(void);

static int shell_cmd_dump_uiview(const struct shell *shell,
					size_t argc, char **argv)
{
	view_manager_dump();
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

#if defined(CONFIG_UI_INPUT_RECORDER) && defined(CONFIG_FILE_STREAM)
#include <file_stream.h>
#include <input_recorder.h>

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
#endif /* defined(CONFIG_UI_INPUT_RECORDER) && defined(CONFIG_STREAM) */

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

static int shell_cmd_dump_res(const struct shell *shell,
					size_t argc, char **argv)
{
	res_manager_dump_info();
	return 0;
}
#endif

static int shell_cmd_wksrc(const struct shell *shell,
					size_t argc, char **argv)
{
	
	sys_s3_wksrc_set(SLEEP_WK_SRC_T0);
	shell_print(shell, "set t0 wakeup\n");
	return 0;
}

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
	rtc_get_time(dev, &tm);
	print_rtc_time(&tm);
}

static void ymdstring_to_tm(const char *timestr, struct rtc_time *tm)
{
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
}

static void hmsstring_to_tm(const char *timestr, struct rtc_time *tm)
{
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
	SHELL_CMD(snandr, NULL, "nand logic read: offset start address; size read len.(alighed to byte)", shell_nand_read),
	SHELL_CMD(snandw, NULL, "nand logic write: offset start address; size read len.(alighed to byte)", shell_nand_write),
	//SHELL_CMD(snandlt, NULL, "nand logic test: offset start address; size read len;", shell_nand_logic_speed_test),
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
#if defined(CONFIG_PM_DIRECT_FORCE_MODE)
	SHELL_CMD(sleep, NULL, "sleep", shell_cmd_sleep),
#endif
#if defined(CONFIG_KERNEL_SHOW_STACK)
	SHELL_CMD(show_stack, NULL, "show kernel stack", shell_show_stack),
#endif
	SHELL_CMD(reboot, NULL, "reboot [adfu]", shell_cmd_reboot),
	SHELL_CMD(kheap, NULL, "kheap [addr]", shell_cmd_khead_dump),

#if defined(CONFIG_UI_MEMORY_MANAGER)
	SHELL_CMD(uimem, NULL, "uimem dump", shell_cmd_dump_uimem),
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

#if defined(CONFIG_UI_INPUT_RECORDER) && defined(CONFIG_FILE_STREAM)
	SHELL_CMD(inputrec, NULL, "input record: inputrec {record|play} {repeat|start|stop} [file]", shell_cmd_input_record),
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
	SHELL_CMD(tool, NULL, "tool [aset]", shell_cmd_tool_connect),
#endif

#if defined(CONFIG_RES_MANAGER)
	SHELL_CMD(resman, NULL, "resource dump", shell_cmd_dump_res),
#endif
	SHELL_CMD(wksrct, NULL, "t0 wksrc set", shell_cmd_wksrc),

#if defined(CONFIG_RTC_ACTS)
	SHELL_CMD(rtc, NULL, "rtc get/set current time", shell_cmd_rtc),
#endif

#if defined(CONFIG_ALARM8HZ_ACTS)
	SHELL_CMD(alarm8hz, NULL, "alarm8hz set alarm time", shell_cmd_alarm8hz),
#endif


	SHELL_SUBCMD_SET_END /* Array terminated. */
);

SHELL_CMD_REGISTER(dbg, &sub_dbg, "dbg commands", NULL);
