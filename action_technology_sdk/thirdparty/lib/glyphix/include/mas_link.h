/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#ifndef __MAS_LINK_H__
#define __MAS_LINK_H__

// #ifdef __cplusplus
// extern "C" {
// #endif

#include "gx_cdef.h"
#include "gx_mutex.h"
#include "mas_proto.h"

/* whether the device is 'up' (set by mas_link_register()) */
#define MAS_LINK_STATUS_UP (0x01L << 0)
/* if set, the link device has an active link (set by the link device driver) */
#define MAS_LINK_STATUS_LINK_UP (0x01L << 1)
/* if set, the link device has received data and has not read by transport layer (set by the link
 * device driver) */
#define MAS_LINK_STATUS_RECV (0x01L << 2)

#define mas_link_device_is_up(link)                                                                \
    (((link)->status & MAS_LINK_STATUS_UP) ? (uint8_t)1 : (uint8_t)0)
#define mas_link_device_is_link_up(link)                                                           \
    (((link)->status & MAS_LINK_STATUS_LINK_UP) ? (uint8_t)1 : (uint8_t)0)

#ifndef MCF_LINK_PORT_MAX
#define MAS_LINK_PORT_MAX 8
#endif

/* MAS link communicate protocol type */
enum mas_link_type {
    MAS_LINK_TYPE_SOCKET = 0x01,
    MAS_LINK_TYPE_UART,
    MAS_LINK_TYPE_SPI,
    MAS_LINK_TYPE_CAN,
    MAS_LINK_TYPE_SMEM,
    MAS_LINK_TYPE_USER,
    MAS_LINK_TYPE_USB,
};

struct mas_arp {
    uint8_t port_id;
    uint8_t dev_id;
    long tick;
    gx_slist_t list;
};

/* MAS link frame */
struct mas_frame {
    uint8_t type;  /* packet type */
    void *payload; /* packet payload */
};

struct mas_link;

/* MAS link communicate protocol operations */
struct mas_link_ops {
    mas_err_t (*init)(struct mas_link *link);
    mas_err_t (*send)(struct mas_link *link, struct mas_pbuf *pbuf, int32_t timeout);
    mas_err_t (*recv)(struct mas_link *link, struct mas_pbuf *pbuf, int32_t timeout);
    mas_err_t (*control)(struct mas_link *link, int cmd, void *arg);
};

enum mas_link_status_cb_type {
    MAS_LINK_STATUS_CB_TYPE_LINK_UP,
    /* changed to 'link up' */
    MAS_LINK_STATUS_CB_TYPE_LINK_DOWN,
    /* changed to 'link down' */
};

/* function prototype for link device status or address change callback functions */
typedef void (*mas_link_status_callback_fn)(struct mas_link *link,
                                            enum mas_link_status_cb_type type);

/* MAS link communicate protocol  */
struct mas_link {
    uint8_t id;                     /* link port ID */
    uint8_t type;                   /* link port type */
    uint16_t mtu;                   /* link MTU */
    const struct mas_link_ops *ops; /* link operations */

    uint8_t *recv_buf;         /* receive buffer */
    size_t recv_bufsz;         /* maximum receive buffer size */
    size_t cur_recv_bufsz;     /* current receive buffer size */
    size_t frames_len;         /* received frame buffer length */
    gx::os::mutex_t recv_lock; /* receive data lock */
    gx::os::mutex_t send_lock; /* send data lock */
    size_t notice_num;         /* send data notice number */
    uint16_t status;           /* device status */
    mas_link_status_callback_fn status_callback;
    /* network interface device flags change callback */

    gx_slist_t list;      /* link list */
    gx_slist_t recv_list; /* link receive data list */
    void *user_data;      /* user-specific data */
};

/* MAS link event operations */
mas_err_t mas_link_notice_recv(struct mas_link **link, int32_t timeout);
mas_err_t mas_link_notice_send(struct mas_link *link);
void mas_link_notice_quit();

mas_err_t mas_link_recv(struct mas_link *link, struct mas_pbuf *pbuf);
mas_err_t mas_link_send(struct mas_link *link, struct mas_pbuf *pbuf, int32_t timeout);

/* MAS link object get */
struct mas_link *mas_link_get_by_pid(uint8_t port_id);
struct mas_link *mas_link_get_by_did(uint8_t dev_id);
struct mas_link *mas_link_get_by_name(const char *device_name);

/* MAS link object register */
int mas_link_register(struct mas_link *link, uint8_t port_id, uint8_t type, uint16_t mtu,
                      void *user_data);

/* MAS link list get */
gx_slist_t mas_link_list_get(void);
struct mas_link *mas_link_get(void);
gx_slist_t mas_link_arp_list_get(void);
uint8_t mas_link_frame_id_add(void);

/* MAS link status callback */
void mas_link_set_status_callback(struct mas_link *link,
                                  mas_link_status_callback_fn status_callback);
void mas_low_level_set_link_status(bool is_up);


// #ifdef  __cplusplus
// }
// #endif

#endif /* __MAS_LINK_H__ */
