#include <kernel.h>
#include <init.h>
#include <string.h>
#include <soc.h>
#include <crc.h>
#include <board_cfg.h>
#include <partition/partition.h>

#include <logging/log.h>

LOG_MODULE_REGISTER(partition, CONFIG_LOG_DEFAULT_LEVEL);

#define PARTITION_TABLE_MAGIC		0x54504341	// 'ACPT'

struct partition_table {
	u32_t magic;
	u16_t version;
	u16_t table_size;
	u16_t part_cnt;
	u16_t part_entry_size;
	u8_t reserved1[4];

	struct partition_entry parts[MAX_PARTITION_COUNT];
	u8_t Reserved2[4];
	u32_t table_crc;
};

static const struct partition_table *g_part_table;

void partition_dump(void)
{
	const struct partition_entry *part;
	char part_name[13];
	int i;

	printk("** Showing partition infomation **\n");
	printk("id  name      offset    type  file_id  mirror_id  flag\n");

	for (i = 0; i < MAX_PARTITION_COUNT; i++) {
		part = &g_part_table->parts[i];

		/* name field to str */
		memcpy(part_name, part->name, 8);
		part_name[8] = '\0';

		if (strlen(part_name) && part->type) {
			printk("%-4d%-10s0x%-8x%-6d%-9d%-11d%-6d\n",
				i, part_name, part->offset, part->type,
				part->file_id, part->mirror_id, part->flag);
		}
	}
}

int partition_get_max_file_size(const struct partition_entry *part)
{
	if (!part)
		return -EINVAL;

	return (part->size - (part->file_offset - part->offset));
}

const struct partition_entry *partition_find_part(u32_t nor_phy_addr)
{
	const struct partition_entry *part;
	unsigned int i;

	 __ASSERT_NO_MSG(g_part_table);

	for (i = 0; i < MAX_PARTITION_COUNT; i++) {
		part = &g_part_table->parts[i];

		if ((part->offset <= nor_phy_addr) &&
			((part->offset + part->size) > nor_phy_addr)) {

			return part;
		}
	}

	return NULL;
}

const struct partition_entry *partition_get_part_by_id(u8_t part_id)
{
	 __ASSERT_NO_MSG(g_part_table);

	if (part_id >= MAX_PARTITION_COUNT)
		return NULL;

	return &g_part_table->parts[part_id];
}

const struct partition_entry *partition_get_current_part(void)
{
	u32_t cur_phy_addr;
	const boot_info_t *p_boot_info = soc_boot_get_info();

	 __ASSERT_NO_MSG(g_part_table);

	cur_phy_addr = p_boot_info->system_phy_addr;

	return partition_find_part(cur_phy_addr);
}

u8_t partition_get_current_mirror_id(void)
{
	const struct partition_entry *part;

	part = partition_get_current_part();
	if (part == NULL)
		return -1;

	return part->mirror_id;
}

u8_t partition_get_current_file_id(void)
{
	const struct partition_entry *part;

	part = partition_get_current_part();
	if (part == NULL)
		return -1;

	return part->file_id;
}

int partition_is_mirror_part(const struct partition_entry *part)
{
	u8_t mirror_id = 0;
	const struct partition_entry *cur_part;
	const boot_info_t *p_boot_info = soc_boot_get_info();

	 __ASSERT_NO_MSG(g_part_table);

	if (partition_is_boot_part(part)) {
		cur_part = partition_find_part(p_boot_info->mbrec_phy_addr);
		if (cur_part) {
			mirror_id = cur_part->mirror_id;
		}
	} else if (partition_is_param_part(part)) {
		cur_part = partition_find_part(p_boot_info->param_phy_addr);
		if (cur_part) {
			mirror_id = cur_part->mirror_id;
		}
	} else {
		mirror_id = partition_get_current_mirror_id();
	}

	if ((part->mirror_id != mirror_id) && (part->mirror_id != PARTITION_MIRROR_ID_INVALID)) {
		return 1;
	}

	return 0;
}

