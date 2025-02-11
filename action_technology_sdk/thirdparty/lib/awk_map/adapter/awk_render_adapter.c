#include <os_common_api.h>
#include <view_manager.h>
#include <awk_service.h>
#include <awk_render_adapter.h>
#include <awk_system_adapter.h>

//#define AWK_RENDER_DEBUG

OS_MUTEX_DEFINE(awk_render_mutex);
static t_awk_view_buffer_info *render_buffer;
static t_awk_view_buffer_info *background_buffer;
static f_render_commit_cbk commit_callback;
#ifdef AWK_RENDER_DEBUG
static uint32_t frame_start_stc = 0;
static uint32_t frame_spend_time = 0;
static uint32_t tile_start_stc = 0;
static uint32_t tile_spend_time = 0;
static uint32_t tile_cnt = 0;
#endif

void awk_render_init(t_awk_view_buffer_info *render_buffer_info,
	t_awk_view_buffer_info *background_buffer_info, f_render_commit_cbk callback)
{
	if (render_buffer_info) {
		render_buffer = awk_mem_malloc_adapter(sizeof(t_awk_view_buffer_info));
		memcpy(render_buffer, render_buffer_info, sizeof(t_awk_view_buffer_info));
	}

	if (background_buffer_info) {
		background_buffer = awk_mem_malloc_adapter(sizeof(t_awk_view_buffer_info));
		memcpy(background_buffer, background_buffer_info, sizeof(t_awk_view_buffer_info));
		view_manager_acquire_draw_lock(OS_FOREVER);
		begin_draw_path();
		awk_begin_drawing(render_buffer, background_buffer->data);
		view_manager_release_draw_lock();
	}

	if (callback) {
		commit_callback = callback;
	}
}

void awk_render_deinit(void)
{
	if (render_buffer) {
		awk_mem_free_adapter(render_buffer);
		render_buffer = NULL;
	}

	if (background_buffer) {
		awk_mem_free_adapter(background_buffer);
		background_buffer = NULL;
		view_manager_acquire_draw_lock(OS_FOREVER);
		begin_dgpath_free();
		view_manager_release_draw_lock();
	}
}

void awk_render_begin_drawing_adapter(uint32_t map_id, awk_render_context_t status)
{
	printk("begin drawing: map_id = %u\n", map_id);
#ifdef AWK_RENDER_DEBUG
	frame_start_stc = os_uptime_get_32();
	frame_spend_time = 0;
	tile_spend_time = 0;
	tile_cnt = 0;
#endif
	if (render_buffer) {
		view_manager_acquire_draw_lock(OS_FOREVER);
		awk_begin_drawing_bg(map_id, render_buffer);
		view_manager_release_draw_lock();
	}
}

void awk_render_commit_drawing_adapter(uint32_t map_id)
{
    printk("commit drawing: map_id = %u\n", map_id);
	if (commit_callback) {
		commit_callback(map_id);
	}
#ifdef AWK_RENDER_DEBUG
	frame_spend_time = os_uptime_get_32() - frame_start_stc;
	printk("\nawk_render_commit_drawing_callback, frame spend %u ms, tile cnt %d spend %u ms (average %u ms)\n\n",
		frame_spend_time, tile_cnt, tile_spend_time, tile_spend_time / tile_cnt);
#endif
}

void awk_render_point_adapter(uint32_t map_id, awk_point_t *point, uint32_t point_size, const awk_paint_style_t *style)
{
	//printk("awk_render_point_adapter, map_id = %u, point = %p, point_size = %u\n", map_id, point, point_size);
	if (point_size < 1) {
		return;
	}
	t_drawing_points_param param;
	param.map_id = map_id;
	param.points = point;
	param.point_size = point_size;
	param.style = style;
	//to render
	if (render_buffer) {
		view_manager_acquire_draw_lock(OS_FOREVER);
		awk_draw_point(&param, render_buffer);
		view_manager_release_draw_lock();
	}
	return;
}

