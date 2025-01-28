#ifdef _NATIVELVGL
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "res_struct.h"
#include <sys/types.h>
#ifdef _MSC_VER
#include <windows.h>
#else
#include <dirent.h>
#endif

#pragma pack (1)

#define SCENE_ITEM_SIZE 3

#define PACK

#define MAX_RES_DATA_SIZE	1024*1024
#define MAX_DIFF_PATH_LEN	256

#ifdef _MSC_VER
wchar_t* charTowchar(const char* str)
{
	int length = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, str, -1, NULL, 0);
	if (length == 0)
		return NULL;

	wchar_t* res = (wchar_t*)malloc(sizeof(wchar_t) * (length + 1));

	MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, str, -1, res, length);
	res[length] = L'\0';

	return res;
}

char* wcharTochar(const wchar_t* str)
{
	int length = WideCharToMultiByte(CP_ACP, 0, str, -1, NULL, 0, NULL, NULL);
	if (length == 0)
		return NULL;

	char* res = (char*)malloc(sizeof(char) * (length + 1));

	WideCharToMultiByte(CP_ACP, 0, str, -1, res, length, NULL, NULL);
	res[length] = '\0';

	return res;
}
#endif

typedef struct _style_head
{
    uint32_t    magic;        /*0x18*/
    uint32_t    scene_sum;
    int8_t      reserve[8];
}style_head_t;

typedef struct _scene
{

    short  x;

    short  y;

    short  width;

    short  height;

    unsigned int background;     

    unsigned char  visible;

    unsigned char   opaque;

    unsigned short  transparence;
    
    unsigned int resource_sum;

    unsigned int child_offset;

    unsigned char direction;
    
    unsigned char keys[ 16 ];
}scene_t;

typedef struct resource_s
{

    uint8_t type;

    uint32_t id;

    uint32_t offset;
}resource_t;


typedef struct _resgroup_resource
{

    resource_t   	 resource;

    int16_t  absolute_x;

    int16_t  absolute_y;

    int16_t  x;

    int16_t  y;

    int16_t  width;

    int16_t  height;

    uint32_t	 background;

    uint8_t  visible;

    uint8_t   opaque;

    uint16_t  transparence;

    uint32_t  	 resource_sum;

    uint32_t child_offset;
}resgroup_resource_t;

typedef struct _string_resource
{

    resource_t   	 resource;

    int16_t  x;

    int16_t  y;

    int16_t  width;

    int16_t  height;

    uint32_t  	 foreground;

    uint32_t	 background;

    uint8_t   visible;

    uint8_t   text_align;

    uint8_t   mode;

    uint16_t   font_height;

    uint16_t str_id;

    uint8_t scroll;

    int8_t direction;

    uint16_t space;

    uint16_t pixel;
}string_resource_t;


typedef struct _picture_resource
{
    resource_t   	 resource;
    int16_t  x;
    int16_t  y;
    int16_t  width;
    int16_t  height;
    uint8_t  visible;  //0x10=rgb ?��? 0x11=argb
    uint16_t pic_id;
}picture_resource_t;

typedef struct _picregion_resource
{

    resource_t   	 resource;

    int16_t  x;

    int16_t  y;

    int16_t  width;

    int16_t  height;

    uint8_t   visible;

    uint16_t 	 frames;

    uint32_t pic_offset;
}picregion_resource_t;

typedef struct _picregion_frame
{

    uint32_t id;

    uint16_t  frame;

    int16_t  x;

    int16_t  y;

    int16_t  width;

    int16_t  height;

    uint16_t  pic_id;
}picregion_frame_t;

