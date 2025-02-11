/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file view manager interface
 */

#ifndef __VIEW_MANGER_H__
#define __VIEW_MANGER_H__

/**
 * @defgroup ui_manager_apis app ui Manager APIs
 * @ingroup system_apis
 * @{
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <ui_region.h>
#include <sys/slist.h>
#include <input_manager.h>
#include <ui_surface.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define TRANSMIT_ALL_KEY_EVENT  0xFFFFFFFF

#define UI_VIEW_ANIM_SHIFT (10)
#define UI_VIEW_ANIM_RANGE (1 << UI_VIEW_ANIM_SHIFT)

#define VIEW_INVALID_ID   VIEW_ID_ALL
#define MSGBOX_INVALID_ID MSGBOX_ID_ALL

/****************************************************************************
* Public Types
****************************************************************************/

/**
 * @brief UI Message ID
 *
 * UI message id enumeration
 */
enum UI_MSG_ID {
	MSG_VIEW_NULL = 0,

	MSG_VIEW_CREATE,  /* creating view, MSG_VIEW_PRELOAD or MSG_VIEW_LAYOUT will be sent to view */
	MSG_VIEW_PRELOAD, /* preload view resource */
	MSG_VIEW_LAYOUT,  /* view layout inflate, can be called by the view itself if required */
	MSG_VIEW_DELETE,  /* deleting view */
	MSG_VIEW_PAINT,   /* update the view normally driven by data changed */
	MSG_VIEW_REFRESH, /* refresh view surface to display */
	MSG_VIEW_FOCUS,   /* view becomes focused */
	MSG_VIEW_DEFOCUS, /* view becomes de-focused */
	MSG_VIEW_PAUSE,   /* view surface becomes unreachable */
	MSG_VIEW_RESUME,  /* view surface becomes reachable */
	MSG_VIEW_UPDATE,  /* resource reload completed */
	MSG_VIEW_RESUME_DISPLAY, /* display unblank */
	MSG_VIEW_KEY,     /* key event, param is struct msg_view_key_data */

	MSG_VIEW_SET_HIDDEN, /* 14 */
	MSG_VIEW_SET_ORDER,
	MSG_VIEW_SET_POS,
	MSG_VIEW_SET_DRAG_ATTRIBUTE,

	MSG_VIEW_SET_CALLBACK, /* 18 */

	MSG_VIEW_SCROLL_BEGIN,
	MSG_VIEW_SCROLL_END,

	MSG_VIEW_SET_BUF_COUNT,

	MSG_DISPLAY_POST, /* 22 */
	MSG_DISPLAY_RESUME,
	MSG_DISPLAY_SUSPEND,
	MSG_DISPLAY_LOCK,
	MSG_DISPLAY_ROTATE,

	/* msgbox */
	MSG_MSGBOX_POPUP,  /* popup message box */
	MSG_MSGBOX_CLOSE,  /* close message box */
	MSG_MSGBOX_CANCEL, /* cancel message box, sine popup unavailable */
	MSG_MSGBOX_PAINT,  /* update message box */
	MSG_MSGBOX_KEY,    /* key event passed to message box, param is struct msg_view_key_data */

	/* gesture setting */
	MSG_GESTURE_SET_SCROLL_DIR, /* 32 */
	MSG_GESTURE_LOCK_SCROLL,
	MSG_GESTURE_STOP_SCROLL,
	MSG_GESTURE_WAIT_RELEASE,

	MSG_VIEW_TRANSFORM, /* 36 */
	MSG_VIEW_TRANSFORM_START,
	MSG_VIEW_TRANSFORM_END,

	/* user message offset for specific view */
	MSG_VIEW_USER_OFFSET = 128,
};

/**
 * @brief UI Callback ID
 *
 * UI callback ID enumeration
 */
enum UI_CALLBACK_ID {
	UI_CB_MSGBOX,
	UI_CB_SCROLL,
	UI_CB_MONITOR,
	UI_CB_KEYEVENT,
	UI_CB_TRANSFORM_SWITCH,
	UI_CB_TRANSFORM_SCROLL,
	UI_CB_TRANSFORM_VSCROLL,
	UI_NUM_CBS,
};

/**
 * @brief View ID
 *
 * view id enumeration
 */
