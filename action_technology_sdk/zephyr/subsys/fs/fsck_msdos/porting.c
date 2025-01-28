#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <fcntl.h>
#include <kernel.h>
#include <disk/disk_access.h>
#include <watchdog_hal.h>
#include "porting.h"

#define SECTOR_SZ	512

#ifdef CONFIG_UI_MEMORY_MANAGER
extern void ui_mem_res_free(void * ptr);
extern void * ui_mem_res_alloc(size_t size);
#define fsck_malloc		ui_mem_res_alloc
#define fsck_free		ui_mem_res_free
#else
#define fsck_malloc		mem_malloc
#define fsck_free		mem_free
#endif

int rdonly;		/* device is opened read only (supersedes above) */
int skipclean;		/* skip check if fs clean */

int ask(int def, const char* info)
{
	def = !rdonly;
	pinfo("%s? %s\n", info, def ? "yes" : "no");
	return def;
}

void* ext_malloc(int size)
{
	void* ptr = fsck_malloc(size);
	pinfo("malloc(%d)=%p\n", size, ptr);
	return ptr;
}

void* ext_calloc(int nmemb, int size)
{
	void* ptr = ext_malloc(nmemb * size);

	if (ptr != NULL) {
		(void)memset(ptr, 0, nmemb * size);
	}

	return ptr;
}

void ext_free(void* ptr)
{
	pinfo("free(%p)\n", ptr);
	fsck_free(ptr);
}

int ext_open(const char* fname)
{
	const char *disk;

	if (*fname == '/') {
		disk = fname + 1;
	} else {
		disk = fname;
	}

	return (int)disk;
}

int ext_close(int fd)
{
	const char *disk = (const char*)fd;

	return disk_access_ioctl(disk, DISK_IOCTL_CTRL_SYNC, NULL);
}

int ext_read(int fd, int off, void* ptr, int size)
{
	const char *disk = (const char*)fd;
	int ret;

	ret = disk_access_read(disk, ptr, off/SECTOR_SZ, size/SECTOR_SZ);
	if (ret) {
		return 0;
	}

#ifdef CONFIG_WATCHDOG
	watchdog_clear();
#endif

	return size;
}

int ext_write(int fd, int off, void* ptr, int size)
{
	const char *disk = (const char*)fd;
	int ret;

	ret = disk_access_write(disk, ptr, off/SECTOR_SZ, size/SECTOR_SZ);
	if (ret) {
		return 0;
	}

	return size;
}
