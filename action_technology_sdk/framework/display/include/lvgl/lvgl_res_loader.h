/*
 * Copyright (c) 2021 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file resource loader interface
 */

#ifndef _LVGL_RES_LOADER_H
#define _LVGL_RES_LOADER_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include <lvgl/lvgl.h>
#include <res_manager_api.h>

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/
/** Possible resources preload states*/
typedef enum
{
	/**< Loading state*/
	LVGL_RES_PRELOAD_STATUS_LOADING = 0,
	/**< Finished state*/
	LVGL_RES_PRELOAD_STATUS_FINISHED,
	/**< Canceled state*/
	LVGL_RES_PRELOAD_STATUS_CANCELED,
}lvgl_res_preload_status_e;

/** Data structure of scene data, inited by 'lvgl_res_load_scene()' */
typedef struct
{
	/**< scene id*/
	uint32_t id;
	/**< horizontal coordinate*/
	lv_coord_t  x;
	/**< vertical coordinate*/
	lv_coord_t  y;
	/**< scene width*/
	uint16_t  width;
	/**< scene height*/
	uint16_t  height;
	/**< background color*/
	lv_color_t background;
	/**< scene opacity*/
	uint32_t  transparence;
	/**< internal pointer to scene data*/
	resource_scene_t* scene_data;
	/**< internal pointer to resource global info*/
	resource_info_t* res_info;
} lvgl_res_scene_t;

/** Data structure of resource group data*/
typedef struct
{
	/**< group id*/
	uint32_t id;
	/**< horizontal coordinate, relative to parent resource  */
	lv_coord_t x;
	/**< vertical coordinate, relative to parent resource*/
	lv_coord_t y;
	/**< group width */
	uint16_t width;
	/**< group height */
	uint16_t height;
	/**< internal pointer to group data*/
	resource_group_t* group_data;
	/**< internal pointer to resource global info*/
	resource_info_t* res_info;	
} lvgl_res_group_t;

/** Data structure of picture region data*/
typedef struct
{
	/**< horizontal coordinate, relative to parent resource  */
	lv_coord_t x;
	/**< vertical coordinate, relative to parent resource*/
	lv_coord_t y;
	/**< region width */
	uint16_t width;
	/**< region height */
	uint16_t height;
	/**< frame number of picture region*/
	uint32_t frames;
	/**< internal pointer to pic region data*/
	resource_picregion_t* picreg_data;
	/**< internal pointer to resource global info*/
	resource_info_t* res_info;	
}lvgl_res_picregion_t;

/** Data structure of string resource data*/
typedef struct
{
	/**< horizontal coordinate, relative to parent resource  */
	lv_coord_t  x;
	/**< vertical coordinate, relative to parent resource*/
	lv_coord_t  y;
	/**< text area width */
	uint16_t  width;
	/**< text area height */
	uint16_t  height;
	/**< text color */
	lv_color_t color;
	/** text background color*/
	lv_color_t bgcolor;
	/** text align */
	uint16_t align;
	/**< text content */
	uint8_t* txt;
	/**< internal pointer to resource global info*/
	resource_info_t* res_info;	
} lvgl_res_string_t;

/**********************
 * GLOBAL PROTOTYPES
 **********************/
/**
* @brief resource cache clearing function
*
* This routine release all cached bitmaps in resource cache system ,called by user view
*
* @param force_clear whether free memory occupied forcefully
*
*/ 
void lvgl_res_cache_clear(uint32_t force_clear);

/**
* @brief resource loader init function
*
* This routine init resource loader, called after system boot only once
*
* @return 0 if invoked succsess.
* @return -1 if invoked failed.
*/ 
int lvgl_res_loader_init(uint32_t screen_w, uint32_t screen_h);

/**
* @brief resource loader deinit function
*
* This routine deinit resource loader, called before system down
*
*/ 
void lvgl_res_loader_deinit(void);

/**
* @brief force close resource files
*
* @param sty_file file path of .sty file
* @param pic_file file path of .res file 
* @param str_file file path of .str file
* 
* @return 0 if invoked succsess.
* @return -1 if invoked failed.
*/
int lvgl_res_close_resources(const char* style_path, const char* picture_path, const char* text_path);

