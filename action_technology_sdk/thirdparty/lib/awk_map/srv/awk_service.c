#include <awk_service.h>
#include <string.h>
#include <kernel.h>
#include <stdio.h>
#include <srv_manager.h>
#include <msg_manager.h>
#include <thread_timer.h>

#define CONFIG_AWK_SRV_STACKSIZE 20480
#define CONFIG_AWK_SVR_PRIORITY 5
#define AWK_SERVICE_NAME "awk_service"
struct thread_timer awk_render_timer;
#define AWK_RENDER_TIMER_PERIOD 30

enum {
	AWK_MSG_START = MSG_SRV_MESSAGE_START,
	AWK_MAP_CALL_FUN_EVENT,
	AWK_MAP_TOUCH_EVENT,
	AWK_MAP_RENDER_EVENT,
};

enum AWK_MAP_CALL_FUN_CMD {
	AWK_USER_WORK,
	AWK_MAP_INIT,
	AWK_MAP_CREATE_VIEW,
	AWK_MAP_DESTROY_VIEW,
	AWK_MAP_UNINIT,
	AWK_MAP_SET_STATUS,
	AWK_MAP_GET_POSTURE,
	AWK_MAP_ADD_OVERLAY,
	AWK_MAP_ADD_TEXTURE,
	AWK_MAP_UPDATE_TEXTURE,
	AWK_MAP_REMOVE_TEXTURE,
};

enum AWK_MAP_RENDER_CMD {
	AWK_RENDER_START,
	AWK_RENDER_STOP,
};

static int awk_srv_send_msg(uint32_t type, uint32_t cmd, int value, void *callback, uint8_t is_sync, uint8_t is_discardable)
{
	struct app_msg msg = {0};
	os_sem return_notify;
	int ret;

	if (is_sync) {
		os_sem_init(&return_notify, 0, 1);
	}

	msg.type = type;
	msg.cmd = cmd;
	msg.value = value;
	msg.callback = callback;

	if (is_sync) {
		msg.sync_sem = &return_notify;
	}

	if (is_discardable) {
		ret = send_async_msg_discardable(AWK_SERVICE_NAME, &msg);
	} else {
		ret = send_async_msg(AWK_SERVICE_NAME, &msg);
	}
	if (false == ret) {
		return -EBUSY;
	}

	if (is_sync) {
		if (os_sem_take(&return_notify, OS_FOREVER)) {
			return -ETIME;
		}
	}

	return 0;
}

int awk_service_start(void)
{
	if (!srv_manager_check_service_is_actived(AWK_SERVICE_NAME)) {
		return (srv_manager_active_service(AWK_SERVICE_NAME) == true) ? 0 : -1;
	}

	return 0;
}

int awk_service_stop(void)
{
	if (srv_manager_check_service_is_actived(AWK_SERVICE_NAME)) {
		return (srv_manager_exit_service(AWK_SERVICE_NAME) == true) ? 0 : -1;
	}

	return 0;
}

#include <utils/acts_ringbuf.h>
static t_awk_touch_data pointer_scan_rbuf_data[25];
static ACTS_RINGBUF_DEFINE(pointer_scan_rbuf, pointer_scan_rbuf_data, sizeof(pointer_scan_rbuf_data));
void awk_srv_map_touch_msg_callback(struct app_msg* msg, int val, void *ptr)
{
	if (msg->value) {
		awk_mem_free_adapter((void *)msg->value);
	}
}

void awk_srv_map_touch_data_input(int mapid, e_awk_map_touch_type type, int16_t x,int16_t y)
{
	t_awk_touch_data data;
	data.map_id = mapid;
	data.x = x;
	data.y = y;

	if (type == AWK_MAP_TOUCH_UPDATE) {
		int put_size = acts_ringbuf_put(&pointer_scan_rbuf, &data, sizeof(data));
		if (put_size < sizeof(data)) {
			printk("awk touch drop:(%d,%d)\n", data.x, data.y);
			return;
		}
		awk_srv_send_msg(AWK_MAP_TOUCH_EVENT, type, 0, NULL, false, true);
	} else {
		t_awk_touch_data *p_data = awk_mem_malloc_adapter(sizeof(t_awk_touch_data));
		memcpy(p_data, &data, sizeof(t_awk_touch_data));
		awk_srv_send_msg(AWK_MAP_TOUCH_EVENT, type, (int)p_data, awk_srv_map_touch_msg_callback, false, false);
	}
}

