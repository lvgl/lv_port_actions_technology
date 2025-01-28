/*
 * Copyright (c) 2018 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file stream interface
 */

#ifndef __IOSTREAM_H__
#define __IOSTREAM_H__

#include <os_common_api.h>
#include <sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif
struct __stream;
typedef struct __stream *io_stream_t;

/** stream notify type */
typedef enum {
	/** stream notify read */
	STREAM_NOTIFY_READ = BIT(0),
	/** stream notify write */
	STREAM_NOTIFY_WRITE = BIT(1),
	/** stream notify seek */
	STREAM_NOTIFY_SEEK = BIT(2),
	/** stream notify flush */
	STREAM_NOTIFY_FLUSH = BIT(3),
	/** stream notify pre write */
	STREAM_NOTIFY_PRE_WRITE = BIT(4),
} stream_notify_type;

typedef void (*stream_observer_notify)(void *observer, int readoff, int writeoff, int total_size,
							unsigned char *buf, int num, stream_notify_type type);
/**
 * @defgroup stream_apis Stream APIs
 * @ingroup system_apis
 * @{
 */


/** seek mode */
typedef enum {
	/** seek from file begin*/
	SEEK_DIR_BEG,
	/** seek from cur position*/
	SEEK_DIR_CUR,
	/** seek from file end*/
	SEEK_DIR_END,
} seek_dir;

/** tell mode */
typedef enum {
	/** tell current off*/
	TELL_CUR,
	/** tell end of*/
	TELL_END,
} tell_mode;

/** stream read or write mode */
typedef enum {
	/** read only stream */
	MODE_IN = 0x01,
	/** write only stream */
	MODE_OUT = 0x02,
	/** read & write stream */
	MODE_IN_OUT = 0x03,
	/** read stream block*/
	MODE_READ_BLOCK = 0x04,
	/** write stream block*/
	MODE_WRITE_BLOCK = 0x08,
	/**
	 * read/write stream block timeout , 1s timeout,
	 * if not set this bit ,timeout for forever
     */
	MODE_BLOCK_TIMEOUT = 0x10,
} stream_mode;

/** stream state */
typedef enum {
	/** stream state init*/
	STATE_INIT = 1,
	/** stream state open */
	STATE_OPEN ,
	/** stream state close */
	STATE_CLOSE ,
} stream_state;

/** structure of stream*/
typedef struct {
	/** stream open operation Function pointer*/
	int (*init)(io_stream_t handle,void *param);
	/** stream open operation Function pointer*/
	int (*open)(io_stream_t handle, stream_mode mode);
	/** stream read operation Function pointer*/
	int (*read)(io_stream_t handle, unsigned char *buf, int num);
	/** stream seek operation Function pointer*/
	int (*seek)(io_stream_t handle, int offset,seek_dir origin);
	/** stream tell operation Function pointer*/
	int (*tell)(io_stream_t handle);
	/** stream get_length operation Function pointer*/
	int (*get_length)(io_stream_t handle);
	/** stream get space operation Function pointer*/
	int (*get_space)(io_stream_t handle);
	/** stream write operation Function pointer*/
	int (*write)(io_stream_t handle, unsigned char *buf, int num);
	/** stream flush operation Function pointer*/
	int (*flush)(io_stream_t handle);
	/** stream close operation Function pointer*/
	int (*close)(io_stream_t handle);
	/** stream destroy operation Function pointer*/
	int (*destroy)(io_stream_t handle);
	void *(*get_ringbuffer)(io_stream_t handle);
} stream_ops_t;

/**
 * @brief create stream , return stream handle
 *
 * This routine provides create stream ,and return stream handle.
 * and stream state is  STATE_INIT
 *
 * @param ops sub stream operations
 * @param init_param create stream init param
 *
 * @return stream handle if create stream success
 * @return NULL  if create stream failed
 */
io_stream_t stream_create(const stream_ops_t  *ops, void *init_param);

/**
 * @brief open stream
 *
 * This routine provides open stream, stream state must be STATE_INIT
 * and after this routine stream state change to STATE_OPEN
 * stream read and write operation can be dong after stream open
 *
 * @param handle handle of stream
 * @param mode mode of stream
 *
 * @return 0 stream open success
 * @return != 0  stream open failed
 */
int stream_open(io_stream_t handle, stream_mode mode);

/**
 * @brief read from stream
 *
 * This routine provides read num of bytes from stream, if stream states is
 * not STATE_OPEN , return err , else return the realy read data len to user
 *
 * @param handle handle of stream
 * @param buf out put data buffer pointer
  * @param num bytes user want to read
 *
 * @return >=0 the realy read data length ,if return < num, means read finished.
 * @return <0  stream read failed
 */
int stream_read(io_stream_t handle, void *buf, int num);

/**
 * @brief write to stream
 *
 * This routine provides write num of bytes to stream, if stream states is
 * not STATE_OPEN , return err , else return the realy write data len to user
 *
 *
 * @param handle handle of stream
 * @param buf data poninter of write
 * @param num bytes user want to write
 *
 * @return >=0 the realy write data length ,if return < num, means write finished.
 * @return <0  stream write failed
 */
int stream_write(io_stream_t handle, const void *buf, int num);

/**
 * @brief seek stream
 *
 * This routine provides seek stream .
 *
 *
 * @param handle handle of stream
 * @param offset offset want to seek
 * @param origin seek mode , net stream only support SEEK_DIR_CUR
 *
 * @return 0 seek success
 * @return !=0  seek failed
 */
