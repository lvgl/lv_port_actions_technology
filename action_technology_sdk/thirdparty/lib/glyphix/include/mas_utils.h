/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */

#pragma once

#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* MAS log level */
typedef enum mas_log {
    MAS_LOG_PHY = 0,
    MAS_LOG_LINK,
    MAS_LOG_MAX,
} mas_log_t;

/* MAS packet buffer */
struct mas_pbuf {
    void *buf;   /* real data address */
    size_t len;  /* current data size */
    size_t size; /* maximum data size */
};

/* MAS packet operations */
struct mas_pbuf *mas_pbuf_alloc(size_t size);
struct mas_pbuf *mas_pbuf_strdup(struct mas_pbuf *pbuf);
void mas_pbuf_free(struct mas_pbuf *pbuf);

/* MAS print raw packet data information */
void mas_print_raw_data(const char *tag, mas_log_t level, const uint8_t *data, size_t size,
                        size_t total);

/* crc16 calculation */
uint16_t mas_crc16_calc(const uint8_t *data, size_t size);

uint32_t _mas_crc32_calculate(uint32_t crc, uint8_t *buf, size_t size);

#ifdef __cplusplus
}
#endif
