#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <os_common_api.h>
#include <res_manager_api.h>
#include <lvgl/lvgl_res_loader.h>
#ifdef CONFIG_UI_MANAGER
#include <ui_manager.h>
#endif
#include "../res_manager/res_mempool.h"

#define RES_PRELOAD_STACKSIZE	1536

typedef enum
{
	PRELOAD_TYPE_IMMEDIATE,
	PRELOAD_TYPE_NORMAL,
	PRELOAD_TYPE_NORMAL_COMPACT,
	PRELOAD_TYPE_BEGIN_CALLBACK,
	PRELOAD_TYPE_END_CALLBACK,
}preload_type_e;

typedef struct _preload_default_t
{
	uint32_t scene_id;
	uint32_t* resource_id;
	uint32_t resource_num;
	void (*callback)(int32_t status, void* param);
	char* style_path;
	char* picture_path;
	char* text_path;
	struct _preload_default_t* next;
}preload_default_t;

#ifdef CONFIG_RES_MANAGER_ENABLE_MEM_LEAK_DEBUG
#define MAX_BITMAP_CHECKABLE	160
#define MAX_STRING_CHECKABLE	64

typedef struct _bitmap_mem_debug_item
{
	uint32_t scene_id;
	uint32_t res_id[MAX_BITMAP_CHECKABLE];
	uint32_t ref[MAX_BITMAP_CHECKABLE];
	uint32_t addr[MAX_BITMAP_CHECKABLE];
	struct _bitmap_mem_debug_item* next;
}bitmap_mem_debug_t;

typedef struct _string_mem_debug_item
{
	uint32_t scene_id;
	uint32_t res_id[MAX_STRING_CHECKABLE];
	uint32_t ref[MAX_STRING_CHECKABLE];
	uint32_t addr[MAX_STRING_CHECKABLE];
	struct _string_mem_debug_item* next;
}string_mem_debug_t;


static bool bitmap_mem_debug_enabled = false;
static uint32_t default_scene_id = 0;
static bitmap_mem_debug_t* bitmap_mem_debug_list = NULL;
static string_mem_debug_t* string_mem_debug_list = NULL;
static void _bitmap_mem_debug_enable(bool enable);

#endif


static os_sem preload_sem;
static os_mutex preload_mutex;
static uint32_t preload_running = 1;
static preload_param_t* param_list = NULL;
static preload_param_t* sync_param_list = NULL;

static lvgl_res_scene_t current_scene;
static lvgl_res_group_t current_group;
static lvgl_res_group_t current_subgrp;

static resource_info_t** res_info;
static int32_t res_manager_inited = 0;

//static int32_t current_preload_count = 0;
static preload_default_t* preload_default_list = NULL;

static int screen_width;
static int screen_height;
static int max_resource_sets;
static uint32_t max_compact_block_size;

static size_t res_mem_align = 4;

#ifndef CONFIG_RES_MANAGER_SKIP_PRELOAD
static char __aligned(ARCH_STACK_PTR_ALIGN) __in_section_unique(ram.noinit.stack)
res_preload_stack[CONFIG_LVGL_RES_PRELOAD_STACKSIZE];

static void _res_preload_thread(void *parama1, void *parama2, void *parama3);
#endif

void lvgl_res_scene_preload_default_cb_for_view(int32_t status, void *view_id);

void lvgl_res_cache_clear(uint32_t force_clear)
{
	res_manager_clear_cache(force_clear);
}

int lvgl_res_loader_init(uint32_t screen_w, uint32_t screen_h)
{
	res_manager_init();
	res_manager_set_screen_size(screen_w, screen_h);

	/* create a thread to preload*/
#ifndef CONFIG_RES_MANAGER_SKIP_PRELOAD
	os_sem_init(&preload_sem, 0, 1);
	os_mutex_init(&preload_mutex);

	int tid = os_thread_create(res_preload_stack, RES_PRELOAD_STACKSIZE,
		_res_preload_thread,
		NULL, NULL, NULL,
		CONFIG_LVGL_RES_PRELOAD_PRIORITY, 0, 0);

	os_thread_name_set((os_tid_t)tid, "res_preload");
#endif
	memset(&current_scene, 0, sizeof(lvgl_res_scene_t));
	memset(&current_group, 0, sizeof(lvgl_res_group_t));
	memset(&current_subgrp, 0, sizeof(lvgl_res_group_t));

	max_resource_sets = res_mem_get_max_resource_sets();
	res_info = (resource_info_t**)res_array_alloc(RES_MEM_SIMPLE_PRELOAD, sizeof(resource_info_t*)*max_resource_sets);
	if(!res_info)
	{
		SYS_LOG_ERR("res_info malloc faild\n");
		return -1;
	}
	memset(res_info, 0, sizeof(resource_info_t*)*max_resource_sets);

	screen_width = screen_w;
	screen_height = screen_h;

	max_compact_block_size = res_mem_get_max_compact_block_size();

	res_manager_inited = 1;

	res_mem_align = res_mem_get_align();

#ifdef CONFIG_RES_MANAGER_ENABLE_MEM_LEAK_DEBUG
	_bitmap_mem_debug_enable(true);
#endif
	return 0;
}

void lvgl_res_loader_deinit(void)
{
	uint32_t i;

	if(res_manager_inited == 0)
	{
		return;
	}

	for(i=0;i<max_resource_sets;i++)
	{
		if(res_info[i] != NULL)
		{
			res_manager_close_resources(res_info[i]);
		}
		res_info[i] = NULL;
	}
	res_array_free(res_info);
}

#ifdef CONFIG_RES_MANAGER_ENABLE_MEM_LEAK_DEBUG
static void _bitmap_mem_debug_enable(bool enable)
{
	bitmap_mem_debug_enabled = enable;
}

static void _bitmap_mem_debug_set_default_scene_id(uint32_t scene_id)
{
	bitmap_mem_debug_t* item = NULL;
	bitmap_mem_debug_t* prev = NULL;
	string_mem_debug_t* sitem = NULL;
	string_mem_debug_t* sprev = NULL;
	int i;
	
	if(!bitmap_mem_debug_enabled)	
	{
		return;
	}

	//SYS_LOG_INF("RES LEAK INF: load scene 0x%x\n", scene_id);
	default_scene_id = scene_id;
	item = bitmap_mem_debug_list;
	while(item)
	{
		if(item->scene_id == scene_id)
		{
			SYS_LOG_INF("RES LEAK WARNING: scene 0x%x maybe not unloaded\n", item->scene_id);
			SYS_LOG_INF("unloaded bitmap ids: \n");
			for(i=0;i<MAX_BITMAP_CHECKABLE;i++)
			{
				if(item->res_id[i] == 0)
				{
					break;
				}
				SYS_LOG_INF("res id %d, ref %d\n", item->res_id[i], item->ref[i]);
			}
			return;
		}
		prev = item;
		item = item->next;
	}

	if(prev == NULL)
	{
		bitmap_mem_debug_list = res_mem_alloc(RES_MEM_POOL_BMP, sizeof(bitmap_mem_debug_t));
		if(!bitmap_mem_debug_list)
		{
			SYS_LOG_INF("no memory to debug res mem leak for scene 0x%x\n", scene_id);
		}
		memset(bitmap_mem_debug_list, 0, sizeof(bitmap_mem_debug_t));
		bitmap_mem_debug_list->scene_id = scene_id;	
	}
	else
	{
		item = res_mem_alloc(RES_MEM_POOL_BMP, sizeof(bitmap_mem_debug_t));
		if(!item)
		{
			SYS_LOG_INF("no memory to debug res mem leak for scene 0x%x\n", scene_id);
		}
		memset(item, 0, sizeof(bitmap_mem_debug_t));
		item->scene_id = scene_id;	
		prev->next = item;
	}

	//string resource debug list init
	sitem = string_mem_debug_list;
	while(sitem)
	{
		if(sitem->scene_id == scene_id)
		{
			SYS_LOG_INF("RES LEAK WARNING: scene 0x%x maybe not unloaded\n", sitem->scene_id);
			SYS_LOG_INF("unloaded string ids: \n");
			for(i=0;i<MAX_STRING_CHECKABLE;i++)
			{
				if(sitem->res_id[i] == 0)
				{
					break;
				}
				SYS_LOG_INF("res id %d, ref %d\n", sitem->res_id[i], sitem->ref[i]);
			}
			return;
		}
		sprev = sitem;
		sitem = sitem->next;
	}

	if(sprev == NULL)
	{
		string_mem_debug_list = res_mem_alloc(RES_MEM_POOL_BMP, sizeof(string_mem_debug_t));
		if(!string_mem_debug_list)
		{
			SYS_LOG_INF("no memory to debug res mem leak for scene 0x%x\n", scene_id);
		}
		memset(string_mem_debug_list, 0, sizeof(string_mem_debug_t));
		string_mem_debug_list->scene_id = scene_id;	
	}
	else
	{
		sitem = res_mem_alloc(RES_MEM_POOL_BMP, sizeof(string_mem_debug_t));
		if(!sitem)
		{
			SYS_LOG_INF("no memory to debug res mem leak for scene 0x%x\n", scene_id);
		}
		memset(sitem, 0, sizeof(string_mem_debug_t));
		sitem->scene_id = scene_id;	
		sprev->next = sitem;
	}	

}

static void _bitmap_mem_debug_add_bitmap_to_scene(uint32_t scene_id, uint32_t id, uint32_t addr)
{
	int i;
	bitmap_mem_debug_t* item;

	if(!bitmap_mem_debug_enabled)	
	{
		return;
	}

	item = bitmap_mem_debug_list;
	while(item)
	{
		if(item->scene_id == scene_id || item->scene_id == default_scene_id)
		{
			break;
		}
		item = item->next;
	}
	
	if(!item)
	{
		SYS_LOG_INF("no mem debug info registered for scene 0x%x\n", scene_id);
		return;
	}

	for(i=0;i<MAX_BITMAP_CHECKABLE;i++)
	{
		if(item->res_id[i] == id)
		{
			item->ref[i]++;
			if(item->addr[i] != addr && item->ref[i] > 1)
			{
				SYS_LOG_INF("RES LEAK WARNING: unmatched addr for same res id %d, previous 0x%x, current 0x%x\n", 
						id, item->addr[i], addr);
			}

			//update addr
			if(item->ref[i] == 1)
			{
				item->addr[i] = addr;
			}
			
			return;			
		}		
		else if(item->res_id[i] == 0)
		{
			break;
		}
	}

	if(i>=MAX_BITMAP_CHECKABLE)
	{
		SYS_LOG_INF("too many pics to check\n");
		return;
	}

	item->res_id[i] = id;
	item->addr[i] = addr;
	item->ref[i] = 1;
}

static void _bitmap_mem_debug_add_string_to_scene(uint32_t scene_id, uint32_t id, uint32_t addr)
{
	int i;
	string_mem_debug_t* item;

	if(!bitmap_mem_debug_enabled)	
	{
		return;
	}

	item = string_mem_debug_list;
	while(item)
	{
		if(item->scene_id == scene_id || item->scene_id == default_scene_id)
		{
			break;
		}
		item = item->next;
	}
	
	if(!item)
	{
		SYS_LOG_INF("no mem debug info registered for scene 0x%x\n", scene_id);
		return;
	}

	for(i=0;i<MAX_STRING_CHECKABLE;i++)
	{
		if(item->res_id[i] == id)
		{
			item->ref[i]++;
			if(item->addr[i] != addr && item->ref[i] > 1)
			{
				SYS_LOG_INF("RES LEAK WARNING: unmatched addr for same string id %d, previous 0x%x, current 0x%x\n", 
						id, item->addr[i], addr);
			}

			//update addr
			if(item->ref[i] == 1)
			{
				item->addr[i] = addr;
			}
			
			return;			
		}		
		else if(item->res_id[i] == 0)
		{
			break;
		}
	}

	if(i>=MAX_STRING_CHECKABLE)
	{
		SYS_LOG_INF("too many strings to check\n");
		return;
	}

	item->res_id[i] = id;
	item->addr[i] = addr;
	item->ref[i] = 1;
}

