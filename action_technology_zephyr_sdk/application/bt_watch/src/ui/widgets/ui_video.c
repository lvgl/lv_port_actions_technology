/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#if CONFIG_VIDEO_PLAYER

/**
 * @file ui_video.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include <assert.h>
#include <ui_mem.h>
#include "ui_video.h"
#include <stdio.h>
#include <file_stream.h>

#define UI_VIDEO_MIN_FPS 17	//16.666 ~ 17
#define MAX_PACKET_LEN 		2048
#ifdef CONFIG_UI_MANAGER
#  define UI_VIDEO_MALLOC(size)        ui_mem_aligned_alloc(MEM_RES, 64, size, __func__)
#  define UI_VIDEO_FREE(ptr)           ui_mem_free(MEM_RES, ptr)
#  define UI_VIDEO_REALLOC(ptr,size)   ui_mem_realloc(MEM_RES,ptr,size,__func__)
#else
#  define UI_VIDEO_MALLOC(size)        lv_mem_alloc(size)
#  define UI_VIDEO_FREE(ptr)           lv_mem_free(ptr)
#  define UI_VIDEO_REALLOC(ptr,size)   lv_mem_realloc(ptr,size)
#endif

/*********************
 *      DEFINES
 *********************/
#define MY_CLASS &ui_video_class

/**********************
 *      TYPEDEFS
 **********************/

/** Data of ui video */
typedef struct {
	lv_obj_t obj;
	lv_timer_t *decode_timer;
	lv_timer_t *up_timer;
	io_stream_t file_stream;
	demuxer_plugin_t *dem_plugin;
	void *dem_handle;
	ve_video_info_t video_info;
	demuxer_packet_t packet_buf;
	const void *src; /* image source */
	lv_area_t trans_area;   /* image transformed area (relative to the top-left of the object) */
	lv_image_header_t header; /* image header */
	lv_point_t pivot;     /* Rotation center of the image, relative to the top-left corner of the image */
	ui_video_play_cb_t play_cb;
	uint16_t angle;    /* Rotation angle in 0.1 degree [0, 3600) of the image */
	uint16_t zoom;
	uint8_t decode_mark : 1;
} _ui_video_t;

/**********************
 *  EXTERNAL PROTOTYPES
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void _ui_video_destructor(const lv_obj_class_t * class_p, lv_obj_t * obj);
static void _ui_video_event(const lv_obj_class_t * class_p, lv_event_t * e);
static void _ui_video_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj);
/**********************
 *  STATIC VARIABLES
 **********************/
const lv_obj_class_t ui_video_class = {
	.destructor_cb = _ui_video_destructor,
	.constructor_cb = _ui_video_constructor,
	.event_cb = _ui_video_event,
	.width_def = LV_SIZE_CONTENT,
	.height_def = LV_SIZE_CONTENT,
	.instance_size = sizeof(_ui_video_t),
	.base_class = &lv_obj_class,
	.name = "ui-video",
};

/**********************
 *      MACROS
 **********************/

/**********************
 *   STATIC FUNCTIONS
 **********************/
static void _ui_video_free(_ui_video_t * video)
{
	if(video == NULL)
		return;
	if(video->decode_timer)
	{
		lv_timer_delete(video->decode_timer);
		video->decode_timer = NULL;
	}
	if(video->up_timer)
	{
		lv_timer_delete(video->up_timer);
		video->up_timer = NULL;
	}
	if(video->src)
	{
		lv_image_dsc_t *video_img = (lv_image_dsc_t *)video->src;
		if(video_img->data)
		{
			UI_VIDEO_FREE((void *)video_img->data);
			video_img->data = NULL;
		}
		lv_mem_free((void *)video->src);
		video->src = NULL;
	}

	if(video->dem_plugin)
	{
		if(video->dem_handle)
		{
			video->dem_plugin->dispose(video->dem_handle);
			video->dem_handle = NULL;
		}
		video->dem_plugin = NULL;
	}
	if (video->file_stream) {
		stream_close(video->file_stream);
		stream_destroy(video->file_stream);
		video->file_stream = NULL;
	}
}

static void _ui_video_destructor(const lv_obj_class_t * class_p, lv_obj_t * obj)
{
	LV_UNUSED(class_p);
	_ui_video_t * video = (_ui_video_t *)obj;
	_ui_video_free(video);
}