typedef struct
{
    uint8_t  magic[4];        //'R', 'E', 'S', 0x19
    uint16_t counts;
    uint16_t w_format;
    uint8_t brgb;
    uint8_t version;
	uint16_t id_start;
	uint8_t reserved;
	uint8_t tile_compress;
	uint8_t compressed_type;
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

typedef struct
{
	/* data */
	uint8_t node_name[128];
	uint32_t hash;
}node_hash_item_t;



FILE* picfp = NULL;
FILE* picfp_diff = NULL;
uint32_t basic_id_end = 0;
uint32_t diff_id_start = 0;
FILE* stybitmapfp = NULL;
FILE* stystringfp = NULL;

node_hash_item_t* node_list = NULL;
uint32_t node_cnt = 0;
uint8_t arch_nodes[2048] = {0};
int32_t node_tail = 0;

FILE* includefp = NULL;
uint8_t include_path[256] = {0};

static void _generate_bitmap_code(FILE* fp, sty_picture_t* bitmap);

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

static int _parse_header_nodes(uint8_t* header)
{
	char* nread;
	uint8_t inbuf[256] = {0};
	int i = 0;
	FILE* fp = NULL;
	node_cnt = 0;

	fp = fopen(header, "rb");
	if(fp == NULL)
	{
		printf("header %s open failed\n", header);
		return -1;
	}

	fseek(fp, 0, SEEK_SET);
	nread = fgets(inbuf, 256, fp);
	while(nread != NULL)
	{
		if(strstr(inbuf, "#define"))
		{
			node_cnt++;
		}
		
		memset(inbuf, 0, 256);
		nread = fgets(inbuf, 256, fp);
	}

	if(node_list)
	{
		free(node_list);
		node_list = NULL;
	}
	node_list = (node_hash_item_t*)malloc(node_cnt*sizeof(node_hash_item_t));
	if(!node_list)
	{
		printf("malloc header hash node list failed\n");
		fclose(fp);
		return -1;
	}

	fseek(fp, 0, SEEK_SET);
	memset(inbuf, 0, 256);
	nread = fgets(inbuf, 256, fp);
	while(nread != NULL)
	{
		if(strstr(inbuf, "#define"))
		{
			sscanf(inbuf, "#define  %s    0x%x", node_list[i].node_name, &node_list[i].hash);
			i++;
		}

		memset(inbuf, 0, 256);
		nread = fgets(inbuf, 256, fp);
	}

	fclose(fp);
	return 0;
}


static int _push_arch_node(uint32_t hash, int frame_id)
{
	uint8_t* node = NULL;

	int i;
	for(i=0;i<node_cnt;i++)
	{
		if(hash == node_list[i].hash)
		{
			node = node_list[i].node_name;
		}
	}

	if(!node)
	{
		return -1;
	}

	if(frame_id > 0)
	{
		//has frame id means it's a pic region, use frame id as an extension to distinguish between frames
		uint8_t ext[8] = {0};
		sprintf(ext, "_%d", frame_id);
		strcpy(&arch_nodes[node_tail], node);
		strcat(&arch_nodes[node_tail], ext);
		node_tail = node_tail + strlen(node) + strlen(ext) + 1;
	}
	else
	{
		strcpy(&arch_nodes[node_tail], node);
		node_tail = node_tail + strlen(node) + 1;
	}


	return 0;
}

static int _pop_arch_node(void)
{
	if(node_tail == 0)
	{
		return 0;
	}

	node_tail -= 2;
	//find previous node tail
	while(node_tail >= 0 && arch_nodes[node_tail] != 0)
	{
		arch_nodes[node_tail] = 0;
		node_tail--; 
	}

	if(node_tail > 0)
	{
		node_tail++;//move to next start of arch node
	}
	else
	{
		node_tail = 0;
	}

	return 0;
}

static int _get_arch_name(uint8_t* buf, int buf_len)
{
	int32_t off = 0;
	int32_t len = 0;

	memset(buf, 0, buf_len);
	while(off < node_tail)
	{
		len = strlen(&arch_nodes[off]);
		if(off > 0)
		{
			strcat(buf, "_");
		}
		strcat(buf, &arch_nodes[off]);
		off = off + len + 1;
	}

	return 0;
}

static int _parse_bitmap(char* sty_data, sty_picture_t* pic)
{
	int ret;
	res_entry_t res_entry;

	memset(&res_entry, 0, sizeof(res_entry_t));
	if(pic->id <= basic_id_end)
	{
		fseek(picfp, pic->id * sizeof(res_entry_t) , SEEK_SET);
		ret = fread(&res_entry, 1, sizeof(res_entry_t), picfp);
	}
	else
	{
		if(picfp_diff)
		{
			fseek(picfp_diff, (pic->id - diff_id_start + 1)*sizeof(res_entry_t), SEEK_SET);
			ret = fread(&res_entry, 1, sizeof(res_entry_t), picfp_diff);
		}
	}
	if (ret < sizeof(res_entry_t))
	{
		printf("cant find sty_id 0x%x\n", pic->sty_id);
		return -1;
	}

	printf("bmp type %d\n", res_entry.type);
	switch (res_entry.type)
	{
	case RES_TYPE_PNG:
	case RES_TYPE_ARGB565:
	case RES_TYPE_ARGB565_LZ4:
		pic->bytes_per_pixel = 3;
		pic->format = RESOURCE_BITMAP_FORMAT_ARGB8565;
		break;
	case RES_TYPE_PIC:
	case RES_TYPE_LOGO:
	case RES_TYPE_RGB565_LZ4:
		pic->bytes_per_pixel = 2;
		pic->format = RESOURCE_BITMAP_FORMAT_RGB565;
		break;
	case RES_TYPE_PIC_ARGB1555:
	case RES_TYPE_ARGB1555_LZ4:
		pic->bytes_per_pixel = 2;
		pic->format = RESOURCE_BITMAP_FORMAT_ARGB1555;		
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
	case RES_TYPE_A8:
	case RES_TYPE_A8_LZ4:
		pic->bytes_per_pixel = 1;
		pic->format = RESOURCE_BITMAP_FORMAT_A8;		
		break;
	case RES_TYPE_ARGB6666:
	case RES_TYPE_ARGB6666_LZ4:
		pic->bytes_per_pixel = 3;
		pic->format = RESOURCE_BITMAP_FORMAT_ARGB6666;		
		break;
	case RES_TYPE_JPEG:
		pic->bytes_per_pixel = 2;
		pic->format = RESOURCE_BITMAP_FORMAT_JPEG;
		break;		
	case RES_TYPE_TILE_COMPRESSED:
		pic->bytes_per_pixel = 4;
		pic->format = RESOURCE_BITMAP_FORMAT_RAW;
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

    if((res_entry.type == RES_TYPE_RGB565_LZ4) || (res_entry.type == RES_TYPE_PIC_ARGB8888_LZ4) || 
		(res_entry.type == RES_TYPE_A8_LZ4) || (res_entry.type == RES_TYPE_ARGB6666_LZ4) ||
		(res_entry.type == RES_TYPE_JPEG) || (res_entry.type == RES_TYPE_TILE_COMPRESSED) ||
		(res_entry.type == RES_TYPE_ARGB565_LZ4) || (res_entry.type == RES_TYPE_ARGB1555_LZ4))
    {
		pic->compress_size = ((res_entry.len_ext << 16) | res_entry.length) - 4;
    }	
    else
    {
    	pic->compress_size = 0;
    }
	pic->bmp_pos = res_entry.offset+4;
	printf("pic bpp %d\n", pic->bytes_per_pixel);

	_generate_bitmap_code(stybitmapfp, pic);
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

			_push_arch_node(res->sty_id, 0);
			_parse_group(sty_data, &group_offset, group, res_data, &res_offset, magic);
			_pop_arch_node();

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

			_push_arch_node(res->sty_id, 0);
			_parse_bitmap(sty_data, res);
			_pop_arch_node();
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

				_push_arch_node(res->sty_id, j+1);
				_parse_bitmap(sty_data, bitmap);
				_pop_arch_node();

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

			_push_arch_node(res->sty_id, 0);
			_parse_group(sty_data, &buf_offset, group, res_data, &res_offset, magic);
			_pop_arch_node();

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

			_push_arch_node(res->sty_id, 0);
			_parse_bitmap(sty_data, res);
			_pop_arch_node();

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

				_push_arch_node(res->sty_id, j+1);
				_parse_bitmap(sty_data, bitmap);
				_pop_arch_node();

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

static void _generate_bitmap_head(FILE* fp, uint8_t* file_name)
{
	uint8_t inbuf[1024] = {0};	
	uint8_t outbuf[2048] = {0};

	sprintf(inbuf, "#include <res_manager_api.h> \n");
	strcat(outbuf, inbuf);

	memset(inbuf, 0, 1024);
	sprintf(inbuf, "#include <lvgl/lvgl.h> \n\n");
	strcat(outbuf, inbuf);

	memset(inbuf, 0, 1024);
	sprintf(inbuf, "static res_bin_info_t res_info = {.res_path=CONFIG_RES_MANAGER_RES_DISK\"/%s\",}; \n\n", file_name);
	strcat(outbuf, inbuf);


	fwrite(outbuf, 1, strlen(outbuf), fp);
}

static void _generate_bitmap_code(FILE* fp, sty_picture_t* bitmap)
{
	uint8_t outbuf[1024] = {0};
	uint8_t inbuf[1024] = {0};
	uint32_t in_len;
	uint32_t data_size = 0;
	uint8_t data[1024] = {0};
	uint8_t bitmap_name[1024] = {0};
	uint8_t cf[32] = {0};

	_get_arch_name(bitmap_name, 1024);
	sprintf(inbuf, "style_bitmap_t bitmap_%s = { \n", bitmap_name);
	strcat(outbuf, inbuf);

	if(bitmap->bytes_per_pixel == 4)
	{
		strcpy(cf, "LV_IMG_CF_RAW_ALPHA");
	}
	else
	{
		strcpy(cf, "LV_IMG_CF_RAW");
	}

	memset(inbuf, 0, 1024);
	sprintf(inbuf, " \
					.type = %d, \n \
					.sty_id = 0x%x,	\n \
					.id = %d, \n \
					.x = %d, \n \
					.y = %d, \n \
					.width = %d, \n \
					.height = %d, \n \
					.bytes_per_pixel = %d, \n \
					.format = %d, \n \
					.bmp_pos = %d, \n \
					.compress_size = %d, \n \
					.magic = %d, \n \
					.buffer = NULL, \n \
					.res_info = &res_info, \n \
				}; \n",
			bitmap->type,
			bitmap->sty_id,
			bitmap->id,
			bitmap->x,
			bitmap->y,
			bitmap->width,
			bitmap->height,
			bitmap->bytes_per_pixel,
			bitmap->format,
			bitmap->bmp_pos,
			bitmap->compress_size,
			bitmap->magic);
	strcat(outbuf, inbuf);


	memset(inbuf, 0, 1024);
	sprintf(inbuf, "lv_image_dsc_t img_%s = { \n", bitmap_name);
	strcat(outbuf, inbuf);

	sprintf(data, "&bitmap_%s", bitmap_name);
	memset(inbuf, 0, 1024);
	sprintf(inbuf, " \
					.data_size = 44, \n \
					.data = (uint8_t*)%s, \n \
					.header = { \n \
						.w = %d,\n \
						.h = %d, \n \
						.cf = %s, \n \
						.always_zero = 0,}, \n \
				}; \n\n", 
			data,
			bitmap->width,
			bitmap->height,
			cf);
	strcat(outbuf, inbuf);

	fwrite(outbuf, 1, strlen(outbuf), fp);

	memset(outbuf, 0, 1024);
	sprintf(outbuf, "extern lv_image_dsc_t img_%s; \n", bitmap_name);
	printf("extern text %s\n", outbuf);
	fwrite(outbuf, 1, strlen(outbuf), includefp);
}

static int _generate_string_id_table(FILE* fp, uint8_t* strvpath, FILE* includefp)
{
	char* nread;
	FILE* strvfp = NULL;
	uint8_t inbuf[256] = {0};
	uint8_t outbuf[256] = {0};
	uint8_t enumbuf[256] = {0};
	uint8_t name[256] = {0};
	int id = 0;
	int item_cnt = 0;

	printf("strvpath %s\n", strvpath);
	strvfp = fopen(strvpath, "rb");
	if(!strvfp)
	{
		printf("open string res value txt failed %s\n", strvpath);
		return -1;
	}

	fseek(strvfp, 0, SEEK_SET);
	nread = fgets(inbuf, 256, strvfp);
	while(nread != NULL)
	{
		item_cnt++;
		nread = fgets(inbuf, 256, strvfp);
	}
	sprintf(outbuf, "#include <res_manager_api.h>\n\nres_string_item_t res_string_id_dic[%d] = {\n", item_cnt+1);
	fwrite(outbuf, 1, strlen(outbuf), fp);

	sprintf(outbuf, "		{.key = NULL,		.value = 0},\n");
	fwrite(outbuf, 1, strlen(outbuf), fp);

	sprintf(enumbuf, "\ntypedef enum{ \n");
	fwrite(enumbuf, 1, strlen(enumbuf), includefp);

	fseek(strvfp, 0, SEEK_SET);
	nread = fgets(inbuf, 256, strvfp);
	while(nread != NULL)
	{
		memset(name, 0, 256);
		sscanf(inbuf, "%s %d", name, &id);
		id++; //id start from 1
		//printf("name %s, id %d\n", name, id);
		memset(outbuf, 0, 256);
		sprintf(outbuf, "		{.key = \"%s\",		.value = %d},\n", name, id);
		fwrite(outbuf, 1, strlen(outbuf), fp);

		memset(enumbuf, 0, 256);
		sprintf(enumbuf, "	id_%s,\n", name);
		fwrite(enumbuf, 1, strlen(enumbuf), includefp);
		
		memset(inbuf, 0, 256);
		nread = fgets(inbuf, 256, strvfp);
	}

	sprintf(outbuf, "	};\n");
	fwrite(outbuf, 1, strlen(outbuf), fp);
	
	memset(enumbuf, 0, 256);
	sprintf(enumbuf, "}res_string_id_e;\n");
	fwrite(enumbuf, 1, strlen(enumbuf), includefp);

	fclose(strvfp);
	return 0;
}

#if 1
int main(int argc, char* argv[])
{
    FILE* orip = NULL;
    FILE* newp = NULL;
    unsigned char* sty_data;
    unsigned char res_data[MAX_RES_DATA_SIZE];
    int res_size;
	int res_offset;
    resource_info_t info;
    resource_scene_t* scenes;
	int i;
	unsigned int* scene_item;
    uint32_t magic;
    int sty_size;
    int ret;
	char diff_path[MAX_DIFF_PATH_LEN];
	char* diff_ext;
	res_head_t head;
	uint8_t code_path[256] = {0};
	uint8_t code_file_name[256] = {0};
	

    if(argc < 6)
    {
        printf("usage: sty_conv [styfile_path] [picfile_path] [new_styfile_path] [sty_header_path] [str_res_dir]");
        return 0;
    }


	printf("%s %d\n", __FILE__, __LINE__);
    orip = fopen(argv[1], "rb");
    if(orip == NULL)
    {
        printf("open sty %s faild \n", argv[1]);
        return -1;
    }

	picfp = fopen(argv[2], "rb");
    if(picfp == NULL)
    {
        printf("open picres %s faild \n", argv[2]);
		fclose(orip);
        return -1;
    }
	fread(&head, 1, sizeof(res_head_t), picfp);
	basic_id_end = head.counts;

	strcpy(code_path, argv[4]);
	uint8_t* dirtail = strrchr(code_path, '\\');
	if(dirtail != NULL)
	{
		*(dirtail+1) = 0;
	}
	else
	{
		memset(code_path, 0, 256);
	}

	ret = _parse_header_nodes(argv[4]);
	if(ret < 0)
	{
		printf("open header %s faild \n", argv[3]);
		fclose(orip);
		fclose(picfp);
		return -1;	
	}

		
	memset(diff_path, 0, MAX_DIFF_PATH_LEN);
	strcpy(diff_path, argv[2]);
	diff_ext = strrchr(diff_path, '.');
	strcpy(diff_ext, "_OTA.res");
	picfp_diff = fopen(diff_path, "rb");
	if(picfp_diff == NULL)
	{
		printf("no diff pic res\n");
	}
	else
	{
		fread(&head, 1, sizeof(res_head_t), picfp_diff);
		diff_id_start = head.id_start;
		if(basic_id_end >= diff_id_start)
		{
			basic_id_end = head.id_start-1;
		}
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
        return -1;
    }
    fclose(orip);

    memset(&info, 0, sizeof(resource_info_t));
    _read_head(sty_data, &info);
	scenes = (resource_scene_t*)malloc(sizeof(resource_scene_t)*info.sum);
	
    res_size = 0;
	res_offset = 0;
	memset(res_data, 0, MAX_RES_DATA_SIZE);
	scene_item = info.scenes;
	printf("scene sum %d\n", info.sum);

	//open res var include file
	strcpy(include_path, code_path);
	strcat(include_path, "res_include.h");
	includefp = fopen(include_path, "wb");
	if(includefp == NULL)
	{
		free(sty_data);
		free(scenes);
		fclose(picfp);
		return -1;
	}
	else
	{		
		uint8_t inbuf[256] = {0};
		sprintf(inbuf, "#ifndef __RES_INCLUDE_H__\n#define __RES_INCLUDE_H__\n\n#include <lvgl/lvgl.h> \n\n");
		fwrite(inbuf, 1, strlen(inbuf), includefp);
	}


	//open bitmap code files
	memset(code_file_name, 0, 256);
	strcpy(code_file_name, code_path);
	strcat(code_file_name, "res_style_bitmap.c");
	stybitmapfp = fopen(code_file_name, "wb");
	if(stybitmapfp == NULL)
	{
		free(sty_data);
		free(scenes);
		fclose(picfp);
		fclose(includefp);
		return -1;
	}	

	//write includes to bitmap code files
	uint8_t* file_name = strrchr(argv[2],'\\');
	if(file_name == NULL)
	{
		file_name = argv[2];
	}
	else
	{
		file_name++;
	}
	_generate_bitmap_head(stybitmapfp, file_name);

	//parse sty and resource data scene by scene
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
		scenes[i].reserve = 0;

		printf("scene_id 0x%x, offset 0x%x, size %d\n", scene_id, offset, size);
		scenes[i].child_offset = res_offset;
		magic = scene_id;
		printf("%d magic 0x%x\n", magic);

		_push_arch_node(scene_id, 0);
        _parse_scene(sty_data, offset, scene, res_data, &res_offset, magic);
		_pop_arch_node();

		scene_item = scene_item + SCENE_ITEM_SIZE;
    }


	newp = fopen(argv[3], "wb");

    if(newp == NULL)
    {
        free(scenes);
        free(sty_data);
        return -1;
    }


	info.scenes = NULL; //clear data field to avoid misunderstanding
    fwrite(&info, 1, sizeof(resource_info_t), newp);
	fseek(newp, 8, SEEK_SET);
    fwrite(scenes, 1, sizeof(resource_scene_t)*info.sum, newp);
    fwrite(res_data, 1, res_offset, newp);
    fclose(newp);
	fclose(picfp);

	fclose(stybitmapfp);

	free(scenes);
	free(sty_data);
	if(node_list)
	{
		free(node_list);
	}

	//open string id info headers
	memset(code_file_name, 0, 256);
	strcpy(code_file_name, code_path);
	strcat(code_file_name, "res_string_id.c");
	stystringfp = fopen(code_file_name, "wb");
	if(stystringfp == NULL)
	{
		return -1;
	} 

	//write string id info to header
	memset(code_path, 0, 256);
	memset(code_file_name, 0, 256);
	strcpy(code_path, argv[2]);
	dirtail = strrchr(code_path, '\\');
	if(dirtail != NULL)
	{
		*(dirtail+1) = 0;
	}
	else
	{
		memset(code_path, 0, 256);
	}
	strcpy(code_file_name, code_path);
	strcat(code_file_name, "strvalue.txt");

	_generate_string_id_table(stystringfp, code_file_name, includefp);
	fclose(stystringfp);

	fclose(includefp);

	//generate string res codes
	uint8_t res_name[256] = {0};
	uint8_t* res_name_off = strrchr(argv[2], '\\');
	if(res_name_off == NULL)
	{
		strcpy(res_name, argv[2]);
	}
	else
	{
		strcpy(res_name, res_name_off+1);
	}
	res_name_off = strrchr(res_name, '.');
	*res_name_off = 0;

	memset(code_path, 0, 256);
	strcpy(code_path, argv[5]);

#ifdef _MSC_VER
	WIN32_FIND_DATAW findData;
	HANDLE hFind;

	wchar_t find_key[MAX_PATH] = { 0 };
	wchar_t* code_path_temp = charTowchar(code_path);
	_swprintf(find_key, L"%s\\*", code_path_temp);
	free(code_path_temp);

	hFind = FindFirstFileW(find_key, &findData);
	if (hFind == NULL)
	{
		printf("open str res dir failed %s\n", code_path);
		return -1;
	}
	do 
	{
		char* fimename = wcharTochar(findData.cFileName);

		if (strncmp(fimename, res_name, strlen(res_name)) == 0)
		{
			res_name_off = strrchr(fimename, '.');
			if (strcmp(res_name_off, ".sty") == 0 || strcmp(res_name_off, ".res") == 0 ||
				strcmp(res_name_off, ".Debug") == 0)
			{
				continue;
			}

			memset(code_file_name, 0, 256);
			strcpy(code_file_name, code_path);
			sprintf(code_file_name, "%s\\%s", code_path, fimename);
			printf("dname %s\n", fimename);

			uint8_t cmd[256] = { 0 };
			sprintf(cmd, "txt_conv.exe %s %s", code_file_name, include_path);
			system(cmd);
		}

		free(fimename);
	} while (FindNextFile(hFind, &findData));
#else
	DIR* strdir = opendir(code_path);
	if(strdir == NULL)
	{
		printf("open str res dir failed %s\n", code_path);
		return -1;
	}

	struct dirent* entry = readdir(strdir);
	while(entry)
	{
		if(strncmp(entry->d_name, res_name, strlen(res_name))==0)
		{
			res_name_off = strrchr(entry->d_name, '.');
			if(strcmp(res_name_off, ".sty") == 0 || strcmp(res_name_off, ".res")==0 || 
				strcmp(res_name_off, ".Debug")==0)
			{
				entry = readdir(strdir);
				continue;			
			}
		}
		else
		{
			entry = readdir(strdir);
			continue;
		}

		memset(code_file_name, 0, 256);
		strcpy(code_file_name, code_path);
		sprintf(code_file_name, "%s/%s", code_path, entry->d_name);
		printf("dname %s\n", entry->d_name);

		uint8_t cmd[256] = {0};
		sprintf(cmd, "txt_conv.exe %s %s", code_file_name, include_path);
		system(cmd);

		entry = readdir(strdir);
	}
#endif

	//close #if in include file
	includefp = fopen(include_path, "a+");
	uint8_t inbuf[256] = {0};
	sprintf(inbuf, "\n#endif //__RES_INCLUDE_H__ \n");
	fwrite(inbuf, 1, strlen(inbuf), includefp);

	fclose(includefp);

    return 0;
}
#endif

#else
#include <string.h>
#include <Stdio.h>
#include <stdlib.h>
#include "res_struct.h"

#pragma pack (1)

#define SCENE_ITEM_SIZE 3

#define PACK

#define MAX_RES_DATA_SIZE	1024*1024
#define MAX_DIFF_PATH_LEN	256


typedef struct _style_head
{
	uint32_t    magic;        /*0x18*/
	uint32_t    scene_sum;
	int8_t      reserve[8];
}style_head_t;

typedef struct _scene
{

	short  x;

	short  y;

	short  width;

	short  height;

	unsigned int background;

	unsigned char  visible;

	unsigned char   opaque;

	unsigned short  transparence;

	unsigned int resource_sum;

	unsigned int child_offset;

	unsigned char direction;

	unsigned char keys[16];
}scene_t;

typedef struct resource_s
{

	uint8_t type;

	uint32_t id;

	uint32_t offset;
}resource_t;


typedef struct _resgroup_resource
{

	resource_t   	 resource;

	int16_t  absolute_x;

	int16_t  absolute_y;

	int16_t  x;

	int16_t  y;

	int16_t  width;

	int16_t  height;

	uint32_t	 background;

	uint8_t  visible;

	uint8_t   opaque;

	uint16_t  transparence;

	uint32_t  	 resource_sum;

	uint32_t child_offset;
}resgroup_resource_t;

typedef struct _string_resource
{

	resource_t   	 resource;

	int16_t  x;

	int16_t  y;

	int16_t  width;

	int16_t  height;

	uint32_t  	 foreground;

	uint32_t	 background;

	uint8_t   visible;

	uint8_t   text_align;

	uint8_t   mode;

	uint16_t   font_height;

	uint16_t str_id;

	uint8_t scroll;

	int8_t direction;

	uint16_t space;

	uint16_t pixel;
}string_resource_t;


typedef struct _picture_resource
{
	resource_t   	 resource;
	int16_t  x;
	int16_t  y;
	int16_t  width;
	int16_t  height;
	uint8_t  visible;  //0x10=rgb ?ê? 0x11=argb
	uint16_t pic_id;
}picture_resource_t;

typedef struct _picregion_resource
{

	resource_t   	 resource;

	int16_t  x;

	int16_t  y;

	int16_t  width;

	int16_t  height;

	uint8_t   visible;

	uint16_t 	 frames;

	uint32_t pic_offset;
}picregion_resource_t;

typedef struct _picregion_frame
{

	uint32_t id;

	uint16_t  frame;

	int16_t  x;

	int16_t  y;

	int16_t  width;

	int16_t  height;

	uint16_t  pic_id;
}picregion_frame_t;

typedef struct
{
	uint8_t  magic[4];        //'R', 'E', 'S', 0x19
	uint16_t counts;
	uint16_t w_format;
	uint8_t brgb;
	uint8_t version;
	uint16_t id_start;
	uint8_t reserved;
	uint8_t tile_compress;
	uint8_t compressed_type;
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


FILE* picfp = NULL;
FILE* picfp_diff = NULL;
uint32_t basic_id_end = 0;
uint32_t diff_id_start = 0;

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

static int _parse_bitmap(char* sty_data, sty_picture_t* pic)
{
	int ret;
	res_entry_t res_entry;

	memset(&res_entry, 0, sizeof(res_entry_t));
	if (pic->id <= basic_id_end)
	{
		fseek(picfp, pic->id * sizeof(res_entry_t), SEEK_SET);
		ret = fread(&res_entry, 1, sizeof(res_entry_t), picfp);
	}
	else
	{
		if (picfp_diff)
		{
			fseek(picfp_diff, (pic->id - diff_id_start + 1) * sizeof(res_entry_t), SEEK_SET);
			ret = fread(&res_entry, 1, sizeof(res_entry_t), picfp_diff);
		}
	}
	if (ret < sizeof(res_entry_t))
	{
		printf("cant find sty_id 0x%x\n", pic->sty_id);
		return -1;
	}

	printf("bmp type %d\n", res_entry.type);
	switch (res_entry.type)
	{
	case RES_TYPE_PNG:
	case RES_TYPE_ARGB565:
	case RES_TYPE_ARGB565_LZ4:
		pic->bytes_per_pixel = 3;
		pic->format = RESOURCE_BITMAP_FORMAT_ARGB8565;
		break;
	case RES_TYPE_PIC:
	case RES_TYPE_LOGO:
	case RES_TYPE_RGB565_LZ4:
		pic->bytes_per_pixel = 2;
		pic->format = RESOURCE_BITMAP_FORMAT_RGB565;
		break;
	case RES_TYPE_PIC_ARGB1555:
	case RES_TYPE_ARGB1555_LZ4:
		pic->bytes_per_pixel = 2;
		pic->format = RESOURCE_BITMAP_FORMAT_ARGB1555;
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
	case RES_TYPE_A8:
	case RES_TYPE_A8_LZ4:
		pic->bytes_per_pixel = 1;
		pic->format = RESOURCE_BITMAP_FORMAT_A8;
		break;
	case RES_TYPE_ARGB6666:
	case RES_TYPE_ARGB6666_LZ4:
		pic->bytes_per_pixel = 3;
		pic->format = RESOURCE_BITMAP_FORMAT_ARGB6666;
		break;
	case RES_TYPE_JPEG:
		pic->bytes_per_pixel = 2;
		pic->format = RESOURCE_BITMAP_FORMAT_JPEG;
		break;
	case RES_TYPE_TILE_COMPRESSED:
		pic->bytes_per_pixel = 4;
		pic->format = RESOURCE_BITMAP_FORMAT_RAW;
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

	if ((res_entry.type == RES_TYPE_RGB565_LZ4) || (res_entry.type == RES_TYPE_PIC_ARGB8888_LZ4) ||
		(res_entry.type == RES_TYPE_A8_LZ4) || (res_entry.type == RES_TYPE_ARGB6666_LZ4) ||
		(res_entry.type == RES_TYPE_JPEG) || (res_entry.type == RES_TYPE_TILE_COMPRESSED) ||
		(res_entry.type == RES_TYPE_ARGB565_LZ4) || (res_entry.type == RES_TYPE_ARGB1555_LZ4))
	{
		pic->compress_size = ((res_entry.len_ext << 16) | res_entry.length) - 4;
	}
	else
	{
		pic->compress_size = 0;
	}
	pic->bmp_pos = res_entry.offset + 4;
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
	for (i = 0; i < group->resource_sum; i++)
	{
		if (resource->type == RESOURCE_TYPE_GROUP)
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

		if (resource->type == RESOURCE_TYPE_PICTURE)
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

		if (resource->type == RESOURCE_TYPE_TEXT)
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
			printf("font size %d, font height %d\n", res->font_size, str->font_height);
			
			res->id = (uint32_t)str->str_id;

			res_offset += sizeof(sty_string_t);
			res->offset = res_offset;
		}

		if (resource->type == RESOURCE_TYPE_PICREGION)
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
			for (j = 0; j < res->frames; j++)
			{
				bitmap->type = RESOURCE_TYPE_PICTURE;
				bitmap->sty_id = res->sty_id;
				res_offset += sizeof(sty_picture_t);
				bitmap->offset = res_offset;
				bitmap->x = res->x;
				bitmap->y = res->y;
				bitmap->width = res->width;
				bitmap->height = res->height;
				bitmap->id = (uint32_t)(buf[1] << 8 | buf[0]);
				bitmap->magic = magic + res->sty_id;
				_parse_bitmap(sty_data, bitmap);
				if (bpp == 0)
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
	for (i = 0; i < scene->resource_sum; i++)
	{
		if (resource->type == RESOURCE_TYPE_GROUP)
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
			printf("group 0x%x, 0x%x, 0x%x, 0x%x, 0x%x\n", group->absolute_x, group->absolute_y, group->x, group->y, group->visible, group->height);
			magic += res->sty_id;
			printf("%d magic 0x%x\n", __LINE__, magic);
			_parse_group(sty_data, &buf_offset, group, res_data, &res_offset, magic);
			res->offset = res_offset;
			magic -= res->sty_id;
		}

		if (resource->type == RESOURCE_TYPE_PICTURE)
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

		if (resource->type == RESOURCE_TYPE_TEXT)
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
			printf("font size %d, font height %d\n", res->font_size, str->font_height);

			res->id = (uint32_t)str->str_id;

			res_offset += sizeof(sty_string_t);
			res->offset = res_offset;
		}

		if (resource->type == RESOURCE_TYPE_PICREGION)
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
			for (j = 0; j < res->frames; j++)
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
				bitmap->id = (uint32_t)(buf[1] << 8 | buf[0]);
				bitmap->magic = magic + res->sty_id;
				_parse_bitmap(sty_data, bitmap);
				if (bpp == 0)
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
	FILE* orip = NULL;
	FILE* newp = NULL;
	unsigned char* sty_data;
	unsigned char res_data[MAX_RES_DATA_SIZE];
	int res_size;
	int res_offset;
	resource_info_t info;
	resource_scene_t* scenes;
	int i;
	unsigned int* scene_item;
	uint32_t magic;
	int sty_size;
	int ret;
	char diff_path[MAX_DIFF_PATH_LEN];
	char* diff_ext;
	res_head_t head;

	printf("%s %d\n\n", __FILE__, __LINE__);

	if (argc < 3)
	{
		printf("usage: sty_conv [styfile_path] [picfile_path] [new_styfile_path]");
		return 0;
	}


	printf("%s %d\n", __FILE__, __LINE__);
	orip = fopen(argv[1], "rb");
	if (orip == NULL)
	{
		printf("open sty %s faild \n", argv[1]);
		return -1;
	}

	picfp = fopen(argv[2], "rb");
	if (picfp == NULL)
	{
		printf("open picres %s faild \n", argv[2]);
		fclose(orip);
		return -1;
	}
	fread(&head, 1, sizeof(res_head_t), picfp);
	basic_id_end = head.counts;

	memset(diff_path, 0, MAX_DIFF_PATH_LEN);
	strcpy(diff_path, argv[2]);
	diff_ext = strrchr(diff_path, '.');
	strcpy(diff_ext, "_OTA.res");
	picfp_diff = fopen(diff_path, "rb");
	if (picfp_diff == NULL)
	{
		printf("no diff pic res\n");
	}
	else
	{
		fread(&head, 1, sizeof(res_head_t), picfp_diff);
		diff_id_start = head.id_start;
		if (basic_id_end >= diff_id_start)
		{
			basic_id_end = head.id_start - 1;
		}
	}


	fseek(orip, 0, SEEK_END);
	sty_size = ftell(orip);

	sty_data = (unsigned char*)malloc(sty_size);
	fseek(orip, 0, SEEK_SET);
	ret = fread(sty_data, 1, sty_size, orip);
	if (ret < sty_size)
	{
		printf("read sty %s failed\n", argv[1]);
		free(sty_data);
		fclose(orip);
		fclose(picfp);
		return -1;
	}
	fclose(orip);

	memset(&info, 0, sizeof(resource_info_t));
	_read_head(sty_data, &info);
	scenes = (resource_scene_t*)malloc(sizeof(resource_scene_t) * info.sum);

	res_size = 0;
	res_offset = 0;
	memset(res_data, 0, MAX_RES_DATA_SIZE);
	scene_item = info.scenes;
	printf("scene sum %d\n", info.sum);

	for (i = 0; i < info.sum; i++)
	{
		uint32_t scene_id = *scene_item;
		unsigned int offset = *(scene_item + 1);
		unsigned int size = *(scene_item + 2);
		scene_t* scene = (scene_t*)(sty_data + offset);
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
		scenes[i].reserve = 0;

		printf("scene_id 0x%x, offset 0x%x, size %d\n", scene_id, offset, size);
		scenes[i].child_offset = res_offset;
		magic = scene_id;
		printf("%d magic 0x%x\n", magic);
		_parse_scene(sty_data, offset, scene, res_data, &res_offset, magic);
		scene_item = scene_item + SCENE_ITEM_SIZE;
	}

	if (argc == 4)
	{
		newp = fopen(argv[3], "wb");
	}
	else
	{
		newp = fopen(argv[1], "wb");
	}
	if (newp == NULL)
	{
		free(scenes);
		free(sty_data);
		return -1;
	}

	info.scenes = NULL; //clear data field to avoid misunderstanding
	fwrite(&info, 1, sizeof(resource_info_t), newp);
	fseek(newp, 8, SEEK_SET);
	fwrite(scenes, 1, sizeof(resource_scene_t) * info.sum, newp);
	fwrite(res_data, 1, res_offset, newp);
	fclose(newp);
	fclose(picfp);

	free(scenes);
	free(sty_data);
	return 0;
}
#endif