static void _bitmap_mem_debug_remove_bitmap_from_scene(uint32_t addr)
{
	int i;
	bitmap_mem_debug_t* item;
	int not_found = 1;
	
	if(!bitmap_mem_debug_enabled)
	{
		return;
	}

	item = bitmap_mem_debug_list;

	while(item)
	{
		for(i=0;i < MAX_BITMAP_CHECKABLE;i++)
		{
			if(item->res_id[i] == 0)
			{
				break;
			}
			
			if(item->addr[i] == addr)
			{
				not_found = 0;
				if(item->ref[i] > 0)
				{
					item->ref[i]--;
				}
				break;
			}
		}

		item = item->next;
	}

	if(not_found == 1)
	{
		SYS_LOG_INF("RES LEAK WARNING: pic addr 0x%x not found when unload\n", addr);
	}
}

static void _bitmap_mem_debug_remove_string_from_scene(uint32_t addr)
{
	int i;
	string_mem_debug_t* item;
	int not_found = 1;
	
	if(!bitmap_mem_debug_enabled)
	{
		return;
	}

	item = string_mem_debug_list;

	while(item)
	{
		for(i=0;i < MAX_STRING_CHECKABLE;i++)
		{
			if(item->res_id[i] == 0)
			{
				break;
			}
			
			if(item->addr[i] == addr)
			{
				not_found = 0;
				if(item->ref[i] > 0)
				{
					item->ref[i]--;
				}
				break;
			}
		}

		item = item->next;
	}

	if(not_found == 1)
	{
		SYS_LOG_INF("RES LEAK WARNING: string addr 0x%x not found when unload\n", addr);
	}
}

static void _bitmap_mem_debug_bitmap_id_check(uint32_t scene_id, bool unload_compact_buffer)
{
	int i;
	bitmap_mem_debug_t* item;
	bitmap_mem_debug_t* prev;
	string_mem_debug_t* sitem;
	string_mem_debug_t* sprev;
	
	if(!bitmap_mem_debug_enabled)
	{
		return;
	}

	//SYS_LOG_INF("RES LEAK INF: unload scene 0x%x, unload compact %d\n", scene_id, unload_compact_buffer);

	//check and release bitmap debug item
	item = bitmap_mem_debug_list;
	prev = NULL;
	while(item)
	{
		if(item->scene_id == scene_id)
		{
			break;
		}
		prev = item;
		item = item->next;
	}

	if(!item)
	{
		SYS_LOG_INF("RES LEAK WARNING: scene to be unload not found 0x%x\n", scene_id);
		return;
	}

	for(i=0;i<MAX_BITMAP_CHECKABLE;i++)
	{
		if(item->res_id[i]!= 0 && item->ref[i] > 0)
		{
			SYS_LOG_INF("RES LEAK WARNING: bitmap maybe leaked, scene 0x%x, res id %d, ref %d\n", item->scene_id, item->res_id[i], item->ref[i]);
		}
	}

	if(prev == NULL)
	{
		bitmap_mem_debug_list = item->next;
	}
	else
	{
		prev->next = item->next;
	}
	res_mem_free(RES_MEM_POOL_BMP, item);

	if(unload_compact_buffer)
	{
		return;
	}

	//check and release string debug item
	sitem = string_mem_debug_list;
	sprev = NULL;
	while(sitem)
	{
		if(sitem->scene_id == scene_id)
		{
			break;
		}
		sprev = sitem;
		sitem = sitem->next;
	}

	if(!sitem)
	{
		SYS_LOG_INF("RES LEAK WARNING: scene to be unload not found 0x%x\n", scene_id);
		return;
	}

	for(i=0;i<MAX_STRING_CHECKABLE;i++)
	{
		if(sitem->res_id[i]!= 0 && sitem->ref[i] > 0)
		{
			SYS_LOG_INF("RES LEAK WARNING: string maybe leaked, res id %d, ref %d\n", sitem->res_id[i], sitem->ref[i]);
		}
	}

	if(sprev == NULL)
	{
		string_mem_debug_list = sitem->next;
	}
	else
	{
		sprev->next = sitem->next;
	}
	res_mem_free(RES_MEM_POOL_BMP, sitem);	
}

static void _bitmap_mem_debug_compact_buffer_check(uint32_t scene_id)
{
	res_manager_compact_buffer_check(scene_id);
}
#endif

void _res_file_close(resource_info_t* info, uint32_t force_close)
{
	uint32_t i;

	if(info == NULL)
	{
		SYS_LOG_ERR("null resource info when unload");
		return;
	}

	for(i=0;i<max_resource_sets;i++)
	{
		if(res_info[i] == info)
		{
			if(res_info[i]->reference >= 1)
			{
				res_info[i]->reference--;
			}

			if(res_info[i]->reference == 0 && force_close)
			{
				res_info[i] = NULL;
				break;
			}
			else
			{
				return;
			}
		}
	}

	if(i >= max_resource_sets)
	{
		SYS_LOG_ERR("resource info not found when unload");
		return;
	}

	res_manager_close_resources(info);
}

resource_info_t* _res_file_open(const char* style_path, const char* picture_path, const char* text_path, uint32_t force_ref, uint32_t in_preload)
{
	resource_info_t* info;
	uint32_t i;
	int32_t empty_slot = -1;
	int32_t expired_slot = -1;

	for(i=0;i<max_resource_sets;i++)
	{
		if(res_info[i] != NULL)
		{
			if(strcmp(style_path, res_info[i]->sty_path)==0)
			{
				if(force_ref)
				{
					res_info[i]->reference++;
				}

				if(in_preload == 0 && strcmp(text_path, res_info[i]->str_path) != 0)
				{
					SYS_LOG_INF("testres: change str file %s to %s\n", res_info[i]->str_path, text_path);
					res_manager_set_str_file(res_info[i], text_path);
				}
				return res_info[i];
			}
			else if(res_info[i]->reference == 0)
			{
				expired_slot = i;
			}
		}
		else if(empty_slot == -1)
		{
			empty_slot = i;
		}
	}

	if(empty_slot == -1 && expired_slot == -1)
	{
		SYS_LOG_ERR("too many resources sets opened");
		for(i=0;i<max_resource_sets;i++)
		{
			SYS_LOG_ERR("res set %d: %s\n", i, res_info[i]->sty_path);
		}		
		return NULL;
	}

	info = res_manager_open_resources(style_path, picture_path, text_path);
	if(info == NULL)
	{
		SYS_LOG_ERR("open resources failed");
		return NULL;
	}

	if(force_ref)
	{
		info->reference = 1;
	}

	if(empty_slot != -1)
	{
		res_info[empty_slot] = info;
	}
	else
	{
		_res_file_close(res_info[expired_slot], 1);
		res_info[expired_slot] = info;
	}

	return info;
}

//caller to ensure no scene is loaded when close files
int lvgl_res_close_resources(const char* style_path, const char* picture_path, const char* text_path)
{
	int slot = -1;
	int i;

	if(style_path == NULL)
	{
		SYS_LOG_ERR("null style path to be closed\n");
		return -1;
	}

	for(i=0;i<max_resource_sets;i++)
	{
		if(res_info[i] != NULL)
		{
			if(strcmp(style_path, res_info[i]->sty_path)==0)
			{
				slot = i;
				break;
			}
		}
	}

	if(slot >= 0)
	{
		//force close, dont check, but warn.
		if(res_info[slot]->reference > 0)
		{
			SYS_LOG_INF("WARNING: closing referred resources\n");
		}

		res_manager_close_resources(res_info[slot]);
		res_info[slot] = NULL;

        preload_default_t* preload = preload_default_list;
        preload_default_t* prev = NULL;
        while(preload)
        {
            if((strcmp(preload->style_path, style_path)==0) &&
                (strcmp(preload->picture_path, picture_path)==0))
            {
                break;
            }

            prev=preload;
            preload = preload->next;
        }

        if(preload == NULL)
        {
            //not in preload default list
            return 0;
        }

        if(preload == preload_default_list)
        {
            preload_default_list = preload->next;
        }
        else
        {
            prev->next = preload->next;
        }

        res_mem_free(RES_MEM_POOL_BMP,preload);
        preload = NULL;
		
		return 0;
	}
	else
	{
		SYS_LOG_ERR("resource file to be closed not found\n");
		return -1;
	}
}


int lvgl_res_load_scene(uint32_t scene_id, lvgl_res_scene_t* scene, const char* style_path, const char* picture_path, const char* text_path)
{
	resource_scene_t* res_scene;
	if(scene == NULL)
	{
		SYS_LOG_ERR("null scene param\n");
		return -1;
	}

	os_strace_u32(SYS_TRACE_ID_RES_SCENE_LOAD, (uint32_t)scene_id);
	scene->res_info = _res_file_open(style_path, picture_path, text_path, 1, 0);
	if(scene->res_info == NULL)
	{
		SYS_LOG_ERR("resource info error\n");
		return -1;
	}

	res_scene = res_manager_load_scene(scene->res_info, scene_id);
	if(res_scene == NULL)
	{
		return -1;
	}

	scene->id = scene_id;
	scene->scene_data = res_scene;
	scene->background = lv_color_hex(res_scene->background);
	scene->x = res_scene->x;
	scene->y = res_scene->y;
	scene->width = res_scene->width;
	scene->height = res_scene->height;
	scene->transparence = res_scene->transparence;
	os_strace_end_call_u32(SYS_TRACE_ID_RES_SCENE_LOAD, (uint32_t)scene_id);

#ifdef CONFIG_RES_MANAGER_ENABLE_MEM_LEAK_DEBUG
	_bitmap_mem_debug_set_default_scene_id(scene_id);
	_bitmap_mem_debug_compact_buffer_check(scene_id);
#endif
	return 0;
}

int32_t lvgl_res_set_pictures_regular(uint32_t scene_id, uint32_t group_id, uint32_t subgroup_id, uint32_t* id, uint32_t num,
												const uint8_t* style_path, const char* picture_path, const char* text_path)
{
	resource_info_t* info;
	int32_t ret;

	info = _res_file_open(style_path, picture_path, text_path, 0, 0);
	if(info == NULL)
	{
		SYS_LOG_ERR("open resource info error\n");
		return -1;
	}

	ret = res_manager_set_pictures_regular(info, scene_id, group_id, subgroup_id, id, num);
	if(ret < 0)
	{
		SYS_LOG_ERR("set regular resource failed\n");
		return -1;
	}

	return ret;
}

int32_t lvgl_res_clear_regular_pictures(uint32_t scene_id, const uint8_t* style_path)
{
	int32_t ret;

	ret = res_manager_clear_regular_pictures(style_path, scene_id);
	if(ret < 0)
	{
		SYS_LOG_ERR("clear resource regular info failed\n");
		return -1;
	}
	return 0;
}