int partition_is_param_part(const struct partition_entry *part)
{
	 __ASSERT_NO_MSG(g_part_table);

	if (part->type == PARTITION_TYPE_PARAM)
		return 1;

	return 0;
}

int partition_is_boot_part(const struct partition_entry *part)
{
	 __ASSERT_NO_MSG(g_part_table);

	if (part->type == PARTITION_TYPE_BOOT)
		return 1;

	return 0;
}

const struct partition_entry *partition_get_part(u8_t file_id)
{
	const struct partition_entry *part;
	int i;

	 __ASSERT_NO_MSG(g_part_table);

	for (i = 0; i < MAX_PARTITION_COUNT; i++) {
		part = &g_part_table->parts[i];
		if ((part->file_id == file_id) && !partition_is_mirror_part(part)) {
			return part;
		}
	}

	return NULL;
}
const struct partition_entry *partition_get_part_for_temp(u8_t file_id)
{
	const struct partition_entry *part;
	int i;

	 __ASSERT_NO_MSG(g_part_table);

	for (i = 0; i < MAX_PARTITION_COUNT; i++) {
		part = &g_part_table->parts[i];
		if (part->file_id == file_id) {
			return part;
		}
	}

	return NULL;
}

const struct partition_entry *partition_get_temp_part(void)
{
	const struct partition_entry *temp_part;

	temp_part = partition_get_part_for_temp(PARTITION_FILE_ID_OTA_TEMP);
	if (temp_part == NULL || (temp_part->type != PARTITION_TYPE_TEMP)) {
		LOG_ERR("cannot found temp part\n");
		return NULL;
	}

	return temp_part;
}

const struct partition_entry *partition_get_mirror_part(u8_t file_id)
{
	const struct partition_entry *part;
	int i;

	 __ASSERT_NO_MSG(g_part_table);

	for (i = 0; i < MAX_PARTITION_COUNT; i++) {
		part = &g_part_table->parts[i];
		if ((part->file_id == file_id) && partition_is_mirror_part(part)) {
			/* found */
			LOG_INF("found write part %d for file_id 0x%x\n", i, file_id);
			return part;
		}
	}

	return NULL;
}

bool partition_is_used_sector(const struct partition_entry *part)
{
	__ASSERT_NO_MSG(g_part_table);
	__ASSERT_NO_MSG(part);

	return part->flag & PARTITION_FLAG_USED_SECTOR ? true : false;
}

const struct partition_entry *partition_get_stf_part(u8_t stor_id, u8_t file_id)
{
	const struct partition_entry *part;
	int i;

	 __ASSERT_NO_MSG(g_part_table);

	for (i = 0; i < MAX_PARTITION_COUNT; i++) {
		part = &g_part_table->parts[i];
		if (((part->storage_id == stor_id) || (part->storage_id== STORAGE_ID_MAX)) && (part->file_id == file_id)) {
			/* found */
			LOG_INF("found write part %d for file_id 0x%x\n", i, file_id);
			return part;
		}
	}

	return NULL;
}

struct device *partition_get_storage_dev(const struct partition_entry *part)
{
	struct device *dev = NULL;
	 __ASSERT_NO_MSG(g_part_table);

	if (!part) {
		return NULL;
	}
	switch (part->storage_id) {
	case STORAGE_ID_NOR:
		dev = (struct device *) device_get_binding(CONFIG_SPI_FLASH_NAME);
		break;
	case STORAGE_ID_SD:
		dev = (struct device *) device_get_binding(CONFIG_SD_NAME);
		break;
	case STORAGE_ID_NAND:
	case STORAGE_ID_BOOTNAND:
		dev = (struct device *) device_get_binding(CONFIG_SPINAND_FLASH_NAME);
		break;
	case STORAGE_ID_DATA_NOR:
		dev = (struct device *) device_get_binding(CONFIG_SPI_FLASH_1_NAME);
		break;
	default:
		break;
	}
	return dev;
}