/**
* @brief resource scene loading funcion
*
* This routine load scene data into memory for later use.
*
* @param scene_id hashed scene identifier
* @param scene pointer to data structure to store scene data
* @param language language id
* @param sty_file file path of .sty file
* @param pic_file file path of .res file 
* @param str_file file path of .str file
*
* @return 0 if invoked succsess.
* @return -1 if invoked failed.
*/
int lvgl_res_load_scene(uint32_t scene_id, lvgl_res_scene_t* scene, const char* sty_file, const char* pic_file, const char* str_file);

/**
* @brief resource group loading funcion
*
* This routine load resource group data from parent scene into memory for later use.
*
* @param scene pointer to parent scene data
* @param id hashed resource group identifier
* @param group pointer to data structure to store group data
*
* @return 0 if invoked succsess.
* @return -1 if invoked failed.
*/
int lvgl_res_load_group_from_scene(lvgl_res_scene_t* scene, uint32_t id, lvgl_res_group_t* group);

/**
* @brief resource group loading funcion
*
* This routine load resource group data from parent group into memory for later use.
*
* @param group pointer to parent group data
* @param id hashed resource sub group identifier
* @param subgroup pointer to data structure to store sub group data
*
* @return 0 if invoked succsess.
* @return -1 if invoked failed.
*/
int lvgl_res_load_group_from_group(lvgl_res_group_t* group, uint32_t id, lvgl_res_group_t* subgroup);


/**
* @brief resource pictures loading funcion
*
* This routine load resource pictures from parent scene into memory for later use.
* picture data include bitmap data and image descriptions for lvgl to paint.
*
* @param scene pointer to parent scene data
* @param id pointer to array of hashed resource picture identifiers 
* @param img pointer to image description array
* @param pt pointer to point array of image positions.
* @param num number of pictures to be loaded.
*
* @return 0 if invoked succsess.
* @return -1 if invoked failed.
*/
int lvgl_res_load_pictures_from_scene(lvgl_res_scene_t* scene, const uint32_t* id, lv_image_dsc_t* img, lv_point_t* pt, uint32_t num);


/**
* @brief resource strings loading funcion
*
* This routine load resource string from parent scene into memory for later use.
*
* @param scene pointer to parent scene data
* @param id pointer to array of hashed resource string identifiers 
* @param str pointer to array for storing resource string data
* @param num number of strings to be loaded.
*
* @return 0 if invoked succsess.
* @return -1 if invoked failed.
*/
int lvgl_res_load_strings_from_scene(lvgl_res_scene_t* scene, const uint32_t* id, lvgl_res_string_t* str, uint32_t num);


/**
* @brief resource pictures loading funcion
*
* This routine load resource pictures from parent group into memory for later use.
* picture data include bitmap data and image descriptions for lvgl to paint.
*
* @param group pointer to parent group data
* @param id pointer to array of hashed resource picture identifiers 
* @param img pointer to image description array
* @param pt pointer to point array of image positions.
* @param num number of pictures to be loaded.
*
* @return 0 if invoked succsess.
* @return -1 if invoked failed.
*/
int lvgl_res_load_pictures_from_group(lvgl_res_group_t* group, const uint32_t* id, lv_image_dsc_t* img, lv_point_t* pt, uint32_t num);


/**
* @brief resource strings loading funcion
*
* This routine load resource string from parent group into memory for later use.
*
* @param group pointer to parent group data
* @param id pointer to array of hashed resource string identifiers 
* @param str pointer to array for storing resource string data
* @param num number of strings to be loaded.
*
* @return 0 if invoked succsess.
* @return -1 if invoked failed.
*/
int lvgl_res_load_strings_from_group(lvgl_res_group_t* group, const uint32_t* id, lvgl_res_string_t* str, uint32_t num);