int lvgl_res_load_group_from_scene(lvgl_res_scene_t* scene, uint32_t id, lvgl_res_group_t* group)
{
	resource_group_t* res_group;

	if(scene == NULL || group == NULL)
	{
		SYS_LOG_ERR("null scene %p or null group %p\n", scene, group);
		return -1;
	}

	res_group = (resource_group_t*) res_manager_get_scene_child(scene->res_info, scene->scene_data, id);
	if(res_group == NULL)
	{
		return -1;
	}

	group->id = id;
	group->group_data = res_group;
	group->x = res_group->sty_data->x;
	group->y = res_group->sty_data->y;
	group->width = res_group->sty_data->width;
	group->height = res_group->sty_data->height;
	group->res_info = scene->res_info;
	return 0;
}

int lvgl_res_load_group_from_group(lvgl_res_group_t* group, uint32_t id, lvgl_res_group_t* subgroup)
{
	resource_group_t* res_group;

	if(group == NULL || subgroup == NULL)
	{
		SYS_LOG_ERR("null scene %p or null group %p\n", group, subgroup);
		return -1;
	}

	res_group = res_manager_get_group_child(group->res_info, group->group_data, id);
	if(res_group == NULL)
	{
		return -1;
	}

	subgroup->id = id;
	subgroup->group_data = res_group;
	subgroup->x = res_group->sty_data->x;
	subgroup->y = res_group->sty_data->y;
	subgroup->width = res_group->sty_data->width;
	subgroup->height = res_group->sty_data->height;
	subgroup->res_info = group->res_info;

	return 0;
}

static int _cvt_bmp2dsc(lv_image_dsc_t *dsc, resource_bitmap_t *bmp)
{
	if (bmp->sty_data->width <= 0 || bmp->sty_data->height <= 0)
		return -EINVAL;

	switch (bmp->sty_data->format) {
	case RESOURCE_BITMAP_FORMAT_ARGB8888:
		dsc->header.cf = LV_COLOR_FORMAT_ARGB8888;
		break;
	case RESOURCE_BITMAP_FORMAT_RGB565:
		dsc->header.cf = LV_COLOR_FORMAT_RGB565;
		break;
	case RESOURCE_BITMAP_FORMAT_ARGB8565:
		dsc->header.cf = LV_COLOR_FORMAT_ARGB8565;
		break;

	case RESOURCE_BITMAP_FORMAT_A8:
		dsc->header.cf = LV_COLOR_FORMAT_A8;
		break;
	case RESOURCE_BITMAP_FORMAT_LVGL_INDEX8:
		dsc->header.cf = LV_COLOR_FORMAT_I8;
		break;
	case RESOURCE_BITMAP_FORMAT_INDEX4:
		dsc->header.cf = LV_COLOR_FORMAT_I4;
		break;
	case RESOURCE_BITMAP_FORMAT_RAW:
		dsc->header.cf = LV_COLOR_FORMAT_RAW;
		break;
	case RESOURCE_BITMAP_FORMAT_JPEG:
#if LV_COLOR_DEPTH >= 24
		dsc->header.cf = LV_COLOR_FORMAT_RGB888;
#else
		dsc->header.cf = LV_COLOR_FORMAT_RGB565;
#endif
		break;
	case RESOURCE_BITMAP_FORMAT_ETC2_EAC:
		dsc->header.cf = LV_COLOR_FORMAT_ETC2_EAC;
		break;
	case RESOURCE_BITMAP_FORMAT_RGB888:
	case RESOURCE_BITMAP_FORMAT_ARGB6666:
	case RESOURCE_BITMAP_FORMAT_ARGB1555:
	case RESOURCE_BITMAP_FORMAT_INDEX8:
	default:
		return -EINVAL;
	}

	dsc->header.magic = LV_IMAGE_HEADER_MAGIC;
	dsc->header.flags = 0;
	dsc->header.w = bmp->sty_data->width;
	dsc->header.h = bmp->sty_data->height;
	dsc->header.stride = (dsc->header.w * lv_color_format_get_bpp(dsc->header.cf) + 7) / 8;
	dsc->data = bmp->buffer;

	switch (dsc->header.cf) {
	case LV_COLOR_FORMAT_RAW:
		//what if compress size is not consistent when upgarded?
		dsc->data_size = bmp->sty_data->compress_size;
		break;
	case LV_COLOR_FORMAT_ETC2_EAC:
		dsc->header.stride = ((dsc->header.w + 0x3) & ~0x3);
		dsc->data_size = dsc->header.stride * ((dsc->header.h + 0x3) & ~0x3);
		break;
	case LV_COLOR_FORMAT_I4:
		dsc->data_size = dsc->header.stride * dsc->header.h + 64;
		break;
	case LV_COLOR_FORMAT_I8:
		dsc->data_size = dsc->header.stride * dsc->header.h + 1024;
		break;
	default:
		dsc->data_size = dsc->header.stride * dsc->header.h;
		break;
	}

	return 0;
}

int lvgl_res_load_pictures_from_scene(lvgl_res_scene_t* scene, const uint32_t* id, lv_image_dsc_t* img, lv_point_t* pt, uint32_t num)
{
	resource_bitmap_t* bitmap;
	uint32_t i;

	if(scene == NULL || id == NULL)
	{
		SYS_LOG_ERR("null param %p, %p, %p", scene, id, img);
		return -1;
	}

	os_strace_u32(SYS_TRACE_ID_RES_PICS_LOAD, (uint32_t)scene->scene_data);
	if(img != NULL)
	{
		for(i=0;i<num;i++)
		{
			img[i].data = NULL;
		}
	}	

	for(i=0;i<num;i++)
	{
		bitmap = (resource_bitmap_t*)res_manager_get_scene_child(scene->res_info, scene->scene_data, id[i]);
		if(bitmap == NULL)
		{
			if(img != NULL)
			{
				lvgl_res_unload_pictures(img, num);
			}
			return -1;
		}

#ifdef CONFIG_RES_MANAGER_ENABLE_MEM_LEAK_DEBUG
		_bitmap_mem_debug_add_bitmap_to_scene(scene->id, bitmap->sty_data->id, (uint32_t)bitmap->buffer);
#endif
		
		if(img != NULL)
		{
			_cvt_bmp2dsc(&img[i], bitmap);
		}

		if(pt != NULL)
		{
			pt[i].x = bitmap->sty_data->x;
			pt[i].y = bitmap->sty_data->y;
		}
		res_manager_free_resource_structure(bitmap);
	}
	os_strace_end_call_u32(SYS_TRACE_ID_RES_PICS_LOAD, (uint32_t)scene->scene_data);
	return 0;
}

int lvgl_res_load_strings_from_scene(lvgl_res_scene_t* scene, const uint32_t* id, lvgl_res_string_t* str, uint32_t num)
{
	resource_text_t* text;
	uint32_t i;

	if(scene == NULL || id == NULL || str == NULL)
	{
		SYS_LOG_ERR("null param %p, %p, %p", scene, id, str);
		return -1;
	}
	os_strace_u32(SYS_TRACE_ID_RES_STRS_LOAD, (uint32_t)scene->scene_data);

	for(i=0;i<num;i++)
	{
		str[i].txt = NULL;
	}

	for(i=0;i<num;i++)
	{
		text = (resource_text_t*)res_manager_get_scene_child(scene->res_info, scene->scene_data, id[i]);
		if(text == NULL)
		{
			lvgl_res_unload_strings(str, i);
			return -1;
		}

#ifdef CONFIG_RES_MANAGER_ENABLE_MEM_LEAK_DEBUG
		_bitmap_mem_debug_add_string_to_scene(scene->id, text->sty_data->id, (uint32_t)text->buffer);
#endif

		str[i].txt = text->buffer;
		str[i].color = lv_color_hex(text->sty_data->color);
		str[i].bgcolor = lv_color_hex(text->sty_data->color);
		str[i].x = text->sty_data->x;
		str[i].y = text->sty_data->y;
		str[i].width = text->sty_data->width;
		str[i].height = text->sty_data->height;
		uint16_t align = text->sty_data->align;
		align = align & 0x3;
		switch(align)
		{
		case 0:
			str[i].align = LV_TEXT_ALIGN_LEFT;
			break;
		case 2:
			str[i].align = LV_TEXT_ALIGN_CENTER;
			break;
		case 1:
			str[i].align = LV_TEXT_ALIGN_RIGHT;
			break;
		default:
			str[i].align = LV_TEXT_ALIGN_AUTO;
			break;
		}
		res_manager_free_resource_structure(text);
	}
	os_strace_end_call_u32(SYS_TRACE_ID_RES_STRS_LOAD, (uint32_t)scene->scene_data);
	return 0;
}

int lvgl_res_load_pictures_from_group(lvgl_res_group_t* group, const uint32_t* id, lv_image_dsc_t* img, lv_point_t* pt, uint32_t num)
{
	resource_bitmap_t* bitmap = NULL;
	uint32_t i;

	if(group == NULL || id == NULL )
	{
		SYS_LOG_ERR("null param %p, %p, %p", group, id, img);
		return -1;
	}

	os_strace_u32(SYS_TRACE_ID_RES_PICS_LOAD, (uint32_t)group->group_data);
	if(img != NULL)
	{
		for(i=0;i<num;i++)
		{
			img[i].data = NULL;
		}
	}

	for(i=0;i<num;i++)
	{
		bitmap = (resource_bitmap_t*)res_manager_get_group_child(group->res_info, group->group_data, id[i]);
		if(bitmap == NULL)
		{
			if(img != NULL)
			{
				lvgl_res_unload_pictures(img, num);
			}
			return -1;
		}

#ifdef CONFIG_RES_MANAGER_ENABLE_MEM_LEAK_DEBUG		
		_bitmap_mem_debug_add_bitmap_to_scene(0, bitmap->sty_data->id, (uint32_t)bitmap->buffer);
#endif

		if(img != NULL)
		{
			_cvt_bmp2dsc(&img[i], bitmap);
		}

		if(pt != NULL)
		{
			pt[i].x = bitmap->sty_data->x;
			pt[i].y = bitmap->sty_data->y;
		}

		res_manager_free_resource_structure(bitmap);
		bitmap = NULL;
	}
	os_strace_end_call_u32(SYS_TRACE_ID_RES_PICS_LOAD, (uint32_t)group->group_data);
	return 0;
}

int lvgl_res_load_strings_from_group(lvgl_res_group_t* group, const uint32_t* id, lvgl_res_string_t* str, uint32_t num)
{
	resource_text_t* text;
	uint32_t i;

	if(group == NULL || id== NULL || str == NULL)
	{
		SYS_LOG_ERR("null param %p, %p, %p", group, id, str);
		return -1;
	}


	os_strace_u32(SYS_TRACE_ID_RES_STRS_LOAD, (uint32_t)group->group_data);
	for(i=0;i<num;i++)
	{
		str[i].txt = NULL;
	}

	for(i=0;i<num;i++)
	{
		text = (resource_text_t*)res_manager_get_group_child(group->res_info, group->group_data, id[i]);
		if(text == NULL)
		{
			lvgl_res_unload_strings(str, i);
			return -1;
		}

#ifdef CONFIG_RES_MANAGER_ENABLE_MEM_LEAK_DEBUG
		_bitmap_mem_debug_add_string_to_scene(0, text->sty_data->id, (uint32_t)text->buffer);
#endif

		str[i].txt = text->buffer;
		str[i].color = lv_color_hex(text->sty_data->color);
		str[i].bgcolor = lv_color_hex(text->sty_data->bgcolor);
		str[i].x = text->sty_data->x;
		str[i].y = text->sty_data->y;
		str[i].width = text->sty_data->width;
		str[i].height = text->sty_data->height;
		uint16_t align = text->sty_data->align;
		align = align & 0x3;
		switch(align)
		{
		case 0:
			str[i].align = LV_TEXT_ALIGN_LEFT;
			break;
		case 2:
			str[i].align = LV_TEXT_ALIGN_CENTER;
			break;
		case 1:
			str[i].align = LV_TEXT_ALIGN_RIGHT;
			break;
		default:
			str[i].align = LV_TEXT_ALIGN_AUTO;
			break;
		}
		res_manager_free_resource_structure(text);
	}

	os_strace_end_call_u32(SYS_TRACE_ID_RES_STRS_LOAD, (uint32_t)group->group_data);
	return 0;
}

