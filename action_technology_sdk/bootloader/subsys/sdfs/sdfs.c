
#include <zephyr/types.h>
#include <strings.h>
#include <stddef.h>
#include <sys/types.h>
#include <device.h>
#include <sdfs.h>
#include <fs/fs.h>
#include <fs/fs_sys.h>
#include <storage/flash_map.h>
#include <partition/partition.h>
#include <linker/linker-defs.h>
#include "sdfs_nand_sd.h"

#ifdef CONFIG_SDFS_NOR_NOT_XIP
#include <drivers/flash.h>
#include <board_cfg.h>
#endif

#define K_SDFS_ADDR		(((unsigned int)__rom_region_start)+((unsigned int)_flash_used))

static struct sd_file  g_sd_file_heap[CONFIG_SD_FILE_MAX];
static bool b_k_sdfs;

#ifdef CONFIG_SDFS_NOR_NOT_XIP
static uint32_t g_k_sdfs_system_addr;
static const struct device *global_nor_dev;
#endif

#define SDFS_INVALID_PART_ID (0xFF)
#define SDFS_INVALID_PART(x) ((x) == SDFS_INVALID_PART_ID)

#define memcpy_flash_data memcpy

#if 0
#define sd_alloc k_malloc
#define sd_free k_free
#else
struct sd_file * sd_alloc(int size)
{
	int i;
	unsigned int key;
	key = irq_lock();
	for(i = 0; i < CONFIG_SD_FILE_MAX; i++){
		if(g_sd_file_heap[i].start == 0){
			g_sd_file_heap[i].start = 1; //use
			break;
		}
	}
	irq_unlock(key);
	if(i == CONFIG_SD_FILE_MAX)
		return NULL;
	else
		return &g_sd_file_heap[i];
}
void sd_free(struct sd_file * sd_file)
{
	unsigned int key;
	key = irq_lock();
	memset(sd_file, 0, sizeof(*sd_file));
	irq_unlock(key);
}
#endif

//#define CONFIG_SD_FS_VADDR_START g_vaddr_start
//static unsigned int g_vaddr_start = 0x0;



static struct sd_dir * sd_find_dir_by_addr(const char *filename, void *buf_size_32, uint32_t adfs_addr)
{
	int num, total, offset;
	struct sd_dir *sd_dir = buf_size_32;

	memcpy_flash_data(buf_size_32, (void *)adfs_addr, sizeof(*sd_dir));

	//printk("sd_dir->fname %s CONFIG_SD_FS_START 0x%x \n",sd_dir->fname,CONFIG_SD_FS_VADDR_START);
	if(memcmp(sd_dir->fname, "sdfs.bin", 8) != 0)
	{
		printk("sdfs.bin invalid, offset=0x%x\n", adfs_addr);
		return NULL;
	}
	total = sd_dir->offset;

	for(offset = adfs_addr + sizeof(*sd_dir), num = 0; num < total; offset += 32)
	{
		memcpy_flash_data(buf_size_32, (void *)offset, 32);
		//printk("%d,file=%s, size=0x%x\n", num, sd_dir->fname, sd_dir->size);
		if(strncasecmp(filename, sd_dir->fname, 12) == 0)
		{
			return sd_dir;
		}
		num++;
	}

	return NULL;
}

#ifdef CONFIG_SDFS_NOR_NOT_XIP
static struct sd_dir *sd_find_dir_by_part(const char *filename, void *buf_size_32, uint8_t part)
{
	int num, total, offset, total_file_size;
	struct sd_dir *sd_dir = buf_size_32, *ret_sd_dir = NULL;
	const struct partition_entry *part_entry;
	int ret;
	uint8_t *sd_dir_buf_ptr = NULL;

	part_entry = partition_get_stf_part(STORAGE_ID_NOR, part + PARTITION_FILE_ID_SDFS_PART_BASE);
	if (!part_entry)
		return NULL;