static void awk_srv_map_touch_data_process(e_awk_map_touch_type type, void *user_data)
{
	t_awk_touch_data data;
	t_awk_touch_data *p_data = user_data;
	uint32_t len;
	int ret = 0;

	if (type == AWK_MAP_TOUCH_UPDATE) {
		for (;;) {
			len = acts_ringbuf_get(&pointer_scan_rbuf, &data, sizeof(data));
			if (len <= 0) {
				return;
			}
			ret = awk_map_touch_update(data.map_id, data.x, data.y);
		}
	} else if (type == AWK_MAP_TOUCH_START) {
		ret = awk_map_touch_begin(p_data->map_id, p_data->x, p_data->y);
	} else if (type == AWK_MAP_TOUCH_END) {
		ret = awk_map_touch_end(p_data->map_id, p_data->x, p_data->y);
	} else if (type == AWK_MAP_TOUCH_CLICKED) {
		int32_t overlay_count = 0;
    	awk_map_point_overlay_t **overlays = awk_map_click_points(p_data->map_id, p_data->x, p_data->y, &overlay_count);
		printk("overlay_count:%d\n", overlay_count);
		if(overlays != NULL && overlay_count > 0) {
	        overlays[0]->base_overlay.is_focus = true;
	        awk_map_update_overlay(p_data->map_id, (awk_map_base_overlay_t *)overlays[0]);
	        awk_map_release_overlays((awk_map_base_overlay_t **)overlays, overlay_count);
	    } else {
	        overlay_count = awk_map_get_overlay_count(p_data->map_id);
	        for(int32_t i = 0; i < overlay_count; ++i) {
	            awk_map_base_overlay_t *overlay = (awk_map_base_overlay_t *)awk_map_get_overlay(p_data->map_id, i);
	            overlay->is_focus = false;
	            awk_map_update_overlay(p_data->map_id, overlay);
	        }
	    }
	}

	//awk_map_do_render();
}
//call function by srv
static void call_func_event_handler(int msg_id, void *param, uint32_t param_size, void *cbk);
static void awk_call_func_by_msg(int msg_id, void *param, uint32_t param_size, void *cbk)
{
	//copy params
	void *msg_ptr = awk_mem_malloc_adapter(sizeof(void *) + sizeof(uint32_t) + param_size);
	uint8_t *tmp_ptr = msg_ptr;
	memcpy(tmp_ptr, &cbk, sizeof(cbk));
	tmp_ptr += sizeof(cbk);
	memcpy(tmp_ptr, &param_size, sizeof(param_size));
	if (param_size > 0) {
		tmp_ptr += sizeof(param_size);
		memcpy(tmp_ptr, param, param_size);
	}
	//send massage to awk service
	awk_srv_send_msg(AWK_MAP_CALL_FUN_EVENT, msg_id, (int)msg_ptr, NULL, false, false);
}

static void awk_srv_call_func_msg_handler(int msg_id, int msg_ptr)
{
	//receive params
	void *cbk;
	void *param = NULL;
	uint32_t param_size;
	uint8_t *tmp_ptr = (uint8_t *)msg_ptr;
	memcpy(&cbk, tmp_ptr, sizeof(cbk));
	tmp_ptr += sizeof(cbk);
	memcpy(&param_size, tmp_ptr, sizeof(param_size));
	if (param_size > 0) {
		tmp_ptr += sizeof(param_size);
		param = tmp_ptr;
	}
	//handle msg to call awk function
	call_func_event_handler(msg_id, param, param_size, cbk);
	//release msg memory
	awk_mem_free_adapter((void *)msg_ptr);
}

