/*!
 * \file      app_config.c
 * \brief     应用配置接口
 * \details   
 * \author    
 * \date      
 * \copyright Actions
 */

#include "list.h"
#include <app_config.h>
#include <sdfs.h>


/*!
 * \brief 获取最小值
 */
#define _MIN(_a, _b)  (((_a) < (_b)) ? (_a) : (_b))


/*!
 * \brief 获取最大值
 */
#define _MAX(_a, _b)  (((_a) > (_b)) ? (_a) : (_b))


typedef struct
{
    u8_t   format[4];
    u8_t   magic [4];
    u16_t  user_version;
    u8_t   minor_version;
    u8_t   major_version;

    u16_t  total_size;
    u16_t  num_cfg_items;
    
} app_config_file_header_t;


typedef struct
{
    u32_t  cfg_id:8;
    u32_t  sub_pos:12;
    u32_t  cfg_len:12;
    
} app_config_item_info_t;


typedef struct
{
    struct list_head  node;

	void *file;
	
    int    num_cfg_items;
    u8_t*  cfg_data;
    
    app_config_item_info_t*  cfg_items;
    
} app_config_file_info_t;



extern struct list_head  app_config_file_list;


struct list_head  app_config_file_list = LIST_HEAD_INIT(app_config_file_list);


/*!
 * \brief 加载应用配置数据文件
 * \n  注: 使用时必须先加载 defcfg.bin, 再加载 usrcfg.bin
 * \n
 * \n  cache_cfg_info : 缓存配置项信息可以加快查找速度
 * \n  cache_cfg_data : 缓存配置项数据可以加快读取速度
 * \return
 *     成功: OK
 * \n  失败: FAIL
 */
bool app_config_load
(
    const char* file_name, bool cache_cfg_info, bool cache_cfg_data)
{
    struct sd_file *file;

    app_config_file_info_t*   file_info;
    app_config_file_header_t  hdr;
    int len = sizeof(app_config_file_header_t); 
	
    if ((file = sd_fopen(file_name)) == NULL)
    {
        goto err;
    }

    if (sd_fread(file, &hdr, sizeof(app_config_file_header_t)) != len)
    {
        goto err;
    }

    if (memcmp(hdr.format, "CFG", 4) != 0 ||
        memcmp(hdr.magic,  "VER", 4) != 0)
    {
        goto err;
    }
    
    file_info = app_mem_malloc(sizeof(app_config_file_info_t));
    memset(file_info, 0, sizeof(app_config_file_info_t));
	
    file_info->num_cfg_items = hdr.num_cfg_items;

    if (cache_cfg_info)
    {
        int  size = hdr.num_cfg_items * sizeof(app_config_item_info_t);

        if (size > 0)
        {
            file_info->cfg_items = app_mem_malloc(size);
            sd_fread(file, file_info->cfg_items, size);	
        }
    }

    if (cache_cfg_data)
    {
        int  offs = 
            sizeof(app_config_file_header_t) + 
            hdr.num_cfg_items * sizeof(app_config_item_info_t);
        
        int  size = hdr.total_size - offs;

        if (size > 0)
        {
            file_info->cfg_data = app_mem_malloc(size);

            sd_fseek(file, offs, FS_SEEK_SET);
            sd_fread(file, file_info->cfg_data, size);	
        }
    }

    if (cache_cfg_info && cache_cfg_data)
    {
        sd_fclose(file);
    }
    else
    {
        file_info->file = file;
    }

    list_add_tail(&file_info->node, &app_config_file_list);
    
	return true;
		
err:
	return false;
}


/*!
 * \brief 读取应用配置数据
 * \n  配置数据在读取前会被初始化为默认值 0
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
int app_config_read
(
    unsigned int cfg_id, void* cfg_data, unsigned int cfg_offs, unsigned int cfg_len)
{
    int  ret_val = 0;
    
    app_config_file_info_t*  file_info;
    app_config_item_info_t   cfg_item;

    int  file_offs;
    int  i;
    
    os_sched_lock();

    memset(cfg_data, 0, cfg_len);

    /* 顺序读取配置文件,
     * 后面读取的新数据覆盖前面读取的数据
     */
    list_for_each_entry(file_info, &app_config_file_list, node)
    {
        int  start_pos = 
            sizeof(app_config_file_header_t) + 
            file_info->num_cfg_items * sizeof(app_config_item_info_t);

        int  result = 0;
        
        file_offs = start_pos;
        
        for (i = 0; i < file_info->num_cfg_items; i++, file_offs += cfg_item.cfg_len)
        {
            int  offs;
            int  len;

            if (file_info->cfg_items != NULL)
            {
                cfg_item = file_info->cfg_items[i];
            }
            else
            {
                offs = sizeof(app_config_file_header_t) + 
                    i * sizeof(app_config_item_info_t);

				sd_fseek(file_info->file, offs, FS_SEEK_SET);
				sd_fread(file_info->file, &cfg_item,  sizeof(app_config_item_info_t)); 
            }

            if (cfg_item.cfg_id != cfg_id)
            {
                continue;
            }

            /* 只读取覆盖需要的部分数据
             */
            offs = _MAX(cfg_item.sub_pos, cfg_offs);
            len  = _MIN(cfg_item.sub_pos + cfg_item.cfg_len, cfg_offs + cfg_len) - offs;

            if (len <= 0)
            {
                continue;
            }

            if (file_info->cfg_data != NULL)
            {
                int  pos = file_offs + (offs - cfg_item.sub_pos) - start_pos;
                
                memcpy((u8_t*)cfg_data + (offs - cfg_offs), 
                    file_info->cfg_data + pos, len);
            }
            else
            {
                int    pos  = file_offs + (offs - cfg_item.sub_pos);
                void*  data = (u8_t*)cfg_data + (offs - cfg_offs);

				sd_fseek(file_info->file, pos, FS_SEEK_SET);
				sd_fread(file_info->file, data,  len);               
            }

            result = 1;
        }

        ret_val += result;
    }

    os_sched_unlock();

    return ret_val;
}


