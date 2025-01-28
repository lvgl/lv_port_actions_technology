#include <string.h>
#include <Stdio.h>
#include <stdlib.h>
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
	uint16_t	w;
	uint16_t	h;
	uint16_t 	what;
    int8_t      reserve[2];
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
    uint8_t reserved[5];
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
unsigned char revert_data[MAX_RES_DATA_SIZE];
unsigned char old_data[MAX_RES_DATA_SIZE];
uint32_t convd_offset;

//to get style file info
static int _write_head(style_head_t* sty_head, resource_info_t* info)
{
    int ret;
	uint32_t sty_size;

	sty_head->magic = 0x18;
	sty_head->scene_sum = info->sum;
	memset(sty_head->reserve, 0, 2);
    return 0;
}

static int _revert_picregion(uint8_t* data, uint32_t* offset, sty_picregion_t* cpicreg, uint8_t* convd_data, int* size)
{
	uint32_t start_offset = *offset;
	sty_picture_t* cbitmap;
	uint16_t* pic_offset;
	int i;
	int total_size = *size;

	cbitmap = (sty_picture_t*)(convd_data + cpicreg->id_offset);
	pic_offset = (uint16_t*)(data + start_offset);
	for(i=0;i<cpicreg->frames;i++)
	{
		*pic_offset = (uint16_t)cbitmap->id;
		cbitmap += sizeof(sty_picture_t);
		pic_offset++;
	}
	start_offset = start_offset + sizeof(uint16_t)*cpicreg->frames;
	*offset = start_offset;
	total_size+=sizeof(uint16_t)*cpicreg->frames;
	*size = total_size;
	return 0;
}

static int _revert_group(uint8_t* data, uint32_t* child_offset, sty_group_t* cgroup, int* size)
{
	uint32_t group_offset = *child_offset;
	uint32_t* base_type;
	uint8_t* group_data;
	//uint8_t* convd_data = (uint8_t*)cgroup;
	uint8_t* convd_data = old_data + convd_offset;
	uint32_t offset = (uint32_t)((uint8_t*)cgroup - convd_data)+sizeof(sty_group_t);
	int i;
	int total_size = *size;

	for(i=0;i<cgroup->resource_sum;i++)
	{
		base_type = (uint32_t*)(convd_data + offset);
		group_data = data + group_offset;
		switch(*base_type)
		{
		case RESOURCE_TYPE_GROUP:
			{
				uint32_t subgroup_offset = 0;
				sty_group_t* csubgroup = (sty_group_t*)base_type;
				resgroup_resource_t* rsubgroup = (resgroup_resource_t*)group_data;
				rsubgroup->resource.type = (uint8_t)csubgroup->type;
				rsubgroup->resource.id = csubgroup->sty_id;
				//fixme
				rsubgroup->absolute_x = csubgroup->x;
				rsubgroup->absolute_y = csubgroup->y;
				rsubgroup->x = csubgroup->x;
				rsubgroup->y = csubgroup->y;
				rsubgroup->width = csubgroup->width;
				rsubgroup->height = csubgroup->height;
				rsubgroup->background = csubgroup->backgroud;
				rsubgroup->child_offset = sizeof(resgroup_resource_t);
				rsubgroup->resource_sum = csubgroup->resource_sum;
				subgroup_offset = sizeof(resgroup_resource_t);
				total_size += sizeof(resgroup_resource_t);
				_revert_group(group_data, &subgroup_offset, csubgroup, &total_size);
				rsubgroup->resource.offset = subgroup_offset;
				if(i+1>=cgroup->resource_sum)
				{
					rsubgroup->resource.offset = 0;
				}				
				group_offset+=subgroup_offset;
			}
			break;
		case RESOURCE_TYPE_PICREGION:
			{
				uint32_t picreg_offset = 0;
				sty_picregion_t* cpicreg = (sty_picregion_t*)base_type;
				picregion_resource_t* rpicreg = (picregion_resource_t*)group_data;
				rpicreg->resource.type = (uint8_t)cpicreg->type;
				rpicreg->resource.id = cpicreg->sty_id;
				rpicreg->x = cpicreg->x;
				rpicreg->y = cpicreg->y;
				rpicreg->width = cpicreg->width;
				rpicreg->height = cpicreg->height;
				rpicreg->frames = cpicreg->frames;
				rpicreg->visible = 1;
				rpicreg->pic_offset = sizeof(picregion_resource_t);
				picreg_offset = sizeof(picregion_resource_t);
				total_size += sizeof(picregion_resource_t);
				_revert_picregion(group_data, &picreg_offset, cpicreg, convd_data, &total_size);
				rpicreg->resource.offset = picreg_offset;
				if(i+1>=cgroup->resource_sum)
				{
					rpicreg->resource.offset = 0;
				}				
				group_offset+=picreg_offset;
			}
			break;
		case RESOURCE_TYPE_PICTURE:
			{
				sty_picture_t* cbitmap = (sty_picture_t*)base_type;
				picture_resource_t* rbitmap = (picture_resource_t*)group_data;
				rbitmap->resource.type = (uint8_t)cbitmap->type;
				rbitmap->resource.id = cbitmap->sty_id;
				rbitmap->x = cbitmap->x;
				rbitmap->y = cbitmap->y;
				rbitmap->width = cbitmap->width;
				rbitmap->height = cbitmap->height;
				rbitmap->pic_id = cbitmap->id;
				if(cbitmap->format == RESOURCE_BITMAP_FORMAT_RGB565 || cbitmap->format == RESOURCE_BITMAP_FORMAT_RGB888)
				{
					rbitmap->visible = 0x10;
				}
				else if(cbitmap->format == RESOURCE_BITMAP_FORMAT_RGB565 || cbitmap->format == RESOURCE_BITMAP_FORMAT_RGB888)
				{
					rbitmap->visible = 0x11;
				}
				else
				{
					rbitmap->visible = 0x10;
				}
				rbitmap->resource.offset = sizeof(picture_resource_t);
				if(i+1>=cgroup->resource_sum)
				{
					rbitmap->resource.offset = 0;
				}				
				group_offset+=sizeof(picture_resource_t);
				total_size+=sizeof(picture_resource_t);
			}
			break;
		case RESOURCE_TYPE_TEXT:
			{
				sty_string_t* ctext = (sty_string_t*)base_type;
				string_resource_t* rtext = (string_resource_t*)group_data;
				rtext->resource.type = (uint8_t)ctext->type;
				rtext->resource.id = ctext->sty_id; 			
				rtext->x = ctext->x;
				rtext->y = ctext->y;
				rtext->width = ctext->width;
				rtext->height = ctext->height;
				rtext->background = ctext->bgcolor;
				rtext->foreground = ctext->color;
				rtext->font_height = ctext->font_size;
				rtext->text_align = ctext->align;
				rtext->str_id = ctext->id;
				rtext->direction = 0;
				rtext->mode = 0;
				rtext->pixel = 1;
				rtext->scroll = 0;
				rtext->space = 0;
				rtext->visible = 1;
				rtext->resource.offset = sizeof(string_resource_t);
				if(i+1>=cgroup->resource_sum)
				{
					rtext->resource.offset = 0;
				}
				group_offset+=sizeof(string_resource_t);
				total_size+=sizeof(string_resource_t);
			}
			break;
		default:
			break;
		}
		
		offset = *(base_type+2);
	}
	*child_offset = group_offset;
	*size = total_size;
}

