#include "video_mem.h"
#include <sys/sys_heap.h>
#include <kernel.h>
#include <init.h>

#define VIDEO_MEMORY_SIZE  (20*1024)

__in_section_unique(VIDEO_PSRAM_REGION)static char __aligned(4) video_mem_pool_buffer[VIDEO_MEMORY_SIZE];
__in_section_unique(VIDEO_PSRAM_REGION) static struct sys_heap video_mem_heap;
static OS_MUTEX_DEFINE(video_mem_mutex);

void *video_mem_malloc(int size)
{
	void *ptr;

	os_mutex_lock(&video_mem_mutex, OS_FOREVER);
	ptr = sys_heap_alloc(&video_mem_heap, size);
	os_mutex_unlock(&video_mem_mutex);

	return ptr;
}

void video_mem_free(void *ptr)
{
	os_mutex_lock(&video_mem_mutex, OS_FOREVER);
	sys_heap_free(&video_mem_heap, ptr);
	os_mutex_unlock(&video_mem_mutex);
}

static int video_mem_init(const struct device *unused)
{
	sys_heap_init(&video_mem_heap, video_mem_pool_buffer, VIDEO_MEMORY_SIZE);
	return 0;
}

SYS_INIT(video_mem_init, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

