/*
 * Copyright (c) 2020, Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 ******************************************************************************
 * @file    jpeg_parser.c
 * @brief   JPEG HAL module driver.
 *          This file provides firmware functions to manage the following
 *          functionalities of the JPEG HW decoder peripheral:
 *           + Initialization and de-initialization functions
 *           + IO operation functions
 *           + Peripheral Control functions
 *           + Peripheral State and Errors functions
 *
 */
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <jpeg_parser.h>

const int exiftag = 0x66697845; //'e''x''i''f'

const uint8_t zigzag[64]=
{   0, 1, 8,16, 9, 2, 3,10,
   17,24,32,25,18,11, 4, 5,
   12,19,26,33,40,48,41,34,
   27,20,13, 6, 7,14,21,28,
   35,42,49,56,57,50,43,36,
   29,22,15,23,30,37,44,51,
   58,59,52,45,38,31,39,46,
   53,60,61,54,47,55,62,63
};

int JFREAD(jpeg_parser_info_t *parser_info, void *buf, int len)
{
	if (parser_info->jpeg_current_offset + len > parser_info->jpeg_size) {
		len = parser_info->jpeg_size - parser_info->jpeg_current_offset;
	}

	memcpy(buf, &parser_info->jpeg_base[parser_info->jpeg_current_offset], len);
	parser_info->jpeg_current_offset += len;
	return len;
}

#define JFTELL(parser_info)  parser_info->jpeg_current_offset 
#define JFOFFSET(parser_info)  parser_info->jpeg_current_offset

/**********************************************************
* get one bytes from jpeg
***********************************************************
**/
static inline uint8_t _jpeg_parser_getbyte(struct jpeg_parser_info *parser_info)
{
	uint8_t rval =0;
	if (JFREAD(parser_info, &rval, 1) == 0) {
		parser_info->nodata = 1;
	}
	return rval;
}

/**********************************************************
* get two bytes from jpeg
***********************************************************
**/
static inline int _jpeg_parser_get2bytes(struct jpeg_parser_info *parser_info)
{
	int len = (_jpeg_parser_getbyte(parser_info)<<8);
	len |= (int)_jpeg_parser_getbyte(parser_info);
	return len;
}

static inline int _jpeg_parser_get2bytesL(struct jpeg_parser_info *parser_info)
{
	int len = _jpeg_parser_getbyte(parser_info);
	len |= (_jpeg_parser_getbyte(parser_info)<<8);
	return len;
}

/**********************************************************
* get four bytes from jpeg
***********************************************************
**/
static inline int _jpeg_parser_get4bytes(struct jpeg_parser_info *parser_info)
{
	int len = (_jpeg_parser_getbyte(parser_info)  << 24);
	len |= (_jpeg_parser_getbyte(parser_info) << 16);
	len |= (_jpeg_parser_getbyte(parser_info) << 8);
	len |= (int)_jpeg_parser_getbyte(parser_info);
	return len;
}

static inline int _jpeg_parser_get4bytesL(struct jpeg_parser_info *parser_info)
{
	int len = _jpeg_parser_getbyte(parser_info);
	len |= (_jpeg_parser_getbyte(parser_info) << 8);
	len |= (_jpeg_parser_getbyte(parser_info) << 16);
	len |= (_jpeg_parser_getbyte(parser_info) << 24);

	return len;
}

/**********************************************************
*	skip len data
***********************************************************
**/
static int _jpeg_parser_skipbytes(struct jpeg_parser_info *parser_info, uint32_t len)
{
	#define TMP_BUF_LEN 128
	uint8_t tmpbuf[TMP_BUF_LEN];
	int rnum =0;

	while(len > 0) {
		if (len > TMP_BUF_LEN) 	{
			rnum = JFREAD(parser_info, tmpbuf, TMP_BUF_LEN);
			if (rnum != TMP_BUF_LEN) {
				parser_info->nodata = 1;
				break;
			}
			len -= TMP_BUF_LEN;
		} else {
			rnum = JFREAD(parser_info, tmpbuf, len);
			if (rnum != len) {
				parser_info->nodata = 1;
			}
			len = 0;
		}
	}
	return 0;
}

static int _jpeg_parser_get_data(struct jpeg_parser_info *parser_info, uint8_t *buf , uint32_t len)
{
	int rnum = JFREAD(parser_info, buf, len);
	if (rnum != len) {
		parser_info->nodata = 1;
	}
	return rnum;
}

