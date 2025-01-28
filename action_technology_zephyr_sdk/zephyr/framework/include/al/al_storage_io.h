/*
 * Copyright (c) 2017 Actions Semi Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.

 * Author: jpl<jianpingliao@actions-semi.com>
 *
 * Change log:
 *	2019/12/16: Created by jpl.
 */

#ifndef __AL_STORAGE_IO_H__
#define __AL_STORAGE_IO_H__


/** audio storage io tell mode */
enum {
	STORAGEIO_TELL_CUR = 0,
	STORAGEIO_TELL_END,
};

/** audio storage io seek mode */
enum {
	STORAGEIO_SEEK_SET = 0,
	STORAGEIO_SEEK_CUR,
	STORAGEIO_SEEK_END,
};

/** audio codec storage io structure */
typedef struct storage_io_s {
	/** read function */
	int (*read)(void *buf, int size, int count, struct storage_io_s *io);
	/** write function */
	int (*write)(void *buf, int size, int count, struct storage_io_s *io);
	/** seek function.
	 * @whence seek mode, STORAGEIO_SEEK_x
	 */
	int (*seek)(struct storage_io_s *io, int offset, int whence);
	/** tell function.
	 * @mode: 0 current position, while 1 end position, that is, file length
	 */
	int (*tell)(struct storage_io_s *io, int mode);
	/** handle of storage io*/
	void *hnd;
} storage_io_t;


#endif /* __AL_STORAGE_IO_H__ */