static void awk_init_handler(f_awk_init_cbk cbk)
{
	int32_t ret;
	awk_context_t context;
	memset(&context, 0, sizeof(awk_context_t));  //务必先memset
	context.device_id = "123";
	context.key = "da4332849a96e764f0c66b2a11aff836";
	context.root_dir = CONFIG_APP_FAT_DISK;            //SDK内部文件夹根路径
	context.offline_map_dir = CONFIG_APP_FAT_DISK"/map";
	context.tile_disk_cache_max_size = 0;         //!< tile磁盘缓存最大空间，单位: MB
	context.tile_mem_cache_max_size = 1536;          //!< tile内存缓存最大空间，单位：KB
	context.tile_style = AWK_MAP_TILE_STYLE_STANDARD_GRID;
	context.tile_load_mode = AWK_MAP_TILE_LOAD_OFFLINE;
	context.tile_pixel_mode = AWK_PIXEL_MODE_RGB_565;
	context.tile_clip_load = false;
	context.tile_background_custom_draw = true;
	context.fast_memory_adapter.mem_malloc = awk_fast_mem_malloc_adapter,
	context.fast_memory_adapter.mem_free = awk_fast_mem_free_adapter,
	//内存相关适配
	context.memory_adapter.mem_malloc = awk_mem_malloc_adapter;
	context.memory_adapter.mem_free = awk_mem_free_adapter;
	context.memory_adapter.mem_calloc = awk_mem_calloc_adapter;
	context.memory_adapter.mem_realloc = awk_mem_realloc_adapter;
	//文件相关适配
	context.file_adapter.file_open = awk_file_open_adapter;
	context.file_adapter.file_close = awk_file_close_adapter;
	context.file_adapter.file_read = awk_file_read_adapter;
	context.file_adapter.file_write = awk_file_write_adapter;
	context.file_adapter.file_mkdir = awk_file_mkdir_adapter;
	context.file_adapter.file_exists = awk_file_exists_adapter;
	context.file_adapter.file_remove = awk_file_remove_adapter;
	context.file_adapter.file_opendir = awk_file_opendir_adapter;
	context.file_adapter.file_closedir = awk_file_closedir_adapter;
	context.file_adapter.file_readdir = awk_file_readdir_adapter;
	context.file_adapter.file_seek = awk_file_seek_adapter;
	context.file_adapter.file_flush = awk_file_flush_adapter;
	context.file_adapter.file_rmdir = awk_file_rmdir_adapter;
	context.file_adapter.file_dir_exists = awk_file_dir_exists_adapter;
	context.file_adapter.file_get_size = awk_file_get_size_adapter;
	context.file_adapter.file_get_last_access = awk_file_get_last_access_adapter;
	context.file_adapter.file_rename = awk_file_rename_adapter;
	context.file_adapter.file_unzip = awk_file_unzip_adapter;
	//网络相关适配
	context.network_adapter.send = awk_aos_network_adapter_send_adapter;
	context.network_adapter.cancel = awk_aos_network_adapter_cancel_adapter;
	//渲染绘制相关适配
	context.render_adapter.begin_drawing = awk_render_begin_drawing_adapter;
	context.render_adapter.commit_drawing = awk_render_commit_drawing_adapter;
	context.render_adapter.draw_point = awk_render_point_adapter;
	context.render_adapter.draw_polyline = awk_render_polyline_adapter;
	context.render_adapter.draw_polygon = awk_render_polygon_adapter;
	context.render_adapter.draw_bitmap = awk_render_bitmap_adapter;
	context.render_adapter.draw_color = awk_render_color_adapter;
	context.render_adapter.draw_text = awk_render_draw_text_adapter;
	context.render_adapter.measure_text = awk_render_measure_text_adapter;
	//线程相关适配
	context.thread_adapter.get_thread_id = awk_get_thread_id_adapter;
	//其他系统相关适配
	context.system_adapter.get_system_time = awk_get_system_time_adapter;
	context.system_adapter.log_printf = awk_printf_adapter;
	ret = awk_init(&context);
	printf("awk_init, %d\n", ret);
	//return result
	t_awk_init_result result;
	result.ret = ret;
	if (cbk) {
		cbk(result);
	}
}