void lvgl_res_unload_scene_compact(uint32_t scene_id)
{
	os_strace_string(SYS_TRACE_ID_RES_UNLOAD, "scene_compact");

	res_manager_unload_scene(scene_id, NULL);

#ifdef CONFIG_RES_MANAGER_ENABLE_MEM_LEAK_DEBUG
	_bitmap_mem_debug_bitmap_id_check(scene_id, true);
#endif	

	os_strace_end_call_u32(SYS_TRACE_ID_RES_UNLOAD, scene_id);
}

void lvgl_res_unload_scene(lvgl_res_scene_t * scene)
{
	if(scene && scene->scene_data)
	{
		os_strace_string(SYS_TRACE_ID_RES_UNLOAD, "scene");

//		res_manager_unload_scene(0, scene->scene_data);
		if(scene->res_info != NULL)
		{
			_res_file_close(scene->res_info, 0);
		}

		os_strace_end_call_u32(SYS_TRACE_ID_RES_UNLOAD, scene->id);

#ifdef CONFIG_RES_MANAGER_ENABLE_MEM_LEAK_DEBUG
		_bitmap_mem_debug_bitmap_id_check(scene->id, false);
#endif	

		scene->id = 0;
		scene->scene_data = NULL;
	}
}

void lvgl_res_unload_group(lvgl_res_group_t* group)
{
	if(group && group->group_data)
	{
		os_strace_string(SYS_TRACE_ID_RES_UNLOAD, "group");

		res_manager_release_resource(group->group_data);
		group->id = 0;
		group->group_data = NULL;

		os_strace_end_call_u32(SYS_TRACE_ID_RES_UNLOAD, 1);
	}
}

void lvgl_res_unload_picregion(lvgl_res_picregion_t* picreg)
{
	if(picreg && picreg->picreg_data)
	{
		os_strace_string(SYS_TRACE_ID_RES_UNLOAD, "picregion");

		res_manager_release_resource(picreg->picreg_data);
		picreg->picreg_data = NULL;

		os_strace_end_call_u32(SYS_TRACE_ID_RES_UNLOAD, 1);
	}
}

void lvgl_res_unload_pictures(lv_image_dsc_t* img, uint32_t num)
{
	uint32_t i;

	os_strace_string(SYS_TRACE_ID_RES_UNLOAD, "pictures");

	for(i=0;i<num;i++)
	{
		if(img[i].data != NULL)
		{
#ifdef CONFIG_RES_MANAGER_ENABLE_MEM_LEAK_DEBUG
			_bitmap_mem_debug_remove_bitmap_from_scene((uint32_t)img[i].data);
#endif
			res_manager_free_bitmap_data((void *)img[i].data);
			img[i].data = NULL;
		}
	}

	os_strace_end_call_u32(SYS_TRACE_ID_RES_UNLOAD, num);
}

void lvgl_res_unload_strings(lvgl_res_string_t* str, uint32_t num)
{
	uint32_t i;

	os_strace_string(SYS_TRACE_ID_RES_UNLOAD, "strings");

	for(i=0;i<num;i++)
	{
		if(str[i].txt != NULL)
		{
#ifdef CONFIG_RES_MANAGER_ENABLE_MEM_LEAK_DEBUG
			_bitmap_mem_debug_remove_string_from_scene((uint32_t)str[i].txt);
#endif
		
			res_manager_free_text_data(str[i].txt);
			str[i].txt = NULL;
		}
	}

	os_strace_end_call_u32(SYS_TRACE_ID_RES_UNLOAD, num);
}

int lvgl_res_load_picregion_from_group(lvgl_res_group_t* group, const uint32_t id, lvgl_res_picregion_t* res_picreg)
{
	resource_picregion_t* picreg;

	if(group == NULL)
	{
		SYS_LOG_ERR("null param %p", group);
		return -1;
	}

	picreg = res_manager_get_group_child(group->res_info, group->group_data, id);
	if(picreg == NULL)
	{
		SYS_LOG_ERR("load pic region faild");
		return -1;
	}

	res_picreg->x = picreg->sty_data->x;
	res_picreg->y = picreg->sty_data->y;
	res_picreg->width = picreg->sty_data->width;
	res_picreg->height = picreg->sty_data->height;
	res_picreg->frames = picreg->sty_data->frames;
	res_picreg->picreg_data = picreg;
	res_picreg->res_info = group->res_info;
	return 0;
}

int lvgl_res_load_picregion_from_scene(lvgl_res_scene_t* scene, const uint32_t id, lvgl_res_picregion_t* res_picreg)
{
	resource_picregion_t* picreg;

	if(scene == NULL)
	{
		SYS_LOG_ERR("null param %p", scene);
		return -1;
	}

	picreg = res_manager_get_scene_child(scene->res_info, scene->scene_data, id);
	if(picreg == NULL)
	{
		SYS_LOG_ERR("load pic region faild");
		return -1;
	}

	res_picreg->x = picreg->sty_data->x;
	res_picreg->y = picreg->sty_data->y;
	res_picreg->width = picreg->sty_data->width;
	res_picreg->height = picreg->sty_data->height;
	res_picreg->frames = (uint32_t)picreg->sty_data->frames;
	res_picreg->picreg_data = picreg;
	res_picreg->res_info = scene->res_info;

	return 0;
}

int lvgl_res_load_pictures_from_picregion(lvgl_res_picregion_t* picreg, uint32_t start, uint32_t end, lv_image_dsc_t* img)
{
	uint32_t i;
	resource_bitmap_t* bitmap;

	if(picreg == NULL || img == NULL ||
		start >= picreg->frames ||
		start > end)
	{
		SYS_LOG_ERR("invalid param %p, %p, %d, %d", picreg, img, start, end);
		return -1;
	}

	os_strace_u32(SYS_TRACE_ID_RES_PICS_LOAD, (uint32_t)picreg->picreg_data);
	for(i=0; i<=end - start; i++)
	{
		img[i].data = NULL;
	}

	if(end >= picreg->frames)
	{
		end = picreg->frames-1;
	}

	for(i=start; i<=end; i++)
	{
		bitmap = res_manager_load_frame_from_picregion(picreg->res_info, picreg->picreg_data, i);
		if (bitmap == NULL)
		{
			lvgl_res_unload_pictures(img, i - start + 1);
			return -1;
		}

#ifdef CONFIG_RES_MANAGER_ENABLE_MEM_LEAK_DEBUG
		_bitmap_mem_debug_add_bitmap_to_scene(0, bitmap->sty_data->id, (uint32_t)bitmap->buffer);
#endif

		if (_cvt_bmp2dsc(&img[i-start], bitmap))
		{
			lvgl_res_unload_pictures(img, i - start + 1);
			return -1;
		}

		res_manager_free_resource_structure(bitmap);
	}

	os_strace_end_call_u32(SYS_TRACE_ID_RES_PICS_LOAD, (uint32_t)picreg->picreg_data);
	return 0;
}


int32_t _check_res_path(uint32_t group_id, uint32_t subgroup_id)
{
	int32_t ret;

	if(current_group.id != group_id && current_group.id != 0)
	{
		lvgl_res_unload_group(&current_subgrp);
		memset(&current_subgrp, 0, sizeof(lvgl_res_group_t));
		lvgl_res_unload_group(&current_group);
		memset(&current_group, 0, sizeof(lvgl_res_group_t));
	}

	if(current_subgrp.id != subgroup_id && current_subgrp.id != 0)
	{
		lvgl_res_unload_group(&current_subgrp);
		memset(&current_subgrp, 0, sizeof(lvgl_res_group_t));
	}

	if(current_group.id != group_id && group_id != 0 )
	{
		ret = lvgl_res_load_group_from_scene(&current_scene, group_id, &current_group);
		if(ret < 0)
		{
			memset(&current_group, 0, sizeof(lvgl_res_group_t));
			return -1;
		}
	}

	if(current_subgrp.id != subgroup_id && subgroup_id != 0)
	{
		ret = lvgl_res_load_group_from_group(&current_group, subgroup_id, &current_subgrp);
		if(ret < 0)
		{
			memset(&current_subgrp, 0, sizeof(lvgl_res_group_t));
			return -1;
		}
	}

	return 0;
}

int lvgl_res_load_pictures(uint32_t group_id, uint32_t subgroup_id, uint32_t* id, lv_image_dsc_t* img, lv_point_t* pt, uint32_t num)
{
	int32_t ret;

	if(current_scene.id == 0 || current_scene.scene_data == NULL)
	{
		SYS_LOG_ERR("invalid scene 0x%x, %p", current_scene.id, current_scene.scene_data);
		return -1;
	}

	ret = _check_res_path(group_id, subgroup_id);
	if(ret < 0)
	{
		return -1;
	}

	if(current_subgrp.id != 0)
	{
		ret = lvgl_res_load_pictures_from_group(&current_subgrp, id, img, pt, num);
		if(ret < 0)
		{
			return -1;
		}
	}
	else if(current_group.id != 0)
	{
		ret = lvgl_res_load_pictures_from_group(&current_group, id, img, pt, num);
		if(ret < 0)
		{
			return -1;
		}
	}
	else
	{
		ret = lvgl_res_load_pictures_from_scene(&current_scene, id, img, pt, num);
		if(ret < 0)
		{
			return -1;
		}
	}
	return 0;
}

int lvgl_res_load_strings(uint32_t group_id, uint32_t subgroup_id, uint32_t* id, lvgl_res_string_t* str, uint32_t num)
{
	int32_t ret;

	if(current_scene.id == 0 || current_scene.scene_data == NULL)
	{
		SYS_LOG_ERR("invalid scene 0x%x, %p", current_scene.id, current_scene.scene_data);
		return -1;
	}

	ret = _check_res_path(group_id, subgroup_id);
	if(ret < 0)
	{
		return -1;
	}

	if(current_subgrp.id != 0)
	{
		ret = lvgl_res_load_strings_from_group(&current_subgrp, id, str, num);
		if(ret < 0)
		{
			return -1;
		}
	}
	else if(current_group.id != 0)
	{
		ret = lvgl_res_load_strings_from_group(&current_group, id, str, num);
		if(ret < 0)
		{
			return -1;
		}
	}
	else
	{
		ret = lvgl_res_load_strings_from_scene(&current_scene, id, str, num);
		if(ret < 0)
		{
			return -1;
		}
	}
	return 0;

}

