/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#ifndef __MAS_H__
#define __MAS_H__

#include "gx_threadpool.h"
#include "mas_utils.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PKG_USING_MAS
// #define MAS_PKT_MAX_SIZE 1024
#define MAS_USING_ACK
#define MAS_ACK_TIMOUT 500
#define MAS_USING_REQ_EXEC
#define MAS_REQ_THREAD_POOL_SIZE       2
#define MAS_REQ_THREAD_POOL_STACK_SIZE 2048
#define MAS_USING_LINK_UART
#define PKG_USING_MAS_LATEST_VERSION
#define PKG_USING_URPC
#define URPC_USING_CJSON

#define MAS_SW_VER     "0.1.0"
#define MAS_SW_VER_NUM 0x000100

/* MAS device ID type */
#define MAS_DEV_ID_RECEIVER  254
#define MAS_DEV_ID_BROADCAST 255

/* MAS packet flag */
#define MAS_PKT_FLAG_NEED_RSP 0x01
#define MAS_PKT_FLAG_NEED_ACK 0x02

/* MAS packet type */
#define MAS_PKT_TYPE_REQ 0x00
#define MAS_PKT_TYPE_RSP 0x01

/* MAS link control commands */
#define MAS_LINK_CTRL_NEED_CRC16 0x01

/* 6 bytes frame header, frame header(1byte) + frame len(2bytes) + frame ID (1byte)  +
 * reserve(1byte) + frame type(1byte) */
#define MAS_UART_FRAME_HEAD_LEN 6
/* D2D package header length */
#define MAS_D2D_PKT_HEAD_LEN 4

#define MAS_UART_PKT_HEAD_LEN                                                                      \
    (MAS_UART_FRAME_HEAD_LEN + MAS_UART_FRAME_TAIL_LEN + MAS_D2D_PKT_HEAD_LEN)

#ifndef MAS_PKT_MAX_SIZE
#define MAS_PKT_MAX_SIZE 20480
#endif

extern size_t MAS_LINK_MTU;
extern bool MAS_LINK_USING_CRC;

struct mas_pkt;

/* MAS request execute feature callback */
typedef int (*mas_req_exec_fn)(struct mas_pkt *in_pkt, struct mas_pkt *out_pkt);

enum mas_link_device_type {
    MAS_LINK_CHAR,
    MAS_LINK_USB,
    MAS_LINK_SOCKET,
};

/* MAS error code */
enum mas_err_code {
    MAS_OK = 0,
    MAS_FAILD,
    MAS_MEM_FULL,
    /* memory full */
    MAS_SEND_TIMEOUT,
    /* send data timeout */
    MAS_SEND_RETRY_ERR,
    /* send data retry error */
    MAS_RECV_TIMEOUT,
    /* receive data timeout */
    MAS_RECV_BUF_FULL,
    /* receive buffer full */
};

typedef int mas_err_t;

/* MAS packet information for receive packet */
struct mas_pkt_info {
    uint8_t dst_id;       /* destination device ID */
    uint8_t src_id;       /* source device ID */
    uint8_t pkt_id;       /* packet ID */
    uint8_t pkt_type : 2; /* packet type */
    uint8_t need_ack : 1; /* packet need ACK */
    uint8_t need_rsp : 1; /* packet need response */
};

/* MAS packet */
struct mas_pkt {
    struct mas_pbuf pbuf;     /* MAS packet buffer */
    struct mas_pkt_info info; /* MAS packet information */
};

/* MAS initialize and start */
int mas_proto_init(uint8_t id);
void mas_proto_proc();
void mas_proto_set_req_pool(gx::ThreadPool *reqpool);

/* MAS D2D transport protocol operations */
int mas_d2d_open(uint8_t dst_id);
int mas_d2d_send(int sid, struct mas_pkt *pkt, int32_t timeout);
int mas_d2d_recv(int sid, struct mas_pkt *pkt, int32_t timeout);
int mas_d2d_close(int sid);

/* MAS register request execute callback function */
int mas_req_exec_fn_register(mas_req_exec_fn req_fn);

/* MAS packet operations */
struct mas_pkt *mas_pkt_make(struct mas_pkt *pkt, void *buf, size_t len, size_t size);
struct mas_pkt *mas_pkt_alloc(int sid, size_t size);
int mas_pkt_free(struct mas_pkt *pkt);
struct mas_pkt *mas_pkt_info_set(struct mas_pkt *pkt, uint8_t type, uint16_t flag);
int mas_pkt_info_get(struct mas_pkt *pkt, uint8_t *type, uint16_t *flag);

/* MAS link protocol operations */
int mas_link_control(uint8_t port_id, int cmd, void *arg);

struct mas_pbuf *mas_pbuf_alloc(size_t size);
void mas_pbuf_free(struct mas_pbuf *pbuf);

#ifdef __cplusplus
}
#endif

#endif /* __MAS_H__ */
