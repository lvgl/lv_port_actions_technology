/*
 * @brief watch地图接口
 */
#ifndef _AWK_MAP_H
#define _AWK_MAP_H

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include "awk_map_defines.h"
#include "awk_adapter.h"

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *    地图操作
*********************/

/**
 * @brief 地图渲染回调
 */
typedef struct _awk_map_render_callback_t awk_map_render_callback_t;
struct _awk_map_render_callback_t {

    /**
     * @brief 点覆盖物开始绘制回调
     * @param {uint32_t} map_id 地图实例id
     * @param {int32_t} guid 覆盖物id
     * @return {bool} 如果允许绘制，则返回true，否则返回false
     */
    bool (*on_point_begin_draw)(uint32_t map_id, int32_t guid);

    /**
     * @brief 点覆盖物结束绘制回调
     * @param {uint32_t} map_id 地图实例id
     * @param {int32_t} guid 覆盖物id
     * @param {int32_t} status 绘制成功返回0，否则返回错误码
     * @return {*}
     */
    void (*on_point_end_draw)(uint32_t map_id, int32_t guid, int32_t status);

    /**
     * @brief 线覆盖物开始绘制回调
     * @param {uint32_t} map_id 地图实例id
     * @param {int32_t} guid 覆盖物id
     * @return {bool} 如果允许绘制，则返回true，否则返回false
     */
    bool (*on_line_begin_draw)(uint32_t map_id, int32_t guid);

    /**
     * @brief 线覆盖物结束绘制回调
     * @param {uint32_t} map_id 地图实例id
     * @param {int32_t} guid 覆盖物id
     * @param {int32_t} status 绘制成功返回0，否则返回错误码
     * @return {*}
     */
    void (*on_line_end_draw)(uint32_t map_id, int32_t guid, int32_t status);

    /**
     * @brief 面覆盖物开始绘制回调
     * @param {uint32_t} map_id 地图实例id
     * @param {int32_t} guid 覆盖物id
     * @return {bool} 如果允许绘制，则返回true，否则返回false
     */
    bool (*on_polygon_begin_draw)(uint32_t map_id, int32_t guid);

    /**
     * @brief 面覆盖物结束绘制回调
     * @param {uint32_t} map_id 地图实例id
     * @param {int32_t} guid 覆盖物id
     * @param {int32_t} status 绘制成功返回0，否则返回错误码
     * @return {*}
     */
    void (*on_polygon_end_draw)(uint32_t map_id, int32_t guid, int32_t status);

    /**
     * @brief tile开始绘制回调
     * @param {uint32_t} map_id 地图实例id
     * @param {uint32_t} tile_x tile的x坐标(P20坐标)
     * @param {uint32_t} tile_y tile的y坐标(P20坐标)
     * @param {uint32_t} zoom 地图缩放级别
     * @return {bool} 如果允许绘制，则返回true，否则返回false
     */
    bool (*on_tile_begin_draw)(uint32_t map_id, uint32_t tile_x, uint32_t tile_y, uint32_t zoom);

    /**
     * @brief tile结束绘制回调
     * @param {uint32_t} map_id 地图实例id
     * @param {uint32_t} tile_x tile的x坐标(P20坐标)
     * @param {uint32_t} tile_y tile的y坐标(P20坐标)
     * @param {uint32_t} zoom 地图缩放级别
     * @param {int32_t} status 绘制成功返回0，否则返回错误码
     * @return {*}
     */
    void (*on_tile_end_draw)(uint32_t map_id, uint32_t tile_x, uint32_t tile_y, uint32_t zoom, int32_t status);

    /**
     * @brief tile开始下载回调（仅在线模式）
     * @param {int32_t} type 下载类型，0表示是绘制下载，1表示是用户调用awk_map_request_tiles()方法下载
     * @param {uint32_t} tile_x tile的x坐标(P20坐标)
     * @param {uint32_t} tile_y tile的y坐标(P20坐标)
     * @param {uint32_t} zoom 地图缩放级别
     * @return {bool} 如果允许下载，则返回true，否则返回false
     */
    bool (*on_tile_begin_download)(int32_t type, uint32_t tile_x, uint32_t tile_y, uint32_t zoom);