int lvgl_res_load_picregion(uint32_t group_id, uint32_t subgroup_id, uint32_t picreg_id, lvgl_res_picregion_t* res_picreg)
{
	int32_t ret;

	if(current_scene.id == 0 || current_scene.scene_data == NULL)
	{
		SYS_LOG_ERR("invalid scene 0x%x, %p", current_scene.id, current_scene.scene_data);
		return -1;
	}

	ret = _check_res_path(group_id, subgroup_id);
	if(ret < 0)
	{
		return -1;
	}

	if(current_subgrp.id != 0)
	{
		ret = lvgl_res_load_picregion_from_group(&current_subgrp, picreg_id, res_picreg);
		if(ret < 0)
		{
			return -1;
		}
	}
	else if(current_group.id != 0)
	{
		ret = lvgl_res_load_picregion_from_group(&current_group, picreg_id, res_picreg);
		if(ret < 0)
		{
			return -1;
		}
	}
	else
	{
		ret = lvgl_res_load_picregion_from_scene(&current_scene, picreg_id, res_picreg);
		if(ret < 0)
		{
			return -1;
		}
	}
	return 0;

}

static preload_param_t* _check_item_in_list(preload_param_t* sublist, resource_bitmap_t* bitmap)
{
	preload_param_t* item;

	if(sublist == NULL)
	{
		SYS_LOG_ERR("shouldnt happen");
		return NULL;
	}
	else
	{
		item = sublist;
		if(item->bitmap && item->bitmap->sty_data->id == bitmap->sty_data->id)
		{
			return NULL;
		}
		
		while(item->next != NULL)
		{
			if(item->bitmap && item->bitmap->sty_data->id == bitmap->sty_data->id)
			{
				return NULL;
			}
			item = item->next;
		}
		
		if(item->bitmap && item->bitmap->sty_data->id == bitmap->sty_data->id)
		{
			return NULL;
		}	
		//found tail		
		return item;
	}
}

static void _add_item_to_list(preload_param_t** sublist, preload_param_t* param)
{
	preload_param_t* item;

	os_strace_u32x4(SYS_TRACE_ID_RES_PRELOAD_ADD, (uint32_t)param->scene_id, (uint32_t)*sublist, (uint32_t)param, (uint32_t)param->next);

	if(*sublist == NULL)
	{
		*sublist = param;
	}
	else
	{
		item = *sublist;
		while(1)
		{
			if(item->next == NULL)
			{
				item->next = param;
				break;
			}
			item = item->next;
		}
	}
	os_strace_end_call_u32(SYS_TRACE_ID_RES_PRELOAD_ADD, (uint32_t)param_list);
}

static void _add_item_to_preload_list(preload_param_t* param)
{
	preload_param_t* item;
	os_mutex_lock(&preload_mutex, OS_FOREVER);
	os_strace_u32x4(SYS_TRACE_ID_RES_PRELOAD_ADD, (uint32_t)param->scene_id, (uint32_t)param_list, (uint32_t)param, (uint32_t)param->next);
	if(param_list == NULL)
	{
		param_list = param;
		os_sem_give(&preload_sem);
	}
	else
	{
		item = param_list;
		while(1)
		{
			if(item->next == NULL)
			{
				item->next = param;
				break;
			}
			item = item->next;
		}
	}

	os_strace_end_call_u32(SYS_TRACE_ID_RES_PRELOAD_ADD, (uint32_t)param_list);
	os_mutex_unlock(&preload_mutex);

}

static void _add_item_to_loading_list(preload_param_t* param)
{
	preload_param_t* item;
	if(sync_param_list == NULL)
	{
		sync_param_list = param;
	}
	else
	{
		item = sync_param_list;
		while(1)
		{
			if(item->next == NULL)
			{
				item->next = param;
				break;
			}
			item = item->next;
		}
	}
}

void _clear_preload_list(uint32_t scene_id)
{
	preload_param_t* item;
	preload_param_t* prev;
	preload_param_t* found;
	os_mutex_lock(&preload_mutex, OS_FOREVER);
	os_strace_u32x2(SYS_TRACE_ID_RES_PRELOAD_CANCEL, (uint32_t)scene_id, (uint32_t)param_list);

	if(param_list == NULL)
	{
		os_strace_end_call_u32(SYS_TRACE_ID_RES_PRELOAD_CANCEL, (uint32_t)param_list);
		os_mutex_unlock(&preload_mutex);
		return;
	}

	item = param_list;
	prev = NULL;
	if(scene_id == 0)
	{
		while(item != NULL)
		{
			param_list = item->next;
			if(item->preload_type == PRELOAD_TYPE_END_CALLBACK)
			{
				if(item->scene_id > 0)
				{
					res_manager_unload_scene(item->scene_id, NULL);
				}			
				if (item->callback)
					item->callback(LVGL_RES_PRELOAD_STATUS_CANCELED, item->param);
			}
			else
			{
				res_manager_free_resource_structure(item->bitmap);
			}
			memset(item, 0, sizeof(preload_param_t));
			res_array_free(item);
			item = param_list;
		}
	}
	else
	{
		while(item != NULL)
		{
			if(item->scene_id == scene_id)
			{
				if(item->preload_type == PRELOAD_TYPE_END_CALLBACK)
				{
					if(item->scene_id > 0)
					{
						res_manager_unload_scene(item->scene_id, NULL);
					}
					if (item->callback)
						item->callback(LVGL_RES_PRELOAD_STATUS_CANCELED, item->param);
				}
				else
				{
					res_manager_free_resource_structure(item->bitmap);
				}				

				if(param_list == item)
				{
					param_list = item->next;
				}
				if(prev != NULL)
				{
					prev->next = item->next;
				}
				found = item;
				item = item->next;
				memset(found, 0, sizeof(preload_param_t));
				res_array_free(found);
			}
			else
			{
				prev = item;
				item = item->next;
			}
		}
	}
	os_strace_end_call_u32(SYS_TRACE_ID_RES_PRELOAD_CANCEL, (uint32_t)param_list);
	os_mutex_unlock(&preload_mutex);
}

void _pause_preload(void)
{
	preload_running = 2;
}

void _resume_preload(void)
{
	preload_running = 1;
}

int lvgl_res_preload_pictures_from_scene(uint32_t scene_id, lvgl_res_scene_t* scene, const uint32_t* id, uint32_t num)
{
#ifdef CONFIG_RES_MANAGER_SKIP_PRELOAD
	return 0;
#else
	int32_t i;
	preload_param_t* param;
	resource_bitmap_t* bitmap;

	if(scene == NULL || id == NULL || num == 0)
	{
		SYS_LOG_ERR("invalid param to preload %p, %p, %d", scene, id, num);
		return -1;
	}

	os_strace_u32(SYS_TRACE_ID_RES_PICS_PRELOAD, (uint32_t)scene->scene_data);
	for(i=0; i<num; i++)
	{
		bitmap = res_manager_preload_from_scene(scene->res_info, scene->scene_data, id[i]);
		if(bitmap == NULL)
		{
			SYS_LOG_ERR("preload %d bitmap error", id[i]);
			continue;
		}

		param = (preload_param_t*)res_array_alloc(RES_MEM_SIMPLE_PRELOAD, sizeof(preload_param_t));
		if(param == NULL)
		{
			SYS_LOG_ERR("no space for param for preload ");
			break;
		}
		if(scene_id > 0)
		{
			param->preload_type = PRELOAD_TYPE_NORMAL_COMPACT;
			param->scene_id = scene_id;
		}
		else
		{
			param->preload_type = PRELOAD_TYPE_NORMAL;
		}
		param->bitmap = bitmap;
		param->next = NULL;
		param->res_info = scene->res_info;

		_add_item_to_preload_list(param);
	}

	os_strace_end_call_u32(SYS_TRACE_ID_RES_PICS_PRELOAD, (uint32_t)scene->scene_data);
	return 0;
#endif //CONFIG_RES_MANAGER_SKIP_PRELOAD
}

int lvgl_res_preload_pictures_from_group(uint32_t scene_id, lvgl_res_group_t* group, const uint32_t* id, uint32_t num)
{
#ifdef CONFIG_RES_MANAGER_SKIP_PRELOAD
	return 0;
#else
	int32_t i;
	preload_param_t* param;
	resource_bitmap_t* bitmap;

	if(group == NULL || id == NULL || num == 0)
	{
		SYS_LOG_ERR("invalid param to preload %p, %p, %d", group, id, num);
		return -1;
	}

	os_strace_u32(SYS_TRACE_ID_RES_PICS_PRELOAD, (uint32_t)group->group_data);
	for(i=0; i<num; i++)
	{
		bitmap = res_manager_preload_from_group(group->res_info, group->group_data, scene_id, id[i]);
		if(bitmap == NULL)
		{
			SYS_LOG_ERR("preload %d bitmap error", id[i]);
			continue;
		}

		param = (preload_param_t*)res_array_alloc(RES_MEM_SIMPLE_PRELOAD, sizeof(preload_param_t));
		if(param == NULL)
		{
			SYS_LOG_ERR("no space for param for preload");
			break;
		}
		if(scene_id > 0)
		{
			param->preload_type = PRELOAD_TYPE_NORMAL_COMPACT;
			param->scene_id = scene_id;
		}
		else
		{
			param->preload_type = PRELOAD_TYPE_NORMAL;
		}
		param->bitmap = bitmap;
		param->next = NULL;
		param->res_info = group->res_info;

		_add_item_to_preload_list(param);
	}
	os_strace_end_call_u32(SYS_TRACE_ID_RES_PICS_PRELOAD, (uint32_t)group->group_data);

	return 0;
#endif //CONFIG_RES_MANAGER_SKIP_PRELOAD
}

int lvgl_res_preload_pictures_from_picregion(uint32_t scene_id, lvgl_res_picregion_t* picreg, uint32_t start, uint32_t end)
{
#ifdef CONFIG_RES_MANAGER_SKIP_PRELOAD
	return 0;
#else
	int32_t i;
	preload_param_t* param;
	resource_bitmap_t* bitmap;


	if(picreg == NULL || start > end)
	{
		SYS_LOG_ERR("invalid param to preload %p, %d, %d", picreg, start, end);
		return -1;
	}

	os_strace_u32(SYS_TRACE_ID_RES_PICS_PRELOAD, (uint32_t)picreg->picreg_data);
	for(i=start; i<=end; i++)
	{
		bitmap = res_manager_preload_from_picregion(picreg->res_info, picreg->picreg_data, i);
		if(bitmap == NULL)
		{
			SYS_LOG_ERR("preload %d bitmap error", i);
			continue;
		}

		param = (preload_param_t*)res_array_alloc(RES_MEM_SIMPLE_PRELOAD, sizeof(preload_param_t));
		if(param == NULL)
		{
			SYS_LOG_ERR("no space for param for preload");
			break;
		}

		if(scene_id > 0)
		{
			param->preload_type = PRELOAD_TYPE_NORMAL_COMPACT;
			param->scene_id = scene_id;
		}
		else
		{
			param->preload_type = PRELOAD_TYPE_NORMAL;
		}
		param->bitmap = bitmap;
		param->next = NULL;
		param->res_info = picreg->res_info;

		_add_item_to_preload_list(param);
	}

	os_strace_end_call_u32(SYS_TRACE_ID_RES_PICS_PRELOAD, (uint32_t)picreg->picreg_data);
	return 0;
#endif //CONFIG_RES_MANAGER_SKIP_PRELOAD
}