/**
* @brief pictures unloading funcion
*
* This routine unload resource pictures from resource cache system.
*
* @param img pointer to image description array
* @param num number of pictures to be unloaded.
*
*/
void lvgl_res_unload_pictures(lv_image_dsc_t* img, uint32_t num);


/**
* @brief resource strings unloading funcion
*
* This routine unload resource strings from resource cache system.
*
* @param str pointer to resource string array
* @param num number of strings to be unloaded.
*
*/
void lvgl_res_unload_strings(lvgl_res_string_t* str, uint32_t num);


/**
* @brief resource group unloading funcion
*
* This routine unload resource group from resource cache system.
*
* @param group pointer to resource group data structure to be unloaded.
*
*/
void lvgl_res_unload_group(lvgl_res_group_t* group);


/**
* @brief resource scene unloading funcion
*
* This routine unload scene data structure from resource cache system.
*
* @param scene pointer to resource scene data structure to be unloaded.
*
*/
void lvgl_res_unload_scene(lvgl_res_scene_t* scene);


/**
* @brief picregion unloading funcion
*
* This routine unload picregion data structure from resource cache system.
*
* @param picreg pointer to picregion data structure to be unloaded.
*
*/ 
void lvgl_res_unload_picregion(lvgl_res_picregion_t* picreg);


/**
* @brief picregion loading funcion
*
* This routine load picregion data from parent group into memory for later use.
*
* @param group pointer to parent group data
* @param id resource identifier of the picregion
* @param res_picreg pointer to picregion data structure
*
* @return 0 if invoked succsess.
* @return -1 if invoked failed.
*/ 
int lvgl_res_load_picregion_from_group(lvgl_res_group_t* group, const uint32_t id, lvgl_res_picregion_t* res_picreg);

/************************************************************************************/
/*!
 * \par  Description:
 *    	从场景中加载图片区域
 * \param[in]    scene  场景数据指针
 * \param[in]    id   	图片区域id
 * \param[in]	 res_picreg	图片资源数据结构指针
 * \return       成功返回0，失败返回-1
 ************************************************************************************/
int lvgl_res_load_picregion_from_scene(lvgl_res_scene_t* scene, const uint32_t id, lvgl_res_picregion_t* res_picreg);

/************************************************************************************/
/*!
 * \par  Description:
 *    	从图片区域中加载图片
 * \param[in]    picreg  图片区域数据指针
 * \param[in]    start   要加载的起始帧数
 * \param[in]	 end	 要加载的结束帧数
 * \param[out]   img   	存放lvgl图片数据的数组指针
 * \return       成功返回0，失败返回-1
 ************************************************************************************/
int lvgl_res_load_pictures_from_picregion(lvgl_res_picregion_t* picreg, uint32_t start, uint32_t end, lv_image_dsc_t* img);


/************************************************************************************/
/*!
 * \par  Description:
 *    	从默认路径加载图片
 * \param[in]    group_id  第一级资源组id，为可选项，不存在时填0
 * \param[in]    subgroup_id  第二级资源组id，为可选项，不存在时填0
 * \param[in]    id   	存放图片资源id的数组指针，为必填项
 * \param[out]   img   	存放lvgl图片数据的数组指针
 * \param[out]   pt   	存放图片坐标的数组指针
 * \param[in]	 num	数组元素个数
 * \return       成功返回0，失败返回-1
 * \note		默认路径从场景id起始，具体资源id结束，最多支持中间两级资源组
 ************************************************************************************/
int lvgl_res_load_pictures(uint32_t group_id, uint32_t subgroup_id, uint32_t* id, lv_image_dsc_t* img, lv_point_t* pt, uint32_t num);

/************************************************************************************/
/*!
 * \par  Description:
 *    	从默认路径加载字符串
 * \param[in]    group_id  第一级资源组id，为可选项，不存在时填0
 * \param[in]    subgroup_id  第二级资源组id，为可选项，不存在时填0
 * \param[in]    id   	存放字符串资源id的数组指针，为必填项
 * \param[out]   str   	存放用于lvgl显示的字符串数据的数组指针
 * \param[in]	 num	数组元素个数
 * \return       成功返回0，失败返回-1
 * \note		默认路径从场景id起始，具体资源id结束，最多支持中间两级资源组
 ************************************************************************************/