    /**
     * @brief poi开始下载回调（仅在线模式）
     * @param {int32_t} type 下载类型，0表示是绘制下载，1表示是用户调用awk_map_request_tiles()方法下载
     * @param {uint32_t} tile_x tile的x坐标(P20坐标)
     * @param {uint32_t} tile_y tile的y坐标(P20坐标)
     * @param {uint32_t} zoom 地图缩放级别
     * @return {bool} 如果允许下载，则返回true，否则返回false
     */
    bool (*on_poi_begin_download)(int32_t type, uint32_t tile_x, uint32_t tile_y, uint32_t zoom);

    /**
     * @brief tile开始绘制回调
     * @param {uint32_t} map_id 地图实例id
     * @param {uint32_t} tile_x tile的x坐标(P20坐标)
     * @param {uint32_t} tile_y tile的y坐标(P20坐标)
     * @param {uint32_t} zoom 地图缩放级别
     * @return {bool} 如果允许绘制，则返回true，否则返回false
     */
    bool (*on_poi_begin_draw)(uint32_t map_id, uint32_t tile_x, uint32_t tile_y, uint32_t zoom);

    /**
     * @brief tile结束绘制回调
     * @param {uint32_t} map_id 地图实例id
     * @param {uint32_t} tile_x tile的x坐标(P20坐标)
     * @param {uint32_t} tile_y tile的y坐标(P20坐标)
     * @param {uint32_t} zoom 地图缩放级别
     * @param {int32_t} status 绘制成功返回0，否则返回错误码
     * @return {*}
     */
    void (*on_poi_end_draw)(uint32_t map_id, uint32_t tile_x, uint32_t tile_y, uint32_t zoom, int32_t status);

    /**
     * @brief tile结束下载回调（仅在线模式）
     * @param {int32_t} type 下载类型，0表示是绘制下载，1表示是用户调用awk_map_request_tiles()方法下载
     * @param {uint32_t} tile_x tile的x坐标(P20坐标)
     * @param {uint32_t} tile_y tile的y坐标(P20坐标)
     * @param {uint32_t} zoom 地图缩放级别
     * @param {awk_map_tile_response_status_t} status 下载状态
     * @return {*}
     */
    void (*on_tile_end_download)(int32_t type, uint32_t tile_x, uint32_t tile_y, uint32_t zoom, awk_map_tile_response_status_t status);

    /**
     * @brief poi结束下载回调（仅在线模式）
     * @param {int32_t} type 下载类型，0表示是绘制下载，1表示是用户调用awk_map_request_tiles()方法下载
     * @param {uint32_t} tile_x tile的x坐标(P20坐标)
     * @param {uint32_t} tile_y tile的y坐标(P20坐标)
     * @param {uint32_t} zoom 地图缩放级别
     * @param {awk_map_tile_response_status_t} status 下载状态
     * @return {*}
     */
    void (*on_poi_end_download)(int32_t type, uint32_t tile_x, uint32_t tile_y, uint32_t zoom, awk_map_tile_response_status_t status);
};

/**
 * @brief 创建地图mapview，方法的调用需要在主流程线程中
 * @param {awk_map_view_param} param 地图mapview参数
 * @return {返回值如果大于0，表示生成的地图实例id，-1表示没有初始化，-2表示license校验失败 -3初始化的线程和当前调用不一致}
 */
int32_t awk_map_create_view(awk_map_view_param_t param);

/**
 * @brief 销毁地图mapview，方法的调用需要在主流程线程中
 * @param {uint32_t} map_id 地图实例id
 * @return {成功返回0, 失败返回error: -1表示没有初始化 -3初始化的线程和当前调用不一致}
 */
int32_t awk_map_destroy_view(uint32_t map_id);

/**
 * @brief 地图绘制，方法的调用需要在主流程线程中
 * @return {成功返回0, 失败返回error: -1表示没有初始化 -3初始化的线程和当前调用不一致}
 */
int32_t awk_map_do_render(void);

/**
 * @brief 将map_id对应的地图暂停绘制，方法的调用需要在主流程线程中
 * @param {uint32_t} map_id 地图实例id
 * @return {成功返回0, 失败返回error: -1表示没有初始化, -2表示没有找到map_id对应的地图实例 -3初始化的线程和当前调用不一致}
 */
int32_t awk_map_pause_render(uint32_t map_id);

/**
 * @brief 将map_id对应的地图恢复绘制，方法的调用需要在主流程线程中
 * @param {uint32_t} map_id 地图实例id
 * @return {成功返回0, 失败返回error: -1表示没有初始化, -2表示没有找到map_id对应的地图实例 -3初始化的线程和当前调用不一致}
 */
