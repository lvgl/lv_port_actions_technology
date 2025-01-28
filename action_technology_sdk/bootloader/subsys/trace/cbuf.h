/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file crc interface
 */

#ifndef CBUF_H_
#define CBUF_H_

typedef struct
{
    int32_t read_ptr;
    int32_t write_ptr;
    int32_t length;
    uint32_t capacity;
    uint8_t *raw_data;
}cbuf_t;

typedef struct
{
    uint8_t *start_addr;
    uint32_t read_len;
}cbuf_dma_t;


static inline int cbuf_get_used_space(cbuf_t *cbuf)
{
    if(!cbuf){
        return 0;
    }
    
    return cbuf->length;
}

static inline int cbuf_get_free_space(cbuf_t *cbuf)
{
    if(!cbuf){
        return 0;
    }

    return cbuf->capacity - cbuf->length;
}

static inline int cbuf_is_init(cbuf_t *cbuf)
{
    if(!cbuf){
        return false;
    }

    return (cbuf->capacity != 0);
}

static inline int cbuf_is_ptr_valid(cbuf_t *cbuf, uint32_t ptr)
{
    if((ptr >= (uint32_t)cbuf->raw_data) \
        && (ptr <= ((uint32_t)cbuf->raw_data + cbuf->capacity))){
        return 1;
    }else{
        return 0;
    }
}

static inline void *cbuf_get_buffer_ptr(cbuf_t *cbuf)
{
    return (void *)cbuf->raw_data;
}

void cbuf_init(cbuf_t *cbuf, void *buffer, int32_t size);

void cbuf_reset(cbuf_t *cbuf);

int cbuf_read(cbuf_t *cbuf, void *buffer, int32_t len);

int cbuf_write(cbuf_t *cbuf, void *buffer, int32_t len);

int cbuf_dma_read(cbuf_t *cbuf, cbuf_dma_t *dma, int32_t read_len);

int cbuf_dma_update_read_ptr(cbuf_t *cbuf, int32_t read_len);

void cbuf_copy_write_ptr(cbuf_t *src, cbuf_t *dest, uint32_t len);

void cbuf_deinit(cbuf_t *cbuf);

int cbuf_prepare_read(cbuf_t *cbuf, void *buffer, int32_t len);

#endif /* CBUF_H_ */
