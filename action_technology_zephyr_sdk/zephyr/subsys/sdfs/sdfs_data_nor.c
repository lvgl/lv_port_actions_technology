
#include <zephyr/types.h>
#include <strings.h>
#include <stddef.h>
#include <sys/types.h>
#include <device.h>
#include <sdfs.h>
#include <partition/partition.h>
#include <drivers/flash.h>
#include <board_cfg.h>

#if IS_ENABLED(CONFIG_SPI_FLASH_2)
static const struct device *g_data_nor_dev;

K_MUTEX_DEFINE(sdfs_mutex);

static const struct device *data_nor_sd_dev(u8_t stor_id)
{
	if (STORAGE_ID_DATA_NOR == stor_id)
		return g_data_nor_dev;

	printk("sdfs dev err:%d\n", stor_id);
	return NULL;
}

static struct sd_dir *data_nor_sd_find_dir_by_addr(const struct device *dev, const char *filename, void *buf_size_32, uint32_t adfs_addr)
{
	int num, total, offset;
	struct sd_dir *sd_dir = buf_size_32;

	printk("read off=0x%x, file=%s\n", adfs_addr, filename);
	if (flash_read(dev, adfs_addr, buf_size_32, sizeof(*sd_dir))) {
		printk("dev=%s, read fail\n", dev->name);
		return NULL;
	}		

	if (memcmp(sd_dir->fname, "sdfs.bin", 8) != 0) {
		printk("sdfs.bin invalid, offset=0x%x\n", adfs_addr);
		return NULL;
	}
	total = sd_dir->offset;

	for (offset = adfs_addr + sizeof(*sd_dir), num = 0; num < total; offset += 32) {
		flash_read(dev, offset, buf_size_32, 32);
		//printk("%d,file=%s, size=0x%x\n", num, sd_dir->fname, sd_dir->size);
		if (strncasecmp(filename, sd_dir->fname, 12) == 0) {
			return sd_dir;
		}
		num++;
	}

	return NULL;
}

struct sd_dir *data_nor_sd_find_dir(u8_t stor_id, u8_t part, const char *filename, void *buf_size_32)
{
	struct sd_dir *sd_d;
	const struct device *dev;
	const struct partition_entry *parti;

	dev = data_nor_sd_dev(stor_id);
	if (dev == NULL)
		return NULL;
	parti = partition_get_stf_part(stor_id, part+PARTITION_FILE_ID_SDFS_PART_BASE);
	if (parti == NULL)
		return NULL;

	k_mutex_lock(&sdfs_mutex, K_FOREVER);
	sd_d = data_nor_sd_find_dir_by_addr(dev, filename, buf_size_32, parti->offset);
	k_mutex_unlock(&sdfs_mutex);
	if (sd_d) {
		 sd_d->offset += parti->offset;
		return sd_d;
	}
	return NULL;
}

int data_nor_sd_fread(u8_t stor_id, struct sd_file *sd_file, void *buffer, int len)
{
	const struct device *dev;
	dev = data_nor_sd_dev(stor_id);
	if(dev == NULL)
		return -1;
	k_mutex_lock(&sdfs_mutex, K_FOREVER);
	flash_read(dev, sd_file->readptr, buffer, len);
	k_mutex_unlock(&sdfs_mutex);
	sd_file->readptr += len;
	return len;
}

static int data_nor_sd_fs_init(const struct device *dev)
{
	g_data_nor_dev = device_get_binding(CONFIG_SPI_FLASH_2_NAME); 

	//printk("data nor sdfs init\n");
	return 0;
}

SYS_INIT(data_nor_sd_fs_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
#endif