int32_t awk_map_resume_render(uint32_t map_id);

/**
 * @brief 添加纹理数据，方法的调用需要在主流程线程中
 * @param {awk_map_texture_data_t} texture_data 纹理参数
 * @return {成功返回值大于0表示纹理id，失败返回error: -1表示没有初始化 -3初始化的线程和当前调用不一致}
 */
int32_t awk_map_add_texture(const awk_map_texture_data_t *texture_data);

/**
 * @brief 更新纹理数据，方法的调用需要在主流程线程中
 * @param {uint32_t} texture_id 纹理id
 * @param {awk_map_texture_data_t} texture_data 纹理参数
 * @return {成功返回0, 失败返回error: -1表示没有初始化, -2表示没有找到texture_id对应的纹理 -3初始化的线程和当前调用不一致}
 */
int32_t awk_map_update_texture(uint32_t texture_id, const awk_map_texture_data_t *texture_data);

/**
 * @brief 移除纹理数据，方法的调用需要在主流程线程中
 * @param {uint32_t} texture_id 纹理id
 * @return {成功返回0, 失败返回error: -1表示没有初始化 -3初始化的线程和当前调用不一致}
 */
int32_t awk_map_remove_texture(uint32_t texture_id);

/**
 * @brief 初始化点覆盖物的结构体，方法的调用需要在主流程线程中
 * @param {awk_map_point_overlay_t} *point_overlay 点覆盖物结构体指针
 * @return {成功返回0, 失败返回error: -1表示没有初始化, -2表示传入参数为NULL -3初始化的线程和当前调用不一致}
 */
int32_t awk_map_init_point_overlay(awk_map_point_overlay_t *point_overlay);

/**
 * @brief 初始化线覆盖物的结构体，方法的调用需要在主流程线程中
 * @param {awk_map_polyline_overlay_t} *point_overlay 线覆盖物结构体指针
 * @return {成功返回0, 失败返回error: -1表示没有初始化, -2表示传入参数为NULL -3初始化的线程和当前调用不一致}
 */
int32_t awk_map_init_line_overlay(awk_map_polyline_overlay_t *line_overlay);

/**
 * @brief 初始化面覆盖物的结构体，方法的调用需要在主流程线程中
 * @param {awk_map_polygon_overlay_t} *point_overlay 面覆盖物结构体指针
 * @return {成功返回0, 失败返回error: -1表示没有初始化, -2表示传入参数为NULL -3初始化的线程和当前调用不一致}
 */
int32_t awk_map_init_polygon_overlay(awk_map_polygon_overlay_t *polygon_overlay);

/**
 * @brief 添加覆盖物，方法的调用需要在主流程线程中
 * @param {uint32_t} map_id 地图实例id
 * @param {awk_map_view_base_overlay_t} overlay 参数
 * @return {成功返回0, 失败返回error: -1表示没有初始化, -2表示已经存在guid对应的覆盖物，-3表示传入的overlay参数为NULL或者geometry_type类型错误
            -4初始化的线程和当前调用不一致}
 */
int32_t awk_map_add_overlay(uint32_t map_id, awk_map_base_overlay_t *overlay);

/**
 * @brief 更新覆盖物，方法的调用需要在主流程线程中
 * @param {uint32_t} map_id 地图实例id
 * @param {awk_map_view_base_overlay_t} overlay 参数
 * @return {成功返回0, 失败返回error: -1表示没有初始化, -2表示没有找到guid对应的覆盖物，-3表示传入的overlay参数为NULL或者geometry_type类型错误
            -4初始化的线程和当前调用不一致}
 */
int32_t awk_map_update_overlay(uint32_t map_id, awk_map_base_overlay_t *overlay);

/**
 * @brief 移除覆盖物，方法的调用需要在主流程线程中
 * @param {uint32_t} map_id 地图实例id
 * @param {uint32_t} overlay_id 覆盖物id
 * @return {成功返回0, 失败返回error: -1表示没有初始化, -2表示没有找到guid对应的覆盖物 -3初始化的线程和当前调用不一致}
 */
int32_t awk_map_remove_overlay(uint32_t map_id, uint32_t overlay_id);

/**
 * @brief 根据overlay_id查找覆盖物信息，方法的调用需要在主流程线程中
 * @param {uint32_t} map_id 地图实例id
 * @param {uint32_t} overlay_id 覆盖物id
 * @return {成功返回覆盖物结构体指针，失败返回NULL}
 */
