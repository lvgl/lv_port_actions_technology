/*
 * @brief 系统适配接口
 */

#ifndef _AWK_ADAPATER_H
#define _AWK_ADAPATER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "awk_defines.h"

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *    绘制相关适配层
*********************/
/**
 * @brief 文字排布的对齐样式
 */
typedef enum {
  AWK_ALIGN_STYLE_NONE,
  AWK_ALIGN_STYLE_CENTER,
  AWK_ALIGN_STYLE_LEFT,
  AWK_ALIGN_STYLE_RIGHT
} awk_align_style_t;

/**
 * @brief 字体粗细样式
 */
typedef enum {
   AWK_FONT_WEIGHT_NORMAL,
   AWK_FONT_WEIGHT_THIN,
   AWK_FONT_WEIGHT_BOLD,
} awk_font_weight_t;

/**
 * @brief 文字样式
 */
typedef struct _awk_text_style_t {
    awk_align_style_t align;
    uint32_t font_size;
    awk_font_weight_t font_weight;
} awk_text_style_t;

/**
 * @brief 填充类型
 */
typedef enum {
   AWK_FILL_STYLE_DRAWING_ONLY,          // 仅填充几何图形
   AWK_FILL_STYLE_DRAWING_AND_STROKE,    // 填充几何图形和描边
   AWK_FILL_STYLE_STROKE_ONLY            // 仅填充描边
} awk_fill_style_t;
  
/**
 * @brief 虚线定义，单位：像素
 */
typedef struct _awk_dash_style_t {
    int32_t painted_length;     //!< 绘制长度
    int32_t unpainted_length;   //!< 空白长度
    int32_t offset;             //!< 起始位置
} awk_dash_style_t;

/**
 * @brief 笔刷样式
 */
typedef struct _awk_paint_style_t {
    uint32_t width;                 //!< 画笔宽度, 单位：像素
    uint32_t color;                 //!< 画笔颜色, ARGB
    float angle;                    //!< 旋转角度，[0-360)，正东为0，逆时针为正
    awk_text_style_t text_style;    //!< 文字样式
    awk_fill_style_t fill_ttyle;    // 填充样式
    awk_dash_style_t dash_style;    //!< 虚线样式
} awk_paint_style_t;

/**
 * @brief 绘制上下文
 */
typedef struct _awk_render_context_t {
    int32_t width;         //!< 画布宽度, 单位：像素
    int32_t height;        //!< 画布高度, 单位：像素
} awk_render_context_t;

/**
 * @brief 绘制相关适配接口
 */
typedef struct _awk_render_adapter_t awk_render_adapter_t;
struct _awk_render_adapter_t {
    /**
     * @brief 开始绘制时回调方法
     * @param {map_id} 地图实例id
     * @param {awk_map_status_t} status
     * @return {*}
     */    
    void (*begin_drawing) (uint32_t map_id, awk_render_context_t status);

    /**
     * @brief 绘制结束时回调方方法
     * @param {map_id} 地图实例id
     * @return {*}
     */    
    void (*commit_drawing)(uint32_t map_id);
    
    /**
     * @brief 需要绘制点时回调
     * @param {map_id} 地图实例id
     * @param {awk_point_t} *point 点的数据
     * @param {uint32_t} point_size 点的个数
     * @param {awk_paint_style_t} *style 绘制样式
     * @return {*}
     */    
    void (*draw_point)(uint32_t map_id, awk_point_t *point, uint32_t point_size, const awk_paint_style_t *style);

    /**
     * @brief 需要绘制线时回调
     * @param {map_id} 地图实例id
     * @param {awk_point_t} *points 线上点的结合
     * @param {uint32_t} point_size 点的数量
     * @param {awk_paint_style_t} *style 绘制样式
     * @return {*}
     */    
    void (*draw_polyline)(uint32_t map_id, awk_point_t *points, uint32_t point_size, const awk_paint_style_t *style);

    /**
     * @brief 需要绘制面时回调
     * @param {map_id} 地图实例id
     * @param {awk_point_t} *points 面的点集合
     * @param {uint32_t} point_size 点的数量
     * @param {awk_paint_style_t} *style 绘制样式
     * @return {*}
     */    
    void (*draw_polygon)(uint32_t map_id, awk_point_t *points, uint32_t point_size, const awk_paint_style_t *style);