	ret = flash_read(global_nor_dev, part_entry->offset, buf_size_32, sizeof(struct sd_dir));
	if (ret < 0) {
		printk("nor read offset:0x%x size:%d error:%d\n",
				part_entry->offset, sizeof(struct sd_dir), ret);
		return NULL;
	}

	if (memcmp(sd_dir->fname, "sdfs.bin", 8) != 0) {
		printk("sdfs.bin invalid, offset=0x%x\n", part_entry->offset);
		return NULL;
	}

	total = sd_dir->offset;
	total_file_size = (total + 1) * sizeof(struct sd_dir);

	sd_dir_buf_ptr = k_malloc(total_file_size);
	if (!sd_dir_buf_ptr) {
		printk("failed to malloc size:%d\n", total_file_size);
		return NULL;
	}

	ret = flash_read(global_nor_dev, part_entry->offset, sd_dir_buf_ptr, total_file_size);
	if (ret < 0) {
		printk("nor read offset:0x%x size:%d error:%d\n",
				part_entry->offset + sizeof(*sd_dir), total_file_size, ret);
		goto out;
	}

	for (offset = (uint32_t)sd_dir_buf_ptr + sizeof(*sd_dir), num = 0; num < total; offset += 32) {
		memcpy_flash_data(buf_size_32, (void *)offset, 32);
		//printk("%d,file=%s, size=0x%x\n", num, sd_dir->fname, sd_dir->size);
		if (strncasecmp(filename, sd_dir->fname, 12) == 0) {
			/* add partition offset */
			sd_dir->offset += part_entry->offset;
			ret_sd_dir = sd_dir;
			break;
		}
		num++;
	}

out:
	k_free(sd_dir_buf_ptr);
	return ret_sd_dir;
}
#endif

static struct sd_dir * sd_find_dir(const char *filename, void *buf_size_32, uint8_t part)
{
	struct sd_dir *sd_d = NULL;

	/* find file from ksdfs which padding by kernel firstly */
	if (b_k_sdfs && SDFS_INVALID_PART(part)){
		sd_d = sd_find_dir_by_addr(filename, buf_size_32, K_SDFS_ADDR);
		if(sd_d) {
			 sd_d->offset += K_SDFS_ADDR;
			return sd_d;
		}
	}

#ifdef CONFIG_SDFS_NOR_NOT_XIP
	if (g_k_sdfs_system_addr && SDFS_INVALID_PART(part)) {
		sd_d = sd_find_dir_by_addr(filename, buf_size_32, g_k_sdfs_system_addr);
		if (sd_d) {
			sd_d->offset += g_k_sdfs_system_addr;
			return sd_d;
		}
	}

	if (!SDFS_INVALID_PART(part)) {
		sd_d = sd_find_dir_by_part(filename, buf_size_32, part);
		return sd_d;
	}
#else
	sd_d = sd_find_dir_by_addr(filename, buf_size_32, CONFIG_SD_FS_VADDR_START);
	if(sd_d) {
		 sd_d->offset += CONFIG_SD_FS_VADDR_START;
		return sd_d;
	}
#endif

	return sd_d;
}
const char* const sd_volume_strs[] = {
	_SDFS_VOL_STRS
};

static const char *sd_get_part_type(const char *filename, uint8_t *stor_id, uint8_t *part)
{
	const char *pc, *ret_ptr = NULL;
	int i;
	*stor_id = STORAGE_ID_NOR;
	*part = SDFS_INVALID_PART_ID;

	if(filename[0] != '/')
		return filename;

	filename += 1;
	pc = strchr(filename, ':');
	if(pc == NULL)  // /*|| pc[2] != '/'*/
		return NULL;

	for(i = 0; i < STORAGE_ID_MAX; i++){
		if(!strncmp(filename, sd_volume_strs[i], strlen(sd_volume_strs[i])))
			break;
	}
	if(i == STORAGE_ID_MAX)
		return NULL;

	*stor_id = i;

	/* /[NOR|NAND|SD]:[A:Z]/s */
	if ((pc[1] >= 'A') && (pc[1] <= 'Z') /*&& (pc[2] == '/')*/) {
		*part = pc[1] - 'A';
		ret_ptr = pc + 3;
	} else {
		*part = SDFS_INVALID_PART_ID;
		ret_ptr = pc + 1;
	}

	return ret_ptr;
}