int partition_file_mapping(u8_t file_id, u32_t vaddr)
{
	const struct partition_entry *part;
	int err, crc_is_enabled;

	part = partition_get_part(file_id);
	if (part == NULL) {
		LOG_ERR("cannot found file_id %d\n", file_id);
		return -ENOENT;
	}

	crc_is_enabled = part->flag & PARTITION_FLAG_ENABLE_CRC ? 1 : 0;

	err = soc_memctrl_mapping(vaddr, part->file_offset, crc_is_enabled);
	if (err) {
		LOG_ERR("cannot mapping file_id %d\n", file_id);
		return err;
	}

	return 0;
}


static int partition_init(const struct device *dev)
{
	u32_t cksum;

	g_part_table = (const struct partition_table *)soc_boot_get_part_tbl_addr();
	if (g_part_table == NULL ||
	    g_part_table->magic != PARTITION_TABLE_MAGIC) {
		LOG_ERR("partition table (%p) has wrong magic\n", g_part_table);
		goto failed;
	}

	cksum = utils_crc32(0, (const uint8_t *)g_part_table, sizeof(struct partition_table) - 4);
	if (cksum != g_part_table->table_crc) {
		LOG_ERR("partition table (%p) checksum error\n", g_part_table);
		goto failed;
	}

	partition_dump();

	return 0;

failed:
	g_part_table = NULL;
	return 0;
}
SYS_INIT(partition_init, PRE_KERNEL_1, 60);

#if defined(CONFIG_SPINAND_ACTS) || defined(CONFIG_MMC_SDCARD)
#include <drivers/spinand.h>
#include <disk/disk_access.h>
#include <drivers/mmc/sd.h>
#define CONFIG_SPINAND_DEV_NAME "spinand"
#define CONFIG_MMC_SDCARD_DEV_NAME "sd"

static int partition_check(const struct device *dev)
{
	const struct partition_entry *part;
	uint32_t sector_count;
	uint32_t sector_size;
	uint64_t capacity = 0, part_size;
	const struct device *dev_storage;
	int ret;

	part = &g_part_table->parts[g_part_table->part_cnt-1];
	printk("%d off=0x%x, size=0x%x, file_id=%d, stroage_id=%d\n",
				g_part_table->part_cnt , part->offset, part->size,
				part->file_id, part->storage_id);
	part_size = part->offset + part->size;
#ifdef CONFIG_SPINAND_ACTS
	if((part->storage_id == STORAGE_ID_BOOTNAND) || (part->storage_id == STORAGE_ID_NAND)){
		dev_storage = device_get_binding(CONFIG_SPINAND_DEV_NAME);
		ret = spinand_storage_ioctl(dev_storage, DISK_IOCTL_GET_SECTOR_COUNT, (void *)&sector_count);
		ret |= spinand_storage_ioctl(dev_storage, DISK_IOCTL_GET_SECTOR_SIZE, (void *)&sector_size);
		if (!ret) {
			capacity = sector_count;
			capacity *= sector_size;
			printk("part size=0x%llx, nand size=0x%llx\n", part_size, capacity);
			if(part_size > capacity){
				printk("---error---: part size over nand size\n");
				k_panic();
			}
		}else{
			printk("err: nand get capacity fail\n");
			while(1);
		}
	}
#endif

#ifdef 	CONFIG_MMC_SDCARD
	if(part->storage_id == STORAGE_ID_SD){
		dev_storage = device_get_binding(CONFIG_MMC_SDCARD_DEV_NAME);
		ret = sd_card_storage_ioctl(dev_storage, DISK_IOCTL_GET_SECTOR_COUNT, (void *)&sector_count);
		ret |= sd_card_storage_ioctl(dev_storage, DISK_IOCTL_GET_SECTOR_SIZE, (void *)&sector_size);
		if (!ret) {
			capacity = sector_count;
			capacity *= sector_size;
			printk("part size=0x%llx, sdcard size=0x%llx\n", part_size, capacity);
			if(part_size > capacity){
				printk("---error---:part size over sdcard size\n");
				k_panic();
			}
		}else{
			printk("err: sd get capacity fail\n");
			while(1);
		}
	}
#endif

	return 0;
}
SYS_INIT(partition_check, APPLICATION, 92);

#endif