static void _ui_video_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj)
{
	LV_UNUSED(class_p);
	_ui_video_t * video = (_ui_video_t *)obj;
	video->zoom = LV_SCALE_NONE;
}

static void draw_img(lv_event_t * e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t * obj = lv_event_get_target(e);
	_ui_video_t * video = (_ui_video_t *)obj;
	lv_area_t coords;

	if (code == LV_EVENT_COVER_CHECK) {
		lv_cover_check_info_t * info = lv_event_get_param(e);
		if (info->res == LV_COVER_RES_MASKED) return;

		if (video->angle != 0 || video->src == NULL) {
			info->res = LV_COVER_RES_NOT_COVER;
			return;
		}

		if (video->header.cf != LV_IMG_CF_TRUE_COLOR && video->header.cf != LV_IMG_CF_RAW) {
			info->res = LV_COVER_RES_NOT_COVER;
			return;
		}

		/*With not LV_OPA_COVER images can't cover an area */
		if (lv_obj_get_style_img_opa(obj, LV_PART_MAIN) != LV_OPA_COVER) {
			info->res = LV_COVER_RES_NOT_COVER;
			return;
		}

		lv_area_set(&coords, video->trans_area.x1 + obj->coords.x1,
					video->trans_area.y1 + obj->coords.y1,
					video->trans_area.x2 + obj->coords.x1,
					video->trans_area.y2 + obj->coords.y1);

		if (_lv_area_is_in(info->area, &coords, 0) == false) {
			info->res = LV_COVER_RES_NOT_COVER;
			return;
		}

		info->res = LV_COVER_RES_COVER;
	} else if (code == LV_EVENT_DRAW_MAIN) {
		if (video->src == NULL)
			return;

		lv_layer_t * layer = lv_event_get_layer(e);

		lv_draw_image_dsc_t draw_dsc;
		lv_draw_image_dsc_init(&draw_dsc);
		//lv_obj_init_draw_image_dsc(obj, LV_PART_MAIN, &draw_dsc);
		draw_dsc.recolor = lv_obj_get_style_image_recolor_filtered(obj, LV_PART_MAIN);
		draw_dsc.recolor_opa = lv_obj_get_style_image_recolor_opa(obj, LV_PART_MAIN);
		draw_dsc.opa = lv_obj_get_style_image_opa(obj, LV_PART_MAIN);
		draw_dsc.rotation = video->angle;
		draw_dsc.scale_x = video->zoom;
		draw_dsc.scale_y = video->zoom;
		draw_dsc.pivot.x = video->pivot.x;
		draw_dsc.pivot.y = video->pivot.y;
		draw_dsc.src = video->src;

		/* FIXME: need tile as lv_img ? */
		lv_area_set(&coords, obj->coords.x1, obj->coords.y1,
					obj->coords.x1 + video->header.w - 1,
					obj->coords.y1 + video->header.h - 1);
		lv_draw_image(layer, &draw_dsc, &coords);
	}
}

static void _ui_video_event(const lv_obj_class_t * class_p, lv_event_t * e)
{
	LV_UNUSED(class_p);

	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t * obj = lv_event_get_target(e);
	_ui_video_t * video = (_ui_video_t *)obj;

	/*Ancestor events will be called during drawing*/
	if (code != LV_EVENT_COVER_CHECK && code != LV_EVENT_DRAW_MAIN && code != LV_EVENT_DRAW_POST) {
		/*Call the ancestor's event handler*/
		lv_res_t res = lv_obj_event_base(MY_CLASS, e);
		if (res != LV_RES_OK) return;
	}

	if (code == LV_EVENT_STYLE_CHANGED) {
		if (video->angle > 0 || video->zoom != LV_SCALE_NONE) {
			/*With transformation it might change*/
			lv_obj_refresh_ext_draw_size(obj);
		}
	} else if (code == LV_EVENT_REFR_EXT_DRAW_SIZE) {
		if (video->angle > 0 || video->zoom != LV_SCALE_NONE) {
			lv_coord_t * s = lv_event_get_param(e);
			lv_coord_t pad_ori = *s;
			lv_coord_t w = lv_obj_get_width(obj);
			lv_coord_t h = lv_obj_get_height(obj);

			*s = LV_MAX(*s, pad_ori - video->trans_area.x1);
			*s = LV_MAX(*s, pad_ori - video->trans_area.y1);
			*s = LV_MAX(*s, pad_ori + video->trans_area.x2 - w + 1);
			*s = LV_MAX(*s, pad_ori + video->trans_area.y2 - h + 1);
		}
	} else if (code == LV_EVENT_HIT_TEST) {
		lv_hit_test_info_t * info = lv_event_get_param(e);
		lv_area_t coords;
		coords.x1 = video->trans_area.x1 + obj->coords.x1;
		coords.y1 = video->trans_area.y1 + obj->coords.y1;
		coords.x2 = video->trans_area.x2 + obj->coords.x1;
		coords.y2 = video->trans_area.y2 + obj->coords.y1;

		info->res = _lv_area_is_point_on(&coords, info->point, 0);
	} else if (code == LV_EVENT_GET_SELF_SIZE) {
		lv_point_t * p = lv_event_get_param(e);
		if (video->src) {
			p->x = video->header.w;
			p->y = video->header.h;
		}
	} else if (code == LV_EVENT_DRAW_MAIN || code == LV_EVENT_DRAW_POST || code == LV_EVENT_COVER_CHECK) {
		draw_img(e);
	}
}