static void awk_map_create_view_handler(t_awk_srv_map_create_param *param, f_awk_map_create_view_cbk cbk)
{
	t_awk_srv_map_create_result result;
	result.map_id = awk_map_create_view(param->view_param);             //创建地图，返回地图id
	printf("awk_map_create_view, ret: %d, %d, %d\n", result.map_id, param->view_param.port.height, param->view_param.port.width);
	if (result.map_id < 0) {
		goto out;
	}
	awk_map_set_center(result.map_id, param->map_center);
	awk_map_set_level(result.map_id, param->zoom);

	awk_map_render_callback_t render_callback;
	render_callback.on_point_begin_draw = awk_map_on_point_begin_draw_callback;
	render_callback.on_point_end_draw = awk_map_on_point_end_draw_callback;
	render_callback.on_line_begin_draw = awk_map_on_line_begin_draw_callback,
	render_callback.on_line_end_draw = awk_map_on_line_end_draw_callback,
	render_callback.on_polygon_begin_draw = awk_map_on_polygon_begin_draw_callback,
	render_callback.on_polygon_end_draw = awk_map_on_polygon_end_draw_callback,
	render_callback.on_poi_begin_draw = awk_map_on_poi_begin_draw_callback,
	render_callback.on_poi_end_draw = awk_map_on_poi_end_draw_callback,
	render_callback.on_tile_begin_draw = awk_map_on_tile_begin_draw_callback;
	render_callback.on_tile_end_draw = awk_map_on_tile_end_draw_callback;
	awk_map_set_render_callback(render_callback);
out:
	if (cbk) {
		cbk(result);
	}
}

static void awk_map_destroy_view_handler(t_awk_map_destroy_param *param, f_awk_map_destroy_view_cbk cbk)
{
	t_awk_map_destroy_result result;
	result.ret = awk_map_destroy_view(param->map_id);
	if (cbk) {
		cbk(result);
	}
}

static void awk_uninit_handler(f_awk_uninit_cbk cbk)
{
	t_awk_uninit_result result;
	result.ret = awk_uninit();
	if (cbk) {
		cbk(result);
	}
}

static void awk_add_overlay_handler(t_awk_map_overlay_param *param)
{
	int ret = -1;
	awk_map_point_overlay_t point_overlay = {0};
	awk_map_polyline_overlay_t polyline_overlay = {0};
	awk_map_polygon_overlay_t polygon_overlay = {0};

	switch (param->type) {
		case AWK_POINT_OVERLAY:
			ret = awk_map_init_point_overlay(&point_overlay);
			if (ret) {
				return;
			}
			memcpy(&param->point_overlay.base_overlay, &point_overlay.base_overlay, sizeof(awk_map_base_overlay_t));
			printk("awk_add_overlay_handler, add point, mapid:%d, id %d\n", param->map_id, param->point_overlay.normal_marker.icon_texture.texture_id);
			ret = awk_map_add_overlay((uint32_t)param->map_id, (awk_map_base_overlay_t *)&param->point_overlay);
			break;
		case AWK_POLYLINE_OVERLAY:
			ret = awk_map_init_line_overlay(&polyline_overlay);
			if (ret) {
				return;
			}
			memcpy(&param->polyline_overlay.base_overlay, &polyline_overlay.base_overlay, sizeof(awk_map_base_overlay_t));
			ret = awk_map_add_overlay((uint32_t)param->map_id,(awk_map_base_overlay_t *)&param->polyline_overlay);
			if (param->polyline_overlay.points) {
				awk_mem_free_adapter(param->polyline_overlay.points);
			}
			if (param->polyline_overlay.point_flags) {
				awk_mem_free_adapter(param->polyline_overlay.point_flags);
			}
			break;
		case AWK_POLYGON_OVERLAY:
			ret = awk_map_init_polygon_overlay(&polygon_overlay);
			if (ret) {
				return;
			}
			memcpy(&param->polygon_overlay.base_overlay, &polygon_overlay.base_overlay, sizeof(awk_map_base_overlay_t));
			ret = awk_map_add_overlay((uint32_t)param->map_id,(awk_map_base_overlay_t *)&param->polygon_overlay);
			if (param->polygon_overlay.points) {
				awk_mem_free_adapter(param->polygon_overlay.points);
			}
			break;
		default:
			break;
	}
	printk("awk_add_overlay_handler, type %d, ret %d\n", param->type, ret);
}

