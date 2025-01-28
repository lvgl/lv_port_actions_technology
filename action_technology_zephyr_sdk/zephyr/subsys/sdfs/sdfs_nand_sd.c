
#include <zephyr/types.h>
#include <strings.h>
#include <stddef.h>
#include <sys/types.h>
#include <device.h>
#include <sdfs.h>
#include <storage/flash_map.h>
#include <partition/partition.h>
#include <linker/linker-defs.h>
#include <drivers/flash.h>
#include <spicache.h>
#include <board_cfg.h>

#if CONFIG_PSRAM_SIZE > 6144
#define S_CACHE_NUM		(16)
#define L_CACHE_NUM		(15)
#elif CONFIG_PSRAM_SIZE > 2048
#define S_CACHE_NUM		(8)
#define L_CACHE_NUM		(7)
#else
#define S_CACHE_NUM		(4)
#define L_CACHE_NUM		(1)
#endif

#define CACHE_INVALID	(0xffffffff)

#define S_CACHE_SZ		(2*1024)
#define L_CACHE_SZ		(32*1024)

#define ALL_CACHE_SZ	(S_CACHE_SZ*S_CACHE_NUM + L_CACHE_SZ*L_CACHE_NUM)
#define ALL_CACHE_NUM	(S_CACHE_NUM + L_CACHE_NUM)

#define TBUF_SEC_LEN	(CONFIG_SDFS_CACHE_BUF_LEN / 512)

typedef struct cache_item_s {
	sys_dnode_t node;  // used for link list*/
	uint32_t cache_off;  // offset in bytes
	uint32_t cache_len;  // in bytes
	u8_t *cache_buf;     // cache buffer
	uint32_t hit_cnt;    // hit count
} cache_item_t;

static sys_dlist_t	cache_list;
static uint32_t cache_hit_cnt;
static uint32_t cache_miss_cnt;

static cache_item_t __aligned(4) sdfs_cache_item[ALL_CACHE_NUM] __in_section_unique(RES_PSRAM_REGION);
static char __aligned(4) sdfs_cache_buf[ALL_CACHE_SZ] __in_section_unique(RES_PSRAM_REGION);

#ifdef CONFIG_SDFS_READ_BY_CACHE
static char __aligned(4) sdfs_tmp_buf[CONFIG_SDFS_CACHE_BUF_LEN] __in_section_unique(system.bss.sdfs_cache);
#else
static char __aligned(4) sdfs_tmp_buf[CONFIG_SDFS_CACHE_BUF_LEN] __in_section_unique(RES_PSRAM_REGION);
#endif

static const struct device *g_sd_dev;
static const struct device *g_nand_dev;
#if IS_ENABLED(CONFIG_SPI_FLASH_2)
static const struct device *g_data_nor_dev;
#endif


K_MUTEX_DEFINE(sdfs_mutex);

static void nand_sd_cache_init(void)
{
	uint32_t idx;
	cache_item_t *item = sdfs_cache_item;
	u8_t *buf = sdfs_cache_buf;
	
	sys_dlist_init(&cache_list);
	cache_hit_cnt = 0;
	cache_miss_cnt = 0;

	for (idx = 0; idx < ALL_CACHE_NUM; idx ++) {
		// init cache item
		sys_dnode_init(&item->node);
		item->cache_off = CACHE_INVALID;
		if (idx < S_CACHE_NUM) {
			item->cache_len = S_CACHE_SZ;
		} else {
			item->cache_len = L_CACHE_SZ;
		}
		item->cache_buf = buf;
		item->hit_cnt = 0;
		buf += item->cache_len;

		// add cache item to list
		sys_dlist_append(&cache_list, &item->node);
		item ++;
	}
}

static cache_item_t* nand_sd_cache_find(uint32_t addr, uint32_t size)
{
	cache_item_t *item;
	cache_item_t *cache_item = NULL;

	// check size
	if (size > (L_CACHE_SZ - S_CACHE_SZ / 2)) {
		return NULL;
	}

	// find node from first to last
	SYS_DLIST_FOR_EACH_CONTAINER(&cache_list, item, node) {
		if ((item->cache_off <= addr) && ((item->cache_off + item->cache_len) >= (addr + size))) {
			cache_hit_cnt ++;
			item->hit_cnt ++;
			cache_item = item;
			break;
		}
	}

	// move node to first
	if (cache_item && !sys_dlist_is_head(&cache_list, &cache_item->node)) {
		sys_dlist_remove(&cache_item->node);
		sys_dlist_prepend(&cache_list, &cache_item->node);
	}
	
	return cache_item;
}