void awk_render_polyline_adapter(uint32_t map_id, awk_point_t *points, uint32_t point_size, const awk_paint_style_t *style)
{
	//printk("awk_render_polyline_adapter, map_id = %u, points = %p, point_size = %u\n", map_id, points, point_size);
	if (point_size < 2) {
		return;
	}
	t_drawing_points_param param;
	param.map_id = map_id;
	param.points = points;
	param.point_size = point_size;
	param.style = style;
	//to render
	if (render_buffer) {
		view_manager_acquire_draw_lock(OS_FOREVER);
		awk_draw_polyline(&param, render_buffer);
		view_manager_release_draw_lock();
	}
	return;
}

void awk_render_polygon_adapter(uint32_t map_id, awk_point_t *points, uint32_t point_size, const awk_paint_style_t *style)
{
    //printk("awk_render_polygon_adapter: map_id = %u, points = %p, point_size = %u\n", map_id, points, point_size);
	if (point_size < 3) {
		return;
	}
	t_drawing_points_param param;
	param.map_id = map_id;
	param.points = points;
	param.point_size = point_size;
	param.style = style;
	//to render
	if (render_buffer) {
		view_manager_acquire_draw_lock(OS_FOREVER);
		awk_draw_polygon(&param, render_buffer);
		view_manager_release_draw_lock();
	}
	return;
}

void awk_render_bitmap_adapter(uint32_t map_id, awk_rect_area_t area, awk_bitmap_t bitmap, const awk_paint_style_t *style)
{
	printk("awk_render_bitmap_adapter: map_id = %u\n", map_id);
#ifdef AWK_RENDER_DEBUG
	uint32_t stc = os_uptime_get_32();
#endif
	t_draw_bitmap_param param;
	param.map_id = map_id;
	memcpy(&param.area, &area, sizeof(awk_rect_area_t));
	memcpy(&param.bitmap, &bitmap, sizeof(awk_bitmap_t));
	param.style = style;
	//to render
	if (render_buffer) {
		view_manager_acquire_draw_lock(OS_FOREVER);
		awk_draw_bitmap(&param, render_buffer);
		view_manager_release_draw_lock();
	}
#ifdef AWK_RENDER_DEBUG
	printk("awk_render_bitmap_adapter, begin stc:%u, spend %u ms\n", stc, os_uptime_get_32() - stc);
#endif
	return;
}

void awk_render_draw_text_adapter(uint32_t map_id, awk_point_t center, const char *text, const awk_paint_style_t *style)
{
	t_draw_text_param param;
	param.map_id = map_id;
	memcpy(&param.center, &center, sizeof(awk_point_t));
	param.text = text;
	param.style = style;
	//to render
	if (render_buffer) {
		view_manager_acquire_draw_lock(OS_FOREVER);
		awk_draw_text(&param, render_buffer);
		view_manager_release_draw_lock();
	}
	return;
}


void awk_render_color_adapter(uint32_t map_id, awk_rect_area_t area, const awk_paint_style_t *style)
{
	t_draw_color_param param;
	param.map_id = map_id;
	memcpy(&param.area, &area, sizeof(awk_rect_area_t));
	param.style = style;
	//to render
	if (render_buffer) {
		view_manager_acquire_draw_lock(OS_FOREVER);
		awk_draw_color(&param, render_buffer);
		view_manager_release_draw_lock();
	}
	return;
}


bool awk_render_measure_text_adapter(uint32_t map_id, const char *text, const awk_paint_style_t *style, int32_t *width, int32_t *ascender, int32_t *descender)
{
    printk("awk_render_measure_text_adapter: map_id = %u, text = %s, style = %p, width = %p, ascender = %p, descender = %p\n", map_id, text, style, width, ascender, descender);
    return true; // 根据实际情况返回true或false
}

bool awk_map_on_point_begin_draw_callback(uint32_t map_id, int32_t guid)
{
    printk("\n awk_map_on_point_begin_draw_callback, map_id:%u, guid:%d\n", map_id, guid);
	return true;
}