    /**
     * @brief 需要绘制bitmap时回调
     * @param {map_id} 地图实例id
     * @param {awk_rect_area_t} area 区域
     * @param {awk_bitmap_t} bitmap bitmap信息
     * @param {awk_paint_style_t} *style 样式
     * @return {*}
     */    
    void (*draw_bitmap)(uint32_t map_id, awk_rect_area_t area, awk_bitmap_t bitmap, const awk_paint_style_t *style);

    /**
     * @brief 需要绘制文字时回调
     * @param {map_id} 地图实例id
     * @param {awk_point_t} center 文字中心位置
     * @param {char} *text 文字内容
     * @param {awk_paint_style_t} *style 绘制样式
     * @return {*}
     */    
    void (*draw_text)(uint32_t map_id, awk_point_t center, const char *text, const awk_paint_style_t *style);

    /**
     * @brief 需要绘制颜色时回调
     * @param {map_id} 地图实例id
     * @param {awk_rect_area_t} area 区域
     * @param {awk_paint_style_t} *style 样式
     * @return {*}
     */    
    void (*draw_color)(uint32_t map_id, awk_rect_area_t area, const awk_paint_style_t *style);
    
    /**
     * @brief 测量文字宽度
     * @param {map_id} 地图实例id
     * @param text 字符串
     * @param style 文字绘制样式
     * @param width 文字的宽度
     * @param ascender 文字的上行高度
     * @param descender 文字的下行高度
     */
    bool (*measure_text)(uint32_t map_id, const char *text, const awk_paint_style_t *style, int32_t *width, int32_t *ascender, int32_t *descender);
};

/*********************
 *    文件系统相关适配层
*********************/

/**
 * @brief 读文件夹时的文件节点信息
 */
typedef struct _awk_file_node {
    char file_name[128];    //!< 文件名
    size_t file_size;       //!< 文件大小
} awk_file_node;

/**
 * @brief 读文件夹的结果
 */
typedef struct _awk_readdir_result {
    awk_file_node *nodes;   //!< 文件节点信息
    size_t size;            //!< 文件节点数量
} awk_readdir_result;

/**
 * @brief 文件相关适配接口
 */
typedef struct _awk_file_adapter_t awk_file_adapter_t;
struct _awk_file_adapter_t {
    /**
     * @brief 打开文件的回调,参考fopen
     * @return {*}
     */    
    void* (*file_open) (const char *filename, const char *mode);
    /**
     * @brief 关闭文件的回调，参考fclose
     * @param {void*} handler
     * @return {*}
     */    
    int (*file_close) (void* handler);
    /**
     * @brief 读文件的回调，参考fread
     * @param {void} *ptr buffer
     * @param {size_t} size 大小
     * @param {size_t} nmembs 块数量
     * @param {void*} handler 文件指针
     * @return {*}
     */    
    size_t (*file_read) (void *ptr, size_t size, void* handler);
    /**
     * @brief 写文件时回调，参考fwrite
     * @param {void} *ptr buffer
     * @param {size_t} size 大小
     * @param {size_t} nmembs 块数量
     * @param {void*} handler 文件指针
     * @return {*}
     */    
    size_t (*file_write) (void *ptr, size_t size, void* handler);
    /**
     * @brief 参考fseek
     * @param {void} *handler 文件指针
     * @param {long} offset 偏移
     * @param {int} where 位置
     * @return {*}
     */
    int (*file_seek) (void *handler, long offset, int where);
    /**
     * @brief 参考fflush
     * @param {void} *handler 文件指针
     * @return {*}
     */
    int (*file_flush) (void *handler);
    /**
     * @brief 文件是否存在
     * @param {char} *path 文件路径
     * @return {*}
     */    
    bool (*file_exists) (const char *path);
    /**
     * @brief 移除文件，参考remove
     * @param {char} *path 路径
     * @return {*}
     */    
    int (*file_remove) (const char *path);
    /**
     * @brief 文件夹是否存在
     * @param {char} *path 路径
     * @return {*}
     */    
    bool (*file_dir_exists) (const char *path);
    /**
     * @brief 创建文件夹时回调，参考mkdir
     * @param {char} *path 路径
     * @param {uint16_t} model 权限
     * @return {*}
     */    
    int (*file_mkdir) (const char *path, uint16_t model);
    /**
     * @brief 删除文件夹时回调，参考rmdir
     * @param {char} *path 路径
     * @return {*}
     */    
    int (*file_rmdir) (const char *path);
    /**
     * @brief 打开文件夹时的回调，参考opendir
     * @param {char} *path 路径
     * @return {*}
     */    
    void* (*file_opendir)(const char *path);
    /**
     * @brief 关闭文件夹时的回调
     * @param {void} *dir 文件夹指针
     * @return {*}
     */    
    int (*file_closedir)(void *dir);
    /**
     * @brief 读文件夹的内容
     * @param {void} *dir 文件夹指针
     * @param {awk_readdir_result} result 填充读取的结果
     * @return {成功或失败}
     */    
    bool (*file_readdir)(void *dir, awk_readdir_result *result);
    /**
     * @brief 获取文件大小时的回调
     * @param {char} *path 路径
     * @return {*}
     */
    size_t (*file_get_size) (const char *path);
    /**
     * @brief 获取文件最后访问的回调
     * @param {char} *path 路径
     * @return {*}
     */
    long (*file_get_last_access) (const char *path);
    /**
     * @brief 重命名文件，参考rename
     * @param {char} *old_name
     * @param {char} *new_name
     * @return {*}
     */    
    int  (*file_rename) (const char *old_name, const char *new_name);
    /**
     * @brief  解压适配接口
     * @param {char} *zip_file 压缩文件路径
     * @param {char} *out_dir 输出文件目录
     * @return {*} 成功或失败
     */    
    bool (*file_unzip)(const char *zip_file, const char *out_dir);
};