enum UI_VIEW_ID {
	VIEW_ID_ALL = 0,
	VIEW_ID_USER_OFFSET = 1,
};

/**
 * @brief Msgbox ID
 *
 * msgbox id enumeration
 */
enum UI_MSGBOX_ID {
	MSGBOX_ID_ALL = 0,
	MSGBOX_ID_USER_OFFSET = 1,
};

/**
 * @brief UI View Type
 *
 * View GUI type
 */
enum UI_VIEW_TYPE {
	UI_VIEW_Unknown = 0,
	UI_VIEW_LVGL,
	UI_VIEW_USER, /* user defined */

	NUM_VIEW_TYPES,
};

/**
 * @brief UI View Create Flags
 *
 * View create flags
 */
enum UI_VIEW_CREATE_FLAGS {
	UI_CREATE_FLAG_NO_PRELOAD = (1 << 0), /* ignore resource preload when created */
	UI_CREATE_FLAG_SHOW = (1 << 1), /* show the view by default */
	UI_CREATE_FLAG_TRANSPARENT = (1 << 2), /* view content is transparent */
	UI_CREATE_FLAG_FLOATING = (1 << 3), /* the view will never get focused */
	UI_CREATE_FLAG_NO_FB = (1 << 4), /* initial no surface buffer */
	UI_CREATE_FLAG_POST_ON_PAINT = (1 << 5), /* only post the view to display when painted */
	/* indicate the view can be scrolled by setting drag attr.
	 * views with flag UI_CREATE_FLAG_FLOATING will never be scrollable.
	 */
	UI_CREATE_FLAG_SCROLLABLE = (1 << 6),
};

/**
 * @brief UI View State Flags
 *
 * View state flags
 */
enum UI_VIEW_FLAGS {
	UI_FLAG_HIDDEN       = (1 << 0),
	UI_FLAG_FOCUSED      = (1 << 1), /* user observable focus status */
	UI_FLAG_PAUSED       = (1 << 2),

	UI_FLAG_INFLATED     = (1 << 3), /* layout inflated */
	/* UI_FLAG_PAINTED      = (1 << 4), */
	UI_FLAG_UPDATED      = (1 << 5), /*view updated, dont update again until defocused*/

	UI_FLAG_DELETING     = (1 << 7), /* view is deleting */
};

/**
 * @brief UI View Refresh State Flags
 *
 * View refresh state flags
 */
enum UI_VIEW_REFRESH_FLAGS {
	/* flags for view refresh */
	UI_REFR_FLAG_MOVED         = (1 << 0), /* refresh due to position changed */
	UI_REFR_FLAG_CHANGED       = (1 << 1), /* refresh due to content changed */
	UI_REFR_FLAG_FIRST_CHANGED = (1 << 2), /* first refresh in frame due to content changed */
	UI_REFR_FLAG_LAST_CHANGED  = (1 << 3), /* last refresh in frame due to content changed */
};

/**
 * @brief UI View Drag Attribute
 *
 * View drag attribute
 *
 * DOWN/UP/LEFT/RIGHT indicates the drop/move direction
 */
enum UI_VIEW_DRAG_ATTRIBUTE {
	UI_DRAG_NONE      = 0,
	UI_DRAG_DROPDOWN  = (1 << 0),
	UI_DRAG_DROPUP    = (1 << 1),
	UI_DRAG_DROPLEFT  = (1 << 2),
	UI_DRAG_DROPRIGHT = (1 << 3),
	UI_DRAG_MOVEDOWN  = (1 << 4),
	UI_DRAG_MOVEUP    = (1 << 5),
	UI_DRAG_MOVELEFT  = (1 << 6),
	UI_DRAG_MOVERIGHT = (1 << 7),

	UI_DRAG_SNAPEDGE  = (1 << 8), /* can snap back to edge */

	UI_DRAG_DROP_MASK = 0x0f,
	UI_DRAG_MOVE_MASK = 0xf0,
	UI_DRAG_DIR_MASK  = (UI_DRAG_DROP_MASK | UI_DRAG_MOVE_MASK),
};

/**
 * @brief UI View Animation State
 *
 * View animation state
 *
 * DOWN/UP/LEFT/RIGHT indicates the drop/move direction
 */
