/*
 * Copyright (c) 2013-2020 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __SDFS_H__
#define __SDFS_H__

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifndef FS_SEEK_SET
#define FS_SEEK_SET	0	/* Seek from beginning of file. */
#endif
#ifndef FS_SEEK_CUR
#define FS_SEEK_CUR	1	/* Seek from current position. */
#endif
#ifndef FS_SEEK_END
#define FS_SEEK_END	2	/* Seek from end of file.  */
#endif


#define _SDFS_VOL_STRS	"NOR","SD","NAND","DNOR","NAND"
/**
 * @brief Structure to describe file information
 *
 * Used in functions that processes file.
 *
 * @param start Offset of file in sd file system
 * @param size Size of file
 * @param readptr Read pointer of File.
 */

struct sd_file
{
	int start;
	int size;
	int readptr;
	unsigned char storage_id;
	unsigned char file_id;
	unsigned char used;
	unsigned char reserved[1];
};

struct sd_dir
{
	const unsigned char fname[12];	//8+1+3
	int offset;
	int size;
	unsigned int reserved[2];
	unsigned int checksum;
};

/**
 * @brief File open
 *
 * Opens an existing file.
 *
 * @param file_name The name of file to open  /SD:A-Z/file
 *
 * @retval NULL if error
 * @retval other Pointer to the file object
 */
struct sd_file * sd_fopen (const char *filename);

/**
 * @brief File close
 *
 * Closes the file.
 *
 * @param sd_file Pointer to the file object
 *
 * @return N/A
 */
void sd_fclose(struct sd_file *sd_file);

/**
 * @brief File read
 *
 * Reads items of data of len bytes long.
 *
 * @param sd_file Pointer to the file object
 * @param buffer Pointer to the data buffer
 * @param len Number of bytes to be read
 *
 * @return Number of bytes read. On success, it will be equal to number of
 * items requested to be read. Or returns the file size if there are not 
 * enough bytes available in file. 
 */
int sd_fread(struct sd_file *sd_file, void *buffer, int len);

/**
 * @brief Get current file position.
 *
 * Retrieves the current position in the file.
 *
 * @param sd_file Pointer to the file object
 *
 * @retval position Current position in file
 * Current revision does not validate the file object.
 */
int sd_ftell(struct sd_file *sd_file);

/**
 * @brief File seek
 *
 * Moves the file position to a new location in the file. The offset is added
 * to file position based on the 'whence' parameter.
 *
 * @param sd_file Pointer to the file object
 * @param offset Relative location to move the file pointer to
 * @param whence Relative location from where offset is to be calculated.
 * - FS_SEEK_SET = from beginning of file
 * - FS_SEEK_CUR = from current position,
 * - FS_SEEK_END = from end of file.
 *
 * @retval 0 Success
 * @retval -ERRNO errno code if error.
 */
int sd_fseek(struct sd_file *sd_file, int offset, unsigned char whence);

/**
 * @brief Retrieves file size
 *
 * Checks the size of a file specified by the path
 *
 * @param filename name to the file
 *
 * @retval 0 Success
 * @retval -ERRNO errno code if error
 */
int sd_fsize(const char *filename);

/**
 * @brief Mapping file
 *
 * Mapping file to access files directly using addresses.
 *
 * @param filename name to the file
 * @param addr Pointer to the file addr
 * @param addr Pointer to the file size
 *
 * @retval 0 Success
 * @retval -ERRNO errno code if error
 */
int sd_fmap(const char *filename, void** addr, int *len);


/**
 * @brief sdfs_xip_check_valid  check sdfs(sdfs is xip read)

 * @retval 0 Success
 * @retval else error
 */
int sdfs_xip_check_valid(unsigned int xip_addr_start);

/**
 * @brief sdfs_verify  check sdfs

 * @retval 0 Success
 * @retval else error
 */
int sdfs_verify(const char *mnt_point);


/**
 * @brief sdfs get checksum

 * @retval checksum
 */
unsigned int sdfs_chksum(const char *mnt_point);


#endif