#include <string.h>
#include <Stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "res_struct.h"

#pragma pack (1)

#define SCENE_ITEM_SIZE 3

#define PACK

#define MAX_RES_DATA_SIZE	1024*1024


/*style file header*/
typedef struct _style_head
{
    uint32_t    magic;        /*0x18*/
    uint32_t    scene_sum;
    int8_t      reserve[8];
}style_head_t;

typedef struct _scene
{
    /*! scene x coordinate*/
    short  x;
    /*! scene y coordinate */
    short  y;
    /*! scene width  */
    short  width;
    /*! scene height */
    short  height;
    /*! scene background color*/
    unsigned int background;     
    /*! scene visible flag  */
    unsigned char  visible;
    /*! scene opaque flag */
    unsigned char   opaque;
    /*! scene transparency value */
    unsigned short  transparence;   
    /*! scene number*/
    unsigned int resource_sum;
    /*! offset of first child*/
    unsigned int child_offset;
    /*! scene direction: horizontal or vertical*/
	unsigned char direction;
    /*! reserved*/
    unsigned char keys[ 16 ];
}scene_t;

typedef struct resource_s
{
    /*! resource type*/
    uint8_t type;
    /*! resource hashed sty id */
    uint32_t id;
    /*! offset to next resource*/
    uint32_t offset;
}resource_t;

/*!
*\brief
    resource group structure
*/
typedef struct _resgroup_resource
{
    /*! resource base data*/
    resource_t   	 resource;

    /*! absolute x coordinate */
    int16_t  absolute_x;
    /*! absolute y coordinate */
    int16_t  absolute_y;
    /*! x coordinate relative to parent resource */
    int16_t  x;
    /*! y coordinate relative to parent resource */
    int16_t  y;

    /*! group width */
    int16_t  width;
    /*! group height */
    int16_t  height;
    /*! group background color */
	uint32_t	 background;
    /*! group visible flag  */
    uint8_t  visible;
    /*! group opaque flag */
    uint8_t   opaque;
    /*! group transparency value */
    uint16_t  transparence;

    /*! resource number in group*/
    uint32_t  	 resource_sum;
    /*! offset to first child*/
    uint32_t child_offset;
}resgroup_resource_t;

/*!
*\brief
    string resource structure
*/
typedef struct _string_resource
{
    /*! resource base data*/
    resource_t resource;
    /*! relative x coordinate to parent resource*/
    int16_t  x;
    /*! relative y coordinate to parent resource*/
    int16_t  y;
    /*! string area width */
    int16_t  width;
    /*! string area height */
    int16_t  height;
    /*! font color */
    uint32_t foreground;
    /*! string area background color(for non transparent text)*/
    uint32_t background;
    /*! string visible flag */
    uint8_t   visible;
    /*! text alignment*/
    uint8_t   text_align;
    /*! text mode(transparent or not) */
    uint8_t   mode;
    /*! font height/size */
    uint8_t   font_height;

    /*! res id in str file*/
	uint16_t str_id;
    /*! scroll flag*/
    uint8_t scroll;
    /*! scroll direction */
    int8_t direction;
    /*! space 2 scrolls */
    uint16_t space;
    /*! scroll step size */
    uint16_t pixel;
}string_resource_t;


/*!
*\brief
    picture resource structure
*/
typedef struct _picture_resource
{
	/*! resource base data*/
    resource_t   	 resource;
    int16_t  x;
    int16_t  y;
    int16_t  width;
    int16_t  height;
    uint8_t  visible;  //0x10=rgb £¬ 0x11=argb
    uint16_t pic_id;
}picture_resource_t;

/*!
*\brief
    picregion resource structure
*/
typedef struct _picregion_resource
{
    /*! resource base data*/
    resource_t  resource;
    /*! x coordinate relative to parent resource*/
    int16_t  x;
    /*! y coordinate relative to parent resource*/
    int16_t  y;
    /*! picregion width*/
    int16_t  width;
    /*! picregion height*/
    int16_t  height;
    /*! picregion visible flag*/
    uint8_t  visible;
    /*! total frame number*/
    uint16_t frames;
    /*! offset to first frame resource data*/
    uint32_t pic_offset;
}picregion_resource_t;