enum UI_VIEW_ANIMATION_STATE {
	UI_ANIM_NONE,
	UI_ANIM_START,
	UI_ANIM_RUNNING,
	UI_ANIM_STOP,
};

/**
 * @brief UI View Animation Type
 *
 * View animation type
 *
 * DOWN/UP/LEFT/RIGHT indicates the drop/move direction
 */
enum UI_VIEW_SLIDE_ANIMATION_TYPE {
	UI_ANIM_SLIDE_IN_DOWN = 1,
	UI_ANIM_SLIDE_IN_UP,
	UI_ANIM_SLIDE_IN_RIGHT,
	UI_ANIM_SLIDE_IN_LEFT,
	UI_ANIM_SLIDE_OUT_UP,
	UI_ANIM_SLIDE_OUT_DOWN,
	UI_ANIM_SLIDE_OUT_LEFT,
	UI_ANIM_SLIDE_OUT_RIGHT,
};

struct ui_view_anim_cfg;
struct view_data;

typedef struct ui_transform_param {
	graphic_buffer_t *dst;
	graphic_buffer_t *src_old;
	graphic_buffer_t *src_new; /* can be NULL to indicate background color */
	ui_region_t region_old;
	ui_region_t region_new;
	ui_region_t crop_old;
	ui_region_t crop_new;
	uint16_t rotation;
	uint8_t first_frame : 1;
	uint8_t round_screen : 1;

	/* Only for switch effect's touch tracking */
	uint8_t tp_pressing;
	ui_point_t tp_start;
	ui_point_t tp_vect;
} ui_transform_param_t;

/**
 * @typedef ui_view_proc
 * @brief Callback API to process view message
 * @param view_id view id
 * @param view_data pointer to structure view_data
 * @param msg_id message id
 * @param msg_data message data
 *
 * @retval 0 on success else error codes
 */
typedef int (*ui_view_proc_t)(uint16_t view_id, uint8_t msg_id, void *msg_data); /* deprecated prototype */
typedef int (*ui_view_proc2_t)(uint16_t view_id, struct view_data *view_data, uint8_t msg_id, void *msg_data);

typedef int (*ui_get_state_t)(void);
typedef bool (*ui_state_match_t)(uint32_t current_state, uint32_t match_state);

/**
 * @typedef ui_msgbox_popup_cb
 * @brief Callback API to popup/close the msgbox
 * @param msgbox_id msgbox id
 * @param msg_id message id
 * @param msg_data message data
 *                 For MSG_MSGBOX_POPUP/CLOSE, this is the view id to attached
 *                 For MSG_MSGBOX_PAINT, this is the exact message data passed to uisrv by MSG_MSGBOX_PAINT.
 *                 For MSG_MSGBOX_KEY, this is pointer to structure ui_key_msg_data
 * @param user_data user data, only passed for MSG_MSGBOX_POPUP/CANCEL
 *
 * @retval N/A
 */
typedef void (*ui_msgbox_popup_cb_t)(uint16_t msgbox_id, uint8_t msg_id, void *msg_data, void *user_data);

/**
 * @typedef ui_scroll_cb
 * @brief Callback API to notify view scroll result, should not block or call any GUI API.
 * @param view_id view id
 * @param msg_id view message id, MSG_VIEW_SCROLL_BEGIN or MSG_VIEW_SCROLL_END
 *
 * @retval N/A
 */
typedef void (*ui_scroll_cb_t)(uint16_t view_id, uint8_t msg_id);

/**
 * @typedef ui_monitor_cb
 * @brief Callback API to notify view state, like focus/de-focus, layout, delete
 * @param view_id view id
 * @param msg_id view message id
 * @param msg_data view message data
 *
 * @retval N/A
 */
typedef void (*ui_monitor_cb_t)(uint16_t view_id, uint8_t msg_id, void *msg_data);

/**
 * @typedef ui_keyevent_cb
 * @brief Callback API to notify (gesture) event
 * @param view_id the focused view when event takes place
 * @param event event id
 *
 * @retval N/A
 */
typedef void (*ui_keyevent_cb_t)(uint16_t view_id, uint32_t event);

/**
 * @typedef ui_transform_cb
 * @brief Callback API to perform transform for UI scroll/switch effects
 * @param param pointer transform parameters
 * @param trans_end trans_end flag, user can write 1 to force it end
 *
 * @retval N/A
 */