static int _jpeg_parser_mark_skip(struct jpeg_parser_info *parser_info)
{
	int section_len =  _jpeg_parser_get2bytes(parser_info) - 2;

	_jpeg_parser_skipbytes(parser_info, section_len);

	return EN_NORMAL;
}

/**********************************************************
*	jpeg deal for 0xc0 mark
***********************************************************
**/
static int _jpeg_parser_sof0(struct jpeg_parser_info *parser_info)
{
	struct jpeg_info_t *jpeg_info = &parser_info->jpeg_info;
	int sof_len=0;
	int i=0;
	int w=0;
	int h=0;
	int Nnum=0;
	int HV=0;
	int H[3] = {0};
	int V[3]= {0};

	sof_len = _jpeg_parser_get2bytes(parser_info);

	_jpeg_parser_getbyte(parser_info);

	//x,y
	h = _jpeg_parser_get2bytes(parser_info);
	w = _jpeg_parser_get2bytes(parser_info);
	Nnum = _jpeg_parser_getbyte(parser_info);

	if (Nnum > 3) {
		return EN_NOSUPPORT;
	}

	sof_len -= 8;

	for (i = 0; i < Nnum; i++) {
		_jpeg_parser_getbyte(parser_info);
		HV = _jpeg_parser_getbyte(parser_info);
		H[i] = HV >> 4;
		V[i] = HV & 0xf;
		_jpeg_parser_getbyte(parser_info);
		sof_len -= 3;
	}

	jpeg_info->yuv_mode = YUV_OTHER;

	if ((H[0]==1)&&(V[0]==1)&&(Nnum == 1))
	{
		jpeg_info->yuv_mode = YUV100;
	}
	if ((H[0]==2)&&(V[0]==2)&&(H[1]==1)&&(V[1]==1)&&(H[2]==1)&&(V[2]==1))
	{
		jpeg_info->yuv_mode = YUV420;
	}
	if ((H[0]==2)&&(V[0]==2)&&(H[1]==1)&&(V[1]==2)&&(H[2]==1)&&(V[2]==2))
	{
		jpeg_info->yuv_mode = YUV422;
	}
	if ((H[0]==2)&&(V[0]==1)&&(H[1]==1)&&(V[1]==1)&&(H[2]==1)&&(V[2]==1))
	{
		jpeg_info->yuv_mode = YUV211H;
	}
	if ((H[0]==1)&&(V[0]==2)&&(H[1]==1)&&(V[1]==1)&&(H[2]==1)&&(V[2]==1))
	{
		jpeg_info->yuv_mode = YUV211V;
	}
	if ((H[0]==1)&&(V[0]==1)&&(H[1]==1)&&(V[1]==1)&&(H[2]==1)&&(V[2]==1))
	{
		jpeg_info->yuv_mode = YUV111;
	}

	if (sof_len > 0)
	{
		_jpeg_parser_skipbytes(parser_info, sof_len);
	}

	jpeg_info->image_w = w;
	jpeg_info->image_h = h;

	return EN_NORMAL;
}

