/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ui service interface
 */

#ifndef FRAMEWORK_DISPLAY_INCLUDE_UI_SERVICE_H_
#define FRAMEWORK_DISPLAY_INCLUDE_UI_SERVICE_H_

/**
 * @defgroup ui_manager_apis app ui Manager APIs
 * @ingroup system_apis
 * @{
 */

#include <input_manager.h>
#include <view_manager.h>
#include <ui_surface.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
	VIEW_SCROLL_MEM_DEF = 0,
	VIEW_SCROLL_MEM_SAVE,
	VIEW_SCROLL_MEM_LOWEST,
};

typedef struct ui_srv_init_param {
	/* allow paused view (has no surface buffer) to scroll */
	uint8_t scrl_mem_mode;
	uint8_t view_ovl_opa;
	uint8_t trans_buf_cnt;
} ui_srv_init_param_t;

/**
 * @struct ui_view_gui_callback
 * @brief Structure to register gui view callback
 */
typedef struct ui_view_gui_callback {
	/* init the GUI, must return the GUI context/display from surface */
	void * (*init)(surface_t * surface);
	/* deinit the GUI context/display */
	void (*deinit)(void * display);
	/* draw(refresh) paused */
	int (*pause)(void * display);
	/* draw(refresh) resumed, also decide whether invalidate all */
	int (*resume)(void * display, bool full_invalidate);
	/* set as the default context for convenience */
	int (*set_default)(void * display);
	/* make the GUI can get the input event */
	int (*focus)(void * display, bool focus);
	/* draw(refresh) the GUI dirty or full-screen region immediately */
	int (*refresh)(void * display, bool full_refresh);
	/* update orientation of the GUI display */
	int (*update_orientation)(void * display, uint16_t rotation);
	/* GUI period task handler */
	void (*task_handler)(void);
} ui_view_gui_callback_t;

/**
 * @struct ui_input_gui_callback
 * @brief Structure to register gui input callback
 */
typedef struct ui_input_gui_callback {
	/* do nothing until next release, except preserving the current pressing
	 * widget's pressed state.
	 * will be called when view scrolling begin or focus change
	 */
	void (*wait_release)(void * user_data);
	/* enable/disable input */
	void (*enable)(bool en, void * user_data);
	/* put input data into GUI */
	int (*put_data)(const input_dev_data_t * data, void * user_data);
} ui_input_gui_callback_t;

/**
 * @struct ui_gesture_callback
 * @brief Structure to register gesture callback
 */
typedef struct ui_gesture_callback {
	/* scroll begin.
	 * return 0 on success else negative code
	 */
	int (*scroll_begin)(input_dev_t * pointer_dev);
	/* scroll the gui views */
	void (*scroll)(input_dev_t * pointer_dev);
	/* scroll end */
	void (*scroll_end)(input_dev_t * pointer_dev);
} ui_gesture_callback_t;

/**
 * @brief Register gui callback to view manager
 *
 * @param type ui view type, see @enum UI_VIEW_TYPE
 * @param opa_pixel_format perferred opaque pixel format
 * @param trans_pixel_format perferred transparent pixel format
 * @param callback pointer to structure ui_view_gui_callback
 *
 * @retval 0 on success else negative errno code.
 */
int ui_service_register_gui_view_callback(uint8_t type,
		uint32_t opa_pixel_format, uint32_t trans_pixel_format,
		const ui_view_gui_callback_t *callback);

/**
 * @brief Register gui callback to input dispatcher
 *
 * @param type input type, INPUT_DEV_TYPE_x
 * @param callback pointer to structure ui_input_gui_callback
 * @param user_data user data passed to callback function.
 *
 * @retval 0 on success else negative errno code.
 */
int ui_service_register_gui_input_callback(uint8_t type,
		const ui_input_gui_callback_t *callback, void *user_data);

/**
 * @brief Register gesture callback to ui service
 *
 * @param callback pointer to structure ui_input_gui_callback
 *
 * @retval 0 on success else negative errno code.
 */
int ui_service_register_gesture_callback(const ui_gesture_callback_t *callback);

/**
 * @brief Register gesture default callback to ui service
 *
 * @retval 0 on success else negative errno code.
 */
int ui_service_register_gesture_default_callback(void);

#ifdef __cplusplus
}
#endif
/**
 * @} end defgroup system_apis
 */

#endif /* FRAMEWORK_DISPLAY_INCLUDE_UI_SERVICE_H_ */