static void invalidate_image_area(lv_obj_t * obj)
{
	_ui_video_t * video = (_ui_video_t *)obj;

	lv_area_t abs_area;
	abs_area.x1 = video->trans_area.x1 + obj->coords.x1;
	abs_area.y1 = video->trans_area.y1 + obj->coords.y1;
	abs_area.x2 = video->trans_area.x2 + obj->coords.x1;
	abs_area.y2 = video->trans_area.y2 + obj->coords.y1;

	lv_obj_invalidate_area(obj, &abs_area);
}

static void refresh_image_area(lv_obj_t * obj)
{
	_ui_video_t * video = (_ui_video_t *)obj;

	invalidate_image_area(obj);

	if (video->angle > 0 || video->zoom != LV_SCALE_NONE) {
		_lv_img_buf_get_transformed_area(&video->trans_area, video->header.w,
				video->header.h, video->angle, video->zoom, &video->pivot);
		lv_obj_refresh_ext_draw_size(obj);
	} else {
		video->trans_area.x1 = 0;
		video->trans_area.y1 = 0;
		video->trans_area.x2 = video->header.w - 1;
		video->trans_area.y2 = video->header.h - 1;
	}

	invalidate_image_area(obj);
}

static void _ui_video_reset(_ui_video_t * video)
{
	seek_info_t seek_info = {0};
	seek_info.seek_cmd = SEEK_TIME;
	seek_info.curtime = 0;
	video->dem_plugin->seek(video->dem_handle,&seek_info);
}

static void _ui_video_decode(_ui_video_t * video)
{
	if(video->decode_mark)
		return;
	demuxer_packet_t packet_buf = {0};
	packet_buf.packet_type = UNKOWN_ES;
	packet_buf.data = lv_mem_alloc(MAX_PACKET_LEN);
	if (!packet_buf.data) {
		LV_LOG_ERROR("decode alloc error");
		return;
	}
	int ret = EN_MEMERR;
	for(uint8_t i = 0 ; i < 2 ; i++)
	{
		ret = video->dem_plugin->getframe(video->dem_handle, &packet_buf);
		if (ret != EN_NORMAL)
		{
			if(EN_FILEISEND == ret)
			{
				_ui_video_reset(video);
				i = 0;
			}
			else
			{
				lv_mem_free(packet_buf.data);
				LV_LOG_ERROR("getframe fail");
				return;
			}
		}
		if(packet_buf.packet_type == VIDEO_ES)
			break;
	}
	lv_mem_free(packet_buf.data);
	if(packet_buf.packet_type == VIDEO_ES)
	{
		void *jpg_data = UI_VIDEO_MALLOC(packet_buf.data_len);
		if (!jpg_data) {
			LV_LOG_ERROR("buf alloc %d fail", packet_buf.data_len);
			return;
		}
		int padding = ((4 - ((packet_buf.data_len)&(0x3)))&0x3);

		stream_read(video->file_stream, jpg_data, packet_buf.data_len);

		lv_image_dsc_t *video_img = (lv_image_dsc_t *)video->src;
		int len = jpg_decode(jpg_data, packet_buf.data_len, (void *)video_img->data, HAL_JPEG_OUT_RGB565, 
			video_img->header.w, 0, 0, video_img->header.w, video_img->header.h);
		UI_VIDEO_FREE(jpg_data);

		//skip padding
		if (padding)
		{
			char *temp_buf = lv_mem_alloc(padding);
			if(!temp_buf)
			{
				LV_LOG_ERROR("temp_buf alloc %d fail", padding);
				return;
			}
			stream_read(video->file_stream, temp_buf, padding);
			lv_mem_free(temp_buf);
		}

		if(len != video_img->data_size)
		{
			LV_LOG_ERROR("buf alloc %d fail", packet_buf.data_len);
			return;
		}
	}

	video->decode_mark = true;
}

