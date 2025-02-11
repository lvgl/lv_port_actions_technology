/*
 * @brief map数据结构定义
 */

#ifndef _AWK_MAP_DEFINES_H
#define _AWK_MAP_DEFINES_H

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include "awk_defines.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 视口尺寸结构
 */
typedef struct _awk_map_view_port_t {
    int32_t width;      //!< 视口宽度，单位：像素
    int32_t height;     //!< 视口高度，单位：像素
} awk_map_view_port_t;

/**
 * @brief 2d经纬度坐标结构
 */
typedef struct _awk_map_coord2d_t {
    double lon;        //!< 经度
    double lat;        //!< 纬度
} awk_map_coord2d_t;

/**
 * @brief 画布点坐标int64_t
 */
typedef struct _awk_point_64_t {
    int64_t x;
    int64_t y;
} awk_point_64_t;

/**
 * @brief 地图姿态数据结构
 */
typedef struct _awk_map_posture_t {
    awk_map_coord2d_t map_center;     //!< 地图中心点
    awk_map_view_port_t view_port;    //!< 视口尺寸
    awk_map_view_port_t drawing_port; //!< 绘制的视口尺寸
    float scale;                      //!< 缩放比例
    float level;                      //!< 地图比例尺, 范围：3~20
    float roll_angle;                 //!< 地图旋转角, [0-360)，正北为0，顺时针为正
} awk_map_posture_t;

/**
 * @brief 地图Overlay几何图元类型
 */
 typedef enum _awk_map_overlay_type {
    AWK_MAP_OVERLAY_TYPE_POINT,       //!< 点类型
    AWK_MAP_OVERLAY_TYPE_LINE,        //!< 线类型
    AWK_MAP_OVERLAY_TYPE_POLYGON,     //!< 面类型
    AWK_MAP_OVERLAY_TYPE_UNKNOWN      //!< 未设定
} awk_map_overlay_type;

typedef enum {
    AWK_MAP_TILE_STYLE_STANDARD_GRID = 0,   //!< 使用1x的栅格地图
    AWK_MAP_TILE_SATELLITE = 1,             //!< 使用1x的卫星地图
    AWK_MAP_TILE_STYLE_GRID_AND_POI = 2     //!< 使用1x的无poi文案的底图，配上动态绘制的poi文字
} awk_map_tile_style_t;

/**
 * @brief 地图mapview参数 
 */
typedef struct _awk_map_view_param_t {
    awk_map_view_port_t port;           //!< 视口宽高
} awk_map_view_param_t;

/**
 * @brief 纹理数据结构
 */
typedef struct _awk_bitmap_t awk_map_texture_data_t;

/**
 * @brief 点纹理信息
 */
typedef struct _awk_map_point_texture_info_t {
    int32_t texture_id;                        //!< 纹理id
    float anchor_x;                             //!< 锚点x位置，取值[0.0,1.0]
    float anchor_y;                             //!< 锚点y位置，取值[0.0,1.0]
    float scale;                                //!< 缩放比例
} awk_map_point_texture_info_t;

/**
 * @brief 点纹理信息
 */
typedef struct _awk_map_point_overlay_marker_t {
    awk_map_point_texture_info_t icon_texture;     //!< icon纹理
    awk_map_point_texture_info_t name_texture;     //!< name纹理
    awk_map_point_texture_info_t bubble_texture;   //!< 气泡纹理
} awk_map_point_overlay_marker_t;

/**
 * @brief 地图覆盖物基类
 */
typedef struct _awk_map_base_overlay_t {
    awk_map_overlay_type geometry_type;
    int32_t group_id;                //!< 组id，同一个组且类型相同的覆盖物的焦点态是互斥的
    int32_t map_id;                  //!< overlay所属的地图实例id
    int32_t priority;                //!< 绘制优先级
    bool visible;                    //!< 是否可见
    bool is_focus;                   //!< 是否存于焦点态
    float min_level;                 //!< 可显示的最小地图级别
    float max_level;                 //!< 可显示的最大地图级别

    int32_t guid;
} awk_map_base_overlay_t;

/**
 * @brief 地图点覆盖物
 */
typedef struct _awk_map_point_overlay_t {
    awk_map_base_overlay_t base_overlay;
    awk_map_coord2d_t position;                        //!< 点的经纬度坐标
    float angle;                                       //!< 旋转角度，取值[0-360) ，正东为0，逆时针为正
    awk_map_point_overlay_marker_t normal_marker;      //!< 普通状态下的纹理信息，当base_overlay.is_focus=false时，必须设置，否则会无法显示
    awk_map_point_overlay_marker_t focus_marker;       //!< 焦点状态下的纹理信息，当base_overlay.is_focus=true时，必须设置，否则会无法显示
    bool icon_visible;                                 //!< icon图标是否可见
    bool name_visible;                                 //!< name图标是否可见
    bool bubble_visible;                               //!< 气泡图标是否可见

} awk_map_point_overlay_t;

