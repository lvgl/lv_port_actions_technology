#ifndef _AWK_SERVICE_H
#define _AWK_SERVICE_H

#include <awk.h>
#include <awk_adapter.h>
#include <awk_render_adapter.h>
#include <awk_system_adapter.h>
/* awk touch */
typedef enum {
	AWK_MAP_TOUCH_START,
	AWK_MAP_TOUCH_UPDATE,
	AWK_MAP_TOUCH_END,
	AWK_MAP_TOUCH_CLICKED,
} e_awk_map_touch_type;

typedef struct {
	int map_id;
	int16_t x;
	int16_t y;
} t_awk_touch_data;

void awk_srv_map_touch_data_input(int mapid, e_awk_map_touch_type state, int16_t x,int16_t y);

/* awk init */
typedef struct awk_init_result {
	//output
	int ret;
} t_awk_init_result;

typedef void (*f_awk_init_cbk)(t_awk_init_result ret);

/* awk create map view*/
typedef struct awk_srv_map_create_param {
	//input
	awk_map_view_param_t view_param;
	awk_map_coord2d_t map_center;
    float zoom;
} t_awk_srv_map_create_param;

typedef struct awk_srv_map_create_result {
	//output
	int map_id;
} t_awk_srv_map_create_result;

typedef void (*f_awk_map_create_view_cbk)(t_awk_srv_map_create_result result);

/* awk destroy map view*/
typedef struct awk_map_destroy_param {
	//input
	int map_id;
} t_awk_map_destroy_param;

typedef struct awk_map_destroy_result {
	//output
	int ret;
} t_awk_map_destroy_result;

typedef void (*f_awk_map_destroy_view_cbk)(t_awk_map_destroy_result result);



/* awk uninit */
typedef struct awk_uninit_result {
	//output
	int ret;
} t_awk_uninit_result;

typedef void (*f_awk_uninit_cbk)(t_awk_uninit_result result);


typedef enum {
	AWK_POINT_OVERLAY,
	AWK_POLYLINE_OVERLAY,
	AWK_POLYGON_OVERLAY,
} e_overlay_type;

typedef struct awk_map_overlay_param {
	//input
	int map_id;
	e_overlay_type type;
	union {
		awk_map_point_overlay_t point_overlay;
		awk_map_polyline_overlay_t polyline_overlay;
		awk_map_polygon_overlay_t polygon_overlay;
	};
} t_awk_map_overlay_param;

typedef void (*f_awk_map_add_overlay_cbk)(int ret);

typedef struct awk_map_add_polygon_overlay_param {
	//input
	int map_id;
	awk_map_polygon_overlay_t overlay;
} t_awk_map_add_polygon_overlay_param;

typedef struct awk_map_add_polyline_overlay_param {
	//input
	int map_id;
	awk_map_polyline_overlay_t overlay;
} t_awk_map_add_polyline_overlay_param;

typedef void (*f_awk_map_add_texture_cbk)(int32_t texture_id);

typedef enum {
	AWK_MAP_SET_CENTER,
	AWK_MAP_SET_LEVEL,
	AWK_MAP_SET_ROLL_ANGLE,
	AWK_MAP_SET_VIEW_PORT,
} e_set_status_type;

typedef struct awk_map_set_status_param {
	//input
	int map_id;
	e_set_status_type type;
	union {
		awk_map_view_port_t view_port;
		float roll_angle;
		float level;
		awk_map_coord2d_t coord2d;
	};
} t_awk_map_set_status_param;

typedef void (*f_awk_map_get_posture_cbk)(awk_map_posture_t posture);

typedef void (*f_awk_user_work_cbk)(void *user_data, uint32_t data_size);
/**
 * 将在awk主线程执行用户自定义的回调函数，可在其他线程可调用。
 *
 * @param cbk 指向自定义回调函数的指针，该回调函数将在awk主线程上下文被调用。
 * @param user_data 传递给回调函数的用户自定义数据。
 * @param data_size 用户数据的大小。
 */
void awk_service_user_work(f_awk_user_work_cbk cbk, void *user_data, uint32_t data_size);

/**
 * 将通知awk主线程开启高德地图渲染工作。
 *
 * 注意：请先初始化好相关资源，再调此接口。
 */
void awk_service_render_start(void);

/**
 * 将通知awk主线程停止高德地图渲染工作。
 *
 * 注意：关闭高德地图功能时先调此接口，再调用摧毁地图和反初始化接口。
 */
void awk_service_render_stop(void);

/**
 * 以下为封装好的异步调用高德地图sdk接口的函数，可在其他线程调用。
 *
 * 注意：只封装部分功能函数接口，如果用户需要自定义调用逻辑，请使用awk_service_user_work函数接口实现
 */
void awk_init_async(f_awk_init_cbk cbk);
void awk_uninit_async(f_awk_uninit_cbk cbk);
void awk_map_create_view_async(t_awk_srv_map_create_param param, f_awk_map_create_view_cbk cbk);
void awk_map_destroy_view_async(t_awk_map_destroy_param param, f_awk_map_destroy_view_cbk cbk);
void awk_map_add_point_overlay_async(int map_id, awk_map_point_overlay_t pointOverlay);
void awk_map_add_polyline_overlay_async(int map_id, awk_map_polyline_overlay_t lineOverlay);
void awk_map_add_polygon_overlay_async(int map_id, awk_map_polygon_overlay_t polygonOverlay);
void awk_map_add_texture_async(const awk_map_texture_data_t *texture_data, f_awk_map_add_texture_cbk cbk);
void awk_map_remove_texture_async(uint32_t texture_id);
void awk_map_set_center_async(int map_id, awk_map_coord2d_t coord2d);
void awk_map_set_level_async(int map_id, float level);
void awk_map_set_roll_angle_async(int map_id, float roll_angle);
void awk_map_set_view_port_async(int map_id, awk_map_view_port_t view_port);
void awk_map_get_posture_async(uint32_t map_id, f_awk_map_get_posture_cbk cbk);

#endif