#ifndef CONFIG_RES_MANAGER_SKIP_PRELOAD
static void _res_preload_thread(void *parama1, void *parama2, void *parama3)
{
	preload_param_t* param_item;
	int32_t ret = 0;

	while(preload_running)
	{
		os_mutex_lock(&preload_mutex, OS_FOREVER);
		
		if(param_list == NULL)
		{
			os_mutex_unlock(&preload_mutex);
			os_sem_take(&preload_sem, OS_FOREVER);
			os_mutex_lock(&preload_mutex, OS_FOREVER);
		}

		if(param_list == NULL)
		{
			os_mutex_unlock(&preload_mutex);
			continue;
		}
		os_strace_u32x2(SYS_TRACE_ID_RES_SCENE_PRELOAD_0, (uint32_t)param_list, (uint32_t)param_list->next);
		param_item = param_list;
		param_list = param_item->next;
		

		if(preload_running == 2)
		{
			os_strace_end_call_u32(SYS_TRACE_ID_RES_SCENE_PRELOAD_0, (uint32_t)param_list);
			os_mutex_unlock(&preload_mutex);
			continue;
		}

		if(param_item->preload_type == PRELOAD_TYPE_NORMAL)
		{
			ret = res_manager_preload_bitmap(param_item->res_info, param_item->bitmap);				
			res_manager_free_resource_structure(param_item->bitmap);
		}
		else if(param_item->preload_type == PRELOAD_TYPE_NORMAL_COMPACT)
		{
			ret = res_manager_preload_bitmap_compact(param_item->scene_id, param_item->res_info, param_item->bitmap);	
			res_manager_free_resource_structure(param_item->bitmap);
		}
		else if(param_item->preload_type == PRELOAD_TYPE_BEGIN_CALLBACK)
		{
			if (param_item->callback)
				param_item->callback(LVGL_RES_PRELOAD_STATUS_LOADING, param_item->param);

		}
		else if(param_item->preload_type == PRELOAD_TYPE_END_CALLBACK)
		{
			//user callback ,add at the end of preloaded pics to inform user about preload finish
			//res_manager_preload_finish_check(param_item->scene_id);

			if (param_item->callback)
				param_item->callback(LVGL_RES_PRELOAD_STATUS_FINISHED, param_item->param);
//			_dump_sram_usage();
			
		}
		else
		{
			//presumably already freed, just continue;
			SYS_LOG_ERR("unknown preload type: %d\n", param_item->preload_type);
		}
		memset(param_item, 0, sizeof(preload_param_t));
		res_array_free(param_item);
		os_strace_end_call_u32(SYS_TRACE_ID_RES_SCENE_PRELOAD_0, (uint32_t)param_list);
		os_mutex_unlock(&preload_mutex);
	}
}
#endif

void _dump_preload_list(void)
{
	preload_param_t* item = param_list;

	while(item)
	{
		printf("preload item scene 0x%x, type %d\n", item->scene_id, item->preload_type);
		item=item->next;
	}
}

int lvgl_res_preload_cancel(void)
{
#ifndef CONFIG_RES_MANAGER_SKIP_PRELOAD
	_clear_preload_list(0);
#endif
	return 0;
}

int lvgl_res_preload_cancel_scene(uint32_t scene_id)
{
#ifndef CONFIG_RES_MANAGER_SKIP_PRELOAD

	_clear_preload_list(scene_id);

//	_dump_preload_list();
#endif
	return 0;
}

int _res_preload_pictures_from_picregion(uint32_t scene_id, lvgl_res_picregion_t* picreg, uint32_t start, uint32_t end, preload_param_t** sublist)
{
	int32_t i;
	preload_param_t* param;
	preload_param_t* tail;
	resource_bitmap_t* bitmap;
	int preload_count = 0;

	if(picreg == NULL || start > end)
	{
		SYS_LOG_ERR("invalid param to preload %p, %d, %d", picreg, start, end);
		return -1;
	}

	os_strace_u32(SYS_TRACE_ID_RES_PICS_PRELOAD, (uint32_t)picreg->picreg_data);
	for(i=start; i<=end; i++)
	{
		bitmap = res_manager_preload_from_picregion(picreg->res_info, picreg->picreg_data, i);
		if(bitmap == NULL)
		{
			SYS_LOG_ERR("preload %d bitmap error", i);
			continue;
		}
		tail = _check_item_in_list(*sublist, bitmap);
		if(tail == NULL)
		{
			//already in sublist, FIXME:only checked sublist for performance
			res_manager_release_resource(bitmap);
			continue;
		}

		param = (preload_param_t*)res_array_alloc(RES_MEM_SIMPLE_PRELOAD, sizeof(preload_param_t));
		if(param == NULL)
		{
			SYS_LOG_ERR("no space for param for preload");
			break;
		}

		if(scene_id > 0)
		{
			param->preload_type = PRELOAD_TYPE_NORMAL_COMPACT;
			param->scene_id = scene_id;
		}
		else
		{
			param->preload_type = PRELOAD_TYPE_NORMAL;
		}
		param->bitmap = bitmap;
		param->next = NULL;
		param->res_info = picreg->res_info;

		tail->next = param;	
		preload_count++;
	}

	os_strace_end_call_u32(SYS_TRACE_ID_RES_PICS_PRELOAD, (uint32_t)picreg->picreg_data);
	return preload_count;

}


void _add_preload_total_size(uint32_t scene_id, uint32_t* ptotal_size, uint32_t size)
{
	uint32_t total_size;

	if(ptotal_size == NULL)
	{
		return;
	}

	size = ((size+res_mem_align-1)/res_mem_align)*res_mem_align;
	total_size = *ptotal_size;

	if(total_size + size > max_compact_block_size)
	{
		//split block
		SYS_LOG_INF("\n ###  scene 0x%x, split total_size %d\n", scene_id, total_size);
		if(total_size > 0)
		{
			res_manager_init_compact_buffer(scene_id, total_size);
		}
		
		if(size >= max_compact_block_size)
		{
			SYS_LOG_INF("\n ###  scene 0x%x, split total_size %d\n", scene_id, size);
			res_manager_init_compact_buffer(scene_id, size);
			total_size = 0;
		}
		else
		{
			total_size = size;
		}
		*ptotal_size = total_size;
	}
	else
	{
		total_size += size;
		*ptotal_size = total_size;
	}

	return;
}

int _res_preload_group_compact(resource_info_t* info, uint32_t scene_id, uint32_t pargroup_id, resource_group_t* group, uint32_t* ptotal_size, uint32_t preload, preload_param_t** sublist)
{
	preload_param_t* param;
	preload_param_t* tail;
	resource_group_t* res_group;
	resource_bitmap_t* bitmap;
	lvgl_res_picregion_t picreg;
	uint32_t count = 0;
	uint32_t offset = 0;
	uint32_t total_size = *ptotal_size;	
	uint32_t inc_size = 0;
	uint32_t buf_block_struct_size = res_manager_get_bitmap_buf_block_unit_size();
	int picreg_count = 0;

	if(info == NULL)
	{
		SYS_LOG_ERR("resources not opened\n");
		return -1;
	}

	while(1)
	{
		void* resource = res_manager_preload_next_group_child(info, group, &count, &offset, scene_id, pargroup_id);
		if(resource == NULL)
		{
			if(count >= group->sty_data->resource_sum)
			{
				//reach end of scene
				break;
			}

			//text or invalid param which shouldnt happen here
			continue;
		}
		
		uint32_t type = *((uint32_t*)(*((uint32_t*)(resource))));

		switch(type)
		{
		case RESOURCE_TYPE_GROUP:
			res_group = (resource_group_t*)resource;
			if(res_group->sty_data->resource_sum > 0)
			{
				if(pargroup_id > 0)
				{
					//regular check only support 2 level depth, dont check regular after that
					_res_preload_group_compact(info, scene_id, 0xffffffff, (resource_group_t*)resource, &total_size, preload, sublist);
				}
				else
				{
					_res_preload_group_compact(info, scene_id, group->sty_data->sty_id, (resource_group_t*)resource, &total_size, preload, sublist);
				}
			}
			res_manager_release_resource(resource);
			break;
		case RESOURCE_TYPE_PICREGION:
			picreg.picreg_data = (resource_picregion_t*)resource;
			picreg.res_info = info;
			if(preload)
			{
				picreg_count = _res_preload_pictures_from_picregion(scene_id, &picreg, 0, picreg.picreg_data->sty_data->frames-1, sublist);
			}
			
			if(picreg_count > 0 && picreg.picreg_data->regular_info == 0)
			{
				inc_size = (picreg.picreg_data->sty_data->width*picreg.picreg_data->sty_data->height*picreg.picreg_data->sty_data->bytes_per_pixel + buf_block_struct_size);
				inc_size = ((inc_size+res_mem_align-1)/res_mem_align)*res_mem_align;
				inc_size = inc_size*picreg_count;			
				_add_preload_total_size(scene_id, &total_size, inc_size);
				//total_size += (picreg.picreg_data->sty_data->width*picreg.picreg_data->sty_data->height*picreg.picreg_data->sty_data->bytes_per_pixel + buf_block_struct_size)*picreg_count;			
			}

			res_manager_release_resource(resource);
			break;
		case RESOURCE_TYPE_PICTURE:
			bitmap = (resource_bitmap_t*)resource;
			tail = _check_item_in_list(*sublist, bitmap);
			if(tail == NULL)
			{
				//already in sublist, FIXME:only checked sublist for performance
				res_manager_release_resource(resource);
				continue;
			}
			if(bitmap->regular_info == 0)
			{
				if(bitmap->sty_data->width == screen_width  && bitmap->sty_data->height == screen_height && bitmap->sty_data->bytes_per_pixel == 2)
				{
					inc_size = buf_block_struct_size;
					_add_preload_total_size(scene_id, &total_size, inc_size);
					//total_size += buf_block_struct_size;
				}
				else
				{
					inc_size = (bitmap->sty_data->width*bitmap->sty_data->height*bitmap->sty_data->bytes_per_pixel + buf_block_struct_size);
					_add_preload_total_size(scene_id, &total_size, inc_size);
					//total_size += (bitmap->sty_data->width*bitmap->sty_data->height*bitmap->sty_data->bytes_per_pixel + buf_block_struct_size);
				}
			}
			
			if(preload)
			{
				param = (preload_param_t*)res_array_alloc(RES_MEM_SIMPLE_PRELOAD, sizeof(preload_param_t));
				if(param == NULL)
				{
					SYS_LOG_ERR("no space for param for preload");
					break;
				}

				if(scene_id == 0)
				{
					param->preload_type = PRELOAD_TYPE_NORMAL;
				}
				else
				{
					param->preload_type = PRELOAD_TYPE_NORMAL_COMPACT;
					param->scene_id = scene_id;
				}
				param->bitmap = bitmap;
				param->next = NULL;
				param->res_info = info;

				_add_item_to_list(sublist, param);
			}
			break;
		default:
			//ignore text resource
			break;
		}
	}

	*ptotal_size = total_size;
	return 0;
}

