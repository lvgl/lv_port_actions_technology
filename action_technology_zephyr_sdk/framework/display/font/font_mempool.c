#include <os_common_api.h>
#include <mem_manager.h>
#include <ui_mem.h>
#include <string.h>
#include "bitmap_font_api.h"

#ifdef CONFIG_SIMULATOR
#include <kheap.h>
#define ALLOC_NO_WAIT OS_NO_WAIT
#else
#define ALLOC_NO_WAIT K_NO_WAIT
#endif

typedef struct _cache_mem_info
{
	void* ptr;
	size_t size;
	struct _cache_mem_info* next;
}cache_mem_info_t;


#define CMAP_CACHE_SIZE							1024*8

#ifdef CONFIG_BITMAP_FONT_USE_HIGH_FREQ_CACHE
#ifdef CONFIG_BITMAP_FONT_HIGH_FREQ_CACHE_SIZE
#define BITMAP_FONT_HIGH_FREQ_CACHE_SIZE		CONFIG_BITMAP_FONT_HIGH_FREQ_CACHE_SIZE
#else
#define BITMAP_FONT_HIGH_FREQ_CACHE_SIZE		1500*1024
#endif //CONFIG_BITMAP_FONT_HIGH_FREQ_CACHE_SIZE
#else
#define BITMAP_FONT_HIGH_FREQ_CACHE_SIZE		0
#endif //CONFIG_BITMAP_FONT_USE_HIGH_FREQ_CACHE

#ifdef CONFIG_BITMAP_PER_FONT_CACHE_SIZE

#define BITMAP_FONT_CACHE_SIZE				CONFIG_BITMAP_PER_FONT_CACHE_SIZE
#else
#define BITMAP_FONT_CACHE_SIZE				1024*64
#endif //CONFIG_BITMAP_PER_FONT_CACHE_SIZE

#ifdef CONFIG_BITMAP_FONT_MAX_OPENED_FONT
#define MAX_OPEND_FONT						CONFIG_BITMAP_FONT_MAX_OPENED_FONT
#else
#define MAX_OPEND_FONT						2
#endif //CONFIG_BITMAP_FONT_MAX_OPENED_FONT

#ifdef CONFIG_BITMAP_FONT_SUPPORT_EMOJI
#define MAX_EMOJI_FONT						1
#define MAX_EMOJI_NUM						100
#else
#define MAX_EMOJI_FONT						0
#define MAX_EMOJI_NUM						0
#endif  //CONFIG_BITMAP_FONT_SUPPORT_EMOJI

//#define BITMAP_FONT_PSRAM_SIZE				(MAX_OPEND_FONT+MAX_EMOJI_FONT)*(BITMAP_FONT_CACHE_SIZE+CMAP_CACHE_SIZE)+MAX_EMOJI_NUM*sizeof(emoji_font_entry_t)+BITMAP_FONT_HIGH_FREQ_CACHE_SIZE

#ifdef CONFIG_BITMAP_FONT_CACHE_POOL_SIZE
#define BITMAP_FONT_PSRAM_SIZE				CONFIG_BITMAP_FONT_CACHE_POOL_SIZE
#else
#define BITMAP_FONT_PSRAM_SIZE				360*1024
#endif

#define DEBUG_FONT_GLYPH					0
#define PRINT_FONT_GLYPH_ERR				0


#ifdef CONFIG_FREETYPE_FONT_MAX_FACES
#define FREETYPE_FONT_MAX_FACES				CONFIG_FREETYPE_FONT_MAX_FACES
#else
#define FREETYPE_FONT_MAX_FACES				2
#endif

#ifdef CONFIG_FREETYPE_FONT_MAX_SIZES
#define FREETYPE_FONT_MAX_SIZES				CONFIG_FREETYPE_FONT_MAX_SIZES
#else
#define FREETYPE_FONT_MAX_SIZES				3
#endif

