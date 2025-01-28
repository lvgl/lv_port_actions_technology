/*
 * Copyright (c) 2018 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file input manager interface
 */

#ifndef __INPUT_MANGER_H__
#define __INPUT_MANGER_H__

#include <stdbool.h>
#include <input_manager_type.h>
/**
 * @defgroup input_manager_apis App Input Manager APIs
 * @ingroup system_apis
 * @{
 */

/*
 * Input device types
 */
#define INPUT_DEV_TYPE_KEYBOARD		1
#define INPUT_DEV_TYPE_TOUCHPAD		2

/** key action type */
enum GESTURE_TYPE
{
	/** gesture drop down */
	GESTURE_DROP_DOWN  = 1,
	/** gesture drop up */
	GESTURE_DROP_UP,
	/** gesture drop left */
	GESTURE_DROP_LEFT,
	/** gesture drop right */
	GESTURE_DROP_RIGHT,
};

#define GESTURE_HOR_BITFIELD ((0x1 << GESTURE_DROP_LEFT) | (0x1 << GESTURE_DROP_RIGHT))
#define GESTURE_VER_BITFIELD ((0x1 << GESTURE_DROP_DOWN) | (0x1 << GESTURE_DROP_UP))
#define GESTURE_ALL_BITFIELD (GESTURE_HOR_BITFIELD | GESTURE_VER_BITFIELD)

/** key action type */
enum KEY_VALUE
{
	/** key press down */
    KEY_VALUE_DOWN      = 1,
	/** key press release */
	KEY_VALUE_UP       = 0,
};

/** key event STAGE */
enum KEY_EVENT_STAGE
{
	/** key press pre process */
    KEY_EVENT_PRE_DONE   = 0,
	/** key press event done */
	KEY_EVENT_DONE       = 1,
};

/** States for input devices*/
enum {
	INPUT_DEV_STATE_REL = 0,
	INPUT_DEV_STATE_PR,
};

typedef uint8_t input_dev_state_t;

/** Possible input device types*/
enum {
	INPUT_DEV_TYPE_NONE,    /**< Uninitialized state*/
	INPUT_DEV_TYPE_POINTER, /**< Touch pad, mouse, external button*/
	INPUT_DEV_TYPE_KEYPAD,  /**< Keypad or keyboard*/
	INPUT_DEV_TYPE_BUTTON,  /**< External (hardware button) which is assigned to a specific point of the
	                          screen*/
	INPUT_DEV_TYPE_ENCODER, /**< Encoder with only Left, Right turn and a Button*/
};

typedef uint8_t input_dev_type_t;

/**
 * Represents a point on the screen.
 */
typedef struct {
	int16_t x;
	int16_t y;
} point_t;

/** Data structure passed to an input driver to fill */
typedef struct _input_dev_data_t {
	union {
		struct {          /**< For INPUT_DEV_TYPE_POINTER the currently pressed point*/
			point_t point;
			int8_t gesture_dir; /* gesture direction */
			int8_t scroll_dir;  /* scroll direction */
		};

		uint32_t key;     /**< For INPUT_DEV_TYPE_KEYPAD the currently pressed key*/
		uint32_t btn_id;  /**< For INPUT_DEV_TYPE_BUTTON the currently pressed button*/
		int16_t enc_diff; /**< For INPUT_DEV_TYPE_ENCODER number of steps since the previous read*/
	};

	input_dev_state_t state; /**< INPUT_DEV_STATE_REL or INPUT_DEV_STATE_PR*/
} input_dev_data_t;

/** Initialized by the user and registered by 'lv_indev_add()'*/
typedef struct _input_drv_t {
	/**< Input device type*/
	input_dev_type_t type;
	struct device *input_dev;

	/**< Function pointer to read input device data.
	 * Return 'true' if there is more data to be read (buffered).
	 * Most drivers can safely return 'false' */
	bool (*read_cb)(struct _input_drv_t * indev_drv, input_dev_data_t * data);

	/** Called when an action happened on the input device.
	 * The second parameter is the event from `lv_event_t`*/
	void (*feedback_cb)(struct _input_drv_t *, uint8_t);

	void (*enable)(struct _input_drv_t * indev_drv, bool enable);

#ifdef CONFIG_INPUT_DEV_ACTS_KNOB
	bool (*filter_cb)(void *val);
#endif
	/**< Number of pixels to slide before actually drag the object*/
	uint8_t scroll_limit;

	/**< Drag throw slow-down in [%]. Greater value means faster slow-down */
	uint8_t scroll_throw;

	/**< At least this difference should between two points to evaluate as gesture*/
	uint8_t gesture_min_velocity;

	/**< At least this difference should be to send a gesture*/
	uint8_t gesture_limit;

	/**< Long press time in milliseconds*/
	uint16_t long_press_time;

	/**< Repeated trigger period in long press [ms] */
	uint16_t long_press_rep_time;
} input_drv_t;