static void _ui_video_decode_timer(lv_timer_t * timer)
{
	_ui_video_t * video = (_ui_video_t *)timer->user_data;
	_ui_video_decode(video);
	lv_timer_delete(video->decode_timer);
	video->decode_timer = NULL;
}

static void _ui_video_update_timer(lv_timer_t * timer)
{
	_ui_video_t * video = (_ui_video_t *)timer->user_data;
	if(video->decode_timer)
	{
		lv_timer_delete(video->decode_timer);
		video->decode_timer = NULL;
	}
	if(!video->decode_mark)
		_ui_video_decode(video);
	if(video->play_cb)
		video->play_cb((lv_obj_t *)video);
	lv_obj_invalidate((lv_obj_t *)video);
	video->decode_mark = false;
	if(video->decode_timer == NULL)
		video->decode_timer = lv_timer_create(_ui_video_decode_timer,5,video);
}

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

lv_obj_t *ui_video_create(lv_obj_t * parent)
{
	lv_obj_t * obj = lv_obj_class_create_obj(MY_CLASS, parent);
	lv_obj_class_init_obj(obj);
	return obj;
}

/**
 * Set ui_video path.
 */
void ui_video_set_path(lv_obj_t *obj, const char *path)
{
	LV_ASSERT_OBJ(obj, MY_CLASS);
	char *header_buf = NULL;
	_ui_video_t * video = (_ui_video_t *)obj;

	_ui_video_free(video);
	video->file_stream = file_stream_create(path);
	if (!video->file_stream) {
		goto err_out;
	}

	int ret = stream_open(video->file_stream, MODE_IN);
	if (ret < 0) {
		LV_LOG_ERROR("ui_video stream_open error !");
		goto err_out;
	}

	header_buf = lv_mem_alloc(512);
	if(header_buf == NULL)
	{
		LV_LOG_ERROR("header_buf error");
		goto err_out;
	}

	ret = stream_read(video->file_stream, header_buf, 512);
	if (ret < 0) {
		LV_LOG_ERROR("ui_video stream_read error !");
		goto err_out;
	}

	if ((header_buf[8] == 'A')&&(header_buf[9] == 'V')&&(header_buf[10] == 'I')&&(header_buf[11] == ' '))
	{
		if (!((header_buf[0x156] == 'A')&&(header_buf[0x157] == 'V')&&(header_buf[0x158] == 'I')&&(header_buf[0x159] == '1')))
		{
			LV_LOG_ERROR("ui_video format avi check error !");
			goto err_out;
		}
	}
	else
	{
		LV_LOG_ERROR("ui_video format invalid!");
		goto err_out;
	}
	lv_mem_free(header_buf);
	header_buf = NULL;
	// seek to head
	stream_seek(video->file_stream, 0, SEEK_DIR_BEG);
	video->dem_plugin = avi_api();
	if(video->dem_plugin == NULL)
		goto err_out;
	ve_media_info_t media_info;
	video->dem_handle = video->dem_plugin->open(video->file_stream, &media_info);
	if(video->dem_handle == NULL)
	{
		LV_LOG_ERROR("ui_video dem_plugin->open failed!");
		goto err_out;
	}
	SYS_LOG_INF("index_flag: %d", media_info.index_flag);
	SYS_LOG_INF("v_width: %d", media_info.video_info.width);
	SYS_LOG_INF("v_height: %d", media_info.video_info.height);
	SYS_LOG_INF("vtotal_time: %d", media_info.video_info.vtotal_time);
	SYS_LOG_INF("frame_rate: %d", media_info.video_info.frame_rate);
	if (media_info.video_info.width > CONFIG_PANEL_TIMING_VACTIVE || media_info.video_info.height > CONFIG_PANEL_TIMING_HACTIVE)
	{
		LV_LOG_ERROR("video %dx%d not support!", media_info.video_info.width, media_info.video_info.height);
		goto err_out;
	}
	memcpy(&video->video_info, &media_info.video_info, sizeof(ve_video_info_t));
	if(video->src == NULL)
	{
		lv_image_dsc_t *video_img = NULL;
		video_img = lv_mem_alloc(sizeof(lv_image_dsc_t));
		if(video_img == NULL)
		{
			LV_LOG_ERROR("lv_mem_alloc video_img error");
			goto err_out;
		}
		video->src = (const void *)video_img;
		lv_memset_00(video_img, sizeof(lv_image_dsc_t));
		video_img->header.cf = LV_IMG_CF_RGB_565;
		video_img->header.w = video->video_info.width;
		video_img->header.h = video->video_info.height;
		video_img->data_size = video_img->header.w * video_img->header.h * (lv_img_cf_get_px_size(video_img->header.cf)/8);
		video_img->data = UI_VIDEO_MALLOC(video_img->data_size);
		if(video_img->data == NULL)
		{
			LV_LOG_ERROR("UI_VIDEO_MALLOC video_img->data error");
			goto err_out;
		}
	}

	uint32_t rate = 1000/video->video_info.frame_rate;
	if(rate < UI_VIDEO_MIN_FPS)
		rate = UI_VIDEO_MIN_FPS;
	video->up_timer = lv_timer_create(_ui_video_update_timer, rate, video);
	if(video->up_timer == NULL)
		goto err_out;
	_ui_video_decode(video);
	video->decode_mark = false;
	if (video->src) {
		lv_img_decoder_get_info(video->src, &video->header);

		video->pivot.x = video->header.w / 2;
		video->pivot.y = video->header.h / 2;

		refresh_image_area(obj);
		lv_obj_refresh_self_size(obj);
	}
	lv_obj_invalidate(obj);
	return;
err_out:
	LV_LOG_ERROR("ui_video error");
	if(header_buf)
		lv_mem_free(header_buf);
	_ui_video_free(video);
}