/*!
*\brief
    picregion frame data structure
*/
typedef struct _picregion_frame
{
    /*! id */
    uint32_t id;
    /*! frame id*/
    uint16_t  frame;
    /*! x coordinate relative to parent resource*/
    int16_t  x;
    /*! y coordinate relative to parent resource*/
    int16_t  y;
    /*! pic width */
    int16_t  width;
    /*! pic height */
    int16_t  height;
    /*! res id in pic res file*/
    uint16_t  pic_id;
}picregion_frame_t;

//resource file header
typedef struct
{
    uint8_t  magic[4];        //'R', 'E', 'S', 0x19
    uint16_t counts;
    uint16_t w_format;
    uint8_t brgb;
    uint8_t version;
	uint16_t start_id;
    uint8_t reserved[3];
    uint8_t ch_extend;
}res_head_t;

typedef struct
{
    uint32_t   offset;
    uint16_t   length;
    uint8_t    type;
    uint8_t    name[8];
    uint8_t    len_ext;
}res_entry_t;

typedef struct _history_item
{
	uint32_t id;
	uint16_t format;
	uint16_t bytes_per_pixel;
	uint32_t compress_size;
	uint32_t bmp_pos;
	struct _history_item* next;
}history_bitmap_data_t;

typedef struct
{
	uint32_t id;
	history_bitmap_data_t* ptr;
}history_index_t;

FILE* picfp;
uint32_t start_res_id = 0;
unsigned char res_data[MAX_RES_DATA_SIZE];
unsigned char old_data[MAX_RES_DATA_SIZE];
history_bitmap_data_t* history_data = NULL;
history_index_t* history_id_array = NULL;
uint32_t max_history_num = 0;

//to get style file info
static int _read_head(char* sty_data, resource_info_t* info)
{
    style_head_t* head;
    int ret;
	uint32_t sty_size;

	head = (style_head_t*)sty_data;
	info->sum = head->scene_sum;
    //FIXME
	info->scenes = (int32_t*)(sty_data + sizeof(style_head_t));

    return 0;
}

static int _instert_history_bitmap_data(sty_picture_t* bitmap)
{
	history_bitmap_data_t* item;
	history_bitmap_data_t* prev;
	
	if(bitmap == NULL)
	{
		printf("insert invalid history data\n");
		return -1;
	}

	item = history_data;
	prev = item;
	while(item != NULL)
	{
		if(bitmap->id == item->id)
		{
			//loaded
			return 0;			
		}
		else if(bitmap->id < item->id)
		{
			break;
		}

		prev = item;
		item = item->next;
	}

	history_bitmap_data_t* data = (history_bitmap_data_t*)malloc(sizeof(history_bitmap_data_t));
	data->id = bitmap->id;
	data->format = bitmap->format;
	data->compress_size = bitmap->compress_size;
	data->bytes_per_pixel = bitmap->bytes_per_pixel;
	data->bmp_pos = bitmap->bmp_pos;
	data->next = NULL;
	
	if(history_data == NULL)
	{
		history_data = data;
	}
	else if(item == prev)
	{
		//insert at list head
		data->next = item;
		history_data = data;
	}
	else
	{
		prev->next = data;
		data->next = item;
	}

	max_history_num++;
	return 0;
}

static int _load_history_picregion_bitmap_data(sty_picregion_t* picreg, uint8_t* data)
{
	int i;
	uint8_t* frame_data = (uint8_t*)picreg + sizeof(sty_picregion_t);
	sty_picture_t* bitmap;

	for(i=0;i<picreg->frames;i++)
	{
		bitmap = (sty_picture_t*)(frame_data + sizeof(sty_picture_t)*i);
		_instert_history_bitmap_data(bitmap);
	}
}

