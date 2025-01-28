/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file system event interface
 */

#ifndef _SYS_EVENT_H_
#define _SYS_EVENT_H_
#include <os_common_api.h>
#include <sys_event_type.h>

/**
 * @defgroup sys_event_apis App system event Manager APIs
 * @ingroup system_apis
 * @{
 */

/** sys event ui map structure */
struct sys_event_ui_map {
	/** sys event id */
    uint8_t  sys_event;
	/** ui event id */
    uint8_t  ui_event;
	/** ui event type */
    uint8_t  stable_event;
};

/**
 * @brief send system message
 *
 * @details send system message to system app, if tws
 * master mode send system message to slave too.
 *
 * @param message message id want to send
 *
 * @return N/A
 */
void sys_event_send_message_new(uint32_t message, uint8_t cmd, void* extra_data, int extra_data_size);

#define sys_event_send_message(message) sys_event_send_message_new(message, 0, NULL, 0)



/**
 * @brief report inut event
 *
 * @details report system event to system app.
 *
 * @param key_event input event
 *
 * @return N/A
 */
void sys_event_report_input(uint32_t key_event);
/**
 * @brief system event notify
 *
 * @details send system event to system app, if tws
 * master mode send system event to slave too.
 *
 * @param event event id want to send
 *
 * @return N/A
 */
void sys_event_notify(uint32_t event);

/**
 * @brief process system event
 *
 * @details this routine Find UI event through sys event ID according to
 *  sys event UI map table and send UI event  .
 *
 * @param event system event id
 *
 * @return N/A
 */
void sys_event_process(uint32_t event);

/**
 * @brief register a system event map
 *
 * @details register system event map, user can define system event an ui event map
 *  when device sent system event ,may auto trigge ui event which mapped by event map 
 *
 * @param event_map user define event map table
 * @param size event map table size
 * @param sys_view_id  system view id
 *
 * @return N/A
 */

void sys_event_map_register(const struct sys_event_ui_map *event_map, int size, int sys_view_id);
/**
 * @} end defgroup sys_event_apis
 */
#endif

