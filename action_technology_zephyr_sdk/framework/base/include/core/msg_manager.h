/*
 * Copyright (c) 2018 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file massage manager interface
 */

#ifndef __MSG_MANAGER_H__
#define __MSG_MANAGER_H__

#include <os_common_api.h>
#include <sys/slist.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MSG_CONTENT_SIZE 4

typedef enum
{
	MSG_NULL = 0,

	MSG_START_APP ,
	MSG_SWITCH_APP,
	MSG_INIT_APP,
	MSG_EXIT_APP,
	MSG_SUSPEND_APP,
	MSG_RESUME_APP,

	MSG_KEY_INPUT,
	MSG_INPUT_EVENT,
	MSG_UI_EVENT,
	MSG_TTS_EVENT,
	MSG_HOTPLUG_EVENT,
	MSG_VOLUME_CHANGED_EVENT,

	MSG_POWER_OFF,
	MSG_STANDBY,
	MSG_LOW_POWER,
	MSG_NO_POWER,
	MSG_REBOOT,

    /* sync message no replay */
	MSG_REPLY_NO_REPLY,

	/* message process success */
	MSG_REPLY_SUCCESS,

	/* message process failed */
	MSG_REPLY_FAILED,

	MSG_BAT_CHARGE_EVENT,
	MSG_SR_INPUT,
	MSG_FACTORY_DEFAULT,
	MSG_START_RECONNECT,
	MSG_ENTER_PAIRING_MODE,
	MSG_WIFI_DISCONNECT,
	MSG_RETURN_TO_CALLER,

	MSG_BT_EVENT,
	MSG_BT_ENGINE_READY, //ep-mode ignore MSG_BT_ENGINE_READY
	MSG_BT_MGR_EVENT,
	MSG_TWS_EVENT,

	MSG_SHUT_DOWN,

	MSG_BT_REMOTE_VOL_SYNC,

	MSG_SYS_EVENT,

	MSG_SENSOR_EVENT,

	MSG_BLE_ANCS_AMS_SERVICE,

    MSG_APP_BLE_SUPER_BQB,
	MSG_APP_TOOL_INIT,

	MSG_EARLY_SUSPEND_APP,
	MSG_LATE_RESUME_APP,

    MSG_BT_PAIRING_EVENT,

    MSG_BT_DEVICE_ERROR,

	MSG_BLE_OTA_MASTER_EVENT,

	/* service defined message */
	MSG_SRV_MESSAGE_START           = 128,

	/* application defined message */
	MSG_APP_MESSAGE_START           = 200,
} msg_type;


struct msg_listener
{
	sys_snode_t node;
	char * name;
	os_tid_t tid;
#ifdef CONFIG_TASK_WDT
	int channel_id;
#endif
};

#define LISTENER_INFO(_node) CONTAINER_OF(_node, struct msg_listener, node)



/**
 * @defgroup msg_manager_apis Message Manager APIs
 * @ingroup system_apis
 * @{
 */

struct app_msg;

typedef void (*MSG_CALLBAK)(struct app_msg*, int, void*);

#define send_async_msg(receiver, msg) \
			msg_manager_send_async_msg(receiver, msg)

#define receive_msg(msg, timeout) \
			msg_manager_receive_msg(msg, timeout)

#define poll_msg(msg, sem, timeout) \
				msg_manager_poll_msg(msg, sem, timeout)

/** message structure */
/** @brief app message structure.
 *  @param sender who send this message
 *  @param type message type ,keyword of message
 *  @param cmd message cmd , used to send opration type
 *  @param reserve  resrved byte, user can used this byte set info
 *  @param valuuser data , to transmission some user info
 *  @param callback callback of message process finished.
 *  @param sync_sem sem for sync.
 */
struct app_msg
{
	uint16_t sender;
	uint8_t type;
	uint8_t cmd;
	uint8_t reserve;
	uint8_t __pad[3];
	union {
		char content[MSG_CONTENT_SIZE];
		short short_content[MSG_CONTENT_SIZE / 2];
		int value;
		void *ptr;
	};
	MSG_CALLBAK callback;
	os_sem *sync_sem;
};

/**
 * @brief msg_manager_init
 *
 * This routine msg_manager_init
 *
 * @return true  init success
 * @return false init failed
 */

bool msg_manager_init(void);

/**
 * @brief Send a Asynchronous message
 *
 * This routine Send a Asynchronous message
 *
 * @param receiver name of message receiver
 * @param msg store the received message
 *
 * @return true send success
 * @return false send failed
 */
bool msg_manager_send_async_msg(char * receiver , struct app_msg *msg);

/**
 * @brief receive message
 *
 * This routine receive msg from others
 *
 * @param msg store the received message
 * @param timeout time out of receive message
 *
 * @return NULL The listener is not in the message linsener list
 * @return tid  target thread id of the message linsener
 */
