/****************************************************************************
 *
 * ftsystem.c
 *
 *   ANSI-specific FreeType low-level system interface (body).
 *
 * Copyright (C) 1996-2020 by
 * David Turner, Robert Wilhelm, and Werner Lemberg.
 *
 * This file is part of the FreeType project, and may only be used,
 * modified, and distributed under the terms of the FreeType project
 * license, LICENSE.TXT.  By continuing to use, modify, or distribute
 * this file you indicate that you have read the license and
 * understand and accept it fully.
 *
 */

  /**************************************************************************
   *
   * This file contains the default interface used by FreeType to access
   * low-level, i.e. memory management, i/o access as well as thread
   * synchronisation.  It can be replaced by user-specific routines if
   * necessary.
   *
   */


#include <ft2build.h>
#include FT_CONFIG_CONFIG_H
#include <freetype/internal/ftdebug.h>
#include <freetype/internal/ftstream.h>
#include <freetype/ftsystem.h>
#include <freetype/fterrors.h>
#include <freetype/fttypes.h>

#include "../../font_mempool.h"
#include <zephyr.h>
#include <fs/fs.h>
#include <os_common_api.h>


  /**************************************************************************
   *
   *                      MEMORY MANAGEMENT INTERFACE
   *
   */

  /**************************************************************************
   *
   * It is not necessary to do any error checking for the
   * allocation-related functions.  This will be done by the higher level
   * routines like ft_mem_alloc() or ft_mem_realloc().
   *
   */

void *ft_smalloc(size_t size)
{
	void* ptr;
	ptr = bitmap_font_cache_malloc(size);
	if(ptr == NULL)
	{
    SYS_LOG_ERR("ft malloc faild\n");
	}

	return ptr;
}

void ft_sfree(void *ptr)
{
	bitmap_font_cache_free(ptr);
}

void *ft_srealloc(void *ptr, size_t requested_size)
{
	void *new_ptr;
	size_t copy_size;

	if (ptr == NULL) {
		return ft_smalloc(requested_size);
	}

	if (requested_size == 0) {
		ft_sfree(ptr);
		return NULL;
	}

	if(requested_size % 4 != 0)
	{
		requested_size = (requested_size/4 + 1)*4;
	}	

	copy_size = bitmap_font_cache_get_size(ptr);
	if(copy_size == requested_size)
	{
		return ptr;
	}

	new_ptr = ft_smalloc(requested_size);
	if (new_ptr == NULL) {
		return NULL;
	}

	if (copy_size > requested_size)
	{
		copy_size = requested_size;
	}
	memcpy(new_ptr, ptr, copy_size);
	
	ft_sfree(ptr);

	return new_ptr;

}

void *ft_scalloc(size_t nmemb, size_t size)
{
	void *ptr;
	size_t total = nmemb*size;

	SYS_LOG_INF("calloc total %d\n", total);
	ptr = ft_smalloc(total);

	if (ptr != NULL) {
	  memset(ptr, 0, total);
	}

	return ptr;
}




  /**************************************************************************
   *
   * @Function:
   *   ft_alloc
   *
   * @Description:
   *   The memory allocation function.
   *
   * @Input:
   *   memory ::
   *     A pointer to the memory object.
   *
   *   size ::
   *     The requested size in bytes.
   *
   * @Return:
   *   The address of newly allocated block.
   */
  FT_CALLBACK_DEF( void* )
  ft_alloc( FT_Memory  memory,
            long       size )
  {
    FT_UNUSED( memory );

    return ft_smalloc( (size_t)size );
  }


  /**************************************************************************
   *
   * @Function:
   *   ft_realloc
   *
   * @Description:
   *   The memory reallocation function.
   *
   * @Input:
   *   memory ::
   *     A pointer to the memory object.
   *
   *   cur_size ::
   *     The current size of the allocated memory block.
   *
   *   new_size ::
   *     The newly requested size in bytes.
   *
   *   block ::
   *     The current address of the block in memory.
   *
   * @Return:
   *   The address of the reallocated memory block.
   */
  FT_CALLBACK_DEF( void* )
  ft_realloc( FT_Memory  memory,
              long       cur_size,
              long       new_size,
              void*      block )
  {
    FT_UNUSED( memory );
    FT_UNUSED( cur_size );

    return ft_srealloc( block, (size_t)new_size );
  }


  /**************************************************************************
   *
   * @Function:
   *   ft_free
   *
   * @Description:
   *   The memory release function.
   *
   * @Input:
   *   memory ::
   *     A pointer to the memory object.
   *
   *   block ::
   *     The address of block in memory to be freed.
   */
  FT_CALLBACK_DEF( void )
  ft_free( FT_Memory  memory,
           void*      block )
  {
    FT_UNUSED( memory );

    ft_sfree( block );
  }


  /**************************************************************************
   *
   *                    RESOURCE MANAGEMENT INTERFACE
   *
   */

#ifndef FT_CONFIG_OPTION_DISABLE_STREAM_SUPPORT

  /**************************************************************************
   *
   * The macro FT_COMPONENT is used in trace mode.  It is an implicit
   * parameter of the FT_TRACE() and FT_ERROR() macros, used to print/log
   * messages during execution.
   */
#undef  FT_COMPONENT
#define FT_COMPONENT  io

  /* We use the macro STREAM_FILE for convenience to extract the       */
  /* system-specific stream handle from a given FreeType stream object */