const awk_map_base_overlay_t *awk_map_find_overlay(uint32_t map_id, uint32_t overlay_id);

/**
 * @brief 获取map_id对应的地图所有覆盖物的数量，方法的调用需要在主流程线程中
 * @param {uint32_t} map_id 地图实例id
 * @return {覆盖物的数量}
 */
uint32_t awk_map_get_overlay_count(uint32_t map_id);

/**
 * @brief 根据索引获取map_id对应地图的覆盖物，方法的调用需要在主流程线程中
 * @param {uint32_t} map_id 地图实例id
 * @param {uint32_t} index 覆盖物索引
 * @return {成功返回覆盖物结构体指针，失败返回NULL}
 */
const awk_map_base_overlay_t *awk_map_get_overlay(uint32_t map_id, uint32_t index);

/**
 * @brief 设置地图视口，方法的调用需要在主流程线程中
 * @param {uint32_t} map_id 地图实例id
 * @param {view_port} view_port 视口参数
 * @return {成功返回0, 失败返回error: -1表示没有初始化 -3初始化的线程和当前调用不一致}
 */
int32_t awk_map_set_view_port(uint32_t map_id, awk_map_view_port_t view_port);

/**
 * @brief 设置地图渲染回调接口实例
 * @param {awk_map_render_callback_t} callback 渲染回调接口实例
 * @return {成功返回0, 失败返回error: -1表示没有初始化 -3初始化的线程和当前调用不一致}
 */
int32_t awk_map_set_render_callback(awk_map_render_callback_t callback);

/**
 * @brief 设置地图中心，方法的调用需要在主流程线程中
 * @param {uint32_t} map_id 地图实例id
 * @param {coord2D} coord2d 地图中心点经纬度坐标
 * @return {成功返回0, 失败返回error: -1表示没有初始化 -3初始化的线程和当前调用不一致}
 */
int32_t awk_map_set_center(uint32_t map_id, awk_map_coord2d_t coord2d);

/**
 * @brief 设置地图级别，方法的调用需要在主流程线程中
 * @param {uint32_t} map_id 地图实例id
 * @param {float} level 地图级别
 * @return {成功返回0, 失败返回error: -1表示没有初始化 -3初始化的线程和当前调用不一致 -4表示level级别不合法}
 */
int32_t awk_map_set_level(uint32_t map_id, float level);

/**
 * @brief 设置地图的旋转角度，方法的调用需要在主流程线程中
 * @param {uint32_t} map_id 地图实例id
 * @param {float} roll_angle 旋转角度
 * @return {成功返回0, 失败返回error: -1表示没有初始化 -3初始化的线程和当前调用不一致 -4表示当前的tile_style不支持该能力}
 */
int32_t awk_map_set_roll_angle(uint32_t map_id, float roll_angle);

/**
 * @brief 获取地图的状态，方法的调用需要在主流程线程中
 * @param {uint32_t} map_id 地图实例id
 * @param {awk_map_posture_t} *posture 返回地图状态
 * @return {成功返回0, 失败返回error: -1表示没有初始化 -3初始化的线程和当前调用不一致}
 */
int32_t awk_map_get_posture(uint32_t map_id, awk_map_posture_t *posture);

/**
 * @brief 在线请求并进行缓存瓦片，SDK内部会判断是否已经存在磁盘缓存和离线数据，如果存在缓存则不会走在线请求
 * @param {awk_rect_area_t} req_rect 需要缓存的屏幕区域
 * @param {awk_map_coord2d_t} *map_center 需要缓存的地图中心点
 * @param {float} map_level 需要缓存的地图级别
 * @param {int32_t} 请求的优先级，值请参考awk_http_priority
 * @return {返回大于0的值为请求id, 失败返回error: -1表示没有初始化 -3初始化的线程和当前调用不一致 -4表示参数非法}
 */
int32_t awk_map_request_tiles(awk_rect_area_t req_rect, awk_map_coord2d_t map_center, float map_level, int32_t priority);

/**
 * @brief 取消在线请求缓存瓦片
 * @param {int32_t} req_id 请求id
 * @return {成功返回0,  失败返回error: -1表示没有初始化 -3初始化的线程和当前调用不一致}
 */
int32_t awk_map_cancel_request_tiles(int32_t req_id);

