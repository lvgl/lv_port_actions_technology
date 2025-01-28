/*
 * Copyright (c) 2020, Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stream.h>
#include <mem_manager.h>
#include <arithmetic_storage_io.h>


#define STORAGE_BLOCK (2048)
#define STORAGE_BLOCK_MASK (0xFFFFF800)
#ifdef CONFIG_ACTIONS_PARSER
static u8_t storage_block_buff[STORAGE_BLOCK];
#endif
struct storage_block_info_t {
	storage_io_t *io;
	int data_size;
	int file_offset;
	int r_offset;
	u8_t *buff;
};

static struct storage_block_info_t storage_block = {
    .io = NULL,
};

static int _storage_stream_read(void *buf, int size, int count, storage_io_t *io)
{
	struct storage_block_info_t *info = &storage_block;

	if (!io || !io->hnd)
		return -EINVAL;

	if (size != 1)
		count *= size;

	if (info->io && info->io == io) {
        io_stream_t stream = (io_stream_t)io->hnd;
        
        size = count;
        while((size > 0) && (info->r_offset < stream->total_size)) {
            if((info->r_offset - info->file_offset) >= info->data_size) {
                info->file_offset += info->data_size;
                
    			info->data_size = stream_read(stream, info->buff, STORAGE_BLOCK);
                if(info->data_size <= 0) {
                    if(size == count) {
                        return info->data_size;
                    } else {
                        break;
                    }
                }

                continue;
            }

            int len = info->file_offset + info->data_size - info->r_offset;
            len = min(len, size);
            memcpy(buf, &info->buff[info->r_offset - info->file_offset], len);

            info->r_offset += len;
            size -= len;
            buf = (char*)buf + len;
        }

		return count - size;
	} else {
		int len = stream_read((io_stream_t)io->hnd, buf, count);

		return len;
	}
}

static int _storage_stream_write(void *buf, int size, int count, storage_io_t *io)
{
	if (!io || !io->hnd)
		return -EINVAL;

	if (size != 1)
		count *= size;

	return stream_write((io_stream_t)io->hnd, buf, count);
}

static int _storage_stream_seek(storage_io_t *io, int offset, int whence)
{
	struct storage_block_info_t *info = &storage_block;
    
	if (!io || !io->hnd)
		return -EINVAL;

	if (info->io && info->io == io) {
        io_stream_t stream = (io_stream_t)io->hnd;
        int target_off;

        switch(whence) {
    	case SEEK_DIR_BEG:
    		target_off = offset;
    		break;
    	case SEEK_DIR_CUR:
			target_off = info->r_offset + offset;
    		break;
    	case SEEK_DIR_END:
    		target_off = stream->total_size + offset;
    		break;
    	default:
    		SYS_LOG_ERR("mode not support 0x%x \n", whence);
    		return -1;
    	}

        if(target_off < 0) {
            SYS_LOG_WRN("err offset %d, %d, %d\n", offset, whence, target_off);
            target_off = 0;
        } else if(target_off > stream->total_size) {
            SYS_LOG_WRN("err offset %d, %d, %d\n", offset, whence, target_off);
            target_off = stream->total_size;
        }
        
        info->data_size = 0;
        info->file_offset = target_off & STORAGE_BLOCK_MASK;
		info->r_offset = target_off;
		return stream_seek((io_stream_t)io->hnd, info->file_offset, SEEK_DIR_BEG);
	} else {
        /* STORAGEIO_SEEK_xxx equal to io_stream_t SEEK_DIR_xxx */
    	return stream_seek((io_stream_t)io->hnd, offset, whence);
	}
}

static int _storage_stream_tell(storage_io_t *io, int mode)
{
	io_stream_t stream;

	if (!io || !io->hnd)
		return -EINVAL;

	stream = (io_stream_t)io->hnd;
	switch (mode) {
	case STORAGEIO_TELL_CUR:
        if(storage_block.io && (storage_block.io == io)) {
            return storage_block.r_offset;
        } else {
    		return stream_tell(stream);
    	}
        break;
	case STORAGEIO_TELL_END:
		return stream->total_size;
	default:
		return -EINVAL;
	}
}

storage_io_t *storage_io_block_stream(void *stream)
{
	storage_io_t *io = mem_malloc(sizeof(storage_io_t));
	if (io) {
		io->hnd = stream;
		io->read = _storage_stream_read;
		io->write = _storage_stream_write;
		io->seek = _storage_stream_seek;
		io->tell = _storage_stream_tell;
	}

	struct storage_block_info_t *info = &storage_block;
	if (!info->io) {
		memset(info, 0 , sizeof(struct storage_block_info_t));
#ifdef CONFIG_ACTIONS_PARSER
		info->buff = storage_block_buff;
#endif
		info->io = io;
	}

	return io;
}

storage_io_t *storage_io_wrap_stream(void *stream)
{
	storage_io_t *io = mem_malloc(sizeof(storage_io_t));
	if (io) {
		io->hnd = stream;
		io->read = _storage_stream_read;
		io->write = _storage_stream_write;
		io->seek = _storage_stream_seek;
		io->tell = _storage_stream_tell;
	}

	return io;
}

void storage_io_unwrap_stream(storage_io_t *io)
{
	struct storage_block_info_t *info = &storage_block;

	if (info->io && info->io == io) {
		memset(info, 0 , sizeof(struct storage_block_info_t));
		info->io = NULL;
	}

	mem_free(io);
}