static void awk_map_set_status_handler(t_awk_map_set_status_param *param)
{
	int ret = 0;
	switch (param->type) {
		case AWK_MAP_SET_CENTER:
			ret = awk_map_set_center(param->map_id, param->coord2d);
			printk("awk_map_set_center, lon:%f, lat:%f, ret:%d\n", param->coord2d.lon, param->coord2d.lat, ret);
			break;
		case AWK_MAP_SET_LEVEL:
			ret = awk_map_set_level(param->map_id, param->level);
			printk("awk_map_set_level, level:%f, ret:%d\n", param->level, ret);
			break;
		case AWK_MAP_SET_ROLL_ANGLE:
			ret = awk_map_set_roll_angle(param->map_id, param->roll_angle);
			printk("awk_map_set_level, roll_angle:%f, ret:%d\n", param->roll_angle, ret);
			break;
		case AWK_MAP_SET_VIEW_PORT:
			ret = awk_map_set_view_port(param->map_id, param->view_port);
			printk("awk_map_set_view_port, w:%d, h:%d, ret:%d\n", param->view_port.width, param->view_port.height, ret);
			break;
		default:
			printk("awk_set_status_handler, unknow type!!\n");
			ret = -1;
			break;
	}

}

static void awk_map_get_posture_handler(uint32_t *param, f_awk_map_get_posture_cbk cbk)
{
	int ret = 0;
	awk_map_posture_t posture;
	ret = awk_map_get_posture(*param, &posture);
	if (ret == 0 && cbk) {
		(cbk)(posture);
	}
}

static void call_func_event_handler(int msg_id, void *param, uint32_t param_size, void *cbk)
{
	int32_t ret = 0;

	switch (msg_id) {
		case AWK_USER_WORK:
			if (cbk) {
				((f_awk_user_work_cbk)cbk)(param, param_size);
			}
			break;
		case AWK_MAP_INIT:
			awk_init_handler(cbk);
			break;
		case AWK_MAP_CREATE_VIEW:
			awk_map_create_view_handler(param, cbk);
			break;
		case AWK_MAP_DESTROY_VIEW:
			awk_map_destroy_view_handler(param, cbk);
			break;
		case AWK_MAP_UNINIT:
			awk_uninit_handler(cbk);
			break;
		case AWK_MAP_ADD_OVERLAY:
			awk_add_overlay_handler(param);
			break;
		case AWK_MAP_ADD_TEXTURE:
			ret = awk_map_add_texture((const awk_map_texture_data_t *)param);
			printk("texture_id %d\n", ret);
			if (cbk) {
				((f_awk_map_add_texture_cbk)cbk)(ret);
			}
			break;
		case AWK_MAP_REMOVE_TEXTURE:
			ret = awk_map_remove_texture(*((uint32_t *)param));
			printk("remove texture_id %d, ret:%d\n", *((uint32_t *)param), ret);
			if (cbk) {
				((f_awk_map_add_texture_cbk)cbk)(ret);
			}
			break;
		case AWK_MAP_SET_STATUS:
			awk_map_set_status_handler(param);
			break;
		case AWK_MAP_GET_POSTURE:
			awk_map_get_posture_handler(param, cbk);
			break;
		default:
			break;
	}
}

void awk_init_async(f_awk_init_cbk cbk)
{
	awk_call_func_by_msg(AWK_MAP_INIT, NULL, 0, cbk);
}

void awk_map_create_view_async(t_awk_srv_map_create_param param, f_awk_map_create_view_cbk cbk)
{
	awk_call_func_by_msg(AWK_MAP_CREATE_VIEW, &param, sizeof(param), cbk);
}

void awk_map_destroy_view_async(t_awk_map_destroy_param param, f_awk_map_destroy_view_cbk cbk)
{
	awk_call_func_by_msg(AWK_MAP_DESTROY_VIEW, &param, sizeof(param), cbk);
}

void awk_uninit_async(f_awk_uninit_cbk cbk)
{
	awk_call_func_by_msg(AWK_MAP_UNINIT, NULL, 0, cbk);
}