static int _load_history_group_bitmap_data(sty_group_t* group, uint8_t* data)
{
	int i;
	uint32_t* base_res;
	uint32_t offset;
	uint32_t child_offset = group->child_offset;
	printf("group 0x%x, sum %d\n", group->sty_id, group->resource_sum);
	for(i=0;i<group->resource_sum;i++)
	{
		base_res = (uint32_t*)(data + child_offset);
		offset = *(base_res+2);
		switch(*base_res)
		{
		case RESOURCE_TYPE_GROUP:
			{
				sty_group_t* group = (sty_group_t*)base_res;
				_load_history_group_bitmap_data(group, data);
			}
			break;
		case RESOURCE_TYPE_PICREGION:
			{
				sty_picregion_t* picreg = (sty_picregion_t*)base_res;
				_load_history_picregion_bitmap_data(picreg, data);
			}
			break;
		case RESOURCE_TYPE_PICTURE:
			{
				sty_picture_t* bitmap = (sty_picture_t*)base_res;
				_instert_history_bitmap_data(bitmap);
			}
			break;
		case RESOURCE_TYPE_TEXT: 
		default:
			break;
		}
		child_offset = offset;
	}
}


static int _load_history_bitmap_data(uint8_t* data, resource_info_t* info)
{
	resource_scene_t* scenes;
	int i,k;
	uint32_t* base_res;
	uint32_t offset;
	uint32_t res_offset = 0;
	uint8_t* child_data = data + sizeof(resource_scene_t)*info->sum;
	uint8_t* scene_data;

	printf("%s %d\n", __FILE__, __LINE__);
	scenes = (resource_scene_t*)data;
	for(i=0;i<info->sum;i++)
	{
		printf("scene 0x%x sum %d\n", scenes[i].scene_id, scenes[i].resource_sum);
		scene_data = scenes[i].child_offset + child_data;
		for(k=0;k<scenes[i].resource_sum;k++)
		{
			base_res = (uint32_t*)scene_data;
			offset = *(base_res+2);
			//printf("res type %d\n", *base_res);
			switch(*base_res)
			{
			case RESOURCE_TYPE_GROUP:
				{
					sty_group_t* group = (sty_group_t*)base_res;
					_load_history_group_bitmap_data(group, child_data);
				}
				break;
			case RESOURCE_TYPE_PICREGION:
				{
					sty_picregion_t* picreg = (sty_picregion_t*)base_res;
					_load_history_picregion_bitmap_data(picreg, child_data);
				}
				break;
			case RESOURCE_TYPE_PICTURE:
				{
					sty_picture_t* bitmap = (sty_picture_t*)base_res;
					_instert_history_bitmap_data(bitmap);
				}
				break;
			case RESOURCE_TYPE_TEXT:
			default:
				break;
			}
			scene_data = child_data+offset;
		}
		
	}

	history_id_array = (history_index_t*)malloc(sizeof(history_index_t)*max_history_num);
	if(history_id_array == NULL)
	{
		printf("no memory for history id array\n");
		return -1;
	}
	history_bitmap_data_t* item;
	item = history_data;
	i=0;
	while(item)
	{
		history_id_array[i].id = item->id;
		history_id_array[i].ptr = item;
		i++;
		item = item->next;
	}
	
	return 0;
}

static int _search_history_bitmap_data(char* sty_data, sty_picture_t* pic)
{
	int i;
	int low = 0;
	int high = max_history_num-1;
	int mid = high/2;
	uint32_t mid_id = history_id_array[mid].id;
	history_bitmap_data_t* item;
	
	while(low <= high)
	{
		mid_id = history_id_array[mid].id;
		if(pic->id < mid_id)
		{
			high = mid - 1;			
			mid = low + (high - low)/2;
		}
		else if(pic->id > mid_id)
		{
			low = mid + 1;
			mid = low + (high - low)/2;
		}
		else
		{
			//found
			item = history_id_array[mid].ptr;
			pic->compress_size = item->compress_size;
			pic->bytes_per_pixel = item->bytes_per_pixel;
			pic->format = item->format;
			pic->bmp_pos = item->bmp_pos;
			return 0;
		}
	}

	//not found;
	printf("sty 0x%x, id %d, not found in history data\n", pic->sty_id, pic->id);
	return -1;
}