/*********************
 *    内存相关适配层
*********************/

/**
 * @brief 内存相关适配接口
 */
typedef struct _awk_memory_adapter_t awk_memory_adapter_t;
struct _awk_memory_adapter_t {
    /**
     * @brief 释放内存时回调，参考free
     * @param {void*} ptr
     * @return {*}
     */    
    void (*mem_free)(void* ptr);
    /**
     * @brief 分配内存时回调，参考malloc
     * @return {*}
     */    
    void* (*mem_malloc) (size_t size);
    /**
     * @brief 分配内存的回调，参考calloc
     * @return {*}
     */    
    void* (*mem_calloc) (size_t count, size_t size);
    /**
     * @brief 重新分配内存的回调，参考realloc
     * @return {*}
     */    
    void* (*mem_realloc) (void* ptr, size_t size);
};
    
/**
 * @brief 内存相关适配接口（针对大内存优化）
 */
typedef struct _awk_fast_memory_adapter_t awk_fast_memory_adapter_t;
struct _awk_fast_memory_adapter_t {
    /**
     * @brief 释放内存时回调，参考free
     * @param {void*} ptr
     * @return {*}
     */
    void (*mem_free)(void* ptr);
    /**
     * @brief 分配内存时回调，参考malloc
     * @param {int} type 内存类型，0表示tile缓存内存，1表示tile绘制内存
     * @return {*}
     */
    void* (*mem_malloc) (size_t size, int type);
};

/*********************
 *    网络相关适配层
*********************/

AWK_DATA_MAP_DEFINE(http_header_t, char*, char*);
#define awk_http_header_t awk_map_http_header_t

/*
 * @brief 网络请求类型
 */
typedef enum _awk_http_method {
    AWK_HTTP_METHOD_POST,
    AWK_HTTP_METHOD_GET,
} awk_http_method;
    
/*
 * @brief 网络请求优先级
 */
typedef enum _awk_http_priority {
    AWK_HTTP_PRIORITY_LOW,       //!< 低优先级
    AWK_HTTP_PRIORITY_MEDIUM,    //!< 普通优先级
    AWK_HTTP_PRIORITY_HIGH,      //!< 高优先级
} awk_http_priority;

/**
 * @brief aos缓冲区接口
 */
typedef struct _awk_http_buffer_t awk_http_buffer_t;
struct _awk_http_buffer_t {
    uint8_t *buffer;            //!< buffer内容
    size_t length;              //!< buffer长度
};