static cache_item_t* nand_sd_cache_alloc(uint32_t addr, uint32_t size)
{
	cache_item_t *cache_item = (cache_item_t*)sys_dlist_peek_tail(&cache_list);
	uint32_t cache_len;

	// check size
	if (size > (L_CACHE_SZ - S_CACHE_SZ / 2)) {
		return NULL;
	}
	if (size > S_CACHE_SZ / 2) {
		cache_len = L_CACHE_SZ;
	} else {
		cache_len = S_CACHE_SZ;
	}

	// find node from last to first
	while (cache_item) {
		if (cache_item->cache_len == cache_len) {
			cache_miss_cnt ++;
			cache_item->cache_off = addr & ~(S_CACHE_SZ / 2 - 1);
			cache_item->hit_cnt = 0;
			break;
		}
		cache_item = (cache_item_t*)sys_dlist_peek_prev(&cache_list, &cache_item->node);
	}

	// move node to first
	if (cache_item && !sys_dlist_is_head(&cache_list, &cache_item->node)) {
		sys_dlist_remove(&cache_item->node);
		sys_dlist_prepend(&cache_list, &cache_item->node);
	}

	return cache_item;
}

static void nand_sd_cache_dump(uint32_t level)
{
	static uint32_t ratio_saved = 0;
	cache_item_t *item;
	uint32_t ratio = 0;

	if ( (cache_hit_cnt + cache_miss_cnt) > 0) {
		ratio = cache_hit_cnt * 100 / (cache_hit_cnt + cache_miss_cnt);
	}

	if (ratio_saved != ratio) {
		printk("[sdfs_cache] hit=%d, miss=%d, hit-rate=%d%%\n", cache_hit_cnt, cache_miss_cnt, ratio);
		ratio_saved = ratio;
	}

	if (level > 0) {
		// find node from first to last
		SYS_DLIST_FOR_EACH_CONTAINER(&cache_list, item, node) {
			if (item->cache_off != CACHE_INVALID) {
				printk("  [%d] off=0x%08x, len=%d\n", item->hit_cnt, item->cache_off, item->cache_len);
			}
		}
	}
}

static const struct device *nand_sd_dev(u8_t stor_id)
{
	if(STORAGE_ID_NAND == stor_id || STORAGE_ID_BOOTNAND == stor_id)
		return  g_nand_dev;
	if(STORAGE_ID_SD == stor_id)
		return  g_sd_dev;
#if IS_ENABLED(CONFIG_SPI_FLASH_2)	
	if(STORAGE_ID_DATA_NOR == stor_id)
		return  g_data_nor_dev;
#endif

	printk("sdfs dev err:%d\n", stor_id);
	return NULL;
}
#if 0
static void dump(void *data, size_t len)
{
	uint8_t *buf = data;
	size_t i, width = 8;

	for (i = 0; i < len; i++) {

		if ((i % width) == 0) {
			printk("0x%08lx\t", POINTER_TO_INT(buf + i));
		}

		printk("%02x ", buf[i]);

		if (((i + 1) % width) == 0 || i == (len - 1)) {
			printk("\n");
		}
	}
}
#endif
static int sdfs_flash_read(const struct device *dev, off_t offset, void *data,
			 size_t len)
{
	return flash_read(dev, offset<<9, data, len<<9);
}

static int nand_sd_data_read(const struct device *dev, char *buf,uint32_t adfs_addr, uint32_t size) 
{
	cache_item_t *item = nand_sd_cache_find(adfs_addr, size);
	uint32_t byte_off, sec_off, len;
	int ret;

//	printk("[sdfs] 0x%x %d\n", adfs_addr, size);

	// cache miss
	if (item == NULL) {
		item = nand_sd_cache_alloc(adfs_addr, size);
		if (item) {
#ifdef CONFIG_SDFS_READ_BY_CACHE
			// read flash to cache
			ret = sdfs_flash_read(dev, (item->cache_off >> 9), sdfs_tmp_buf, (item->cache_len >> 9));
			if(ret < 0)
				return -1;
			// copy to cache
			memcpy(item->cache_buf, sdfs_tmp_buf, item->cache_len);
#else
			ret = sdfs_flash_read(dev, (item->cache_off >> 9) , item->cache_buf, (item->cache_len >> 9));
			if(ret < 0)
				return -1;
#endif
		}
	}

	// cache copy
	if (item) {
		memcpy(buf, item->cache_buf + (adfs_addr - item->cache_off), size);
		return 0;
	}

	// large read
	byte_off = adfs_addr & 0x1ff;
	sec_off = adfs_addr >> 9;
#ifdef CONFIG_SDFS_READ_BY_CACHE
	sec_end = (adfs_addr + size + 0x1ff) >> 9;
	while (sec_off < sec_end) {
		// read flash
		len = sec_end - sec_off;
		if (len > TBUF_SEC_LEN) {
			len = TBUF_SEC_LEN;
		}
		ret = sdfs_flash_read(dev, sec_off, sdfs_tmp_buf, len);
		if(ret < 0)
			return -2;
		sec_off += len;
		
		// copy to buf
		len = (len << 9) - byte_off;
		if (len > size) {
			len = size;
		}
		memcpy(buf, sdfs_tmp_buf + byte_off, len);
		spi1_cache_ops(SPI_CACHE_FLUSH_ALL, buf + byte_off, len);
		buf += len;
		size -= len;
		byte_off = 0;
	}
#else
	// read first sector
	if (byte_off > 0) {
		// read flash
		ret = sdfs_flash_read(dev, sec_off, sdfs_tmp_buf, 1);
		if(ret < 0)
			return -1;
		sec_off += 1;
		
		// copy to buf
		len = 512 - byte_off;
		memcpy(buf, sdfs_tmp_buf + byte_off, len);
		buf += len;
		size -= len;
	}
	// read large
	if (size >= 512) {
		// read flash
		len = (size >> 9);
		ret = sdfs_flash_read(dev, sec_off, buf, len);
		if(ret < 0)
			return -1;
		sec_off += len;
		len = (len << 9);
		buf += len;
		size -= len;
	}
	// read last sector
	if (size > 0) {
		// read flash
		ret = sdfs_flash_read(dev, sec_off, sdfs_tmp_buf, 1);
		if(ret < 0)
			return -1;
		
		// copy to buf
		memcpy(buf, sdfs_tmp_buf, size);
	}
#endif
	// dump cache
	nand_sd_cache_dump(0);
	
	return 0;

}