void awk_map_add_point_overlay_async(int map_id, awk_map_point_overlay_t pointOverlay)
{
	t_awk_map_overlay_param overlay_param = {0};
	overlay_param.map_id = map_id;
	overlay_param.type = AWK_POINT_OVERLAY;
	memcpy(&overlay_param.point_overlay, &pointOverlay, sizeof(overlay_param.point_overlay));
	awk_call_func_by_msg(AWK_MAP_ADD_OVERLAY, &overlay_param, sizeof(overlay_param), NULL);
}

void awk_map_add_polyline_overlay_async(int map_id, awk_map_polyline_overlay_t lineOverlay)
{
	t_awk_map_overlay_param overlay_param = {0};
	overlay_param.map_id = map_id;
	overlay_param.type = AWK_POLYLINE_OVERLAY;
	memcpy(&overlay_param.polyline_overlay, &lineOverlay, sizeof(overlay_param.polyline_overlay));
	if (lineOverlay.points) {
		overlay_param.polyline_overlay.points = awk_mem_malloc_adapter(overlay_param.polyline_overlay.point_size * sizeof(awk_map_coord2d_t));
		memcpy(overlay_param.polyline_overlay.points, lineOverlay.points, overlay_param.polyline_overlay.point_size * sizeof(awk_map_coord2d_t));
	}
	if (lineOverlay.point_flags) {
		overlay_param.polyline_overlay.point_flags = awk_mem_malloc_adapter(overlay_param.polyline_overlay.point_flags_size * sizeof(uint8_t));
		memcpy(overlay_param.polyline_overlay.point_flags, lineOverlay.point_flags, overlay_param.polyline_overlay.point_flags_size * sizeof(uint8_t));
	}
	awk_call_func_by_msg(AWK_MAP_ADD_OVERLAY, &overlay_param, sizeof(overlay_param), NULL);
}

void awk_map_add_polygon_overlay_async(int map_id, awk_map_polygon_overlay_t polygonOverlay)
{
	t_awk_map_overlay_param overlay_param = {0};
	overlay_param.map_id = map_id;
	overlay_param.type = AWK_POLYGON_OVERLAY;
	memcpy(&overlay_param.polygon_overlay, &polygonOverlay, sizeof(overlay_param.polygon_overlay));
	if (polygonOverlay.points) {
		overlay_param.polygon_overlay.points = awk_mem_malloc_adapter(overlay_param.polygon_overlay.point_size * sizeof(awk_map_coord2d_t));
		memcpy(overlay_param.polygon_overlay.points, polygonOverlay.points, overlay_param.polygon_overlay.point_size * sizeof(awk_map_coord2d_t));
	}
	awk_call_func_by_msg(AWK_MAP_ADD_OVERLAY, &overlay_param, sizeof(overlay_param), NULL);
}

void awk_map_add_texture_async(const awk_map_texture_data_t *texture_data, f_awk_map_add_texture_cbk cbk)
{
	awk_call_func_by_msg(AWK_MAP_ADD_TEXTURE, (void *)texture_data, sizeof(awk_map_texture_data_t), cbk);
}

void awk_map_remove_texture_async(uint32_t texture_id)
{
	awk_call_func_by_msg(AWK_MAP_REMOVE_TEXTURE, &texture_id, sizeof(texture_id), NULL);
}

void awk_map_set_center_async(int map_id, awk_map_coord2d_t coord2d)
{
	t_awk_map_set_status_param map_status;
	map_status.map_id = map_id;
	map_status.type = AWK_MAP_SET_CENTER;
	memcpy(&map_status.coord2d, &coord2d, sizeof(map_status.coord2d));
	awk_call_func_by_msg(AWK_MAP_SET_STATUS, &map_status, sizeof(map_status), NULL);
}

void awk_map_set_level_async(int map_id, float level)
{
	t_awk_map_set_status_param map_status;
	map_status.map_id = map_id;
	map_status.type = AWK_MAP_SET_LEVEL;
	memcpy(&map_status.level, &level, sizeof(map_status.level));
	awk_call_func_by_msg(AWK_MAP_SET_STATUS, &map_status, sizeof(map_status), NULL);
}