#ifdef CONFIG_FREETYPE_FONT_MAX_SUBCACHE_BYTES
#define FREETYPE_FONT_MAX_SUBCACHE_BYTES				CONFIG_FREETYPE_FONT_MAX_SUBCACHE_BYTES
#else
#define FREETYPE_FONT_MAX_SUBCACHE_BYTES				150*1024
#endif

#if CONFIG_FREETYPE_FONT_SHAPE_CACHE_SIZE > 0
static char __aligned(4) shape_cache_buffer[CONFIG_FREETYPE_FONT_SHAPE_CACHE_SIZE] __in_section_unique(font.bss.cache);
static struct k_heap shape_cache_pool = {
    .heap = {
        .init_mem = shape_cache_buffer,
        .init_bytes = CONFIG_FREETYPE_FONT_SHAPE_CACHE_SIZE,
    },	
};
#endif

static char __aligned(4) bmp_font_info_buffer[12 * 1024] __in_section_unique(font.bss.cache);
static struct k_heap font_info_pool = {
    .heap = {
        .init_mem = bmp_font_info_buffer,
        .init_bytes = 12 * 1024,
    },	
};

#if defined(CONFIG_BITMAP_FONT_CACHE_POOL_SIZE) && CONFIG_BITMAP_FONT_CACHE_POOL_SIZE >= 12 * 1024
static char __aligned(4) bmp_font_cache_buffer[BITMAP_FONT_PSRAM_SIZE - 12 * 1024] __in_section_unique(font.bss.cache);
static struct k_heap font_mem_pool = {
    .heap = {
        .init_mem = bmp_font_cache_buffer,
        .init_bytes = BITMAP_FONT_PSRAM_SIZE - 12 * 1024,
    },
};

#endif


static int font_cache_peak_size = 0;
static int font_cache_total_size = 0;
static int font_cache_used_size = 0;
static int font_cahce_item_num = 0;
static cache_mem_info_t* cache_mem_info;
static int font_cache_inited = 0;

void bitmap_font_cache_init(void)
{
	if(font_cache_inited == 0)
	{
	    font_cache_total_size = BITMAP_FONT_PSRAM_SIZE;
	    font_cache_used_size = 0;

		os_printk("bitmap_font_cache_init\n");

		k_heap_init(&font_info_pool, font_info_pool.heap.init_mem, font_info_pool.heap.init_bytes);
#if defined(CONFIG_BITMAP_FONT_CACHE_POOL_SIZE) && CONFIG_BITMAP_FONT_CACHE_POOL_SIZE >= 10000
	    k_heap_init(&font_mem_pool, font_mem_pool.heap.init_mem, font_mem_pool.heap.init_bytes);
#endif

#if CONFIG_FREETYPE_FONT_SHAPE_CACHE_SIZE > 0
        k_heap_init(&shape_cache_pool, shape_cache_pool.heap.init_mem, shape_cache_pool.heap.init_bytes);
#endif
		font_cache_inited = 1;
	}
}

static void _add_cache_mem_info(void* ptr, uint32_t size)
{
    cache_mem_info_t* item=NULL;

    //item = mem_malloc(sizeof(cache_mem_info_t));

    item = k_heap_alloc(&font_info_pool, sizeof(cache_mem_info_t), ALLOC_NO_WAIT);
    if(item == NULL)
    {
        SYS_LOG_INF("cache mem info malloc failed %d, %d\n", font_cache_used_size, font_cahce_item_num);
        return;
    }

    item->ptr = ptr;
    item->size = size;
    item->next = NULL;
    if(cache_mem_info==NULL)
    {
        cache_mem_info = item;
    }
    else
    {
        cache_mem_info_t* pos = cache_mem_info;
        cache_mem_info_t* prev = cache_mem_info;
        while(pos)
        {
            prev = pos;
            pos = pos->next;
        }
        prev->next = item;
    }

    font_cache_used_size += size;
	font_cahce_item_num++;
	if(font_cache_peak_size < font_cache_used_size)
	{
		font_cache_peak_size = font_cache_used_size;
		//SYS_LOG_INF("font_cache_peak_size %d\n", font_cache_peak_size);
	}

	//SYS_LOG_INF("font_cache_used_size %d, font_cahce_item_num %d\n", font_cache_used_size, font_cahce_item_num);
    SYS_LOG_DBG("testfont cache mem add %d, total %d, peak size %d\n", size, font_cache_used_size, font_cache_peak_size);
}