static int _parse_bitmap(char* sty_data, sty_picture_t* pic)
{
	int ret;
	res_entry_t res_entry;
	uint32_t id_diff = pic->id - start_res_id;

	if(id_diff < 0)
	{
		return _search_history_bitmap_data(sty_data, pic);
	}
		

	memset(&res_entry, 0, sizeof(res_entry_t));
	fseek(picfp, id_diff * sizeof(res_entry_t) , SEEK_SET);
	ret = fread(&res_entry, 1, sizeof(res_entry_t), picfp);
	if (ret < sizeof(res_entry_t))
	{
		printf("cant find sty_id 0x%x\n", pic->sty_id);
		return -1;
	}

	switch (res_entry.type)
	{
	case RES_TYPE_PNG:
		pic->bytes_per_pixel = 3;
		pic->format = RESOURCE_BITMAP_FORMAT_ARGB8565;
		break;
	case RES_TYPE_PIC:
	case RES_TYPE_LOGO:
	case RES_TYPE_PIC_LZ4:
		pic->bytes_per_pixel = 2;
		pic->format = RESOURCE_BITMAP_FORMAT_RGB565;
		break;
	case RES_TYPE_PIC_RGB888:
		pic->bytes_per_pixel = 3;
		pic->format = RESOURCE_BITMAP_FORMAT_RGB888;
		break;
	case RES_TYPE_PIC_ARGB8888:
	case RES_TYPE_PIC_ARGB8888_LZ4:
		pic->bytes_per_pixel = 4;
		pic->format = RESOURCE_BITMAP_FORMAT_ARGB8888;
		break;
	case RES_TYPE_PNG_COMPRESSED:
	case RES_TYPE_PIC_COMPRESSED:
	case RES_TYPE_LOGO_COMPRESSED:
	case RES_TYPE_PIC_RGB888_COMPRESSED:
	case RES_TYPE_PIC_ARGB8888_COMPRESSED:
	default:
		printf("not supported sty_id 0x%x, id %d\n", pic->sty_id, pic->id);
		return -1;
	}

    if((res_entry.type == RES_TYPE_PIC_LZ4) || (res_entry.type == RES_TYPE_PIC_ARGB8888_LZ4))
    {
		pic->compress_size = ((res_entry.len_ext << 16) | res_entry.length) - 4;
    }	
    else
    {
    	pic->compress_size = 0;
    }
	pic->bmp_pos = res_entry.offset+4;
//	printf("pic 0x%x, pos %d, bpp %d\n", pic->sty_id, pic->bmp_pos, pic->bytes_per_pixel);
	return 0;
}

