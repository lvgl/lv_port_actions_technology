/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file cbuf interface
 */

#include <kernel.h>
#include "cbuf.h"
#include <string.h>

int cbuf_read(cbuf_t *cbuf, void *buffer, int32_t len)
{
    int32_t read_len = len;
    int32_t copy_len;
    uint32_t flags;

    if(!cbuf){
        return 0;
    }

    if(cbuf->length < len){
        return 0;
    }

    if(cbuf->read_ptr >= cbuf->capacity){
        cbuf->read_ptr = 0;
    }
    
    //buffer is null only change read ptr and length
    if(!buffer){
		flags = irq_lock();
        cbuf->read_ptr += len;
        if (cbuf->capacity != 0){
            cbuf->read_ptr %= cbuf->capacity;
        }
        cbuf->length -= len;
		irq_unlock(flags);
		return len;
    }

    copy_len = cbuf->capacity - cbuf->read_ptr;
    if(copy_len > len){
        copy_len = len;
    }

    len -= copy_len;

    memcpy(buffer, cbuf->raw_data + cbuf->read_ptr, copy_len);

    if(len == 0){
        cbuf->read_ptr += copy_len;
    }else{
        memcpy((uint8_t *)buffer + copy_len, cbuf->raw_data, len);
        cbuf->read_ptr = len;
    }

    flags = irq_lock();
    cbuf->length -= read_len;
    irq_unlock(flags);

    return read_len;
}

int cbuf_dma_read(cbuf_t *cbuf, cbuf_dma_t *dma, int32_t read_len)
{
    uint32_t copy_len;

    if(!cbuf){
        goto ret;
    }

    if(cbuf->length < read_len){
        read_len = cbuf->length;
        goto ret;
    }

    if(cbuf->read_ptr >= cbuf->capacity){
        cbuf->read_ptr = 0;
    }

    copy_len = cbuf->capacity - cbuf->read_ptr;
    
    if(copy_len > read_len){
        copy_len = read_len;
    }

    dma->read_len = copy_len;
    dma->start_addr = cbuf->raw_data + cbuf->read_ptr;

    return read_len; 
ret:
    dma->read_len = 0;
    return 0;
}

int cbuf_dma_update_read_ptr(cbuf_t *cbuf, int32_t read_len)
{
    uint32_t flags;

    flags = irq_lock();

    cbuf->read_ptr += read_len;
    
    cbuf->length -= read_len;
    
    irq_unlock(flags);

    return 0;
}

int cbuf_prepare_read(cbuf_t *cbuf, void *buffer, int32_t len)
{
    int32_t read_len = len;
    int32_t copy_len;

    if(!cbuf){
        return 0;
    }

    if(cbuf->length < len){
        return 0;
    }

    if(!buffer){
        return 0;
    }

    if(cbuf->read_ptr >= cbuf->capacity){
        cbuf->read_ptr = 0;
    }
    
    copy_len = cbuf->capacity - cbuf->read_ptr;
    if(copy_len > len){
        copy_len = len;
    }

    len -= copy_len;

    memcpy(buffer, cbuf->raw_data + cbuf->read_ptr, copy_len);

    if(len){
        memcpy((uint8_t *)buffer + copy_len, cbuf->raw_data, len);
    }

    return read_len;

}


int cbuf_write(cbuf_t *cbuf, void *buffer, int32_t len)
{
    uint32_t write_len;
    uint32_t rem_len;
    uint32_t flags;

    if((!cbuf) || (len == 0) || (len > cbuf->capacity)){
        return 0;
    }

    if((cbuf->capacity - cbuf->length) < len){
        return 0;
    }

    if(cbuf->write_ptr >= cbuf->capacity){
        cbuf->write_ptr = 0;
    }

    write_len = cbuf->capacity - cbuf->write_ptr;
    if(write_len >= len){
        memcpy(cbuf->raw_data + cbuf->write_ptr, buffer, len);
        cbuf->write_ptr += len;
    }else{
        rem_len = len - write_len;
        memcpy(cbuf->raw_data + cbuf->write_ptr, buffer, write_len);
        memcpy(cbuf->raw_data, (uint8_t *)buffer + write_len, rem_len);
        cbuf->write_ptr = rem_len;
    }
    
    flags = irq_lock();
    cbuf->length += len;
    irq_unlock(flags);

    return len;
}

void cbuf_init(cbuf_t *cbuf, void *buffer, int32_t size)
{
    uint32_t flags;

    flags = irq_lock();

    cbuf->raw_data = buffer;
    cbuf->capacity = size;
    cbuf->write_ptr = 0;
    cbuf->read_ptr = 0;
    cbuf->length = 0;
    irq_unlock(flags);
}

void cbuf_reset(cbuf_t *cbuf)
{
    uint32_t flags;

    flags = irq_lock();
    cbuf->write_ptr = 0;
    cbuf->read_ptr = 0;
    cbuf->length = 0;
    irq_unlock(flags);
}

void cbuf_copy_write_ptr(cbuf_t *src, cbuf_t *dest, uint32_t len)
{
	uint32_t flags;

	flags = irq_lock();

	dest->write_ptr = src->write_ptr;
    dest->length += len;

	irq_unlock(flags);
}

void cbuf_deinit(cbuf_t *cbuf)
{
    memset(cbuf, 0, sizeof(cbuf_t));
}