static void _remove_cache_mem_info(void* ptr)
{
    cache_mem_info_t* item;
    cache_mem_info_t* prev;

    item = cache_mem_info;
    prev = NULL;
    while(item)
    {
        if(item->ptr == ptr)
        {
            break;
        }

        prev = item;
        item = item->next;
    }

    if(item == NULL)
    {
        SYS_LOG_INF("cache mem info not found %p\n", ptr);
        return;
    }

    if(prev)
    {
        prev->next = item->next;
    }

    if(item == cache_mem_info)
    {
        cache_mem_info = item->next;
    }
	font_cahce_item_num--;
    font_cache_used_size -= item->size;
    SYS_LOG_DBG("testfont cache mem remove %d, total %d\n", item->size, font_cache_used_size);
    k_heap_free(&font_info_pool, item);
}

uint32_t _get_cache_mem_info(void* ptr)
{
    cache_mem_info_t* item;

    item = cache_mem_info;
    while(item)
    {
        if(item->ptr == ptr)
        {
            break;
        }

        item = item->next;
    }

	if (item) {
		return item->size;
	}

	return 0;
}

void* bitmap_font_cache_malloc(uint32_t size)
{
	void *ptr;

	if(size % 4 != 0)
	{
		size = (size/4 + 1)*4;
	}
#if defined(CONFIG_BITMAP_FONT_CACHE_POOL_SIZE) && CONFIG_BITMAP_FONT_CACHE_POOL_SIZE >= 10000
    ptr = k_heap_alloc(&font_mem_pool, size, ALLOC_NO_WAIT);
#else
	ptr = ui_mem_gui_alloc(size);
#endif

	if (ptr == NULL) {
		SYS_LOG_ERR("font cache heap alloc failed, size %d, used %d, peak %d\n", size, font_cache_used_size, font_cache_peak_size);
		return ptr;
	}

	_add_cache_mem_info(ptr, size);
	return ptr;
}

void bitmap_font_cache_free(void* ptr)
{
#if defined(CONFIG_BITMAP_FONT_CACHE_POOL_SIZE) && CONFIG_BITMAP_FONT_CACHE_POOL_SIZE >= 10000
	k_heap_free(&font_mem_pool, ptr);
#else
	ui_mem_gui_free(ptr);
#endif
	_remove_cache_mem_info(ptr);
}

uint32_t bitmap_font_cache_get_size(void* ptr)
{
	return _get_cache_mem_info(ptr);
}

uint32_t bitmap_font_get_max_fonts_num(void)
{
	return (MAX_OPEND_FONT+MAX_EMOJI_FONT);
}

//use as default size if not configd for current font size
uint32_t bitmap_font_get_font_cache_size(void)
{
	return BITMAP_FONT_CACHE_SIZE;
}

uint32_t bitmap_font_get_max_emoji_num(void)
{
	return MAX_EMOJI_NUM;
}

uint32_t bitmap_font_get_cmap_cache_size(void)
{
	return CMAP_CACHE_SIZE;
}

void bitmap_font_cache_dump_info(void)
{
    SYS_LOG_INF("bitmap font cache info dump: total used %d, peak size %d, max size %d\n", font_cache_used_size, font_cache_peak_size, font_cache_total_size);
}

int bitmap_font_glyph_debug_is_on(void)
{
#if DEBUG_FONT_GLYPH==1
	return 1;
#else
	return 0;
#endif
}

int bitmap_font_glyph_err_print_is_on(void)
{
#if PRINT_FONT_GLYPH_ERR==1
	return 1;
#else
	return 0;
#endif
}

