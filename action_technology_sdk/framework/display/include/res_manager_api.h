#ifndef __RES_MANAGER_API_H__
#define __RES_MANAGER_API_H__

#ifndef CONFIG_SIMULATOR
#include <fs/fs.h>
#endif

typedef enum 
{
	RESOURCE_TYPE_GROUP=3,
	RESOURCE_TYPE_PICTURE=4,
	RESOURCE_TYPE_TEXT=2,
	RESOURCE_TYPE_PICREGION=1, //fixme
} resource_type_e;

typedef enum
{
	RES_MEM_POOL_BMP=0,
	RES_MEM_POOL_TXT,
	RES_MEM_POOL_SCENE,
	RES_MEM_POOL_TYPE_MAX,
}res_mem_pool_type_e;

typedef enum
{
	RES_MEM_SIMPLE_INNER=0,
	RES_MEM_SIMPLE_PRELOAD,
	RES_MEM_SIMPLE_COMPACT,
	RES_MEM_SIMPLE_TYPE_MAX,
}res_mem_simple_type_e;

typedef enum
{
	RESOURCE_BITMAP_FORMAT_RGB565,
	RESOURCE_BITMAP_FORMAT_ARGB8565,
	RESOURCE_BITMAP_FORMAT_RGB888,
	RESOURCE_BITMAP_FORMAT_ARGB8888,
	RESOURCE_BITMAP_FORMAT_A8,
	RESOURCE_BITMAP_FORMAT_ARGB6666,
	RESOURCE_BITMAP_FORMAT_RAW,
	RESOURCE_BITMAP_FORMAT_JPEG,
	RESOURCE_BITMAP_FORMAT_ARGB1555,
	RESOURCE_BITMAP_FORMAT_INDEX8,
	RESOURCE_BITMAP_FORMAT_INDEX4,
	RESOURCE_BITMAP_FORMAT_ETC2_EAC,
	RESOURCE_BITMAP_FORMAT_LVGL_INDEX8,
} resource_bitmap_format_e;

typedef struct resource_s
{
    uint32_t type;
    uint32_t id;
    uint32_t offset;
}resource_t;

typedef struct
{		  
	int16_t  x;
	int16_t  y;
	int16_t  width;
	int16_t  height;
	uint32_t background;	 
	uint32_t  transparence;
	uint32_t resource_sum;
	uint32_t child_offset;
	uint32_t scene_id;
	uint16_t direction;	
	uint16_t  visible;
	uint16_t   opaque;	

} resource_scene_t;	

typedef struct
{
	uint32_t type;
	uint32_t sty_id;
	uint32_t offset;
	int16_t x;
	int16_t y;   
	uint16_t width;
	uint16_t height;
	uint32_t backgroud;
	uint32_t opaque;
	uint32_t resource_sum;
	uint32_t child_offset;	
} sty_group_t;

typedef struct
{
	uint32_t type;	
	uint32_t sty_id;	
	uint32_t offset;
	uint32_t id;
	int16_t x;
	int16_t y;
	uint16_t width;
	uint16_t height;
	uint16_t bytes_per_pixel;
	uint16_t format;
	uint32_t bmp_pos;
	uint32_t compress_size;
	uint32_t magic;
} sty_picture_t;

/*!
*\brief
    data structure of string resource
*/
typedef struct
{
	/** resource type*/
	uint32_t type;
	/** hashed sty id*/
	uint32_t sty_id;	
	/** offset to text content*/
	uint32_t offset;	
	/** resource id*/
	uint32_t id;
	/** x coord to parent  */
	int16_t x;
	/** y coord to parent  */
	int16_t y;
	/** text area width  */
	uint16_t width;
	/** text area height  */
	uint16_t height;
	/** font size  */
	uint16_t font_size;
	/** text algin mode */
	uint16_t align;
	/** text color  */
	uint32_t color;
	/** string background color*/
	uint32_t bgcolor;	
} sty_string_t;

typedef struct
{
	uint32_t type;
	uint32_t sty_id;	
	uint32_t offset;	
 	int16_t x;
	int16_t y;
	uint16_t width;
	uint16_t height;
	uint16_t format;	
	uint16_t bytes_per_pixel;
	uint32_t frames;
	uint32_t id_offset;
	uint32_t magic;
}sty_picregion_t;


typedef struct _pic_search_param_s
{
#ifdef CONFIG_SIMULATOR
	void *res_fp;
#else	
	struct fs_file_t res_fp;
#endif
	uint32_t id_start;
	uint32_t id_end;
	uint32_t* pic_offsets;
	uint32_t* compress_size;
}pic_search_param_t;

typedef struct style_s
{
#ifdef CONFIG_SIMULATOR
	void *style_fp;
	void *pic_fp;
	void *text_fp;
#else
	struct fs_file_t style_fp;
	struct fs_file_t pic_fp;
	struct fs_file_t text_fp;
#endif
	uint32_t sum; 
	resource_scene_t* scenes;
	uint8_t* sty_data;
	uint8_t* sty_mem;	
	uint8_t* sty_path;
	uint8_t* str_path;
	uint32_t reference;
	pic_search_param_t* pic_search_param;
	uint32_t pic_search_max_volume;
#ifdef CONFIG_RES_MANAGER_USE_STYLE_MMAP
	uint8_t* pic_res_mmap_addr;
#endif
} resource_info_t;

typedef struct
{
	sty_group_t* sty_data;
	uint8_t reserve[8];
} resource_group_t;