/**
 * @brief aos请求类
 */
typedef struct _awk_http_request_t {
    char *url;                                        //!< 访问的url地址
    awk_http_priority priority;                       //!< 请求优先级
    awk_http_method method;                           //!< 请求访问方式,默认值为get请求
    awk_http_buffer_t *body;                          //!< body内容
    awk_http_header_t headers;                        //!< header项
} awk_http_request_t;

/**
 * @brief aos返回的数据结构
 */
typedef struct _awk_http_response_t {
    uint64_t request_id;                              //!< reqId
    char* url;                                        //!< 请求的url地址
    uint32_t status_code;                             //!< 请求返回的状态码，如200、404
    awk_http_buffer_t *body;                           //!< body内容
    awk_http_header_t headers;                         //!< header项
} awk_http_response_t;

/**
 * @brief aos返回结果回调接口，网络代理方法的回调时需要在主流程线程中进行回调
 */
typedef struct _awk_http_response_callback_t awk_http_response_callback_t;
struct _awk_http_response_callback_t {
    /**
     * @brief 接收到header后回调，最多只需回调一次，需要包含响应的头信息，header为必填
     */
    void (*on_receive_header) (awk_http_response_callback_t *callback, const awk_http_response_t *response);
    /**
     * @brief 接收到body后回调，根据body大小，可回调多次，body为必填
     */
    void (*on_receive_body) (awk_http_response_callback_t *callback, const awk_http_response_t *response);
    /**
     * @brief 请求数据失败后回调
     */
    void (*on_fail) (awk_http_response_callback_t *callback, const awk_http_response_t *response, int32_t error);
    /**
     * @brief 所有数据返回成功后回调，最多只回调一次，可以不携带body，header数据
     */
    void (*on_success) (awk_http_response_callback_t *callback, const awk_http_response_t *response);
    /**
     * @brief 取消请求后，如果请求仍然在队列中，则回调该方法，最多只会回调一次
     */
    void (*on_canceled) (awk_http_response_callback_t *callback, const awk_http_response_t *response);
};

/**
 * @brief 网络相关适配接口
 */
typedef struct _awk_network_adapter_t awk_network_adapter_t;
struct _awk_network_adapter_t {
    /**
     * @brief 发送请求，请求的处理可以在异步线程中发起
     * @param request aos请求类
     * @param callback 返回结果回调接口
     * @return 请求id 请求id不能重复，可以自增处理
     */
    uint64_t (*send) (const awk_http_request_t *request, awk_http_response_callback_t *callback);
    /**
     * @brief 取消请求，需要在主流程线程中处理
     * @param request_id 需要取消请求的id
     */
    void (*cancel) (uint64_t request_id);
};


/*********************
 *    其他系统相关适配层
*********************/

/**
 * @brief 系统相关适配接口
 */
typedef struct _awk_system_adapter_t awk_system_adapter_t;
struct _awk_system_adapter_t {
    /**
     * @brief 获取系统时间回调
     * @return {*}
     */
    uint64_t (*get_system_time)(void);

    /**
     * @brief print回调，参考printf
     * @param {char*} __fmt
     * @return {*}
     */
    int (*log_printf)(const char* __fmt, ...);
};

/*********************
 *    线程相关适配层
*********************/

/**
 * @brief 线程相关适配接口
 */
typedef struct _awk_thread_adapter_t {
    /**
     * @brief 获取当前调用线程的id
     * @return 线程id
     */
    uint64_t (*get_thread_id)(void);
} awk_thread_adapter_t;

/*********************
 *    瓦片文件
*********************/

/**
 * @brief 瓦片文件代理
 * 
 */
typedef struct _awk_map_tile_file_adapter_t {
    /**
     * @brief 加载瓦片回调，返回瓦片数据
     * @param {char*} tile_file_key 瓦片文件key {char*} file_path 返回的文件路径,256字节 {size_t*} file_offset返回的文件偏移,{size_t*} file_size文件大小
     */
    bool (*on_tile_file)(const char* tile_file_key, char *file_path, size_t *file_offset, size_t *file_size);
} awk_map_tile_file_adapter_t;

#ifdef __cplusplus
}
#endif

#endif

