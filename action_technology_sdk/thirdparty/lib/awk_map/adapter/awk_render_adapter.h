/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __AWK_RENDER_ADAPTER_H__
#define __AWK_RENDER_ADAPTER_H__
#include <awk.h>
#include <string.h>
#include <os_common_api.h>
#include <msg_manager.h>
#include <freetype_font_api.h>
#include "../include/awk_adapter.h"


enum AWK_UI_MSG_EVENT_TYPE {
	MSG_AWK_UI_EVENT = MSG_APP_MESSAGE_START,
};

typedef void (*f_render_commit_cbk)(uint32_t map_id);

typedef struct awk_view_buffer_info {
	uint8_t cf;
    uint16_t width;
    uint16_t height;
	uint16_t stride; /* number of bytes per line */
	uint8_t *data;
    uint32_t data_size;
} t_awk_view_buffer_info;

typedef struct drawing_param {
	sys_dlist_t node;
	uint32_t type;
	void *param;
	uint32_t param_size;
} t_drawing_param;

typedef struct begin_drawing_param {
	uint32_t map_id;
	awk_render_context_t status;
} t_begin_drawing_param;

typedef enum {
	AWK_DRAW_POINT,
	AWK_DRAW_POLYLINE,
	AWK_DRAW_POLYGON,
} e_draw_paints_type;

typedef struct drawing_points_param {
	uint32_t map_id;
	awk_point_t *points;
	uint32_t point_size;
	const awk_paint_style_t *style;
} t_drawing_points_param;

typedef struct draw_bitmap_param {
	uint32_t map_id;
	awk_rect_area_t area;
	awk_bitmap_t bitmap;
	const awk_paint_style_t *style;
} t_draw_bitmap_param;

typedef struct draw_text_param {
	uint32_t map_id;
	awk_point_t center;
	const char *text;
	const awk_paint_style_t *style;
} t_draw_text_param;

typedef struct draw_color_param {
	uint32_t map_id;
	awk_rect_area_t area;
	const awk_paint_style_t *style;
} t_draw_color_param;

void awk_render_init(t_awk_view_buffer_info *render_buffer_info,
	t_awk_view_buffer_info *background_buffer_info, f_render_commit_cbk callback);
void awk_render_deinit(void);
void awk_render_begin_drawing_adapter(uint32_t map_id, awk_render_context_t status);
void awk_render_commit_drawing_adapter(uint32_t map_id);
void awk_render_point_adapter(uint32_t map_id, awk_point_t *point, uint32_t point_size, const awk_paint_style_t *style);
void awk_render_polyline_adapter(uint32_t map_id, awk_point_t *points, uint32_t point_size, const awk_paint_style_t *style);
void awk_render_polygon_adapter(uint32_t map_id, awk_point_t *points, uint32_t point_size, const awk_paint_style_t *style);
void awk_render_bitmap_adapter(uint32_t map_id, awk_rect_area_t area, awk_bitmap_t bitmap, const awk_paint_style_t *style);
void awk_render_draw_text_adapter(uint32_t map_id, awk_point_t center, const char *text, const awk_paint_style_t *style);
void awk_render_color_adapter(uint32_t map_id, awk_rect_area_t area, const awk_paint_style_t *style);
bool awk_render_measure_text_adapter(uint32_t map_id, const char *text, const awk_paint_style_t *style, int32_t *width, int32_t *ascender, int32_t *descender);

bool awk_map_on_point_begin_draw_callback(uint32_t map_id, int32_t guid);
void awk_map_on_point_end_draw_callback(uint32_t map_id, int32_t guid, int32_t status);
bool awk_map_on_line_begin_draw_callback(uint32_t map_id, int32_t guid);
void awk_map_on_line_end_draw_callback(uint32_t map_id, int32_t guid, int32_t status);
bool awk_map_on_polygon_begin_draw_callback(uint32_t map_id, int32_t guid);
void awk_map_on_polygon_end_draw_callback(uint32_t map_id, int32_t guid, int32_t status);
bool awk_map_on_tile_begin_draw_callback(uint32_t map_id, uint32_t tile_x, uint32_t tile_y, uint32_t zoom);
void awk_map_on_tile_end_draw_callback(uint32_t map_id, uint32_t tile_x, uint32_t tile_y, uint32_t zoom, int32_t status);
bool awk_map_on_poi_begin_draw_callback(uint32_t map_id, uint32_t tile_x, uint32_t tile_y, uint32_t zoom);
void awk_map_on_poi_end_draw_callback(uint32_t map_id, uint32_t tile_x, uint32_t tile_y, uint32_t zoom, int32_t status);
bool awk_map_on_tile_begin_download_callback(int32_t type, uint32_t tile_x, uint32_t tile_y, uint32_t zoom);
void awk_map_on_tile_end_download_callback(int32_t type, uint32_t tile_x, uint32_t tile_y, uint32_t zoom, awk_map_tile_response_status_t status);

void awk_draw_bitmap(t_draw_bitmap_param *param,t_awk_view_buffer_info *buffer_info);
void awk_draw_text(t_draw_text_param *param, t_awk_view_buffer_info *buffer_info);
void awk_draw_polygon(t_drawing_points_param *param, t_awk_view_buffer_info *buffer_info);
void awk_draw_polyline(t_drawing_points_param *param, t_awk_view_buffer_info *buffer_info);
void awk_draw_point(t_drawing_points_param *param, t_awk_view_buffer_info *buffer_info);
void awk_draw_color(t_draw_color_param *param, t_awk_view_buffer_info *buffer_info);

int awk_draw_textsvg2(t_draw_text_param *param, t_awk_view_buffer_info *buffer_info);
void awk_begin_drawing(t_awk_view_buffer_info *buffer_info,uint8_t *addr);
void begin_draw_path(void);
void awk_begin_drawing_bg(uint32_t map_id,t_awk_view_buffer_info *buffer_info);
int awk_font();
int awk_font_render(unsigned long unicode);
void awk_font_free();
void begin_dgpath_free();
int awk_font_render(unsigned long unicode);
int resolver_awk_font_metric(freetype_font_t*font,unsigned long unicode,uint8_t *w,uint8_t*h,uint8_t *advance,uint8_t *x,uint8_t *y);
freetype_font_t* resolver_awk_font_init();
uint8_t* resolver_awk_font(freetype_font_t*font,unsigned long unicode);
int resolver_awk_font_close(freetype_font_t * font);




#endif