/** Run time data of input devices
 * Internally used by the library, you should not need to touch it.
 */
typedef struct _input_dev_runtime_t {
	input_dev_state_t state; /**< Current state of the input device. */
	int8_t scroll_dir; /* scroll direction */
	int8_t last_scroll_dir; /* last scroll direction */

	uint16_t pre_view_id;
	uint16_t current_view_id;
	uint16_t last_view_id;
	uint16_t view_id;
	uint16_t related_view_id;

	/*Flags*/
	uint8_t long_pr_sent : 1;
	uint8_t reset_query : 1;
	uint8_t disabled : 1;
	uint8_t wait_until_release : 1;

	union {
		struct { /*Pointer and button data*/
			point_t act_point; /**< Current point of input device. */
			point_t last_point; /**< Last point of input device. */
			point_t vect; /**< Difference between `act_point` and `last_point`. */
			point_t scroll_start; /**< Last point of input device. */
			point_t scroll_sum; /*Count the dragged pixels to check scroll_limit*/
			point_t scroll_throw_vect;
			int16_t last_scroll_off;

			point_t gesture_sum; /*Count the gesture pixels to check gesture_limit*/
			/*Flags*/
			uint8_t scroll_in_prog : 1;
			uint8_t scroll_to_screen : 1;
			uint8_t scroll_disabled : 1; /* disable gesture scrolling */
			uint8_t scroll_wait_until_release : 1; /* do nothing until next release */
			uint8_t scroll_outscreen_en : 1; /* enable to scroll out of screen then snap back to edge */
			uint8_t gesture_sent : 1; /* gesture event reported or not */
			int8_t gesture_dir; /* gesture direction */
		} pointer;

		struct { /*Keypad data*/
			input_dev_state_t last_state;
			uint32_t last_key;
		} keypad;
	} types;

	uint32_t pr_timestamp;         /**< Pressed time stamp*/
	uint32_t longpr_rep_timestamp; /**< Long press repeat time stamp*/
} input_dev_runtime_t;

/** The main input device descriptor with driver, runtime data ('proc') and some additional
 * information*/
typedef struct _input_dev_t {
	/**< Input used flag*/
	uint8_t used_flag:1;
	input_drv_t driver;
	input_dev_runtime_t proc;
} input_dev_t;

typedef struct input_dev_init_param {
	/**< Input init param*/
	uint16_t long_press_timer;
	uint16_t super_long_press_timer;
	uint16_t super_long_press_6s_timer;

	uint16_t quickly_click_duration;
	uint16_t key_event_cancel_duration;
	uint16_t hold_delay_time;
	uint16_t hold_interval_time;
} input_dev_init_param;

typedef void (*event_trigger)(uint32_t key_value, uint16_t type);

void input_manager_dev_enable(input_dev_t *indev);
input_dev_t *input_manager_get_input_dev(input_dev_type_t dev_type);

/**
 * Read data from an input device.
 * @param indev pointer to an input device
 * @param data input device will write its data here
 * @return false: no more data; true: there more data to read (buffered)
 */

bool input_dev_read(input_dev_t * indev, input_dev_data_t * data);

/**
 * @brief input manager init funcion
 *
 * This routine calls init input manager ,called by main
 *
 * @param event_cb when keyevent report ,this callback called before
 * key event dispatch.
 *
 * @return true if invoked succsess.
 * @return false if invoked failed.
 */

bool input_manager_init(event_trigger event_cb);

/**
 * @brief Lock the input event
 *
 * This routine calls lock input event, input event may not send to system
 * if this function called.
 *
 *
 * @return true if invoked succsess.
 * @return false if invoked failed.
 */

bool input_manager_lock(void);

/**
 * @brief unLock the input event
 *
 * This routine calls unlock input event, input event may send to system
 * if this function called.
 *
 * @note input_manager_lock and input_manager_unlock Must match
 *
 * @return true if invoked succsess.
 * @return false if invoked failed.
 */

bool input_manager_unlock(void);
/**
 * @brief get the status of input event lock
 *
 * @return true if input event is locked
 * @return false if input event is not locked.
 */
bool input_manager_islock(void);

/**
 * @brief set filter key event
 *
 * @return true set success
 * @return false set failed.
 */
bool input_manager_filter_key_itself(void);



int input_keypad_onoff_inquiry_enable(bool inner_onoff, bool enable);

int input_keypad_onoff_inquiry(bool inner_onoff, int keycode);

void input_manager_set_init_param(input_dev_init_param *data);

void input_manager_boost(bool boost);

#ifdef CONFIG_INPUT_DEV_ACTS_KNOB
void input_pointer_register_filter_cb(bool (*filter_cb)(void *val));
#endif

/**
 * @} end defgroup input_manager_apis
 */
#endif