#define STREAM_FILE( stream )  ( (struct fs_file_t*)stream->descriptor.pointer )


  /**************************************************************************
   *
   * @Function:
   *   ft_ansi_stream_close
   *
   * @Description:
   *   The function to close a stream.
   *
   * @Input:
   *   stream ::
   *     A pointer to the stream object.
   */
  FT_CALLBACK_DEF( void )
  ft_ansi_stream_close( FT_Stream  stream )
  {
#ifdef CONFIG_FILE_SYSTEM
    struct fs_file_t *file = STREAM_FILE(stream);

    fs_close( file );
    bitmap_font_cache_free( file );
#endif
    stream->descriptor.pointer = NULL;
    stream->size               = 0;
    stream->base               = NULL;
  }


  /**************************************************************************
   *
   * @Function:
   *   ft_ansi_stream_io
   *
   * @Description:
   *   The function to open a stream.
   *
   * @Input:
   *   stream ::
   *     A pointer to the stream object.
   *
   *   offset ::
   *     The position in the data stream to start reading.
   *
   *   buffer ::
   *     The address of buffer to store the read data.
   *
   *   count ::
   *     The number of bytes to read from the stream.
   *
   * @Return:
   *   The number of bytes actually read.  If `count' is zero (this is,
   *   the function is used for seeking), a non-zero return value
   *   indicates an error.
   */
  FT_CALLBACK_DEF( unsigned long )
  ft_ansi_stream_io( FT_Stream       stream,
                     unsigned long   offset,
                     unsigned char*  buffer,
                     unsigned long   count )
  {
#ifdef CONFIG_FILE_SYSTEM
    struct fs_file_t *file = STREAM_FILE( stream );
    long read_cnt = 0;
	int32_t ret;

    if ( !count && offset > stream->size )
      return 1;

    if ( stream->pos != offset )
    {
      	ret = fs_seek( file, (long)offset, FS_SEEK_SET );
	  	if(ret < 0)
	  	{
	  		SYS_LOG_ERR("fs seek failed %d\n", ret);
	  	}
    }

	if(count == 0)
	{
		return 0;
	}
	
    read_cnt = fs_read( file, buffer, count );
    return ( read_cnt > 0 ) ? read_cnt : 0;
#else
    return 0;
#endif
  }


  /* documentation is in ftstream.h */
  FT_BASE_DEF( FT_Error )
  FT_Stream_Open( FT_Stream    stream,
                  const char*  filepathname )
  {
#ifdef CONFIG_FILE_SYSTEM
	  struct fs_file_t* file = bitmap_font_cache_malloc(sizeof(struct fs_file_t));
	  int32_t ret;

    if(!file)
    {
        SYS_LOG_ERR("malloc freetype font file struct failed\n");
        FT_ERROR(( "FT_Stream_Open:"
                  " could not open `%s'\n", filepathname ));      
        return FT_Err_Cannot_Open_Resource;
    }
    memset(file, 0, sizeof(struct fs_file_t));

    if ( !stream )
      return FT_Err_Invalid_Stream_Handle;

    stream->descriptor.pointer = NULL;
    stream->pathname.pointer   = (char*)filepathname;
    stream->base               = NULL;
    stream->pos                = 0;
    stream->read               = NULL;
    stream->close              = NULL;


    ret = fs_open(file, filepathname, FS_O_READ);
    if (ret < 0)
    {
      SYS_LOG_ERR("fs_open ret %d\n", ret);
      bitmap_font_cache_free( file );
      FT_ERROR(( "FT_Stream_Open:"
                " could not open `%s'\n", filepathname ));      
      return FT_Err_Cannot_Open_Resource;      
    }

    fs_seek( file, 0, FS_SEEK_END );
    stream->size = (unsigned long)fs_tell( file );
    if ( !stream->size )
    {
      FT_ERROR(( "FT_Stream_Open:" ));
      FT_ERROR(( " opened `%s' but zero-sized\n", filepathname ));

      fs_close( file );
      bitmap_font_cache_free( file );
      return FT_THROW( Cannot_Open_Stream );
    }
    fs_seek( file, 0, FS_SEEK_SET );

    stream->descriptor.pointer = file;
    stream->read  = ft_ansi_stream_io;
    stream->close = ft_ansi_stream_close;

    FT_TRACE1(( "FT_Stream_Open:" ));
    FT_TRACE1(( " opened `%s' (%d bytes) successfully\n",
                filepathname, stream->size ));

    return FT_Err_Ok;
#else
    return FT_Err_Cannot_Open_Resource;
#endif
  }

#endif /* !FT_CONFIG_OPTION_DISABLE_STREAM_SUPPORT */

#ifdef FT_DEBUG_MEMORY

  extern FT_Int
  ft_mem_debug_init( FT_Memory  memory );

  extern void
  ft_mem_debug_done( FT_Memory  memory );

#endif


  /* documentation is in ftobjs.h */

  FT_BASE_DEF( FT_Memory )
  FT_New_Memory( void )
  {
    FT_Memory  memory;


    memory = (FT_Memory)ft_smalloc( sizeof ( *memory ) );
    if ( memory )
    {
      memory->user    = NULL;
      memory->alloc   = ft_alloc;
      memory->realloc = ft_realloc;
      memory->free    = ft_free;
#ifdef FT_DEBUG_MEMORY
      ft_mem_debug_init( memory );
#endif
    }

    return memory;
  }


  /* documentation is in ftobjs.h */

  FT_BASE_DEF( void )
  FT_Done_Memory( FT_Memory  memory )
  {
#ifdef FT_DEBUG_MEMORY
    ft_mem_debug_done( memory );
#endif
    ft_sfree( memory );
  }


/* END */