struct sd_file * sd_fopen (const char *filename)
{
	struct sd_dir *sd_dir;
	uint8_t buf_size_32[32];
	struct sd_file *sd_file;
	uint8_t stor_id, part;
	const char *fname;

	fname = sd_get_part_type(filename, &stor_id, &part);
	if(fname == NULL){
		printk("sdfs file %s invalid\n", filename);
		return NULL;
	}
	printk("sdfs:stor_id=%d, p=%d\n",stor_id, part);
	if(stor_id == STORAGE_ID_NOR)
		sd_dir = sd_find_dir(fname, (void *)buf_size_32, part);
	else
		sd_dir = nand_sd_find_dir(stor_id, part, fname, (void *)buf_size_32);
	if(sd_dir == NULL)
	{
		printk("%s no this file %s\n", __FUNCTION__, filename);
		return NULL;
	}

	sd_file = sd_alloc(sizeof(*sd_file));
	if(sd_file == NULL)
	{
		printk("%s malloc(%d) failed\n", __FUNCTION__, (int)sizeof(*sd_file));
		return NULL;
	}

	sd_file->start = sd_dir->offset;
	sd_file->size = sd_dir->size;
	sd_file->readptr = sd_file->start;
	sd_file->file_id = part;
	sd_file->storage_id = stor_id;
	return sd_file;
}

void sd_fclose(struct sd_file *sd_file)
{
	sd_free(sd_file);
}

int sd_fread(struct sd_file *sd_file, void *buffer, int len)
{
	unsigned int size_in_512, read_size;

	if ((sd_file->readptr - sd_file->start + len) > sd_file->size)
	{
		len = sd_file->size - (sd_file->readptr - sd_file->start);
	}
	if(len <= 0)
		return 0;

	read_size = len;

	if(sd_file->storage_id != STORAGE_ID_NOR)
		return nand_sd_sd_fread(sd_file->storage_id, sd_file, buffer, read_size);

	size_in_512 = 512 - (sd_file->readptr % 512);
	size_in_512 = size_in_512 > len ? len : size_in_512;
	if(size_in_512 > 0)
	{
#ifdef CONFIG_SDFS_NOR_NOT_XIP
		if (SDFS_INVALID_PART(sd_file->file_id)) {
			memcpy_flash_data(buffer, (void *)sd_file->readptr, size_in_512);
		} else {
			if (flash_read(global_nor_dev, (uint32_t)sd_file->readptr, buffer, size_in_512) < 0) {
				printk("failed to read offset:0x%x size:%d\n",
						(uint32_t)sd_file->readptr, size_in_512);
				return 0;
			}
		}
#else
		memcpy_flash_data(buffer, (void *)sd_file->readptr, size_in_512);
#endif

		buffer = (uint8_t *)buffer + size_in_512;
		sd_file->readptr += size_in_512;
		len -= size_in_512;
	}

	for(; len > 0; buffer = (uint8_t *)buffer + size_in_512, sd_file->readptr += size_in_512, len -= size_in_512)
	{
		size_in_512 = len > 512 ? 512 : len;
#ifdef CONFIG_SDFS_NOR_NOT_XIP
		if (SDFS_INVALID_PART(sd_file->file_id)) {
			memcpy_flash_data(buffer, (void *)sd_file->readptr, size_in_512);
		} else {
			if (flash_read(global_nor_dev, (uint32_t)sd_file->readptr, buffer, size_in_512) < 0) {
				printk("failed to read offset:0x%x size:%d\n",
						(uint32_t)sd_file->readptr, size_in_512);
				return 0;
			}
		}
#else
		memcpy_flash_data(buffer, (void *)sd_file->readptr, size_in_512);
#endif
	}

	return read_size;
}