static int _parse_group(char* sty_data, int* pgroup_offset, resgroup_resource_t* group, unsigned char* res_data, int* pres_offset, uint32_t magic)
{
	int res_offset = *pres_offset;
	int group_offset = *pgroup_offset;
	unsigned char* buf;
	resource_t* resource;
	int count = 0;
	int i, j;
	int group_size;
	int bpp = 0;

	group_offset += group->child_offset;
	buf = (uint8_t*)sty_data + group_offset;
	resource = (resource_t*)buf;
	printf("group res sum %d\n", group->resource_sum);
	for(i=0;i<group->resource_sum;i++)
	{
	    if(resource->type == RESOURCE_TYPE_GROUP)
	    {
			//buf is now on group resource start
	    	sty_group_t* res = (sty_group_t*)(res_data + res_offset);
			resgroup_resource_t* group = (resgroup_resource_t*)resource;
			res->type = (uint32_t)resource->type;
			res->sty_id = (uint32_t)resource->id;
			res->x = (int16_t)group->x;
			res->y = (int16_t)group->y;
			res->width = (uint16_t)group->width;
			res->height = (uint16_t)group->height;
			res->backgroud = (uint32_t)group->background;
			res->opaque = (uint32_t)group->transparence;
			res->resource_sum = (uint32_t)group->resource_sum;
			
			res_offset += sizeof(sty_group_t);
			res->child_offset = (uint32_t)res_offset;

			magic += res->sty_id;
			printf("%d magic 0x%x\n", __LINE__, magic);
			_parse_group(sty_data, &group_offset, group, res_data, &res_offset, magic);
			res->offset = res_offset;
			magic -= res->sty_id;
	    }

	    if(resource->type == RESOURCE_TYPE_PICTURE)
	    {
	    	//buf is now on resource start
	    	sty_picture_t* res = (sty_picture_t*)(res_data + res_offset);
			picture_resource_t* pic = (picture_resource_t*)resource;
			res->type = (uint32_t)resource->type;
			res->sty_id = resource->id;
			res->x = (int16_t)pic->x;
			res->y = (int16_t)pic->y;
			res->width = (uint16_t)pic->width;
			res->height = (uint16_t)pic->height;
			res->format = (uint16_t)pic->visible;
			res->id = (uint32_t)pic->pic_id;
			res->magic = magic + res->sty_id;
			printf("0x%x, 0x%x\n", magic, res->sty_id);
			printf("%d magic 0x%x\n", __LINE__, res->magic);
			res_offset += sizeof(sty_picture_t);
			res->offset = res_offset;
			_parse_bitmap(sty_data, res);
	    }

	    if(resource->type == RESOURCE_TYPE_TEXT)
	    {
	    	//buf is now on resource start
	    	sty_string_t* res = (sty_string_t*)(res_data + res_offset);
			string_resource_t* str = (string_resource_t*)resource;
			res->type = (uint32_t)resource->type;
			res->sty_id = resource->id;
			res->x = (int16_t)str->x;
			res->y = (int16_t)str->y;
			res->width = (uint16_t)str->width;
			res->height = (uint16_t)str->height;
			res->color = (uint32_t)str->foreground;
			res->bgcolor = (uint32_t)str->background;
			res->align = (uint16_t)str->text_align;
			res->font_size = (uint16_t)str->font_height;
			res->id = (uint32_t)str->str_id;

			res_offset += sizeof(sty_string_t);
			res->offset = res_offset;
	    }

		if(resource->type == RESOURCE_TYPE_PICREGION)
		{
			sty_picregion_t* res = (sty_picregion_t*)(res_data + res_offset);
			picregion_resource_t* picreg = (picregion_resource_t*)resource;
			res->type = (uint32_t)resource->type;
			res->sty_id = resource->id;
			res->x = (int16_t)picreg->x;
			res->y = (int16_t)picreg->y;
			res->width = (uint16_t)picreg->width;
			res->height = (uint16_t)picreg->height;
			res->format = (uint16_t)picreg->visible;
			res->frames = (uint32_t)picreg->frames;

			res_offset += sizeof(sty_picregion_t);
			res->id_offset = res_offset;
			uint8_t* tmp = (res_data + res_offset);
			sty_picture_t* bitmap = (sty_picture_t*)tmp;
			buf = (char*)resource + picreg->pic_offset;
			printf("11111\n");
			for(j = 0; j < res->frames; j++)
			{
				bitmap->type = RESOURCE_TYPE_PICTURE;
				bitmap->sty_id = res->sty_id;
				res_offset += sizeof(sty_picture_t);
				bitmap->offset = res_offset;
				bitmap->x = res->x;
				bitmap->y = res->y;
				bitmap->width = res->width;
				bitmap->height = res->height;
				bitmap->id = (uint32_t)(buf[1]<<8 | buf[0]);
				bitmap->magic = magic + res->sty_id;
				_parse_bitmap(sty_data, bitmap);
				if(bpp == 0)
				{
					bpp = bitmap->bytes_per_pixel;
				}
				printf("w %d h%d\n", bitmap->width, bitmap->height);
				buf += 2;
				bitmap = (sty_picture_t*)(res_data + res_offset);
			}
			res->bytes_per_pixel = bpp;
			printf("22222\n");
			res->offset = res_offset;
			res->magic = magic + res->sty_id;
			printf("%d magic 0x%x\n", __LINE__, res->magic);

		}	

		group_offset += resource->offset;
		buf = (uint8_t*)sty_data + group_offset;
		resource = (resource_t*)buf;
	}

	*pres_offset = res_offset;
	return 0;	

}