static struct sd_dir * nand_sd_find_dir_by_addr(const struct device *dev, const char *filename, void *buf_size_32, uint32_t adfs_addr)
{
	int num, total, offset;
	struct sd_dir *sd_dir = buf_size_32;

	printk("read off=0x%x, file=%s\n", adfs_addr, filename);
	if(nand_sd_data_read(dev, (char *)buf_size_32, adfs_addr, sizeof(*sd_dir))){
		printk("dev=%s, read fail\n", dev->name);
		return NULL;
	}		

	if(memcmp(sd_dir->fname, "sdfs.bin", 8) != 0)
	{
		printk("sdfs.bin invalid, offset=0x%x\n", adfs_addr);
		return NULL;
	}
	total = sd_dir->offset;

	for(offset = adfs_addr + sizeof(*sd_dir), num = 0; num < total; offset += 32)
	{
		nand_sd_data_read(dev, (char *)buf_size_32, offset, 32);
		//printk("%d,file=%s, size=0x%x\n", num, sd_dir->fname, sd_dir->size);
		if(strncasecmp(filename, sd_dir->fname, 12) == 0)
		{
			return sd_dir;
		}
		num++;
	}

	return NULL;
}

struct sd_dir * nand_sd_find_dir(u8_t stor_id, u8_t part, const char *filename, void *buf_size_32)
{
	struct sd_dir * sd_d;
	const struct device *dev;
	const struct partition_entry *parti;

	dev = nand_sd_dev(stor_id);
	if(dev == NULL)
		return NULL;
	parti = partition_get_stf_part(stor_id, part+PARTITION_FILE_ID_SDFS_PART_BASE);
	if(parti == NULL)
		return NULL;

	k_mutex_lock(&sdfs_mutex, K_FOREVER);
	sd_d = nand_sd_find_dir_by_addr(dev, filename, buf_size_32, parti->offset);
	k_mutex_unlock(&sdfs_mutex);
	if(sd_d) {
		 sd_d->offset += parti->offset;
		return sd_d;
	}
	return NULL;
}


int nand_sd_sd_fread(u8_t stor_id, struct sd_file *sd_file, void *buffer, int len)
{
	const struct device *dev;
	dev = nand_sd_dev(stor_id);
	if(dev == NULL)
		return -1;
	k_mutex_lock(&sdfs_mutex, K_FOREVER);
	nand_sd_data_read(dev, (char *)buffer, sd_file->readptr, len);
	k_mutex_unlock(&sdfs_mutex);
	sd_file->readptr += len;
	return len;
}

static int nand_sd_fs_init(const struct device *dev)
{
	g_sd_dev = device_get_binding("sd");
	g_nand_dev = device_get_binding("spinand");

	if (!g_sd_dev) {
		printk("sdfs cannot found device sd\n");
	}
	if (!g_nand_dev) {
		printk("sdfs cannot found device spinand\n");
	}
#if IS_ENABLED(CONFIG_SPI_FLASH_2)
	g_data_nor_dev = device_get_binding(CONFIG_SPI_FLASH_2_NAME); 
#endif

	nand_sd_cache_init();
	//printk("sdfs nand init\n");
	return 0;
}

SYS_INIT(nand_sd_fs_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);