int sd_ftell(struct sd_file *sd_file)
{
    return (sd_file->readptr - sd_file->start);
}

int sd_fseek(struct sd_file *sd_file, int offset, unsigned char whence)
{
	if (whence == FS_SEEK_SET)
	{
		if (offset > sd_file->size)
			return -1;

		sd_file->readptr = sd_file->start + offset;
		return 0;
	}

	if (whence == FS_SEEK_CUR)
	{
		if(sd_file->readptr + offset < sd_file->start
			|| sd_file->readptr + offset > sd_file->start + sd_file->size)
		{
			return -1;
		}

		sd_file->readptr += offset;
		return 0;
	}

	if (whence == FS_SEEK_END)
	{
		if(offset > 0 || offset + sd_file->size < 0)
			return -1;

		sd_file->readptr = sd_file->start + sd_file->size + offset;
		return 0;
	}

	return -EINVAL;
}

int sd_fsize(const char *filename)
{
	struct sd_file *fd = sd_fopen(filename);
	int file_size;

	if (!fd) {
		return -EINVAL;
	}

	file_size = fd->size;

	sd_fclose(fd);

	return file_size;
}

int sd_fmap(const char *filename, void** addr, int* len)
{
	struct sd_file *fd = sd_fopen(filename);
	if (!fd) {
		return -EINVAL;
	}
	if(fd->storage_id != STORAGE_ID_NOR)
		return -EINVAL;

	if (addr)
		*addr = (void *)fd->start;

	if (len)
		*len = fd->size;

	sd_fclose(fd);

	return 0;
}

static int sd_fs_init(const struct device *dev)
{
	struct sd_dir sd_dir;
	printk("sdfs: init mapping to 0x%x, koff=0x%x\n", CONFIG_SD_FS_VADDR_START, K_SDFS_ADDR);
	memcpy_flash_data(&sd_dir, (void *)K_SDFS_ADDR, sizeof(sd_dir));
	if(memcmp(sd_dir.fname, "sdfs.bin", 8) == 0){
		printk("ksdfs.bin ok\n");
		b_k_sdfs = true;
	}else{
		b_k_sdfs = false;
	}

#ifdef CONFIG_SDFS_NOR_NOT_XIP
	const struct partition_entry *part_sdfs = partition_get_part(PARTITION_FILE_ID_SDFS);
	const struct partition_entry *part_system = partition_get_part(PARTITION_FILE_ID_SYSTEM);

	if ((part_sdfs->offset < part_system->offset)
		|| ((part_sdfs->offset - part_system->offset) >= CONFIG_SDFS_NOR_NOT_XIP_MAX_COPY_OFFSET)
		|| ((part_sdfs->offset - part_system->offset + part_sdfs->size) > CONFIG_SDFS_NOR_NOT_XIP_MAX_COPY_OFFSET)) {
		printk("sdfs partition offset(0x%x) invalid", part_sdfs->offset);
		return -1;
	}

	g_k_sdfs_system_addr = CONFIG_FLASH_BASE_ADDRESS + part_sdfs->offset - part_system->offset;

	global_nor_dev = device_get_binding(CONFIG_SDFS_NOR_DEV_NAME);
	if (!global_nor_dev) {
		printk("failed to get nor device:%s\n", CONFIG_SDFS_NOR_DEV_NAME);
		return -1;
	}
#else
	int err = partition_file_mapping(PARTITION_FILE_ID_SDFS, CONFIG_SD_FS_VADDR_START);
	if (err) {
		printk("sdfs: cannot mapping part file_id %d", PARTITION_FILE_ID_SDFS);
		return -1;
	}
#endif

	return 0;
}

SYS_INIT(sd_fs_init, PRE_KERNEL_1, 80);