static int _parse_scene(char* sty_data, int scene_offset, scene_t* scene, unsigned char* res_data, int* pres_offset, uint32_t magic)
{
	int res_offset = *pres_offset;
	unsigned char* buf;
	resource_t* resource;
	int count = 0;
	int i, j;
	int group_size;
	int buf_offset;
	int bpp = 0;

	buf_offset = scene_offset + scene->child_offset;
	buf = (unsigned char*)sty_data + buf_offset;
	resource = (resource_t*)buf;
	printf("scene res sum %d\n", scene->resource_sum);
	for(i=0;i<scene->resource_sum;i++)
	{
	    if(resource->type == RESOURCE_TYPE_GROUP)
	    {
			//buf is now on group resource start
	    	sty_group_t* res = (sty_group_t*)(res_data + res_offset);
			resgroup_resource_t* group = (resgroup_resource_t*)resource;
			res->type = (uint32_t)resource->type;
			res->sty_id = (uint32_t)resource->id;
			res->x = (int16_t)group->x;
			res->y = (int16_t)group->y;
			res->width = (uint16_t)group->width;
			res->height = (uint16_t)group->height;
			res->backgroud = (uint32_t)group->background;
			res->opaque = (uint32_t)group->transparence;			
			res->resource_sum = (uint32_t)group->resource_sum;
			
			res_offset += sizeof(sty_group_t);
			res->child_offset = (uint32_t)res_offset;
			printf("buf_offset 0x%x, group child offset 0x%x, sum %d\n", buf_offset, group->child_offset, group->resource_sum);
			printf("group 0x%x, 0x%x, 0x%x, 0x%x, 0x%x\n",group->absolute_x, group->absolute_y, group->x, group->y, group->visible, group->height);
			magic += res->sty_id;
			printf("%d magic 0x%x\n", __LINE__, magic);
			_parse_group(sty_data, &buf_offset, group, res_data, &res_offset, magic);
			res->offset = res_offset;
			magic -= res->sty_id;
	    }

	    if(resource->type == RESOURCE_TYPE_PICTURE)
	    {
	    	//buf is now on resource start
	    	sty_picture_t* res = (sty_picture_t*)(res_data + res_offset);
			picture_resource_t* pic = (picture_resource_t*)resource;
			res->type = (uint32_t)resource->type;
			res->sty_id = resource->id;
			res->x = (int16_t)pic->x;
			res->y = (int16_t)pic->y;
			res->width = (uint16_t)pic->width;
			res->height = (uint16_t)pic->height;
			res->format = (uint16_t)pic->visible;
			res->id = (uint32_t)pic->pic_id;
			_parse_bitmap(sty_data, res);
			res_offset += sizeof(sty_picture_t);
			res->offset = res_offset;
			res->magic = magic + res->sty_id;
			printf("0x%x, 0x%x\n", magic, res->sty_id);
			printf("%d magic 0x%x\n", __LINE__, res->magic);
			
	    }

	    if(resource->type == RESOURCE_TYPE_TEXT)
	    {
	    	//buf is now on resource start
	    	sty_string_t* res = (sty_string_t*)(res_data + res_offset);
			string_resource_t* str = (string_resource_t*)resource;
			res->type = (uint32_t)resource->type;
			res->sty_id = resource->id;
			res->x = (int16_t)str->x;
			res->y = (int16_t)str->y;
			res->width = (uint16_t)str->width;
			res->height = (uint16_t)str->height;
			res->color = (uint32_t)str->foreground;
			res->bgcolor = (uint32_t)str->background;
			res->align = (uint16_t)str->text_align;
			res->font_size = (uint16_t)str->font_height;
			res->id = (uint32_t)str->str_id;

			res_offset += sizeof(sty_string_t);
			res->offset = res_offset;
	    }

		if(resource->type == RESOURCE_TYPE_PICREGION)
		{
			sty_picregion_t* res = (sty_picregion_t*)(res_data + res_offset);
			picregion_resource_t* picreg = (picregion_resource_t*)resource;
			res->type = (uint32_t)resource->type;
			res->sty_id = resource->id;
			res->x = (int16_t)picreg->x;
			res->y = (int16_t)picreg->y;
			res->width = (uint16_t)picreg->width;
			res->height = (uint16_t)picreg->height;
			res->format = (uint16_t)picreg->visible;
			res->frames = (uint32_t)picreg->frames;

			res_offset += sizeof(sty_picregion_t);
			res->id_offset = res_offset;
			uint8_t* tmp = (res_data + res_offset);
			sty_picture_t* bitmap = (sty_picture_t*)tmp;
			buf = (char*)resource + picreg->pic_offset;
			printf("33333: 0x%x, id offset 0x%x\n", resource->id, res->id_offset);
			for(j = 0; j < res->frames; j++)
			{
				bitmap->type = RESOURCE_TYPE_PICTURE;
				bitmap->sty_id = res->sty_id;
				res_offset += sizeof(sty_picture_t);
				bitmap->offset = res_offset;
				bitmap->x = res->x;
				bitmap->y = res->y;
				bitmap->width = res->width;
				bitmap->height = res->height;
				bitmap->format = res->format;
				bitmap->id = (uint32_t)(buf[1]<<8 | buf[0]);
				bitmap->magic = magic + res->sty_id;
				_parse_bitmap(sty_data, bitmap);
				if(bpp == 0)
				{
					bpp = bitmap->bytes_per_pixel;
				}
				printf("w %d h%d\n", bitmap->width, bitmap->height);
				buf += 2;
				bitmap = (sty_picture_t*)(res_data + res_offset);
			}
			res->bytes_per_pixel = bpp;
			res->offset = res_offset;
			res->magic = magic + res->sty_id;
			printf("%d magic 0x%x\n", __LINE__, res->magic);
			
			printf("44444: offset 0x%x, id_offset 0x%x\n", res->offset, res->id_offset);
			
		}	

		buf_offset += resource->offset;
		buf = sty_data + buf_offset;
		resource = (resource_t*)buf;
	}

	*pres_offset = res_offset;
	return 0;
}