int stream_seek(io_stream_t handle, int offset,seek_dir origin);

/**
 * @brief tell stream
 *
 * This routine provides return stream len , this interface only support
 * for file stream ,return the file len.
 *
 * @param handle handle of stream
 *
 * @return len return the stream len
 */
int stream_tell(io_stream_t handle);

/**
 * @brief check stream finished
 *
 * This routine provides check the stream finined
 *
 * @param handle handle of stream
 *
 * @return true finished else no finished
 */
bool stream_check_finished(io_stream_t handle);

/**
 * @brief flush stream
 *
 * This routine provides flush stream , change state to STATE_CLOSE
 * read and write seek tell operation not allowed after this routine
 * execute.
 *
 * @param handle handle of stream
 *
 * @return 0 close success
 * @return !=0  close failed
 */
int stream_flush(io_stream_t handle);

/**
 * @brief close stream
 *
 * This routine provides close stream , change state to STATE_CLOSE
 * read and write seek tell operation not allowed after this routine
 * execute.
 *
 * @param handle handle of stream
 *
 * @return 0 close success
 * @return !=0  close failed
 */
int stream_close(io_stream_t handle);

/**
 * @brief destroy stream
 *
 * This routine provides destroy stream , free all stream resurce
 * not allowed access stream handle after this routine execute.
 *
 * @param handle handle of stream
 *
 * @return 0 destroy success
 * @return !=0  destroy failed
 */
int stream_destroy(io_stream_t handle);

/**
 * @brief get stream data length
 *
 * This routine provides get stream length.
 *
 * @param handle handle of stream
 *
 * @return -1 get failed
 * @return size of cache data length
 */
int stream_get_length(io_stream_t handle);

/**
 * @brief get stream base ringbuffer
 *
 * This routine provides get stream ringbuffer datastructer
 *
 * @param handle handle of stream
 *
 * @return ringbuffer base address
 */
void *stream_get_ringbuffer(io_stream_t handle);

/**
 * @brief get stream free space
 *
 * This routine provides get stream free space. only support by in_out_mode
 * stream
 *
 * @param handle handle of stream
 *
 * @return -1 get failed
 * @return size of free space in stream
 */
int stream_get_space(io_stream_t handle);

/**
 * @brief stream attach
 *
 * This routine provides attach dest stream to origin stream, data will be read/write to dest stream
 * when data write to origin stream,
 *
 * @param origin handle of origin stream
 * @param attach_stream handle of dest attach stream
 * @param attach_type MODE_IN, MODE_OUT
 *
 * @return 0 attach success
 * @return !=0  attach failed
 */
int stream_attach(io_stream_t origin, io_stream_t attach_stream, int attach_type);

/**
 * @brief stream detach
 *
 * This routine provides detach dest stream from origin stream
 *
 * @param origin handle of origin stream
 * @param attach_stream handle of dest attach stream
 *
 * @return 0 detach success
 * @return !=0  detach failed
 */
int stream_detach(io_stream_t origin, io_stream_t attach_stream);

/**
 * @brief stream dump information
 *
 * This routine provides dumping stream information
 *
 * @param stream handle of stream
 * @param name dump name of stream
 * @param line_prefix Prefix of each line.
 *
 * @return N/A
 */
void stream_dump(io_stream_t stream, const char *name, const char *line_prefix);

/**
 * @cond INTERNAL_HIDDEN
 */

/** structure of stream*/
struct __stream {
	/** stream type */
	uint8_t type;
    /** stream mode */
	uint8_t mode;
	/** stream state */
	uint8_t state;
	/** stream format for play*/
	uint8_t format;
	/** read off set of stream*/
	uint32_t rofs;
	/** write off set of stream*/
	uint32_t wofs;
	/** state of stream is wroking*/
	uint32_t isworking:1;
	/** flag of stream write finished*/
	uint32_t write_finished:1;
	/** flag of stream write must wait for free space*/
	uint32_t write_pending:1;
	/** flag of stream write must wait for free space*/
	uint32_t cache_size_changed:1;

	/** file cache size*/
	uint32_t cache_size;
	/** total size*/
	uint32_t total_size;

	os_sem *sync_sem;

	void *observer[2];
	uint8_t  observer_type[2];
	stream_observer_notify observer_notify[2];

	/* attach direction (MODE_IN, MODE_OUT) : origin stream data read or write */
	uint8_t attach_mode[1];
	/* stream array that will be attach to */
	io_stream_t attach_stream[1];
	/* stream is attache */
	io_stream_t attached_stream;
	/* attach lock */
	os_mutex attach_lock;

	const stream_ops_t  *ops;
	void *data;
};

/**
 * @brief set stream cache size
 *
 * This routine provides set stream cache size. only support by in_out_mode
 * stream
 *
 * @param handle handle of stream
 * @param size cache size of stream
 *
 * @return 0 set success
 * @return !=0  set failed
 */
int stream_set_cache_size(io_stream_t handle, int size);

/**
 * @brief add stream observer
 *
 * This routine provides add stream observer. if stream data changed , stream observer must be
 * notified by stream
 *
 * @param handle handle of stream
 * @param observer handle of observer
 * @param notify notify function
 * @param type notify type
 *
 * @return 0 set success
 * @return !=0  set failed
 */
int stream_set_observer(io_stream_t handle, void *observer, stream_observer_notify notify, uint8_t type);

/**
 * INTERNAL_HIDDEN @endcond
 */

/**
 * @} end defgroup stream_apis
 */

#ifdef __cplusplus
}
#endif

#endif /* __IOSTREAM_H__ */