/**********************************************************
*	jpeg deal for 0xe1 mark
***********************************************************/
static int _jpeg_parser_app1(struct jpeg_parser_info *parser_info)
{
	struct jpeg_info_t *jpeg_info = &parser_info->jpeg_info;
	int app1_len =0;
	int idf_entry=0;
	int cur_offset=0;
	int idf1_offset=0;
	int i=0;
	int tag=0;
	int start_file_pos=0;
	int thumbnail_pos=0;
	int DateTimeValueOffset=0;
	int byteorder = 0;
	int (*get2b)(struct jpeg_parser_info *) = 0;
	int (*get4b)(struct jpeg_parser_info *) = 0;
	int ifdoffset = 0;
	cur_offset = 0;

	//len
	app1_len = _jpeg_parser_get2bytes(parser_info);

	if (app1_len < 6) {		
	    return EN_NORMAL;
	}

	//exif
	tag = _jpeg_parser_get4bytesL(parser_info);

	if ((tag != exiftag) || (app1_len <= 8)) {
		_jpeg_parser_skipbytes(parser_info, app1_len - 2 - 4);
		//printk("tag %x  exiftag %x \n",tag,exiftag);
		return EN_NORMAL;
	}

	//00 00
	_jpeg_parser_skipbytes(parser_info, 2);
	//offset start
	start_file_pos = JFTELL(parser_info);

	byteorder = _jpeg_parser_get2bytes(parser_info);
	if (byteorder == 0x4d4d) {
		get2b = _jpeg_parser_get2bytes;			//big
		get4b = _jpeg_parser_get4bytes;
	} else {
		get2b = _jpeg_parser_get2bytesL;			//little
		get4b = _jpeg_parser_get4bytesL;
	}

	//_jpeg_parser_skipbytes(pjpg, 6);
	_jpeg_parser_skipbytes(parser_info, 2);
	ifdoffset = get4b(parser_info);

	if (ifdoffset != 0x08) {
		_jpeg_parser_skipbytes(parser_info, app1_len - 8 - 8);
		//printk("ifdoffset %x \n",ifdoffset);
		return EN_NORMAL;
	}

	//IDF0
	idf_entry = get2b(parser_info);

	for (i = 0;i < idf_entry; i++) {
		tag = get2b(parser_info);
		if (tag == 0x0132) 	{
			_jpeg_parser_skipbytes(parser_info, 6);
			DateTimeValueOffset = get4b(parser_info);
			continue;
		}
		_jpeg_parser_skipbytes(parser_info, 10);
	}

	cur_offset += 10 + idf_entry * 12;

	idf1_offset = get2b(parser_info);
	cur_offset += 2;

	if (idf1_offset == 0)	//some pic have padding???
	{
		idf1_offset = get2b(parser_info);
		cur_offset += 2;

		if (idf1_offset == 0) {
			_jpeg_parser_skipbytes(parser_info, app1_len - (cur_offset + 8));
			//printk("ifdoffset %x \n",idf1_offset);
			return EN_NORMAL;
		}
	}

	//IDF1
	if ((DateTimeValueOffset == 0) || (DateTimeValueOffset > idf1_offset)) {
		_jpeg_parser_skipbytes(parser_info, idf1_offset - cur_offset);
	} else	{
		_jpeg_parser_skipbytes(parser_info, DateTimeValueOffset - cur_offset);
		//get time
		_jpeg_parser_get_data(parser_info, jpeg_info->Date, 20);

		_jpeg_parser_skipbytes(parser_info, idf1_offset - DateTimeValueOffset - 20);
	}

	cur_offset = idf1_offset;

	idf_entry = get2b(parser_info);

	for(i = 0;i < idf_entry; i++) {
		tag = get2b(parser_info);

		if (tag == 0x0201) {
			_jpeg_parser_skipbytes(parser_info, 6);
			thumbnail_pos = get4b(parser_info);
			continue;
		}
		_jpeg_parser_skipbytes(parser_info, 10);
	}

	cur_offset += 2 + (idf_entry) * 12;

	if (DateTimeValueOffset > idf1_offset) {
		_jpeg_parser_skipbytes(parser_info, DateTimeValueOffset - cur_offset);
		//get time
		_jpeg_parser_get_data(parser_info, jpeg_info->Date, 20);
		app1_len -= (DateTimeValueOffset + 20 + 8);
	} else {
		app1_len -= (idf1_offset + 8);
		app1_len -= (2 + (idf_entry)*12);
	}

	if (app1_len > 0) {
	    _jpeg_parser_skipbytes(parser_info, app1_len);
	}

	parser_info->thumbnailoffset = thumbnail_pos + start_file_pos;

	//start_file_pos = JFTELL(parser_info);
	return EN_NORMAL;
}
/**********************************************************
*	jpeg deal for 0xc4 mark
***********************************************************
**/
static int _jpeg_parser_dht(struct jpeg_parser_info *parser_info)
{
	struct jpeg_info_t *jpeg_info = &parser_info->jpeg_info;
	uint8_t *curtable=0;
	uint8_t *curvaltable=0;
	int dht_len=0;
	int len=0;
	int i=0;
	uint8_t tcth=0;

	dht_len = _jpeg_parser_get2bytes(parser_info);

	dht_len -= 2;
	//get len
	while(dht_len > 16) {
		//len i
		tcth = _jpeg_parser_getbyte(parser_info);

		if (tcth & 0xf0) {
			if (tcth & 0x0f) {
				curtable    = &jpeg_info->AC_TAB1[0];
				curvaltable = (uint8_t*)ACHuf_1;
			} else {
				curtable = &jpeg_info->AC_TAB0[0];
				curvaltable = (uint8_t*)ACHuf_0;
			}
		} else {
			if (tcth & 0x0f) {
				curtable = &jpeg_info->DC_TAB1[0];
				curvaltable = (uint8_t*)DCHuf_1;
			} else {
				curtable = &jpeg_info->DC_TAB0[0];
				curvaltable = (uint8_t*)DCHuf_0;
			}
		}

		_jpeg_parser_get_data(parser_info, curtable, 16);

		len = 0;
		for(i = 0;i < 16;i++) {
			len += curtable[i];
		}
		//val i
		_jpeg_parser_get_data(parser_info, curvaltable, len);

		dht_len -= (len + 17);
	}

	if (dht_len) {
		_jpeg_parser_skipbytes(parser_info, dht_len);
	}
#if 0
	printk("ACHuf_0 table: \n");
	for(i = 0; i < 64; i++){
		printk("0x%x ",*(uint8_t *)(ACHuf_0 + i));
	}
	printk("\n");
	printk("ACHuf_1 table: \n");
	for(i = 0; i < 64; i++){
		printk("0x%x ",*(uint8_t *)(ACHuf_1 + i));
	}
	printk("\n");
	printk("DCHuf_0 table: \n");
	for(i = 0; i < 64; i++){
		printk("0x%x ",*(uint8_t *)(DCHuf_0 + i));
	}
	printk("\n");
	printk("DCHuf_1 table: \n");
	for(i = 0; i < 64; i++){
		printk("0x%x ",*(uint8_t *)(DCHuf_1 + i));
	}
	printk("\n");
#endif
	return EN_NORMAL;
}

