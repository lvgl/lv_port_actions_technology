/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#ifndef __MAS_LINK_CHAR_H__
#define __MAS_LINK_CHAR_H__

#include "gx_mutex.h"
#include <mas_link.h>
#include <mas_proto.h>

enum mas_uart_frame_index {
    MAS_UART_FRAME_INDEX_HEAD = 0,
    MAS_UART_FRAME_INDEX_LEN1,
    MAS_UART_FRAME_INDEX_LEN2,
    MAS_UART_FRAME_INDEX_ID,
    MAS_UART_FRAME_INDEX_RESERVE,
    MAS_UART_FRAME_INDEX_TYPE,
    MAS_UART_FRAME_INDEX_CRC1,
    MAS_UART_FRAME_INDEX_CRC2,
};

/* MAS link uart frame */
struct mas_uart_frame {
    uint8_t head;           /* frame header */
    uint16_t len;           /* frame length */
    uint8_t id;             /* frame ID */
    uint8_t reserve;        /* reserve bytes */
    struct mas_frame frame; /* MAS common frame */
    uint16_t crc16;         /* crc16 check value*/
    uint8_t end_sign;       /* end sign */
};

struct mas_link_char_port_ops {
    size_t (*device_write)(struct mas_link_char *link_char, const void *buffer, size_t size);
};

struct mas_link_char {
    struct mas_link link;                   /* link object */
    uint8_t need_crc16;                     /* frame need crc16 check */
    uint8_t *recv_stack;                    /* recv thread stack */
    uint8_t *recv_mq_pool;                  /* recv message pool */
    gx::os::timer_t recv_timeout;           /* recv frame timeout timer */
    gx::os::semaphore_t send_dma_sem;       /* send data dma semaphore */
    uint8_t send_dma_flag;                  /* send data using dma flag */
    const mas_link_char_port_ops *port_ops; /* porting interface */
    void *user_data;                        /* user-specific data */
};

class MasDiscardManager {
public:
    MasDiscardManager(float per);
    ~MasDiscardManager();

    static MasDiscardManager *instance() { return m_instance; }

    bool isDiscardData();

    void stopTest();

private:
    gx::Mutex m_discardMutex{};
    float m_percentage;
    float m_curPercentage;
    int64_t m_count;
    int32_t m_discardCount;
    static MasDiscardManager *m_instance;
};

#endif