#ifdef CONFIG_FILE_SYSTEM

static int sdfs_open(struct fs_file_t *zfp, const char *file_name, fs_mode_t flags)
{
	struct sd_file * sdf;
	if (zfp == NULL || file_name == NULL) {
		return -EINVAL;
	}

	if (zfp->filep) {
		/* file has been opened */
		return -EEXIST;
	}
	sdf = sd_fopen(file_name);
	if(sdf == NULL)
		return -EINVAL;

	zfp->filep = (void *)sdf;
	return 0;
}

static int sdfs_close(struct fs_file_t *zfp)
{
	if (zfp == NULL) {
		return -EINVAL;
	}
	if (zfp->filep) {
		sd_fclose((struct sd_file *)zfp->filep);
		zfp->filep = NULL;
	} else {
		return -EIO;
	}
	return 0;
}
static ssize_t sdfs_read(struct fs_file_t *zfp, void *ptr, size_t size)
{
	if (zfp == NULL || ptr == NULL) {
		return -EINVAL;
	}
	return sd_fread((struct sd_file *)zfp->filep, ptr, size);
}


static int sdfs_seek(struct fs_file_t *zfp, off_t offset, int whence)
{
	if (!zfp) {
		return -EINVAL;
	}
	return sd_fseek((struct sd_file *)zfp->filep, offset, whence);
}

static off_t sdfs_tell(struct fs_file_t *zfp)
{
	if (!zfp) {
		return -EINVAL;
	}
	return sd_ftell((struct sd_file *)zfp->filep);
}
static int sdfs_stat(struct fs_mount_t *mountp,
		      const char *path, struct fs_dirent *entry)
{
	int ret;
	if (mountp == NULL || path == NULL || entry == NULL) {
		return -EINVAL;
	}
	ret = sd_fsize(path);
	if(ret < 0) {
		printk("%s not exist\n", path);
		return -EINVAL;
	}
	entry->type = FS_DIR_ENTRY_FILE;
	entry->size = ret;
	return 0;
}

static int sdfs_statvfs(struct fs_mount_t *mountp,
			 const char *path, struct fs_statvfs *stat)
{
	if (mountp == NULL || path == NULL || stat == NULL) {
		return -EINVAL;
	}
	memset(stat, 0, sizeof(struct fs_statvfs));
	stat->f_bsize = 512;
	return 0;
}

static int sdfs_mount(struct fs_mount_t *mountp)
{
	uint8_t stor_id, part;
	const char *fname;
	const struct partition_entry *parti;

	if (mountp == NULL) {
		return -EINVAL;
	}
	fname = sd_get_part_type(mountp->mnt_point, &stor_id, &part);
	if(fname == NULL){
		printk("sdfs mount fail,%s\n", mountp->mnt_point);
		return -EINVAL;
	}
	parti = partition_get_stf_part(stor_id, part+PARTITION_FILE_ID_SDFS_PART_BASE);
	if(parti == NULL){
		printk("sdfs mount get parit fail,%s\n", mountp->mnt_point);
		return -EINVAL;
	}

	return 0;
}

static int sdfs_unmount(struct fs_mount_t *mountp)
{
	if (mountp == NULL) {
		return -EINVAL;
	}
	return 0;
}


/* File system interface */
const struct fs_file_system_t sdfs_fs = {
	.open = sdfs_open,
	.close = sdfs_close,
	.read = sdfs_read,
	.lseek = sdfs_seek,
	.tell = sdfs_tell,
	.mount = sdfs_mount,
	.unmount = sdfs_unmount,
	.stat = sdfs_stat,
	.statvfs = sdfs_statvfs,

};

static int fs_sdfs_init(const struct device *dev)
{
	int ret;
	ret = fs_register(FS_SDFS, &sdfs_fs);
	printk("sdfs fs_register=%d\n", ret);
	return 0;
}


SYS_INIT(fs_sdfs_init, POST_KERNEL, 99);

#endif