typedef struct
{
	sty_picture_t* sty_data;	
	uint8_t* buffer;
	uint32_t regular_info;
} resource_bitmap_t;

typedef struct
{
	sty_string_t* sty_data;
	uint8_t* buffer;	
	uint8_t reserve[4];
} resource_text_t;

typedef struct
{
	sty_picregion_t* sty_data;
	uint32_t regular_info;
	uint8_t reserve[4];
}resource_picregion_t;

typedef struct _compact_buffer_s
{
	uint32_t scene_id;
	uint32_t free_size;
	uint32_t offset;
	uint8_t* addr;
	struct _compact_buffer_s* next;
}compact_buffer_t;

typedef struct _preload_param
{
	uint32_t preload_type;
	resource_bitmap_t* bitmap;
	uint32_t scene_id;
	void (*callback)(int32_t , void*);
	void* param;
	resource_info_t* res_info;
	struct _preload_param* next;
}preload_param_t;


typedef struct
{
	uint8_t res_path[256];
#ifdef CONFIG_SIMULATOR
	void *pic_fp;
#else
	struct fs_file_t pic_fp;
#endif
	uint32_t inited;
	pic_search_param_t* pic_search_param;
	uint32_t pic_search_max_volume;
#ifdef CONFIG_RES_MANAGER_USE_STYLE_MMAP
	uint8_t* pic_res_mmap_addr;
#endif	
}res_bin_info_t;

typedef struct
{
	/* data */
	uint32_t type;	
	uint32_t sty_id;	
	uint32_t id;
	int16_t x;
	int16_t y;
	uint16_t width;
	uint16_t height;
	uint16_t bytes_per_pixel;
	uint16_t format;
	uint32_t bmp_pos;
	uint32_t compress_size;
	uint32_t magic;	
	uint8_t* buffer;
	res_bin_info_t* res_info;
}style_bitmap_t;

typedef struct
{
	const uint8_t* key;
	int value;
}res_string_item_t;

void res_manager_init(void);
void res_manager_set_screen_size(uint32_t screen_w, uint32_t screen_h);

void res_manager_clear_cache(uint32_t force_clear);

resource_info_t* res_manager_open_resources( const char* style_path, const char* picture_path, const char* text_path );

int32_t res_manager_set_str_file(resource_info_t* info, const char* text_path);

void res_manager_close_resources( resource_info_t* res_info );


resource_scene_t* res_manager_load_scene(resource_info_t* res_info, uint32_t scene_id);


void res_manager_unload_scene(uint32_t scene_id, resource_scene_t* scene);

void* res_manager_get_scene_child(resource_info_t*res_info, resource_scene_t* scene, uint32_t id);


void* res_manager_get_group_child(resource_info_t*res_info, resource_group_t* resgroup, uint32_t id );


void res_manager_release_resource(void* resource);


void res_manager_free_resource_structure(void* resource);


void res_manager_free_bitmap_data(void* data);


void res_manager_free_text_data(void* data);

void* res_manager_preload_from_scene(resource_info_t* res_info, resource_scene_t* scene, uint32_t id);
resource_bitmap_t* res_manager_preload_from_group(resource_info_t* res_info, resource_group_t* group, uint32_t scene_id, uint32_t id);
resource_bitmap_t* res_manager_preload_from_picregion(resource_info_t* res_info, resource_picregion_t* picreg, uint32_t frame);

int res_manager_set_pictures_regular(resource_info_t* info, uint32_t scene_id, uint32_t group_id, uint32_t subgroup_id, uint32_t* id, uint32_t num);

int32_t res_manager_clear_regular_pictures(const uint8_t* sty_path, uint32_t scene_id);

int32_t res_manager_init_compact_buffer(uint32_t scene_id, size_t size);
uint32_t res_manager_get_bitmap_buf_block_unit_size(void);

int32_t res_manager_preload_bitmap(resource_info_t* res_info, resource_bitmap_t* bitmap);
int32_t res_manager_preload_bitmap_compact(uint32_t scene_id, resource_info_t* res_info, resource_bitmap_t* bitmap);
void* res_manager_preload_next_scene_child(resource_info_t* info, resource_scene_t* scene, uint32_t* count, uint32_t* offset);
void* res_manager_preload_next_group_child(resource_info_t * info, resource_group_t* group, int* count, uint32_t* offset, uint32_t scene_id, uint32_t pargroup_id);

void res_manager_preload_finish_check(uint32_t scene_id);

uint32_t res_manager_scene_is_loaded(uint32_t scene_id);
void res_manager_dump_info(void);
resource_bitmap_t* res_manager_load_frame_from_picregion(resource_info_t* info, resource_picregion_t* picreg, uint32_t frame);

int res_manager_load_bitmap_for_decoder(resource_bitmap_t* bitmap, int force);
void res_manager_free_bitmap_for_decoder(void* ptr);
int res_manager_decode_bitmap_to_img_dsc(void* raw, void* decoded);

int res_manager_set_current_string_res(const uint8_t** string_res, uint32_t string_cnt);
uint8_t* res_manager_get_string(uint8_t* key);
uint8_t* res_manager_get_string_from_id(int id);

#ifdef CONFIG_RES_MANAGER_ENABLE_MEM_LEAK_DEBUG
void res_manager_compact_buffer_check(uint32_t scene_id);
#endif
uint32_t res_mem_get_mem_peak(void);
void res_manager_clear_decoder_cache(void);
#endif /*__RES_MANAGER_API_H__*/