static int _revert_scene(char* child_data, resource_scene_t* convd_scene, uint32_t* revert_offset, int* size)
{
	scene_t* scene;
	uint32_t scene_offset = *revert_offset;
	uint8_t* scene_data;
	uint8_t* convd_data = old_data + convd_offset;
	int i;
	uint32_t* base_type;
	uint32_t offset;
	resource_t* base_res;
	int total_size = *size;

	printf("sizeof scene_t %d, convd_scene->resource_sum %d\n", sizeof(scene_t), convd_scene->resource_sum);
	scene = (scene_t*)(child_data+scene_offset);
	scene->background = convd_scene->background;
	scene->child_offset = sizeof(scene_t);
	scene->direction = convd_scene->direction;
	scene->width = convd_scene->width;
	scene->height = convd_scene->height;
	scene->x = convd_scene->x;
	scene->y = convd_scene->y;
	scene->opaque = convd_scene->opaque;
	scene->transparence = convd_scene->transparence;
	scene->visible = convd_scene->visible;
	scene->resource_sum = convd_scene->resource_sum;
	memset(scene->keys, 0, 16);

	scene_offset+= sizeof(scene_t);
	total_size += sizeof(scene_t);
	offset = convd_scene->child_offset;
	printf("scene id 0x%x, x %d, y %d, w %d, h %d, child offset %d\n ", convd_scene->scene_id,
		convd_scene->x, convd_scene->y, convd_scene->width, convd_scene->height, convd_scene->child_offset);
		
	for(i=0;i<convd_scene->resource_sum;i++)
	{
		uint32_t tmp_id;
		base_type = (uint32_t*)(convd_data + offset);
		scene_data = child_data+scene_offset;
		tmp_id = *(base_type+1);
		printf("id 0x%x\n", *(base_type+1));
		printf("base_type %d, offset %d\n", *base_type, offset);

		if(tmp_id == 0x621ceaeb)
		{
			printf("here\n");
		}
		
		switch(*base_type)
		{
		case RESOURCE_TYPE_GROUP:
			{
				uint32_t group_offset = 0;
				sty_group_t* cgroup = (sty_group_t*)base_type;
				resgroup_resource_t* rgroup = (resgroup_resource_t*)scene_data;
				rgroup->resource.type = (uint8_t)cgroup->type;
				rgroup->resource.id = cgroup->sty_id;
				//fixme
				rgroup->absolute_x = cgroup->x;
				rgroup->absolute_y = cgroup->y;
				rgroup->x = cgroup->x;
				rgroup->y = cgroup->y;
				rgroup->width = cgroup->width;
				rgroup->height = cgroup->height;
				rgroup->background = cgroup->backgroud;
				rgroup->child_offset = sizeof(resgroup_resource_t);
				rgroup->resource_sum = cgroup->resource_sum;
				group_offset = sizeof(resgroup_resource_t);
				printf("groupid 0x%x, group_offset %d, child_offset %d\n", cgroup->sty_id ,group_offset, cgroup->child_offset);
				total_size+=sizeof(resgroup_resource_t);
				_revert_group(scene_data, &group_offset, cgroup, &total_size);
				rgroup->resource.offset = group_offset;
				if(i+1>=convd_scene->resource_sum)
				{
					rgroup->resource.offset = 0;
				}				
				scene_offset+=group_offset;
			}
			break;
		case RESOURCE_TYPE_PICREGION:
			{
				uint32_t picreg_offset = 0;
				sty_picregion_t* cpicreg = (sty_picregion_t*)base_type;
				picregion_resource_t* rpicreg = (picregion_resource_t*)scene_data;
				rpicreg->resource.type = (uint8_t)cpicreg->type;
				rpicreg->resource.id = cpicreg->sty_id;
				rpicreg->x = cpicreg->x;
				rpicreg->y = cpicreg->y;
				rpicreg->width = cpicreg->width;
				rpicreg->height = cpicreg->height;
				rpicreg->frames = cpicreg->frames;
				rpicreg->visible = 1;
				rpicreg->pic_offset = sizeof(picregion_resource_t);
				picreg_offset = sizeof(picregion_resource_t);
				total_size += sizeof(picregion_resource_t);
				_revert_picregion(scene_data, &picreg_offset, cpicreg, convd_data, &total_size);
				rpicreg->resource.offset = picreg_offset;
				if(i+1>=convd_scene->resource_sum)
				{
					rpicreg->resource.offset = 0;
				}				
				scene_offset+=picreg_offset;
			}
			break;
		case RESOURCE_TYPE_PICTURE:
			{
				sty_picture_t* cbitmap = (sty_picture_t*)base_type;
				picture_resource_t* rbitmap = (picture_resource_t*)scene_data;
				rbitmap->resource.type = (uint8_t)cbitmap->type;
				rbitmap->resource.id = cbitmap->sty_id;
				rbitmap->x = cbitmap->x;
				rbitmap->y = cbitmap->y;
				rbitmap->width = cbitmap->width;
				rbitmap->height = cbitmap->height;
				rbitmap->pic_id = cbitmap->id;
				if(cbitmap->format == RESOURCE_BITMAP_FORMAT_RGB565 || cbitmap->format == RESOURCE_BITMAP_FORMAT_RGB888)
				{
					rbitmap->visible = 0x10;
				}
				else if(cbitmap->format == RESOURCE_BITMAP_FORMAT_ARGB8565 || cbitmap->format == RESOURCE_BITMAP_FORMAT_ARGB8888
					||cbitmap->format == RESOURCE_BITMAP_FORMAT_ARGB6666)
				{
					rbitmap->visible = 0x11;
				}
				else
				{
					rbitmap->visible = 0x10;
				}
				rbitmap->resource.offset = sizeof(picture_resource_t);
				if(i+1>=convd_scene->resource_sum)
				{
					rbitmap->resource.offset = 0;
				}				
				scene_offset+=sizeof(picture_resource_t);
				total_size += sizeof(picture_resource_t);
			}	
			break;
		case RESOURCE_TYPE_TEXT:
			{
				printf("text \n\n");
				sty_string_t* ctext = (sty_string_t*)base_type;
				string_resource_t* rtext = (string_resource_t*)scene_data;
				rtext->resource.type = (uint8_t)ctext->type;
				rtext->resource.id = ctext->sty_id;				
				rtext->x = ctext->x;
				rtext->y = ctext->y;
				rtext->width = ctext->width;
				rtext->height = ctext->height;
				rtext->background = ctext->bgcolor;
				rtext->foreground = ctext->color;
				rtext->font_height = ctext->font_size;
				rtext->text_align = ctext->align;
				rtext->str_id = ctext->id;
				rtext->direction = 0;
				rtext->mode = 0;
				rtext->pixel = 1;
				rtext->scroll = 0;
				rtext->space = 0;
				rtext->visible = 1;
				rtext->resource.offset = sizeof(string_resource_t);
				if(i+1>=convd_scene->resource_sum)
				{
					rtext->resource.offset = 0;
				}
				
				scene_offset+=sizeof(string_resource_t);
				total_size += sizeof(string_resource_t);
			}
			break;
		default:
			break;
		}
		offset = *(base_type+2);

	}

	*size = total_size;
	*revert_offset = scene_offset;
}


