/*
 * Copyright (c) 2018 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file app manager interface
 */

#ifndef __APP_MANGER_H__
#define __APP_MANGER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
/**
 * @brief system APIs
 * @defgroup system_apis system APIs
 * @{
 * @}
 */

/**
 * @defgroup app_manager_apis App manager APIs
 * @ingroup system_apis
 * @{
 */

/** @def APP_ID_ILLEGAL
 *
 *  @brief illegal app id
 *
 *  @details this can used as illegal app id
 */
#define APP_ID_ILLEGAL     NULL

/* app entry structure */
struct app_entry_t
{
	/** app name */
	char * name;
	/** pointer of app stack  */
	char * stack;
	/** sizeof of app stack  */
	uint16_t stack_size;
	/** priority of app */
	uint8_t priority;
	/** attribute of app */
	uint8_t attribute;
	/** parama1 of app */
	void * p1;
	/** parama2 of app */
	void * p2;
	/** parama3 of app */
	void * p3;
	/** main thread loop of app */
	void (*thread_loop)(void * parama1,void * parama2,void * parama3);
	/** input event handle */
	bool (*input_event_handle)(int key_event,int event_stage);
};

/**
 * @brief app manager init funcion
 *
 * This routine calls init app manager ,called by main
 *
 *
 * @return true if invoked succsess.
 * @return false if invoked failed.
 */

bool app_manager_init(void);

/**
 * @brief Statically define and initialize app entry for app.
 *
 * The App entry define statically,
 *
 * Each app must define the app entry info so that the system wants
 * to start app to knoe the corresponding information
 *
 * @param app_name Name of the app.
 * @param app_stack stack of app used.
 * @param app_stack_size stack size of app uded.
 * @param app_priority priority of app .
 * @param app_attribute attribue of app .
 * @param parama1 paramal of app .
 * @param parama2 parama2 of app .
 * @param parama3 parama3 of app .
 * @param fn main thread loop of app .
 */
#ifdef CONFIG_SIMULATOR
#define APP_DEFINE(app_name, app_stack,app_stack_size, app_priority,\
		app_attribute,parama1,parama2,parama3, fn, event_handle)
#else
#define APP_DEFINE(app_name, app_stack,app_stack_size, app_priority,\
		app_attribute,parama1,parama2,parama3, fn, event_handle)	\
        const struct app_entry_t __app_entry_##app_name	\
		__attribute__((__section__(".app_entry")))= {	\
		.name = #app_name,					\
		.stack = app_stack,					\
		.stack_size = app_stack_size,		\
		.priority = app_priority,		    \
		.attribute = app_attribute,			\
		.p1 = parama1,                      \
		.p2 = parama2,                      \
        .p3 = parama3,                      \
		.thread_loop = fn,                   \
		.input_event_handle = event_handle  \
	}
#endif
/**
 * @brief Get app tid accord app name
 *
 * This routine calls if app is not actived ,return NULL ,
 * if the target app is actived ,return the target app thread Id
 *
 * @param app_name name of target app.
 *
 * @return Thread ID of target thread.
 */

void * app_manager_get_apptid(char * app_name);

/**
 * @brief Make target App actived
 *
 * This routine calls if the target app is already actived , return true Direct
 * if the app not actived ,actived the target app,
 * if current actived app not the target app ,we first exit current actived app.
 *
 * @param app_name name of target app.
 *
 * @return true if invoked succsess.
 * @return false if invoked failed.
 */

bool app_manager_active_app(char * app_name);

/**
 * @brief return current actived app name
 *
 * This routine calls return current active app name
 *
 * @return NULL if no actived app current
 * @return void * current actived app name
 */

void* app_manager_get_current_app(void);

/**
 * @brief return previous actived app name
 *
 * This routine calls return previous active app name
 *
 * @return NULL if no actived app current
 * @return void * previous actived app name
 */

void* app_manager_get_prev_app(void);

/**
 * @brief app thread exit
 *
 * This routine called by app ,when app received exit message
 * we must call app thread exit to clear app info
 *
 * @param app_name name of target app.
 * @return true if invoked succsess.
 * @return false if invoked failed.
 */

bool app_manager_thread_exit(char * app_name);

/**
 * @brief Make target App exit
 *
 * This routine calls if the target app is actived , we exit this target app
 *
 *
 * @param app_name name of target app.
 * @param to_default marked Whether or not starts the default app directly after
 * exiting this application.
 *
 * @return true if invoked succsess.
 * @return false if invoked failed.
 */

bool app_manager_exit_app(char * app_name, bool to_default);

/**
 * @brief notify target app message
 *
 * This routine calls to nodify target app message
 *
 * @param app_name target all ,if app name is null, notify all app
 * @param msg_type notify message type
 *
 * @return true if invoked succsess.
 * @return false if invoked failed.
 */

void app_manager_notify_app(char * app_name, int msg_type);

/**
 * INTERNAL_HIDDEN @endcond
 */
/**
 * @} end defgroup app_manager_apis
 */

#ifdef __cplusplus
}
#endif
#endif
