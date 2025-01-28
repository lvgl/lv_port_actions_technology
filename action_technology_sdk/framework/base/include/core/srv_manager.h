/*
 * Copyright (c) 2018 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __SRV_MANAGER_H__
#define __SRV_MANAGER_H__


#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief service manager api
 * @defgroup srv_manager_apis Service Manager APIs
 * @ingroup system_apis
 * @{
 */
#define BLUETOOTH_SERVICE_NAME         "bluetooth"

/** @def MEDIA_SERVICE_NAME
 *
 *  @brief name of media service
 *
 *  @details this can used as media service id
 */
#define MEDIA_SERVICE_NAME             "media"

/** @def UI_SERVICE_NAME
 *
 *  @brief name of UI service
 *
 *  @details this can used as UI service id
 */
#define UI_SERVICE_NAME             "ui_service"

/** @def SENSOR_SERVICE_NAME
 *
 *  @brief name of Sensor service
 *
 *  @details this can used as Sensor service id
 */
#define SENSOR_SERVICE_NAME         "sensor_service"

/** @def ALL_RECEIVER_NAME
 *
 *  @brief name of all service and app
 *
 *  @details this used send boardcast message

 */
#define ALL_RECEIVER_NAME    "all"

/** @def APP_PRIORITY
 *
 *  @brief define the app thread priority
 *
 *  @details all app can used the APP_PRIORITY as default priority
 */
#define APP_PRIORITY 10

/** @def SRV_PRIORITY
 *
 *  @brief define the service thread priority
 *
 *  @details all service can used the SRV_PRIORITY as default priority
 */
#define SRV_PRIORITY 11

/* app or service attribute options */
enum attribute_type
{
	/** attribute Type of resident app or service . */
    RESIDENT_APP   = 1,
	/** attribute Type of forground app or service . */
	FOREGROUND_APP = 2,
	/** attribute Type of background app or service . */
	BACKGROUND_APP = 3,
	/** attribute Type mask */
	APP_ATRTRIBUTE_MASK = 0x0f,
	/** attribute Type default app mark */
	DEFAULT_APP    = 0x10,
};

/* app entry structure */
struct service_entry_t
{
	/** service name */
	char * name;
	/** pointer of service stack  */
	char * stack;
	/** sizeof of service stack  */
	uint16_t stack_size;
	/** priority of service  */
	uint8_t priority;
	/** attribute of service */
	uint8_t attribute;
	/** parama1 of service */
	void * p1;
	/** parama1 of service */
	void * p2;
	/** parama1 of service */
	void * p3;
	/** main thread loop of service */
	void (*thread_loop)(void * parama1,void * parama2,void * parama3);
};

/**
 * @brief service manager init funcion
 *
 * This routine calls init service manager ,called by main
 *
 *
 * @return true if invoked succsess.
 * @return false if invoked failed.
 */

bool srv_manager_init(void);

/**
 * @brief Statically define and initialize service entry for service.
 *
 * The service entry define statically.
 *
 * Each app must define the app entry info so that the system wants
 * to start app to knoe the corresponding information
 *
 * @param service_name Name of the service.
 * @param service_stack stack of service used.
 * @param service_stack_size stack size of service used.
 * @param service_priority priority of service .
 * @param service_attribute attribue of service .
 * @param parama1 paramal of service .
 * @param parama2 parama2 of service .
 * @param parama3 parama3 of service .
 * @param fn main thread loop of service .
 */
#define SERVICE_DEFINE(service_name, service_stack,service_stack_size, \
		service_priority,service_attribute,parama1,parama2,parama3, fn)	\
        const struct service_entry_t __service_entry_##service_name	\
		__attribute__((__section__(".service_entry")))= {	\
		.name = #service_name,					\
		.stack = service_stack,					\
		.stack_size = service_stack_size,		\
		.priority = service_priority,		    \
		.attribute = service_attribute,			\
		.p1 = parama1,                      \
		.p2 = parama2,                      \
        .p3 = parama3,                      \
		.thread_loop = fn                   \
	}

/**
 * @brief Make target service actived
 *
 * This routine calls if the target service is already actived , return true Direct
 * if the service not actived ,actived the target service,
 * if current actived service not the target service ,we first exit current actived service.
 *
 * @param srv_name name of target service.
 *
 * @return true if invoked succsess.
 * @return false if invoked failed.
 */

bool srv_manager_active_service(char * srv_name);

/**
 * @brief Make target service exit
 *
 * This routine calls if the target service is actived , we exit this target service
 *
 *
 * @param srv_name name of target service.
 *
 * @return true if invoked succsess.
 * @return false if invoked failed.
 */

bool srv_manager_exit_service(char * srv_name);

/**
 * @brief service thread exit
 *
 * This routine called by app ,when app received exit message
 * we must call app thread exit to clear app info
 *
 * @param srv_name name of target service .
 * @return true if invoked succsess.
 * @return false if invoked failed.
 */

bool srv_manager_thread_exit(char * srv_name);

/**
 * @brief check service is actived
 *
 * This routine give the states of target service
 *
 * @param srv_name name of target service .
 * @return true if the target service actived
 * @return false if the target service not actived
 */

bool srv_manager_check_service_is_actived(char * srv_name);

/**
 * @brief notify all service message
 *
 * This routine calls to nodify all service message
 *
 * @param srv_name target all ,if srv name is null, notify all srv
 * @param msg_type notify message type
 *
 * @return N/A
 */

void srv_manager_notify_service(char *srv_name, int msg_type);

/**
 * @brief get service tid
 *
 * This routine calls to service tid
 *
 * @param srv_name target all ,if srv name is null, notify all srv
 *
 * @return service tid
 */
void *srv_manager_get_servicetid(char *srv_name);

/**
 * @} end defgroup srv_manager_apis
 */


#ifdef __cplusplus
}
#endif

#endif