/**
 * @brief 经纬度坐标转屏幕坐标，方法的调用需要在主流程线程中
 * @param {uint32_t} map_id 地图实例id
 * @param {awk_map_coord2d_t} lonlat 经纬度坐标
 * @param {int32_t} *x 返回屏幕坐标-x坐标值
 * @param {int32_t} *y 返回屏幕坐标-y坐标值
 * @return {成功返回0, 失败返回error: -1表示没有初始化 -3初始化的线程和当前调用不一致}
 */
int32_t awk_map_lonlat_to_xy(uint32_t map_id, awk_map_coord2d_t lonlat, int32_t *x, int32_t *y);

/**
 * @brief 屏幕坐标转经纬度坐标，方法的调用需要在主流程线程中
 * @param {uint32_t} map_id 地图实例id
 * @param {int32_t} x 屏幕坐标-x坐标值
 * @param {int32_t} y 屏幕坐标-y坐标值
 * @param {awk_map_coord2d_t} *lonlat 经纬度坐标
 * @return {成功返回0, 失败返回error: -1表示没有初始化 -3初始化的线程和当前调用不一致}
 */
int32_t awk_map_xy_to_lonlat(uint32_t map_id, int32_t x, int32_t y, awk_map_coord2d_t *lonlat);


/*********************
 *    地图手势
*********************/

/**
 * @brief 开始挪图手势，方法的调用需要在主流程线程中
 * @param {uint32_t} map_id 地图实例id
 * @param {int32_t} x 挪图时触摸的点x坐标
 * @param {int32_t} y 挪图时触摸的点y坐标
 * @return {成功返回0, 失败返回error: -1表示没有初始化, -2表示没有找到map_id对应的地图实例 -3初始化的线程和当前调用不一致}
 */
int32_t awk_map_touch_begin(uint32_t map_id, int32_t x, int32_t y);

/**
 * @brief 持续挪图过程，方法的调用需要在主流程线程中
 * @param {uint32_t} map_id 地图实例id
 * @param {int32_t} x 挪图时触摸的点x坐标
 * @param {int32_t} y 挪图时触摸的点y坐标
 * @return {成功返回0, 失败返回error: -1表示没有初始化, -2表示没有找到map_id对应的地图实例 -3初始化的线程和当前调用不一致}
 */
int32_t awk_map_touch_update(uint32_t map_id, int32_t x, int32_t y);

/**
 * @brief 停止挪图手势，方法的调用需要在主流程线程中
 * @param {uint32_t} map_id 地图实例id
 * @param {int32_t} x 挪图时触摸的点x坐标
 * @param {int32_t} y 挪图时触摸的点y坐标
 * @return {成功返回0, 失败返回error: -1表示没有初始化, -2表示没有找到map_id对应的地图实例 -3初始化的线程和当前调用不一致}
 */
int32_t awk_map_touch_end(uint32_t map_id, int32_t x, int32_t y);

/**
 * @brief 获取单击屏幕时命中的点覆盖物，如果命中多个点覆盖物，返回的结果中会按照priority从大到小进行排序
 * @param {uint32_t} map_id 地图实例id
 * @param {int32_t} x 屏幕坐标-x坐标值
 * @param {int32_t} y 屏幕坐标-y坐标值
 * @param {int32_t} *overlay_count 返回命中的点覆盖物数量
 * @return {成功返回命中的点覆盖物数组指针，失败返回NULL。返回的覆盖物数组指针需要调用awk_map_release_overlays()方法进行释放}
 */
awk_map_point_overlay_t **awk_map_click_points(uint32_t map_id, int32_t x, int32_t y, int32_t *overlay_count);

/**
 * @brief 释放覆盖物数组指针
 * @param {awk_map_base_overlay_t} **overlays 覆盖物数组指针
 * @param {int32_t} overlay_count 覆盖物数量
 * @return {成功返回0, 失败返回error: -1表示没有初始化, -2表示overlays为NULL或者overlay_count为0 -3初始化的线程和当前调用不一致}
 */
int32_t awk_map_release_overlays(awk_map_base_overlay_t **overlays, int32_t overlay_count);

/*********************
 *    离线地图下载
*********************/

/**
 * @brief 地图下载回调
 */