int lvgl_res_load_strings(uint32_t group_id, uint32_t subgroup_id, uint32_t* id, lvgl_res_string_t* str, uint32_t num);

/************************************************************************************/
/*!
 * \par  Description:
 *    	使用默认路径加载图片区域
 * \param[in]    group_id  第一级资源组id，为可选项，不存在时填0
 * \param[in]    subgroup_id  第二级资源组id，为可选项，不存在时填0
 * \param[in]    id   	图片区域id，为必填项
 * \param[in]	 res_picreg	图片资源数据结构指针
 * \return       成功返回0，失败返回-1
 * \note		默认路径从场景id起始，具体资源id结束，最多支持中间两级资源组
 ************************************************************************************/
int lvgl_res_load_picregion(uint32_t group_id, uint32_t subgroup_id, uint32_t picreg_id, lvgl_res_picregion_t* res_picreg);


/**
* @brief pictures preload initiating funcion
*
* This routine add all or ceratin picture resources in a scene to preload list.
*
* @param scene_id hashed scene identifier of the scene to operate on. All picture resources would be added to preload list if resource_id is NULL.
* @param resource_id list of id to add to preload list. if a group id is specified, all pictures in the group are added to preload list.
* @param resource_num number of ids in id list
* @param callback callback function to notify status change
* @param user_data param passes to callback function
* @param sty_file file path of .sty file
* @param pic_file file path of .res file 
* @param str_file file path of .str file
*
* @return 0 if invoked succsess.
* @return -1 if invoked failed.
*/
int lvgl_res_preload_scene_compact(uint32_t scene_id, const uint32_t* resource_id, uint32_t resource_num, void (*callback)(int32_t, void *), void* user_data, 
											const char* style_path, const char* picture_path, const char* text_path);

/**
* @brief pictures preload initiating funcion
*
* This routine add picture resources in a scene to preload list.
*
* @param scene_id hashed scene identifier of the scene to operate on
* @param scene pointer to the scene data structure
* @param id pointer to the array of ids of picture resources to preload
* @param number picture number of the array
*
* @return 0 if invoked succsess.
* @return -1 if invoked failed.
*/
int lvgl_res_preload_pictures_from_scene(uint32_t scene_id, lvgl_res_scene_t* scene, const uint32_t* id, uint32_t num);

/**
* @brief pictures preload initiating funcion
*
* This routine add picture resources in a group to preload list.
*
* @param scene_id hashed scene identifier of the scene to operate on
* @param group pointer to the group data structure
* @param id pointer to the array of ids of picture resources to preload
* @param number picture number of the array
*
* @return 0 if invoked succsess.
* @return -1 if invoked failed.
*/
int lvgl_res_preload_pictures_from_group(uint32_t scene_id, lvgl_res_group_t* group, const uint32_t* id, uint32_t num);


/**
* @brief picregion preload initiating funcion
*
* This routine add picregion frames to preload list.
*
* @param scene_id hashed scene identifier of the scene to operate on
* @param picreg pointer to the picregion data structure
* @param start start frame to preload
* @param end end frame to preload
*
* @return 0 if invoked succsess.
* @return -1 if invoked failed.
*/
int lvgl_res_preload_pictures_from_picregion(uint32_t scene_id, lvgl_res_picregion_t* picreg, uint32_t start, uint32_t end);

/**
* @brief resource preload canceling funcion
*
* This routine cancel all preloads, and release memory already used
*
*
* @return 0 if invoked succsess.
* @return -1 if invoked failed.
*/
int lvgl_res_preload_cancel(void);

/**
* @brief resource preload canceling funcion
*
* This routine cancel preloading of specific scene, and release memory already used
*
* @param scene_id hashed scene identifier of the scene to operate on
*
* @return 0 if invoked succsess.
* @return -1 if invoked failed.
*/
int lvgl_res_preload_cancel_scene(uint32_t scene_id);