int main(int argc, char* argv[])
{
	FILE* oldp = NULL;
    FILE* orip = NULL;
    FILE* newp = NULL;
    unsigned char* sty_data;
    int res_size;
	int res_offset;
	resource_info_t old_info;
    resource_info_t info;
    resource_scene_t* scenes;
	int i;
	unsigned int* scene_item;
    uint32_t magic;
    int sty_size;
	int old_size;
	res_head_t res_head;
    int ret;

	printf("%s %d\n\n", __FILE__, __LINE__);

    if(argc < 5)
    {
        printf("usage: sty_conv_diff [old_convd_sty] [new_ori_sty] [new_pic_res] [new_convd_sty]");
        return 0;
    }


	printf("%s %d\n", __FILE__, __LINE__);
    oldp = fopen(argv[1], "rb");
    if(oldp == NULL)
    {
        printf("open old sty %s faild \n", argv[1]);
        return -1;
    }

	
    orip = fopen(argv[2], "rb");
    if(orip == NULL)
    {
        printf("open original new sty %s faild \n", argv[2]);
		fclose(oldp);
        return -1;
    }

	picfp = fopen(argv[3], "rb");
    if(picfp == NULL)
    {
        printf("open pic res %s faild \n", argv[3]);
		fclose(oldp);
		fclose(orip);
        return -1;
    }


    fseek(orip, 0, SEEK_END);
    sty_size = ftell(orip);

    sty_data = (unsigned char*)malloc(sty_size);
	fseek(orip, 0, SEEK_SET);
    ret = fread(sty_data, 1, sty_size, orip);
    if(ret < sty_size)
    {
        printf("read sty %s failed\n", argv[1]);
        free(sty_data);
        fclose(orip);
		fclose(picfp);
		fclose(oldp);
        return -1;
    }
    fclose(orip);

	printf("%s %d\n", __FILE__, __LINE__);
	//read info of old convd sty data
	fseek(oldp, 0, SEEK_END);
	old_size = ftell(oldp);

	fseek(oldp, 0, SEEK_SET);
	fread(&old_info, 1, sizeof(resource_info_t), oldp);
	printf("%s %d\n", __FILE__, __LINE__);
	ret = fread(old_data, 1, old_size-sizeof(resource_info_t), oldp);
	if(ret < old_size-sizeof(resource_info_t))
	{
        printf("read sty %s failed\n", argv[1]);
        free(sty_data);
		fclose(picfp);
		fclose(oldp);		
        fclose(orip);
        return -1;		
	}

	fread(&res_head, 1, sizeof(res_head_t), picfp);
	start_res_id = res_head.start_id;
	
	printf("%s %d\n", __FILE__, __LINE__);
	_load_history_bitmap_data(old_data, &old_info);
	printf("max_history_num %d\n", max_history_num);	

	int k;
	for(k=0;k<max_history_num;k++)
	{
		printf("history id %d, bmp_pos %d\n", history_id_array[k].ptr->id, history_id_array[k].ptr->bmp_pos);
	}

	return 0;


    memset(&info, 0, sizeof(resource_info_t));
    _read_head(sty_data, &info);
	scenes = (resource_scene_t*)malloc(sizeof(resource_scene_t)*info.sum);
	
    res_size = 0;
	res_offset = 0;
	memset(res_data, 0, MAX_RES_DATA_SIZE);
	scene_item = info.scenes;
	printf("scene sum %d\n", info.sum);
	
    for(i=0;i<info.sum;i++)
    {
    	uint32_t scene_id = *scene_item;
		unsigned int offset = *( scene_item + 1 );
		unsigned int size = *( scene_item + 2 );
		scene_t* scene = (scene_t*)(sty_data+offset);
		scenes[i].scene_id = scene_id;
		scenes[i].x = scene->x;
		scenes[i].y = scene->y;
		scenes[i].background = scene->background;
		scenes[i].width = scene->width;
		scenes[i].height = scene->height;
		scenes[i].resource_sum = scene->resource_sum;
		scenes[i].direction = scene->direction;
		scenes[i].visible = scene->visible;
		scenes[i].transparence = scene->transparence;
		scenes[i].opaque = scene->opaque;

		printf("scene_id 0x%x, offset 0x%x, size %d\n", scene_id, offset, size);
		scenes[i].child_offset = res_offset;
		magic = scene_id;
		printf("%d magic 0x%x\n", magic);
        _parse_scene(sty_data, offset, scene, res_data, &res_offset, magic);
		scene_item = scene_item + SCENE_ITEM_SIZE;
    }

	if(argc == 5)
	{
		newp = fopen(argv[4], "wb");
	}
	else
	{
    	newp = fopen(argv[2], "wb");
	}
    if(newp == NULL)
    {
        free(scenes);
        free(sty_data);
        return -1;
    }

    fwrite(&info, 1, sizeof(resource_info_t), newp);
    fwrite(scenes, 1, sizeof(resource_scene_t)*info.sum, newp);
    fwrite(res_data, 1, res_offset, newp);
    fclose(newp);
	fclose(picfp);
	
	free(scenes);
	free(sty_data);
    return 0;
}