typedef struct _awk_map_download_callback_t awk_map_download_callback_t;
struct _awk_map_download_callback_t {
    /**
     * @brief 开始下载
     * @param {awk_download_callback_t} *callback
     * @return {*}
     */
    void (*on_started)(awk_map_download_callback_t *callback);
    /**
     * @brief 下载的进度回调
     * @param {awk_download_callback_t} *callback
     * @param {float} progress 下载进度
     * @return {*}
     */
    void (*on_progress)(awk_map_download_callback_t *callback, float progress);
    /**
     * @brief 下载完成
     * @param {awk_download_callback_t} *callback
     * @return {*}
     */
    void (*on_finish) (awk_map_download_callback_t *callback);
    /**
     * @brief 下载停止
     * @param {awk_download_callback_t} *callback
     * @return {*}
     */
    void (*on_stop) (awk_map_download_callback_t *callback);
    /**
     * @brief 下载错误
     * @param {awk_download_callback_t} *callback
     * @param {int32_t} error_code 错误码
     * @param {char*} msg 错误信息
     * @return {*}
     */
    void (*on_error) (awk_map_download_callback_t *callback, int32_t error_code, const char* msg);
};

/**
 * @brief 下载离线地图，方法的调用需要在主流程线程中, 目前下载不支持并发任务，多个下载需顺序执行
 * @param {char} *adcode 城市的adcode,城市/区 adcode（只有北京、上海、广州、深圳、杭州、天津、重庆、成都、哈尔滨、拉萨、南宁、昆明、长春、沈阳、大庆、温州、青岛 有区级）必填 无缺省值
 * @param {uint32_t} level 地图级别 地图层级（目前支持7-16级）必填 无缺省值
 * @param {awk_download_callback_t} *download_callback 下载回调
 * @return {成功返回0, 失败返回error: -1表示没有初始化 -3初始化的线程和当前调用不一致}
 */
int32_t awk_map_start_download_offline_data(const char *adcode, uint32_t level, awk_map_download_callback_t *download_callback);

/**
 * @brief 停止离线地图下载，方法的调用需要在主流程线程中
 * @return {成功返回0, 失败返回error: -1表示没有初始化 -3初始化的线程和当前调用不一致}
 */
int32_t awk_map_stop_download_offline_data();

/**
 * @brief 通过经纬度和范围下载地图数据
 * @param center 中心点经纬度
 * @param range 范围公里
 * @param expect_level 需要下载的级别
 * @param download_callback 下载回调
 * @return {成功返回0,失败返回error}
 */
int32_t awk_map_start_download_region_range(awk_map_coord2d_t center, awk_map_range_t range, uint8_t expect_level, awk_map_download_callback_t *download_callback);

/**
 * @brief 停止经纬度范围下载
 * @return {成功返回0,失败返回error}
 */
int32_t awk_map_stop_download_region_range(void);

typedef enum {
    AWK_MAP_TILE_TYPE_TILE = 0, // 瓦片数据
    AWK_MAP_TILE_TYPE_POI = 1,  // POI数据
} awk_map_tile_type_t;
/**
 * @brief 瓦片下载信息
 *
 */
typedef struct _awk_map_tile_download_info_t {
    awk_map_tile_type_t tile_type;
    char tile_file_key[64];
    awk_http_request_t tile_request;
} awk_map_tile_download_info_t;

/**
 * @brief 瓦片下载回调
 *
 */
typedef struct _awk_map_tile_download_callback_t {
    /**
     * @brief 成功的回调
     *
     */
    void (*on_success) (const char *key, awk_map_tile_download_info_t *tile_download_infos, uint32_t tile_download_size);

    /**
     * @brief 失败的回调
     *
     */
    void (*on_fail) (const char *key, int32_t error_code, const char* msg);
} awk_map_tile_download_callback_t;
/**
 * @brief 按照传入的经纬度点下载地图数据
 *
 * @param key 用户传入的key，通过回调带出
 * @param points 经纬度点集合
 * @param point_size 经纬度点数量
 * @param download_callback 回调
 * @return int32_t  {成功返回0,失败返回error}
 */
int32_t awk_map_download_polyline_region(const char *key, awk_map_coord2d_t *points,uint32_t point_size,awk_map_tile_download_callback_t *download_callback);

/**
 * @brief 瓦片下载完后通知sdk已下载的瓦片文件信息
 *
 * @param tile_file_key 瓦片文件key
 * @param downloadTileResponesJson rest服务返回的json信息
 * @return int32_t
 */
int32_t awk_map_notify_tile_file_downloaded(const char *tile_file_key,  const char* downloadTileResponesJson);

/**
 * @brief 瓦片文件删除是通知sdk已删除的瓦片文件
 *
 * @param tile_file_key 瓦片文件key
 * @return int32_t
 */