typedef void (*ui_transform_cb_t)(const ui_transform_param_t *param, int *trans_end);

/**
 * @typedef ui_view_anim_path
 * @brief Callback API for setting view dragging animation
 * @param scroll_throw_vect indicated the drag speed, it uses similar algorithm as LVGL.
 * @param cfg pointer to structure ui_view_anim_cfg filled by callback
 *
 * @retval N/A
 */
typedef void (*ui_view_drag_anim_cb_t)(uint16_t view_id,
		const ui_point_t *scroll_throw_vect, struct ui_view_anim_cfg *cfg);

/**
 * @typedef ui_view_anim_path
 * @brief Callback API to compute the view move position
 * @param elaps the ratio of elapsed time to the duration. range [0, UI_VIEW_ANIM_RANGE]
 *
 * @retval the interpolation value of move ratio, range [0, UI_VIEW_ANIM_RANGE]
 */
typedef int32_t (*ui_view_anim_path_cb_t)(int32_t elaps);

/* notify the user the animation has stopped */
typedef void (*ui_view_anim_stop_cb_t)(uint16_t view_id, const ui_point_t *pos);

/**
 * @struct ui_key_msg_data
 * @brief Structure hoding data of MSG_VIEW_KEY or MSG_MSGBOX_KEY
 *
 * @var event key event
 * @var done the key event processing is done
 */
typedef struct ui_key_msg_data {
	uint32_t event;
	bool done; /* the processing is done */
} ui_key_msg_data_t;

typedef ui_key_msg_data_t msg_view_key_data_t;

/**
 * @struct view_user_msg_data
 * @brief Structure hoding data of MSG_VIEW_PAINT or user-defined message
 *
 * @var id user-defined UI id
 * @var data user-defined message data
 */
typedef struct view_user_msg_data {
	uint16_t id;
	union {
		void * data;
		uintptr_t value;
	};
} view_user_msg_data_t;

/**
 * @struct view_data
 * @brief Structure hoding data of view gui-specific data
 *
 * @var display gui specific display
 * @var surface gui rendering surface
 * @var presenter user-specific presenter
 * @var user_data user-specific data
 */
typedef struct view_data {
	/* gui data */
	//void *context; /* gui context */
	void *display; /* gui display */
	struct surface *surface; /* gui surface to draw on */

	/* custom data */
	const void *presenter; /* view presenter passed by app */
	void *user_data; /* application defined data */
} view_data_t;

/* app entry structure */
typedef struct view_entry {
	/** app id */
	const char *app_id;
	/** proc function of view */
	ui_view_proc_t  proc; /* deprecated variable */
	ui_view_proc2_t proc2;
	/**view id */
	uint16_t id;
	/**default order */
	uint8_t default_order;
	/**view type */
	uint8_t type;
	/** view width */
	uint16_t width;
	/** view height */
	uint16_t height;
} view_entry_t;

typedef struct {
	sys_snode_t node;

	const view_entry_t *entry;
	ui_region_t region;
	uint32_t inst_id;    /* instance ID */
	uint8_t flags;       /* see enum UI_VIEW_FLAGS */
	uint8_t refr_flags;  /* see enum UI_VIEW_REFRESH_FLAGS */
	uint8_t create_flags;
	uint8_t painted;     /* indicate first paint finished after layout  */
	uint8_t order;
	/*
	 * internal focus status
	 * modified both by uisrv and display-workq (byte-write is atomic)
	 */
	uint8_t focused;
	uint16_t drag_attr;
	ui_view_drag_anim_cb_t drag_anim_cb;

	view_data_t data;
} ui_view_context_t;

typedef struct ui_view_anim_cfg {
	ui_point_t start;
	ui_point_t stop;
	uint16_t duration;
	ui_view_anim_path_cb_t path_cb;
	ui_view_anim_stop_cb_t stop_cb;
} ui_view_anim_cfg_t;

typedef struct {
	uint8_t state;
	uint8_t is_slide : 1;

	uint16_t view_id;
	uint16_t last_view_id;
	ui_point_t last_view_offset;

	uint32_t start_time;
	uint16_t elapsed;

	ui_view_anim_cfg_t cfg;
} ui_view_animation_t;

/****************************************************************************
 * Macros
 ****************************************************************************/