int bitmap_font_get_high_freq_enabled(void)
{
#ifdef CONFIG_BITMAP_FONT_USE_HIGH_FREQ_CACHE
	return 1;
#else
	return 0;
#endif
}

void bitmap_font_get_decompress_param(int bmp_size, int font_size, int* in_size, int* line_size)
{
	*in_size = bmp_size*3/2;
	*line_size = ((font_size+3)/4)*4*2;    
}


int freetype_font_get_font_fixed_bpp(void)
{
#ifdef CONFIG_FREETYPE_FONT_BITMAP_BPP
	return CONFIG_FREETYPE_FONT_BITMAP_BPP;
#else
	return 2;
#endif
}

int freetype_font_get_max_face_num(void)
{
	return FREETYPE_FONT_MAX_FACES;
}


int freetype_font_get_max_size_num(void)
{
	return FREETYPE_FONT_MAX_SIZES;
}

uint32_t freetype_font_get_font_cache_size(void)
{
#ifdef CONFIG_FREETYPE_PER_FONT_CACHE_SIZE
	return CONFIG_FREETYPE_PER_FONT_CACHE_SIZE;
#else
	return 65536;
#endif
}

int freetype_font_get_max_ftccache_bytes(void)
{
	return FREETYPE_FONT_MAX_SUBCACHE_BYTES;
}

int freetype_font_get_memory_face_enabled(void)
{
#ifndef CONFIG_SIMULATOR
#ifdef CONFIG_FREETYPE_FONT_ENABLE_MEMORY_FACE
	return 1;
#else
	return 0;
#endif
#else
    return 0;
#endif
}


int freetype_font_use_svg_path(void)
{
#ifdef CONFIG_FREETYPE_FONT_ENABLE_SVG_PATH
	return 1;
#else
	return 0;
#endif
}

int freetype_font_get_max_vertices(void)
{
#ifdef CONFIG_FREETYPE_FONT_ENABLE_SVG_PATH
	return 256;
#else
	return 1;
#endif    
}

int freetype_font_enable_subpixel(void)
{
#ifdef ENABLE_SUBPIX
    return 1;
#else
    return 0;
#endif
}


int emoji_font_use_mmap(void)
{
#ifdef CONFIG_EMOJI_FONT_USE_MMAP
	return 1;
#else
	return 0;
#endif
}

#if CONFIG_FREETYPE_FONT_SHAPE_CACHE_SIZE > 0
static int shape_cache_num = 0;
static int shape_cache_peak = 0;
#endif
void* freetype_font_shape_cache_malloc(uint32_t size)
{
#if CONFIG_FREETYPE_FONT_SHAPE_CACHE_SIZE > 0
	void *ptr;
    ptr = k_heap_alloc(&shape_cache_pool, size, ALLOC_NO_WAIT);

	if (ptr == NULL) {
		SYS_LOG_ERR("shape cache heap alloc failed, size %d\n", size);
		return ptr;
	}

    shape_cache_num++;
    if(shape_cache_num > shape_cache_peak)
    {
        shape_cache_peak = shape_cache_num;
    }
    //SYS_LOG_INF("shape cache num %d, peak %d\n", shape_cache_num, shape_cache_peak);
	return ptr;
#else
    return NULL;
#endif
}

void freetype_font_shape_cache_free(void* ptr)
{
#if CONFIG_FREETYPE_FONT_SHAPE_CACHE_SIZE > 0
    k_heap_free(&shape_cache_pool, ptr);
    shape_cache_num--;
    //SYS_LOG_INF("shape cache num %d, peak %d\n", shape_cache_num, shape_cache_peak);
#else
    return;
#endif
}

int freetype_font_get_shape_info_size(void)
{
#if CONFIG_FREETYPE_FONT_SHAPE_CACHE_SIZE > 0
    return CONFIG_FREETYPE_SHAPE_INFO_CACHE_SIZE;
#else
    return 0;
#endif
}
