#ifndef _VIDEO_MEM_H_
#define _VIDEO_MEM_H_

#include <os_common_api.h>

//申请psram内存
void *video_mem_malloc(int size);

//释放psram内存
void video_mem_free(void *ptr);

#endif