/**
 * @brief Statically define and initialize view entry for view.
 *
 * The view entry define statically,
 *
 * Each view must define the view entry info so that the system wants
 * to find view to knoe the corresponding information
 *
 * @param app_id Name of the app.
 * @param view_proc view proc function.
 * @param view_get_state view get state function.
 * @param view_key_map key map of view .
 * @param view_id view id of view .
 * @param default_order default order of view .
 * @param view_w view width .
 * @param view_h view height.
 */
#ifdef CONFIG_SIMULATOR
#  define VIEW_ENTRY_ATTR
#else
#  define VIEW_ENTRY_ATTR __attribute__((__section__(".view_entry")))
#endif

#define VIEW_DEFINE(app_name, view_proc, view_get_state, view_key_map,\
		view_id, order, view_type, view_w, view_h)	\
	const struct view_entry __view_entry_##app_name##view_id	\
		VIEW_ENTRY_ATTR = {	\
		.app_id = #app_name,							\
		.proc = view_proc,								\
		.id = view_id,									\
		.default_order = order,							\
		.type = view_type,								\
		.width = view_w,								\
		.height = view_h,								\
	}

#define VIEW_DEFINE2(app_name, view_proc, view_get_state, view_key_map,\
		view_id, order, view_type, view_w, view_h)	\
	const struct view_entry __view_entry_##app_name##view_id	\
		VIEW_ENTRY_ATTR = {	\
		.app_id = #app_name,							\
		.proc2 = view_proc,								\
		.id = view_id,									\
		.default_order = order,							\
		.type = view_type,								\
		.width = view_w,								\
		.height = view_h,								\
	}

