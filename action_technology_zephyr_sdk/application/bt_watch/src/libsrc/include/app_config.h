/*!
 * \file      app_config.h
 * \brief     应用配置接口
 * \details   
 * \author    
 * \date      
 * \copyright Actions
 */

#ifndef _LIB_APP_CONFIG_H
#define _LIB_APP_CONFIG_H


#include <config.h>
#include "config_al.h"
#include <zephyr/types.h>
#include <os_common_api.h>


/*!
 * \brief 结构体成员大小.
 */
#ifndef SIZE
#define SIZE(_struct, _member)  sizeof(((_struct*)0)->_member)
#endif


#ifndef offsetof
#define offsetof(type, member) ((size_t) &((type *)0)->member)
#endif


/*!
 * \brief 加载应用配置数据文件
 * \n  注: 使用时必须先加载 defcfg.bin, 再加载 usrcfg.bin
 * \n
 * \n  cache_cfg_info : 缓存配置项信息可以加快查找速度
 * \n  cache_cfg_data : 缓存配置项数据可以加快读取速度
 * \return
 *     成功: true
 * \n  失败: false
 */
extern bool app_config_load
(
    const char* file_name, bool cache_cfg_info, bool cache_cfg_data
);


/*!
 * \brief 读取应用配置数据
 * \n  配置数据在读取前会被初始化为 0
 * \n
 * \param cfg_id   : 应用配置 ID
 * \param cfg_data : 保存配置数据
 * \param cfg_offs : 读取配置位置偏移
 * \param cfg_len  : 读取配置数据长度
 * \return
 *     配置数据为默认配置时返回 1
 * \n  配置数据非默认配置时返回 2
 * \n  失败返回 0
 */
extern int app_config_read
(
    unsigned int cfg_id, void* cfg_data, unsigned int cfg_offs, unsigned int cfg_len
);


/*!
 * \brief 获取应用配置项数值
 * \n
 * \param cfg_id    : 应用配置 ID
 * \param item_offs : 配置项位置偏移
 * \param item_size : 配置项大小
 * \param def_value : 当配置项读取失败时返回该默认值
 * \return
 *     成功: 配置项数值
 * \n  失败: 默认值
 */
static inline unsigned int app_config_get_value
(
    unsigned int cfg_id, unsigned int item_offs, unsigned int item_size, unsigned int def_value)
{
    unsigned int  value = 0;

    if (!app_config_read(cfg_id, &value, item_offs, item_size))
    {
        value = def_value;
    }

    return value;
}


/*!
 * \brief 获取应用配置项数值
 * \n
 * \param _cfg_id     : 应用配置 ID
 * \param _cfg_struct : 配置结构体类型
 * \param _cfg_item   : 配置项
 * \param _def_value  : 当配置项读取失败时返回该默认值
 * \return
 *     成功: 配置项数值
 * \n  失败: 默认值
 */
#define app_config_get_item(_cfg_id, _cfg_struct, _cfg_item, _def_value)  \
    \
    app_config_get_value(_cfg_id, offsetof(_cfg_struct, _cfg_item),  \
        SIZE(_cfg_struct, _cfg_item), _def_value)


#endif  // _LIB_APP_CONFIG_H