int main(int argc, char* argv[])
{
	FILE* convdp = NULL;
    FILE* revertp = NULL;
    int res_size;
	int res_offset;
	resource_info_t convd_info;
    style_head_t sty_head;
    resource_scene_t* scenes;
	int i;
	uint32_t* scene_item;
	uint32_t* scene_item_start;
    uint32_t magic;
    int revert_size;
	int convd_size;
	uint32_t revert_offset;
	uint8_t* child_data;
    int ret;

	printf("%s %d\n\n", __FILE__, __LINE__);

    if(argc < 3)
    {
        printf("usage: sty_conv_revert [convd_sty] [reverted_sty]");
        return 0;
    }


	printf("%s %d\n", __FILE__, __LINE__);
    convdp = fopen(argv[1], "rb");
    if(convdp == NULL)
    {
        printf("open convd sty %s faild \n", argv[1]);
        return -1;
    }

	
    revertp = fopen(argv[2], "wb");
    if(revertp == NULL)
    {
        printf("open dst revert sty %s faild \n", argv[2]);
		fclose(convdp);
        return -1;
    }

	//read info of old convd sty data
	fseek(convdp, 0, SEEK_END);
	convd_size = ftell(convdp);

	fseek(convdp, 0, SEEK_SET);
	fread(&convd_info, 1, sizeof(resource_info_t), convdp);
	ret = fread(old_data, 1, convd_size-sizeof(resource_info_t), convdp);
	if(ret < convd_size-sizeof(resource_info_t))
	{
        printf("read sty %s failed\n", argv[1]);
		fclose(convdp);
        return -1;		
	}

	memset(revert_data, 0, MAX_RES_DATA_SIZE);

    memset(&sty_head, 0, sizeof(style_head_t));
	
    _write_head(&sty_head, &convd_info);

	scene_item = (uint32_t*)malloc(sizeof(uint32_t)*3*convd_info.sum);
	scene_item_start = scene_item;
	memset(scene_item, 0, sizeof(uint32_t)*3*convd_info.sum);
	scenes = (resource_scene_t*)old_data;
	//revert_offset = sizeof(uint32_t)*3*convd_info.sum + sizeof(style_head_t);
	uint32_t head_size = sizeof(uint32_t)*3*convd_info.sum + sizeof(style_head_t);
	revert_offset = 0;
	child_data = revert_data;
	
	convd_offset = convd_info.sum*sizeof(resource_scene_t);

    printf("convd_info.sum %d\n", convd_info.sum);
    for(i=0;i<convd_info.sum;i++)
    //for(i=0;i<1;i++)
    {
    	int size = 0;
    	*scene_item = scenes[i].scene_id;
		scene_item++;
		*scene_item = revert_offset+head_size;
		scene_item++;

		if(scenes[i].scene_id == 0x5849a9f4)
		{
			printf("here\n");
		}
		
		printf("scene id 0x%x, offset %d\n", scenes[i].scene_id, revert_offset);
		_revert_scene(child_data, &scenes[i], &revert_offset, &size);
		printf("revert_offset %d, size %d\n\n",revert_offset, size);
		*scene_item = size;
		scene_item++; 
//		child_data += size;
    }

	scene_item = scene_item_start;
	for(i=0;i<convd_info.sum;i++)
	{
		printf("scene item 0x%x, 0x%x, 0x%x\n", *scene_item, *(scene_item+1), *(scene_item+2));
		scene_item+=3;
	}
	printf("sizeof(string_resource_t) %d, sizeof(picture_resource_t) %d\n", sizeof(string_resource_t), sizeof(picture_resource_t));
	
	scene_item = scene_item_start;
/*
	sty_head.w = 454;
	sty_head.h = 454;
	sty_head.what = 1;
*/
    fwrite(&sty_head, 1, sizeof(style_head_t), revertp);
    fwrite(scene_item, 1, sizeof(uint32_t)*3*convd_info.sum, revertp);
    fwrite(revert_data, 1, revert_offset, revertp);
    fclose(revertp);
	fclose(convdp);
	
	free(scene_item);
    return 0;
}