void awk_map_on_point_end_draw_callback(uint32_t map_id, int32_t guid, int32_t status)
{
    printk("\n awk_map_on_point_end_draw_callback, map_id:%u, guid:%d, status:%d\n", map_id, guid, status);
}


bool awk_map_on_line_begin_draw_callback(uint32_t map_id, int32_t guid)
{
    printk("\n awk_map_on_line_begin_draw_callback, map_id:%u, guid:%d\n", map_id, guid);
	return true;
}

void awk_map_on_line_end_draw_callback(uint32_t map_id, int32_t guid, int32_t status)
{
    printk("\n awk_map_on_line_end_draw_callback, map_id:%u, guid:%d, status:%d\n", map_id, guid, status);
}

bool awk_map_on_polygon_begin_draw_callback(uint32_t map_id, int32_t guid)
{
    printk("\n awk_map_on_polygon_begin_draw_callback, map_id:%u, guid:%d\n", map_id, guid);
    return true;
}

void awk_map_on_polygon_end_draw_callback(uint32_t map_id, int32_t guid, int32_t status) {
    printk("\n awk_map_on_polygon_end_draw_callback, map_id:%u, guid:%d, status:%d\n", map_id, guid, status);
}

bool awk_map_on_tile_begin_draw_callback(uint32_t map_id, uint32_t tile_x, uint32_t tile_y, uint32_t zoom) {
    printk("\n awk_map_on_tile_begin_draw_callback, map_id:%u, tile_x:%u, tile_y:%u, zoom:%u\n", map_id, tile_x, tile_y, zoom);
#ifdef AWK_RENDER_DEBUG
	tile_start_stc = os_uptime_get_32();
	printk("tile_begin_draw, stc:%u\n", tile_start_stc);
#endif
    return true;
}

void awk_map_on_tile_end_draw_callback(uint32_t map_id, uint32_t tile_x, uint32_t tile_y, uint32_t zoom, int32_t status) {
    printk("\n awk_map_on_tile_end_draw_callback, map_id:%u, tile_x:%u, tile_y:%u, zoom:%u, status:%d\n", map_id, tile_x, tile_y, zoom, status);
#ifdef AWK_RENDER_DEBUG
	printk("tile_end_draw, spend %u ms\n", os_uptime_get_32() - tile_start_stc);
	tile_spend_time += os_uptime_get_32() - tile_start_stc;
	tile_cnt ++;
#endif
}

bool awk_map_on_poi_begin_draw_callback(uint32_t map_id, uint32_t tile_x, uint32_t tile_y, uint32_t zoom) {
    printk("\n awk_map_on_poi_begin_draw_callback, map_id:%u, tile_x:%u, tile_y:%u, zoom:%u\n", map_id, tile_x, tile_y, zoom);
    return true;
}

void awk_map_on_poi_end_draw_callback(uint32_t map_id, uint32_t tile_x, uint32_t tile_y, uint32_t zoom, int32_t status) {
    printk("\n awk_map_on_poi_end_draw_callback, map_id:%u, tile_x:%u, tile_y:%u, zoom:%u, status:%d\n", map_id, tile_x, tile_y, zoom, status);
}


bool awk_map_on_tile_begin_download_callback(int32_t type, uint32_t tile_x, uint32_t tile_y, uint32_t zoom) {
    printk("\n awk_map_on_tile_begin_download_callback, type:%d, tile_x:%u, tile_y:%u, zoom:%u\n", type, tile_x, tile_y, zoom);
    return true;
}

void awk_map_on_tile_end_download_callback(int32_t type, uint32_t tile_x, uint32_t tile_y, uint32_t zoom, awk_map_tile_response_status_t status) {
    printk("\n awk_map_on_tile_end_download_callback, type:%d, tile_x:%u, tile_y:%u, zoom:%u, status:%d\n", type, tile_x, tile_y, zoom, status);
}