/**
* @brief scene resource release funcion
*
* This routine releases all memory used by resources preloaded in compact form in specific scene.
*
* @param scene_id hashed scene identifier of the scene to operate on
*
*/
void lvgl_res_unload_scene_compact(uint32_t scene_id);

/**
* @brief default resource preload callback funcion
*
* This routine is the default callback function of resource preload thread.
*
* @param status current preload status
* @param view_id view id to notify.(view id is cast from void* to int) 
*
*/
void lvgl_res_scene_preload_default_cb_for_view(int32_t status, void *view_id);

/**
* @brief regular resources setting funcion
*
* This routine flag certain picture resources as regular, 
* regular pictures will remain in memory no matter the scene resources be unloaded or not
*
* @param scene_id hashed scene identifier of the scene to operate on
* @param group_id hashed scene identifier of the group where regular resources belong to (can be NULL).
* @param subgroup_id hashed scene identifier of the subgroup where regular resources belong to (can be NULL).
* @param id list of ids of picture resources
* @param num number of picture resources
* @param style_path file path of .sty file where the scene is defined 
* @param picture_path file path of .res file
* @param texxt_path file path of string file
*
* @return 0 if invoked succsess.
* @return -1 if invoked failed.
*/
int32_t lvgl_res_set_pictures_regular(uint32_t scene_id, uint32_t group_id, uint32_t subgroup_id, uint32_t* id, uint32_t num,  
									const uint8_t* style_path, const char* picture_path, const char* text_path);

/**
* @brief regular resources clearing funcion
*
* This routine release memory of loaded regular resources in specific scene
*
* @param scene_id hashed scene identifier of the scene to operate on
* @param style_path file path of .sty file where the scene is defined 
*
* @return 0 if invoked succsess.
* @return -1 if invoked failed.
*/
int32_t lvgl_res_clear_regular_pictures(uint32_t scene_id, const uint8_t* style_path);

/**
* @brief resource loading check funcion
*
* This routine check whether resources of the scene loaded or not.
*
* @param scene_id hashed scene identifier of the scene to operate on
*
* @return 1 if scene is loaded.
* @return 0 if scene isn't loaded.
*/
int lvgl_res_scene_is_loaded(uint32_t scene_id);

/**
* @brief default resources preload setting funcion
*
* This routine set all or ceratin picture resources in a scene to be preload later.
*
* @param scene_id hashed scene identifier of the scene to operate on. All picture resources would be added to preload list if resource_id is NULL.
* @param resource_id list of id to add to preload list. if a group id is specified, all pictures in the group are added to preload list.
* @param resource_num number of ids in id list
* @param callback callback function after preload finished, set it to NULL if there's no special need.
* @param sty_file file path of .sty file
* @param pic_file file path of .res file 
* @param str_file file path of .str file
*
* @return 0 if invoked succsess.
* @return -1 if invoked failed.
*/
int lvgl_res_preload_scene_compact_default_init(uint32_t scene_id, const uint32_t* resource_id, uint32_t resource_num, void (*callback)(int32_t, void *),
											const char* style_path, const char* picture_path, const char* text_path);

/**
* @brief scene default preload funcion
*
* This routine preload a preset scene resources, only can be used if lvgl_res_preload_scene_compact_default_init()
* has been called of the same scene.
*
* @param scene_id hashed scene identifier of the scene to operate on
* @param view_id param to be passed to callback function
* @param reload_update flag of whether it's first layout or focus update, true for focus upload, false for first layout
* @param load_serial flag of whethr it's a async load, true for sync serial loading, false for async loading.
*
* @return 0 if invoked succsess.
* @return -1 if invoked failed.
*/
int lvgl_res_preload_scene_compact_default(uint32_t scene_id, uint32_t view_id, bool reload_update, bool load_serial);


int lvgl_res_set_current_string_res(const uint8_t** string_res, uint32_t string_cnt);
uint8_t* lvgl_res_get_string(uint8_t* key);


/**********************
 *      MACROS
 **********************/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*_LVGL_RES_LOADER_H*/