/*
len: 2bytes
table 0-3
{
   tq
   V[64] ()
}
*/
/**********************************************************
*	jpeg deal for 0xdb mark
***********************************************************
**/
static int _jpeg_parser_dqt(struct jpeg_parser_info *parser_info)
{
	struct jpeg_info_t *jpeg_info = &parser_info->jpeg_info;
	uint8_t *curtable=0;
	int dqt_len=0;
	int i=0;
	uint8_t tq=0;

	dqt_len = _jpeg_parser_get2bytes(parser_info);

	dqt_len -= 2;
	//get len
	while(dqt_len > 16) {
		//len i
		tq = _jpeg_parser_getbyte(parser_info);

		if (tq == 0x0) {
			curtable = (uint8_t*)QT_0;
		} else if (tq == 0x1) {
			curtable = (uint8_t*)QT_1;
		} else {
			curtable = (uint8_t*)QT_2;
		}
		//len = 0;
	    for (i = 0; i < 64; i++) {
			curtable[zigzag[i]]= _jpeg_parser_getbyte(parser_info);
	    }

		dqt_len -= 65;

		jpeg_info->getQTablenum++;

		if (tq == 0x1) {
			memcpy((char*)QT_2, (const char*)QT_1, 64);
		}
	}

	if (dqt_len) {
		_jpeg_parser_skipbytes(parser_info, dqt_len);
	}
#if 0
	printk("QT_0 table: tq %d \n",tq);
	for(i = 0; i < 64; i++){
		printk("0x%x ",*(uint8_t *)(QT_0 + i));
	}
	printk("\n");
	printk("QT_1 table: \n");
	for(i = 0; i < 64; i++){
		printk("0x%x ",*(uint8_t *)(QT_1 + i));
	}
	printk("\n");
	printk("QT_2 table: \n");
	for(i = 0; i < 64; i++){
		printk("0x%x ",*(uint8_t *)(QT_2 + i));
	}
	printk("\n");
#endif
	return EN_NORMAL;
}