void awk_map_set_roll_angle_async(int map_id, float roll_angle)
{
	t_awk_map_set_status_param map_status;
	map_status.map_id = map_id;
	map_status.type = AWK_MAP_SET_ROLL_ANGLE;
	memcpy(&map_status.roll_angle, &roll_angle, sizeof(map_status.roll_angle));
	awk_call_func_by_msg(AWK_MAP_SET_STATUS, &map_status, sizeof(map_status), NULL);
}

void awk_map_set_view_port_async(int map_id, awk_map_view_port_t view_port)
{
	t_awk_map_set_status_param map_status;
	map_status.map_id = map_id;
	map_status.type = AWK_MAP_SET_VIEW_PORT;
	memcpy(&map_status.view_port, &view_port, sizeof(map_status.view_port));
	awk_call_func_by_msg(AWK_MAP_SET_STATUS, &map_status, sizeof(map_status), NULL);
}

void awk_map_get_posture_async(uint32_t map_id, f_awk_map_get_posture_cbk cbk)
{
	awk_call_func_by_msg(AWK_MAP_GET_POSTURE, &map_id, sizeof(map_id), cbk);
}

void awk_service_user_work(f_awk_user_work_cbk cbk, void *user_data, uint32_t data_size)
{
	awk_call_func_by_msg(AWK_USER_WORK, user_data, data_size, cbk);
}

void awk_service_render_start(void)
{
	awk_srv_send_msg(AWK_MAP_RENDER_EVENT, AWK_RENDER_START, 0, NULL, true, false);
}

void awk_service_render_stop(void)
{
	awk_srv_send_msg(AWK_MAP_RENDER_EVENT, AWK_RENDER_STOP, 0, NULL, true, false);
}

static void awk_srv_render_msg_handler(int msg_id)
{
	switch (msg_id) {
		case AWK_RENDER_START:
			thread_timer_start(&awk_render_timer, AWK_RENDER_TIMER_PERIOD, AWK_RENDER_TIMER_PERIOD);
			break;
		case AWK_RENDER_STOP:
			thread_timer_stop(&awk_render_timer);
			break;
		default:
			break;
	}
}

static void awk_map_render_timer_handler(struct thread_timer *ttimer, void *expiry_fn_arg)
{
	int ret;
	ret = awk_map_do_render();
	if (ret) {
		SYS_LOG_ERR("map render err:%d\n", ret);
	}
}

static void awk_service_main_loop(void *parama1, void *parama2, void *parama3)
{
	struct app_msg msg = {0};
	bool terminaltion = false;
	bool suspended = false;
	int timeout;

	SYS_LOG_INF("awk service enter");
	thread_timer_init(&awk_render_timer, awk_map_render_timer_handler, NULL);
	while (!terminaltion) {
		timeout = suspended ? OS_FOREVER : thread_timer_next_timeout();
		if (receive_msg(&msg, timeout)) {
			SYS_LOG_INF("awk service msg %d %d",msg.type, msg.cmd);
			switch (msg.type) {
			case MSG_INIT_APP:
				break;
			case MSG_EXIT_APP:
			 	terminaltion = true;
			 	break;
			case AWK_MAP_CALL_FUN_EVENT:
				awk_srv_call_func_msg_handler(msg.cmd, msg.value);
			 	break;
			case AWK_MAP_TOUCH_EVENT:
				awk_srv_map_touch_data_process(msg.cmd, (void *)msg.value);
				break;
			case AWK_MAP_RENDER_EVENT:
				awk_srv_render_msg_handler(msg.cmd);
				break;
			default:
				break;
			}

			if (msg.callback != NULL) {
				msg.callback(&msg, 0, NULL);
			}

			if (msg.sync_sem != NULL) {
				os_sem_give(msg.sync_sem);
			}
		}
		thread_timer_handle_expired();
	}
}

char __aligned(ARCH_STACK_PTR_ALIGN) awk_srv_stack_area[CONFIG_AWK_SRV_STACKSIZE];
SERVICE_DEFINE(awk_service, awk_srv_stack_area, sizeof(awk_srv_stack_area), \
				CONFIG_AWK_SVR_PRIORITY, BACKGROUND_APP, \
				NULL, NULL, NULL, awk_service_main_loop);

