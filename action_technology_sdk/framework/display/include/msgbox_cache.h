/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FRAMEWORK_DISPLAY_INCLUDE_MSGBOX_H_
#define FRAMEWORK_DISPLAY_INCLUDE_MSGBOX_H_

#include <errno.h>
#include <ui_manager.h>

/**
 * @typedef msgbox_popup_cb_t
 * @brief Callback to popup a specific msgbox
 *
 * This callback should return the GUI handle of the msgbox.
 * For LVGL, it is the root object of the msgbox widgets.
 *
 * @param msgbox_id Message box ID.
 * @param msg_id    Message ID, see @enum UI_MSG_ID.
 * @param msg_data  Message data.
 *                 For MSG_MSGBOX_POPUP, this is the root/parent handle to attached
 *                 For MSG_MSGBOX_PAINT, this is the exact message data passed by msgbox_cache_paint().
 *                 For MSG_MSGBOX_CLOSE, this is the window handle returned by MSG_MSGBOX_POPUP.
 * @param user_data user data
 *
 * @return the GUI handle of the msgbox if err == 0 else NULL
 */
typedef void * (*msgbox_popup_cb_t)(uint16_t msgbox_id, uint8_t msg_id, void *msg_data, void *user_data);

/**
 * @typedef msgbox_handler_t
 * @brief Callback to popup a specific msgbox
 *
 * This callback should return the GUI handle of the msgbox.
 * For LVGL, it is the root object of the msgbox widgets.
 *
 * @param msgbox_id Message box ID.
 * @param msg_id    Message ID, see @enum UI_MSG_ID.
 * @param msg_data  Message data.
 *                 For MSG_MSGBOX_POPUP, this is the exact user_data passed by msgbox_cache_popup().
 *                 For MSG_MSGBOX_CANCEL, this is the exact user_data passed by msgbox_cache_popup().
 *                 For MSG_MSGBOX_PAINT, this is the exact user_data passed by msgbox_cache_paint().
 *                 For MSG_MSGBOX_CLOSE, this is NULL so far.
 * @param hwnd The window handle which the msgbox is attached to. For LVGL, it is a pointer
 *             to structure lv_obj_t, user can assign user_data of it by lv_obj_set_user_data().
 *             Available for MSG_MSGBOX_POPUP, MSG_MSGBOX_PAINT and MSG_MSGBOX_CLOSE.
 *
 * @return 0 on success else negative error code
 */
typedef int (*msgbox_handler_t)(uint16_t msgbox_id, uint8_t msg_id, void * msg_data, void * hwnd);

/**
 * @struct msgbox_dsc
 * @brief Structure to describe msgbox
 */
typedef struct msgbox_dsc {
	uint16_t id;
	uint8_t order;
	uint8_t __pad[1];

	/*
	 * Initialize either popup_cb or handler, not required both.
	 * msgbox cache will always invoke handler if exist, otherwise popup_cb.
	 */
	msgbox_popup_cb_t popup_cb; /* deprecated variable */
	msgbox_handler_t handler;
} msgbox_dsc_t;

#ifdef CONFIG_LVGL

/**
 * @brief Initialize the msgbox cache
 *
 * @param ids array of msgbox ids
 * @param cbs array of msgbox popup callback
 * @param num number of msgbox ids
 *
 * @retval 0 on success else negative code.
 */
int msgbox_cache_init(const msgbox_dsc_t *dsc, uint8_t num);

/**
 * @brief Deinitialize the msgbox cache
 *
 * @retval N/A.
 */
void msgbox_cache_deinit(void);

/**
 * @brief Enable msgbox popup or not
 *
 * @retval N/A.
 */
void msgbox_cache_set_en(bool en);

/**
 * @brief Popup a msgbox
 *
 * @param id msgbox id
 * @param user_data user data
 *
 * @retval 0 on success else negative code.
 */
int msgbox_cache_popup(uint16_t id, void * user_data);

/**
 * @brief Popup a msgbox
 *
 * @param id msgbox id. CLose all if set to MSGBOX_ID_ALL
 * @param bsync synchronous flag
 *
 * @retval 0 on success else negative code.
 */
int msgbox_cache_close(uint16_t id, bool bsync);

/**
 * @brief Paint a msgbox
 *
 * @param id msgbox id
 * @param user_data user data
 *
 * @retval 0 on success else negative code.
 */
int msgbox_cache_paint(uint16_t id, void * user_data);

/**
 * @brief Get number of popups at present
 *
 * @retval number of popups.
 */
uint8_t msgbox_cache_num_popup_get(void);

/**
 * @brief Get the top most msgbox ID
 *
 * @retval the top most msgbox ID.
 */
uint16_t msgbox_cache_get_top(void);

/**
 * @brief Dump popups
 *
 * @retval N/A
 */
void msgbox_cache_dump(void);

#else /* CONFIG_LVGL */

static inline int msgbox_cache_init(const msgbox_dsc_t *dsc, uint8_t num)
{
	return -ENOSYS;
}

static inline void msgbox_cache_deinit(void) {}
static inline void msgbox_cache_set_en(bool en) {}
static inline int msgbox_cache_popup(uint16_t id, void *user_data) { return -ENOSYS; }
static inline int msgbox_cache_close(uint16_t id, bool bsync) { return -ENOSYS; }
static inline int msgbox_cache_paint(uint16_t id, uint32_t reason) { return -ENOSYS; }
static inline uint8_t msgbox_cache_num_popup_get(void) { return 0; }
static inline void msgbox_cache_dump(void) { }

#endif /* CONFIG_LVGL */

#endif /* FRAMEWORK_DISPLAY_INCLUDE_MSGBOX_H_ */