int _res_preload_scene_compact(uint32_t scene_id, const uint32_t* resource_id, uint32_t resource_num, void (*callback)(int32_t, void *), void* user_data,
											const char* style_path, const char* picture_path, const char* text_path, bool async_preload)
{
	resource_info_t* info;
	preload_param_t* param;
	preload_param_t* sublist;
	preload_param_t* tail;
	resource_bitmap_t* bitmap;
	resource_scene_t* res_scene;
	resource_group_t* res_group;
	void* resource;
	lvgl_res_picregion_t picreg;
	uint32_t count = 0;
	uint32_t offset = 0;
	uint32_t total_size = 0;
	uint32_t inc_size = 0;
	uint32_t buf_block_struct_size = res_manager_get_bitmap_buf_block_unit_size();
	int picreg_count = 0;

	info = _res_file_open(style_path, picture_path, text_path, 0, 1);
	if(info == NULL)
	{
		SYS_LOG_ERR("resource info error");
		return-1;
	}

	os_strace_u32(SYS_TRACE_ID_RES_SCENE_PRELOAD_1, (uint32_t)scene_id);	
	res_scene = res_manager_load_scene(info, scene_id);
	if(res_scene == NULL)
	{
		return -1;
	}
	os_strace_end_call_u32(SYS_TRACE_ID_RES_SCENE_PRELOAD_1, (uint32_t)scene_id);
	os_strace_u32(SYS_TRACE_ID_RES_SCENE_PRELOAD_3, (uint32_t)scene_id);	
	sublist = NULL;
	param = (preload_param_t*)res_array_alloc(RES_MEM_SIMPLE_PRELOAD, sizeof(preload_param_t));
	if(param == NULL)
	{
		SYS_LOG_ERR("no space for param for preload");
		return -1;
	}
	memset(param, 0, sizeof(preload_param_t));
	param->callback = callback;
	param->param = user_data;
	param->scene_id = scene_id;
	param->preload_type = PRELOAD_TYPE_BEGIN_CALLBACK;
	param->next = NULL;
	param->res_info = NULL;
	_add_item_to_list(&sublist, param);

	//preload group, be sure that scene compact buffer is inited already
	if(resource_id != NULL)
	{
		uint32_t i;
		for(i=0;i<resource_num;i++)
		{
			resource = (void*)res_manager_preload_from_scene(info, res_scene, resource_id[i]);
			if(resource == NULL)
			{
				//somehow not found, just ignore it
				continue;
			}
			uint32_t type = *((uint32_t*)(*((uint32_t*)(resource))));
			switch(type)
			{
			case RESOURCE_TYPE_GROUP:
				res_group = (resource_group_t*)resource;
				if(res_group->sty_data->resource_sum > 0)
				{
					_res_preload_group_compact(info, scene_id, 0, (resource_group_t*)resource, &total_size, 1, &sublist);
				}
				res_manager_release_resource(resource);
				break;
			case RESOURCE_TYPE_PICREGION:
				picreg.picreg_data = (resource_picregion_t*)resource;
				picreg.res_info = info;
				picreg_count = _res_preload_pictures_from_picregion(scene_id, &picreg, 0, picreg.picreg_data->sty_data->frames-1, &sublist);
				if(picreg_count > 0 && picreg.picreg_data->regular_info == 0)
				{
					inc_size = (picreg.picreg_data->sty_data->width*picreg.picreg_data->sty_data->height*picreg.picreg_data->sty_data->bytes_per_pixel + buf_block_struct_size);
					inc_size = ((inc_size+res_mem_align-1)/res_mem_align)*res_mem_align;
					inc_size = inc_size*picreg_count;
					_add_preload_total_size(scene_id, &total_size, inc_size);
					//total_size += (picreg.picreg_data->sty_data->width*picreg.picreg_data->sty_data->height*picreg.picreg_data->sty_data->bytes_per_pixel + buf_block_struct_size)*picreg_count;
				}
				res_manager_release_resource(resource);
				break;
			case RESOURCE_TYPE_PICTURE:
				bitmap = (resource_bitmap_t*)resource;
				tail = _check_item_in_list(sublist, bitmap);
				if(tail == NULL)
				{
					//already in sublist, FIXME:only checked sublist for performance
					res_manager_release_resource(resource);
					continue;
				}				
				if(bitmap->regular_info == 0)
				{
					if(bitmap->sty_data->width == screen_width	&& bitmap->sty_data->height == screen_height && bitmap->sty_data->bytes_per_pixel == 2)
					{
						inc_size = buf_block_struct_size;
						_add_preload_total_size(scene_id, &total_size, inc_size);
						//total_size += buf_block_struct_size;
					}
					else
					{
						inc_size = (bitmap->sty_data->width*bitmap->sty_data->height*bitmap->sty_data->bytes_per_pixel + buf_block_struct_size); 			
						_add_preload_total_size(scene_id, &total_size, inc_size);
						//total_size += (bitmap->sty_data->width*bitmap->sty_data->height*bitmap->sty_data->bytes_per_pixel + buf_block_struct_size);				
					}
				}

				param = (preload_param_t*)res_array_alloc(RES_MEM_SIMPLE_PRELOAD, sizeof(preload_param_t));
				if(param == NULL)
				{
					SYS_LOG_ERR("no space for param for preload");
					break;
				}
				param->preload_type = PRELOAD_TYPE_NORMAL_COMPACT;
				param->bitmap = bitmap;
				param->scene_id = scene_id;
				param->next = NULL;
				param->res_info = info;

				tail->next = param;
				break;
			default:
				break;
			}
		}

		param = (preload_param_t*)res_array_alloc(RES_MEM_SIMPLE_PRELOAD, sizeof(preload_param_t));
		if(param == NULL)
		{
			SYS_LOG_ERR("no space for param for preload");
			return -1;
		}
		memset(param, 0, sizeof(preload_param_t));
		param->callback = callback;
		param->param = user_data;
		param->scene_id = scene_id;
		param->preload_type = PRELOAD_TYPE_END_CALLBACK;
		param->next = NULL;

		_add_item_to_list(&sublist, param);

		if(total_size > 0)
		{
			res_manager_init_compact_buffer(scene_id, total_size);	
		}
		else
		{
			//for scene without pic resources save a node for status check
			res_manager_init_compact_buffer(scene_id, buf_block_struct_size);
		}
		
		if(async_preload)
		{
			_add_item_to_preload_list(sublist);		
		}
		else
		{
			_add_item_to_loading_list(sublist);
		}
		
//		_dump_sram_usage();
		os_strace_end_call_u32(SYS_TRACE_ID_RES_SCENE_PRELOAD_3, (uint32_t)scene_id);

		SYS_LOG_INF("\n #### scene res 0x%x, total_size %d\n", scene_id, total_size);
		return 0;
	}

	//calculate scene bitmap buffer
//	_pause_preload();
	//fill preload list
	while(1)
	{
		void* resource = res_manager_preload_next_scene_child(info, res_scene, &count, &offset);
		if(resource == NULL)
		{
			if(count >= res_scene->resource_sum)
			{
				//reach end of scene
				break;
			}

			//text or invalid param which shouldnt happen here
			continue;
		}

		uint32_t type = *((uint32_t*)(*((uint32_t*)(resource))));
		SYS_LOG_DBG("scene 0x%x, resource type %d, count %d\n",scene_id, type, count);
		switch(type)
		{
		case RESOURCE_TYPE_GROUP:
			res_group = (resource_group_t*)resource;
			if(res_group->sty_data->resource_sum > 0)
			{
				_res_preload_group_compact(info, scene_id, 0, (resource_group_t*)resource, &total_size, 1, &sublist);
			}
			res_manager_release_resource(resource);
			break;
		case RESOURCE_TYPE_PICREGION:
			picreg.picreg_data = (resource_picregion_t*)resource;
			picreg.res_info = info;
			picreg_count = _res_preload_pictures_from_picregion(scene_id, &picreg, 0, picreg.picreg_data->sty_data->frames-1, &sublist);
			if(picreg_count > 0 && picreg.picreg_data->regular_info == 0)
			{
				inc_size = (picreg.picreg_data->sty_data->width*picreg.picreg_data->sty_data->height*picreg.picreg_data->sty_data->bytes_per_pixel + buf_block_struct_size);
				inc_size = ((inc_size+res_mem_align-1)/res_mem_align)*res_mem_align;
				inc_size = inc_size*picreg_count;			
				_add_preload_total_size(scene_id, &total_size, inc_size);
				//total_size += (picreg.picreg_data->sty_data->width*picreg.picreg_data->sty_data->height*picreg.picreg_data->sty_data->bytes_per_pixel + buf_block_struct_size)*picreg_count;			
			}
			res_manager_release_resource(resource);
			break;
		case RESOURCE_TYPE_PICTURE:
			bitmap = (resource_bitmap_t*)resource;
			tail = _check_item_in_list(sublist, bitmap);
			if(tail == NULL)
			{
				//already in sublist, FIXME:only checked sublist for performance
				res_manager_release_resource(resource);
				continue;
			}			
			if(bitmap->regular_info == 0)
			{
				if(bitmap->sty_data->width == screen_width  && bitmap->sty_data->height == screen_height && bitmap->sty_data->bytes_per_pixel == 2)
				{
					inc_size = buf_block_struct_size;
					_add_preload_total_size(scene_id, &total_size, inc_size);
					//total_size += buf_block_struct_size;
				}
				else
				{
					inc_size = (bitmap->sty_data->width*bitmap->sty_data->height*bitmap->sty_data->bytes_per_pixel + buf_block_struct_size); 			
					_add_preload_total_size(scene_id, &total_size, inc_size);
					//total_size += (bitmap->sty_data->width*bitmap->sty_data->height*bitmap->sty_data->bytes_per_pixel + buf_block_struct_size); 			
				}
			}

			param = (preload_param_t*)res_array_alloc(RES_MEM_SIMPLE_PRELOAD, sizeof(preload_param_t));
			if(param == NULL)
			{
				SYS_LOG_ERR("no space for param for preload");
				break;
			}

			param->preload_type = PRELOAD_TYPE_NORMAL_COMPACT;
			param->bitmap = bitmap;
			param->scene_id = scene_id;
			param->next = NULL;
			param->res_info = info;

			tail->next = param;
			break;
		default:
			//ignore text resource
			break;
		}
	}

	param = (preload_param_t*)res_array_alloc(RES_MEM_SIMPLE_PRELOAD, sizeof(preload_param_t));
	if(param == NULL)
	{
		SYS_LOG_ERR("no space for param for preload");
		return -1;
	}

	memset(param, 0, sizeof(preload_param_t));
	param->callback = callback;
	param->param = user_data;
	param->scene_id = scene_id;
	param->preload_type = PRELOAD_TYPE_END_CALLBACK;
	param->next = NULL;
	_add_item_to_list(&sublist, param);

	if(total_size > 0)
	{
		res_manager_init_compact_buffer(scene_id, total_size);
	}
	else
	{
		//for scene without pic resources save a node for status check
		res_manager_init_compact_buffer(scene_id, buf_block_struct_size);
	}
	
	if(async_preload)
	{
		_add_item_to_preload_list(sublist); 	
	}
	else
	{
		_add_item_to_loading_list(sublist);
	}
//	_dump_sram_usage();
	os_strace_end_call_u32(SYS_TRACE_ID_RES_SCENE_PRELOAD_3, (uint32_t)scene_id);
	SYS_LOG_INF("\n ###  scene 0x%x, total_size %d\n", scene_id, total_size);
	return 0;
}