void ui_video_set_pivot(lv_obj_t * obj, lv_coord_t pivot_x, lv_coord_t pivot_y)
{
	LV_ASSERT_OBJ(obj, MY_CLASS);

	_ui_video_t * video = (_ui_video_t *)obj;

	if (video->pivot.x != pivot_x || video->pivot.y != pivot_y) {
		video->pivot.x = pivot_x;
		video->pivot.y = pivot_y;

		if (video->src && (video->angle > 0 || video->zoom != LV_SCALE_NONE))
			refresh_image_area(obj);
	}
}

void ui_video_set_rotation(lv_obj_t * obj, int16_t angle)
{
	LV_ASSERT_OBJ(obj, MY_CLASS);

	_ui_video_t * video = (_ui_video_t *)obj;

	while (angle >= 3600) angle -= 3600;
	while (angle < 0) angle += 3600;

	if (angle != video->angle) {
		video->angle = angle;

		if (video->src)
			refresh_image_area(obj);
	}
}

void ui_video_set_scale(lv_obj_t * obj, uint16_t zoom)
{
	LV_ASSERT_OBJ(obj, MY_CLASS);

	_ui_video_t * video = (_ui_video_t *)obj;

	if (zoom == 0) zoom = 1;

	if (zoom != video->zoom) {
		video->zoom = zoom;

		if (video->src)
			refresh_image_area(obj);
	}
}

void ui_video_resume(lv_obj_t *obj)
{
	LV_ASSERT_OBJ(obj, MY_CLASS);
	_ui_video_t * video = (_ui_video_t *)obj;
	if(video->up_timer)
		lv_timer_resume(video->up_timer);
}

void ui_video_pause(lv_obj_t *obj)
{
	LV_ASSERT_OBJ(obj, MY_CLASS);
	_ui_video_t * video = (_ui_video_t *)obj;
	if(video->up_timer)
		lv_timer_pause(video->up_timer);
}

void ui_video_reset(lv_obj_t *obj)
{
	LV_ASSERT_OBJ(obj, MY_CLASS);
	_ui_video_t * video = (_ui_video_t *)obj;
	_ui_video_reset(video);
}

void ui_video_play_cb(lv_obj_t *obj, ui_video_play_cb_t cb)
{
	LV_ASSERT_OBJ(obj, MY_CLASS);
	_ui_video_t * video = (_ui_video_t *)obj;
	video->play_cb = cb;
}
#endif
