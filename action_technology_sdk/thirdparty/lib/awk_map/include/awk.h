#ifndef _AWK_H
#define _AWK_H

#include "awk_adapter.h"
#include "map/awk_map.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 上下文
 */
typedef struct _awk_context_t {
    char *device_id;                           //!< 设备id
    char *key;                                 //!< 开放平台key,需要使用高德开放平台申请的智能硬件key
    char *root_dir;                            //!< SDK内部文件夹根路径
    char *offline_map_dir;                     //!< 下载好的离线地图路径
    
    bool tile_zip;                             //!< 瓦片是否压缩
    bool tile_clip_load;                       //!< 瓦片在加载时是否进行裁剪，只有当tile_mem_cache_max_size为0时才有效
    bool tile_background_custom_draw;          //!< 瓦片背景（背景色&网格线）是否自行绘制
    bool tile_buff_mem_outer_free;             //!< 瓦片绘制内存是否由外部释放，true表示由外部释放，false表示由SDK内部释放，只有tile_clip_load为false时生效
    bool tile_cache_decoded_bitmap;            //!< 缓存瓦片时是否缓存解码后的位图
    int32_t tile_load_mode;                    //!< 地图瓦片加载类型，离线，在线，在线离线混合，具体枚举值见：awk_map_tile_load_mode_t
    awk_pixel_mode_t tile_pixel_mode;          //!< 地图瓦片像素格式
    uint32_t tile_disk_cache_max_size;         //!< tile磁盘缓存最大空间，单位: MB
    uint32_t tile_mem_cache_max_size;          //!< tile内存缓存最大空间，单位：KB
    uint32_t poi_tile_disk_cache_max_size;     //!< poi磁盘缓存最大空间，单位: MB
    uint32_t poi_tile_mem_cache_max_size;      //!< poi瓦片内存缓存最大空间，单位：KB
    awk_map_tile_style_t tile_style;           //!< 瓦片样式类型
    uint32_t max_file_count_in_dir;            //!< 一个文件目录最大文件数，0表示不限定文件数
    uint32_t max_one_file_size;                //!< 一个文件最大大小，0表示不限定文件大小，单位：MB
    bool need_force_render;                    //!< 是否需要强制刷新
    awk_render_adapter_t render_adapter;       //!< 绘制相关代理
    awk_file_adapter_t file_adapter;           //!< 文件io相关代理
    awk_memory_adapter_t memory_adapter;       //!< 内存相关代理
    awk_fast_memory_adapter_t fast_memory_adapter;  //!< 内存相关代理(针对大内存进行优化)
    awk_network_adapter_t network_adapter;     //!< 网络相关代理
    awk_thread_adapter_t thread_adapter;       //!< 线程相关代理
    awk_system_adapter_t system_adapter;       //!< 其他系统相关代理
    awk_map_tile_file_adapter_t tile_file_adapter; //!< 瓦片文件相关代理
} awk_context_t;


/**
 * @brief 地图环境初始化，方法的调用需要在主流程线程中
 * @param {awk_context} *context 环境参数
 * @return {成功返回0, 失败返回错误码: -1表示已经初始化，不能重复初始化, -100表示参数context为空, -101表示context.key为空，-102表示context.device_id为空，-103表示context.root_dir为空，-200开头错误码表示参数context.render_adapter的函数指针为空，-300开头错误码表示参数context.file_adapter的函数指针为空，-400开头错误码表示参数context.memory_adapter的函数指针为空，-500开头错误码表示参数context.system_adapter的函数指针为空, -600开头错误码表示参数context.network_adapter的函数指针为空 -700开头错误码表示参数context.thread_adapter的函数指针为空，-800开头错误码表示参数context.tile_file_adapter的函数指针为空}
 */
int32_t awk_init(awk_context_t *context);

/**
 * @brief 地图环境反初始化，方法的调用需要在主流程线程中
 * @return {成功返回0, 失败返回错误码：-1表示没有初始化 -3初始化的线程和当前调用不一致}
 */
int32_t awk_uninit(void);

/**
 * @brief 设备激活回调, 方法的调用需要在主流程线程中
 * @param {bool} result 成功或失败
 * @param {int} code 错误码
 * @param {char*} msg 错误信息
 * @return {*} 
 */
typedef void (*awk_device_active_callback) (bool result, int code, const char* msg);

/**
 * @brief 激活设备，需要在设备初次使用时调用，需要保证网络可用，未激活会导致SDK不可用，方法的调用需要在主流程线程中
 * @param {awk_device_active_callback} callback 回调结果
 */
void awk_activate_device(awk_device_active_callback callback);

/**
 * @brief 清除磁盘缓存, 方法的调用需要在主流程线程中
 * @return {成功返回0, 失败返回错误码：-1表示没有初始化，-3初始化的线程和当前调用不一致}
 */
int32_t awk_clear_disk_cache(void);

#ifdef __cplusplus
}
#endif

#endif