int lvgl_res_preload_scene_compact(uint32_t scene_id, const uint32_t* resource_id, uint32_t resource_num, void (*callback)(int32_t, void *), void* user_data,
											const char* style_path, const char* picture_path, const char* text_path)
{
#ifdef CONFIG_RES_MANAGER_SKIP_PRELOAD
	if(callback == lvgl_res_scene_preload_default_cb_for_view)
	{
		ui_view_update((uint32_t)user_data);
	}
	else
	{
		callback(LVGL_RES_PRELOAD_STATUS_FINISHED, user_data);
	}
	return 0;
#else
	int ret;	
	ret = _res_preload_scene_compact(scene_id, resource_id, resource_num, callback, user_data, style_path, picture_path, text_path, 1);
	if(ret < 0)
	{
		SYS_LOG_ERR("preload failed 0x%x\n", scene_id);
		return -1;
	}

	return 0;
#endif //CONFIG_RES_MANAGER_SKIP_PRELOAD
}

int lvgl_res_preload_scene_compact_serial(uint32_t scene_id, const uint32_t* resource_id, uint32_t resource_num, void (*callback)(int32_t, void *), void* user_data,
											const char* style_path, const char* picture_path, const char* text_path)
{
#ifdef CONFIG_RES_MANAGER_SKIP_PRELOAD
	if(callback == lvgl_res_scene_preload_default_cb_for_view)
	{
		ui_view_update((uint32_t)user_data);
	}
	else
	{
		callback(LVGL_RES_PRELOAD_STATUS_FINISHED, user_data);
	}
	return 0;
#else
	int ret;
	preload_param_t* param_item;
	ret = _res_preload_scene_compact(scene_id, resource_id, resource_num, callback, user_data, style_path, picture_path, text_path, 0);
	if(ret < 0)
	{
		SYS_LOG_ERR("preload failed 0x%x\n", scene_id);
		return -1;
	}

	if(sync_param_list == NULL)
	{
		SYS_LOG_ERR("invalid loading list\n");
		return -1;
	}
	
	while(sync_param_list)
	{
		param_item = sync_param_list;
		sync_param_list = param_item->next;
		
		if(param_item->preload_type == PRELOAD_TYPE_NORMAL)
		{
			ret = res_manager_preload_bitmap(param_item->res_info, param_item->bitmap);			
			res_manager_free_resource_structure(param_item->bitmap);
		}
		else if(param_item->preload_type == PRELOAD_TYPE_NORMAL_COMPACT)
		{
			ret = res_manager_preload_bitmap_compact(param_item->scene_id, param_item->res_info, param_item->bitmap);
			res_manager_free_resource_structure(param_item->bitmap);	
		}
		else if(param_item->preload_type == PRELOAD_TYPE_BEGIN_CALLBACK)
		{
			//FIXME
		}
		else if(param_item->preload_type == PRELOAD_TYPE_END_CALLBACK)
		{
			//FIXME
		}
		else
		{
			//presumably already freed
			SYS_LOG_ERR("unknown preload type: %d\n", param_item->preload_type);
			continue;
		}
		memset(param_item, 0, sizeof(preload_param_t));
		res_array_free(param_item);
	}
	return 0;
#endif //CONFIG_RES_MANAGER_SKIP_PRELOAD
}

void lvgl_res_scene_preload_default_cb_for_view(int32_t status, void *view_id)
{
	SYS_LOG_INF("preload view %u, status %d\n", (uint32_t)view_id, status);

	if (status == LVGL_RES_PRELOAD_STATUS_LOADING) {
		os_strace_u32(SYS_TRACE_ID_VIEW_PRELOAD, (uint32_t)view_id);
	} else {
		os_strace_end_call_u32(SYS_TRACE_ID_VIEW_PRELOAD, (uint32_t)view_id);
	}

#ifdef CONFIG_UI_MANAGER
	if (status == LVGL_RES_PRELOAD_STATUS_FINISHED) {
		ui_view_update((uint32_t)view_id);
	}
#endif
}

int lvgl_res_preload_scene_compact_default_init(uint32_t scene_id, const uint32_t* resource_id, uint32_t resource_num, void (*callback)(int32_t, void *),
											const char* style_path, const char* picture_path, const char* text_path)
{
#ifdef CONFIG_RES_MANAGER_SKIP_PRELOAD
	return 0;
#else

	preload_default_t* record;
	preload_default_t* item;
	int32_t i;
	uint32_t path_total_len;
	uint32_t pic_path_start;
	uint32_t str_path_start;

	if(style_path == NULL)
	{
		SYS_LOG_ERR("invalid init param, need style path\n");
		return -1;
	}

	path_total_len = strlen(style_path)+1;
	path_total_len = ((path_total_len+3)/4)*4;
	pic_path_start = path_total_len;
	if(picture_path)
	{
		path_total_len += strlen(picture_path)+1;
		path_total_len = ((path_total_len+3)/4)*4;
	}
	str_path_start = path_total_len;
	if(text_path)
	{
		path_total_len += strlen(text_path)+1;
		path_total_len = ((path_total_len+3)/4)*4;
	}

	record = preload_default_list;
	while(record != NULL)
	{
		if(record->scene_id == scene_id)
		{
			return 0;
		}
		record = record->next;
	}

	item = res_mem_alloc(RES_MEM_POOL_BMP ,sizeof(preload_default_t)+path_total_len+resource_num*sizeof(uint32_t));
	if(item == NULL)
	{
		SYS_LOG_ERR("alloc preload defautl data failed 0x%x\n", scene_id);
		res_manager_dump_info();
		return -1;
	}
	item->scene_id = scene_id;
	if(resource_id)
	{
		item->resource_id = (uint32_t*)((uint32_t)item+sizeof(preload_default_t)+path_total_len);
	}
	else
	{
		item->resource_id = NULL;
	}
	item->resource_num = resource_num;
	item->callback = callback;

	item->style_path = (char*)((uint32_t)item+sizeof(preload_default_t));
	memset(item->style_path, 0, strlen(style_path)+1);
	strcpy(item->style_path, style_path);

	if(picture_path)
	{
		item->picture_path = item->style_path + pic_path_start;
		memset(item->picture_path, 0, strlen(picture_path)+1);
		strcpy(item->picture_path, picture_path);
	}
	else
	{
		item->picture_path = NULL;
	}

	if(text_path)
	{
		item->text_path = item->style_path + str_path_start;
		memset(item->text_path, 0, strlen(text_path)+1);
		strcpy(item->text_path, text_path);
	}
	else
	{
		item->text_path = NULL;
	}

	item->next = NULL;

	if(resource_id)
	{
		for(i=0;i<resource_num;i++)
		{
			item->resource_id[i] = resource_id[i];
		}
	}

	if(preload_default_list == NULL)
	{
		preload_default_list = item;
		return 0;
	}

	record = preload_default_list;
	while(record != NULL)
	{
		if(record->next == NULL)
		{
			record->next = item;
			return 0;
		}
		else
		{
			record = record->next;
		}
	}
	return 0;
#endif //CONFIG_RES_MANAGER_SKIP_PRELOAD
}

int lvgl_res_scene_is_loaded(uint32_t scene_id)
{
	return res_manager_scene_is_loaded(scene_id);
}

int lvgl_res_preload_scene_compact_default_with_path(uint32_t scene_id, uint32_t view_id, bool reload_update, bool load_serial,
																	const char* style_path, const char* picture_path, const char* text_path)
{
	int ret = 0;
#ifdef CONFIG_RES_MANAGER_SKIP_PRELOAD
	ui_view_update((uint32_t)view_id);
#else
	preload_default_t* record = preload_default_list;

	while(record != NULL)
	{
		if(record->scene_id == scene_id)
		{
			if(reload_update)
			{
#ifdef CONFIG_UI_MANAGER
				view_set_refresh_en(view_id, 0);
#endif
				if(record->callback == NULL)
				{
					ret = lvgl_res_preload_scene_compact(scene_id, record->resource_id, record->resource_num,
									lvgl_res_scene_preload_default_cb_for_view, (void*)view_id,
									style_path, picture_path, text_path);
				}
				else
				{
					ret = lvgl_res_preload_scene_compact(scene_id, record->resource_id, record->resource_num,
									record->callback, (void*)view_id,
									style_path, picture_path, text_path);
				}
			}
			else if(!load_serial)
			{
				if(record->callback == NULL)
				{
					ret = lvgl_res_preload_scene_compact(scene_id, record->resource_id, record->resource_num,
									lvgl_res_scene_preload_default_cb_for_view, (void*)view_id,
									style_path, picture_path, text_path);
				}
				else
				{
					ret = lvgl_res_preload_scene_compact(scene_id, record->resource_id, record->resource_num,
									record->callback, (void*)view_id,
									style_path, picture_path, text_path);
				}
			}
			else
			{
				if(record->callback == NULL)
				{
					ret = lvgl_res_preload_scene_compact_serial(scene_id, record->resource_id, record->resource_num,
									lvgl_res_scene_preload_default_cb_for_view, (void*)view_id,
									style_path, picture_path, text_path);
				}
				else
				{
					ret = lvgl_res_preload_scene_compact_serial(scene_id, record->resource_id, record->resource_num,
									record->callback, (void*)view_id,
									style_path, picture_path, text_path);
				}
			}
		}
		record = record->next;
	}
#endif //CONFIG_RES_MANAGER_SKIP_PRELOAD
	return ret;
}

int lvgl_res_preload_scene_compact_default(uint32_t scene_id, uint32_t view_id, bool reload_update, bool load_serial)
{
	int ret = 0;
#ifdef CONFIG_RES_MANAGER_SKIP_PRELOAD
	ui_view_update((uint32_t)view_id);
#else
	preload_default_t* record = preload_default_list;

	while(record != NULL)
	{
		if(record->scene_id == scene_id)
		{
			if(reload_update)
			{
#ifdef CONFIG_UI_MANAGER
				view_set_refresh_en(view_id, 0);
#endif
				if(record->callback == NULL)
				{
					ret = lvgl_res_preload_scene_compact(scene_id, record->resource_id, record->resource_num,
									lvgl_res_scene_preload_default_cb_for_view, (void*)view_id,
									record->style_path, record->picture_path, record->text_path);
				}
				else
				{
					ret = lvgl_res_preload_scene_compact(scene_id, record->resource_id, record->resource_num,
									record->callback, (void*)view_id,
									record->style_path, record->picture_path, record->text_path);
				}
			}
			else if(!load_serial)
			{
				if(record->callback == NULL)
				{
					ret = lvgl_res_preload_scene_compact(scene_id, record->resource_id, record->resource_num,
									lvgl_res_scene_preload_default_cb_for_view, (void*)view_id,
									record->style_path, record->picture_path, record->text_path);
				}
				else
				{
					ret = lvgl_res_preload_scene_compact(scene_id, record->resource_id, record->resource_num,
									record->callback, (void*)view_id,
									record->style_path, record->picture_path, record->text_path);
				}
			}
			else
			{
				if(record->callback == NULL)
				{
					ret = lvgl_res_preload_scene_compact_serial(scene_id, record->resource_id, record->resource_num,
									lvgl_res_scene_preload_default_cb_for_view, (void*)view_id,
									record->style_path, record->picture_path, record->text_path);
				}
				else
				{
					ret = lvgl_res_preload_scene_compact_serial(scene_id, record->resource_id, record->resource_num,
									record->callback, (void*)view_id,
									record->style_path, record->picture_path, record->text_path);
				}
			}
		}
		record = record->next;
	}
#endif //CONFIG_RES_MANAGER_SKIP_PRELOAD
	return ret;
}

int lvgl_res_set_current_string_res(const uint8_t** string_res, uint32_t string_cnt)
{
	return res_manager_set_current_string_res(string_res, string_cnt);
}

uint8_t* lvgl_res_get_string(uint8_t* key)
{
	return res_manager_get_string(key);
}