#ifdef __cplusplus
#define EXTERN extern "C"
extern "C"
{
#else
#define EXTERN extern
#endif

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/**
 * @brief get view data
 *
 * This routine get view data.
 *
 * @param view_id id of view
 *
 * @return view data on success else NULL
 */
view_data_t *view_get_data(uint16_t view_id);

/**
 * @brief get view display
 *
 * This routine get view display.
 *
 * @param data view data
 *
 * @return view display on success else NULL
 */
static inline void *view_get_display(view_data_t *data)
{
	return data->display;
}

/**
 * @brief get view surface
 *
 * This routine get view gui surface.
 *
 * @param data view data
 *
 * @return view surface on success else NULL
 */
static inline void *view_get_surface(view_data_t *data)
{
	return data->surface;
}

/**
 * @brief get view data
 *
 * This routine get view data.
 *
 * @param view_id id of view
 *
 * @return view data on success else NULL
 */
static inline const void *view_get_presenter(view_data_t *data)
{
	return data->presenter;
}

/**
 * @brief set current draw refresh rate.
 *
 * @param view_id id of view
 * @param rate_hz refresh rate in Hz.
 *
 * @return the old refresh rate.
 */
uint8_t view_set_refresh_rate(uint16_t view_id, uint8_t rate_hz);

/**
 * @brief set current draw refresh rate.
 *
 * @param view_id id of view
 * @param timeout timeout in milliseconds. unused at present.
 *
 * @return 0 on success else negative code.
 */
int view_wait_for_refresh(uint16_t view_id, int timeout);

/**
 * @cond INTERNAL_HIDDEN
 */

/**
 * @brief enable view refresh to display or not
 *
 * This routine enable view refresh to display or not
 *
 * @param view_id id of view
 * @param enabled enable or not
 *
 * @retval 0 on succsess else negative errno code.
 */
int view_set_refresh_en(uint16_t view_id, bool enabled);

/**
 * @brief view refresh
 *
 * This routine provide view refresh
 *
 * @param view_id id of view
 *
 * @retval 0 on succsess else negative errno code.
 */
int view_refresh(uint16_t view_id);

/**
 * INTERNAL_HIDDEN @endcond
 */

/**
 * @brief query view is hidden
 *
 * @param view_id id of view
 *
 * @retval the query result
 */
bool view_is_hidden(uint16_t view_id);

/**
 * @brief query view is visible (even partially) now or not
 *
 * A view to be visible, must not be hidden, and also partially at least on the screen.
 *
 * @param view_id id of view
 *
 * @retval the query result
 */
bool view_is_visible(uint16_t view_id);

/**
 * @brief Query view is focused or not
 *
 * @param view_id id of view
 *
 * @return query result.
 */
bool view_is_focused(uint16_t view_id);

/**
 * @brief Query view is focused or not
 *
 * @param view_id id of view
 *
 * @return query result.
 */
bool view_is_paused(uint16_t view_id);

/**
 * @brief Query view is layout-inflated or not
 *
 * @param view_id id of view
 *
 * @return query result.
 */
bool view_is_inflated(uint16_t view_id);

/**
 * @brief Query view is scrollable or not
 *
 * @param view_id id of view
 *
 * @return query result.
 */
bool view_is_scrollable(uint16_t view_id);

/**
 * @brief Query view is scrolling or not
 *
 * @param view_id id of view
 *
 * @return query result.
 */
bool view_is_scrolling(uint16_t view_id);

/**
 * @brief get x coord of position of view
 *
 * @param view_id id of view
 *
 * @retval x coord
 */
int16_t view_get_x(uint16_t view_id);

/**
 * @brief get y coord of position of view
 *
 * @param view_id id of view
 *
 * @retval y coord
 */
int16_t view_get_y(uint16_t view_id);

/**
 * @brief get position of view
 *
 * @param view_id id of view
 * @param x pointer to store x coordinate
 * @param y pointer to store y coordinate
 *
 * @retval 0 on success else negative code
 */
int view_get_pos(uint16_t view_id, int16_t *x, int16_t *y);

/**
 * @brief get width of view
 *
 * @param view_id id of view
 *
 * @retval width on success else negative code
 */
int16_t view_get_width(uint16_t view_id);

/**
 * @brief get height of view
 *
 * @param view_id id of view
 *
 * @retval height on success else negative code
 */
int16_t view_get_height(uint16_t view_id);

/**
 * @brief get position of view
 *
 * @param view_id id of view
 * @param x pointer to store x coordinate
 * @param y pointer to store y coordinate
 *
 * @retval 0 on success else negative code
 */
int view_get_region(uint16_t view_id, ui_region_t *region);

/**
 * @brief set position of view
 *
 * It will stop the gesture scrolling before setting position.
 *
 * @param view_id id of view
 * @param x new x coordinate
 * @param y new y coordinate
 *
 * @retval 0 on success else negative code
 */
int view_set_pos(uint16_t view_id, int16_t x, int16_t y);

/**
 * @brief set position of view during gesture scrolling
 *
 * @param view_id id of view
 * @param x new x coordinate
 * @param y new y coordinate
 *
 * @retval 0 on success else negative code
 */
int view_set_drag_pos(uint16_t view_id, int16_t x, int16_t y);

/**
 * @brief set view drag attribute
 *
 * This routine set view drag attribute. If drag_attribute > 0, it implicitly make the view
 * visible, just like view_set_hidden(view_id, false) called.
 *
 * @param view_id id of view
 * @param drag_attribute drag attribute, see enum UI_VIEW_DRAG_ATTRIBUTE
 * @param keep_pos if true, keep the positon unchanged, else move the view to the drag position
 *
 * @retval 0 on succsess else negative errno code.
 */
int view_set_drag_attribute(uint16_t view_id, uint16_t drag_attribute, bool keep_pos);

/**
 * @brief get view drag attribute
 *
 * @param view_id id of view
 *
 * @retval drag attribute if view exist else negative code
 */
uint16_t view_get_drag_attribute(uint16_t view_id);

/**
 * @brief query view has move attribute (DRAG_ATTRIBUTE_MOVExx)
 *
 * @param view_id id of view
 *
 * @retval the query result
 */
bool view_has_move_attribute(uint16_t view_id);

/**
 * @brief set drag callback of view
 *
 * The callback will be called when the single dragged finished. During the callback,
 * the user can start the animation to some other position.
 *
 * @param view_id id of view
 * @param drag_cb callback for dragging one view, not involved in view switching
 *
 * @retval 0 on success else negative code
 */
int view_set_drag_anim_cb(uint16_t view_id, ui_view_drag_anim_cb_t drag_cb);

/**
 * @brief get diplay X resolution
 *
 * @retval X resolution in pixels
 */
int16_t view_manager_get_disp_xres(void);

/**
 * @brief get diplay Y resolution
 *
 * @retval Y resolution in pixels
 */
int16_t view_manager_get_disp_yres(void);

/**
 * @brief get diplay orientation
 *
 * The total display rotation is the panel physical rotation plus the user rotation.
 *
 * @retval rotation (CW) in degrees
 */
uint16_t view_manager_get_disp_rotation(void);

/**
 * @brief get dragged view
 *
 * This routine provide get dragged view.
 *
 * @param gesture gesture, see enum GESTURE_TYPE
 * @param towards_screen store the drag direction, whether dragged close to screen or away from screen.
 *
 * @retval view id.
 */
uint16_t view_manager_get_draggable_view(uint8_t gesture, bool *towards_screen);

/**
 * @brief get focused view
 *
 * This routine provide get focused view
 *
 * @retval view id of focused view
 */
uint16_t view_manager_get_focused_view(void);

/**
 * @brief dump the view informations to the console.
 *
 * @retval N/A.
 */
void view_manager_dump(void);

/**
 * @brief refocus view pre animation
 *
 * @param view_id the view that will be focused.
 *
 * @retval N/A
 */
void view_manager_pre_anim_refocus(uint16_t view_id);

/**
 * @brief refocus view post animation
 *
 * @retval N/A
 */
void view_manager_post_anim_refocus(void);

/**
 * @brief fill the start and stop points of the slide animation
 *
 * @param view_id the view to take the animation
 * @param cfg animation config
 * @param animation_type animation type, see  enum UI_VIEW_ANIMATION_TYPE
 *
 * @retval 0 on success else negative code.
 */
int view_manager_get_slide_animation_config(uint16_t view_id,
		ui_view_anim_cfg_t *cfg, uint8_t animation_type);

/**
 * @brief start the view animation on view switching
 *
 * @param view_id the view to take the animation
 * @param last_view_id the related view which will fade out if exists
 * @param animation_type animation type, see  enum UI_VIEW_ANIMATION_TYPE
 * @param cfg animation config
 *
 * @retval 0 on success else negative code.
 */
int view_manager_slide_animation_start(uint16_t view_id,
		uint16_t last_view_id, uint8_t animation_type, ui_view_anim_cfg_t *cfg);

/**
 * @brief fill the start and stop points of the drag animation
 *
 * @param view_id the view to take the animation
 * @param cfg animation config
 * @param runtime pointer to structure input_dev_runtime_t
 *
 * @retval 0 on success else negative code.
 */
int view_manager_get_drag_animation_config(uint16_t view_id,
		ui_view_anim_cfg_t *cfg, input_dev_runtime_t *runtime);

/**
 * @brief start the view animation caused on single long view dragging
 *
 * @param view_id the view to take the animation
 * @param cfg animation config
 *
 * @retval 0 on success else negative code.
 */
int view_manager_drag_animation_start(uint16_t view_id, ui_view_anim_cfg_t *cfg);

/**
 * @brief get current view id
 *
 * @param N/A
 *
 * @retval id id of current opertation view
 */
uint16_t view_manager_get_current_view_id(void);

/**
 * @brief get view entry
 *
 * @param view_id view id
 *
 * @retval pointer to view entry
 */
view_entry_t * view_manager_get_view_entry(uint16_t view_id);

/**
 * @brief check if view cache is scrolling
 *
 * @param N/A
 *
 * @retval 1: scrolling, 0: stasis
 */
bool view_manager_is_scrolling(void);

/**
 * @brief set callback
 *
 * This routine set callback
 *
 * @param id callback id
 * @param callback callback function
 *
 * @retval 0 on success else negative code.
 */
int view_manager_set_callback(uint8_t id, void * callback);

/**
 * @brief Acquire draw big lock
 *
 * Hold the mutex when accessing critical resource shared by GUI task and UI effects.
 *
 * @param timeout_ms timeout in milliseconds. Set OS_FOREVER to indicate forever
 *
 * @retval 0 on success else negative code.
 */
int view_manager_acquire_draw_lock(int timeout_ms);

/**
 * @brief Release draw big lock
 *
 * @retval 0 on success else negative code.
 */
int view_manager_release_draw_lock(void);

#undef EXTERN
#ifdef __cplusplus
}
#endif

/**
 * @} end defgroup system_apis
 */
#endif /* __VIEW_MANGER_H__ */
