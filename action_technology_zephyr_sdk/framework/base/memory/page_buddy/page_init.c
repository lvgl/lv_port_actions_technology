/********************************************************************************
 *                            USDK(GL5120_GIT)
 *                            Module: SYSTEM
 *                 Copyright(c) 2003-2017 Actions Semiconductor,
 *                            All Rights Reserved.
 *
 * History:
 *      <author>      <time>                      <version >          <desc>
 *      wuyufan  2017.9.22-AM 11:20:45             1.0             build this file
 ********************************************************************************/
/*!
 * \file     page_init.c
 * \brief
 * \author
 * \version  1.0
 * \date  2017.9.22-AM 11:20:45
 *******************************************************************************/
#include "heap.h"
#include "page_inner.h"
//#define ADATA_INDEX ((unsigned char)&__ram_adata_mpool0_index)

uint8_t __attribute__((__aligned__(2048))) mem_pool[RAM_POOL_PAGE_SIZE * RAM_POOL_PAGE_NUM];

int pagepool_init(void)
{
	int zone, i;
	unsigned char index;

	/*check link data fit define data */
	while (!(POOL0_NUM <= RAM_MPOOL0_MAX_NUM))
		;

	for (zone = 0; zone < 1; zone++) {
		freepage_num[zone] = pagepool_size[zone];
		for (i = 0; i < pagepool_size[zone]; i++) {
			pagepool[zone][i].ibank_no = -1;
			index = (zone << 7) | i;
			pagepool_freelist_add(index);
		}
	}

    buddy_rom_data_init();

	return 0;
}
