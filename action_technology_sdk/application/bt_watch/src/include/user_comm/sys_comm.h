/*!
 * \file	  sys_comm.h
 * \brief	  System interface
 * \details   
 * \author	  
 * \date	  
 * \copyright Actions
 */

#ifndef _SYS_COMM_H
#define _SYS_COMM_H


//#include <app_config.h>

/*!
 * \brief Loop buffer structure
 */
typedef struct
{
	u8_t*  data_buf;
	u16_t  buf_size;
	u16_t  data_count;
	u16_t  read_pos;
	u16_t  write_pos;
	
} loop_buffer_t;

/*!
 * \brief Loop buffer write data
 * \n  Return the actual write data length
 */
u32_t loop_buffer_write(loop_buffer_t* loop_buf, void* data, u32_t size);


/*!
 * \brief Loop buffer read data
 * \n  Return the actual read data length
 */
u32_t loop_buffer_read(loop_buffer_t* loop_buf, void* data, u32_t size);


char *myitoa(int num,char *str,int radix);

int print_hex(const char* prefix, const void* data, size_t size);

#endif	// _SYS_COMM_H