bool msg_manager_receive_msg(struct app_msg *msg, int timeout);

/**
 * @brief poll message
 *
 * This routine poll msg from others
 *
 * @param msg store the received message
 * @param sem semaphore to poll
 * @param timeout time out of poll message
 *
 * @return OS_POLL_NONE  timeout
 * @return OS_POLL_MSG   msg ready
 * @return OS_POLL_SEM   sem ready
 */
int msg_manager_poll_msg(struct app_msg *msg, os_sem *sem, int timeout);

/**
 * @brief get free async msg num
 *
 * Asynchronous message has a number of restrictions, when sending an asynchronous message,
 * the recipient did not receive before, asynchronous message occupies a memory,
 * only when the recipient copy will be released after the release of the memory,
 * the current support of up to 10 asynchronous messages,
 * This routine Returns the number of asynchronous free asynchronous messages
 *
 * @return NUM  num of async msg num
 */
int msg_manager_get_free_msg_num(void);

/**
 * @brief drop all message
 *
 * This routine drop all message in mail box
 *
 */

void msg_manager_drop_all_msg(void);

/**
 * @brief check have new message
 *
 * This routine check have new in mail box
 *
 * @return number of new message in mailbox
 */
int msg_manager_get_pending_msg_cnt(void);

/**
 * @brief dump busy message info
 *
 * This routine dump busy message info
 *
 * @return N/A
 */
void msg_manager_dump_busy_msg(void);

/**
 * @cond INTERNAL_HIDDEN
 */
/**
 * @brief Send a synchronization message
 *
 * This routine Send a synchronization message
 *
 * @param receiver name of message receiver
 * @param msg store the received message
 *
 * @return true send success
 * @return false send failed
 */
#define send_sync_msg(receiver, msg) \
			msg_manager_receive_msg(receiver, msg)
bool msg_manager_send_sync_msg(char *receiver , struct app_msg *msg);

/**
 * @brief Send a synchronization message immediately and discardable
 *
 * This routine Send a periodic aynchronization message immediately,
 * If the first message is not processed by the recipient, the user will
 * send the second message, the second message will be discarded at this time.
 * Keep only one such message in the queue
 *
 * @param receiver name of message receiver
 * @param msg store the received message
 *
 * @return true send success
 * @return false send failed
 */
#define send_async_msg_discardable(receiver, msg) \
			msg_manager_send_async_msg_discardable(receiver, msg)

bool msg_manager_send_async_msg_discardable(char *receiver, struct app_msg *msg);
/**
 * @brief add message listener
 *
 * This routine add target thread to messsage lisener list
 * all app or service who want to receive message from others, must
 * add themslef to message listener
 *
 * @param name target thread name ,witch marked thread.
 * @param tid target thread id.
 *
 * @return true execution succeed
 * @return false execution failed
 */
bool msg_manager_add_listener(char * name , os_tid_t tid);

/**
 * @brief remove listener from message listener list
 *
 * This routine remove target thread from messsage lisener list
 * all app or service must removed from message listener list
 * before is will exit.
 *
 * @param name target thread name ,witch marked thread.
 *
 * @return true execution succeed
 * @return false execution failed
 */

bool msg_manager_remove_listener(char * name);

/**
 * @brief get the thread id from message listener list by name
 *
 * This routine remove target thread from messsage lisener list
 * all app or service must removed from message listener list
 * before is will exit.
 *
 * @param name target thread name ,witch marked thread.
 *
 * @return NULL The listener is not in the message linsener list
 * @return tid  target thread id of the message linsener
 */

os_tid_t  msg_manager_listener_tid(char * name);
/**
 * @brief get receiver name by tid
 *
 * This routine get receiver name by tid
 *
 * @param tid thread id
 *
 * @return name of receiver for target tid
 */
char* msg_manager_get_name_by_tid(int tid);

/**
 * @brief get current sender name
 *
 * This routine get current sender name
 *
 * @return name of sender
 */
char* msg_manager_get_current(void);

/**
 * @brief lock msg manager
 *
 * This routine lock msg manager
 *
 * @return N/A
 */
void msg_manager_lock(void);

/**
 * @brief unlock msg manager
 *
 * This routine gunlock msg manager
 *
 * @return N/A
 */

void msg_manager_unlock(void);

/**
 * @brief get the current channel id for task wdt
 *
 * This routine get current channel id for task wdt.
 *
 * @return -1 current task wdt is not enable
 * @return channel_id  current task wdt cahnnel id
 */
int msg_manager_get_current_channel_id(void);

/**
 * INTERNAL_HIDDEN @endcond
 */

/**
 * @} end defgroup msg_manager_apis
 */

#ifdef __cplusplus
}
#endif

#endif