/**
 * @brief   polyline 纹理资源
 * @remark  polyline overlay item显示2条压盖在一起的线：外部描边线、内部填充线
 *          绘制的时候先绘制外部描边线、后绘制内部填充线
 */
typedef struct _awk_map_line_overlay_marker_t {
    int32_t line_width;            //!< 线的宽度，单位：像素
    int32_t line_color;            //!< 线的颜色，ARGB

    int32_t border_width;          //!< 边框的宽度，单位：像素
    int32_t border_color;          //!< 边框的颜色，ARGB

    int32_t dash_painted_length;    //!< 绘制长度
    int32_t dash_unpainted_length;  //!< 空白长度
    int32_t dash_offset;            //!< 起始位置

    bool use_dash;                 //!< 是否使用虚线
} awk_map_line_overlay_marker_t;

/**
 * 关键点标识数据标识 标志位1Byte
 */
typedef enum {
    AWK_MAP_POINT_FLAG_DEFAULT          = 0x00,      //!< 默认值
    AWK_MAP_POINT_FLAG_NORMAL_POINT     = 0x01,      //!< 普通点
    AWK_MAP_POINT_FLAG_KEY_POINT        = 0x02,      //!< 关键点
} awk_map_point_flag_t;

/**
 * @brief 地图线覆盖物
 */
typedef struct _awk_map_polyline_overlay_t {
    awk_map_base_overlay_t base_overlay;
    awk_map_coord2d_t *points;                    //!< 点序列经纬度坐标
    size_t point_size;                            //!< 点的数量
    uint8_t *point_flags;                         //!< 关键点标识数据(二维坐标点标识数据), 值见枚举awk_map_point_flag_t
    size_t point_flags_size;                      //!< 关键点标识的数量
    int32_t tolerance;                            //!< 点偏离直线的容忍度（单位：像素），默认是10像素
    size_t max_keep_point_size;                   //!< 点经过抽稀后保留的最大数量
    awk_map_line_overlay_marker_t normal_marker;  //!< 普通状态下的纹理信息，当base_overlay.is_focus=false时，必须设置，否则会无法显示
    awk_map_line_overlay_marker_t focus_marker;   //!< 焦点状态下的纹理信息，当base_overlay.is_focus=true时，必须设置，否则会无法显示
} awk_map_polyline_overlay_t;

/**
 * @brief 地图线面覆盖物
 */
typedef struct _awk_map_polygon_overlay_t {
    awk_map_base_overlay_t base_overlay;
    awk_map_coord2d_t *points;                    //!< 面的点序列经纬度坐标
    size_t point_size;                            //!< 点的数量
    int32_t color;                                //!< 填充颜色, ARGB
} awk_map_polygon_overlay_t;

/**
 * @brief 地图加载瓦片类型
 */
typedef enum {
    AWK_MAP_TILE_LOAD_OFFLINE = 0x01,                              //!< 瓦片加载离线模式
    AWK_MAP_TILE_LOAD_ONLINE = 0x02                                //!< 瓦片加载在线模式
} awk_map_tile_load_mode_t;
    
/**
 * @brief 地图范围
 */
typedef enum {
    AWK_MAP_RANGE_2KM = 1,
    AWK_MAP_RANGE_5KM,
    AWK_MAP_RANGE_10KM,
    AWK_MAP_RANGE_70KM,
    AWK_MAP_RANGE_600KM,
    AWK_MAP_RANGE_5000KM,
    AWK_MAP_RANGE_GLOBAL,
} awk_map_range_t;

/**
 * @brief 在线瓦片请求状态
 */
typedef enum {
    AWK_TILE_RESPONSE_STARUS_SUCCESS,           //!< 请求成功
    AWK_TILE_RESPONSE_STARUS_REQ_FAIL,          //!< 请求失败
    AWK_TILE_RESPONSE_STARUS_DECODE_FAIL,       //!< 解码失败
    AWK_TILE_RESPONSE_STARUS_REQ_CANCELLED,     //!< 请求取消
    AWK_TILE_RESPONSE_STARUS_HAS_REQUESTING,    //!< 正在请求
    AWK_TILE_RESPONSE_STARUS_IMAGE_INVALID,     //!< 图片非法
    AWK_TILE_RESPONSE_STARUS_UNKNOWN            //!< 未知错误
} awk_map_tile_response_status_t;

/**
 * @brief 矩形区域经纬度描述
 * 
 */
typedef struct _awk_rect_location_coord_t {
    awk_map_coord2d_t left_top;
    awk_map_coord2d_t right_bottom;
} awk_rect_location_coord_t;

#ifdef __cplusplus
}
#endif

#endif