int32_t awk_map_notify_tile_file_deleted(const char *tile_file_key);

/**
 * @brief 程序异常退出时，或差异时的同步
 *
 * @param tile_file_keys
 * @param tile_file_size
 * @return int32_t
 */
int32_t awk_map_sync_tile_file(char **tile_file_keys, uint32_t tile_file_size);

/**
 * @brief 离线数据下载类型
 *
 */
typedef enum _awk_map_offline_type {
    OFFLINE_GDB_UNKNOWN,
    OFFLINE_GDB_ADCODE,
    OFFLINE_GDB_LOCATION_RANGE,
    OFFLINE_GDB_TILE_DONWLOAD
} awk_map_offline_type;

/**
 * @brief 离线数据下载类型中adcode下载相关
 *
 */
typedef struct _awk_map_offline_gdb_adcode_t {
    char adcode[8];     //!< adcode
} awk_map_offline_gdb_adcode_t;

/**
 * @brief 离线数据下载类型经纬度下载相关
 *
 */
typedef struct _awk_map_offline_gdb_location_t {
    awk_map_coord2d_t location;             //!< 经纬度
    awk_map_range_t range;                  //!< 范围枚举
    uint8_t expect_level;                   //!< 需要的级别
} awk_map_offline_gdb_location_t;

/**
 * @brief 下载信息
 *
 */
typedef struct _awk_map_offline_gdb_info_t {
    uint8_t level;                                  //!< 级别
    awk_map_offline_type type;                      //!< 下载类型
    char file_name[128];                            //!< 文件名称
    awk_map_offline_gdb_adcode_t adcode_info;       //!< adcode下载相关信息
    awk_map_offline_gdb_location_t location_info;   //!< 经纬度下载相关信息
} awk_map_offline_gdb_info_t;

/**
 * @brief 查询下载的结果
 *
 */
typedef struct _awk_map_offline_gdb_query_result_t {
    size_t size;                            //!< 数量
    awk_map_offline_gdb_info_t *infos;      //!< info信息数组
} awk_map_offline_gdb_query_result_t;

/**
 * @brief 查询已下载的区域信息
 *
 * @param expect_level 需要的级别
 * @param location 经纬度信息
 * @param range 范围
 * @param result 成功返回已下载的信息
 * @return int32_t {成功返回0,失败返回error}
 */
int32_t awk_map_list_download_region_range(awk_map_coord2d_t location, awk_map_range_t range, uint8_t expect_level, awk_map_offline_gdb_query_result_t *result);

/**
 * @brief 删除下载的区域信息
 *
 * @param expect_level 需要的级别
 * @param location 经纬度信息
 * @param range 范围
 * @return int32_t
 */
int32_t awk_map_delete_download_region_range(awk_map_coord2d_t location, awk_map_range_t range, uint8_t expect_level);

/**
 * @brief 列出已下载的离线地图信息，方法的调用需要在主流程线程中
          读取结果中的regions在不需要时需要手动释放，否则会有内存泄露
 *
 * @param adcode 按adcode过滤，NULl代表不过滤
 * @param level  按地图级别过滤，0代表不过滤
 * @param result 成功时填充结果信息，读取结果中的regions在不需要时需要手动释放，否则会有内存泄露
 * @return {成功返回0, 失败返回error: -1表示没有初始化, -2表示传入参数校验失败 -3初始化的线程和当前调用不一致}
 */
int32_t awk_map_list_download_offline_regions(const char *adcode, uint32_t level, awk_map_offline_gdb_query_result_t *result);

/**
 * @brief 删除下载的离线地图数据，方法的调用需要在主流程线程中
 *
 * @param adcode    adcode
 * @param level     地图级别
 * @return {成功返回0, 失败返回error: -1表示没有初始化, -2表示传入参数校验失败 -3初始化的线程和当前调用不一致}
 */
int32_t awk_map_delete_download_offline_region(const char *adcode, uint32_t level);

/**
 * @brief 同步下载区域信息，只在外部指定离线数据时使用, 方法的调用需要在主流程线程中
 * @see awk_context_t 当offline_map_dir设置后使用
 * @return {成功返回0, 失败返回error: -1表示没有初始化, -2表示其他异常 -3初始化的线程和当前调用不一致}
 */
int32_t awk_map_sync_download_offline_region(void);
#ifdef __cplusplus
}
#endif

#endif