/**********************************************************
*	jpeg deal for 0xda mark
***********************************************************
**/
static int _jpeg_parser_sos(struct jpeg_parser_info *parser_info)
{
	struct jpeg_info_t *jpeg_info = &parser_info->jpeg_info;
	int sos_len=0;
	int i=0;
	unsigned int tmp=0;
	unsigned int cIndex=0;

	sos_len = _jpeg_parser_get2bytes(parser_info);

	sos_len -= 2;
	jpeg_info->scan.Ns = _jpeg_parser_getbyte(parser_info);

	if (jpeg_info->scan.Ns > 3) {
		return EN_NOSUPPORT;
	}

	if (jpeg_info->scan.Ns == 1) {
		tmp = _jpeg_parser_getbyte(parser_info);
		cIndex = tmp - 1;

		if (cIndex > 2)	{
			return EN_NOSUPPORT;
		}

		jpeg_info->scan.Cs[cIndex] = tmp;
		tmp = _jpeg_parser_getbyte(parser_info);

		jpeg_info->scan.Td[cIndex] = tmp >> 4;
		jpeg_info->scan.Ta[cIndex] = tmp & 0x0F;

		/* decoding info */
		jpeg_info->amountOfQTables = 1;
	} else {
		for (i = 0; i < jpeg_info->scan.Ns; i++)	{
			jpeg_info->scan.Cs[i] = _jpeg_parser_getbyte(parser_info);
			tmp = _jpeg_parser_getbyte(parser_info);

			jpeg_info->scan.Td[i] = tmp >> 4;	/* which DC table */
			jpeg_info->scan.Ta[i] = tmp & 0x0F;	/* which AC table */

		}
		jpeg_info->amountOfQTables = 3;
	}
	//ss,se,ahal
	_jpeg_parser_skipbytes(parser_info, 3);

	if ((jpeg_info->amountOfQTables == 3)&&(jpeg_info->getQTablenum == 1)) {
		memcpy((char*)QT_1, (const char*)QT_0, 64);
		memcpy((char*)QT_2, (const char*)QT_1, 64);
	}

	jpeg_info->stream_addr = &parser_info->jpeg_base[parser_info->jpeg_current_offset];
	jpeg_info->stream_size = parser_info->jpeg_size - parser_info->jpeg_current_offset;

	return EN_NORMAL;
}

/**********************************************************
* get mark from jpeg
***********************************************************
**/
static int _jpeg_parser_get_mark(struct jpeg_parser_info *parser_info)
{
	uint8_t tag=0;

	do {
		do 	{
			if (parser_info->nodata) {
				return M_NODATA;
			}
			tag = _jpeg_parser_getbyte(parser_info);
		} while(tag != 0xff);

		while(tag == 0xff) {
			if (parser_info->nodata) {
				return M_NODATA;
			}
			tag = _jpeg_parser_getbyte(parser_info);
		}

		if (tag != 0) {
			break;
		}
	}while(1);

	return tag;
}

const jpeg_marker_handle_t jpeg_rout[]=
{
	{M_SOF0, _jpeg_parser_sof0},//0xc0
	//{M_APP0, _jpeg_parser_app0},//0xe0
	{M_APP1, _jpeg_parser_app1},//0xe1
	{M_DHT,  _jpeg_parser_dht},//0xc4
	{M_DQT,  _jpeg_parser_dqt},//0xdb
	{M_SOS,  _jpeg_parser_sos},//0xda
};


/**********************************************************
* parser jpeg info
***********************************************************
**/
int jpeg_parser_process(struct jpeg_parser_info *parser_info, int mode)
{
	jpeg_marker_handle_t *mark_handle = NULL;
	uint8_t tag = 0;
	int i = 0;
	int rts = EN_NORMAL;

	for(;;) {
		if (parser_info->nodata)  {
			break;
		}
		//get mark
		tag = _jpeg_parser_get_mark(parser_info);
		//printk("tag: %x \n",tag);
		switch (tag)	{
			case M_SOF1:                      // 0xc1
			case M_SOF2:                      // 0xc2
			case M_SOF3:                      // 0xc3
			case M_SOF9:                      // 0xc9
			case M_SOF10:                     // 0xca
			case M_SOF11:	                  // 0xcb
				return EN_NOSUPPORT;
				//break;
			case M_SOI:                       // 0xd8
				continue;
			default:
				break;
		}

		mark_handle = NULL;

		for (i = 0; i < 5; i++) {
			if (tag == jpeg_rout[i].marker) {
				mark_handle = (jpeg_marker_handle_t *)&jpeg_rout[i];
			}
		}

		if (mark_handle) {
			rts = (rt_status_t)mark_handle->marker_pro((void *)parser_info);
			if (rts || (parser_info->thumbnailoffset != 0)) {
				return rts;
			}
		} else	{
			_jpeg_parser_mark_skip((void *)parser_info);
		}

		if (tag == M_SOS) {
			return 0;
		}
	}
	// da end
	return 0;
}
