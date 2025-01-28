/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#ifndef __MAS_TRANS_H__
#define __MAS_TRANS_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <mas_proto.h>

/* MAS D2D packet header length (4 bytes),
   SRC ID(1byte) + DST ID(1byte) + PKT ID(1byte) + PKT type(1byte) */
#define MAS_D2D_PKT_HEAD_LEN 4
/* MAS D2D packet buffer max */
#define MAS_D2D_PKT_MAX_SIZE (MAS_PKT_MAX_SIZE + MAS_D2D_PKT_HEAD_LEN)

struct mas_d2d_pkt {
    uint8_t dst_id;        /* destination device ID */
    uint8_t src_id;        /* source device ID */
    uint8_t pkt_id;        /* packet ID */
    uint8_t pkt_type : 2;  /* packet type */
    uint8_t need_ack : 1;  /* need ACK */
    uint8_t need_rsp : 1;  /* need response */
    uint8_t priority : 2;  /* packet priority */
    uint8_t reserve : 2;   /* reserve data */
    struct mas_pbuf *pbuf; /* MAS d2d packet buffer */
    size_t len;            /* MAS d2d packet length */
};

enum mas_d2d_pkt_type {
    MAS_D2D_PKT_TYPE_REQ = 0,
    MAS_D2D_PKT_TYPE_RSP,
    MAS_D2D_PKT_TYPE_ACK,
    MAS_D2D_PKT_TYPE_BROAD,
    MAS_D2D_PKT_TYPE_PROXY,
    MAS_D2D_PKT_TYPE_MAX,
};

enum mas_d2d_pkt_index {
    MAS_D2D_PKT_INDEX_SRC_ID = 0,
    MAS_D2D_PKT_INDEX_DST_ID,
    MAS_D2D_PKT_INDEX_PKT_ID,
    MAS_D2D_PKT_INDEX_PKT_INFO,
};

/* MAS transport layer protocol type */
enum mas_trans_type {
    MAS_TRANS_TYPE_D2D = 0x01,
    MAS_TRANS_TYPE_RESERVE,
    /* reserved for furture use */
    MAS_TRANS_TYPE_USER
};

#ifdef MCF_SECURITY_AES256
/* AES data length align */
#define MAS_AES_ALIGN(len)             RT_ALIGN(len, 16)
/* AES data header length */
#define MAS_AES_DATA_HEAD              2
#endif

/* get MAS object  */
struct mas_object *mas_object_get(void);

/* MAS security operations */
mas_err_t mas_security_init(void);
mas_err_t mas_security_pack(void *input, size_t in_len, void **output, size_t *out_len);
mas_err_t mas_security_unpack(void *input, size_t in_len, void **output, size_t *out_len);

#ifdef  __cplusplus
}
#endif

#endif /* __MAS_TRANS_H__ */
